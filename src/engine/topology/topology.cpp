#include "engine/topology/topology.hpp"
#include "database/dbmanager.hpp"
#include "database/queries.hpp"
#include "engine/topology/deployment.hpp"
#include "engine/topology/postop.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <algorithm>
#include <deque>
#include <iostream>
#include <random>

std::shared_ptr<rpki> topology::getRPKI() const { return rpki_; }
std::shared_ptr<router> topology::getRouter(const std::string &asn) const {
  auto node = graph_->getNode(asn);
  if (!node.has_value()) {
    return nullptr;
  }
  return node.value();
}

route *topology::craftRoute(router *victim, router *attacker, int numberOfHops) {
  std::vector<router *> path;

  if (numberOfHops == 0) {
    path.push_back(attacker);
  } else if (numberOfHops == 1) {
    path.push_back(victim);
    path.push_back(attacker);
  } else {
    path.push_back(victim);
    auto sampledRouters = getRandomRouters(numberOfHops - 1);
    // Convert shared_ptr routers to raw pointers and exclude attacker
    std::vector<router *> rawRouters;
    std::transform(sampledRouters.begin(), sampledRouters.end(), std::back_inserter(rawRouters),
                   [attacker](const auto &r) { return (r.get() != attacker) ? r.get() : nullptr; });

    // Remove any null pointers that were filtered out
    rawRouters.erase(std::remove(rawRouters.begin(), rawRouters.end(), nullptr), rawRouters.end());

    path.insert(path.end(), rawRouters.begin(),
                rawRouters.begin() + std::min(numberOfHops - 1, (int)rawRouters.size()));
    path.push_back(attacker);
  }

  auto *newRoute = new route();
  newRoute->setDestination(victim);
  newRoute->setPath(path);
  newRoute->setOriginValid(numberOfHops != 0);
  newRoute->setPathEndValid(numberOfHops <= 1);
  newRoute->setAuthenticated(false);
  return newRoute;
}

void topology::hijack(router *victim, router *attacker, int numberOfHops) {
  if (!hasDeployment()) {
    throw std::runtime_error("No deployment strategy set");
  }

  route *badRoute = craftRoute(victim, attacker, numberOfHops);
  std::cout << "Hijacking route: " << badRoute->toString() << std::endl;

  if (!badRoute) {
    throw std::runtime_error("Failed to craft bad route.");
  }

  std::deque<route *> routes;

  try {
    auto neighbors = graph_->getNeighbors(attacker->getId());
    for (const auto &[neighbor_id, _] : neighbors) {
      if (auto neighborRouter = getRouter(neighbor_id)) {
        route *forwardedRoute = attacker->forward(badRoute, neighborRouter.get());
        if (forwardedRoute) {
          routes.push_back(forwardedRoute);
        }
      }
    }

    while (!routes.empty()) {
      route *currentRoute = routes.front();
      routes.pop_front();

      if (currentRoute->getPath().empty()) {
        std::cerr << "Route has an empty path. Skipping." << std::endl;
        continue;
      }

      router *finalRouter = currentRoute->getPath().back();
      if (!finalRouter) {
        std::cerr << "Final router in route path is null. Skipping." << std::endl;
        continue;
      }

      std::vector<router *> learnedNeighbors = finalRouter->learnRoute(currentRoute);
      for (router *neighbor : learnedNeighbors) {
        if (neighbor) {
          route *forwardedRoute = finalRouter->forward(currentRoute, neighbor);
          if (forwardedRoute) {
            routes.push_back(forwardedRoute);
          }
        }
      }
    }

  } catch (const std::exception &e) {
    delete badRoute;
    throw;
  }

  delete badRoute;
}

std::shared_ptr<route> topology::getRoute(const router *source, const router *target) const {
  if (!source || !target) {
    return nullptr;
  }

  route *rawRoute = source->getRoute(target->getId());
  if (!rawRoute) {
    return nullptr;
  }

  return std::make_shared<route>(*rawRoute);
}

