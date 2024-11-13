# Things to Do
- [ ] Improvements on `parser.go`
      - [ ] Parser could be modularized a bit more seperating read and parse logics
      - [ ] Adding more configuration options such as seperater token etc.
      - [ ] Making parser more optimize with concurrency
      - [ ] Adding support for different file formats
- [ ] Improvements on `graph.go`
      - [ ] AS relation data structure could be adjusted to something more suitable rather then two different maps holding provider customer relations on both ways
- [ ] Concrete error handling and logging
- [ ] Defining specification of the engine more clearly
- [ ] IP management
      - [ ] Subnet per link instead of IP per nodes in the link
      - [ ] There are some redundant operations especially on the funciton `GenerateRouterIPs`
- [ ] There is a design conflict about topology and graph logic, either merge them or abstract graph operations
      solely based on the graph data structures e.g. neighbors, edges, vertex, sub graph, adjacency graph etc.
      and topology should implement more network related operations such as peer, customer, provider relationshisp
      ASes logic etc.
- [ ] Interactive CLI
- [ ] Fully testing routing logic on a simple test topology
- [ ] Implementing ASPA 
- [ ] Implementing Prefix Hijacking

# Plan
Initial design should be as close as possible to fully abstracted BGP routing simulator only, routing should be done on ASes only > Try to turn this into hybrid approach as outlined on the design.md && Try to replicate ASPA deployment startegies > Alternative approach > In-network part
