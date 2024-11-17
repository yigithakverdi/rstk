package cli

import (
  // Core library imports
  "fmt"
  "strconv"
  "strings"

  // Internal library imports
  "rstk/internal/router"
  "rstk/internal/graph"
  "rstk/internal/parser"
  "rstk/internal/protocols"

  // Github library imports
  // "github.com/peterh/liner"
)

// Command that initializes the topology with the given AS relationships
func (s *State) CInit(args []string) {
  t := graph.Topology{}
  
  // Since Init() function requires AS relationships, we need to parse the
  // related data source, so every time this command is called, I/O operation
  // will be done, need to inform user on this
  fmt.Println("[+] Parsing AS relationships...")

  // Furthermore the path is temporarly hardcoded this also needs to be
  // left to the user, or a default path should be provided, informing the user
  // on this as  well
  fmt.Println("[?] Path is hardcoded, please change the path in the code")
  path := "/home/yigit/workspace/github/fix/data/test-2/test.as-rel2.txt"
  
  asRelsList, err := parser.GetAsRelationships(path)
  if err != nil {
    fmt.Println("[!] Error parsing AS relationships:", err)
    return
  }
  
  fmt.Println("[+] Initializing topology...")
  t.Init(asRelsList)

  // Update the state of the CLI as well to reflect changes on the other 
  // commands as well
  fmt.Println("[?] State updated")
  s.Topology = &t

}

// Command that prints help messages
func (s* State) CShowHelp(commands map[string]Command) {
    fmt.Println("Available commands:")
    for name, cmd := range commands {
        fmt.Printf("  %s - %s\n", name, cmd.Description)
    }
}

// Command that prints the current topology
func (s* State) CPrintTpopology() {
  t := s.Topology

  fmt.Println("[+] Current topology:")
  t.PrintTopology()
}

// Command that obtains router given the as, and shows the information
// about it
func (s* State) CGetRouter(args []string) {
  t := s.Topology

  // Argument check
  if len(args) != 1 {
    fmt.Println("[!] Usage: show-router <AS>")
    return
  }
  
  // TODO note that this operation is pretty much inefficient since
  // I am casting string to integer, and then hashing that integer
  // since hashing function requires integer --> string conversion
  // globally thoroughout this project
  //
  // However only thing is being done here is adding "r" to the begining
  // of the integer, so instead it could be done as concatenating "r" to
  // argument string (however if the hashing is more complex this might stay
  // as it is)

  // Casting the string to integer then hashing it then, accessing it via
  // GetRouter method since the method requires hashed version of the idenfitifer
  // of the router
  as := args[0]

  // Casting it to integer
  asNumber, _ := strconv.Atoi(as)

  // Hashing the as number (int) to hashed string version of it
  asHash := fmt.Sprintf("r%d", asNumber)
  
  // Getting the router from the topology
  r := t.GetRouter(asHash)

  fmt.Println("[+] Current router:")
  fmt.Println(r.ToString())
}


// Command for obtaining relations of the given two router (that are neighbors)
func (s *State) CGetRelation(args []string) {
  // Argument check
  if len(args) != 2 {
    fmt.Println("[!] Usage: show-relation <AS1> <AS2>")
    return
  }
  
  // Getting the topology from the state
  t := s.Topology

  // Casting the string to integer then hashing it then, accessing it via
  // GetRouter method since the method requires hashed version of the idenfitifer
  // of the router
  as1 := args[0]
  as2 := args[1]

  // Casting it to integer
  asNumber1, _ := strconv.Atoi(as1)
  asNumber2, _ := strconv.Atoi(as2)

  // Hashing the as number (int) to hashed string version of it
  asHash1 := fmt.Sprintf("r%d", asNumber1)
  asHash2 := fmt.Sprintf("r%d", asNumber2)

  // Getting the router from the topology
  router1 := t.GetRouter(asHash1)
  router2 := t.GetRouter(asHash2)

  // Getting the relation between the routers
  relation := router1.GetRelation(*router2)
  if relation != 2 {
    fmt.Println("[+] Relation between the routers:")
    fmt.Printf("    AS%d -> AS%d: %d\n", router1.ASNumber, router2.ASNumber, relation)
  } else {
    fmt.Println("[!] Relation not found, routers might not be neighbors")
  }
}

