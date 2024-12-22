#pragma once

#include "router.hpp"
#include <vector>
#include <string>

class Route;
class Router;

class Route {
public:
  Router *destination = nullptr;
  std::vector<Router *> path;

  bool authenticated = false;
  bool originValid = false;
  bool pathEndInvalid = false;

public:
  Route();
  Route(Router *destination, std::vector<Router *> path)
      : destination(destination), path(path) {}
  bool ContainsCycle() const;
  void ReversePath();
  std::string ToString() const;
};
