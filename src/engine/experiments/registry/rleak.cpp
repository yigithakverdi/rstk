#include "engine/experiments/registry/rleak.hpp"
#include "cli/ui.hpp"
#include "engine/engine.hpp"
#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/aspa/aspa.hpp"
#include "plugins/manager.hpp"
#include "router/route.hpp"
#include <fstream>
#include <iomanip>
#include <memory>
#include <numeric>
#include <thread>

RouteLeakExperiment::RouteLeakExperiment(std::queue<Trial> &input_queue,
                                         std::queue<double> &output_queue,
                                         std::shared_ptr<Topology> topology,
                                         std::string deploymentType)
    : ExperimentWorker(input_queue, output_queue, nullptr), // Notice we pass nullptr here
      deploymentType_(deploymentType) {
  // Set the attacker hops to 1 by default

  // Load the CAIDA topology during construction
  if (!setupTopology()) {
    throw std::runtime_error("Failed to initialize CAIDA topology");
  }
}

Router *RouteLeakExperiment::findLeakedRoute(const Route *route) {
  if (!route || route->path.size() < 3) {
    return nullptr;
  }

  // Check each AS in path except origin and destination
  for (size_t i = 1; i < route->path.size() - 1; i++) {
    Router *current_as = route->path[i];
    Router *previous_as = route->path[i - 1];
    Router *next_as = route->path[i + 1];

    // Skip if current is origin or destination
    if (current_as == route->destination || current_as == route->path.front()) {
      continue;
    }

    Relation prev_relation = current_as->GetRelation(previous_as);
    Relation next_relation = current_as->GetRelation(next_as);

    // Case 1: Peer sends route to other peer or upstream
    if (prev_relation == Relation::Peer &&
        (next_relation == Relation::Peer || next_relation == Relation::Provider)) {
      return current_as;
    }

    // Case 2: Downstream sends route to other peer or upstream
    else if (prev_relation == Relation::Provider &&
             (next_relation == Relation::Peer || next_relation == Relation::Provider)) {
      return current_as;
    }
  }

  return nullptr;
}

double RouteLeakExperiment::calculateRouteLeakSuccess(std::shared_ptr<Router> attacker,
                                                      std::shared_ptr<Router> victim) {
  if (!attacker || !victim) {
    return 0.0;
  }

  size_t bad_routes = 0;
  size_t total_routes = 0;

  // Iterate through all ASes in the topology
  for (const auto &[id, router] : topology_->G->nodes) {
    auto route_it = router->routerTable.find(victim->ASNumber);
    if (route_it != router->routerTable.end() && route_it->second) {
      total_routes++;

      // Check for leaked route
      Router *offending_router = findLeakedRoute(route_it->second);
      if (offending_router) {
        bad_routes++;
        if (offending_router != attacker.get()) {
          throw std::runtime_error("Attacker mismatches offending AS");
        }
      }
    }
  }

  // Return percentage of compromised routes
  return total_routes > 0 ? (static_cast<double>(bad_routes) / total_routes) * 100.0 : 0.0;
}

