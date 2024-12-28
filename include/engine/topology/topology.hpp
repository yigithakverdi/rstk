// Topology.hpp
#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include "deployment.hpp"
#include "engine/rpki/rpki.hpp"
#include "graph/graph.hpp"
#include "logger/verbosity.hpp"
#include "parser/parser.hpp"
#include "router/relation.hpp"
#include "router/router.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
class RouterController;
class Topology;

class Topology {
public:
  Topology(const std::vector<AsRel> &asRelsList, std::shared_ptr<RPKI> rpki);
  ~Topology();

  Route *CraftRoute(Router *victim, Router *attacker, int numberOfHops);

  void setDeploymentTrue() { deployment_applied_ = true; }
  void FindRoutesTo(Router *target, VerbosityLevel verbosity = VerbosityLevel::QUIET);
  void Hijack(Router *victim, Router *attacker, int numberOfHops,
              VerbosityLevel verbosity = VerbosityLevel::QUIET);

  void PopulateNeighbors();
  void assignNeighbors(const AsRel &asRel);
  void assignTiers();
  void ValidateDeployment();
  void setDeploymentStrategy(std::unique_ptr<DeploymentStrategy> strategy);
  void deploy();
  void clearDeployment();
  void clearRoutingTables();

  std::vector<std::shared_ptr<Router>> RandomSampleRouters(size_t count) const;
  std::vector<std::shared_ptr<Router>> GetTierOne() const;
  std::vector<std::shared_ptr<Router>> GetTierTwo() const;
  std::vector<std::shared_ptr<Router>> GetTierThree() const;
  std::vector<std::shared_ptr<Router>> GetByCustomerDegree() const;
  std::shared_ptr<Router> GetRouter(int routerHash) const;
  std::vector<Router *> RandomSampleExcluding(int count, Router *attacker);
  std::string TopologyName;

private:
  Relation inverseRelation(Relation rel) const;
  bool deployment_applied_ = false;
  std::unique_ptr<DeploymentStrategy> deploymentStrategy_;

public:
  std::unique_ptr<Graph<std::shared_ptr<Router>>> G;
  std::unordered_map<int, std::unordered_map<int, Edge>> PredMap;
  std::unordered_map<int, std::unordered_map<int, Edge>> AdjMap;
  std::string TopologyType;
  std::shared_ptr<RPKI> RPKIInstance;
};

#endif // TOPOLOGY_HPP
