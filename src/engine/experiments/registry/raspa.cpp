#include "engine/experiments/registry/raspa.hpp"
#include "engine/experiments/experiments.hpp"
#include "plugins/aspa/aspa.hpp"
#include "plugins/manager.hpp"
#include <sstream>

ASPADeploymentExperiment::ASPADeploymentExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                                                   std::shared_ptr<Topology> topology, double objectDeployment,
                                                   double policyDeployment)
    : ExperimentWorker(input_queue, output_queue, topology), objectDeployment_(objectDeployment),
      policyDeployment_(policyDeployment), victimAS_(0), attackerAS_(0) // Initialize if necessary
{
  // Constructor body (if needed)
}

void ASPADeploymentExperiment::initializeTrial() {
  Trial t;
  t.victim = topology_->G->nodes[1];
  t.attacker = topology_->G->nodes[2];
  input_queue_.push(t);
}

size_t ASPADeploymentExperiment::calculateTotalTrials() const {
  // Example: one trial per combination of tier1 and tier2 ASes
  return input_queue_.size();
}

double ASPADeploymentExperiment::runTrial(const Trial &trial) {
  auto strategy = std::make_unique<ASPADeployment>(objectDeployment_, policyDeployment_);
  
  topology_->setDeploymentStrategy(std::move(strategy));
  topology_->deploy();

  trial.attacker->proto = ProtocolFactory::Instance().CreateProtocol(trial.attacker->ASNumber);
  topology_->FindRoutesTo(trial.victim.get());
  topology_->Hijack(trial.victim.get(), trial.attacker.get(), 1);

  return 0.0;
}
