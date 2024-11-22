package graph

import (
	// Core imports
	"fmt"

	// Internal packages
	"rstk/internal/parser"
  "rstk/internal/router"

	// Base data structure for topology, and node strucutres
	// relations, edge, vertex representation are all based on
	// below package

	// TODO in the future, I aim to build a graph package, that
	// is specificially designed for networking context
	"github.com/dominikbraun/graph"

	// Another data structure used for conducting various operations on the topology
	// such as calculating shortest path, finding routes from determined router etc.

	// TODO again I might build more specfic data structure for networking context
	"github.com/edwingeng/deque"
  log "github.com/sirupsen/logrus"
)

// TODO note for temporarly, I am hardcoding the topology with network context, in normal
// case, or intended design, topology should be abstracted away with graph package, and
// on top of the graph, whatever the user creates, it is built on top of the graph.

// TODO since topologies could vary, in networking context, for example in our simulator, we have
// heavily focused on BGP routing, so node relations are included in the code as well, most of the
// data structures are related to BGP context, this will in the future, with the graph, also will
// be abstracted away with more modular structure, with either a scripting with 'lua' or 'python'
// or some templating or some configurations user be able to create their own topologies, with their
// own routing, policy, route logics, etc.

// Below are all the data strcutres used within the topology

// Main topology structure, encapsulating the graph and map of ASes in the
// topology for easy access, however this creates duality, so it must be carefully
// handled possibly with the direct memory referncing from topology
type Topology struct {
  G graph.Graph[string, *router.Router]
  ASES map[int]*router.Router
  
  // Policy factor allows dynmaic assignment of policies to routers, depending on the
  // user configurations more heterogeneous topologies could be created, with different
  // policy types, such as ASPA, BGP, OSPF, etc.
  PolicyFactory *PolicyFactory

  // Predecessor map and ancestor map is pre-computed, and stored in the topology as well
  // this significantly reduces the computation time with the downside of memory usage
  // for larger topologies
  PredMap map[string]map[string]graph.Edge[string]
  AdjMap map[string]map[string]graph.Edge[string]

  // Topology type is used for determining policy assignment strategies to routers, depending
  // on provided argument, different number of policies are assigned to routers
  TopologyType string
}


// Function for initializing the topology, feeding the graph data structure with the
// specified formats, in this case only AS relationships are fed into the graph
// it returns the topology graph G and the map of ASes in the topology, also
// updates the current instances of the topology

// TODO in the future, a more general format of data structure could be build, and
// these could be integrated with the different data sources from CAIDA, RIPE, etc.
// these data sources then creates different type of topologies, such as BGP topology,
// OSPF topology, data center topology, etc.

// TODO for temporarly AS relationships are passed as argument to Init() function, which is/
// very inconvinante at the moment, since this requires manually defining parsing on other
// unrelated parts of the code (i.e. when calling this function on CLI package commands), 
// this needs to be generalized as much as possible
func (t* Topology) Init(asRelsList []parser.AsRel) (graph.Graph[string, *router.Router], 
  map[int]*router.Router) {

  // TODO For now also hardcoding topology type to specific policy types, all the routers
  // will be set according to below topology type, types are based on the `policy_factory.go`
  // GetPolicy function, later on also this should be moved to elsewhere, and be more
  // configurable, or user should be able to define their own policies
  t.TopologyType = "ASPA"
  log.Infof("Initializing topology with %s topology type for all routers", t.TopologyType)
  
  // Store ASes in lookup table for easy access
  ases := make(map[int]*router.Router)

  // Initialize the policy factory
  t.PolicyFactory = NewPolicyFactory() 

  // Nodes are represented as hashed route values
  routerHash := func(r *router.Router) string {
    return fmt.Sprintf("r%d", r.ASNumber)
  }

  g := graph.New(routerHash, graph.Directed(), graph.Acyclic())
  
	// Populate the graph with vertices and edges
	for _, rel := range asRelsList {
    as1 := rel.AS1
		as2 := rel.AS2
		relation := router.Relation(rel.Relation)
   
    // Step 1: Create routers if they don't exist, then add them to AS map, for later
    // accessing them for adding them to graph, besides policies and AS numbers,
    // also routing tables are initialized to empty here
    //
    // TODO policy initialization also might needs to be done here
    // currently every router initialized with default PolicyFactory
    // (asNumber int, topologyType string, neighbors map[int]router.Relation) 
    if _, exists := ases[as1]; !exists {
      ases[as1] = &router.Router{
        ASNumber: as1, 
        RouteTable: make(map[int]*router.Route),
      }
    }
    if _, exists := ases[as2]; !exists {
      ases[as2] = &router.Router{
        ASNumber: as2, 
        RouteTable: make(map[int]*router.Route),
      } 
    }

    // Step 2: Add neighbors to routers, and add routers to the graph, initially adding it
    // to AS map, referning it from the AS map to add the vertice in the next step
    neighbor1 := router.Neighbor{
        Relation: router.Relation(relation),
        Router:   ases[as2], // Neighboring router
    }
    ases[as1].Neighbors = append(ases[as1].Neighbors, neighbor1)
    ases[as1].Tier = t.SetTier(ases[as1].Neighbors)
    ases[as1].Policy = t.PolicyFactory.GetPolicy(as1, t.TopologyType, ases[as1].Neighbors)

    // For bidirectional relationships
    neighbor2 := router.Neighbor{
        Relation: inverseRelation(router.Relation(relation)),
        Router:   ases[as1],
    }
    ases[as2].Neighbors = append(ases[as2].Neighbors, neighbor2)
    ases[as2].Tier = t.SetTier(ases[as2].Neighbors)
    ases[as2].Policy = t.PolicyFactory.GetPolicy(as1, t.TopologyType, ases[as2].Neighbors)

    
    // TODO Step X: Policy initialization, is also needed since when learning or finding routes to
    // a given AS, policies are checked first
    
    // Step 3: Adding vertices (nodes or routers) to the graph
    _ = g.AddVertex(ases[as1])
		_ = g.AddVertex(ases[as2])
    
    // Step 4: Calculate the router hashes for adding edges to the graph
    as1Hash := routerHash(ases[as1])
    as2Hash := routerHash(ases[as2])

    // Step 5: Adding edges to the graph, with the relation between the routers
    _ = g.AddEdge(as1Hash, as2Hash, graph.EdgeWeight(int(relation)))
    _ = g.AddEdge(as2Hash, as1Hash, graph.EdgeWeight(int(inverseRelation(relation))))
  }

  // Initialize the adjacency and predecessor maps of the populated topology
  t.AdjMap, _ = g.AdjacencyMap()
  t.PredMap, _ = g.PredecessorMap()

  // Update the topology with the graph and ASes
  t.G = g
  t.ASES = ases
  
	return g, ases
}

