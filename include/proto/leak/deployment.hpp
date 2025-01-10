#pragma once
#ifndef LEAKDEPLOYMENT_HPP
#define LEAKDEPLOYMENT_HPP

#include "engine/topology/deployment.hpp"

class RandomLeakDeployment : public IDeployment {
public:
  RandomLeakDeployment(double objectPercentage, double policyPercentage, topology &t);
  void deploy(topology &t) override;
  void clear(topology &t) override;
  bool validate(topology &t) override;

private:
  double objectPercentage_;
  double policyPercentage_;
  topology &t_;
};

class SelectiveLeakDeployment : public IDeployment {
public:
  SelectiveLeakDeployment(double objectPercentage, double policyPercentage, topology &t);
  void deploy(topology &t) override;
  void clear(topology &t) override;
  bool validate(topology &t) override;

private:
  double objectPercentage_;
  double policyPercentage_;
  topology &t_;
};

#endif // LEAKDEPLOYMENT_HPP
