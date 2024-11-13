package router 

import (
  "fmt"
  "rstk/internal/interfaces"
)

const (
    Customer interfaces.Relation = -1
    Peer     interfaces.Relation = 0
    Provider interfaces.Relation = 1
)

// Ensure Router implements interfaces.Router
var _ interfaces.Router = &Router{}

// Ensure Route implements interfaces.Route
var _ interfaces.Route = &Route{}

// Router model, could be AS, could be inra-AS router, could be a clinet
// defined for broad capturing of the routing operations
type Interface struct {
    Name string
    IP   string
}

type Packet struct {
    SourceAddr      string
    DestinationAddr string
    Size            int
    Protocol        string
}

type Route struct {
  FinalAS         interfaces.Router 
  FirstHopAS      interfaces.Router 
  Path            []interfaces.Router
  
  PathLength      int
	Authenticated   bool
	OriginInvalid   bool
  PathEndInvalid  bool
  Packet          Packet 
}

func (r* Route) GetPath() []interfaces.Router {
  return r.Path
}

func (r* Route) ContainsCycle() bool {
  path := r.Path
  visited := make(map[int]bool)
  for _, router := range path {
    if visited[router.GetASNumber()] {
      return true
    }
    visited[router.GetASNumber()] = true
  }
  return false
}

// func (r *Route) ToString() string {
//   routerString := "" 
//   routerString += fmt.Sprintf("<Origin router:%d", r.Origin().GetASNumber())
//   routerString += fmt.Sprintf(", Final router:%d", r.GetFinal().GetASNumber())
//
//   routerString += ", Path: "
//   for _, router := range r.GetPath() {
//     routerString += fmt.Sprintf("%d ", router.GetASNumber())
//   }
//   routerString += ">"
//   return routerString
// }

// String representation of the route, in the form of
// Route(dest=4, path=[AS-4])

func (r *Route) ToString() string {
    pathString := ""
    for i, hop := range r.Path {
        if i > 0 {
            pathString += ", "
        }
        
        pathString += fmt.Sprintf("AS-%d", hop.GetASNumber()) 
    }
    return fmt.Sprintf("Route(dest=%d, path=[%s])", r.GetFinal().GetASNumber(), pathString)
}


func (r *Route) Length() int {
  return r.PathLength
}

func (r *Route) Origin() interfaces.Router {
  if(len(r.Path) > 0) {
    return r.GetPath()[0]
  }
  return nil
}

func (r *Route) GetAuthenticated() bool {
  return r.Authenticated
}

func (r *Route) GetFinal() interfaces.Router {
  if(len (r.Path) > 0) {
    return r.GetPath()[len(r.Path) - 1]
  }
  return nil 
}

func (r *Route) GetFirstHop() interfaces.Router {
  // r.Path holds addresses of the list of routers such as 0xc0008214b70
  // thus printing AS number of the first element in the path as

  return r.Path[0]
}

func (r *Route) GetFinalAS() interfaces.Router {
  return r.FinalAS
}

func (r *Route) IsAuthenticated() bool {
  return r.Authenticated
}

func (r *Route) IsOriginInvalid() bool {
  return r.OriginInvalid
}

func (r *Route) IsPathEndInvalid() bool {
  return r.PathEndInvalid
}

type Router struct {
  Providers  []interfaces.Router
  Peers      []interfaces.Router
  Customers  []interfaces.Router
  Interfaces []interfaces.Relation
  Policy     interfaces.Policy
  Neighbors  []interfaces.Neighbor
  RouteTable map[int]interfaces.Route

	ID         string
  RouterType string
  ASNumber   int
	RouterID   string
  isCritical bool

}

// Router to string
func (r *Router) ToString() string {
  return fmt.Sprintf("Router %d", r.ASNumber)
}

func (r *Router) SetPolicy(policy interfaces.Policy) {
  r.Policy = policy
}

func (r *Router) GetPolicy() interfaces.Policy {
  return r.Policy
}

func (r *Router) GetRouteTable() map[int]interfaces.Route {
  return r.RouteTable
}

// func (r *Router) GetPacket() Packet {
//   return Packet(r.RouteTable[len(r.RouteTable) - 1].Packet)
// }

func (r *Router) GetASNumber() int {
  return r.ASNumber
}

func (r *Router) IsCritical() bool {
  return r.isCritical
}

func (r *Router) ResetRouteTable() {
  r.RouteTable = make(map[int]interfaces.Route)
}

func (r *Router) ForceRoute(route interfaces.Route) {
  r.RouteTable[route.GetFinal().GetASNumber()] = route 
}

