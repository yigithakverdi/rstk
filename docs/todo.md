## Refactoring
- [+] Use logger implementation (removed all logging temporarly, will
  implementation it along the way when implementing CLI and other protocols and
  implement the rest when deubgging problems especially for ASPA)
- [+] Create an interactive CLI, with UI elements such as loading bar, spinners
  etc.
- [+] Implement ASCones and BGP-Sec (directly map them from Python)
- [+] Provide an interactive, visualization of the following elements:
        - Viewing topology
        - Viewing routers (routing tables, protocols they are using, protocol
          state etc.)
        - More tidied up visualizaiton of logs, such as rather then showing all
          logs for whole routing operations with commands only inspect related
          routers routing operations logs
- [+] Final refactoring on the experiment logic 
- [ ] Engine implementation and integreation to CLI
- [ ] Implement the whole CAIDA dataset, using the whole dataset

## General Things to Consider
- [ ] Deployment strategies can be mediated through ProtocolFactory, currently
  ProtocolFactory do not have any usage under protocol instantiation, however
  `CreateProtocol` method under ProtocolFactory is a good place to mediate
  deployment strategies. This could be further aruged
- [ ] Some of the test cases are considered under wrong context such as topology
  some of the test cases should be grouped or considered under ASPA tests, or
  some of the complex routing either should be under base test case or router
  test cases, though these categorization is not solid yet, test case outcomes
  should not changed after refactoring since context of the test will not change
  when moved from topology_test to aspa_test or base_test since the topology
  will be same anyway
- [ ] `AdjMap` and `PredMap` are not utilized and not poopulated however they
  were accessed under the `Hijack` method, thus problems were occurring removed
  this usage under `Hijack` and using neighbor method, usage of these fields
  might or might not be needed in the future 
- [ ] Later on a clean-up on where classes needs to recide and where they should
  be included from needed. For example, currently `topology.hpp` is cluttered
  too much with bunch of classes, these could be separated in their own
  respective header files later on
- [ ] There might be need of `ResetPlugin`, `ResetPolicy` and `ResetProtocol`
  methods under topology to reset all routers plugins, policies and protocols
- [ ] Protocol specific commands, such as when implementing a protocol, it's
  commands should be implemented under the protocol classes as well


Temporarl storing the experiment old template version here: ```cpp // To
implement experiments, use the following template class NewExperiment : public
ExperimentWorker { public: NewExperiment(std::queue<Trial>& input,
std::queue<double>& output, std::shared_ptr<Topology> topology, int param1,
float param2) : ExperimentWorker(input, output, topology), param1_(param1),
param2_(param2) {} protected: double runTrial(const Trial& trial) override { //
Implementation } private: int param1_; float param2_; };

// In initialization code: REGISTER_EXPERIMENT( NewExperiment, "new-experiment",
"Description of new experiment", "param1: Description of param1", "param2:
Description of param2"); ```