// Method for setting tiers of the routers
func (t *Topology) SetTier(neighbors []router.Neighbor) int {
  var hasCustomer, hasProvider bool
  for _, neighbor := range neighbors {
    if neighbor.Relation == router.Customer {
      hasCustomer = true
    }

    if neighbor.Relation == router.Provider {
      hasProvider = true
    }
  }

  if(hasCustomer && hasProvider) {
    return 2 
  } else if(hasCustomer) {
    return 1
  } else if(hasProvider) {
    return 3
  } else {
    return 0
  }
}

// Adds a router to the topology, that is alread intialized with the Init() function
// used for including different type of routers to support different type of scenarios
// such as introducing a rouge router, that conducts prefix hijacking, or route leaking
// etc.
func (t *Topology) AddRouter(r *router.Router) {
    // Check if topology is initialized
    if t.G == nil {
        fmt.Println("Topology is not initialized yet")
        return
    }

    // Check if the router with the same AS number already exists
    if _ , exists := t.ASES[r.ASNumber]; exists {
        fmt.Printf("Router with AS%d already exists, adding it as a hijacker\n", r.ASNumber)
        fmt.Printf("Duplicate routers will exist in the graph\n")
        fmt.Printf("Overwriting existing one, all routing information will be lost\n")
        
        // **Important:** Instead of overwriting, consider handling multiple routers per ASNumber
        // For simplicity, we'll proceed with overwriting but note the caveat
        t.ASES[r.ASNumber] = r
    } else {
        // Add the new router to the ASES map
        t.ASES[r.ASNumber] = r
    }

    // Validate that all specified neighbors exist in the topology
    for _, neighbor := range r.Neighbors {
        if _, exists := t.ASES[neighbor.Router.ASNumber]; !exists {
            fmt.Printf("Neighbor with AS%d does not exist in the topology\n", neighbor.Router.ASNumber)
            return
        }
    }

    // Add the new router to the graph
    _ = t.G.AddVertex(r)

    // Establish bidirectional edges in the graph
    for _, neighbor := range r.Neighbors {
        neighborHash := RouterHash(neighbor.Router)
        _ = t.G.AddEdge(RouterHash(r), neighborHash, graph.EdgeWeight(int(neighbor.Relation)))
        _ = t.G.AddEdge(neighborHash, RouterHash(r), graph.EdgeWeight(int(inverseRelation(neighbor.Relation))))
    }

    // Update Neighbor Routers' Neighbor Lists
    for _, neighbor := range r.Neighbors {
        // existingNeighbor := t.ASES[neighbor.Router.ASNumber] // Retrieve from ASES map
        existingNeighbor := t.GetRouter(RouterHash(neighbor.Router))
        inverseRel := inverseRelation(neighbor.Relation)

        // Create a new Neighbor instance for the existing neighbor to include the new router
        newNeighbor := router.Neighbor{
            Relation: inverseRel,
            Router:   r,
        }

        // Check if the new router is already a neighbor to avoid duplicates
        alreadyNeighbor := false
        for _, n := range existingNeighbor.Neighbors {
            if n.Router.ASNumber == r.ASNumber {
                alreadyNeighbor = true
                log.Infof("AS%d is already a neighbor to AS%d with relation %d", n.Router.ASNumber, 
                  r.ASNumber, n.Relation)
                break
            }
        }

        // Append the new neighbor if not already present
        if !alreadyNeighbor {
            existingNeighbor.Neighbors = append(existingNeighbor.Neighbors, newNeighbor)
            log.Infof("Added AS%d as a neighbor to AS%d with relation %d", r.ASNumber, 
                existingNeighbor.ASNumber, inverseRel)
        }
    }

    // Update the adjacency and predecessor maps
    t.AdjMap, _ = t.G.AdjacencyMap()
    t.PredMap, _ = t.G.PredecessorMap()

    log.Infof("Successfully added Router AS%d to the topology", r.ASNumber)
}

 
// Main routing related function for finding routes to a specific destination, it is 
// the one of the core functions within the routing logic of the simulator
func (t *Topology) FindRoutesTo(target *router.Router) {
  log.Infof("Starting route propagation to target AS%d", target.ASNumber)

  // Initializing the queue for finding routes to the target
  routes := deque.NewDeque()
  
  // Step 0: Initially force current router to itself, but before that
  // we need to construct the related route instance
  route := &router.Route{
    Dest: target,
    Path: []*router.Router{target},
    OriginInvalid: false,
    PathEndInvalid: false,
    Authenticated: true,
  }
  target.ForceRoute(route)
  log.Debugf("Forcing route to target AS%d", target.ASNumber)

  // Step 1: Add neighbors of the target router to the queue
  for _, neighbor := range target.Neighbors {
    // We need to create route object for each neighbor of the target
    // and add it to the queue
    routes.PushBack(target.OriginateRoute(neighbor.Router))
    log.Debugf("Adding neighbor AS%d to the queue", neighbor.Router.ASNumber)
  }

  // Step 3: Loop in while until all the possible routes are exhausted
  // such as no possible routes left according to policies
  for routes.Len() > 0 {
    route := routes.PopFront().(*router.Route)
    final := route.Path[len(route.Path)-1]
    log.Debugf("Processing route to AS%d via AS%d", route.Dest.ASNumber, final.ASNumber)
    for _, neighbor := range final.LearnRoute(route) {
      routes.PushBack(final.ForwardRoute(route, neighbor))
      log.Debugf("Forwarded route from AS%d to neighbor AS%d", final.ASNumber, neighbor.ASNumber)
    }
  } 
  log.Infof("Completed route propagation to target AS%d", target.ASNumber)
}

