#include "proto/base/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "proto/base/base.hpp"

BaseDeployment::BaseDeployment(topology &t) : IDeployment(t), t_(t) {}
void BaseDeployment::deploy(topology &t) {
  auto &graph = t.getGraph();
  for (const auto &[id, router] : graph->getNodes()) {
    if (!router->getProtocol()) {
      auto baseProto = std::make_unique<BaseProtocol>();
      router->setProtocol(std::move(baseProto));
    }
  }
}

bool BaseDeployment::validate(topology &t) { return true; }
void BaseDeployment::clear(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    router->setProtocol(std::make_unique<BaseProtocol>());
  }
}
