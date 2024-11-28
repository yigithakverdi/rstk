package rpki

import (
  "rstk/internal/common"
)

type void = common.Void

// ASPA related data structures published to RPKI by the routers
type ASPA struct {
  USPAS map[int]map[int]void
}

type RPKI struct {
 ASPA ASPA
}

// TODO currently RPKI is just a, simple struct storing bunch of global data, later on
// it will be changed to more accurate represntation of RPKI system
// Creates the RPKI instances
func NewRPKI() *RPKI {
  return &RPKI{
    ASPA: ASPA{
      USPAS: make(map[int]map[int]void),
    },
  }
}

func (rpki *RPKI) ResetUSPASRecord(asID int) {
  rpki.ASPA.USPAS[asID] = make(map[int]void)
}