// Method for returning humand readable string representation of a topology 
func (t *Topology) PrintTopology() {
  for node := range t.AdjMap {
    router := t.GetRouter(node)
    fmt.Printf("Node: %s", router.ToString())
    fmt.Println()
  }
}

// Method to obtain router from the topology given its AS number
func (t* Topology) GetRouter(routerHash string) *router.Router {
  g := t.G
  r, err := g.Vertex(routerHash)
  if(err != nil) {
    if(err == graph.ErrVertexNotFound) {
      fmt.Println("Router not found")
      return nil
    } else {
      fmt.Println("Error retrieving router")
    }
  } else {
    return r
  }
  return nil
}

// Helper function for inversing a relation such as, given provider, it's inverse is customer
// for peer it is same, it is required when adding neighbors of routers, especially on Init()
// funciton
func inverseRelation(rel router.Relation) router.Relation {
  switch rel {
  case router.Customer:
      return router.Provider
  case router.Provider:
      return router.Customer
  case router.Peer:
      return router.Peer
  default:
      return rel
  }
}

// Helper function for hashing routing integers to string since it is quite used a lot in this
// package, instead of defining in the Init() function as in the
// routerHash := func(r *router.Router) string {
//   return fmt.Sprintf("r%d", r.ASNumber)
// }
func RouterHash(r *router.Router) string {
  return fmt.Sprintf("r%d", r.ASNumber)
}

// Method for determining reachability of a given AS, from every other AS in the graph,
// ASes are taken from the AS map initialized in the Init() function, and for each AS
// obtained from that map, GetRouter function is called and from there on the reachability
// of the AS is calculated
//
// In the original Python code, following approach is preferred, utilizing more of the graph
// packages methods. 
func (t *Topology) Reachability() {

}

// Method for determining reachability of every AS to every other AS in the graph. It is
// expensive operation, most probably performs worse on larget topologies
// TODO could research on faster ways, parallelizing, caching, memoization etc.
func (t *Topology) ReachabilityAll() {

}

// Helpre function for Reachability and ReachabilityAll functions, it builds a reachability graph
// of the whole topology, with left and right indicators for each AS. It constructs in the end
// a directed graph
func (t* Topology) BuildReachabilityGraph() {

}



