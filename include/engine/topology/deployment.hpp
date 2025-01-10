#pragma once
#ifndef DEPLOYMENT_HPP
#define DEPLOYMENT_HPP

class topology;
class IDeployment {
public:
  IDeployment(topology &t);
  virtual ~IDeployment();
  virtual void deploy(topology &t) = 0;
  virtual void clear(topology &t) = 0;
  virtual bool validate(topology &t) = 0;

private:
  topology &t_;
};

#endif // DEPLOYMENT_HPP
