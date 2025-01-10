#pragma once
#ifndef ASAPDEPLOYMENT_HPP 
#define ASPADEPLOYMENT_HPP

#include "engine/topology/deployment.hpp"

class RandomDeployment : public IDeployment {
public:
  RandomDeployment(double objectPercentage, double policyPercentage, topology &t);
  void deploy(topology &t) override;
  void clear(topology &t) override;
  bool validate(topology &t) override;

private:
  double objectPercentage_;
  double policyPercentage_;
  topology &t_;
};

class SelectiveDeployment : public IDeployment {
public:
  SelectiveDeployment(double objectPercentage, double policyPercentage, topology &t);
  void deploy(topology &t) override;
  void clear(topology &t) override;
  bool validate(topology &t) override;

private:
  double objectPercentage_;
  double policyPercentage_;
  topology &t_;
};

#endif // DEPLOYMENT_HPP
