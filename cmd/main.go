package main

import (
	// "log"
	"fmt"
	// "os"
	// "rstk/internal/engine"
  "rstk/internal/parser"
  "rstk/internal/graph"
	// "github.com/dominikbraun/graph"
	// "github.com/dominikbraun/graph/draw"
)

func main() {
	// Initialize the graph from AS relationships
	// g := engine.InitializeGraph("data/serial-2/20151201.as-rel2.txt", []string{"#"})
  
  asNumber := 393809

  parser := parser.Parser {
    AsRelFilePath: "data/serial-2/20151201.as-rel2.txt",
    BlacklistTokens: []string{"#"},
  }
  parser.ParseFile()
  g := graph.PopulateGraph(parser.AsRelationships)
  
  providers, _ :=  graph.GetProviders(g, asNumber)
  fmt.Printf("%v", providers)
  
  customer, _ :=  graph.GetCustomers(g, asNumber)
  fmt.Printf("\n%v", customer)

	//  adjMap, err := g.AdjacencyMap()
	//
	//  if err != nil {
	//    panic(err)
	//  }
	//
	//  if neighbours, exists := adjMap[1]; exists {
	//    for _, neighbour := range neighbours {
	//      fmt.Println(neighbour)
	//    }
	//  } else {
	// 	fmt.Println("Vertex 1 not found.")
	// }
 
  // file, _ := os.Create("./mygraph.gv")
  // _ = draw.DOT(gTemp, file)
  // Initialize the simulation configuration
	// simulationConfig := engine.InitializeSimulationConfig()

  // Set up simulation directory and config file
	// err := engine.SetupSimulationDirectory(&simulationConfig)
	// if err != nil {
	// 	log.Fatalf("Setup failed: %v", err)
	// }
	
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
