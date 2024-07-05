# Things to Implement

- An RPKI infrastructure for the simulator. *Also RPKI is implemented simple in the paper*
- If there are additional security mechanisms already adopted with 45% similar to RPKI then we need to implement thoose as well
- Two of the path-plausability algorithms, ASPA and AS-Cones (both of them uses RPKI) *Exactly as in the drafts*
- IETF draft for classification of route leak scenarios [Methods for Detection and Mitigaiton of BGP Route Leaks](https://datatracker.ietf.org/doc/draft-ietf-grow-route-leak-detection-mitigation/09/)
  - For ASPA there are already implementations available, CA-sofware Krill, RP-software Routinator, OctoRPKI, etc.
- As an addition to paper we can implement the BGPsec protocol
- A simulation framework/engine for conducting these type of research, that considers following features
  - High scalability (novelties usually occurs from this, lack of scalability)
  - Realistic simulation of the Internet (and also from this, lack of low-level representation of the protocols)
  - Low level features like packet loss, latency, etc.
  - Implemented in C++ or Rust
  - ... more ideas needed
- For simulation scenarios a set of policies are implemented hence a policy implementation might be good idea (such as for default ASes Gao-Rexford, for leaking routes, Route Leak Policy is desigend etc.)
- Object creation is another thing to implement besides policies, policies and objects are used to create scenarios they work in tandem
- Complexity analysis on BGPy and bpgsim on their scalability
- In order to provide a framework to test innovative algorithms, the National Institute of Standards and Technology (NIST) developed the BGP-Secure Routing Extension (SRx) software suite, which allows a fairly easy integration of new security algorithms [13]. Once the algorithm is implemented, the framework allows for native deployment on the host or the instantiation of a containerized environment in which it is possible to connect multiple routers. However, the creation of testbeds is limited to a predefined configuration file, which has to be manually created and can only be executed on a single host, limiting the testbed’s size BGPEval Rodday et al. 2023

## Novelties To Add

- More scalable and research focused simulator/engine for intra/inter-domain routing security  scenarios (could be worth to not keep within security context)
- Hybrid Security Model (very hard to come up with secure protocol PhD level, requires solid understanding of cryptography, internet protocols, protocols design, etc.)
- Deep learning/Machine learning based detection for BGP behavior, comparison to ASPA and AS-Cones
- Using Deep learning/Machine learning to find or possibly RL to find best path for routing, considering which of the ASes deployed RPKI etc. and other indications similar to this
- They used ASPA verification for AS-Cones verification because AS-Cones verification is premature and incomplete, we can further work on bunch of trial-and-error experimentation to find ideal AS-Cones verification
- A plugin to extend Kathara??
- There have been research proposals to identify ROV-filtering ASes on the control plane [19] and the data plane [20], but it remains a challenge due to the limited visibility and different sets of vantage points. It could be useful resource for producing novelties from here
- As also mentioned in the thing to implement for simulation framework/engine we can represent more details of BGP a low-level implementations of BGP
- Recent drafts on BGP security such as:
  - [SPL verification](https://datatracker.ietf.org/doc/draft-ietf-sidrops-spl-verification/) 14 June 2024
  - [Same-Origin-Policy for RPKI](https://datatracker.ietf.org/doc/draft-ietf-sidrops-rrdp-same-origin/) Apr 2024 - Jun 2024 (ongoing)
- ~~Test the scalability of Kathara and suggest improvements on it~~ This is already done check out the Kathara website, paper and Megaolos paper on the scalability experiments
- Here is another novel work direclty references our paper [BGP-iSec: Improved Security for Internet Routing Agains Post-ROV Attacks](https://www.semanticscholar.org/paper/BGP-iSec%3A-Improved-Security-of-Internet-Routing-Morris-Herzberg/a7587c2ebcadcd71876ef0901e73cdaf276330a9)

Generally it seems that overall research field is focused on security of BGP protocols, routing attacks, prefix hijacking and other security issues, such as one study that references the BGP-iSec, named [The CURE to Vulnerabilitiesi in RPKI Validation](https://www.semanticscholar.org/paper/The-CURE-To-Vulnerabilities-in-RPKI-Validation-Mirdita-Schulmann/e57a9b5b44d3f322d89a82ee1a765ae9fccdd0d4#citing-papers) also focuses on the vulnerabilities on the RPKI side

Also another research highly influenced by the CURE is [Byzantine-Secure Relying Party for Resilient RPKI](https://www.semanticscholar.org/paper/Byzantine-Secure-Relying-Party-for-Resilient-RPKI-Friess-Mirdita/9f4812d7e8bf6ddeefe18c7dde80c90930571949) is also about RPKI security etc.

By 

## How simualtions will be done?

Using graph topology?
Using multiple virtual machines?

## Alternatives

- Megalos: A Scalable Architecture for the Virtualization of Large Network Scenarios [Scazzariello 2021 et al.]