// Command for finding route to a given router, it is one of the main routing functions in the simulator
// and runs several sub methods such as learn route (contains policy implementations)
func (s *State) CFindRoutesTo(args []string) { 
  // First checking the arguments
  if len(args) != 1 {
    fmt.Println("[!] Usage: find-routes-to <AS>")
    return
  }

  // Getting the topology from the state
  t := s.Topology

  // Casting the string to integer then hashing it then, accessing it via
  // GetRouter method since the method requires hashed version of the idenfitifer
  // of the router
  as := args[0]

  // Casting it to integer
  asNumber, _ := strconv.Atoi(as)
  
  // Hashing the as number (int) to hashed string version of it
  asHash := fmt.Sprintf("r%d", asNumber)

  // Getting the router from the topology
  router := t.GetRouter(asHash)

  // Getting the routes to the given router, using the FindRoutesTo function from
  // the topology `func (t *Topology) FindRoutesTo(target *router.Router)` 
  t.FindRoutesTo(router)
}

// Command for adding a new router to the topology
func (s *State) CAddRouter(args []string) {
  // First checking the arguments, there is an additional option for this
  // according to that option variables are assigned
  if len(args) != 5  || args[1] != "--neighbor" || args[3] != "--relation" {
    fmt.Println("[!] Usage: add-router <AS> --neighbor ASNumber --relation <Customer(1)|Peer(0)|Provider(-1)>")
    fmt.Println("[!] Example: add-router 1 --neighbor 2 --relation -1")
    return
  }

  // String to integer for AS number provided
  as := args[0]
  asNumber, _ := strconv.Atoi(as)

  // Neighbor as number
  neighbor := args[2]
  neighborNumber, _ := strconv.Atoi(neighbor)

  // Relation between the router and the neighbor
  relation := args[4]
  relationNumber, _ := strconv.Atoi(relation)

  // Getting the topology from the state
  t := s.Topology

  // Checking the arguments
  // TODO for now same AS added every time this command is called, this should be
  // called with either parameters provided as arguments, configurations or other
  // methods (need to create router instance and neighbor instances here)
  r := router.NewRouter(asNumber)
  
  // Adding neighbor the routers neighbor list with its provided relation
  
  r.Neighbors = append(r.Neighbors, router.Neighbor{
    Router: t.GetRouter(fmt.Sprintf("r%d", neighborNumber)), 
    Relation: router.Relation(relationNumber),
  })

  fmt.Printf("[+] Printing router object as string:\n%s\n", r.ToString())

  // Adding the router to the topology
  fmt.Println("[+] Adding router to the topology...")
  t.AddRouter(r)
}

// Command for adding route hijacking to the topology
func (s *State) CHijack(args []string) {
  // Validating the arguments
  if len(args) != 3 {
    fmt.Println("[!] Usage: hijack-route <victim> <attacker> <Number of Hops>")
    return
  }

  // Getting the topology from the state
  t := s.Topology

  // Casting the string to integer then hashing it then, accessing it via
  // GetRouter method since the method requires hashed version of the idenfitifer
  // of the router
  as1 := args[0] // Victim AS
  as2 := args[1] // Attacker AS
  hops := args[2]

  // Casting it to integer
  asNumber1, _ := strconv.Atoi(as1)
  asNumber2, _ := strconv.Atoi(as2)
  numHops, _ := strconv.Atoi(hops)

  // Hashing the as number (int) to hashed string version of it
  asHash1 := fmt.Sprintf("r%d", asNumber1)
  asHash2 := fmt.Sprintf("r%d", asNumber2)

  // Getting the router from the topology
  router1 := t.GetRouter(asHash1)
  router2 := t.GetRouter(asHash2)

  // Hijacking the route
  fmt.Println("[+] Hijacking route...")
  t.Hijack(router1, router2, numHops)
}

