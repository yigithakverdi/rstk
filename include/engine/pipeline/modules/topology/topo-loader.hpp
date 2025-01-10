#pragma once
#ifndef TOPOMODULE_HPP
#define TOPOMODULE_HPP

#include "engine/ebus.hpp"
#include "engine/pipeline/pipeline.hpp"
#include "engine/topology/topology.hpp"
#include <memory>

class TopologyModule : public IModule {
public:
  void initialize(std::shared_ptr<ebus> bus) override;
  void cleanup() override;
  std::string name() const override;
  bool isReady() const override;
  void debugPrint() const;

  void setTopologyConfig(const std::string &config) { topologyConfig_ = config; }
  std::string getTopologyConfig() const { return topologyConfig_; }
  std::shared_ptr<topology> getTopology() const { return currentTopology_; }

private:
  void handleLoadTopology(std::shared_ptr<IEvent> evt);
  void handleTopologyBuilt(std::shared_ptr<topology> topo);
  bool validateTopology(std::shared_ptr<topology> topo);

  std::shared_ptr<ebus> bus_;
  std::shared_ptr<topology> currentTopology_;
  bool initialized_{false};
  std::string topologyConfig_;
};

#endif // MODULE_HPP
