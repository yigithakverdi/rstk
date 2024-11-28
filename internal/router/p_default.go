package router

import (
  "fmt"

  log "github.com/sirupsen/logrus"
)

// Ensure DefaultPolicy implements the Policy interface
var _ IPolicy = (*DefaultPolicy)(nil)

// Preference rule type for defining the preference rules for the routes
// later on used on the choosing a route out of two, used in the PreferRoute functions
// as slice type
type PreferenceRule func(route *Route) int

// Parnet policy struct that should enacpsulate variuous policy implementaitons of the user
// intial or base policy is default that accepts route if it does not contain cycle, and
// other very basic policy implementations
type DefaultPolicy struct {}

// Method for accepting route, by default if the route does not contain cycle, it is accepted
func (dp *DefaultPolicy) AcceptRoute(route *Route) bool {
  log.Infof("Route contains cycle: %v", route.ContainsCycle())
  return !route.ContainsCycle()
}

// Method for preferring route, by comparing the two routes, and choosing the preferred one
// to prefer different rules are used defined on the preferenceRules() as []PreferenceRule
// slices, depending on the outcome of those rules either true or false returned indicating
// route is preferred or not
func (dp *DefaultPolicy) PreferRoute(currentRoute, newRoute *Route) bool {
    // Ensure that both routes have the same destination
    if currentRoute.Dest.ASNumber != newRoute.Dest.ASNumber {
        fmt.Printf("Routes must have the same destination AS. Current route to AS%d, new route to AS%d\n",
            currentRoute.Dest.ASNumber, newRoute.Dest.ASNumber)
        return false
    }

    // Iterate over the preference rules
    for _, rule := range dp.PreferenceRules() {
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
func (dp *DefaultPolicy) PreferenceRules() []PreferenceRule {
    return []PreferenceRule{
        localPref,
        asPathLength,
        nextHopASNumber,
    }
}

// TODO invalid return value might cause some problems here, also need to check if get relation
// is working correctly or not
func (dp *DefaultPolicy) ForwardTo(route *Route, relation Relation) bool {
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
