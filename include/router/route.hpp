#pragma once
#ifndef ROUTE_HPP
#define ROUTE_HPP

#include <string>
#include <vector>

class router;
class route {
public:
  route();
  ~route();
  bool hasCycle() const;
  void reversePath();

  std::string toString();
  std::string toPathString();

  const std::vector<router *> &getPath() const { return path; }
  router *getDestination() const { return destination; }
  bool isAuthenticated() const { return authenticated; }
  bool isOriginValid() const { return originValid; }
  bool isPathEndValid() const { return pathEndValid; }
  void setPath(const std::vector<router *> &path) { this->path = path; }
  void setDestination(router *destination) { this->destination = destination; }
  void setAuthenticated(bool authenticated) { this->authenticated = authenticated; }
  void setOriginValid(bool originValid) { this->originValid = originValid; }
  void setPathEndValid(bool pathEndValid) { this->pathEndValid = pathEndValid; }

private:
  router *destination;
  bool authenticated;
  bool originValid;
  bool pathEndValid;
  std::vector<router *> path;
};

#endif // ROUTE_HPP
