// Topology.cpp
#include "engine/topology/topology.hpp"
#include "cli/cli.hpp"
#include "graph/graph.hpp"
#include "logger/verbosity.hpp"
#include "plugins/base/base.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

Topology::Topology(const std::vector<AsRel> &asRelsList, std::shared_ptr<RPKI> rpki)
    : G(std::make_unique<Graph<std::shared_ptr<Router>>>()), RPKIInstance(rpki) {

  for (const AsRel &asRel : asRelsList) {
    if (G->nodes.find(asRel.AS1) == G->nodes.end()) {
      auto source =
          std::make_shared<Router>(asRel.AS1, 1, std::make_unique<BaseProtocol>(), RPKIInstance);
      G->addNode(asRel.AS1, source);
    } else {
    }

    if (G->nodes.find(asRel.AS2) == G->nodes.end()) {
      auto target =
          std::make_shared<Router>(asRel.AS2, 1, std::make_unique<BaseProtocol>(), RPKIInstance);
      G->addNode(asRel.AS2, target);
    } else {
    }
  }

  for (const AsRel &asRel : asRelsList) {
    try {
      double weight = 1.0;
      G->addEdge(asRel.AS1, asRel.AS2, weight);
      G->addEdge(asRel.AS2, asRel.AS1, weight);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error adding edge between " << asRel.AS1 << " and " << asRel.AS2 << ": "
                << e.what() << std::endl;
    }

    assignNeighbors(asRel);
  }

  assignTiers();
}

Topology::~Topology() {}

void Topology::ValidateDeployment() {
  if (!deployment_applied_) {
    throw std::runtime_error("Deployment strategy must be applied before simulation");
  }
}

void Topology::setDeploymentStrategy(std::unique_ptr<DeploymentStrategy> strategy) {
  deploymentStrategy_ = std::move(strategy);
}

void Topology::deploy() {
  if (!deploymentStrategy_) {
    throw std::runtime_error("No deployment strategy set");
  }
  deploymentStrategy_->deploy(*this);
  deployment_applied_ = true;
}

void Topology::clearDeployment() {
  if (deploymentStrategy_) {
    deploymentStrategy_->clear(*this);
  }
  deployment_applied_ = false;
}

void Topology::clearRoutingTables() {
  for (const auto &[id, router] : G->nodes) {
    router->Clear();
  }
}

std::vector<std::shared_ptr<Router>> Topology::GetByCustomerDegree() const {
  // Create vector of routers with their customer counts
  std::vector<std::pair<std::shared_ptr<Router>, size_t>> routerCustomerCounts;

  // For each router, count its customers
  for (const auto &[id, router] : G->nodes) {
    size_t customerCount = router->GetCustomers().size();
    routerCustomerCounts.emplace_back(router, customerCount);
  }

  // Sort routers by customer count in descending order
  std::sort(routerCustomerCounts.begin(), routerCustomerCounts.end(),
            [](const auto &a, const auto &b) {
              return a.second > b.second; // Descending order
            });

  // Extract just the routers in sorted order
  std::vector<std::shared_ptr<Router>> sortedRouters;
  sortedRouters.reserve(routerCustomerCounts.size());
  for (const auto &[router, count] : routerCustomerCounts) {
    sortedRouters.push_back(router);
  }

  return sortedRouters;
}

std::vector<std::shared_ptr<Router>> Topology::GetTierOne() const {
  std::vector<std::shared_ptr<Router>> tierOne;
  for (const auto &[id, router] : G->nodes) {
    if (router->Tier == 1) {
      tierOne.push_back(router);
    }
  }
  return tierOne;
}

std::vector<std::shared_ptr<Router>> Topology::GetTierTwo() const {
  std::vector<std::shared_ptr<Router>> tierTwo;
  for (const auto &[id, router] : G->nodes) {
    if (router->Tier == 2) {
      tierTwo.push_back(router);
    }
  }
  return tierTwo;
}

std::vector<std::shared_ptr<Router>> Topology::GetTierThree() const {
  std::vector<std::shared_ptr<Router>> tierThree;
  for (const auto &[id, router] : G->nodes) {
    if (router->Tier == 3) {
      tierThree.push_back(router);
    }
  }
  return tierThree;
}

