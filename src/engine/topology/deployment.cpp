// DeploymentStrategy.cpp
#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"
#include <stdexcept>

void DeploymentStrategy::deploy(Topology &topology) { throw std::logic_error("deploy() not implemented"); }

void DeploymentStrategy::clear(Topology &topology) { throw std::logic_error("clear() not implemented"); }

bool DeploymentStrategy::validate(const Topology &topology) const {
  if (topology.G->nodes.empty()) {
    return false;
  }
  return true;
}
