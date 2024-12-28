#include "engine/experiments/registry/rhijack.hpp"
#include "cli/ui.hpp"
#include "engine/engine.hpp"
#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/aspa/aspa.hpp"
#include "plugins/manager.hpp"
#include <fstream>
#include <iomanip>
#include <memory>
#include <numeric>
#include <thread>

RouteHijackExperiment::RouteHijackExperiment(std::queue<Trial> &input_queue,
                                             std::queue<double> &output_queue,
                                             std::shared_ptr<Topology> topology,
                                             std::string deploymentType)
    : ExperimentWorker(input_queue, output_queue, nullptr), // Notice we pass nullptr here
      deploymentType_(deploymentType) {
  // Set the attacker hops to 1 by default
  attackerHops_ = 1;
  const double step = 10.0;
  matrix_size = static_cast<size_t>(100 / step) + 1;
  results = std::vector<std::vector<double>>(matrix_size, std::vector<double>(matrix_size, 0.0));
  display = std::make_unique<ProgressDisplay>();

  // Load the CAIDA topology during construction
  if (!setupTopology()) {
    throw std::runtime_error("Failed to initialize CAIDA topology");
  }
}

double RouteHijackExperiment::calculateAttackerSuccessRate(std::shared_ptr<Router> attacker,
                                                           std::shared_ptr<Router> victim) {
  if (!attacker || !victim) {
    return 0.0;
  }

  size_t bad_routes = 0;
  size_t total_routes = 0;

  // Iterate through all ASes in the topology
  for (const auto &[id, router] : topology_->G->nodes) {
    // Check if this AS has a route to the victim
    auto route_it = router->routerTable.find(victim->ASNumber);
    if (route_it != router->routerTable.end() && route_it->second) {
      total_routes++;

      // Find attacker in path
      auto &path = route_it->second->path;
      auto it = std::find(path.begin(), path.end(), attacker.get());

      // Check if attacker is in path and victim is right before it
      if (it != path.end() && it != path.begin()) {
        if (*(it - 1) == victim.get()) {
          bad_routes++;
        }
      }
    }
  }

  // Return ratio of compromised routes (without multiplying by 100)
  return total_routes > 0 ? static_cast<double>(bad_routes) / total_routes : 0.0;
}

void RouteHijackExperiment::initializeTrial() {
  int numberOfTrials = 100;

  // Ensure the topology is initialized
  if (!topology_ || topology_->G->nodes.empty()) {
    throw std::runtime_error("Topology not properly initialized");
  }

  // Check if the topology has enough routers
  size_t totalRouters = topology_->G->nodes.size();
  if (totalRouters < 2) {
    throw std::runtime_error("Not enough routers in topology for trials");
  }

  try {
    for (int i = 0; i < numberOfTrials; ++i) {
      // Sample two distinct routers
      auto sampledRouters = topology_->RandomSampleRouters(2);

      // In rare cases, sampling might return fewer routers
      if (sampledRouters.size() < 2) {
        throw std::runtime_error("Failed to sample enough routers for trial " +
                                 std::to_string(i + 1));
      }

      // Create and enqueue the trial
      Trial t;
      t.victim = sampledRouters[0];
      t.attacker = sampledRouters[1];
      input_queue_.push(t);
    }
  } catch (const std::exception &e) {
    throw; // Re-throw the exception after stopping the spinner
  }
}