// Command to add an ASPA to a router
func (s *State) CAddASPA(args []string) {
    if len(args) != 3 {
        fmt.Println("[!] Usage: add-aspa <RouterAS> <CustomerAS> <ProviderAS1,ProviderAS2,...>")
        return
    }

    routerAS, err := strconv.Atoi(args[0])
    if err != nil {
        fmt.Println("[!] Invalid RouterAS number")
        return
    }

    customerAS, err := strconv.Atoi(args[1])
    if err != nil {
        fmt.Println("[!] Invalid CustomerAS number")
        return
    }

    providerASStr := args[2]
    providerASList := strings.Split(providerASStr, ",")
    var providerAS []int
    for _, as := range providerASList {
        asNum, err := strconv.Atoi(strings.TrimSpace(as))
        if err != nil {
            fmt.Printf("[!] Invalid ProviderAS number: %s\n", as)
            return
        }
        providerAS = append(providerAS, int(asNum))
    }

    // Create the ASPAObject
    aspa := protocols.ASPAObject{
        CustomerAS:  int(customerAS),
        ASPASet:     providerAS,
        Signature:   []byte{}, // Assuming signature handling is out of scope
        UnionSPAS:   providerAS, // Simplified for this example
    }

    // Retrieve the router
    router := s.Topology.GetRouter(fmt.Sprintf("r%d", routerAS))
    if router == nil {
        fmt.Printf("[!] Router AS%d not found in topology\n", routerAS)
        return
    }

    // Add ASPA to the router
    router.AddASPA(aspa)

    fmt.Printf("[+] ASPA added to Router AS%d: Customer AS%d, Providers %v\n", routerAS, customerAS, providerAS)
}

// Command to remove an ASPA from a router
func (s *State) CRemoveASPA(args []string) {
    if len(args) != 2 {
        fmt.Println("[!] Usage: remove-aspa <RouterAS> <CustomerAS>")
        return
    }

    routerAS, err := strconv.Atoi(args[0])
    if err != nil {
        fmt.Println("[!] Invalid RouterAS number")
        return
    }

    customerAS, err := strconv.Atoi(args[1])
    if err != nil {
        fmt.Println("[!] Invalid CustomerAS number")
        return
    }

    // Retrieve the router
    router := s.Topology.GetRouter(fmt.Sprintf("r%d", routerAS))
    if router == nil {
        fmt.Printf("[!] Router AS%d not found in topology\n", routerAS)
        return
    }

    // Remove ASPA from the router
    router.RemoveASPA(customerAS)

    fmt.Printf("[+] ASPA for Customer AS%d removed from Router AS%d\n", customerAS, routerAS)
}

// Command to show ASPA configurations of a router
func (s *State) CShowASPA(args []string) {
    if len(args) != 1 {
        fmt.Println("[!] Usage: show-aspa <RouterAS>")
        return
    }

    routerAS, err := strconv.Atoi(args[0])
    if err != nil {
        fmt.Println("[!] Invalid RouterAS number")
        return
    }

    // Retrieve the router
    router := s.Topology.GetRouter(fmt.Sprintf("r%d", routerAS))
    if router == nil {
        fmt.Printf("[!] Router AS%d not found in topology\n", routerAS)
        return
    }

    // Retrieve ASPA list
    aspaList := router.GetASPA()
    if len(aspaList) == 0 {
        fmt.Printf("[+] Router AS%d has no ASPA configurations\n", routerAS)
        return
    }

    fmt.Printf("[+] ASPA configurations for Router AS%d:\n", routerAS)
    for _, aspa := range aspaList {
        fmt.Printf("  Customer AS: %d\n", aspa.CustomerAS)
        fmt.Printf("  Providers: %v\n", aspa.ASPASet)
        fmt.Printf("  Signature: %x\n", aspa.Signature)
        fmt.Println()
    }
}
