// Router.hpp
#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "engine/rpki/rpki.hpp"
#include "logger/verbosity.hpp"
#include "plugins/plugins.hpp"
#include "relation.hpp"

std::string relationToString(Relation rel);

class Route;
class Router;

struct Neighbor {
  Relation relation;
  Router *router;
};

class Router {
public:
  std::unordered_map<int, Route *> routerTable;
  std::map<int, Neighbor> neighbors_;
  int ASNumber = 0;
  int Tier = 0;
  std::unique_ptr<Protocol> proto;
  std::shared_ptr<RPKI> rpki;

public:
  Router();
  Router(int ASNumber);
  Router(int ASNumber, int Tier, std::unique_ptr<Protocol> proto,
         std::shared_ptr<RPKI> rpki)
      : ASNumber(ASNumber), Tier(Tier), proto(std::move(proto)), rpki(rpki) {
  }

  Relation GetRelation(Router *router);
  std::vector<Router *> LearnRoute(Route *route, VerbosityLevel verbosity = VerbosityLevel::QUIET);
  void ForceRoute(Route *route);
  void Clear();
  Route *OriginateRoute(Router *nextHop);
  Route *ForwardRoute(Route *route, Router *nextHop);
  std::vector<Neighbor> GetPeers();
  std::vector<Neighbor> GetCustomers();
  std::vector<Neighbor> GetProviders();
  std::string toString() const;
  friend std::ostream &operator<<(std::ostream &os, const Router &router);
};

#endif // ROUTER_HPP
