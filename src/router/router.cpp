// Router.cpp
#include <iostream>

#include "plugins/manager.hpp"
#include "plugins/plugins.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include "sstream"

Router::Router() : ASNumber(0), Tier(0), proto(ProtocolFactory::Instance().CreateProtocol(ASNumber)), rpki(nullptr) {}

Router::Router(int ASNumber)
    : ASNumber(ASNumber), Tier(0), proto(ProtocolFactory::Instance().CreateProtocol(ASNumber)), rpki(nullptr) {}

Relation Router::GetRelation(Router *router) {
  for (const auto &[asNum, neighbor] : neighbors_) {
    if (neighbor.router == router) {
      return neighbor.relation;
    }
  }
  return Relation::Unknown;
}

// Function to convert Relation enum to string
std::string relationToString(Relation rel) {
  switch (rel) {
  case Relation::Customer:
    return "Customer";
  case Relation::Provider:
    return "Provider";
  case Relation::Peer:
    return "Peer";
  default:
    return "Unknown";
  }
}

// Implementation of the toString method
std::string Router::toString() const {
  std::ostringstream oss;
  oss << "Router Information:\n";
  oss << "-------------------\n";
  oss << "AS Number: " << ASNumber << "\n";
  oss << "Tier: " << Tier << "\n";
  oss << "Protocol: " << (proto ? proto->getProtocolName() : "None") << "\n";
  oss << "RPKI: " << (rpki ? "Enabled" : "Disabled") << "\n";
  oss << "Neighbors:\n";

  if (neighbors_.empty()) {
    oss << "  No neighbors.\n";
  } else {
    for (const auto &pair : neighbors_) {
      int neighborAS = pair.first;
      const Neighbor &neighbor = pair.second;
      oss << "  - AS" << neighborAS << " (" << relationToString(neighbor.relation) << ")\n";
    }
  }

  oss << "Routing Table:\n";
  if (routerTable.empty()) {
    oss << "  No routes.\n";
  } else {
    for (const auto &routePair : routerTable) {
      int destinationAS = routePair.first;
      Route *route = routePair.second;
      oss << "  - Destination AS" << destinationAS << ": ";
      if (route) {
        oss << "Path: ";
        for (const auto &hop : route->path) {
          oss << "AS" << hop->ASNumber << " -> ";
        }
        oss << "\n";
      } else {
        oss << "Invalid Route\n";
      }
    }
  }

  return oss.str();
}

// Overloaded operator<< implementation
std::ostream &operator<<(std::ostream &os, const Router &router) {
  os << router.toString();
  return os;
}

std::vector<Router *> Router::LearnRoute(Route *route, VerbosityLevel verbosity) {
  if (!route || ASNumber == route->destination->ASNumber) {
    return {};
  }

  if (verbosity == VerbosityLevel::NORMAL) {
    // Add path validation visualization
    std::cout << "\n┌─ Path Analysis for AS" << ASNumber << " ──────────\n";
    std::cout << "│ Validating route to AS" << route->destination->ASNumber << "\n";
  }

  // Show hop-by-hop path with validation
  if (verbosity == VerbosityLevel::NORMAL) {
    std::cout << "├─ Path trace:\n";

    for (size_t i = 0; i < route->path.size(); i++) {
      Router *hop = route->path[i];
      std::string status;
      if (i < route->path.size() - 1) {
        Router *nextHop = route->path[i + 1];
        Relation rel = hop->GetRelation(nextHop);
        // Check for valley-free violations
        if (rel == Relation::Provider && i > 0) {
          status = "🔴 Valley violation";
        } else {
          status = "🟢 Valid hop";
        }
      }
      std::cout << "│  " << (i == route->path.size() - 1 ? "└" : "├") << "─ AS" << hop->ASNumber << " ("
                << (i == 0 ? "Origin" : "Transit") << ") " << status << "\n";
    }
  }

  if (!proto->acceptRoute(*route)) {
    if (verbosity == VerbosityLevel::NORMAL) {
      std::cout << "└─ ❌ Route rejected by policy\n\n";
    }
    return {};
  }

  auto existingRoute = routerTable.find(route->destination->ASNumber);
  if (existingRoute != routerTable.end() && !proto->preferRoute(*existingRoute->second, *route)) {
    return {};
  }

  routerTable[route->destination->ASNumber] = route;

  // Determine which relations we can forward to
  std::vector<Router *> neighborsToForward;
  for (const auto &[neighborAS, neighbor] : neighbors_) {
    if (route->path.size() >= 2) {
      // Correctly determine the sourceRelation as the relation between this
      // router and the originator
      Router *originatorAS = route->path[route->path.size() - 2];
      Relation sourceRelation = GetRelation(originatorAS);

      if (proto->canForwardTo(sourceRelation, neighbor.relation)) {
        neighborsToForward.push_back(neighbor.router);
      } else {
      }
    }
  }

  return neighborsToForward;
}

void Router::ForceRoute(Route *route) { routerTable[route->destination->ASNumber] = route; }

void Router::Clear() { routerTable.clear(); }

Route *Router::OriginateRoute(Router *nextHop) {
  if (!nextHop) {
    return nullptr;
  }

  auto *route = new Route();
  route->destination = this;     // Set destination as the originating router
  route->path = {this, nextHop}; // Path includes both source and next hop
  route->originValid = false;
  route->pathEndInvalid = false;
  route->authenticated = true;
  return route;
}

Route *Router::ForwardRoute(Route *route, Router *nextHop) {
  if (!route || !nextHop) {
    std::cerr << "ForwardRoute: route or nextHop is null." << std::endl;
    return nullptr;
  }
  auto *newRoute = new Route(*route);
  newRoute->path.push_back(nextHop);
  newRoute->originValid = false;
  newRoute->pathEndInvalid = false;
  newRoute->authenticated = true;
  return newRoute;
}

std::vector<Neighbor> Router::GetPeers() {
  std::vector<Neighbor> peers;
  for (const auto &[asNum, neighbor] : neighbors_) {
    if (neighbor.relation == Relation::Peer) {
      peers.push_back(neighbor);
    }
  }
  return peers;
}

std::vector<Neighbor> Router::GetCustomers() {
  std::vector<Neighbor> customers;
  for (const auto &[asNum, neighbor] : neighbors_) {
    if (neighbor.relation == Relation::Customer) {
      customers.push_back(neighbor);
    }
  }
  return customers;
}

std::vector<Neighbor> Router::GetProviders() {
  std::vector<Neighbor> providers;
  for (const auto &[asNum, neighbor] : neighbors_) {
    if (neighbor.relation == Relation::Provider) {
      providers.push_back(neighbor);
    }
  }
  return providers;
}
