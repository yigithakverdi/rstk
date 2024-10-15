package main

import (
	"log"
	"rstk/internal/engine"
)

func main() {
	// Initialize the graph from AS relationships
	g := engine.InitializeGraph("data/serial-2/20151201.as-rel2.txt", []string{"#"})

	// Initialize the simulation configuration
	simulationConfig := engine.InitializeSimulationConfig()

	// Set up simulation directory and config file
	err := engine.SetupSimulationDirectory(&simulationConfig)
	if err != nil {
		log.Fatalf("Setup failed: %v", err)
	}

	// Generate the topology for the simulation
	asNumber := 1
	topology := engine.GenerateTopology(asNumber, g, simulationConfig.Topology)

	// Generate and handle collision domains
	err = engine.GenerateCollisionDomains(simulationConfig.KatharaConfigPath, topology)
	if err != nil {
		log.Fatalf("Failed to generate collision domains: %v", err)
	}

	// Assign IP addresses for each on the generated topology
	engine.GenerateRouterIPs(topology)

	// Generate device startup configurations
	engine.GenerateFRRConfigurations(topology)
}
