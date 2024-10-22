package models

type Neighbor struct {
	IP string
	AS int
}

type Router struct {
	ID         string
	ASNumber   int
	RouterID   string
	Neighbors  []Neighbor
	Interfaces []Interface
}

type Route struct {
	Path          []string
	Final         string 
	FirstHop      string
	Authenticated bool
	OriginInvalid bool
	Packet        Packet 
}

type Interface struct {
	Name string
	IP   string
}

type Packet struct {
	SourceAddr   string 
	DestinationAddr string 
	Size         int    
	Protocol     string 
}
