// Router.cpptopolo
#include <iostream>

#include "plugins/manager.hpp"
#include "plugins/plugins.hpp"
#include "router/relation.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include "sstream"

Router::Router()
    : ASNumber(0), Tier(0), proto(ProtocolFactory::Instance().CreateProtocol(ASNumber)),
      rpki(nullptr) {}

Router::Router(int ASNumber)
    : ASNumber(ASNumber), Tier(0), proto(ProtocolFactory::Instance().CreateProtocol(ASNumber)),
      rpki(nullptr) {}

Relation Router::GetRelation(Router *router) {
  for (const auto &[asNum, neighbor] : neighbors_) {
    if (neighbor.router == router) {
      return neighbor.relation;
    }
  }
  return Relation::Unknown;
}

Route *Router::GetRoute(int destinationAS) const {
  auto it = routerTable.find(destinationAS);
  if (it != routerTable.end()) {
    return it->second;
  }
  return nullptr;
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
        oss << "Path: [";
        // Iterate in reverse to show forward path
        for (int i = route->path.size() - 1; i >= 0; --i) {
          oss << "AS" << route->path[i]->ASNumber;
          // Show relationship between consecutive ASes
          if (i > 0) {
            Relation rel = route->path[i]->GetRelation(route->path[i - 1]);
            oss << " -(" << relationToString(rel) << ")-> ";
          }
        }
        oss << "]\n";
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

  if (!proto->acceptRoute(*route)) {
    return {};
  }

  auto existingRoute = routerTable.find(route->destination->ASNumber);
  if (existingRoute != routerTable.end() && !proto->preferRoute(*existingRoute->second, *route)) {
    return {};
  }

  routerTable[route->destination->ASNumber] = route;

  // First determine which relations we can forward to
  std::set<Relation> forwardToRelations;
  Router *currentAS = route->path.back();
  Router *sourceAS = route->path[route->path.size() - 2];
  Relation sourceRelation = currentAS->GetRelation(sourceAS);

  /**/
  /*std::cout << "Current AS: " << currentAS->ASNumber << std::endl;*/
  /*std::cout << "Source AS: " << sourceAS->ASNumber << std::endl;*/
  /*std::cout << "AS" << currentAS->ASNumber << " received route from AS" << sourceAS->ASNumber*/
  /*          << " with relation: " << relationToString(sourceRelation) << std::endl;*/

  // Only check neighbors we're actually connected to
  std::vector<Router *> neighborsToForward;
  for (const auto &[neighborAS, neighbor] : neighbors_) {
    bool canForward = proto->canForwardTo(sourceRelation, neighbor.relation);
    /*std::cout << "Checking if AS" << ASNumber << " can forward to AS" << neighborAS << " ("*/
    /*          << relationToString(neighbor.relation) << "): " << (canForward ? "YES" : "NO")*/
    /*          << std::endl;*/

    if (canForward) {
      /*std::cout << "AS" << ASNumber << " will forward to AS" << neighborAS*/
      /*          << " because it received route from " << relationToString(sourceRelation)*/
      /*          << " and neighbor is " << relationToString(neighbor.relation) << std::endl;*/
      neighborsToForward.push_back(neighbor.router);
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
