package router

// Preference rule, decides on the preference of the route based on the relation between the
// final AS and the first hop after the destination
func localPref(route *Route) int {
    final := route.Path[len(route.Path)-1]
    firstHop := route.Path[len(route.Path)-2]

    if firstHop == nil {
      // Invalid route for this rule  
      return -1 
    }  

    relation := final.GetRelation(*firstHop)
    if relation != 2 {
        return int(relation)
    }
    // If no valid relation found (in this case relation is returend 2 which is invalid) then
    // by defualt returning -1 since this is the original case from the reference
    return -1
}

// Preference rule, decides on the preference of the route based on the length of the 
// AS path
func asPathLength(route *Route) int {
    return len(route.Path)
}

// Preference rule, decides on the preference of the route based on the AS number of the 
// next hop
func nextHopASNumber(route *Route) int {
  firstHop := route.Path[len(route.Path)-2]
  if firstHop != nil {
    return firstHop.ASNumber
  }
  return -1
}
