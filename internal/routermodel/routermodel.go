package routermodel

// Define the basic types for Route, ASys, and Relation
type Relation int

const (
	CUSTOMER Relation = iota
	PEER
	PROVIDER
)

type Packet struct {
	SourceAddr   string 
	DestinationAddr string 
	Size         int    
	Protocol     string 
}

type Route struct {
	Path          []ASys
	Final         ASys
	FirstHop      ASys
	Authenticated bool
	OriginInvalid bool
	Packet        Packet 
}

type ASys struct {
	ID      string
	Relation Relation
	ASPA    []string 
	ASCONES []string 
}

// RoutingPolicy is the interface for routing policies
type RoutingPolicy interface {
	AcceptRoute(route Route) bool
	PreferRoute(current, new Route) bool
	ForwardTo(route Route, relation Relation) bool
}

// DefaultPolicy implements common logic for routing policies
type DefaultPolicy struct{}

func (dp *DefaultPolicy) AcceptRoute(route Route) bool {
	// Basic acceptance logic
	return !containsCycle(route)
}

func (dp *DefaultPolicy) PreferRoute(current, new Route) bool {
	if current.Final.ID != new.Final.ID {
		panic("routes must have the same final AS")
	}

	for _, rule := range dp.PreferenceRules() {
		currentVal := rule(current)
		newVal := rule(new)

		if currentVal != nil && newVal != nil {
			if *currentVal < *newVal {
				return false
			}
			if *newVal < *currentVal {
				return true
			}
		}
	}
	return false
}

func (dp *DefaultPolicy) ForwardTo(route Route, relation Relation) bool {
	// Forwarding logic based on relationships
	return route.FirstHop.Relation == CUSTOMER || relation == CUSTOMER
}

// PreferenceRules returns a list of rules for preference
func (dp *DefaultPolicy) PreferenceRules() []func(Route) *int {
	return []func(Route) *int{
		func(route Route) *int {
			val := int(route.FirstHop.Relation)
			return &val
		},
		func(route Route) *int {
			val := len(route.Path)
			return &val
		},
		func(route Route) *int {
      // Using initial character as a placeholder temporarly
			val := int(route.FirstHop.ID[0]) 
			return &val
		},
	}
}

// Policy for handling PBR based on packet attributes
type PBRPolicy struct {
	DefaultPolicy
	PacketCriteria map[string]string // Key-value pairs for matching packet criteria
}

func (p *PBRPolicy) AcceptRoute(route Route) bool {
	// Check if the route matches the specified packet criteria for PBR
	return p.matchesPacketCriteria(route.Packet) && p.DefaultPolicy.AcceptRoute(route)
}

func (p *PBRPolicy) matchesPacketCriteria(packet Packet) bool {
	// Check each criterion in PacketCriteria
	for key, value := range p.PacketCriteria {
		switch key {
		case "SourceAddr":
			if packet.SourceAddr != value {
				return false
			}
		case "DestinationAddr":
			if packet.DestinationAddr != value {
				return false
			}
		case "Size":
			if packet.Size != int(value[0]-'0') { // Simplified for example
				return false
			}
		case "Protocol":
			if packet.Protocol != value {
				return false
			}
		}
	}
	return true
}

// Custom routing policy implementations
type RPKIPolicy struct {
	DefaultPolicy
}

func (rp *RPKIPolicy) AcceptRoute(route Route) bool {
	return rp.DefaultPolicy.AcceptRoute(route) && !route.OriginInvalid
}

// Helper function to check if a route contains a cycle
func containsCycle(route Route) bool {
	// Implementation for cycle detection (omitted for brevity)
	return false
}
