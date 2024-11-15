package router

import (
  // Core library encodings
  "fmt"
  "strings"

  // Internal libraries

  // Github packages
  log "github.com/sirupsen/logrus"
)

// Main node structure, encapsulate all of the router related logic, such as routing,
// policy, learning routes

// TODO some of the routing related functions are implemented in the topology (graph)
// it should be moved to router package, since it is closely related to the router/routing
// logic, such as FindRoutesTo

// Relation is a type for storing AS relationships
type Relation int

// Constants for AS relationships
const (
  Customer Relation = -1
  Peer     Relation = 0
  Provider Relation = 1
  Invalid  Relation = 2
)

// Neighbor struct is used for storing the neighbors of a router, such as IP, relation
// also all the neighbor related operations about router is handled here, if the neighbor
// related operations gets more complex, it can be moved to a separate file inside router
// package as `neighbor.go`
type Neighbor struct {
  Relation Relation
  Router   *Router
}

// Router struct encapsulates, routing tabales, policies, AS numbers, IP, interfaces etc.
// TODO some optimization might be required on how providers, peers, customers are stored
// instead of storing them seperately we can only use neighbors since neighbor struct
// also encapsulates the relation between the routers
type Router struct {
  Neighbors  []Neighbor
  RouteTable map[int]*Route

  ASNumber   int
  Policy     Policy  
}

// Method for returning human readable string format of the neighbor
func (n Neighbor) ToString() string {
	return fmt.Sprintf("<AS%d (%d)>", n.Router.ASNumber, n.Relation)
}

// Method for returning humand readable string representation of a router
func (r Router) ToString() string {
	var sb strings.Builder
	sb.WriteString(fmt.Sprintf("Router AS%d\n", r.ASNumber))
	if len(r.Neighbors) > 0 {
		sb.WriteString("    Neighbors: [")
    for _, neighbor := range r.Neighbors {
      sb.WriteString(fmt.Sprintf("%v,", neighbor.ToString()))
		}
	} else {
		sb.WriteString("No neighbors configured.\n")
  }
  
  // TODO trimming needs to be done for the neighbors string, since it will have a comma
  // as it is done on the routing table, as done below
  sb.WriteString("]\n")

  // After showing the neighbors of the router, also show the routing table of the router as well
  // with the same indentation level of the neighbor so that they are aligned and looking good
  sb.WriteString("    Routing Table: [")
  for _, route := range r.RouteTable {
    sb.WriteString(fmt.Sprintf("%v,", route.ToString()))
  }  

  // Before returning remove the last comma from the string (comma before the closing bracket)
  // such as making it from ,] to ]
  return strings.TrimSuffix(sb.String(), ",") + "]"  
}

// Method for checking relation of the current instance of the router with the given router
func (r *Router) GetRelation(router Router) Relation {
  for _, neighbor := range r.Neighbors {
    if neighbor.Router.ASNumber == router.ASNumber {
      return neighbor.Relation
    }
  }
  fmt.Println("Router not found in the neighbors, returning Invalid (2)")
  return Invalid
}

