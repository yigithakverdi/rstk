package models 

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

// Route realted functions
type Route struct {
	Path            []Router
  PathLength      int
  FinalAS         int 
  FirstHopAS      int
	Authenticated   bool
	OriginInvalid   bool
  PathEndInvalid  bool
  Packet          Packet 
}

func (r *Route) Length() int {
  return r.PathLength
}

func (r *Route) Origin() Router {
  if(len(r.Path) > 0) {
    return r.Path[0]
  }
  return Router{}
}

func (r *Route) FirstHop() Router {
  if(len(r.Path) > 1) {
    return r.Path[len(r.Path) - 2]
  }
  return Router{}
}

func (r *Route) Final() Router {
  if(len (r.Path) > 0) {
    return r.Path[len(r.Path) - 1]
  }
  return Router{}
}


type Router struct {
	ID         string
  RouterType string
  ASNumber   int
	RouterID   string
	Neighbors  []Neighbor
	Interfaces []Interface
  IsCritical bool
  RouteTable map[int]Route
  Policy     Policy
}


// Below is unused interfaces for decoupling graph from router and engine
// vice versa. Define an methods are implemented in their respective packages
// If the engine needs to interact with models.Router methods that depend on graph, 
// consider defining interfaces in models and implementing them in engine.
// type GraphProvider interface {
//     GetNeighbors(router Router, g ggraph.Graph[int, int]) ([]Router, error)
// }
//
// type RoutingEngine interface {
//     FindRoutesTo(router *Router, targetAS Router)
// }


func (r *Router) ResetRouteTable() {
  r.RouteTable = make(map[int]Route)
  r.RouteTable[r.ASNumber] = Route {
    Path : []Router{*r},
    OriginInvalid : false,
    PathEndInvalid : false,
    Authenticated : r.IsCritical,
  }
}

func (r *Router) GetRoute(targetAS Router) Route {
  return r.RouteTable[targetAS.ASNumber]
}

func (r *Router) ForceRoute(route Route) {
  r.RouteTable[route.FinalAS] = route
}

func (r *Router) LearnRoute(route Route) []Neighbor {
    // If the route's destination is the router itself, do nothing
    if route.FinalAS == r.ASNumber {
        return nil
    }

    // Check if the route is acceptable according to the policy
    if !r.Policy.AcceptRoute(route) {
        return nil
    }

    // Check if the new route is preferred over the existing one
    existingRoute, exists := r.RouteTable[route.FinalAS]
    if exists && !r.Policy.PreferRoute(existingRoute, route) {
        return nil
    }

    // Update the routing table with the new route
    r.RouteTable[route.FinalAS] = route

    // Determine which neighbors to advertise the route to
    forwardToRelations := make(map[Relation]bool)
    relations := []Relation{Provider, Peer, Customer}
    for _, relation := range relations {
        if r.Policy.ForwardTo(route, relation) {
            forwardToRelations[relation] = true
        }
    }

    // Collect neighbors to advertise to
    neighborsToAdvertise := []Neighbor{}
    for _, neighbor := range r.Neighbors {
        if forwardToRelations[neighbor.Relation] {
            neighborsToAdvertise = append(neighborsToAdvertise, neighbor)
        }
    }

    return neighborsToAdvertise
}


func (r *Router) OrginateRoute(nextHop Router) Route {
  return Route {
    Path : []Router{*r, nextHop},
    OriginInvalid : false,
    PathEndInvalid : false,
    Authenticated : r.IsCritical,
  }
}

func (r* Router) ForwardRoute(route Route, nextHop Router) Route {
  return Route {
    Path : append(route.Path, nextHop),
    OriginInvalid : route.OriginInvalid,
    PathEndInvalid : route.PathEndInvalid,
    Authenticated : route.Authenticated && nextHop.IsCritical,
  }
}
