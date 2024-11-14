package router 

import (
  "fmt"
)

// Preference rule type for defining the preference rules for the routes
// later on used on the choosing a route out of two, used in the PreferRoute functions
// as slice type
type PreferenceRule func(route *Route) int

// Parnet policy struct that should enacpsulate variuous policy implementaitons of the user
// intial or base policy is default that accepts route if it does not contain cycle, and
// other very basic policy implementations
type Policy struct {
  PolicyType string
}

// Method for accepting route, by default if the route does not contain cycle, it is accepted
func (p *Policy) AcceptRoute(route *Route) bool {
  return !route.ContainsCycle()
}

// Method for preferring route, by comparing the two routes, and choosing the preferred one
// to prefer different rules are used defined on the preferenceRules() as []PreferenceRule
// slices, depending on the outcome of those rules either true or false returned indicating
// route is preferred or not
func (p *Policy) PreferRoute(currentRoute, newRoute *Route) bool {
    // Ensure that both routes have the same destination
    if currentRoute.Dest.ASNumber != newRoute.Dest.ASNumber {
        fmt.Printf("Routes must have the same destination AS. Current route to AS%d, new route to AS%d\n",
            currentRoute.Dest.ASNumber, newRoute.Dest.ASNumber)
        return false
    }

    // Iterate over the preference rules
    for _, rule := range p.preferenceRules() {
        currentVal := rule(currentRoute)
        newVal := rule(newRoute)

        if currentVal != -1 && newVal != -1 {
            if currentVal < newVal {
                // Current route is preferred
                return false
            }
            if newVal < currentVal {
                // New route is preferred
                return true
            }
            // If values are equal, continue to the next rule
        }
    }

    // If all preference rules are equal, do not prefer the new route
    return false
}

// Core method for defining the preference rules for the routes, to be used in the
// PreferRoute method, returns []PreferenceRule slice containing the rules for the routes
func (p *Policy) preferenceRules() []PreferenceRule {
    return []PreferenceRule{
        localPref,
        asPathLength,
        nextHopASNumber,
    }
}

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

// TODO invalid return value might cause some problems here, also need to check if get relation
// is working correctly or not
func (p *Policy) ForwardTo(route *Route, relation Relation) bool {
  final := route.Path[len(route.Path)-1]    // Last AS in the path
  firstHop := route.Path[len(route.Path)-2] // AS before the final AS
  
  // Get the relation between final and first hop
  firstHopRel := final.GetRelation(*firstHop)
  if firstHopRel == Invalid {
    fmt.Println("First hop relation not found")
    return false
  }

  return firstHopRel == Customer || relation == Customer  
}
