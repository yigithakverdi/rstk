#ifndef ASPADEPLOYMENTEXPERIMENT_HPP
#define ASPADEPLOYMENTEXPERIMENT_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include <memory>
#include <queue>

class ASPADeploymentExperiment : public ExperimentWorker {
public:
  ASPADeploymentExperiment(std::queue<Trial> &input_queue, std::queue<double> &output_queue,
                           std::shared_ptr<Topology> topology, double objectDeployment, double policyDeployment);

protected:
  void initializeTrial() override;
  size_t calculateTotalTrials() const override;
  double runTrial(const Trial &trial) override;

private:
  double objectDeployment_;
  double policyDeployment_;
  int victimAS_;
  int attackerAS_;
};

#endif // ASPADEPLOYMENTEXPERIMENT_HPP
