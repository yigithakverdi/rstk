#ifndef RLEAK_HPP
#define RLEAK_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include <memory>
#include <queue>

class RouteLeakExperiment : public ExperimentWorker {
public:
  RouteLeakExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                      std::shared_ptr<Topology> topology, std::string deploymentType);

  // Experiment specific measurement methods
  double calculateRouteLeakSuccess(std::shared_ptr<Router> attacker,
                                   std::shared_ptr<Router> victim);
  Router *findLeakedRoute(const Route *route);

protected:
  void initializeTrial() override;
  size_t calculateTotalTrials() const override;
  double runTrial(const Trial &trial) override;
  bool setupTopology() override;
  void run() override;

private:
  double objectDeployment_;
  double policyDeployment_;
  std::string deploymentType_;
  int victimAS_;
  int attackerAS_;
  int attackerHops_;
};

#endif // RCAIDA_HPP
