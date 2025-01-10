#include "engine/topology/deployment.hpp"
#include "engine/topology/topology.hpp"
#include "proto/aspa/aspa.hpp"
#include "proto/leak/deployment.hpp"

RandomLeakDeployment::RandomLeakDeployment(double objectPercentage, double policyPercentage, topology &t)
    : IDeployment(t), objectPercentage_(objectPercentage), policyPercentage_(policyPercentage), t_(t) {}
void RandomLeakDeployment::deploy(topology &t) {
  clear(t);

  size_t totalRouters = t.getGraph()->getNodes().size();
  size_t objectCount = static_cast<size_t>(totalRouters * objectPercentage_ / 100.0);
  size_t policyCount = static_cast<size_t>(totalRouters * policyPercentage_ / 100.0);

  auto objectRouters = t.getRandomRouters(objectCount);
  auto policyRouters = t.getRandomRouters(policyCount);

  auto rpki = t.getRPKI();
  if (!rpki) {
    throw std::runtime_error("RPKI not initialized");
  }

  for (const auto &router : policyRouters) {
    router->setProtocol(std::make_unique<ASPAProtocol>(t.getRPKI()));
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaobj obj = aspaProto->createASPAObject(router.get());
      aspaProto->addASPAObject(obj);
      aspaProto->updateUSPAS();
    }
  }

  for (const auto &router : objectRouters) {
    if (!dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      if (auto aspa = std::make_unique<ASPAProtocol>(rpki)) {
        aspa->publishToRPKI(router.get());
      }
    }
  }
}

bool RandomLeakDeployment::validate(topology &t) { return true; }
void RandomLeakDeployment::clear(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaProto->getASPAObjects(router.get()).clear();
    }
  }

  t.getRPKI()->clearUSPAs();
}

SelectiveLeakDeployment::SelectiveLeakDeployment(double objectPercentage, double policyPercentage,
                                                 topology &t)
    : IDeployment(t), objectPercentage_(objectPercentage), policyPercentage_(policyPercentage), t_(t) {}
void SelectiveLeakDeployment::deploy(topology &t) {
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

  for (const auto &router : policyTargets) {
    router->setProtocol(std::make_unique<ASPAProtocol>(t.getRPKI()));
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaobj obj = aspaProto->createASPAObject(router.get());
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

bool SelectiveLeakDeployment::validate(topology &t) { return true; }
void SelectiveLeakDeployment::clear(topology &t) {
  for (const auto &[id, router] : t.getGraph()->getNodes()) {
    if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router->getProtocol())) {
      aspaProto->getASPAObjects(router.get()).clear();
    }
  }

  t.getRPKI()->clearUSPAs();
}
