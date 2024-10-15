package engine

import (
	"encoding/binary"
	"fmt"
	"log"
	"net"
	"os"
	"path/filepath"
	"rstk/internal/engine/manager"
	"rstk/internal/graph"
	"rstk/internal/parser"
	"text/template"

	"rstk/pkg/models"

	"github.com/google/uuid"
)

type EdgeConfig struct {
	Source          string
	Destination     string
	Bandwidth       int
	Latency         int
	CollusionDomain string
}

type NodeConfig struct {
	CPU       int
	Memory    int
	ID        string
	IP        string
	Protocol  string
	Interface string
}

type GlobalConfig struct {
	Nodes []NodeConfig
	Edges []EdgeConfig
}

type TopologyConfig struct {
	Depth           int
	BranchingFactor int
	Redundancy      bool
	IPVersion       int
	IPBase          string
	IPMap           map[int]string
}

type SimulationConfig struct {
	Topology          TopologyConfig
	Global            GlobalConfig
	SimulationID      string
	KatharaConfigPath string
}

// Function for generating a unique simulation ID using UUID.
func GenerateSimulationID() string {
	return uuid.New().String()
}

// Initializes and parses the AS relationship file, returning the populated graph.
func InitializeGraph(asRelFilePath string, blacklistTokens []string) graph.Graph {
	parser := parser.Parser{
		AsRelFilePath:   asRelFilePath,
		BlacklistTokens: blacklistTokens,
	}
	parser.ParseFile()
  return graph.PopulateGraph(parser.AsRelationships)
}

// Initializes and returns the simulation configuration.
func InitializeSimulationConfig() SimulationConfig {
	return SimulationConfig{
		Topology: TopologyConfig{
			Depth:           2,
			BranchingFactor: 1,
			Redundancy:      false,
		},
		Global:       GlobalConfig{},
		SimulationID: GenerateSimulationID(),
	}
}

// Initializes and creates the Kathara configuration file and directory structure.
func SetupSimulationDirectory(simulationConfig *SimulationConfig) error {
	manager.CreateSimulationDirectory(simulationConfig.SimulationID)
	configFilePath, err := manager.CreateKatharaConfigFile(simulationConfig.SimulationID)
	if err != nil {
		return fmt.Errorf("failed to create configuration file %s: %w", configFilePath, err)
	}
	simulationConfig.KatharaConfigPath = configFilePath
	return nil
}

// Function for generating the topology for the simulation. It takes the AS number, graph, and topology
// configuration as input.
func GenerateTopology(asNumber int, g graph.Graph, config TopologyConfig) map[int]graph.Node {
	visited := make(map[int]bool)
	topology := make(map[int]graph.Node)
	traverseASMap(asNumber, config.Depth, config.BranchingFactor, visited, g, topology)
	return topology
}

// Function for generating collision domains for the topology. It takes the simulation ID and the
// topology as input. It iterates over each node in the topology and creates collision domains
// for each of its neighbors.
func GenerateCollisionDomains(katharaConfigPath string, topology map[int]graph.Node) error {
	for _, node := range topology {
		collisionDomains := manager.CreateCollisionDomains(node)

		log.Printf("Collision domains for AS %d", node.ASNumber)
		for domain := range collisionDomains {
			log.Printf("%s", domain)
		}
		err := manager.WriteCollisionDomainsToKatharaConfigFile(katharaConfigPath, collisionDomains)
		if err != nil {
			log.Fatalf("Failed to write collision domains to file %s: %v", katharaConfigPath, err)
		}
	}
	return nil
}

func GenerateRouterIPs(topology map[int]graph.Node) {
	type RouterLink struct {
		r1 int
		r2 int
	}

	// Using the net package to generate IP addresses
  ipNet := &net.IPNet{
		IP:   net.IPv4(192, 168, 0, 0),
		Mask: net.CIDRMask(16, 32),
	}

	assignedIPs := make(map[RouterLink][2]string)
	currentSubnet := ipNet.IP.Mask(ipNet.Mask).To4()

	incrementIP := func(ip net.IP) {
		ipInt := binary.BigEndian.Uint32(ip)
		ipInt += 4
		binary.BigEndian.PutUint32(ip, ipInt)
	}

	for _, node := range topology {
		neighbors := append(node.Customer, node.Peer...)
		neighbors = append(neighbors, node.Provider...)

		for _, neighbor := range neighbors {
			link := RouterLink{r1: node.ASNumber, r2: neighbor}
			reverseLink := RouterLink{r1: neighbor, r2: node.ASNumber}

			if _, exists := assignedIPs[link]; !exists {
				router1IP := make(net.IP, 4)
				router2IP := make(net.IP, 4)
				copy(router1IP, currentSubnet)
				copy(router2IP, currentSubnet)

				router1IP[3] += 1
				router2IP[3] += 2

				assignedIPs[link] = [2]string{router1IP.String(), router2IP.String()}
				assignedIPs[reverseLink] = [2]string{router2IP.String(), router1IP.String()}

				incrementIP(currentSubnet)
			}
		}
	}

	for link, routerLinks := range assignedIPs {
		if val, ok := topology[link.r1]; ok {
			l := getInterfaceFromLink(link.r1, link.r2, val)
			topology[link.r1].IPPerInterface[l] = routerLinks[0]
		}
	}
}

