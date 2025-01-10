#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"

IDeployment::IDeployment(topology &t) : t_(t) {}
IDeployment::~IDeployment() = default;
void IDeployment::deploy(topology &t) { throw std::runtime_error("deploy() not implemented"); }
void IDeployment::clear(topology &t) { throw std::runtime_error("clear() not implemented"); }

bool IDeployment::validate(topology &t) {
  if (t.getGraph()->getNodes().empty()) {
    return false;
  }
  return true;
}