std::shared_ptr<graph<std::shared_ptr<router>>> &topology::getGraph() { return graph_; }
void topology::assignTiers() {
  for (const auto &[id, router] : graph_->getNodes()) {
    router->setTier(getTier(id));
  }
}

void topology::print() const {
  std::cout << "Topology Information:\n";
  std::cout << "==================\n";

  std::cout << "\nNodes by Tier:\n";
  for (const auto &[id, r] : graph_->getNodes()) {
    std::cout << "AS" << id << " (Tier " << r->getTier() << ")\n";

    std::cout << "  Providers: ";
    for (const auto *provider : r->getProviders()) {
      std::cout << provider->getId() << " ";
    }

    std::cout << "\n  Peers: ";
    for (const auto *peer : r->getPeers()) {
      std::cout << peer->getId() << " ";
    }

    std::cout << "\n  Customers: ";
    for (const auto *customer : r->getCustomers()) {
      std::cout << customer->getId() << " ";
    }
    std::cout << "\n\n";
  }

  // Print summary statistics
  std::cout << "\nSummary:\n";
  std::cout << "Total ASes: " << graph_->getNodes().size() << "\n";
  std::cout << "Tier 1 ASes: " << getTierOne().size() << "\n";
  std::cout << "Tier 2 ASes: " << getTierTwo().size() << "\n";
  std::cout << "Tier 3 ASes: " << getTierThree().size() << "\n";
}

std::shared_ptr<IDeployment> topology::getDeployment() const { return deploymentStrategy_; }
topology::topology() {

  // Get all the nodes from the database, database configurations are done
  // on the initial stages of the pipeline, and connection should already
  // be established.
  auto &db = dbmanager::instance();
  auto nodes = db.executeQuery(queries::GET_NODES);

  // Create a new graph and RPKI object to store the topology information
  graph_ = std::make_shared<graph<std::shared_ptr<router>>>();
  rpki_ = std::make_shared<rpki>();

  // Iterate over the nodes and create a router object for each node.
  // The router object will be added to the graph, with the related
  // connections and relations
  for (const auto &row : nodes) {
    std::string id = row["node_id"].as<std::string>();
    std::string asn = row["name"].as<std::string>();

    auto r = std::make_shared<router>(id, asn, graph_);
    graph_->addNode(id, r);
  }

  // Fetch all connections and add them to the graph bi-directionally
  // meaning if there is a provider-customer or customer-provider
  // relationship between two nodes, the edge will be added to the
  // graph in both directions.
  auto connections = db.executeQuery(queries::GET_CONNECTIONS);
  for (const auto &row : connections) {
    std::string source = row["source_node_id"].as<std::string>();
    std::string target = row["target_node_id"].as<std::string>();
    double weight = row["weight"].as<double>();

    graph_->addEdge(source, target, -weight);
    graph_->addEdge(target, source, weight);
  }

  // Once all the topology information is loaded, we need to configure additional core
  // infrastructure related information to the routers, RPKI etc.
  std::vector<std::unique_ptr<postop::config>> postOpsChain;
  postOpsChain.push_back(std::make_unique<postop::deploymentconfig>());
  postOpsChain.push_back(std::make_unique<postop::rpkiconfig>());
  postOpsChain.push_back(std::make_unique<postop::tierconfig>());
  for (const auto &config : postOpsChain) {
    config->configure(*this);
  }
}

topology::~topology() {}
bool topology::hasRouter(const std::string &asn) const {
  if (graph_->hasNode(asn)) {
    return true;
  }
  return false;
}

bool topology::hasDeployment() const { return deploymentStrategy_ != nullptr; }
int topology::getTier(const std::string &asn) const {
  auto router = getRouter(asn);
  if (!router) {
    throw std::runtime_error("router not found for AS" + asn);
  }

  auto providers = router->getProviders();
  auto customers = router->getCustomers();

  if (providers.empty() && !customers.empty()) {
    return 1;
  } else if (!providers.empty() && customers.empty()) {
    return 3;
  } else if (!providers.empty() && !customers.empty()) {
    return 2;
  }
  return 3;
}

