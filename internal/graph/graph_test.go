package graph 

import (
  "fmt"
  "testing"
  log "github.com/sirupsen/logrus"

  "rstk/internal/parser"
  "rstk/internal/router"
)

// Helper function to initialize the topology from AS relationships file
func initializeTopology(t *testing.T, topologyType string) *Topology {
	// Parse AS relationships
  asRelFilePath := "/home/yigit/workspace/github/rstk-worktree/rstk-fix/data/test-2/test.as-rel2.txt"
	asRelsList, err := parser.GetAsRelationships(asRelFilePath)
	if err != nil {
		t.Fatalf("Failed to parse AS relationships: %v", err)
	}

	// Initialize Topology
	topology := Topology{}
	topology.TopologyType = topologyType
	g, ases := topology.Init(asRelsList)

	// Deploy ASPA objects randomly
	topology.CreateASPAObjectsRandomly(100) // 100% deployment for most tests

	// Assign the initialized graph and ASes to the topology
	topology.G = g
	topology.ASES = ases

  log.Infof("RPKI: %v", topology.RPKI)
	return &topology
}

// Helper function to create a route
func createRoute(t *testing.T, topology Topology, pathASIDs []int) *router.Route {
	// for i, asID := range pathASIDs {
	// 	as, exists := ases[asID]
	// 	if !exists {
	// 		t.Fatalf("AS%d not found in topology", asID)
	// 	}
	// 	path[i] = as
	// }

	path := make([]*router.Router, len(pathASIDs))
	for i, asID := range pathASIDs {
		asHash := fmt.Sprintf("r%d", asID)
		as := topology.GetRouter(asHash)
		if as == nil {
			t.Fatalf("Router with AS%d (hash: %s) not found in topology", asID, asHash)
		}
		path[i] = as
	}  
  

	route := &router.Route{
		Dest:            path[0], // victim AS
		Path:            path,
		OriginInvalid:   false,
	PathEndInvalid:  false,
		Authenticated:   false,
	}

	return route
}

// Constants representing ASPA validation outcomes
const (
	ASPAValid   = "valid"
	ASPAInvalid = "invalid"
	ASPAUnknown = "unknown"
)


