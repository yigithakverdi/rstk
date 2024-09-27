package main

import (
	"log"
	"rstk/internal/engine"
	"rstk/internal/engine/manager"

	"rstk/internal/graph"
	"rstk/internal/parser"
)

func main() {
	parser := parser.Parser{
		AsRelFilePath:   "data/serial-2/20151201.as-rel2.txt",
		BlacklistTokens: []string{"#"},
	}

	// Initializing the manager package
	manager.Init()

	// Parse the file and store the relationships in the parser struct
	parser.ParseFile()

	// Populate the graph with the relationships
	g := graph.PopulateGraph(parser.AsRelationships)

	// AS number
	asNumber := 1

	// Topology configurations
	topologyConfig := engine.TopologyConfig{
		Depth:           2,
		BranchingFactor: 1,
		Redundancy:      false,
	}

	// Simulation configurations
	simulationConfig := engine.SimulationConfig{
		Global:       engine.GlobalConfig{},
		SimulationID: engine.GenerateSimulationID(),
	}

	manager.CreateSimulationDirectory(simulationConfig.SimulationID)
	configFilePath, err := manager.CreateKatharaConfigFile(simulationConfig.SimulationID)

	if err != nil {
		log.Fatalf("Failed to create configuration file %s: %v", configFilePath, err)
	}

	simulationConfig.KatharaConfigPath = configFilePath

	topology := engine.GenerateTopology(asNumber, g, topologyConfig)
	engine.GenerateCollisionDomains(simulationConfig.KatharaConfigPath, topology)
}
