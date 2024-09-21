# Things to Implement

- An RPKI infrastructure for the simulator. *Also RPKI is implemented simple in the paper*
- Two of the path-plausability algorithms, ASPA and AS-Cones (both of them uses RPKI) *Exactly as in the drafts*
- IETF draft for classification of route leak scenarios [Methods for Detection and Mitigaiton of BGP Route Leaks](https://datatracker.ietf.org/doc/draft-ietf-grow-route-leak-detection-mitigation/09/)
- For ASPA there are already implementations available, CA-sofware Krill, RP-software Routinator, OctoRPKI, etc.
- As an addition to paper we can implement the BGPsec protocol
- A simulation framework/engine for conducting these type of research, that considers following features
  - High scalability (novelties usually occurs from this, lack of scalability)
  - Realistic simulation of the Internet (and also from this, lack of low-level representation of the protocols)
  - Low level features like packet loss, latency, etc.
  - Implemented in C++ or Rust
- For simulation scenarios a set of policies are implemented hence a policy implementation might be good idea (such as for default ASes Gao-Rexford, for leaking routes, Route Leak Policy is desigend etc.)
- Object creation is another thing to implement besides policies, policies and objects are used to create scenarios they work in tandem
- Complexity analysis on BGPy and bpgsim on their scalability
- In order to provide a framework to test innovative algorithms, the National Institute of Standards and Technology (NIST) developed the BGP-Secure Routing Extension (SRx) software suite, which allows a fairly easy integration of new security algorithms [13]. Once the algorithm is implemented, the framework allows for native deployment on the host or the instantiation of a containerized environment in which it is possible to connect multiple routers. However, the creation of testbeds is limited to a predefined configuration file, which has to be manually created and can only be executed on a single host, limiting the testbed’s size BGPEval Rodday et al. 2023.
