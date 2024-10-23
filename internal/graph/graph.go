package graph

import (
	"fmt"
	"rstk/internal/parser"
  "rstk/pkg/models"
	"github.com/dominikbraun/graph"
)

type Relation int

const (
	Customer Relation = -1
	Peer     Relation = 0
)

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

func LookUpRouter(asNumber int) (models.Router, error) {
  // Here, you would implement a way to get the Router by ASNumber.
  // This could involve looking up the router in a database, a map, or any other structure.
  // For this example, let's assume there's a predefined function or map that gives us the router.
  return models.Router{
    ASNumber: asNumber,
  }, nil
}

// Function to map AS number to Router
func GetRouterByASNumber(asNumber int) (models.Router, error) {
	// Here, you would implement a way to get the Router by ASNumber.
	// This could involve looking up the router in a database, a map, or any other structure.
	// For this example, let's assume there's a predefined function or map that gives us the router.
	router, err := LookUpRouter(asNumber) // Example function for retrieving the router by AS number
	if err != nil {
		return models.Router{}, err
	}
	return router, nil
}

// Function to get customers (outgoing edges)
func GetCustomers(g graph.Graph[int, int], as int) ([]models.Router, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	if neighbors, exists := adjMap[as]; exists {
		customers := []models.Router{}
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Customer) {
				// Get the router corresponding to the target AS number
				router, err := GetRouterByASNumber(target)
				if err == nil {
					customers = append(customers, router)
				}
			}
		}
		return customers, nil
	}
	return nil, nil
}

// Function to get providers (incoming edges)
func GetProviders(g graph.Graph[int, int], as int) ([]models.Router, error) {
	predMap, err := g.PredecessorMap()
	if err != nil {
		return nil, err
	}
	if predecessors, exists := predMap[as]; exists {
		providers := []models.Router{}
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Customer) { // Providers have incoming customer edges
				// Get the router corresponding to the predecessor AS number
				router, err := GetRouterByASNumber(predecessor)
				if err == nil {
					providers = append(providers, router)
				}
			}
		}
		return providers, nil
	}
	return nil, nil
}

// Function to get peers (both directions)
func GetPeers(g graph.Graph[int, int], as int) ([]models.Router, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	peers := []models.Router{}

	// Check for outgoing peer edges
	if neighbors, exists := adjMap[as]; exists {
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Peer) {
				// Get the router corresponding to the target AS number
				router, err := GetRouterByASNumber(target)
				if err == nil {
					peers = append(peers, router)
				}
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
				// Get the router corresponding to the predecessor AS number
				router, err := GetRouterByASNumber(predecessor)
				if err == nil {
					peers = append(peers, router)
				}
			}
		}
	}
	return peers, nil
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number as a list of Router objects.
// It looks up the neighbors and classifies them based on the Node's relationship data.
func GetNeighbors(router models.Router, g graph.Graph[int, int]) ([]models.Router, error) {
	asNumber := router.ASNumber

	// Get the customers, peers, and providers of the AS number
	customers, err := GetCustomers(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Customers of AS%d: %v\n", asNumber, customers)

	peers, err := GetPeers(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Peers of AS%d: %v\n", asNumber, peers)

	providers, err := GetProviders(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Providers of AS%d: %v\n", asNumber, providers)

	// Concatenate customers, peers, and providers into one slice of routers
	neighbors := append(customers, peers...)
	neighbors = append(neighbors, providers...)

	return neighbors, nil
}

// Detects if there is a cycle exists and provides the path of a cycle
func ContainsCycle() {

}
