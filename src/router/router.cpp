#include "router/router.hpp"
#include "graph/graph.hpp"
#include "proto/manager.hpp"
#include "proto/proto.hpp"
#include "router/route.hpp"
#include "sstream"
#include <iostream>

router::router() : id_(""), tier_(0), proto_(nullptr), rpki_(nullptr) {}
router::router(std::string id, std::string name, std::shared_ptr<graph<std::shared_ptr<router>>> g)
    : id_(id), name_(name), graph_(g), tier_(0), proto_(nullptr), rpki_(nullptr) {}
router::router(std::string id, std::string name, std::shared_ptr<graph<std::shared_ptr<router>>> g, int tier,
               std::unique_ptr<IProto> proto, std::shared_ptr<rpki> rpki)
    : id_(id), name_(name), graph_(g), tier_(tier), proto_(std::move(proto)), rpki_(rpki) {}
router::~router() {}
std::string router::getId() const { return id_; }
int router::getTier() const { return tier_; }
void router::setProtocol(std::unique_ptr<IProto> proto) { proto_ = std::move(proto); }
void router::setTier(int tier) { tier_ = tier; }
IProto *router::getProtocol() const { return proto_.get(); }

relation router::getRelation(router *r) const {
  if (!graph_ || !r) {
    return relation::unknown;
  }

  // Get neighbors and edge weight using graph API
  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (neighbor_id == r->getId()) {
      // Convert weight to relation
      if (weight > 0)
        return relation::provider;
      if (weight < 0)
        return relation::customer;
      return relation::peer;
    }
  }
  return relation::unknown;
}

route *router::getRoute(std::string destination) const {
  if (rtable_.find(destination) != rtable_.end()) {
    return rtable_.at(destination);
  }
  return nullptr;
}

std::vector<router *> router::learnRoute(route *r) { return std::vector<router *>(); }

void router::forceRoute(route *r) {
  if (rtable_.find(r->getDestination()->getId()) != rtable_.end()) {
    rtable_.erase(r->getDestination()->getId());
  }
  rtable_.insert({r->getDestination()->getId(), r});
}

void router::clearRTable() { rtable_.clear(); }
route *router::originate(route *r) {
  if (!r) {
    return nullptr;
  }

  auto *new_route = new route();
  new_route->setDestination(this);
  new_route->setPath({this, r->getDestination()});
  new_route->setAuthenticated(true);
  new_route->setOriginValid(false);
  new_route->setPathEndValid(true);

  return new_route;
}

route *router::forward(route *r) {
  if (!r) {
    return nullptr;
  }

  auto *new_route = new route(*r);
  auto current_path = r->getPath();
  current_path.push_back(this);
  new_route->setPath(current_path);
  new_route->setOriginValid(false);
  new_route->setPathEndValid(true);
  new_route->setAuthenticated(true);

  return new_route;
}

std::vector<router *> router::getPeers() {
  std::vector<router *> peers;
  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (weight == 0) { // Peer relationship has weight of 0
      if (auto r = graph_->getNode(neighbor_id)) {
        peers.push_back(r.value().get());
      }
    }
  }
  return peers;
}

std::vector<router *> router::getCustomers() {
  std::vector<router *> customers;
  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (weight > 0) {
      if (auto r = graph_->getNode(neighbor_id)) {
        customers.push_back(r.value().get());
      }
    }
  }
  return customers;
}

std::vector<router *> router::getProviders() {
  std::vector<router *> providers;
  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (weight < 0) {
      if (auto r = graph_->getNode(neighbor_id)) {
        providers.push_back(r.value().get());
      }
    }
  }
  return providers;
}

int router::getNumber() const {
  std::string number = id_.substr(2);
  return std::stoi(number);
}

std::string router::toString() const {
  std::stringstream ss;
  ss << "Router: " << id_ << " Tier: " << tier_ << " Proto: " << proto_->name();
  return ss.str();
}
