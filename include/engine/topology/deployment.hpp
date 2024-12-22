#pragma once
#ifndef DEPLOYMENT_HPP
#define DEPLOYMENT_HPP

class Topology;
class DeploymentStrategy {
public:
  virtual ~DeploymentStrategy() = default;
  virtual void deploy(Topology &topology) = 0;
  virtual void clear(Topology &topology) = 0;
  virtual bool validate(const Topology &topology) const = 0;
};

#endif // DEPLOYMENT_HPP
