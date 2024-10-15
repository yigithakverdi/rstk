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

type Interface struct {
	Name string
	IP   string
}
