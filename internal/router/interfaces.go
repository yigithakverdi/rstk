package router

type IPolicy interface {
  AcceptRoute(route *Route) bool
  PreferRoute(currentRoute, newRoute *Route) bool
  ForwardTo(route *Route, relation Relation) bool
  // HandleRouteAddition(router *Router, route *Route)
  // Could be used for handling route addition to routing table in more
  // granular approach such as
  // 
  // if r.Policy.HandleRouteAddition != nil {
  //   r.Policy.HandleRouteAddition(r, route)
  // } else {
  //     r.RouteTable[route.Dest.ASNumber] = route
  // }
}
