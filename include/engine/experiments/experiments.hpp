#pragma once
#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include "engine/topology/topology.hpp"
#include <queue>
#include <string>

class topology;
class router;
class route;

struct trial {
  std::shared_ptr<router> victim;
  std::shared_ptr<router> attacker;
};

class IExperiment {
public:
  IExperiment(std::queue<trial> &input, std::queue<double> &output, std::shared_ptr<topology> t);
  virtual ~IExperiment() = 0;
  virtual void stop() = 0;
  virtual void run() = 0;
  virtual size_t total() const = 0;
  virtual double runTrial(const trial &t) = 0;
  virtual void init() = 0;
  virtual void setupTopology() = 0;

private:
  size_t total_;
  size_t current_{0};
  std::chrono::steady_clock::time_point start_;
  std::string formatDuration(int seconds);
  std::queue<trial> &input_;
  std::queue<double> &output_;
  std::shared_ptr<topology> topology_;
  bool stopped_{false};
};

#endif // EXPERIMENTS_HPP
