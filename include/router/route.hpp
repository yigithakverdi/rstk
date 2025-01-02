#pragma once

#include "router.hpp"
#include "router/relation.hpp"
#include <string>
#include <vector>

class Route;
class Router;

class Route {
public:
  Router *destination = nullptr;
  std::vector<Router *> path;

  bool authenticated = false;
  bool originValid = false;
  bool pathEndInvalid = false;

public:
  Route();
  Route(Router *destination, std::vector<Router *> path) : destination(destination), path(path) {}
  bool ContainsCycle() const;
  void ReversePath();
  std::string PathToString() const;
  std::string ToString() const;

  int CountPeerLinks() const {
    int peer_links = 0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
      if (path[i]->GetRelation(path[i + 1]) == Relation::Peer &&
          path[i + 1]->GetRelation(path[i]) == Relation::Peer) {
        peer_links++;
      }
    }
    return peer_links;
  }

  bool IsValidValleyFree() const { return CountPeerLinks() <= 1; }
};