// TestASPACase01: Upstream Path Verification - Only Customers, only a single hop
func TestASPACase01(t *testing.T) {
	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 9
 
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid 
	if result != expected {
		t.Errorf("TestASPACase01 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase02: Upstream Path Verification - Only Customers
func TestASPACase02(t *testing.T) {
	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 1
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid 
	if result != expected {
		t.Errorf("TestASPACase02 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase03: Upstream Path Verification - RouteLeak by AS6
// Slide15 Trajectory 1
func TestASPACase03(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 3
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 6, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid 
	if result != expected {
		t.Errorf("TestASPACase03 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase04: Upstream Path Verification - RouteLeak by AS6, but AS2 has no ASPA
func TestASPACase04(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 1
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 6, 3, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set AS2's ASPA to nil
	as2 := topology.GetRouter("r2")
	if as2 == nil {
		t.Fatalf("AS2 router not found in topology")
	}
	as2.ASPAList = nil  
  topology.RPKI.ResetUSPASRecord(as2.ASNumber)


  
	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "unknown"
	expected := router.ASPAUnknown 
	if result != expected {
		t.Errorf("TestASPACase04 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase05: Upstream Path Verification - Only Customers, but AS9 has no ASPA
func TestASPACase05(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 1
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set AS9's ASPA to nil
	as9 := topology.GetRouter("r9")
  log.Infof("AS9 ASPA set to nil")
	if as9 == nil {
		t.Fatalf("AS9 router not found in topology")
	}
	as9.ASPAList = nil   
  topology.RPKI.ResetUSPASRecord(as9.ASNumber)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "unknown"
	expected := router.ASPAUnknown
	if result != expected {
		t.Errorf("TestASPACase05 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase06: Upstream Path Verification - Two customers, then lateral peer
// Slide15 Trajectory 2
func TestASPACase06(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 6
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase06 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase07: Upstream Path Verification - Two customers, then lateral peer, but AS9 has no ASPA
func TestASPACase07(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 6
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set AS9's ASPA to nil
	as9 := topology.GetRouter("r9")
	if as9 == nil {
		t.Fatalf("AS2 router not found in topology")
	}
	as9.ASPAList = nil  
  topology.RPKI.ResetUSPASRecord(as9.ASNumber)  

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase07 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase08: Upstream Path Verification - Customer to Provider, to Peer, to Peer
// Slide14 Trajectory 3
func TestASPACase08(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 9
	verifyingASID := 7
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 5, 6, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase08 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase09: Upstream Path Verification - Customer to Provider, to Customer, to Peer
// Slide14 Trajectory 2
func TestASPACase09(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 5
	verifyingASID := 7
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 2, 6, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase09 failed: expected %s, got %s", expected, result)
	}
}

func TestASPACase10(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Upstream Path Verification - Customer to Provider, to Peer, to Provider
    // Slide14 Trajectory 4

    // Select victim and verifying AS
    victimASID := 9
    verifyingASID := 3

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 5, 6, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Invalid"
    expected := router.ASPAInvalid
    if result != expected {
        t.Errorf("TestASPACase10 failed: expected %s, got %s", expected, result)
    }
}

// TestASPACase11: Downstream Path Verification - Upstream sending route directly
func TestASPACase11(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 9
	verifyingASID := 17
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase11 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase12: Downstream Path Verification - Customer of Upstream sending route
func TestASPACase12(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 9
	verifyingASID := 10
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 5, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase12 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase13: Downstream Path Verification - Peer of Upstream sending route
func TestASPACase13(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 6
	verifyingASID := 10
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 5, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase13 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase14: Downstream Path Verification - Upstream of Upstream sending route
// Slide15 Trajectory 5
func TestASPACase14(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 2
	verifyingASID := 10
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 5,  verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase14 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase15: Downstream Path Verification - Peer of Upstream of Upstream sending route
// Slide15 Trajectory 6
func TestASPACase15(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 6
	verifyingASID := 17
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 5, 9, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase15 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase16: Downstream Path Verification - Inverted V Shape
// Slide15 Trajectory 3
func TestASPACase16(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 12
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 6, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase16 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase17: Downstream Path Verification - Inverted V Shape - Opposite direction
func TestASPACase17(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 12
	verifyingASID := 17
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 6, 2, 5, 9, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase17 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase18: Downstream Path Verification - Inverted V Shape with p2p at apex
// Slide15 Trajectory 4
func TestASPACase18(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 14
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 3, 7, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "valid"
	expected := router.ASPAValid
	if result != expected {
		t.Errorf("TestASPACase18 failed: expected %s, got %s", expected, result)
	}
}

func TestASPACase19(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape with p2p at apex - Opposite direction

    // Select victim and verifying AS
    victimASID := 14
    verifyingASID := 17

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 7, 3, 2, 5, 9, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Valid"
    expected := router.ASPAValid
    if result != expected {
        t.Errorf("TestASPACase19 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase20(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape, but AS5 has no ASPA

    // Select victim and verifying AS
    victimASID := 17
    verifyingASID := 12

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 9, 5, 2, 6, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5
    as5 := topology.GetRouter("r5")
    if as5 == nil {
        t.Fatalf("AS5 router not found in topology")
    }
    as5.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase20 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase21(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape - Opposite direction, but AS5 has no ASPA

    // Select victim and verifying AS
    victimASID := 12
    verifyingASID := 17

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 6, 2, 5, 9, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5
    as5 := topology.GetRouter("r5")
    if as5 == nil {
        t.Fatalf("AS5 router not found in topology")
    }
    as5.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
  }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase21 failed: expected %s, got %s", expected, result)
    }
}



// TestASPACase22: Downstream Path Verification - Inverted V Shape with p2p at apex, but AS5 has no ASPA
func TestASPACase22(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 14
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 3, 7, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set AS5's ASPA to nil
	as5 := topology.GetRouter("r5")
	if as5 == nil {
		t.Fatalf("AS2 router not found in topology")
	}
	as5.ASPAList = nil  
  topology.RPKI.ResetUSPASRecord(as5.ASNumber)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "unknown"
	expected := router.ASPAUnknown
	if result != expected {
		t.Errorf("TestASPACase22 failed: expected %s, got %s", expected, result)
	}
}

func TestASPACase23(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape with p2p at apex - Opposite direction, but AS5 has no ASPA

    // Select victim and verifying AS
    victimASID := 14
    verifyingASID := 17

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 7, 3, 2, 5, 9, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5
    as5 := topology.GetRouter("r5")
    if as5 == nil {
        t.Fatalf("AS5 router not found in topology")
    }
    as5.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase23 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase24(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape, but AS5 and AS6 have no ASPA

    // Select victim and verifying AS
    victimASID := 17
    verifyingASID := 12

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 9, 5, 2, 6, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5 and AS6
    as5 := topology.GetRouter("r5")
    as6 := topology.GetRouter("r6")
    if as5 == nil || as6 == nil {
        t.Fatalf("AS5 or AS6 router not found in topology")
    }
    as5.ASPAList = nil
    as6.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)
    topology.RPKI.ResetUSPASRecord(as6.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase24 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase25(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape - Opposite direction, but AS5 and AS6 have no ASPA

    // Select victim and verifying AS
    victimASID := 12
    verifyingASID := 17

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 6, 2, 5, 9, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5 and AS6
    as5 := topology.GetRouter("r5")
    as6 := topology.GetRouter("r6")
    if as5 == nil || as6 == nil {
        t.Fatalf("AS5 or AS6 router not found in topology")
    }
    as5.ASPAList = nil
    as6.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)
    topology.RPKI.ResetUSPASRecord(as6.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase25 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase26(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape with p2p at apex, but AS5 and AS6 have no ASPA

    // Select victim and verifying AS
    victimASID := 17
    verifyingASID := 14

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 9, 5, 2, 3, 7, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5 and AS6
    as5 := topology.GetRouter("r5")
    as6 := topology.GetRouter("r6")
    if as5 == nil || as6 == nil {
        t.Fatalf("AS5 or AS6 router not found in topology")
    }
    as5.ASPAList = nil
    as6.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)
    topology.RPKI.ResetUSPASRecord(as6.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase26 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase27(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Inverted V Shape with p2p at apex - Opposite direction, but AS5 and AS6 have no ASPA

    // Select victim and verifying AS
    victimASID := 14
    verifyingASID := 17

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 7, 3, 2, 5, 9, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Remove ASPA from AS5 and AS6
    as5 := topology.GetRouter("r5")
    as6 := topology.GetRouter("r6")
    if as5 == nil || as6 == nil {
        t.Fatalf("AS5 or AS6 router not found in topology")
    }
    as5.ASPAList = nil
    as6.ASPAList = nil
    topology.RPKI.ResetUSPASRecord(as5.ASNumber)
    topology.RPKI.ResetUSPASRecord(as6.ASNumber)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Unknown"
    expected := router.ASPAUnknown
    if result != expected {
        t.Errorf("TestASPACase27 failed: expected %s, got %s", expected, result)
    }
}

func TestASPACase30(t *testing.T) {
    topology := initializeTopology(t, "ASPA")

    // Downstream Path Verification - Route Leak by AS6 to Upstream

    // Select victim and verifying AS
    victimASID := 17
    verifyingASID := 14

    verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

    // Define the path
    pathASIDs := []int{victimASID, 9, 5, 2, 6, 3, 7, verifyingASID}

    // Create the route
    route := createRoute(t, *topology, pathASIDs)

    // Deploy ASPA objects randomly with 100%
    topology.CreateASPAObjectsRandomly(100)

    // Set verifying AS's policy to ASPAPolicy
    verifyingAS.Policy = &router.ASPAPolicy{
        Router: verifyingAS,
    }

    // Perform ASPA validation
    result, _ := verifyingAS.PerformASPA(route)

    // Assert the result is "Invalid"
    expected := router.ASPAInvalid
    if result != expected {
        t.Errorf("TestASPACase30 failed: expected %s, got %s", expected, result)
    }
}


// TestASPACase31: Downstream Path Verification - Route Leak by AS6 to Lateral peer
func TestASPACase31(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 14
	
	verifyingAS := topology.ASES[14]

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 6, 7, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase31 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase32: Downstream Path Verification - Route Leak by AS6 and again by AS7
func TestASPACase32(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 17
	verifyingASID := 18
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 9, 5, 2, 6, 3, 7, 4, 8, 16, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase32 failed: expected %s, got %s", expected, result)
	}
}

// TestASPACase33: Downstream Path Verification - Route Leak by AS6 and again by AS7, opposite direction
func TestASPACase33(t *testing.T) {
	 

	topology := initializeTopology(t, "ASPA")

	// Select victim and verifying AS
	victimASID := 18
	verifyingASID := 17
	
	verifyingAS := topology.GetRouter(fmt.Sprintf("r%d", verifyingASID))

	// Define the path
	pathASIDs := []int{victimASID, 16, 8, 4, 7, 3, 6, 2, 5, 9, verifyingASID}

	// Create the route
	route := createRoute(t, *topology, pathASIDs)

	// Deploy ASPA objects randomly with 100%
	topology.CreateASPAObjectsRandomly(100)

	// Set verifying AS's policy to ASPAPolicy
	verifyingAS.Policy = &router.ASPAPolicy{
		Router: verifyingAS,
	}

	// Perform ASPA validation
	result, _ := verifyingAS.PerformASPA(route)

	// Assert the result is "invalid"
	expected := router.ASPAInvalid
	if result != expected {
		t.Errorf("TestASPACase33 failed: expected %s, got %s", expected, result)
	}
}
