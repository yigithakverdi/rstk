#include "plugins/base/base.hpp"
#include "plugins/plugins.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"

bool BasePolicyEngine::shouldAcceptRoute(const Route &route) const {
  if (route.ContainsCycle()) {
    return false;
  }
  return true;
}

std::vector<std::function<int(const Route &)>> BasePolicyEngine::getPreferenceRules() const {
  return {[this](const Route &route) { return calculateLocalPreference(route); },
          [this](const Route &route) { return calculateASPathLength(route); },
          [this](const Route &route) { return getNextHopASNumber(route); }};
}

bool BasePolicyEngine::shouldPreferRoute(const Route &currentRoute, const Route &newRoute) const {
  if (currentRoute.destination != newRoute.destination) {
    return false;
  }

  for (const auto &rule : getPreferenceRules()) {
    int currentVal = rule(currentRoute);
    int newVal = rule(newRoute);

    if (currentVal != -1 && newVal != -1) {
      if (newVal < currentVal) {
        return true;
      }
      if (currentVal < newVal) {
        return false;
      }
    }
  }

  return false;
}

bool BasePolicyEngine::canForwardRoute(Relation sourceRelation, Relation targetRelation) const {
  if (sourceRelation == Relation::Customer) {
    return true;
  }

  if (sourceRelation == Relation::Peer || sourceRelation == Relation::Provider) {
    return targetRelation == Relation::Customer;
  }

  return false;
}

int BasePolicyEngine::calculateLocalPreference(const Route &route) const {
  if (route.path.size() < 2) {
    return -1;
  }

  Router *finalRouter = route.destination;
  Router *firstHopRouter = route.path[route.path.size() - 2];

  Relation relation = finalRouter->GetRelation(firstHopRouter);

  if (relation != Relation::Unknown) {
    return static_cast<int>(relation);
  }

  return -1;
}

int BasePolicyEngine::calculateASPathLength(const Route &route) const {
  return static_cast<int>(route.path.size());
}

int BasePolicyEngine::getNextHopASNumber(const Route &route) const {
  if (route.path.size() < 2) {
    return -1;
  }

  Router *nextHopRouter = route.path[route.path.size() - 2];
  return nextHopRouter->ASNumber;
}

BaseProtocol::BaseProtocol() : Protocol(std::make_unique<BasePolicyEngine>()) {}

std::string BaseProtocol::getProtocolName() const { return "Base Protocol Implementation"; }

void BaseDeploymentStrategy::deploy(Topology &topology) {
  clear(topology);

  for (const auto &[id, router] : topology.G->nodes) {
    if (!router->proto || router->proto->getProtocolName() != "Base Protocol Implementation") {
      router->proto = std::make_unique<BaseProtocol>();
    }
  }
}

void BaseDeploymentStrategy::clear(Topology &topology) {
  for (const auto &[id, router] : topology.G->nodes) {
    router->proto = std::make_unique<BaseProtocol>();
    router->routerTable.clear();
  }
}

std::string BaseProtocol::getDetailedProtocolInfo() const { return getProtocolInfo(); }

std::pair<int, int> BaseProtocol::getDeploymentStats(const Topology &topology) const {
  return {0, 0};
}

bool BaseDeploymentStrategy::validate(const Topology &topology) const {
  for (const auto &[id, router] : topology.G->nodes) {
    if (!router->proto || router->proto->getProtocolName() != "Base Protocol Implementation") {
      return false;
    }
  }
  return true;
}
