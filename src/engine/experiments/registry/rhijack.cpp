#include "engine/experiments/registry/rhijack.hpp"
#include "plugins/base/base.hpp"

RouteHijackExperiment::RouteHijackExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                                             std::shared_ptr<Topology> topology, int n_hops)
    : ExperimentWorker(input_queue, output_queue, topology), n_hops_(n_hops) {
  // Constructor body (if needed)
}

double RouteHijackExperiment::runTrial(const Trial &trial) {
  if (!trial.victim || !trial.attacker) {
    return 0.0;
  }

  auto strategy = std::make_unique<BaseDeploymentStrategy>();
  topology_->setDeploymentStrategy(std::move(strategy));
  topology_->deploy();

  topology_->FindRoutesTo(trial.victim.get());
  topology_->Hijack(trial.victim.get(), trial.attacker.get(), n_hops_);

  return 0.0;
  // return calculateAttackerSuccess(trial.attacker, trial.victim);
}

size_t RouteHijackExperiment::calculateTotalTrials() const { return input_queue_.size(); }

void RouteHijackExperiment::initializeTrial() {
  Trial t;
  t.victim = topology_->G->nodes[1];
  t.attacker = topology_->G->nodes[2];
  input_queue_.push(t);
}
