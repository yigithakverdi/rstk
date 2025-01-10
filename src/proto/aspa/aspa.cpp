#include "proto/aspa/aspa.hpp"
#include "engine/rpki/aspa/aspa.hpp"
#include "engine/rpki/rpki.hpp"
#include "proto/policy.hpp"
#include "proto/proto.hpp"
#include "router/router.hpp"
#include <string>

ASPAPolicy::ASPAPolicy(std::shared_ptr<rpki> rpki) : rpki_(rpki) { initRules(); };
ASPAPolicy::ASPAPolicy(std::shared_ptr<rpki> rpki, std::shared_ptr<ASPAProtocol> aspaProtocol) : rpki_(rpki) {
  initRules();
};

bool ASPAPolicy::validate(const route &r) const {
  result res = performASPA(r);
  if (r.hasCycle() && res == result::invalid) {
    return false;
  }
  return true;
};

bool ASPAPolicy::prefer(const route &current, const route &newRoute) const {
  if (current.getDestination() != newRoute.getDestination()) {
    return false;
  }

  for (const auto &rule : rules_) {
    int currVal = rule.evaluator(current);
    int newVal = rule.evaluator(newRoute);
    if (currVal == -1 || newVal == -1) {
      continue;
    }
    if (currVal != newVal) {
      return rule.higherIsBetter ? (newVal > currVal) : (newVal < currVal);
    }
  }
  return false;
};

int ASPAPolicy::localPreference(const route &r) const {
  if (r.getPath().size() < 2) {
    return -1;
  }

  router *final = r.getDestination();
  router *first = r.getPath()[r.getPath().size() - 2];

  relation rel = final->getRelation(first);
  if (rel != relation::unknown) {
    return static_cast<int>(rel);
  }

  return -1;
};

int ASPAPolicy::asPathLength(const route &r) const { return static_cast<int>(r.getPath().size()); };
int ASPAPolicy::nextHopASNumber(const route &r) const {
  if (r.getPath().size() < 2) {
    return -1;
  }

  router *nextHop = r.getPath()[r.getPath().size() - 2];
  return nextHop->getNumber();
};

void ASPAPolicy::initRules() {
  rules_ = {{rules::localPreference, [this](const route &r) { return localPreference(r); }, true},
            {rules::asPathLength, [this](const route &r) { return asPathLength(r); }, false},
            {rules::nextHopASNumber, [this](const route &r) { return nextHopASNumber(r); }, false}};
};

bool ASPAPolicy::forward(relation source, relation target) const {
  if (source == relation::customer) {
    return true;
  }

  return (source == relation::customer) || (target == relation::customer);
}

result ASPAPolicy::performASPA(const route &r) const { return result::unknown; };
void ASPAProtocol::updateUSPAS() {
  std::unordered_map<std::string, std::set<std::string>> customerProviders;

  for (const auto &obj : aspaSet_) {
    const std::string &customer = obj.getCustomer();
    const auto &providers = obj.getProviders();
    customerProviders[customer].insert(providers.begin(), providers.end());
  }

  for (const auto &[customer, providers] : customerProviders) {
    aspaobj newObj;
    newObj.setCustomer(customer);
    for (const auto &provider : providers) {
      newObj.addProvider(provider);
    }
    rpki_->addUSPA(customer, providers);
  }
}

void ASPAProtocol::publishToRPKI(router *r) {}
ASPAProtocol::ASPAProtocol(std::shared_ptr<rpki> rpki)
    : IProto(std::make_unique<ASPAPolicy>(rpki)), rpki_(rpki) {}

void ASPAProtocol::addASPAObject(const aspaobj &obj) {
  aspaSet_.push_back(obj);
  updateUSPAS();
};

std::vector<aspaobj> ASPAProtocol::getASPAObjects(router *r) const { return aspaSet_; };
aspaobj ASPAProtocol::getASPAObject(router *r) const {
  auto it = std::find_if(aspaSet_.begin(), aspaSet_.end(), [asNumber = r->getId()](const aspaobj &obj) {
    return obj.getCustomer() == asNumber;
  });

  return (it != aspaSet_.end()) ? *it : aspaobj();
}

aspaobj ASPAProtocol::createASPAObject(router *r) const {
  std::set<std::string> providers;

  // For tier 1 ASes, mark as origin
  if (r->getTier() == 1) {
    providers.insert("0");
  } else {
    // Get all providers from router's neighbors
    for (const auto &provider : r->getProviders()) {
      providers.insert(provider->getId());
    }
  }

  return aspaobj(r->getId(), providers);
}

std::string ASPAProtocol::name() const { return "ASPA"; };
std::string ASPAProtocol::info() const { return "Autonomous System Provider Authorization"; };

bool ASPAPolicy::hasASPA(router *r) const {
  const auto &uspas = rpki_->getUSPAs();
  return std::any_of(uspas.begin(), uspas.end(),
                     [id = r->getId()](const aspaobj &obj) { return obj.getCustomer() == id; });
}

bool ASPAPolicy::isProvider(router *as, router *provider) const {
  const auto &uspas = rpki_->getUSPAs();
  auto it = std::find_if(uspas.begin(), uspas.end(),
                         [asId = as->getId()](const aspaobj &obj) { return obj.getCustomer() == asId; });

  if (it == uspas.end()) {
    return false;
  }

  const auto &providers = it->getProviders();
  return std::find(providers.begin(), providers.end(), provider->getId()) != providers.end();
}
