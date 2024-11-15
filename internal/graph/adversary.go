package graph

// Adversary contains, all the malicious actions, prefix hijacks, route leak configurations
// and other custom actions, mainly AS path manipulations

import (
  // Core library imports
  "fmt"
  "time"
  "math/rand"

  // Internal library imports
  "rstk/internal/router"

  // Github library imports
	"github.com/edwingeng/deque"
  log "github.com/sirupsen/logrus"

)


// Method for hijacking given n hops, such as n = 0 means, path is only the attacker itself,
// n = 1 means path is attacker and victim AS, and more then n means there are random or 
// carefuly crafted ASes in the middle
func (t *Topology) Hijack(victim *router.Router, attacker *router.Router, n int) {
  log.Infof("Starting route hijacking from attacker AS%d to vicmtim AS%d", victim.ASNumber, 
    attacker.ASNumber)
  if n < 0 {
    fmt.Println("number of hops must be non-negative")
    return
  }

  var path []*router.Router

  if n == 0 {
    // Attacker claims to be the originpath
    log.Debugf("Attacker claims to be the origin")
    path = []*router.Router{attacker}
  } else if n == 1 {
    // Attacker claims a direct path to victim
    log.Debugf("Attacker claims a direct path to victim")
    path = []*router.Router{victim, attacker}
  } else {
    // Attacker includes random ASes in the path
    // Exclude victim and attacker from the list of ASes
    log.Debugf("Attacker includes random ASes in the path")
    asysSet := make(map[int]*router.Router)
    for asn, asys := range t.ASES {
        if asn != victim.ASNumber && asn != attacker.ASNumber {
            asysSet[asn] = asys
        }
    }

    // Convert map to slice for sampling
    asysList := make([]*router.Router, 0, len(asysSet))
    for _, asys := range asysSet {
        asysList = append(asysList, asys)
    }

    // Randomly select n-1 ASes
    if n-1 > len(asysList) {
        fmt.Println("Not enough ASes to build the path")
        return
    }
    middle := randomSample(asysList, n-1)

    // Build the path: [victim] + middle ASes + [attacker]
    path = []*router.Router{victim}
    path = append(path, middle...)
    path = append(path, attacker)
  }

  badRoute := &router.Route{
    Dest:           victim,
    Path:           path,
    OriginInvalid:  n == 0,
    PathEndInvalid: n <= 1,
    Authenticated:  false,
  }

  log.Debugf("Constructed random bad route %v", badRoute.PathASNumbers())

  routes := deque.NewDeque()
  for _, neighbor := range attacker.Neighbors {
    forwardedRoute := attacker.ForwardRoute(badRoute, neighbor.Router)
    routes.PushBack(forwardedRoute)
    log.Debugf("Adding neighbor AS%d to the queue", neighbor.Router.ASNumber)
  }
  
  for routes.Len() > 0 {
    route := routes.PopFront().(*router.Route)
    final := route.Path[len(route.Path)-1]
    log.Debugf("Processing route to AS%d via AS%d", route.Dest.ASNumber, final.ASNumber)

    // Forward the route to neighbors according to policy
    for _, neighbor := range final.LearnRoute(route) {
     routes.PushBack(final.ForwardRoute(route, neighbor))
     log.Debugf("Forwarded route from AS%d to neighbor AS%d", final.ASNumber, neighbor.ASNumber)
    }
  }
  log.Infof("Completed hijacking")
}


func randomSample(asysList []*router.Router, k int) []*router.Router {
  rand.Seed(time.Now().UnixNano())
  shuffled := make([]*router.Router, len(asysList))
  copy(shuffled, asysList)
  rand.Shuffle(len(shuffled), func(i, j int) {
    shuffled[i], shuffled[j] = shuffled[j], shuffled[i]
  })
  return shuffled[:k]
}