std::vector<std::shared_ptr<Router>> Topology::RandomSampleRouters(size_t count) const {
  std::vector<std::shared_ptr<Router>> allRouters;
  for (const auto &[id, router] : G->nodes) {
    allRouters.push_back(router);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(allRouters.begin(), allRouters.end(), gen);

  count = std::min(count, allRouters.size());
  return std::vector<std::shared_ptr<Router>>(allRouters.begin(), allRouters.begin() + count);
}

void Topology::FindRoutesTo(Router *target, VerbosityLevel verbosity) {
  ValidateDeployment();
  std::deque<Route *> routes;

  if (verbosity >= VerbosityLevel::NORMAL) {

    std::cout << "\n═══════════════════════════════════════\n";
    std::cout << "Finding routes to AS" << target->ASNumber << "\n";
    std::cout << "═══════════════════════════════════════\n\n";
  }

  for (const auto &[neighborAS, neighbor] : target->neighbors_) {
    Route *route = target->OriginateRoute(neighbor.router);
    if (route) {
      if (verbosity >= VerbosityLevel::NORMAL) {
        std::cout << "📍 Originating route via AS" << neighbor.router->ASNumber << " ("
                  << relationToString(neighbor.relation) << ")\n";
      }
      routes.push_back(route);
    }
  }

  int pathCount = 0;
  while (!routes.empty()) {
    Route *route = routes.front();
    routes.pop_front();
    std::vector<Router *> neighbors;
    Router *finalRouter = route->path.back();
    if (verbosity == VerbosityLevel::NORMAL) {
      neighbors = finalRouter->LearnRoute(route, VerbosityLevel::NORMAL);
    } else {
      neighbors = finalRouter->LearnRoute(route);
    }

    if (!neighbors.empty()) {
      pathCount++;
      if (verbosity >= VerbosityLevel::NORMAL) {
        std::cout << "\n🔄 Valid path #" << pathCount << " discovered:\n";
        std::cout << "   " << route->ToString() << "\n";
      }
    }

    for (Router *neighbor : neighbors) {
      Route *newRoute = finalRouter->ForwardRoute(route, neighbor);
      if (newRoute) {
        if (verbosity >= VerbosityLevel::NORMAL) {
          std::cout << "📍 AS" << finalRouter->ASNumber << " announcing route to AS"
                    << neighbor->ASNumber << " ("
                    << relationToString(finalRouter->GetRelation(neighbor)) << ")\n";
        }
        routes.push_back(newRoute);
      }
    }
  }

  if (verbosity >= VerbosityLevel::NORMAL) {
    std::cout << "\n✅ Route discovery complete: " << pathCount << " valid paths found\n";
    std::cout << "═══════════════════════════════════════\n\n";
  }
}

void Topology::assignTiers() {
  for (const auto &[id, router] : G->nodes) {
    auto customers = router->GetCustomers();
    auto providers = router->GetProviders();

    if (providers.empty()) {
      router->Tier = 1;
    } else if (customers.empty()) {
      router->Tier = 3;
    } else {
      router->Tier = 2;
    }
  }
}

void Topology::Hijack(Router *victim, Router *attacker, int numberOfHops,
                      VerbosityLevel verbosity) {
  ValidateDeployment();

  if (verbosity >= VerbosityLevel::NORMAL) {
    std::cout << "\n🚨 Simulating hijack attack\n";
    std::cout << "═══════════════════════════════════════\n";
    std::cout << "Attacker: AS" << attacker->ASNumber << "\n";
    std::cout << "Target: AS" << victim->ASNumber << "\n";
    std::cout << "Path length: " << numberOfHops << " hops\n";
    std::cout << "═══════════════════════════════════════\n\n";
  }

  Route *badRoute = CraftRoute(victim, attacker, numberOfHops);
  if (!badRoute) {
    throw std::runtime_error("Failed to craft bad route.");
  }

  if (verbosity >= VerbosityLevel::NORMAL) {
    std::cout << "📡 Broadcasting malicious route:\n";
    std::cout << badRoute->ToString() << "\n\n";
  }

  if (!badRoute) {
    throw std::runtime_error("Failed to craft bad route.");
  }

  std::deque<Route *> routes;

  try {
    const auto &neighbors = G->getNeighbors(attacker->ASNumber);

    for (const Edge &edge : neighbors) {
      int neighborAS = edge.targetNodeId;
      auto neighborRouter = GetRouter(neighborAS);
      if (neighborRouter) {
        Route *forwardedRoute = attacker->ForwardRoute(badRoute, neighborRouter.get());
        if (forwardedRoute) {
          routes.push_back(forwardedRoute);
        }
      }
    }

    while (!routes.empty()) {
      Route *route = routes.front();
      routes.pop_front();

      if (route->path.empty()) {
        std::cerr << "Route has an empty path. Skipping." << std::endl;
        continue;
      }

      Router *finalRouter = route->path.back();
      if (!finalRouter) {
        std::cerr << "Final router in route path is null. Skipping." << std::endl;
        continue;
      }

      std::vector<Router *> learnedNeighbors = finalRouter->LearnRoute(route);
      for (Router *neighbor : learnedNeighbors) {
        if (neighbor) {
          Route *forwardedRoute = finalRouter->ForwardRoute(route, neighbor);
          if (forwardedRoute) {
            routes.push_back(forwardedRoute);
          }
        }
      }
    }

  } catch (const std::exception &e) {
    delete badRoute;
    throw;
  }

  delete badRoute;
}

std::vector<Router *> Topology::RandomSampleExcluding(int count, Router *attacker) {
  std::vector<Router *> availableRouters;
  for (const auto &pair : G->nodes) {
    Router *router = pair.second.get();
    if (router != attacker) {
      availableRouters.push_back(router);
    }
  }

  if (availableRouters.empty()) {
    std::cerr << "No available routers to sample from." << std::endl;
    return {};
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(availableRouters.begin(), availableRouters.end(), gen);

  if (count > static_cast<int>(availableRouters.size())) {
    count = availableRouters.size();
  }

  std::vector<Router *> sampledRouters(availableRouters.begin(), availableRouters.begin() + count);
  return sampledRouters;
}

Route *Topology::CraftRoute(Router *victim, Router *attacker, int numberOfHops) {
  std::vector<Router *> path;

  if (numberOfHops == 0) {
    path.push_back(attacker);
  } else if (numberOfHops == 1) {
    path.push_back(victim);
    path.push_back(attacker);
  } else {
    path.push_back(victim);
    std::vector<Router *> sampledRouters = RandomSampleExcluding(numberOfHops - 1, attacker);
    path.insert(path.end(), sampledRouters.begin(), sampledRouters.end());
    path.push_back(attacker);
  }

  Route *route = new Route();
  route->destination = victim;
  route->path = path;
  route->originValid = (numberOfHops != 0);
  route->pathEndInvalid = (numberOfHops <= 1);
  route->authenticated = false;
  return route;
}

void Topology::assignNeighbors(const AsRel &asRel) {
  int as1 = asRel.AS1;
  int as2 = asRel.AS2;

  auto router1 = G->nodes[as1];
  auto router2 = G->nodes[as2];

  if (asRel.Relation == -1) {
    router1->neighbors_[as2] = Neighbor{Relation::Customer, router2.get()};
    router2->neighbors_[as1] = Neighbor{Relation::Provider, router1.get()};
  } else if (asRel.Relation == 0) {
    router1->neighbors_[as2] = Neighbor{Relation::Peer, router2.get()};
    router2->neighbors_[as1] = Neighbor{Relation::Peer, router1.get()};
  }
}

std::shared_ptr<Router> Topology::GetRouter(int routerHash) const {
  auto it = G->nodes.find(routerHash);
  if (it != G->nodes.end()) {
    return it->second;
  }
  return nullptr;
}

Relation Topology::inverseRelation(Relation rel) const {
  switch (rel) {
  case Relation::Customer:
    return Relation::Provider;
  case Relation::Provider:
    return Relation::Customer;
  case Relation::Peer:
    return Relation::Peer;
  default:
    return Relation::Unknown;
  }
}
