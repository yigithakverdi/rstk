## Problem Statement: While tools like Kathará provide container-based network
emulation with support for various routing protocols, there is a lack of
comprehensive simulation frameworks that integrate traditional BGP routing.
Existing simulators do not sufficiently support the modelling of complex BGP
(BGPy) or require a great amount of computational resources (BGPEval). Each
alternative implementations of core elements in BGP require major changes in the
simulators

## Basic (Minimal) Simulator Features Minimal features can be described as, up
and running Kathara emulation, emulated topology is based on real-life AS
relation datasets (i.e CAIDA).

In the Kathara emulation, there are, adversarial nodes for simulating prefix
hijacks, and nodes that route leaks occur. To sum it up minimal simulator is
basically rough automation of the Kathara configurations given real-life
topology, and adversarial nodes that simulate BGP hijacks and nodes that have
route leaks.


## Simulator Full features Main operations will be done on the 'engine' and it
will use several functionalities abstracted through 'manager' such as, file
manager which will manage file operations, creating configurations files,
updating configuration files or Kathara manager which will open close Kathara
emulations etc. Here are the full list of features of the simulator:

- A plugin system that will make researchers implementations easier, possibly
with scripting languages such as lua, P4 or Python. (will provide implementation
of different varitions of BGP elements such as RPKI, ROV, BGP-Sec etc.)
- Built-in protocols such as ASPA, AS-Cones, BGP-Sec etc.
- A complete implementation of threat model, that will be integrated to
simulation environment with pre-defined scenarios, cases


## Optional Features (may or may not included in the final full implementations)
These features represents concpets that surfaced when conducting my literature
review. Most of the ideas probably fail to move to full development process
since most of them are bound to the inter-contianer communications so there is
significant trade-off on performance complexitiy:

- Control plane, data plane specific functionalities:
    - Using P4 to control the behaviour of the data plane within the simulator,
    by customizing packet processing pipelines
    - Simulation of how BGP security mechanisms affect packet forwarding at the
    data plane level
    - Using security enforcements directly in the data plane (such as BGP-iSec,
    BRP-RPKI and other alternative implementations)
    - DPDK for high-performance fast packer processing in user space

- Comparing DPDK, P4 implemetned simulator with the standard implementation
- Through data plane and control plane specific functions simulating DDoS
scenarios
- Showing how DPDK and P4 could potentially enable resource allocation
efficently against standard raw container emulated networks
- Using DPDK to create high-performance traffic generators for stress-testing
the data plane (this could be bounded the containers)
- Providing different module for control and data planes to simulate threat
modelling, an example could be introducing anomalous data plane processing, for
anomaly injection, or adversarial-policies such as based on control plane events
trigger data plane adjustments
- Parallelization could be done on the control and data plane operations,
especially intensive tasks could be concurrently processed
- Various additional testing such as how control plane protocols adapt to
adverse data plane conditions
- Feedback loops where control plane adjusts data plane configurations in
response to performance metrics
- Modularity for plug-and-play functionality where different data and control
plane implementations can be swapped
    - Most probably for this well-defined and develoepd API is required
    - Engine wll manage and control flow between modules
    - Dynamic loading, loading modules at runtime based on configuration files
    or command line arguments
    - Modules register themeselves with the core engine, providing their
    capabilities and interfaces
    - Eventually these modules need to have some sort of communication between
    them, this could be done using event handling, event bus, message queue,
    modules publish and subscribe to events (e.g. packet received, route
    updated) or direct method calls.

- No Kathara extended image for DPDK
- Great contribution could be implementation of SCION in the simulator, most of
the experimentations are done on the real-life network distributed accross
different universities in Europe.
- Enhancing simulator capabilities with DPDK
    - DPDK accelerated bridge
    - Inter-contianer communication via shared memory memif or vhost-user or
    similar technology to facilitate high-speed communication between
    container-based
- Implementing distributed portion of the simulator
    - Distributed verification algorithm to verify integrity of the simulator
    framework
