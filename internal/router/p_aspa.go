package router


import (
  "fmt"
  log "github.com/sirupsen/logrus"
)

// Ensure ASPA policy implements the Policy interface
var _ IPolicy = (*ASPAPolicy)(nil)

// ASPAPolicy contains everything methods, need. In this case, it contains, U-SPAS table
// neighbors of the current router, and the current AS number
type ASPAPolicy struct {  
  // Adding a back reference to the router itself, since I need to access router related
  // fields instead of recrating them here causing more problems and redundancy
  Router    *Router
}


// Helper function that builds route instance with it's AS path reversed
func (r *Route) Reverse() *Route {
  reversedPath := make([]*Router, 0)
  for i := len(r.Path) - 1; i >= 0; i-- {
    reversedPath = append(reversedPath, r.Path[i])
  }
  return &Route{
    Dest: r.Dest,
    Path: reversedPath,

    Authenticated: r.Authenticated,
    OriginInvalid: r.OriginInvalid,
    PathEndInvalid: r.PathEndInvalid,
  }
}
    

func (ap *ASPAPolicy) AcceptRoute(route *Route) bool {
  result, err := ap.Router.PerformASPA(route)
  
  if err != nil {
    fmt.Printf("Error: %s\n", err)
    return false
  }
  fmt.Printf("ASPA Validation Result: %s\n", result)
  
  if !route.ContainsCycle() && result == ASPAInvalid {
    return false
  }
  return true
}

// Helper function for obtaining only unique AS numbers from the AS path, kind of a 
// compressed AS path
func (r *Route) compressASPATH() (uniqueASes []*Router) {
  asSet := make(map[int]bool)
  uniqueASes = make([]*Router, 0)
  for _, asys := range r.Path {
    asNum := asys.ASNumber
    if !asSet[asNum] {
        asSet[asNum] = true
        uniqueASes = append(uniqueASes, asys)
    }
  }
  return uniqueASes
} 

func (r *Relation) ToString() string {
  var relationString string
  switch *r {
    case Customer:
      relationString = "Customer"
    case Peer:
      relationString = "Peer"
    case Provider:
      relationString = "Provider"
    default:
      relationString = "Unknown"
  }
  return relationString
}

func (r *Router) PerformASPA(route *Route) (ASPAResult, error) {
    // Reverse the AS path
    route = route.Reverse()

    log.Infof("Received route %v ASPA policy for current AS%d", route.PathASNumbers(), r.ASNumber)

    if len(route.Path) < 2 {
        log.Warnf("Route length below verifiable")
        return ASPAInvalid, fmt.Errorf("Route length below verifiable")
    }

    neighborAS := route.Path[1]
    relation := r.GetRelation(*neighborAS)
    
    log.Infof("AS%d is %v of AS%d", neighborAS.ASNumber, relation.ToString(), r.ASNumber)

    uniqueASes := route.compressASPATH()
    log.Infof("Unique ASes in the path: %v", uniqueASes)
    N := len(uniqueASes)
    log.Infof("Length of the path: %d", N)
    if N < 2 {
        log.Warnf("Route length below verifiable")
        return ASPAInvalid, fmt.Errorf("Route length below verifiable")
    }

    var result ASPAResult

    // Determine if we are in upstream or downstream verification
    log.Infof("Length of unique AS path %d (N)", N)
    for _, as := range uniqueASes {
      log.Infof("AS: %d", as.ASNumber)
    }
    if relation == Customer || relation == Peer {
        log.Infof("Evaluating under customer or peer relation with upstream path verification")
        result = r.upstreamPathVerification(uniqueASes, N)
    } else if relation == Provider {
        log.Infof("Evaluating under provider relation with downstream path verification")
        result = r.downstreamPathVerification(uniqueASes, N)
    } else {
        log.Warnf("Unknown or unsupported neighbor relation for AS%d", neighborAS.ASNumber)
        return ASPAInvalid, fmt.Errorf("Unknown relationship type: %v", relation)
    }
    log.Infof("ASPA validation result: %v", result)
    return result, nil
}

func (r *Router) upstreamPathVerification(uniqueASes []*Router, N int) ASPAResult {
    log.Infof("Performing upstream path verification")
    // Steps 1-3 are omitted as they are handled earlier
    // Compute max_up_ramp and min_up_ramp
    maxUpRamp, minUpRamp := r.computeUpRamp(uniqueASes, N)
    log.Infof("Max up ramp: %d, Min up ramp: %d", maxUpRamp, minUpRamp)
    if maxUpRamp < N {
        log.Warnf("Max up ramp is less than N")
        return ASPAInvalid
    } else if minUpRamp < N {
        log.Warnf("Min up ramp is less than N")
        return ASPAUnknown
    } else {
        log.Infof("maxUpRamp(%d) >= N and minUpRamp(%d) >= N(%d)", maxUpRamp, minUpRamp, N)
        log.Infof("ASPA validation successful")
        return ASPAValid
    }
}

