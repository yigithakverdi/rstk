package graph 

import (
	"fmt"
  "rstk/internal/parser"
  "rstk/internal/models/router"
  "rstk/internal/models/policy"
  "rstk/internal/interfaces"
  "github.com/dominikbraun/graph"
  "github.com/edwingeng/deque"
  "strconv"
)

type Relation int

const (
    Customer Relation = -1
    Peer     Relation = 0
    Provider Relation = 1
)

type Topology struct {
  G graph.Graph[string, *router.Router]
  ASES map[int]*router.Router
  
  // Computing predecessor map and adjacency map takes
  // too long so caching them here
  PredMap map[string]map[string]graph.Edge[string]
  AdjMap map[string]map[string]graph.Edge[string]
}

// Ensure Topology implements interfaces.topology
var _ interfaces.Topology = &Topology{}

func (t* Topology) FindRoutesTo(target *router.Router) {
  // Initialize a deque to hold routes processed
  routes := deque.NewDeque()

  // **Step 1: Add self-route to the target router's routing table**
  // Create a route where the path contains only the target router
  selfRoute := &router.Route{
    FinalAS:        target,
    Path:           []interfaces.Router{target},
    OriginInvalid:  false,
    PathEndInvalid: false,
    Authenticated:  true,
  }

  // Add the self-route to the target router's routing table
  target.ForceRoute(selfRoute)    

  // For each neighbor of the target router, originate a new route and add it to the deque
  fmt.Printf("Target router: %#v\n", target)
  fmt.Println()
  fmt.Printf("Routes that will be propagated: \n")


  for _, neighbor := range target.Neighbors {
    route := target.OriginateRoute(neighbor.Router)
    routes.PushBack(route)
    fmt.Printf("%s\n", route.ToString())
  }
  
  fmt.Println() 
  fmt.Printf("Propagating routes through the network...\n")

  // Propagate route information through the network
  for routes.Len() > 0 {
    routeInterface := routes.PopFront()
    route, ok := routeInterface.(interfaces.Route)
    if !ok {
      fmt.Println("Error converting route to interfaces.Route")
      continue
    }
    
    // Get the final AS of the route
    // finalAS := route.GetFinal()
    currentAS := route.GetPath()[len(route.GetPath())-1]

    // fmt.Printf("Route from AS %d to AS %d\n", route.GetFirstHop().GetASNumber(), currentAS.GetASNumber())
    fmt.Printf("Route path: ")
    for _, router := range route.GetPath() {
      fmt.Printf("%d ", router.GetASNumber())
    }

    // Learn the route at the current AS and get the neighbors to forward to
    neighborstToAdvertise := currentAS.LearnRoute(route)
    fmt.Println("Route is learned, advertising next neighbors")

    // For each neighbor, forward the route and add it to the deque
    for _, neighbor := range neighborstToAdvertise {
      newRoute := currentAS.ForwardRoute(route, neighbor.Router)
      routes.PushBack(newRoute)
    }
    fmt.Println()
  }
}

func (t* Topology) GetRelationships(sourceRouter interfaces.Router, targetRouter interfaces.Router) (interfaces.Relation, error) {
  // Get the AS numbers of the source and target routers
  sourceASNumber := sourceRouter.GetASNumber()
  targetASNumber := targetRouter.GetASNumber()

  // Get the source and target nodes in the graph (temporarily using routerHash implementation)
  sourceNode := fmt.Sprintf("r%d", sourceASNumber)
  targetNode := fmt.Sprintf("r%d", targetASNumber)

  // Get the edge between the source and target nodes
  edge, err := t.G.Edge(sourceNode, targetNode)
  if err != nil {
    return 0, err
  }

  // Get the weight of the edge
  weight := edge.Properties.Weight
  return interfaces.Relation(weight), nil
}

// PopulateGraphGraph creates a graph of AS relationships returns the graph 
// and a map of ASes for easy access
func (t* Topology) PopulateTopology(asRels []parser.AsRel) (graph.Graph[string, *router.Router], map[int]*router.Router) {
  
  // Store ASes in lookup table for easy access
  ases := make(map[int]*router.Router)

  // Nodes are represented as hashed route values
  routerHash := func(r *router.Router) string {
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
    // as1Router, err := t.GetRouterByASNumber(as1)
    // as2Router, err := t.GetRouterByASNumber(as2)
    
    // Create routers using CreateRouter methods
    // Create routers if they don't already exist
    if _, exists := ases[as1]; !exists {
      ases[as1] = router.CreateRouter(as1, &policy.Policy{T: t})
    }
    if _, exists := ases[as2]; !exists {
      ases[as2] = router.CreateRouter(as2, &policy.Policy{T: t})
    }
 
    // Add the AS numbers as vertices
		_ = g.AddVertex(ases[as1])
		_ = g.AddVertex(ases[as2])

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
      _ = g.AddEdge(routerHash(ases[as1]), routerHash(ases[as2]), graph.EdgeWeight(int(Customer))) // Provider -> Customer
      _ = g.AddEdge(routerHash(ases[as2]), routerHash(ases[as1]), graph.EdgeWeight(int(Provider))) // Customer -> Provider (inferred)

    case int(Peer): // AS1 and AS2 are peers
      _ = g.AddEdge(routerHash(ases[as1]), routerHash(ases[as2]), graph.EdgeWeight(int(Peer)))
      _ = g.AddEdge(routerHash(ases[as2]), routerHash(ases[as1]), graph.EdgeWeight(int(Peer))) // Bidirectional for peers
    }
  }    	
	return g, ases
}