bool RouteHijackExperiment::setupTopology() {
  try {
    Spinner spinner;
    spinner.start();
    std::cout << "\nLoading CAIDA topology...\n";

    Parser parser;
    auto rpki = std::make_shared<RPKI>();
    auto relations = parser.GetAsRelationships("/home/yigit/workspace/github/rstk-worktree/"
                                               "rstk-refactor/data/caida/20151201.as-rel2.txt");

    std::shared_ptr<Topology> new_topology = nullptr;
    while (true) {
      try {
        new_topology = std::make_shared<Topology>(relations, rpki);
        break;
      } catch (...) {
        // Optionally, log the exception or handle it
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    topology_ = new_topology;
    topology_->TopologyName = "CAIDA_2014_12_01";

    Engine::Instance().updateTopology(topology_);

    spinner.stop();
    std::cout << "Loaded CAIDA topology with " << topology_->G->nodes.size() << " relationships\n";

    return true;

  } catch (const std::exception &e) {
    std::cout << "Failed to load topology: " << e.what() << "\n";
    return false;
  }
}

size_t RouteHijackExperiment::calculateTotalTrials() const { return input_queue_.size(); }

double RouteHijackExperiment::runTrial(const Trial &trial) {
  if (!topology_)
    throw std::runtime_error("Topology not initialized");

  topology_->clearDeployment();
  topology_->clearRoutingTables();

  // Fix the deployment strategy creation
  std::unique_ptr<DeploymentStrategy> strategy;
  if (deploymentType_ == "random") {
    strategy = std::make_unique<RandomDeployment>(objectDeployment_, policyDeployment_);
  } else {
    strategy = std::make_unique<SelectiveDeployment>(objectDeployment_, policyDeployment_);
  }

  topology_->setDeploymentStrategy(std::move(strategy));
  topology_->deploy();

  if (!trial.victim || !trial.attacker)
    return 0.0;

  trial.attacker->proto = ProtocolFactory::Instance().CreateProtocol(trial.attacker->ASNumber);
  topology_->FindRoutesTo(trial.victim.get());
  topology_->Hijack(trial.victim.get(), trial.attacker.get(), attackerHops_);

  return calculateAttackerSuccessRate(trial.attacker, trial.victim);
}

void RouteHijackExperiment::run() {
  const double step = 10.0;
  size_t matrix_size = static_cast<size_t>(100 / step) + 1;
  size_t total_configs = matrix_size * matrix_size;
  size_t current_config = 0;

  // Initialize progress displays
  ProgressDisplay display;

  for (double obj_pct = 0; obj_pct <= 100; obj_pct += step) {
    for (double pol_pct = 0; pol_pct <= 100; pol_pct += step) {
      objectDeployment_ = obj_pct;
      policyDeployment_ = pol_pct;
      current_config++;

      // Update matrix progress
      double matrix_progress = (static_cast<double>(current_config) / total_configs) * 100;
      display.updateMatrixProgress(matrix_progress, obj_pct, pol_pct);

      // Capture the number of trials before popping from the queue
      size_t total_trials_for_this_config = input_queue_.size();
      size_t trial_count = 0;
      std::vector<double> trial_results;

      while (!input_queue_.empty() && !stopped_) {
        Trial trial = input_queue_.front();
        input_queue_.pop();
        trial_count++;

        double result = runTrial(trial);
        trial_results.push_back(result);

        // Use total_trials_for_this_config instead of input_queue_.size()
        double trial_progress =
            (static_cast<double>(trial_count) / total_trials_for_this_config) * 100.0;
        display.updateTrialProgress(trial_progress, result, trial.victim->ASNumber,
                                    trial.attacker->ASNumber);
      }

      double avg = !trial_results.empty()
                       ? std::accumulate(trial_results.begin(), trial_results.end(), 0.0) /
                             trial_results.size()
                       : 0.0;

      size_t i = static_cast<size_t>(obj_pct / step);
      size_t j = static_cast<size_t>(pol_pct / step);
      results[i][j] = avg;

      initializeTrial();
    }
  }

  std::string filename = "hijack_matrix_" + deploymentType_ + ".csv";
  std::ofstream out(filename);
  for (const auto &row : results) {
    for (size_t i = 0; i < row.size(); i++) {
      out << row[i];
      if (i < row.size() - 1)
        out << ",";
    }
    out << "\n";
  }

  auto &engine = Engine::Instance();
  engine.updateExperimentProgress(matrix_size * matrix_size);
  engine.setState(EngineState::INITIALIZED);
}
