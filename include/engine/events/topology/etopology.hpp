#pragma once
#ifndef ETOP_HPP
#define ETOP_HPP

#include "engine/ebus.hpp"
#include "engine/topology/topology.hpp"

class TopologyLoadedEvent : public BaseEvent {
public:
  explicit TopologyLoadedEvent(std::shared_ptr<topology> t) { setData("topology", t); }
  std::type_index getType() const override { return std::type_index(typeid(TopologyLoadedEvent)); }
  std::string getName() const override { return "TopologyLoaded"; }
};

// Event when topology is ready for experiments
class TopologyReadyEvent : public BaseEvent {
public:
  explicit TopologyReadyEvent(std::shared_ptr<topology> topology) { setData("topology", topology); }
  std::type_index getType() const override { return std::type_index(typeid(TopologyReadyEvent)); }
  std::string getName() const override { return "TopologyReady"; }
};

#endif // ETOP_HPP
