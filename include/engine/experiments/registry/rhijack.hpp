#ifndef R_HIJACK_HPP
#define R_HIJACK_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include "cli/ui.hpp"
#include <memory>
#include <queue>

class RouteHijackExperiment : public ExperimentWorker {
public:
  RouteHijackExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                        std::shared_ptr<Topology> topology, std::string deploymentType);

  // Experiment specific measurement methods
  double calculateAttackerSuccessRate(std::shared_ptr<Router> attacker,
                                      std::shared_ptr<Router> victim);

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

  std::vector<std::vector<double>> results;
  std::unique_ptr<ProgressDisplay> display;
  size_t matrix_size;
};

#endif // RCAIDA_HPP
