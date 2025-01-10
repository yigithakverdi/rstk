#include "engine/topology/postop.hpp"
#include "engine/topology/topology.hpp"

postop::config::~config() {}

void postop::rpkiconfig::configure(topology &t) {
  auto& graph = t.getGraph();
  auto rpki = t.getRPKI();

  for (const auto &[asn, router] : graph->getNodes()) {
    auto providers = router->getProviders();

    std::set<std::string> provider_asns;
    for (const auto *provider : providers) {
      provider_asns.insert(provider->getId());
    }

    if (!provider_asns.empty()) {
      rpki->addUSPA(asn, provider_asns);
    }
  }
}

void postop::tierconfig::configure(topology &t) {
  auto& graph = t.getGraph();

  for (const auto &[asn, router] : graph->getNodes()) {
    auto customers = router->getCustomers();
    auto providers = router->getProviders();

    if (providers.empty() && !customers.empty()) {
      router->setTier(1);
    } else if (!providers.empty() && customers.empty()) {
      router->setTier(3);
    } else if (!providers.empty() && !customers.empty()) {
      router->setTier(2);
    } else {
      router->setTier(3);
    }
  }
}
