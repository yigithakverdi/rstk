#ifndef ROUTEHIJACKEDEXPERIMENT_HPP
#define ROUTEHIJACKEDEXPERIMENT_HPP

#include <queue>
#include <memory>
#include "engine/experiments/experiments.hpp"
#include "engine/topology/topology.hpp"

class RouteHijackExperiment : public ExperimentWorker {
public:
    RouteHijackExperiment(std::queue<Trial> &input_queue,
                          std::queue<double> &output_queue,
                          std::shared_ptr<Topology> topology,
                          int n_hops);

protected:
    double runTrial(const Trial &trial) override;
    size_t calculateTotalTrials() const override;
    void initializeTrial() override;

private:
    int n_hops_;
};

#endif // ROUTEHIJACKEDEXPERIMENT_HPP