// Method for learning route according to policies and state of the routing table and route
// information recieved, it returns the neighbors that can recieve the route, or forwardable
// neighbors according to policy (customer, peer, provider)
func (r *Router) LearnRoute(route *Route) []*Router {
  log.Debugf("AS%d is learning route to AS%d via path %v", r.ASNumber, route.Dest.ASNumber, route.PathASNumbers())

  // If the current reference innstance of router is the same as the destination of the route
  // then the route is not valid, since the route cannot be same as the current routers
  // destination
  if r.ASNumber == route.Dest.ASNumber {
    log.Debugf("Route is not valid, since the destination is the same as the current router")
    return []*Router{}
  }

  // Every route should be filtered through "Accept Route Policy", which can vary according to
  // policy implementation of the user, however by default for now all the policies are defined
  // and considered under DefaultPolicy, thus "Accept Route Policy" is for now checking if contains
  // cycle or not
  if route.ContainsCycle() {
    log.Debugf("Route is not valid, since it contains cycle")
    return []*Router{}
  }

  // Final check is, if route exists in the routing table, of the current router instance, and
  // if the route in the routing table is preferred over the recieved route then we return 
  // empty list of routers, preference check is done through PreferRoute method
  if existingRoute, ok := r.RouteTable[route.Dest.ASNumber]; ok {
    if r.Policy.PreferRoute(route, existingRoute) {
      log.Debugf("Route is not valid, since it is not preferred over the existing route")
      return []*Router{}
    }
  }

  // If route passes through all the checks on above:
  //    0. Destination is not the same as the current router
  //    1. Route does not contain cycle
  //    2. Route is not the same as the current router
  // Then we can add the route to the routing table of the current router instance
  r.RouteTable[route.Dest.ASNumber] = route
  log.Infof("AS%d added route to AS%d via path %v", r.ASNumber, route.Dest.ASNumber, route.PathASNumbers())
  log.Infof("Router state after adding route:\n%s", r.ToString())

  // Deciding which neighbor should receive the propagation of current route recieved
  // it is done through ForwardTo method, which is implemented in the policy, for now
  // it is checking if the first hop is customer or not, if it is then the route is forwarded
  // to the first hop, if not then it is not forwarded
  forwardToRelations := make(map[Relation]bool)
  allRelations := []Relation{Customer, Peer, Provider}
  for _, relation := range allRelations {
    if r.Policy.ForwardTo(route, relation) {
      forwardToRelations[relation] = true
    }
  }
  
  // Collecting neighbors to forward route to
  var neighbors []*Router
  for _, neighbor := range r.Neighbors {
    if forwardToRelations[neighbor.Relation] {
      neighbors = append(neighbors, neighbor.Router)
      log.Debugf("AS%d will forward route to neighbor AS%d (relation: %v)", r.ASNumber, 
        neighbor.Router.ASNumber, neighbor.Relation)      
    }
  }
  return neighbors
}

// Function for obtaining peers, providers and customer of the current router instnaces
// caled with it, they are filtered out through the neighbor list according to relation
// values
func (r *Router) GetPeers() []Neighbor {
    return r.getNeighborsByRelation(Peer)
}

func (r *Router) GetCustomers() []Neighbor {
    return r.getNeighborsByRelation(Customer)
}

func (r *Router) GetProviders() []Neighbor {
    return r.getNeighborsByRelation(Provider)
}

// Helper function for obtaining neighbors by relation type
func (r *Router) getNeighborsByRelation(relation Relation) []Neighbor {
    var neighbors []Neighbor
    for _, neighbor := range r.Neighbors {
        if neighbor.Relation == relation {
            neighbors = append(neighbors, neighbor)
        }
    }
    return neighbors
}

// Method for creating a new router instance, only AS number is required for creating
// rest is set to default values, such as empty routing table, empty neighbors etc. 
// user can set these values later on
func NewRouter(asNumber int) *Router {
  return &Router{
    Neighbors:  []Neighbor{},
    RouteTable: make(map[int]*Route),
    ASNumber:   asNumber,
    Policy:     Policy{PolicyType: "DefaultPolicy"},
  }
}

// Function for forcing route, in some cases, route needs to be forced without pre-checks
// such as local preference, length etc.
func (r *Router) ForceRoute(route *Route) {
  r.RouteTable[route.Dest.ASNumber] = route
}

// TODO following methods, OriginateRoute, ForwardRoute, might not be correctly implemented
// such as the final AS and destination AS context might be confused here, the final AS interpreted
// here as the last element in the route.Path however, in our case usually when finding routes to
// using FindRoutesTo function first AS is the destination AS

// OriginateRoute creates a new Route starting from this router and going to nextHop.
func (r *Router) OriginateRoute(nextHop *Router) *Route {
    return &Route{
        Dest:            r,
        Path:            []*Router{r, nextHop},
        OriginInvalid:   false,
        PathEndInvalid:  false,
        Authenticated:   true,
    }
}

// ForwardRoute forwards an existing route to the nextHop.
func (r *Router) ForwardRoute(route *Route, nextHop *Router) *Route {    
    return &Route{
        Dest:            route.Dest, 
        Path:            append(route.Path, nextHop),
        OriginInvalid:   false, 
        PathEndInvalid:  false,
        Authenticated:   true,
    }
}

