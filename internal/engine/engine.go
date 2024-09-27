package engine

import (
	"fmt"
	"log"
	"rstk/internal/engine/manager"
	"rstk/internal/graph"
	"rstk/internal/parser"

	"github.com/google/uuid"
)

type EdgeConfig struct {
	Source          string
	Destination     string
	Bandwidth       int
	Latency         int
	CollusionDomain string
}

type NodeConfig struct {
	CPU       int
	Memory    int
	ID        string
	IP        string
	Protocol  string
	Interface string
}

type GlobalConfig struct {
	Nodes []NodeConfig
	Edges []EdgeConfig
}

type TopologyConfig struct {
	Depth           int
	BranchingFactor int
	Redundancy      bool
}

type SimulationConfig struct {
	Topology          TopologyConfig
	Global            GlobalConfig
	SimulationID      string
	KatharaConfigPath string
}

// Function for generating a unique simulation ID using UUID.
func GenerateSimulationID() string {
	return uuid.New().String()
}

// Initializes and parses the AS relationship file, returning the populated graph.
func InitializeGraph(asRelFilePath string, blacklistTokens []string) graph.Graph {
	parser := parser.Parser{
		AsRelFilePath:   asRelFilePath,
		BlacklistTokens: blacklistTokens,
	}
	parser.ParseFile()
	return graph.PopulateGraph(parser.AsRelationships)
}

// Initializes and returns the simulation configuration.
func InitializeSimulationConfig() SimulationConfig {
	return SimulationConfig{
		Topology: TopologyConfig{
			Depth:           2,
			BranchingFactor: 1,
			Redundancy:      false,
		},
		Global:       GlobalConfig{},
		SimulationID: GenerateSimulationID(),
	}
}

// Initializes and creates the Kathara configuration file and directory structure.
func SetupSimulationDirectory(simulationConfig *SimulationConfig) error {
	manager.CreateSimulationDirectory(simulationConfig.SimulationID)
	configFilePath, err := manager.CreateKatharaConfigFile(simulationConfig.SimulationID)
	if err != nil {
		return fmt.Errorf("failed to create configuration file %s: %w", configFilePath, err)
	}
	simulationConfig.KatharaConfigPath = configFilePath
	return nil
}

func GenerateTopology(asNumber int, g graph.Graph, config TopologyConfig) map[int]graph.Node {
	visited := make(map[int]bool)
	topology := make(map[int]graph.Node)
	traverseASMap(asNumber, config.Depth, config.BranchingFactor, visited, g, topology)
	return topology
}

// Function for generating collision domains for the topology. It takes the simulation ID and the topology as input.
// It iterates over each node in the topology and creates collision domains for each of its neighbors.
func GenerateCollisionDomains(katharaConfigPath string, topology map[int]graph.Node) error {
	for _, node := range topology {
		collisionDomains := manager.CreateCollisionDomains(node)

		log.Printf("Collision domains for AS %d", node.ASNumber)
		for domain := range collisionDomains {
			log.Printf("%s", domain)
		}
		err := manager.WriteCollisionDomainsToKatharaConfigFile(katharaConfigPath, collisionDomains)
		if err != nil {
			log.Fatalf("Failed to write collision domains to file %s: %v", katharaConfigPath, err)
		}
	}
	return nil
}

// A helper function to select up to N customers, N peers, and N providers from the node.
func limitedNode(node graph.Node, branchFactor int) graph.Node {
	// Limit to `branchFactor` customers, peers, and providers
	var customer, peer, provider []int

	// Limit to the first `branchFactor` elements (or less if not enough exist)
	if len(node.Customer) > branchFactor {
		customer = node.Customer[:branchFactor]
	} else {
		customer = node.Customer
	}

	if len(node.Peer) > branchFactor {
		peer = node.Peer[:branchFactor]
	} else {
		peer = node.Peer
	}

	if len(node.Provider) > branchFactor {
		provider = node.Provider[:branchFactor]
	} else {
		provider = node.Provider
	}

	return graph.Node{
		ASNumber: node.ASNumber,
		Customer: customer,
		Peer:     peer,
		Provider: provider,
		Prefix:   node.Prefix,
		Location: node.Location,
		Contacts: node.Contacts,
		Rank:     node.Rank,
	}
}

// Recursive function to traverse the AS map with depth and branching factor limitations
func traverseASMap(asNumber int, depthLimit int, branchFactor int, visited map[int]bool, graph graph.Graph, topology map[int]graph.Node) {
	// Stop if we've reached the depth limit
	if depthLimit == 0 {
		return
	}

	// Check if we've already visited this AS
	if visited[asNumber] {
		return
	}
	visited[asNumber] = true

	// Get the current AS node
	node, exists := graph.Nodes[asNumber]
	if !exists {
		fmt.Printf("AS %d not found\n", asNumber)
		return
	}

	// Print the limited node (with up to `branchFactor` relations)
	limited := limitedNode(node, branchFactor)

	// Add the limited node to the topology
	topology[asNumber] = limited

	// fmt.Printf("%+v\n", limited)

	// Recur for the limited customers, peers, and providers
	for _, customer := range limited.Customer {
		traverseASMap(customer, depthLimit-1, branchFactor, visited, graph, topology)
	}
	for _, peer := range limited.Peer {
		traverseASMap(peer, depthLimit-1, branchFactor, visited, graph, topology)
	}
	for _, provider := range limited.Provider {
		traverseASMap(provider, depthLimit-1, branchFactor, visited, graph, topology)
	}
}
