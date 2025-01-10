#pragma once
#ifndef BASEDEPLOYMENT_HPP 
#define BASEDEPLOYMENT_HPP 

#include "engine/topology/deployment.hpp"

class BaseDeployment : public IDeployment {
public:
  BaseDeployment(topology &t);
  void deploy(topology &t) override;
  void clear(topology &t) override;
  bool validate(topology &t) override;
private:
  topology &t_;
};

#endif // DEPLOYMENT_HPP