func (r *Router) downstreamPathVerification(uniqueASes []*Router, N int) ASPAResult {
    log.Infof("Performing downstream path verification")
    // Steps 1-3 are omitted as they are handled earlier
    // Compute max_up_ramp, min_up_ramp, max_down_ramp, min_down_ramp
    maxUpRamp, minUpRamp := r.computeUpRamp(uniqueASes, N)
    maxDownRamp, minDownRamp := r.computeDownRamp(uniqueASes, N)
    log.Infof("Max up ramp: %d, Min up ramp: %d, Max down ramp: %d, Min down ramp: %d",
                                        maxUpRamp, minUpRamp, maxDownRamp, minDownRamp)

    if maxUpRamp+maxDownRamp < N {
        log.Warnf("Max up ramp + max down ramp is less than N")
        return ASPAInvalid
    } else if minUpRamp+minDownRamp < N {
        log.Warnf("Min up ramp + min down ramp is less than N")
        return ASPAUnknown
    } else {
        log.Infof("maxUpRamp(%d) + maxDownRamp(%d) >= N and minUpRamp(%d) + minDownRamp(%d) >= N(%d)",
                                                    maxUpRamp, maxDownRamp, minUpRamp, minDownRamp, N)
        log.Infof("ASPA validation successful")
        return ASPAValid
    }
}

func (r *Router) computeUpRamp(uniqueASes []*Router, N int) (int, int) {
    log.Infof("Computing up ramp")
    maxUpRamp := N
    minUpRamp := N
    log.Infof("Max and min up ramp set to N(%d) initially", N)
    for i := 0; i+1 < N; i++ { 
        currAS := uniqueASes[i]
        nextAS := uniqueASes[i+1]
        log.Infof("Current AS: %d, Next AS: %d", currAS.ASNumber, nextAS.ASNumber)

        authResult := r.authorized(currAS, nextAS)
        log.Infof("Authorization result: %v", authResult)
        if authResult == "Not Provider+" && maxUpRamp == N {
            log.Infof("Max up ramp set to %d", i+1)
            maxUpRamp = i + 1
        }
        if (authResult == "Not Provider+" || authResult == "No Attestation") && minUpRamp == N {
            log.Infof("Min up ramp set to %d", i+1)
            minUpRamp = i + 1
        }
    }
    return maxUpRamp, minUpRamp
}

func (r *Router) computeDownRamp(uniqueASes []*Router, N int) (int, int) {
    log.Infof("Computing down ramp")
    maxDownRamp := N
    minDownRamp := N
    for i := N - 1; i > 0; i-- {
        currAS := uniqueASes[i]
        prevAS := uniqueASes[i-1]
        log.Infof("Current AS: %d, Previous AS: %d", currAS.ASNumber, prevAS.ASNumber)
        authResult := r.authorized(currAS, prevAS)
        log.Infof("Authorization result: %v", authResult)
        if authResult == "Not Provider+" && maxDownRamp == N {
            log.Infof("Max down ramp set to %d", N-i)
            maxDownRamp = N - i
        }
        if (authResult == "Not Provider+" || authResult == "No Attestation") && minDownRamp == N {
            log.Infof("Min down ramp set to %d", N-i)
            minDownRamp = N - i
        }
    }
    return maxDownRamp, minDownRamp
}

func (r *Router) authorized(cas *Router, pas *Router) string {
    if cas.ASPAList == nil || len(cas.ASPAList) == 0 {
        return "No Attestation"
    }
    providers := cas.ASPAList[0].ASPASet
    if containsInt(providers, pas.ASNumber) {
        return "Provider+"
    }
    return "Not Provider+"
}



func containsInt(s []int, e int) bool {
    for _, a := range s {
        if a == e {
            return true
        }
    }
    return false
}

// Method for preferring route, by comparing the two routes, and choosing the preferred one
// to prefer different rules are used defined on the preferenceRules() as []PreferenceRule
// slices, depending on the outcome of those rules either true or false returned indicating
// route is preferred or not
func (dp *ASPAPolicy) PreferRoute(currentRoute, newRoute *Route) bool {
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
func (ap *ASPAPolicy) PreferenceRules() []PreferenceRule {
    return []PreferenceRule{
        localPref,
        asPathLength,
        nextHopASNumber,
    }
}

// TODO invalid return value might cause some problems here, also need to check if get relation
// is working correctly or not
func (ap *ASPAPolicy) ForwardTo(route *Route, relation Relation) bool {
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


// Helper function for checking if an element exists in a slice
func contains(s []int, e int) bool {
  for _, a := range s {
      if a == e {
          return true
      }
  }
  return false
}

// ASPAResult represents the outcome of the ASPA algorithm.
type ASPAResult string

const (
    ASPAValid   ASPAResult = "Valid"
    ASPAInvalid ASPAResult = "Invalid"
    ASPAUnknown ASPAResult = "Unknown"
)

// Helper function to check if a key exists in a map[int]struct{}
func Contains(m map[int]struct{}, key int) bool {
	_, exists := m[key]
	return exists
}

// Check if ASPA exists in the router
func hasASPA(router *Router) bool {
  return router.ASPAList != nil && len(router.ASPAList) > 0
}