void RouteLeakExperiment::initializeTrial() {
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

  // Create and start the spinner for visual feedback
  Spinner spinner;
  spinner.start();
  std::cout << "\nSampling " << numberOfTrials << " trials...\n";

  try {
    for (int i = 0; i < numberOfTrials; ++i) {
      // Sample two distinct routers
      auto sampledRouters = topology_->RandomSampleRouters(2);

      // In rare cases, sampling might return fewer routers
      if (sampledRouters.size() < 2) {
        spinner.stop();
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
    spinner.stop();
    throw; // Re-throw the exception after stopping the spinner
  }

  // Stop the spinner after sampling is complete
  spinner.stop();
  std::cout << "Sampled " << numberOfTrials << " trials successfully.\n";
}

bool RouteLeakExperiment::setupTopology() {
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

size_t RouteLeakExperiment::calculateTotalTrials() const { return input_queue_.size(); }

double RouteLeakExperiment::runTrial(const Trial &trial) {
  if (!topology_) {
    throw std::runtime_error("Topology not initialized");
  }

  // Making sure everything is clean before starting each trial (it is imperatively done on the
  // `deploy` method though we are calling it related clear methods here again)
  topology_->clearDeployment();    // calls deployment strategy's clear method
  topology_->clearRoutingTables(); // clears routing table of each router in the graph

  if (deploymentType_ == "random") {
    auto strategy = std::make_unique<RandomDeployment>(objectDeployment_, policyDeployment_);
    topology_->setDeploymentStrategy(std::move(strategy));
    topology_->deploy();
  } else if (deploymentType_ == "selecitve") {
    auto strategy = std::make_unique<SelectiveDeployment>(objectDeployment_, policyDeployment_);
    topology_->setDeploymentStrategy(std::move(strategy));
    topology_->deploy();
  } else {
    std::cout << "Wrong choice of deployment type, specify one of these <selective|random>"
              << std::endl;
  }
  if (trial.victim && trial.attacker) {
    trial.attacker->proto = ProtocolFactory::Instance().CreateProtocol(trial.attacker->ASNumber);
    topology_->FindRoutesTo(trial.victim.get());

    double successRate = calculateRouteLeakSuccess(trial.attacker, trial.victim);

    // Use stringstream for better control over formatting
    std::stringstream ss;
    ss << "  Trial: Attacker AS" << trial.attacker->ASNumber << " → Victim AS"
       << trial.victim->ASNumber << " | Success Rate: " << std::fixed << std::setprecision(2)
       << successRate << '%';

    std::cout << ss.str();

    return successRate;
  }
  return 0.0;
}

void RouteLeakExperiment::run() {
  // Matrix size based on 10% steps (can be made configurable if needed)
  const double step = 10.0;
  size_t matrix_size = static_cast<size_t>(100 / step) + 1;
  std::vector<std::vector<double>> results(matrix_size, std::vector<double>(matrix_size, 0.0));

  std::cout << "Starting deployment study...\n";
  ExperimentProgress progress(matrix_size * matrix_size);
  int configCount = 0;

  for (double obj_pct = 0; obj_pct <= 100; obj_pct += step) {
    for (double pol_pct = 0; pol_pct <= 100; pol_pct += step) {
      // Set deployment percentages for this configuration
      objectDeployment_ = obj_pct;
      policyDeployment_ = pol_pct;

      // Clear queues for new configuration
      while (!input_queue_.empty())
        input_queue_.pop();
      while (!output_queue_.empty())
        output_queue_.pop();

      // Run trials for this configuration
      initializeTrial();
      std::vector<double> trial_results;

      while (!input_queue_.empty() && !stopped_) {
        Trial trial = input_queue_.front();
        input_queue_.pop();
        double result = runTrial(trial);
        trial_results.push_back(result);
      }

      // Calculate average success rate
      double avg = !trial_results.empty()
                       ? std::accumulate(trial_results.begin(), trial_results.end(), 0.0) /
                             trial_results.size()
                       : 0.0;

      // Store result
      size_t i = static_cast<size_t>(obj_pct / step);
      size_t j = static_cast<size_t>(pol_pct / step);
      results[i][j] = avg;

      // Update progress display
      progress.update(++configCount);
      std::cout << "\033[3A\r" << std::string(80, ' ') << "\r"
                << "Progress: [" << progress.getBar() << "] " << configCount << "/"
                << (matrix_size * matrix_size) << " (ETA: " << progress.estimateTimeRemaining()
                << ")\n"
                << "Testing " << obj_pct << "% objects, " << pol_pct << "% policy: " << avg
                << "% success\n\n";
    }
  }

  // Save results to CSV
  std::string filename = "leak_matrix_" + deploymentType_ + ".csv";
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
