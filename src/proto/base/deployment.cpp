#include "proto/base/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "proto/base/base.hpp"

BaseDeployment::BaseDeployment(topology &t) : IDeployment(t), t_(t) {}
void BaseDeployment::deploy(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    if (!router->getProtocol()) {
      router->setProtocol(std::make_unique<BaseProtocol>());
    }
  }
}

bool BaseDeployment::validate(topology &t) { return true; }
void BaseDeployment::clear(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    router->setProtocol(std::make_unique<BaseProtocol>());
  }
}
