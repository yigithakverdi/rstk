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
	Direct  map[int]map[int]int
	Reverse map[int]map[int]int
}

// Node struct to represent an AS (Autonomous System) entity
type Node struct {
	ASNumber int
	Customer []int
	Peer     []int
	Provider []int
	Prefix   []string
	Location string
	Contacts []string
	Rank     int
}

// NewGraph creates a new graph with empty maps for direct and reverse relationships
func NewGraph() Graph {
	return Graph{
		Direct:  make(map[int]map[int]int),
		Reverse: make(map[int]map[int]int),
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

		switch relation {
		case int(Customer):

			// Add edge from customer (AS2) to provider (AS1)
			AddEdge(graph.Direct, as2, as1, relation)

			// Optionally, add reverse edge from provider to customer
			AddEdge(graph.Reverse, as1, as2, relation)

		case int(Peer):

			// For peer relationships, add bidirectional edges
			AddEdge(graph.Direct, as1, as2, relation)
			AddEdge(graph.Direct, as2, as1, relation)
			AddEdge(graph.Reverse, as1, as2, relation)
			AddEdge(graph.Reverse, as2, as1, relation)
		}
	}
	return graph
}

// Helper function to add an edge to a graph
func AddEdge(graph map[int]map[int]int, from int, to int, relation int) {
	if graph[from] == nil {
		graph[from] = make(map[int]int)
	}
	graph[from][to] = relation
}

// Helper function to map AS relationships to an adjacency relation
func MapAsToAdjacencyRelation(asRels []parser.AsRel) {
	relationships := make(map[int][]int)
	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relationships[as1] = append(relationships[as1], as2)
	}
	fmt.Println(relationships)
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number.
// It first looks up the direct neighbors and classifies them based on their relationship (customer or peer).
// Then, it efficiently finds the providers using the reverse graph.
func (g Graph) GetNeighbors(asNumber int) (customers, peers, providers []int) {
	if neighbors, exists := g.Direct[asNumber]; exists {
		for neighbor, relation := range neighbors {
			switch relation {
			case int(Customer):
				providers = append(providers, neighbor)
			case int(Peer):
				peers = append(peers, neighbor)
			}
		}
	}

	// Efficiently find providers using the reverse graph
	if reverseNeighbors, exists := g.Reverse[asNumber]; exists {
		for provider := range reverseNeighbors {
			customers = append(customers, provider)
		}
	}

	return customers, peers, providers
}
