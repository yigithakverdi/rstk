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

type Graph map[int]map[int]int

func MapAsToAdjacencyRelation(asRels []parser.AsRel) {
	relationships := make(map[int][]int)
	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relationships[as1] = append(relationships[as1], as2)
	}
	fmt.Println(relationships)
}

// Populates a graph by iterating over the list of AS relationships and adding
// the relationships to the graph. If the relationship is a peer relationship,
// it adds the reverse direction as well. In the end it returns something like
// map[int]map[int]int{1: {2: 0}, 2: {1: 0}} with 1 being a peer of 2 and 2 being
// a peer of 1
func PopulateGraph(asRels []parser.AsRel) Graph {
	graph := make(Graph)
	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relation := rel.Relation

		// Ensure the graph has an entry for AS1
		if graph[as1] == nil {
			graph[as1] = make(map[int]int)
		}

		// No need to cast relation since it is already an int
		graph[as1][as2] = relation

		// If it's a peer relationship, add the reverse direction
		// Assuming 0 represents a peer relationship from the description of
		// CAIDA datatset
		if relation == 0 {
			if _, ok := graph[as2]; !ok {
				graph[as2] = make(map[int]int)
			}
			graph[as2][as1] = relation
		}
	}
	return graph
}

// Check is AS exists in the graph, if exists then llops over it's neighbors that is
// stored in the value of that AS, then by checking each ASes relation in the list of
// obtain values of the AS, it appends to three different slices based on the relation
// note that by default the relation is 1, so if the relation is 1 then it is a provider
func GetNeighbors(graph Graph, asNumber int) (customers, peers, providers []int) {
	if neighbors, exists := graph[asNumber]; exists {
		for neighbor, relation := range neighbors {
			switch Relation(relation) {
			case Customer:
				customers = append(customers, neighbor)
			case Peer:
				peers = append(peers, neighbor)
			default:
				providers = append(providers, neighbor)
			}
		}
	}
	return customers, peers, providers
}
