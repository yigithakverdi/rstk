#pragma once
#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include "engine/rpki/rpki.hpp"
#include "engine/topology/deployment.hpp"
#include "graph/graph.hpp"
#include "router/router.hpp"
#include "router/route.hpp"
#include <memory>

class topology {
public:
  topology();
  ~topology();
  
  void findRoutesTo(router *target);
  void hijack(router *victim, router *attacker, int hops);

  bool hasRouter(const std::string &asn) const;
  bool hasDeployment() const;
  
  int getTier(const std::string &asn) const;
  void setTier(const std::string &asn, int tier);
  void assignTiers();

  void setDeploymentStrategy(IDeployment &d);
  void deploy();
  void clearDeployment();
  void clearRoutingTables();

  std::vector<std::shared_ptr<router>> getByCustomerDegree() const;
  std::vector<std::shared_ptr<router>> getTierOne() const;
  std::vector<std::shared_ptr<router>> getTierTwo() const;
  std::vector<std::shared_ptr<router>> getTierThree() const;
  std::vector<std::shared_ptr<router>> getRandomRouters(size_t count) const;
  std::shared_ptr<rpki> getRPKI() const;

  std::shared_ptr<router> getRouter(const std::string &asn) const;
  std::shared_ptr<route> getRoute(const router *source, const router *target) const;
  std::shared_ptr<router> getRouterByASN(const std::string &asn) const;
  std::shared_ptr<graph<std::shared_ptr<router>>> &getGraph();
  
  
private:
  std::shared_ptr<graph<std::shared_ptr<router>>> graph_;
  std::shared_ptr<rpki> rpki_;
  std::shared_ptr<IDeployment> deploymentStrategy_;
};

#endif // TOPOLOGY_HPP
