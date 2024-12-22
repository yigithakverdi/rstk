#ifndef PLUGINS_HPP
#define PLUGINS_HPP

#include "router/relation.hpp"
#include <memory>
#include <string>

class Route;
class Topology; 
enum class Relation;

class PolicyEngine {
public:
  virtual bool shouldAcceptRoute(const Route &route) const = 0;
  virtual bool shouldPreferRoute(const Route &currentRoute,
                                 const Route &newRoute) const = 0;
  virtual bool canForwardRoute(Relation sourceRelation,
                               Relation targetRelation) const = 0;
  virtual ~PolicyEngine() = default;
};

class Protocol {
public:
  explicit Protocol(std::unique_ptr<PolicyEngine> policyEngine);
  virtual ~Protocol() = default;
  bool acceptRoute(const Route &route) const;
  bool preferRoute(const Route &currentRoute, const Route &newRoute) const;
  bool canForwardTo(Relation sourceRelation, Relation targetRelation) const;
  virtual std::string getProtocolName() const = 0;
  virtual std::string getProtocolInfo() const = 0;
  virtual std::string getDetailedProtocolInfo() const = 0;
  virtual std::pair<int, int>
  getDeploymentStats(const Topology &topology) const = 0;

protected:
  const PolicyEngine &getPolicyEngine() const;

private:
  std::unique_ptr<PolicyEngine> policyEngine_;
};

#endif // PLUGINS_HPP