- Interactive visual interface
- Configuration mining and real-time 'topology extractor' alternative to CAIDA

## Engine Responsabilities Engine will sit between user space and Kathara, works
at lower level, will act as the executor that carries out the instructions
provided by the controller. Core operations of the engine:

- Generate topology based on the graph information (CAIDA and other sources)
- Generate Kathara configuration based on the graph
- Network data assignments (IP, FRR configurations, RPKI, BGP configurations
etc.)

- Container management and optimization
- Resource management, CPU, memory, network
- Performance profiling
- Management of threat models, adversarial
- Management of simulation configuratiosn, RPKI deployment, ASPA deployment etc.
- Data collection, logs, network traffic
- Feedback loop, sending logs, status errors to controller

Future implementations that might or might not be implemented (these are
optional features):

- Implementing parallel simulation (there was paper on allowing multiple
parallel simulations on a single BGP router)
- Plugin system for adding new BGP research implementations easily, protocols,
attack scenarios w/o modifying
- High performance network computing (kernel bypass, macvlan, ipvlan) for
Kathara and Docker

## Controller Responsabilities Operates at a higher level, dealing with user
interactions and overall simulation commands. Acts as the command center where
users specify what they wnat to simulate. Does not handle the technical
execution details. It does following:

- Processing command line arguments, configuration files and API requests
- Providing feedback to users, logs, status errors
- Scenario selection, allowing users to choose from pre-defined scenarios or
create customized parameters for new scenarios
- Controlling flow of simulation, starting, stopping pausing, resuming
- Loading and validating user-provided configurations or settings before passing
them to engine

## Performance Optimizations It is requried to simulate large number of ASes, it
heavily depends on following factors:

- Topology size
- How much control over Kathara and containers we have
- How well profiling and resource allocation is done Following is the
optimization that we can try:
- Lightweight containers, reducing overhead, container optimization (limiti
certain actions etc.)
- Limiting resources allocated to containers
- Lazy initialization of containers, only start container when needed
- Storing results of expensive operations (if there are)
- Reducing number of read, write to disk
- Simulating non-critical ASes as simple route models rather then emulating full
BGP router (DRMSim, BGPy, and Path Plausability paper have useful resource on
this)

## Things That Simulator Will Not Docker

- It will not do any low-level network operations such as packet forwarding,
packet processing, packet maniupulation, packer inspection etc. (no routing, no
firwall implementations, those will be done using already existing routing
daemons)
- It will not implement BGP related operations, routing, message generation,
parsing, validation
- It will not handle any RPKI implementations
- Already implemented things will not be implemented such as RPKI, FRR,
Routinator, only specific experiemntal implementations would be included such as
ASPA, AS-Cones, BGP-iSec etc.

## For Code Style Following documentation is referenced [Go Style
Guide](https://docs.scion.org/en/latest/dev/style/go.html)


## Scenario Structure Scenario name:      Could be anything, but should obey a
specific syntax

Description:        Should describe clearly what this scenario does with inputs,
outputs and operations etc.   

Inputs:             Arguments that will configure the topology, and fed into the
deployment strategy such as, object/policy deploymet percentage

Outputs:            This could be anything received from the topology, such as
logs, route announcements, decline route annoncements, route information on
routing tables, or some custom evalution metrics such as success rate of an
hijack etc.

Threat Model:       Could define specific threat models, such as say, instead of
(Optional)          defining a single hijacker, more complicated, organized
attack scenarios could be defined, such as, multiple simultaneous hijacker, or
leaking routes etc.

## Engine with Internal Bus and Pipeline One way to create a true “Engine” is to
design it around a rich, extensible core that orchestrates sophisticated
processes rather than simply invoking methods. First, establish a central
pipeline: collect inputs, process them in multiple stages, and output results in
a structured way. Each stage in the pipeline can be a separate module—parsers,
analyzers, orchestrators, or optimizers—and the engine core manages coordination
among these modules. By treating every function in the simulation as part of a
bigger pipeline, you lay the groundwork for high scalability and modularity.

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
complexity and sophistication of modern engines.
