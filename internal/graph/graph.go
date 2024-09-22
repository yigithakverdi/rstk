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

		// Ensuring entries exist in both graphs
		if graph.Direct[as1] == nil {
			graph.Direct[as1] = make(map[int]int)
		}
		graph.Direct[as1][as2] = relation

		// For reverse relationships (-1)
		if relation == int(Customer) {
			if graph.Reverse[as2] == nil {
				graph.Reverse[as2] = make(map[int]int)
			}
			graph.Reverse[as2][as1] = relation
		}

		// Handle peer relationships bidirectionally (0)
		if relation == int(Peer) {
			if graph.Direct[as2] == nil {
				graph.Direct[as2] = make(map[int]int)
			}
			graph.Direct[as2][as1] = relation

			if graph.Reverse[as1] == nil {
				graph.Reverse[as1] = make(map[int]int)
			}
			graph.Reverse[as1][as2] = relation
		}
	}
	return graph
}

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
			case int(Customer): // -1
				customers = append(customers, neighbor)
			case int(Peer): // 0
				peers = append(peers, neighbor)
			}
		}
	}

	// Efficiently find providers using the reverse graph
	if reverseNeighbors, exists := g.Reverse[asNumber]; exists {
		for provider := range reverseNeighbors {
			providers = append(providers, provider)
		}
	}

	return customers, peers, providers
}