// Function for generating FRR configurations
func GenerateFRRConfigurations(topology map[int]graph.Node) {

	routerID := 1
	for _, node := range topology {

		// Create the router configuration
		router := models.Router{
			ID:        fmt.Sprintf("%d", routerID),
			ASNumber:  node.ASNumber,
			RouterID:  node.IPPerInterface[0],
			Neighbors: make([]models.Neighbor, 0),
		}

		routerID++

		// Add neighbors
		neighbors := append(node.Customer, node.Peer...)
		neighbors = append(neighbors, node.Provider...)

		for _, neighbor := range neighbors {
			if val, ok := topology[node.ASNumber]; ok {
				l := getInterfaceFromLink(node.ASNumber, neighbor, val)
				router.Neighbors = append(router.Neighbors, models.Neighbor{
					IP:        assignedIPs[node.ASNumber][1],
					AS:        neighbor,
					Interface: fmt.Sprintf("eth%d", l),
				})
			}

			if val, ok := topology[neighbor]; ok {
				l := getInterfaceFromLink(neighbor, node.ASNumber, val)
				router.Neighbors = append(router.Neighbors, models.Neighbor{
					IP:        assignedIPs[neighbor][1],
					AS:        reverseLink.r2,
					Interface: fmt.Sprintf("eth%d", l),
				})
			}
		}

		// Generate the FRR configuration
		generateFRRConfig(router)

	}

	// router := models.Router{
	// 	ID:       "1",
	// 	ASNumber: 65001,
	// 	RouterID: "192.168.1.1",
	// 	Neighbors: []models.Neighbor{
	// 		{IP: "10.0.0.1", AS: 65002},
	// 		{IP: "10.0.0.2", AS: 65003},
	// 	},
	// }

	// Load the template
	templatePath := filepath.Join("configs", "templates", "frr.conf.template")
	tmpl, err := template.ParseFiles(templatePath)
	if err != nil {
		log.Fatalf("Failed to load template: %v", err)
	}

	// Create the output file
	outputPath := filepath.Join("configs", "generated", "router"+router.ID+".conf")
	file, err := os.Create(outputPath)
	if err != nil {
		log.Fatalf("Failed to create output file: %v", err)
	}
	defer file.Close()

	// Execute the template with the router data
	err = tmpl.Execute(file, router)
	if err != nil {
		log.Fatalf("Failed to execute template: %v", err)
	}
}

func getInterfaceFromLink(as1 int, as2 int, node graph.Node) int {
	link := fmt.Sprintf("%d-%d", as1, as2)
	result := 0
	for interfaceID, linkID := range node.Interfaces {
		if linkID == link {
			result = interfaceID
			break
		}
	}
	log.Printf("Interface ID for link %s is %d", link, result)
	return result
}

// A helper function to select up to N customers, N peers, and N providers from the node.
func limitedNode(node graph.Node, branchFactor int) graph.Node {
	// Limit to `branchFactor` customers, peers, and providers
	var customer, peer, provider []int

	// Limit to the first `branchFactor` elements (or less if not enough exist)
	if len(node.Customer) > branchFactor {
		customer = node.Customer[:branchFactor]
	} else {
		customer = node.Customer
	}

	if len(node.Peer) > branchFactor {
		peer = node.Peer[:branchFactor]
	} else {
		peer = node.Peer
	}

	if len(node.Provider) > branchFactor {
		provider = node.Provider[:branchFactor]
	} else {
		provider = node.Provider
	}

	// Initialize interfaces
	interfaceID := 0
	interfaces := make(map[int]string)

	initializeInterfaces := func(asList []int) {
		for _, as := range asList {
			interfaces[interfaceID] = fmt.Sprintf("%d-%d", node.ASNumber, as)
			interfaceID++
		}
	}

	initializeInterfaces(provider)
	initializeInterfaces(customer)
	initializeInterfaces(peer)

	return graph.Node{
		ASNumber:       node.ASNumber,
		Customer:       customer,
		Peer:           peer,
		Provider:       provider,
		Prefix:         node.Prefix,
		Location:       node.Location,
		Interfaces:     interfaces,
		Contacts:       node.Contacts,
		Rank:           node.Rank,
		Type:           node.Type,
		Subnets:        node.Subnets,
		IPPerInterface: make(map[int]string),
	}
}

// Recursive function to traverse the AS map with depth and branching factor limitations
func traverseASMap(asNumber int, depthLimit int, branchFactor int, visited map[int]bool, graph graph.Graph, topology map[int]graph.Node) {
	// Stop if we've reached the depth limit
	if depthLimit == 0 {
		return
	}

	// Check if we've already visited this AS
	if visited[asNumber] {
		return
	}
	visited[asNumber] = true

	// Get the current AS node
	node, exists := graph.Nodes[asNumber]
	if !exists {
		fmt.Printf("AS %d not found\n", asNumber)
		return
	}

	// Print the limited node (with up to `branchFactor` relations)
	limited := limitedNode(node, branchFactor)

	// Add the limited node to the topology
	topology[asNumber] = limited

	// fmt.Printf("%+v\n", limited)

	// Recur for the limited customers, peers, and providers
	for _, customer := range limited.Customer {
		traverseASMap(customer, depthLimit-1, branchFactor, visited, graph, topology)
	}
	for _, peer := range limited.Peer {
		traverseASMap(peer, depthLimit-1, branchFactor, visited, graph, topology)
	}
	for _, provider := range limited.Provider {
		traverseASMap(provider, depthLimit-1, branchFactor, visited, graph, topology)
	}
}
