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
// or some templating or some configurations

// Below are all the data strcutres used within the topology

// Main topology structure, encapsulating the graph and map of ASes in the
// topology for easy access, however this creates duality, so it must be carefully
// handled possibly with the direct memory referncing from topology
type Topology struct {
  G graph.Graph[string, *router.Router]
  ASES map[int]*router.Router

  // Predecessor map and ancestor map is pre-computed, and stored in the topology as well
  // this significantly reduces the computation time with the downside of memory usage
  // for larger topologies
  PredMap map[string]map[string]graph.Edge[string]
  AdjMap map[string]map[string]graph.Edge[string]  
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
func (t* Topology) Init(asRelsList []parser.AsRel) (graph.Graph[string, *router.Router], map[int]*router.Router) {
  
  // Store ASes in lookup table for easy access
  ases := make(map[int]*router.Router)

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
    // currently every router initialized with default policy
    if _, exists := ases[as1]; !exists {
      ases[as1] = &router.Router{
        ASNumber: as1, 
        Policy: router.Policy{PolicyType: "DefaultPolicy"},
        RouteTable: make(map[int]*router.Route),
      }
    }
    if _, exists := ases[as2]; !exists {
      ases[as2] = &router.Router{
        ASNumber: as2, 
        Policy: router.Policy{PolicyType: "DefaultPolicy"},
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

    // For bidirectional relationships
    neighbor2 := router.Neighbor{
        Relation: inverseRelation(router.Relation(relation)),
        Router:   ases[as1],
    }
    ases[as2].Neighbors = append(ases[as2].Neighbors, neighbor2)

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
