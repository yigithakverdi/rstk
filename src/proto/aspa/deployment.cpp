#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "proto/aspa/aspa.hpp"
#include "proto/aspa/deployment.hpp"

RandomDeployment::RandomDeployment(double objectPercentage, double policyPercentage, topology &t)
    : IDeployment(t), objectPercentage_(objectPercentage), policyPercentage_(policyPercentage), t_(t) {}
void RandomDeployment::deploy(topology &t) {
  clear(t);

  if (!t.hasDeployment()) {
    return;
  }

  auto &graph = t.getGraph();
  size_t total = graph->getNodes().size();

  size_t object_count = static_cast<size_t>(total * objectPercentage_ / 100.0);
  size_t policy_count = static_cast<size_t>(total * policyPercentage_ / 100.0);

  auto object_routers = t.getRandomRouters(object_count);
  auto policy_routers = t.getRandomRouters(policy_count);

  auto rpki = t.getRPKI();
  if (!rpki) {
    throw std::runtime_error("RPKI not initialized");
  }

  for (const auto &router : policy_routers) {
    router->setProtocol(std::make_unique<ASPAProtocol>(rpki));

    if (auto aspa = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      auto obj = aspa->createASPAObject(router.get());
      aspa->addASPAObject(obj);
      aspa->updateUSPAS();
    }
  }

  for (const auto &router : object_routers) {
    if (!dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      if (auto aspa = std::make_unique<ASPAProtocol>(rpki)) {
        aspa->publishToRPKI(router.get());
      }
    }
  }
}

void RandomDeployment::clear(topology &t) {
  auto &g = t.getGraph();

  for (const auto &[id, router] : g->getNodes()) {
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaProto->getASPAObjects(router.get()).clear();
    }
  }

  t.getRPKI()->clearUSPAs();
}

bool RandomDeployment::validate(topology &t) { return true; }
SelectiveDeployment::SelectiveDeployment(double objectPercentage, double policyPercentage, topology &t)
    : IDeployment(t), objectPercentage_(objectPercentage), policyPercentage_(policyPercentage), t_(t) {}
void SelectiveDeployment::deploy(topology &t) {
  clear(t);

  auto routersByCustomerDegree = t.getByCustomerDegree();

  size_t totalRouters = t.getGraph()->getNodes().size();
  size_t totalObjectCount = static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
  size_t totalPolicyCount = static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

  std::vector<std::shared_ptr<router>> objectTargets(
      routersByCustomerDegree.begin(),
      routersByCustomerDegree.begin() + std::min(totalObjectCount, routersByCustomerDegree.size()));

  std::vector<std::shared_ptr<router>> policyTargets(
      routersByCustomerDegree.begin(),
      routersByCustomerDegree.begin() + std::min(totalPolicyCount, routersByCustomerDegree.size()));

  auto rpki = t.getRPKI();
  if (!rpki) {
    throw std::runtime_error("RPKI not initialized");
  }

  for (const auto &r : policyTargets) {
    r->setProtocol(std::make_unique<ASPAProtocol>(t.getRPKI()));
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(r->getProtocol())) {
      aspaobj obj = aspaProto->createASPAObject(r.get());
      aspaProto->addASPAObject(obj);
      aspaProto->updateUSPAS();
    }
  }

  for (const auto &router : objectTargets) {
    if (!dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      if (auto aspa = std::make_unique<ASPAProtocol>(rpki)) {
        aspa->publishToRPKI(router.get());
      }
    }
  }
}

bool SelectiveDeployment::validate(topology &t) { return true; }
void SelectiveDeployment::clear(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaProto->getASPAObjects(router.get()).clear();
    }
  }

  t.getRPKI()->clearUSPAs();
}
