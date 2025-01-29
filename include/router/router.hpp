// #pragma once
#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "engine/rpki/rpki.hpp"
#include "graph/graph.hpp"
#include "proto/proto.hpp"
#include "router/route.hpp"
#include <string>
#include <unordered_map>
#include <vector>

enum class relation { customer = -1, peer = 0, provider = 1, unknown = 99 };

class router {
public:
  router();
  router(std::string id, std::string name, std::shared_ptr<graph<std::shared_ptr<router>>> g);
  router(std::string id, std::string name, std::shared_ptr<graph<std::shared_ptr<router>>> g, int tier,
         std::unique_ptr<IProto> proto, std::shared_ptr<rpki> rpki);
  ~router();

  route *getRoute(std::string destination) const;
  relation getRelation(router *r) const;
  std::vector<router *> learnRoute(route *r);
  void forceRoute(route *r);
  void clearRTable();
  route *originate(router *r);
  route *forward(route *r, router *nextHop);
  std::vector<router *> getPeers();
  std::vector<router *> getProviders();
  std::vector<router *> getCustomers();
  std::vector<router *> getNeighbors();
  std::string toString() const;
  std::string getId() const;
  std::string getName() const;
  void printRTable() const;
  std::shared_ptr<rpki> getRPKI() const;
  std::unordered_map<std::string, route *> getRTable() const;
  int getTier() const;
  int getNumber() const;
  void setProtocol(std::unique_ptr<IProto> proto);
  void setTier(int tier);
  void setRPKI(std::shared_ptr<rpki> rpki);
  void setName(std::string name);
  IProto *getProtocol() const;
  relation weightToRelation(int weight);

  std::string toString(relation r) const;

private:
  std::unordered_map<std::string, route *> rtable_;
  std::shared_ptr<graph<std::shared_ptr<router>>> graph_;
  std::string name_;
  std::string id_;
  int tier_;
  std::unique_ptr<IProto> proto_;
  std::shared_ptr<rpki> rpki_;
};

#endif // ROUTER_HPP
