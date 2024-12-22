#include "engine/experiments/registry/rcaida.hpp"
#include "cli/ui.hpp"
#include "engine/engine.hpp"
#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/aspa/aspa.hpp"
#include "plugins/manager.hpp"
#include <thread>

CAIDAExperiment::CAIDAExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                                 std::shared_ptr<Topology> topology, double objectDeployment,
                                 double policyDeployment)
    : ExperimentWorker(input_queue, output_queue, nullptr), // Notice we pass nullptr here
      objectDeployment_(objectDeployment), policyDeployment_(policyDeployment) {

  // Load the CAIDA topology during construction
  if (!setupTopology()) {
    throw std::runtime_error("Failed to initialize CAIDA topology");
  }
}

void CAIDAExperiment::initializeTrial() {
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

bool CAIDAExperiment::setupTopology() {
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

size_t CAIDAExperiment::calculateTotalTrials() const { return input_queue_.size(); }

double CAIDAExperiment::runTrial(const Trial &trial) {
  if (!topology_) {
    throw std::runtime_error("Topology not initialized");
  }

  auto strategy = std::make_unique<ASPADeployment>(objectDeployment_, policyDeployment_);
  topology_->setDeploymentStrategy(std::move(strategy));
  topology_->deploy();

  if (trial.victim && trial.attacker) {
    trial.attacker->proto = ProtocolFactory::Instance().CreateProtocol(trial.attacker->ASNumber);
    topology_->FindRoutesTo(trial.victim.get());
    topology_->Hijack(trial.victim.get(), trial.attacker.get(), 1);
  }

  return 0.0;
}
