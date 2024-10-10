package engine

import (
	"fmt"
	"log"
	"net"
	"rstk/internal/engine/manager"
	"rstk/internal/graph"
	"rstk/internal/parser"

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

	// To follow up on the assigned network links between the routers RouterLink struct
	// is created, it is basically useful for bi-directional checks
	type RouterLink struct {
		r1 int
		r2 int
	}

	// Using the net package to generate IP addresses
  ipNet := &net.IPNet{
		IP:   net.IPv4(192, 168, 0, 0),
		Mask: net.CIDRMask(16, 32),
	}

	// We need to keep track of the assigned IP addresses to avoid conflicts as we loop
	// over each node and their neighbors
	assignedIPs := make(map[RouterLink][2]string)

	// As we loop we also need to keep track of the current subnet address
	currentSubnet := ipNet.IP.Mask(ipNet.Mask)
	copy(currentSubnet, ipNet.IP)

	// Function to increment subnet (move to next /30)
	incrementIP := func(ip net.IP) {
		for j := len(ip) - 1; j >= 0; j-- {
			ip[j] += 4 // Move by 4 IPs (next /30 subnet)
			if ip[j] > 0 {
				break
			}
		}
	}

	// By iterating over each node in the topology and checking via the RouterLink struct
	// if the IP address has been assigned or not we can assign the IP addresses to the
	// interfaces of the routers
	for _, node := range topology {

		// Rather then looping over each neighbor in a seperate loop, all of them
		// are appended to a single slice
		neighbors := append(node.Customer, node.Peer...)
		neighbors = append(neighbors, node.Provider...)

		for _, neighbor := range neighbors {
			link := RouterLink{r1: node.ASNumber, r2: neighbor}
			reverseLink := RouterLink{r1: neighbor, r2: node.ASNumber}

			_, exists := assignedIPs[link]
			_, reverseExists := assignedIPs[reverseLink]

			if !exists && !reverseExists {

				router1IP := net.IP(make([]byte, len(currentSubnet)))
				router2IP := net.IP(make([]byte, len(currentSubnet)))

				copy(router1IP, currentSubnet)
				copy(router2IP, currentSubnet)

				// Increment the last octet by 1 and 2 to assign IPs to the routers
				// last octet is indicated by index 3
				router1IP[3] += 1
				router2IP[3] += 2

				// Storing IP assignments on both directions
				assignedIPs[link] = [2]string{router1IP.String(), router2IP.String()}
				assignedIPs[reverseLink] = [2]string{router2IP.String(), router1IP.String()}

				l := getInterfaceFromLink(neighbor, node.ASNumber, topology[neighbor])

				// Assing IP addresses to Interface-IP map
				node.IPPerInterface[l] = router1IP.String()

				if val, ok := topology[neighbor]; ok {
					l := getInterfaceFromLink(node.ASNumber, neighbor, val)
					topology[neighbor].IPPerInterface[l] = router2IP.String()
				} else {
					log.Printf("AS %d not found in topology", neighbor)
				}

				// Finally increment the subnet
				incrementIP(currentSubnet)
			}
		}

		log.Printf("Node state after assigning IPs for AS %v", node)
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