func SetTopology(asys map[int]*router.Router, g graph.Graph[string, *router.Router]) Topology {
  return Topology {
    G: g,
    ASES: asys,
  }
}

// func GetRelationship(g graph.Graph[string, interfaces.Router], sourceRouter interfaces.Router, targetRouter interfaces.Router) (Relation, error) {
//   // Get the AS numbers of the source and target routers
//   sourceASNumber := sourceRouter.GetASNumber()
//   targetASNumber := targetRouter.GetASNumber()
//
//   // Get the source and target nodes in the graph (temporarily using routerHash implementation)
//   sourceNode := fmt.Sprintf("r%d", sourceASNumber)
//   targetNode := fmt.Sprintf("r%d", targetASNumber)
//
//   // Get the edge between the source and target nodes
//   edge, err := g.Edge(sourceNode, targetNode)
//   if err != nil {
//     return 0, err
//   }
//
//   // Get the weight of the edge
//   weight := edge.Properties.Weight
//   return Relation(weight), nil
// }

func (t* Topology) InitializeAdjacencyMap() {
  adjMap, err := t.G.AdjacencyMap()
  if err != nil {
    fmt.Println("Error initializing adjacency map")
  }
  t.AdjMap = adjMap
}

func (t* Topology) InitializePredecessorMap() {
  predMap, err := t.G.PredecessorMap()
  if err != nil {
    fmt.Println("Error initializing predecessor map")
  }
  t.PredMap = predMap
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

func (t* Topology) LookUpRouter(asNumber int) (*router.Router, error) {
  if router, exists := t.ASES[asNumber]; exists {
    return router, nil
  }
  return &router.Router{}, fmt.Errorf("Router with AS number %d not found", asNumber)
}

// Function to map AS number to Router
func (t* Topology) GetRouterByASNumber(asNumber int) (*router.Router, error) {
	// Here, you would implement a way to get the Router by ASNumber.
	// This could involve looking up the router in a database, a map, or any other structure.
	// For this example, let's assume there's a predefined function or map that gives us the router.
  
  // TODO fundemantal problem on implementations, I need to "reverese-hash" the identifier
  // to get the router from the Graph, basically remove the "r" prefix from the string and
  // convert the rest to integer from string

  router, err := t.LookUpRouter(asNumber) // Example function for retrieving the router by AS number
	if err != nil {
		return router, err 
	}
	return router, nil
}

func ReverseRouterHash(hash string) (int, error) {
  return strconv.Atoi(hash[1:])
}

// Function to get customers (outgoing edges)
func (t* Topology) GetCustomers(g graph.Graph[string, *router.Router], as int) ([]*router.Router, error) {
  // Get the hash of the AS
  asHash := fmt.Sprintf("r%d", as)
    
	if neighbors, exists := t.AdjMap[asHash]; exists {
		customers := []*router.Router{}
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Customer) {
				// Get the router corresponding to the target AS number
        targetInt, _ := ReverseRouterHash(target)
        router, err := t.GetRouterByASNumber(targetInt)
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
func (t* Topology) GetProviders(g graph.Graph[string, *router.Router], as int) ([]*router.Router, error) {
	// predMap, err := g.PredecessorMap()
	// if err != nil {
	// 	return nil, err
	// } 
  
  // Get the hash of the as
  asHash := fmt.Sprintf("r%d", as)

	if predecessors, exists := t.PredMap[asHash]; exists {
		providers := []*router.Router{}
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Customer) { // Providers have incoming customer edges
				// Get the router corresponding to the predecessor AS number
        
        predecessorInt, _ := ReverseRouterHash(predecessor)

        router, err := t.GetRouterByASNumber(predecessorInt)
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
func (t* Topology) GetPeers(g graph.Graph[string, *router.Router], as int) ([]*router.Router, error) {
	// adjMap, err := g.AdjacencyMap()
	// if err != nil {
	// 	return nil, err
	// }
	peers := []*router.Router{}
  
  // Get the hash of the as
  asHash := fmt.Sprintf("r%d", as)

	// Check for outgoing peer edges
	if neighbors, exists := t.AdjMap[asHash]; exists {
		for target, edge := range neighbors {
			if edge.Properties.Weight == int(Peer) {
				// Get the router corresponding to the target AS number
          
        targetInt, _ := ReverseRouterHash(target)

        router, err := t.GetRouterByASNumber(targetInt)
				if err == nil {
					peers = append(peers, router)
				}
			}
		}
	}

  // Get the hash of the as
  asHash = fmt.Sprintf("r%d", as)

	if predecessors, exists := t.PredMap[asHash]; exists {
		for predecessor, edge := range predecessors {
			if edge.Properties.Weight == int(Peer) {
				// Get the router corresponding to the predecessor AS number
        
        predecessorInt, _ := ReverseRouterHash(predecessor)

        router, err := t.GetRouterByASNumber(predecessorInt)
				if err == nil {
					peers = append(peers, router)
				}
			}
		}
	}
	return peers, nil
}

func (t *Topology) PrintTopology() {
  adjMap, err := t.G.AdjacencyMap()
  if err != nil {
      fmt.Println("Error retrieving adjacency map:", err)
      return
  }

  for node, neighbors := range adjMap {
    asInt, _ :=  ReverseRouterHash(node)
    router, _ := t.GetRouterByASNumber(asInt)
    fmt.Printf("Node: %s\n", router.ToString())
    for neighbor, edge := range neighbors {
      asInt, _ :=  ReverseRouterHash(neighbor) 
      router, _ := t.GetRouterByASNumber(asInt)
      fmt.Printf("  Connected to: %s | Edge properties: %+v\n", router.ToString(), edge)
    }
  }
}

// GetNeighbors returns the customers, peers, and providers of a given AS (Autonomous System) number as a list of Router objects.
// It looks up the neighbors and classifies them based on the Node's relationship data.
func (t* Topology) GetNeighbors(router *router.Router, g graph.Graph[string, *router.Router]) ([]*router.Router, error) {
	asNumber := router.ASNumber

	// Get the customers, peers, and providers of the AS number
	customers, err := t.GetCustomers(g, asNumber)
  if err != nil {
		return nil, err
	}

	peers, err := t.GetPeers(g, asNumber)
	if err != nil {
		return nil, err
	}
  
	providers, err := t.GetProviders(g, asNumber)
	if err != nil {
		return nil, err
	}

	// Concatenate customers, peers, and providers into one slice of routers
	neighbors := append(customers, peers...)
	neighbors = append(neighbors, providers...)

	return neighbors, nil
}

// FindRoute performs BFS to find a route from sourceRouter to targetRouter
// func (t *Topology) FindRoute(sourceRouter, targetRouter *router.Router) ([]*router.Router, error) {
//     if sourceRouter.GetASNumber() == targetRouter.GetASNumber() {
//         return []*router.Router{sourceRouter}, nil
//     }
//
//     visited := make(map[int]bool)
//     parentMap := make(map[int]*router.Router) // Map to reconstruct the path
//
//     // Initialize deque for BFS
//     queue := deque.NewDeque()
//     queue.PushBack(sourceRouter)
//     visited[sourceRouter.GetASNumber()] = true
//
//     // BFS loop
//     for queue.Len() > 0 {
//         current := queue.PopFront().(*router.Router)
//
//         // Get neighbors of the current router
//         neighbors, err := t.GetNeighbors(current, t.G)
//         if err != nil {
//             return nil, fmt.Errorf("error fetching neighbors for AS %d: %v", current.GetASNumber(), err)
//         }
//
//         // Process each neighbor
//         for _, neighbor := range neighbors {
//             if visited[neighbor.GetASNumber()] {
//                 continue
//             }
//
//             // Mark neighbor as visited and set parent
//             visited[neighbor.GetASNumber()] = true
//             parentMap[neighbor.GetASNumber()] = current
//
//             // Check if target is reached
//             if neighbor.GetASNumber() == targetRouter.GetASNumber() {
//                 return t.constructPath(parentMap, targetRouter), nil
//             }
//             queue.PushBack(neighbor)
//         }
//     }
//
//     return nil, fmt.Errorf("no route found from AS %d to AS %d", sourceRouter.GetASNumber(), targetRouter.GetASNumber())
// }

// Helper function to construct the path from source to target using parentMap
// func (t *Topology) constructPath(parentMap map[int]*router.Router, targetRouter *router.Router) []*router.Router {
//     path := []*router.Router{}
//     for current := targetRouter; current != nil; current = parentMap[current.GetASNumber()] {
//         path = append([]*router.Router{current}, path...)
//     }
//     return path
// }
