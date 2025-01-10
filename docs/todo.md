## Refactoring
- [+] Use logger implementation (removed all logging temporarly, will
  implementation it along the way when implementing CLI and other protocols and
  implement the rest when deubgging problems especially for ASPA)
- [+] Create an interactive CLI, with UI elements such as loading bar, spinners
  etc.
- [+] Implement ASCones and BGP-Sec (directly map them from Python)
- [+] Provide an interactive, visualization of the following elements:
- Viewing topology
- Viewing routers (routing tables, protocols they are using, protocol state
  etc.)
- More tidied up visualizaiton of logs, such as rather then showing all logs for
  whole routing operations with commands only inspect related routers routing
  operations logs
- [+] Final refactoring on the experiment logic 
- [+] Engine implementation and integreation to CLI
- [+] Implement the whole CAIDA dataset, using the whole dataset

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
- [ ] Experiments could run on separate threads, and progress could be shown on
  the CLI row by row with progress bar for each
- [ ] Currently origin authentication and path-end validation does not checked
  out or used in the experiments, these could be implemented as well and used
  under protocols such as BGP-Sec

## Improved Engine Design "One way to create a true “Engine” is to design it
around a rich, extensible core that orchestrates sophisticated processes rather
than simply invoking methods. First, establish a central pipeline: collect
inputs, process them in multiple stages, and output results in a structured way.
Each stage in the pipeline can be a separate module—parsers, analyzers,
orchestrators, or optimizers—and the engine core manages coordination among
these modules. By treating every function in the simulation as part of a bigger
pipeline, you lay the groundwork for high scalability and modularity.

Next, add an event-driven or message-based communication system. Instead of
direct function calls, modules subscribe and publish to an internal bus (or
message queue). This allows them to share data, send notifications, or request
services from other components without creating tight couplings. It also makes
it possible to dynamically add, remove, or update modules during runtime. You
gain the same level of extensibility that big search engines and recommendation
engines often have—dropping in new pipelines or upgrading modules without
rewriting the entire system.

Include an intelligence or reasoning layer—some logic capable of interpreting
the higher-level context of the simulation. For example, if the simulator is
generating a large number of route leaks, this logic could automatically adjust
or reconfigure certain BGP parameters. Analyzing logs and telemetry data can
direct fine-grained re-optimization, much like how large AI-based systems
iteratively refine their models.

Finally, expose a well-defined plugin API that allows developers or researchers
to hook into the core engine. Similar to how an advanced search or
recommendation engine supports user-defined ranking methods or specialized data
filters, a plugin system encourages a vibrant ecosystem of new protocols,
adversarial behaviors, or large-scale data collectors. By building out the
engine with these layers—pipeline orchestration, an event-driven architecture,
an intelligent reasoning layer, and a plugin-friendly interface—you shift from a
simple aggregator of functions to a robust, evolving system on par with the
complexity and sophistication of modern engines."

## Pipeline
For pipeline there could be anchor events, these anchor events are necessary stages
or modules that are "anchored" somewhere in the pipeline, such as sanity checks, 
validation, or terminal stages, such as starting and ending module/stage of the
pipline, this also could indicate finalizaiton of the program or some phase of the
program

