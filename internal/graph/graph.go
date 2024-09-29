package graph

import (
	"fmt"
	"rstk/internal/parser"
)

type Relation int

const (
	Customer Relation = -1
	Peer     Relation = 0
)

// Two different Graph types, one for the direct relationships and one for the
// reverse relationships of holding customer and provider both ways to make it
// efficient to read
type Graph struct {
	Nodes map[int]Node
}

// Node struct to represent an AS (Autonomous System) entity
type Node struct {
	ASNumber       int
	Customer       []int
	Peer           []int
	Provider       []int
	Prefix         []string
	Location       string
	Interfaces     map[int]string
	Contacts       []string
	Subnets        map[string]int
	Rank           int
	Type           string
	IPPerInterface map[int]string
}

func (g *Graph) UpdateInterface(asInt int, interfaceID int, collisionDomain string) {
	node, exists := g.Nodes[asInt]
	if !exists {
		fmt.Printf("Node %d does not exist\n", asInt)
		return
	}
	node.Interfaces[interfaceID] = collisionDomain
	g.Nodes[asInt] = node
}

// NewGraph creates a new graph with empty maps for direct and reverse relationships
func NewGraph() Graph {
	return Graph{
		Nodes: make(map[int]Node),
	}
}

// PopulateGraph takes a slice of AsRel objects and constructs a Graph.
// It initializes the graph with direct and reverse relationships based on the type of relationship
// (Customer or Peer) between AS (Autonomous Systems) entities.
// - Customer relationships are added to both direct and reverse graphs.
// - Peer relationships are added bidirectionally to both direct and reverse graphs.
// The function returns the populated Graph.
func PopulateGraph(asRels []parser.AsRel) Graph {
	graph := NewGraph()

	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relation := rel.Relation

		// Ensure that AS1 and AS2 exist in the graph nodes
		node1, exists1 := graph.Nodes[as1]
		if !exists1 {
			node1 = Node{ASNumber: as1}
		}

		node2, exists2 := graph.Nodes[as2]
		if !exists2 {
			node2 = Node{ASNumber: as2}
		}

		// It is not allowed to modify fields of struct that is retrieved from a map directly
		// because in Go lookups return copies of the value, and we cannot directly modify
		// those copies, because of that, extract node from the map, modify it's field and then
		// reassign it back to the map.
		switch relation {
		case int(Customer):
			// Add AS2 as a customer of AS1 (provider)
			node1.Customer = append(node1.Customer, as2)

			// Add AS1 as a provider of AS2
			node2.Provider = append(node2.Provider, as1)

		case int(Peer):
			// Add bidirectional peer relationship
			node1.Peer = append(node1.Peer, as2)
			node2.Peer = append(node2.Peer, as1)
		}

		// Update the nodes back into the graph
		graph.Nodes[as1] = node1
		graph.Nodes[as2] = node2
	}

	return graph
}

// MapAsToAdjacencyRelation maps AS relationships to adjacency lists of customers,
// peers, and providers.
func MapAsToAdjacencyRelation(asRels []parser.AsRel) {
	graph := PopulateGraph(asRels) // Assuming we call PopulateGraph first to construct the graph.

	// Create a simple adjacency representation from the graph
	relationships := make(map[int]map[string][]int)

	for asNumber, node := range graph.Nodes {
		relationships[asNumber] = map[string][]int{
			"Customers": node.Customer,
			"Peers":     node.Peer,
			"Providers": node.Provider,
		}
	}

	// Print or use the relationships map
	fmt.Println(relationships)
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number.
// It looks up the neighbors and classifies them based on the Node's relationship data.
func (g Graph) GetNeighbors(asNumber int) (customers, peers, providers []int) {
	// Check if the AS exists in the graph
	if node, exists := g.Nodes[asNumber]; exists {
		// Return the relationships directly from the Node struct
		return node.Customer, node.Peer, node.Provider
	}
	// Return empty slices if the AS number is not found in the graph
	return []int{}, []int{}, []int{}
}
