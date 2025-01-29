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

void router::printRTable() const {
  std::cout << "Routing table for " << id_ << std::endl;
  for (const auto &[dest, route] : rtable_) {
    std::cout << "  " << dest << " -> " << route->toString() << std::endl;
  }
}

relation router::weightToRelation(int weight) {
  relation rel;
  if (weight > 0) {
    rel = relation::customer;
  } else if (weight < 0) {
    rel = relation::provider;
  } else {
    rel = relation::peer;
  }
  return rel;
}

relation router::getRelation(router *r) const {
  if (!graph_ || !r) {
    return relation::unknown;
  }

  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (neighbor_id == r->getId()) {
      if (weight > 0)
        return relation::customer;
      if (weight < 0)
        return relation::provider;
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

std::vector<router *> router::learnRoute(route *r) {
  /*std::cout << "Recieved route: " << r->toString() << std::endl;*/
  if (!r || r->getDestination()->getId() == id_) {
    return {};
  }

  if (!proto_ || !proto_->accept(*r)) {
    return {};
  }

  auto existing = getRoute(r->getDestination()->getId());
  if (existing && !proto_->prefer(*existing, *r)) {
    return {};
  }

  if (rtable_.find(r->getDestination()->getId()) != rtable_.end()) {
    rtable_.erase(r->getDestination()->getId());
  }
  rtable_[r->getDestination()->getId()] = r;

  std::unordered_set<int> validRelations;
  for (const auto rel : {relation::customer, relation::peer, relation::provider}) {
    bool canForward = proto_->forward(r, rel);
    /*std::cout << "    Can forward (" << toString(rel) << "): " << canForward << std::endl;*/
    if (canForward) {
      validRelations.insert(static_cast<int>(rel));
    }
  }

  std::vector<router *> forwardTo;
  for (const auto &[neighbor_id, weight] : graph_->getNeighbors(id_)) {
    relation rel = weightToRelation(weight);
    /*std::cout << "    Neighbor: " << neighbor_id << " Relation: " << toString(static_cast<relation>(rel))*/
    /*          << std::endl;*/
    if (validRelations.find(static_cast<int>(rel)) != validRelations.end()) {
      if (auto node = graph_->getNode(neighbor_id)) {
        forwardTo.push_back(node.value().get());
      }
    }
  }
  return forwardTo;
}

std::unordered_map<std::string, route *> router::getRTable() const { return rtable_; }
std::string router::toString(relation r) const {
  switch (r) {
  case relation::customer:
    return "customer";
  case relation::peer:
    return "peer";
  case relation::provider:
    return "provider";
  default:
    return "unknown";
  }
}

void router::forceRoute(route *r) {
  if (rtable_.find(r->getDestination()->getId()) != rtable_.end()) {
    rtable_.erase(r->getDestination()->getId());
  }
  rtable_.insert({r->getDestination()->getId(), r});
}

void router::clearRTable() { rtable_.clear(); }
route *router::originate(router *nextHop) {
  if (!nextHop) {
    return nullptr;
  }

  auto *new_route = new route();
  new_route->setDestination(this);
  new_route->setPath({this, nextHop});
  new_route->setAuthenticated(true);
  new_route->setOriginValid(false);
  new_route->setPathEndValid(true);
  return new_route;
}

route *router::forward(route *r, router *nextHop) {
  if (!r || !nextHop) {
    std::cerr << "ForwardRoute: route or nextHop is null" << std::endl;
    return nullptr;
  }

  auto *newRoute = new route(*r);
  auto path = newRoute->getPath();
  path.push_back(nextHop);
  newRoute->setPath(path);
  newRoute->setOriginValid(false);
  newRoute->setPathEndValid(true);
  newRoute->setAuthenticated(true);
  return newRoute;
}

std::vector<router *> router::getNeighbors() {
  std::vector<router *> neighbors;
  for (const auto &[neighbor_id, _] : graph_->getNeighbors(id_)) {
    if (auto node = graph_->getNode(neighbor_id)) {
      neighbors.push_back(node.value().get());
    }
  }
  return neighbors;
}

std::vector<router *> router::getPeers() {
  std::vector<router *> peers;
  auto neighbors = graph_->getNeighbors(id_);
  for (const auto &[neighbor_id, weight] : neighbors) {
    if (weight == 0) {
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