void topology::printQueue(std::deque<route *> routes) {
  for (route *r : routes) {
    std::cout << r->toString() << std::endl;
  }
}

void topology::findRoutesTo(router *target) {
  std::deque<route *> routes;

  for (router *neighbor : target->getNeighbors()) {
    route *r = target->originate(neighbor);
    routes.push_back(r);
  }

  while (!routes.empty()) {
    route *curr = routes.front();
    routes.pop_front();
    std::vector<router *> neighbors;
    router *finalRouter = curr->getPath().back();
    neighbors = finalRouter->learnRoute(curr);

    for (router *neighbor : neighbors) {
      route *r = finalRouter->forward(curr, neighbor);
      routes.push_back(r);
    }
    /*printQueue(routes);*/
    /*std::cout << std::endl;*/
  }
}

void topology::setTier(const std::string &asn, int tier) {
  auto router = getRouter(asn);
  if (!router) {
    throw std::runtime_error("router not found for AS" + asn);
  }

  router->setTier(tier);
}

void topology::setDeploymentStrategy(std::shared_ptr<IDeployment> d) { deploymentStrategy_ = d; }
void topology::deploy() {
  if (!deploymentStrategy_) {
    throw std::runtime_error("Deployment strategy not set");
  }
  deploymentStrategy_->deploy(*this);
}

void topology::clearDeployment() {
  if (!deploymentStrategy_) {
    throw std::runtime_error("Deployment strategy not set");
  }
  deploymentStrategy_->clear(*this);
}

void topology::clearRoutingTables() {
  for (const auto &[id, router] : graph_->getNodes()) {
    router->clearRTable();
  }
}

std::vector<std::shared_ptr<router>> topology::getByCustomerDegree() const {
  std::vector<std::pair<std::shared_ptr<router>, size_t>> routerCustomerCounts;
  routerCustomerCounts.reserve(graph_->getNodes().size());

  for (const auto &[id, r] : graph_->getNodes()) {
    size_t customerCount = r->getCustomers().size();
    routerCustomerCounts.emplace_back(r, customerCount);
  }

  std::sort(routerCustomerCounts.begin(), routerCustomerCounts.end(), [](const auto &a, const auto &b) {
    if (a.second == b.second) {
      return a.first->getId() > b.first->getId();
    }
    return a.second > b.second;
  });

  std::vector<std::shared_ptr<router>> sortedRouters;
  sortedRouters.reserve(routerCustomerCounts.size());

  for (const auto &[r, count] : routerCustomerCounts) {
    sortedRouters.push_back(r);
  }

  return sortedRouters;
}

std::vector<std::shared_ptr<router>> topology::getTierOne() const {
  std::vector<std::shared_ptr<router>> tierOne;
  for (const auto &[id, r] : graph_->getNodes()) {
    if (r->getTier() == 1) {
      tierOne.push_back(r);
    }
  }
  return tierOne;
}

std::vector<std::shared_ptr<router>> topology::getTierTwo() const {
  std::vector<std::shared_ptr<router>> tierTwo;
  for (const auto &[id, r] : graph_->getNodes()) {
    if (r->getTier() == 2) {
      tierTwo.push_back(r);
    }
  }
  return tierTwo;
}

std::vector<std::shared_ptr<router>> topology::getTierThree() const {
  std::vector<std::shared_ptr<router>> tierThree;
  for (const auto &[id, r] : graph_->getNodes()) {
    if (r->getTier() == 3) {
      tierThree.push_back(r);
    }
  }
  return tierThree;
}

std::vector<std::shared_ptr<router>> topology::getRandomRouters(size_t count) const {
  std::vector<std::shared_ptr<router>> allRouters;
  allRouters.reserve(graph_->getNodes().size());

  for (const auto &[id, r] : graph_->getNodes()) {
    allRouters.push_back(r);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(allRouters.begin(), allRouters.end(), gen);

  count = std::min(count, allRouters.size());
  return std::vector<std::shared_ptr<router>>(allRouters.begin(), allRouters.begin() + count);
}
