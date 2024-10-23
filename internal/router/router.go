// This module covers all the abstractions for a router, major operations are
//
//        - Route functions
//        - Policy interfacing
//
// 

// type Route struct {
// 	Path          []string
// 	Final         string 
// 	FirstHop      string
// 	Authenticated bool
// 	OriginInvalid bool
// 	Packet        Packet 
// }

package router

// Import models package inside pkg
import (
  "rstk/pkg/models"
  "rstk/internal/graph"
  "github.com/edwingeng/deque"
  ggraph "github.com/dominikbraun/graph"
)

func ResetRouteTable() {

}

func ForceRoute() {

}

func FindrRoutesTo(g ggraph.Graph[int, int], targetAS models.Router) {
  routes := deque.NewDeque()
  targetNeighbors, err:= graph.GetNeighbors(targetAS, g)
  if err != nil {
    return 
  }

  for _, neighbor := range targetNeighbors { 
    routes.PushBack(targetAS.OrginateRoute((neighbor)))
  }

  for routes.Len() > 0 {
    route := routes.PopFront().(models.Route)
    finalRoute := route.Final()
    for _, neighbor := range finalRoute.LearnRoute(route) {
      routes.PushBack(finalRoute.ForwardRoute(route, neighbor))
    }
  }
}


