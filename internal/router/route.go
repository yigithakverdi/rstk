package router

import (
  // Core library encodings
  "fmt"

  // Internal libraries

  // Github packages
)

// Route struct is used for storing routing information, such as the path, path length,
type Route struct {
  Dest            *Router 
  Path            []*Router
  
	Authenticated   bool
	OriginInvalid   bool
  PathEndInvalid  bool
}

// Method for returning humand readable string representation of a route
func (r *Route) ToString() string {
    asNumbers := r.PathASNumbers()
    return fmt.Sprintf("Dest AS%d via path %v", r.Dest.ASNumber, asNumbers)
}

// Method for checking if the route containcycles or not, depending on the
// result of the check, route might be discarded or accepted to be recorded into
// routing table of the router
func (r* Route) ContainsCycle() bool {
  path := r.Path
  visited := make(map[int]bool)
  for _, router := range path {
    if visited[router.ASNumber] {
      return true
    }
    visited[router.ASNumber] = true
  }
  return false
}

func (route *Route) PathASNumbers() []int {
    asNumbers := make([]int, len(route.Path))
    for i, router := range route.Path {
        asNumbers[i] = router.ASNumber
    }
    return asNumbers
}
