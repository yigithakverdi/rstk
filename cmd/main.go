package main

import (
	// "log"
	"fmt"
	// "os"
	"rstk/internal/engine"
  "rstk/internal/parser"
  "rstk/internal/graph"
  "os"
  // "github.com/dominikbraun/graph"
	"github.com/dominikbraun/graph/draw"
)

func main() {
	// Initialize the graph from AS relationships
	// g := engine.InitializeGraph("data/serial-2/20151201.as-rel2.txt", []string{"#"})
  
  asNumber := 1 

  parser := parser.Parser {
    AsRelFilePath: "data/serial-2/20151201.as-rel2.txt",
    BlacklistTokens: []string{"#"},
  }
  parser.ParseFile()
  g := graph.PopulateGraph(parser.AsRelationships)
  
  providers, _ :=  graph.GetProviders(g, asNumber)
  fmt.Printf("Providers: \n%v\n", providers)
  
  customer, _ :=  graph.GetCustomers(g, asNumber)
  fmt.Printf("Customer: \n%v\n", customer)

  // Initialize the simulation config
  simulationConfig := engine.InitializeSimulationConfig()
  
  // Generate the topology for the simulation
  engine.GenerateTopology(asNumber, g, simulationConfig.Topology)  
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
