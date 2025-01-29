#include "engine/topology/topology.hpp"
#include <iostream>
int main() {
  topology t;
  auto r = t.getRouter("AS1");

  std::cout << "Route tables before hijack" << std::endl;
  t.findRoutesTo(r.get());
  for (const auto &[hash, weight] : t.getGraph()->getNodes()) {
    t.getRouter(hash)->printRTable();
  }
  
  std::cout << std::endl;
  std::cout << "Route tables after hijack" << std::endl;
  router *victim = t.getRouter("AS1").get();
  router *attacker = t.getRouter("AS6").get();
  t.hijack(victim, attacker, 1);
  for (const auto &[hash, weight] : t.getGraph()->getNodes()) {
    t.getRouter(hash)->printRTable();
  }

  return 0;
}
