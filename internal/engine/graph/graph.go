package graph 

import (
	"fmt"
	"rstk/internal/parser"
  "rstk/internal/models/router"
  "rstk/internal/interfaces"
  "github.com/dominikbraun/graph"
)

type Relation int

const (
    Customer Relation = -1
    Peer     Relation = 0
    Provider Relation = 1
)

type Topology struct {
  g graph.Graph[string, router.Router]
  ases map[int]router.Router
}

// PopulateGraphGraph creates a graph of AS relationships returns the graph 
// and a map of ASes for easy access
func (t* Topology) PopulateGraph(asRels []parser.AsRel) (graph.Graph[string, router.Router], map[int]router.Router) {
  
  // Store ASes in lookup table for easy access
  ases := make(map[int]router.Router)
  
  // Nodes are represented as hashed route values
  routerHash := func(r router.Router) string {
    return fmt.Sprintf("r%d", r.ASNumber)
  }

  g := graph.New(routerHash, graph.Directed(), graph.Acyclic())

	// Populate the graph with vertices and edges
	for _, rel := range asRels {
		as1 := rel.AS1
		as2 := rel.AS2
		relation := rel.Relation
   
    // TODO one example that I need to seperate topology and graph operations
    // I am accessing topology related methods under pure graph logic 
    // to solve this maybe an intermediary node structure can be created 
    // to represent the graph, later on once the graph is created, network, topology
    // logic can be applied on top of it
    as1Router, err := t.GetRouterByASNumber(as1)
    as2Router, err := t.GetRouterByASNumber(as2)

    // Add the AS numbers to the lookup table
    if _, exists := ases[as1]; !exists {

      if err == nil {
        ases[as1] = as1Router
        ases[as2] = as2Router
      }
    }

		// Add the AS numbers as vertices
		_ = g.AddVertex(as1Router)
		_ = g.AddVertex(as2Router)

		// Add edges based on the relationship type
		// switch relation {
		// case int(Customer): // AS1 is the provider of AS2
		// 	_ = g.AddEdge(routerHash(as1Router), routerHash(as2Router), graph.EdgeWeight(int(Customer))) // Provider -> Customer
		// case int(Peer): // AS1 and AS2 are peers
		// 	_ = g.AddEdge(routerHash(as1Router), routerHash(as2Router), graph.EdgeWeight(int(Peer)))
		// 	_ = g.AddEdge(routerHash(as2Router), routerHash(as1Router), graph.EdgeWeight(int(Peer))) // Bidirectional for peers
		// }

    // Add edges based on the relationship type, now including inferred provider relationships
    switch relation {
    case int(Customer): // AS1 is the provider of AS2
      _ = g.AddEdge(routerHash(as1Router), routerHash(as2Router), graph.EdgeWeight(int(Customer))) // Provider -> Customer
      _ = g.AddEdge(routerHash(as2Router), routerHash(as1Router), graph.EdgeWeight(int(Provider))) // Customer -> Provider (inferred)

    case int(Peer): // AS1 and AS2 are peers
      _ = g.AddEdge(routerHash(as1Router), routerHash(as2Router), graph.EdgeWeight(int(Peer)))
      _ = g.AddEdge(routerHash(as2Router), routerHash(as1Router), graph.EdgeWeight(int(Peer))) // Bidirectional for peers
    }
  }    	
	return g, ases
}

func SetTopology(asys map[int]router.Router, g graph.Graph[string, router.Router]) Topology {
  return Topology {
    g: g,
    ases: asys,
  }
}

func GetRelationship(g graph.Graph[string, interfaces.Router], sourceRouter interfaces.Router, targetRouter interfaces.Router) (Relation, error) {
  // Get the AS numbers of the source and target routers
  sourceASNumber := sourceRouter.GetASNumber()
  targetASNumber := targetRouter.GetASNumber()

  // Get the source and target nodes in the graph (temporarily using routerHash implementation)
  sourceNode := fmt.Sprintf("r%d", sourceASNumber)
  targetNode := fmt.Sprintf("r%d", targetASNumber)

  // Get the edge between the source and target nodes
  edge, err := g.Edge(sourceNode, targetNode)
  if err != nil {
    return 0, err
  }

  // Get the weight of the edge
  weight := edge.Properties.Weight
  return Relation(weight), nil
}

