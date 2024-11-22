package router


import (
  "fmt"
  "rstk/internal/protocols"
  log "github.com/sirupsen/logrus"
)

// Ensure ASPA policy implements the Policy interface
var _ IPolicy = (*ASPAPolicy)(nil)

// ASPAPolicy contains everything methods, need. In this case, it contains, U-SPAS table
// neighbors of the current router, and the current AS number
type ASPAPolicy struct {  
  USPAS     protocols.USPASTable
  Neighbors []Neighbor
  ASNumber  int
}

// ASPA Object related operations are also  defined here, below functions are used only for
// handling the ASPAList containing bunch ASPA objects, later these objects either way 
// processed by the ASPA protocol, depending on the IETF draft
// 
// Notive the difference between other policy methods, these takes also 
// router instances as well
func (ap *ASPAPolicy) AddASPA(r Router, aspa protocols.ASPAObject) {
    r.ASPAList = append(r.ASPAList, aspa)
}

func (ap *ASPAPolicy) AcceptRoute(route *Route) bool {
    // Get the AS_PATH as a slice of AS numbers
    asPath := route.PathASNumbers()

    if len(asPath) == 0 {
        log.Warn("AS_PATH is empty")
        return false
    }

    // Compress the AS_PATH to remove duplicates
    compressedPath := protocols.CompressASPath(asPath)

    // The neighbor AS is the last AS in the AS_PATH
    neighborAS := compressedPath[len(compressedPath)-1]

    // Find the neighbor in the slice that matches the neighborAS
    // TODO here trying to find, the last AS in the compressed path in the neighbors list
    // by looping over all the neighbors in the policy struct of the current router
    // which is very inefficient (considering thousands of AS neighbors)
    //
    // TODO furthemore there is a bad design on the structs such that, Router struct contains
    // Neighbor list containing neighbor structs. Also Router struct contains Policy struct
    // which also contains Neighbor struct if policy is set to AS so there is this kind of
    // redundancy and possible conflicts since Neighbor of Router is updated however Neighbors
    // in the policy might not
    var neighbor *Neighbor
    var exists bool
    for i := range ap.Neighbors {
        if ap.Neighbors[i].Router.ASNumber == neighborAS {
            neighbor = &ap.Neighbors[i]
            exists = true
            break
        }
    }

    if !exists {
        log.Warnf("Neighbor AS%d not found in neighbor relations for AS%d", neighborAS, ap.ASNumber)
        // Decide how to handle unknown neighbor relations (e.g., treat as Invalid)
        return false
    }
      
    var neighborRelation Relation = neighbor.Relation
    var outcome protocols.Outcome

    // Apply the appropriate verification algorithm based on neighbor relation
    switch neighborRelation {
    case Customer, Peer:
        // Use the upstream verification algorithm
        // asPath []int, neighborAS int, isRSClient bool, uspaspTable USPASTable
        log.Infof("AS%d verifying route to AS%d: ASPA validation", ap.ASNumber, route.Dest.ASNumber)
        outcome = protocols.VerifyUpstreamPath(compressedPath, neighborAS, true, ap.USPAS)
    case Provider:
        // Use the downstream verification algorithm
        log.Infof("AS%d verifying route to AS%d: ASPA validation", ap.ASNumber, route.Dest.ASNumber)
        outcome = protocols.VerifyDownstreamPath(compressedPath, neighborAS, ap.USPAS)
    default:
        // If relation is unknown or complex, treat as Invalid
        log.Warnf("Unknown or unsupported neighbor relation for AS%d", neighborAS)
        outcome = protocols.Invalid
    }

    if outcome == protocols.Valid {
        log.Infof("AS%d accepted route to AS%d: ASPA validation %s", ap.ASNumber, route.Dest.ASNumber, outcome)
        return true
    } else {
        log.Infof("AS%d rejected route to AS%d: ASPA validation %s", ap.ASNumber, route.Dest.ASNumber, outcome)
        return false
    }
}




// UpdateASPATable updates the ASPA table based on the router's ASPAList
func (ap *ASPAPolicy) UpdateASPATable(aspaList []protocols.ASPAObject) {
    for _, aspa := range aspaList {
        ap.USPAS[int(aspa.CustomerAS)] = aspa.ASPASet
    }
    log.Infof("ASPA table updated: %+v", ap.USPAS)
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
