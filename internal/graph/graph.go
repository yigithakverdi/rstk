package graph

import (
	// "os"
	"fmt"
	"rstk/internal/parser"

	"github.com/dominikbraun/graph"
	// ggraph "github.com/dominikbraun/graph"
	// "github.com/dominikbraun/graph/draw"
)

type Relation int

const (
	Customer Relation = -1
	Peer     Relation = 0
)

// Two different Graph types, one for the direct relationships and one for the
// reverse relationships of holding customer and provider both ways to make it
// efficient to read
// type Graph struct {
// 	Nodes map[int]Node
// }

// Node struct to represent an AS (Autonomous System) entity
// type Node struct {
// 	ASNumber       int
// 	Customer       []int
// 	Peer           []int
// 	Provider       []int
// 	Prefix         []string
// 	Location       string
// 	Interfaces     map[int]string
// 	Contacts       []string
// 	Subnets        map[string]int
// 	Rank           int
// 	Type           string
// 	IPPerInterface map[int]string
//   ASPAList       []string
//   ASPAEnabled    bool
// }
//

// func (g *Graph) UpdateInterface(asInt int, interfaceID int, collisionDomain string) {
// 	node, exists := g.Nodes[asInt]
// 	if !exists {
// 		fmt.Printf("Node %d does not exist\n", asInt)
// 		return
// 	}
// 	node.Interfaces[interfaceID] = collisionDomain
// 	g.Nodes[asInt] = node
// }

// NewGraph creates a new graph with empty maps for direct and reverse relationships
// func NewGraph() Graph {
// 	return Graph{
// 		Nodes: make(map[int]Node),
// 	}
// }

// PopulateGraph takes a slice of AsRel objects and constructs a Graph.
// It initializes the graph with direct and reverse relationships based on the type of relationship
// (Customer or Peer) between AS (Autonomous Systems) entities.
// - Customer relationships are added to both direct and reverse graphs.
// - Peer relationships are added bidirectionally to both direct and reverse graphs.
// The function returns the populated Graph.
// func PopulateGraph(asRels []parser.AsRel) Graph {
// 	graph := NewGraph()
//
// 	for _, rel := range asRels {
// 		as1 := rel.AS1
// 		as2 := rel.AS2
// 		relation := rel.Relation
//
// 		// Ensure that AS1 and AS2 exist in the graph nodes
// 		node1, exists1 := graph.Nodes[as1]
// 		if !exists1 {
// 			node1 = Node{ASNumber: as1}
// 		}
//
// 		node2, exists2 := graph.Nodes[as2]
// 		if !exists2 {
// 			node2 = Node{ASNumber: as2}
// 		}
//
// 		// It is not allowed to modify fields of struct that is retrieved from a map directly
// 		// because in Go lookups return copies of the value, and we cannot directly modify
// 		// those copies, because of that, extract node from the map, modify it's field and then
// 		// reassign it back to the map.
// 		switch relation {
// 		case int(Customer):
// 			// Add AS2 as a customer of AS1 (provider)
// 			node1.Customer = append(node1.Customer, as2)
//
// 			// Add AS1 as a provider of AS2
// 			node2.Provider = append(node2.Provider, as1)
//
// 		case int(Peer):
// 			// Add bidirectional peer relationship
// 			node1.Peer = append(node1.Peer, as2)
// 			node2.Peer = append(node2.Peer, as1)
// 		}
//
// 		// Update the nodes back into the graph
// 		graph.Nodes[as1] = node1
// 		graph.Nodes[as2] = node2
// 	}
// 	return graph
// }


// PopulateGraphGraph creates a graph of AS relationships
func PopulateGraph(asRels []parser.AsRel) graph.Graph[int, int] {
	// Create a directed acyclic graph of integers
	g := graph.New(graph.IntHash, graph.Directed(), graph.Acyclic())

	// Populate the graph with vertices and edges
	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relation := rel.Relation

		// Add the AS numbers as vertices
		_ = g.AddVertex(as1)
		_ = g.AddVertex(as2)

		// Add edges based on the relationship type
		switch relation {
		case int(Customer): // AS1 is the provider of AS2
			_ = g.AddEdge(as1, as2, graph.EdgeWeight(int(Customer))) // Provider -> Customer
		case int(Peer): // AS1 and AS2 are peers
			_ = g.AddEdge(as1, as2, graph.EdgeWeight(int(Peer)))
			_ = g.AddEdge(as2, as1, graph.EdgeWeight(int(Peer))) // Bidirectional for peers
		}
	}
	return g
}

// Function to get customers (outgoing edges)
func GetCustomers(g graph.Graph[int, int], as int) ([]int, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	if neighbors, exists := adjMap[as]; exists {
		customers := []int{}
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Customer) {
				customers = append(customers, target)
			}
		}
		return customers, nil
	}
	return nil, nil
}

// Function to get providers (incoming edges)
func GetProviders(g graph.Graph[int, int], as int) ([]int, error) {
	predMap, err := g.PredecessorMap()
	if err != nil {
		return nil, err
	}
	if predecessors, exists := predMap[as]; exists {
		providers := []int{}
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Customer) { // Providers have incoming customer edges
				providers = append(providers, predecessor)
			}
		}
		return providers, nil
	}
	return nil, nil
}

// Function to get peers (both directions)
func GetPeers(g graph.Graph[int, int], as int) ([]int, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	peers := []int{}
	// Check for outgoing peer edges
	if neighbors, exists := adjMap[as]; exists {
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Peer) {
				peers = append(peers, target)
			}
		}
	}

	// Check for incoming peer edges (in case it's a directed peer edge)
	predMap, err := g.PredecessorMap()
	if err != nil {
		return nil, err
	}
	if predecessors, exists := predMap[as]; exists {
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Peer) {
				peers = append(peers, predecessor)
			}
		}
	}
	return peers, nil
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number.
// It looks up the neighbors and classifies them based on the Node's relationship data.
func GetNeighbors(asNumber int, g graph.Graph[int, int]) map[int][]int {
  	// Get the customers, peers, and providers of the AS number
	customers, _ := GetCustomers(g, asNumber)
	fmt.Printf("Customers of AS%d: %v\n", asNumber, customers)

	peers, _ := GetPeers(g, asNumber)
	fmt.Printf("Peers of AS%d: %v\n", asNumber, peers)

	providers, _ := GetProviders(g, asNumber)
	fmt.Printf("Providers of AS%d: %v\n", asNumber, providers)

	// Concatenate customers, peers, and providers into one slice
	neighbors := append(customers, peers...)
	neighbors = append(neighbors, providers...)

	// Return a map of asNumber to a slice containing all neighbors
	return map[int][]int{asNumber: neighbors}
}
