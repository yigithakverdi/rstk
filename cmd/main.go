package main

import (
  "rstk/internal/parser"
  "rstk/internal/engine/graph"
)



func main() {
  
  // asNumber := 1 
  
  // Temporarly parser logic is moved here
  parser := parser.Parser {
    AsRelFilePath: "data/serial-2/20151201.as-rel2.txt",
    BlacklistTokens: []string{"#"},
  }
  parser.ParseFile()
  


  g, _ := graph.PopulateGraph(parser.AsRelationships)
  graph.PrintGraph(g)

  // Testing routing functionalities on the created AS graph (without any policy)
  // First lets choose which AS to start the routing from
  // asNumber := 1

  // Get the router by AS number
  // router, err := graph.GetRouterByASNumber(asNumber)
  // if err != nil {
  //   log.Fatalf("Failed to get router by AS number: %v", err)
  // }



  


  // g := graph.BuildReachabilityGraph(ases)
  //
  // graph.PrintGraph(g)



 
  // For testing routing only operating on a small subset of the graph
  // simulationConfig := engine.InitializeSimulationConfig()
  // simulationConfig.Topology = engine.TopologyConfig {
  //   Depth: 2,
  //   BranchingFactor: 2,
  //   Redundancy: false,
  //   IPVersion: 4,
  //   IPBase: "",
  //   IPMap: map[int]string{},
  // }
  //
  // topology := engine.GenerateTopology(asNumber, g, simulationConfig.Topology) 
  // engine.PrintTopology(topology)


  // file, _ := os.Create("./mygraph.gv")
  // _ = draw.DOT(topology, file)

	// Generate the topology for the simulation
	// asNumber := 1
	// topology := engine.GenerateTopology(asNumber, g, simulationConfig.Topology)
	//  fmt.Printf("%v", topology)

	// Generate and handle collision domains
	// err = engine.GenerateCollisionDomains(simulationConfig.KatharaConfigPath, topology)
	// if err != nil {
	// 	log.Fatalf("Failed to generate collision domains: %v", err)
	// }
	//
	// // Assign IP addresses for each on the generated topology
	// engine.GenerateRouterIPs(topology)
	//
	// // Generate device startup configurations
	// engine.GenerateFRRConfigurations(topology)
}
