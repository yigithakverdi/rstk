#include "engine/topology/topology.hpp"
#include <iostream>
int main() {
  topology t;
  auto r = t.getRouter("AS6");
  std::cout << "Router protocol: " << r->getProtocol()->name() << std::endl;
  std::cout << r->getId() << std::endl;
  t.findRoutesTo(r.get());

  std::cout << "Testing route tables:" << std::endl;
  auto r18t = t.getRouter("AS18")->getRTable();
  auto r4t = t.getRouter("AS4")->getRTable();

  for (const auto &[dest, route] : r18t) {
    std::cout << "AS6 via " << route->toString() << std::endl;
  }

  for (const auto &[dest, route] : r4t) {
    std::cout << "AS6 via " << route->toString() << std::endl;
  }

  return 0;
}