func (r *Router) IsInPath(path []interfaces.Router) bool {
  for _, router := range path[:len(path) - 1] {
        if router.GetASNumber() == r.GetASNumber() {
            return true
        }
    }
    return false
}


func (r *Router) LearnRoute(route interfaces.Route) []interfaces.Neighbor {
    fmt.Println()
    fmt.Println("   Learning route...")
    fmt.Println("   Route:", route.ToString())  
    fmt.Println("   Current router:", r.GetASNumber())
    // If the route's destination is the router itself, do nothing
    // if route.GetFinal().GetASNumber() == r.GetASNumber() {
    //     return nil
    // }

    if r.IsInPath(route.GetPath()) {
      fmt.Println("   Route contains a loop. Discarding.")
      return nil
    }


    // Check if the route is acceptable according to the policy
    if !r.Policy.AcceptRoute(route) {
        return nil
    }
    
    fmt.Println("   Route accepted")

    // Check if the new route is preferred over the existing one
    routeTable := r.GetRouteTable()
    
    fmt.Println("   Checking if route is preferred...")  
    existingRoute, exists := routeTable[route.GetFinal().GetASNumber()]
    fmt.Println("   Does route exist in the route table:", exists)
    // fmt.Printf("   Route table: %v, Final AS in the route %d \n", r.RouteTable, route.GetFinal().GetASNumber())
    // Print route table
    for destAS, route := range r.RouteTable {
      fmt.Printf("    [%d]: %s\n", destAS, route.ToString())
    }

    if exists && !r.Policy.PreferRoute(existingRoute, route) {
      return nil
    }
    
    // Update the routing table with the new route
    // r.RouteTable[route.GetFinal().GetASNumber()] = route 
    r.RouteTable[route.GetFirstHop().GetASNumber()] = route
    fmt.Println("   Route learned")
    fmt.Println("   Route table updated:")
    // fmt.Println("   Route table: ", r.RouteTable)
    for destAS, route := range r.RouteTable {
      fmt.Printf("    [%d]: %s\n", destAS, route.ToString())
    }
  
    // Determine which neighbors to advertise the route to
    forwardToRelations := make(map[interfaces.Relation]bool)    
    relations := []interfaces.Relation{interfaces.Provider, interfaces.Peer, interfaces.Customer}
    for _, relation := range relations {
      if r.Policy.ForwardTo(route, relation) {
            forwardToRelations[relation] = true
        }
    }

    // Collect neighbors to advertise to
    neighborsToAdvertise := []interfaces.Neighbor{}
    for _, neighbor := range r.Neighbors {
        if forwardToRelations[neighbor.Relation] {
            neighborsToAdvertise = append(neighborsToAdvertise, neighbor)
        }
    }

    return neighborsToAdvertise
}

// Create router object, it creates a router object then
// it assigns a default policy using create policy which where the
// policies are created (for now it assings default policy for 
// every router) 
func CreateRouter(asNumber int, policy interfaces.Policy) *Router {
  return &Router{
    ASNumber: asNumber,
    RouteTable: make(map[int]interfaces.Route),
    Policy: policy,
  }
}


// type Route struct {
//   FinalAS         interfaces.Router 
//   FirstHopAS      interfaces.Router 
//   Path            []interfaces.Router
//
//   PathLength      int
// 	Authenticated   bool
// 	OriginInvalid   bool
//   PathEndInvalid  bool
//   Packet          Packet 
// }

// def originate_route(self, next_hop: 'AS') -> 'Route':
//     return Route(
//         dest=self.as_id,
//         path=[self, next_hop],
//         origin_invalid=False,
//         path_end_invalid=False,
//         authenticated=self.bgp_sec_enabled,
//     )


// OriginateRoute creates a new Route starting from this router and going to nextHop.
func (r *Router) OriginateRoute(nextHop interfaces.Router) interfaces.Route {
    return &Route{
        FinalAS:         r,                       // Final destination is the current router
        Path:            []interfaces.Router{r, nextHop}, // Path includes current router and next hop
        OriginInvalid:   false,
        PathEndInvalid:  false,
        Authenticated:   true,                  // Set based on whether the current router is critical
    }
}

// ForwardRoute forwards an existing route to the nextHop.
func (r *Router) ForwardRoute(route interfaces.Route, nextHop interfaces.Router) interfaces.Route {
    return &Route{
        FinalAS:         route.GetFinal(),
        Path:            append(route.GetPath(), nextHop),           // Append next hop to the existing path
        OriginInvalid:   route.IsOriginInvalid(),
        PathEndInvalid:  route.IsPathEndInvalid(),
        Authenticated:   route.IsAuthenticated(), // Maintain authentication only if both routers are secure
    }
}
