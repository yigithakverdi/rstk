package models

type Topology struct {
	Routers []Router
	Links   []Link
}

type Link struct {
	Source      string // Router ID of the source router
	Destination string // Router ID of the destination router
	SourceIP    string // IP address on the source router's interface
	DestIP      string // IP address on the destination router's interface
}
