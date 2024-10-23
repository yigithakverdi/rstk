package models 

// Route realted functions
type Route struct {
	Path            []Router
  PathLength      int
  FinalAS         int 
  FirstHopAS      int
	Authenticated   bool
	OriginInvalid   bool
  PathEndInvalid  bool
  Packet          Packet 
}

func (r *Route) Length() int {
  return r.PathLength
}

func (r *Route) Origin() Router {
  if(len(r.Path) > 0) {
    return r.Path[0]
  }
  return Router{}
}

func (r *Route) FirstHop() Router {
  if(len(r.Path) > 1) {
    return r.Path[len(r.Path) - 2]
  }
  return Router{}
}

func (r *Route) Final() Router {
  if(len (r.Path) > 0) {
    return r.Path[len(r.Path) - 1]
  }
  return Router{}
}
