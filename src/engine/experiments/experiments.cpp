#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"
#include <memory>

IExperiment::~IExperiment() {}
IExperiment::IExperiment(std::queue<trial> &input, std::queue<double> &output, std::shared_ptr<topology> t)
    : topology_(t), input_(input), output_(output), stopped_(false) {}



