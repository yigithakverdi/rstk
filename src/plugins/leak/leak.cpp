#include "plugins/leak/leak.hpp"
#include "plugins/base/base.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"

bool LeakPolicyEngine::shouldAcceptRoute(const Route &route) const {
  if (route.ContainsCycle()) {
    return false;
  }
  return true;
}

bool LeakPolicyEngine::shouldPreferRoute(const Route &currentRoute, const Route &newRoute) const {
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

bool LeakPolicyEngine::canForwardRoute(Relation sourceRelation, Relation targetRelation) const {
  return true; // Always forward - route leak
}

std::vector<std::function<int(const Route &)>> LeakPolicyEngine::getPreferenceRules() const {
  return {[this](const Route &route) { return calculateLocalPreference(route); },
          [this](const Route &route) { return calculateASPathLength(route); },
          [this](const Route &route) { return getNextHopASNumber(route); }};
}

int LeakPolicyEngine::calculateLocalPreference(const Route &route) const {
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

int LeakPolicyEngine::calculateASPathLength(const Route &route) const {
  return static_cast<int>(route.path.size());
}

int LeakPolicyEngine::getNextHopASNumber(const Route &route) const {
  if (route.path.size() < 2) {
    return -1;
  }
  Router *nextHopRouter = route.path[route.path.size() - 2];
  return nextHopRouter->ASNumber;
}

LeakProtocol::LeakProtocol() : Protocol(std::make_unique<LeakPolicyEngine>()) {}

std::string LeakProtocol::getProtocolName() const { return "Route Leak Protocol Implementation"; }

std::string LeakProtocol::getDetailedProtocolInfo() const { return getProtocolInfo(); }

std::pair<int, int> LeakProtocol::getDeploymentStats(const Topology & /*topology*/) const {
  return {0, 0};
}
