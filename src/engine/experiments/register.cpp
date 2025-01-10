#include "engine/experiments/register.hpp"

expregister &expregister::instance() {
  static expregister instance;
  return instance;
}

std::vector<expinfo> expregister::getExperiments() const {
  std::vector<expinfo> result;
  for (const auto &[_, info] : experiments_) {
    result.push_back(info);
  }
  return result;
}

bool expregister::hasExperiment(const std::string &name) const {
  return experiments_.find(name) != experiments_.end();
}

std::unique_ptr<IExperiment> expregister::create(const std::string &name, std::queue<trial> &input,
                                                 std::queue<double> &output, std::shared_ptr<topology> t,
                                                 const std::vector<std::string> &args) {
  auto it = experiments_.find(name);
  if (it == experiments_.end()) {
    return nullptr;
  }
  return it->second.factory(input, output, t, args);
}
