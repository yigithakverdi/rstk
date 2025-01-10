#pragma once
#ifndef EXPMODULE_HPP
#define EXPMODULE_HPP

#include "engine/ebus.hpp"
#include "engine/events/experiments/eexp.hpp"
#include "engine/events/topology/etopology.hpp"
#include "engine/experiments/experiments.hpp"
#include "engine/experiments/register.hpp"
#include "engine/pipeline/pipeline.hpp"
#include "engine/topology/topology.hpp"

class ExperimentModule : public IModule {
public:
  void initialize(std::shared_ptr<ebus> bus) override;
  std::string name() const override;
  bool isReady() const override;
  void cleanup() override;

  std::shared_ptr<topology> getTopology() const { return topology_; }
  std::shared_ptr<IExperiment> getCurrentExperiment() const { return currexp_; }
  std::shared_ptr<ebus> getBus() const { return bus_; }

private:
  void handleTopologyReady(std::shared_ptr<IEvent> evt);
  void handleExperimentStart(std::shared_ptr<IEvent> evt);

  std::shared_ptr<topology> topology_;
  std::shared_ptr<IExperiment> currexp_;
  std::shared_ptr<ebus> bus_;
  bool initialized_{false};
};

#endif // MODULE_HPP
