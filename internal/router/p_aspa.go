package router


import (
  "fmt"
  "strings"
  "rstk/internal/protocols"
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

// Helper function for reversing the AS_PATH as per the IETF draft definition of AS_PATH
func (ap *ASPAPolicy) reverseASPath(asPath []int) []int {
    reversed := make([]int, len(asPath))
    for i, j := 0, len(asPath)-1; i < len(asPath); i, j = i+1, j-1 {
        reversed[i] = asPath[j]
    }
    return reversed
}

func (ap *ASPAPolicy) AcceptRoute(route *Route) bool {
    log.Infof("Evaluating route with ASPA policy")

    // Get the AS_PATH as a slice of AS numbers
    asPath := route.PathASNumbers()

    if len(asPath) == 0 {
        log.Warn("AS_PATH is empty")
        return false
    } 
    
    // Reverse the AS_PATH to match the IETF definition, since my AS_PATH concepts is kind of
    // reversed such as at [5 3 1 12 15 6], 5 is the destination and 6 is the source, so reversing
    // makes it [6 15 12 1 3 5], now 6 is the source and 5 is the destination 
    asPath = ap.reverseASPath(asPath)

    // Compress the AS_PATH to remove duplicates
    // TODO not sure about the validty of this method of compressing AS path into
    // single union of ASes
    compressedPath := protocols.CompressASPath(asPath)
    log.Infof("Compressed AS_PATH: %v", compressedPath)


    neighborAS := compressedPath[1]
    log.Infof("Neighbor AS: %d", neighborAS)

    // Find the neighbor in the slice that matches the neighborAS
    // TODO here trying to find, the last AS in the compressed path in the neighbors list
    // by looping over all the neighbors in the policy struct of the current router
    // which is very inefficient (considering thousands of AS neighbors)
    //
    // Access neighbors from the router
    var sb strings.Builder
    neighbors := ap.Router.Neighbors
    sb.WriteString(fmt.Sprintf("Neighbor: "))
    for _, neighbor := range neighbors {
      sb.WriteString(fmt.Sprintf("%d ", neighbor.Router.ASNumber))
    }
    log.Infof(sb.String())

    var neighbor *Neighbor
    var exists bool

    for i := range neighbors {
        if neighbors[i].Router.ASNumber == neighborAS {
            neighbor = &neighbors[i]
            exists = true
            break
        }
    }
    fmt.Println()

    if !exists {
        log.Warnf("Neighbor AS%d not found in neighbor relations for AS%d", neighborAS, ap.Router.ASNumber)
        // Decide how to handle unknown neighbor relations (e.g., treat as Invalid)
        return false
    }
      
    var neighborRelation Relation = neighbor.Relation
    var outcome protocols.Outcome
    log.Infof("Neighbor relation: %v", neighborRelation)

    // Apply the appropriate verification algorithm based on neighbor relation
    log.Infof("Fetching RPKI instance")
    
    // Initializing U_SPAS table from RPKI.ASPA of the current router
    USPAS := ap.Router.ASPA.USPAS

    log.Infof("S-USPAS validation %v", USPAS)
    log.Infof("AS%d from which the route is received, and destination route AS%d", ap.Router.ASNumber, 
                                                                      compressedPath[len(compressedPath)-1])
    switch neighborRelation {
    case Customer, Peer:
        // Use the upstream verification algorithm
        // asPath []int, neighborAS int, isRSClient bool, uspaspTable USPASTable
        log.Infof("Evaluating under customer or peer relation")
        log.Infof("AS%d verifying route to AS%d: ASPA validation", ap.Router.ASNumber, route.Dest.ASNumber)
        outcome, _ = protocols.UpstreamVerifyASPath(ap.Router.ASNumber, compressedPath, USPAS)        
    case Provider:
        // Use the downstream verification algorithm
        log.Infof("Evaluating under provider relation")
        log.Infof("AS%d verifying route to AS%d: ASPA validation", ap.Router.ASNumber, route.Dest.ASNumber)
        outcome, _ = protocols.DownstreamVerifyASPath(ap.Router.ASNumber, compressedPath, USPAS)
  
    default:
        // If relation is unknown or complex, treat as Invalid
        log.Warnf("Unknown or unsupported neighbor relation for AS%d", neighborAS)
        outcome = protocols.Invalid
    }

    if outcome == protocols.Valid {
        log.Infof("AS%d accepted route to AS%d: ASPA validation %s", ap.Router.ASNumber, route.Dest.ASNumber, outcome)
        return true
    } else {
        log.Infof("AS%d rejected route to AS%d: ASPA validation %s", ap.Router.ASNumber, route.Dest.ASNumber, outcome)
        return false
    }
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

