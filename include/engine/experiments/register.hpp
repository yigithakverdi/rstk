#pragma once
#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class IExperiment;
using expfactory = std::function<std::unique_ptr<IExperiment>(
    std::queue<trial> &, std::queue<double> &, std::shared_ptr<topology>, const std::vector<std::string> &)>;

struct expinfo {
  std::string name;
  std::string description;
  std::vector<std::string> parameters;
  expfactory factory;
};

class expregister {
public:
  static expregister &instance();

  void registerExperiment(const std::string &name, const std::string &description,
                          const std::vector<std::string> &parameters, expfactory factory) {
    expinfo info{name, description, parameters, factory};
    experiments_[name] = info;
  }

  std::vector<expinfo> getExperiments() const;
  bool hasExperiment(const std::string &name) const;
  std::unique_ptr<IExperiment> create(const std::string &name, std::queue<trial> &input,
                                      std::queue<double> &output, std::shared_ptr<topology> t,
                                      const std::vector<std::string> &args);

private:
  std::unordered_map<std::string, expinfo> experiments_;
};

#define REGISTER_EXPERIMENT(TYPE, NAME, DESC, ...)                                                           \
  expregister::instance().registerExperiment(                                                                \
      NAME, DESC, std::vector<std::string>{__VA_ARGS__},                                                     \
      [](std::queue<trial> &input, std::queue<double> &output, std::shared_ptr<topology> t,                  \
         const std::vector<std::string> &args) { return std::make_unique<TYPE>(input, output, t); })

#endif // REGISTER_HPP