// BuildReachabilityGraph creates a reachability graph from a map of ASes
func BuildReachabilityGraph(ases map[int]router.Router) graph.Graph[string, router.Router] {
  routerHash := func(r router.Router) string {
    return fmt.Sprintf("r%d", r.ASNumber)
  }

  g := graph.New(routerHash, graph.Directed())

  for asNumber, _ := range ases {
    // lNode := fmt.Sprintf("l%d", asNumber)
    // rNode := fmt.Sprintf("r%d", asNumber)

    lNode := router.Router {
      ASNumber: asNumber,
    }

    rNode := router.Router {
      ASNumber: asNumber,
    }

    _ = g.AddVertex(lNode, graph.VertexAttribute("reachable_from", fmt.Sprintf("%d", 1<<asNumber)))
    _ = g.AddVertex(rNode, graph.VertexAttribute("reachable_from", "0"))
    
    _ = g.AddEdge(routerHash(lNode), routerHash(rNode))    
  }

  for asNumber, r := range ases {
    lNode := fmt.Sprintf("l%d", asNumber)
    rNode := fmt.Sprintf("r%d", asNumber)

    for _, neighbor := range r.Neighbors {
      lNeighbor := fmt.Sprintf("r%d", neighbor.Router.GetASNumber())
      rNeighbor := fmt.Sprintf("l%d", neighbor.Router.GetASNumber())

      switch int(neighbor.Relation) {
      case int(Customer):
        _ = g.AddEdge(rNode, rNeighbor) // r(asys) -> r(neighbor)
      case int(Peer):
        _ = g.AddEdge(lNode, lNeighbor) // l(asys) -> r(neighbor)
      case int(Provider):
        _ = g.AddEdge(lNode, lNeighbor) // l(asys) -> l(neighbor)
      }
    }
  }
  return g
}


func (t* Topology) LookUpRouter(asNumber int) (router.Router, error) {
  if router, exists := t.ases[asNumber]; exists {
    return router, nil
  }
  return router.Router{}, fmt.Errorf("Router with AS number %d not found", asNumber)
}

// Function to map AS number to Router
func (t* Topology) GetRouterByASNumber(asNumber int) (router.Router, error) {
	// Here, you would implement a way to get the Router by ASNumber.
	// This could involve looking up the router in a database, a map, or any other structure.
	// For this example, let's assume there's a predefined function or map that gives us the router.
	router, err := t.LookUpRouter(asNumber) // Example function for retrieving the router by AS number
	if err != nil {
		return router, err 
	}
	return router, nil
}

// Function to get customers (outgoing edges)
func (t* Topology) GetCustomers(g graph.Graph[int, int], as int) ([]router.Router, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	if neighbors, exists := adjMap[as]; exists {
		customers := []router.Router{}
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Customer) {
				// Get the router corresponding to the target AS number
				router, err := t.GetRouterByASNumber(target)
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
func (t* Topology) GetProviders(g graph.Graph[int, int], as int) ([]router.Router, error) {
	predMap, err := g.PredecessorMap()
	if err != nil {
		return nil, err
	}
	if predecessors, exists := predMap[as]; exists {
		providers := []router.Router{}
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Customer) { // Providers have incoming customer edges
				// Get the router corresponding to the predecessor AS number
				router, err := t.GetRouterByASNumber(predecessor)
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
func (t* Topology) GetPeers(g graph.Graph[int, int], as int) ([]router.Router, error) {
	adjMap, err := g.AdjacencyMap()
	if err != nil {
		return nil, err
	}
	peers := []router.Router{}

	// Check for outgoing peer edges
	if neighbors, exists := adjMap[as]; exists {
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Peer) {
				// Get the router corresponding to the target AS number
				router, err := t.GetRouterByASNumber(target)
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
				router, err := t.GetRouterByASNumber(predecessor)
				if err == nil {
					peers = append(peers, router)
				}
			}
		}
	}
	return peers, nil
}

func PrintGraph(g graph.Graph[string, router.Router]) {
    adjMap, err := g.AdjacencyMap()
    if err != nil {
        fmt.Println("Error retrieving adjacency map:", err)
        return
    }

    for node, neighbors := range adjMap {
        fmt.Printf("Node: %v\n", node)
        for neighbor, edge := range neighbors {
            fmt.Printf("  Connected to: %v | Edge properties: %+v\n", neighbor, edge)
        }
    }
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number as a list of Router objects.
// It looks up the neighbors and classifies them based on the Node's relationship data.
func (t* Topology) GetNeighbors(router router.Router, g graph.Graph[int, int]) ([]router.Router, error) {
	asNumber := router.ASNumber

	// Get the customers, peers, and providers of the AS number
	customers, err := t.GetCustomers(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Customers of AS%d: %v\n", asNumber, customers)

	peers, err := t.GetPeers(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Peers of AS%d: %v\n", asNumber, peers)

	providers, err := t.GetProviders(g, asNumber)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Providers of AS%d: %v\n", asNumber, providers)

	// Concatenate customers, peers, and providers into one slice of routers
	neighbors := append(customers, peers...)
	neighbors = append(neighbors, providers...)

	return neighbors, nil
}
