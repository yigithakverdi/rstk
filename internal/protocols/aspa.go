package protocols 

import (
  log "github.com/sirupsen/logrus"
  "rstk/internal/common"
  "fmt"
  "errors"
)

// This file contains the implementation of the ASPA protocol, as described in the IETF draft
// It contains following definitions respectively:
//    - Data structures
//    - Helper functions (for verification and ramp calculations)
//    - Verification functions

// ASPA relations
type Relation string 

// Verification outcomes
type Outcome string

const (
  Valid Outcome = "Valid"
  Unknown Outcome = "Unknown"
  Invalid Outcome = "Invalid"
)

const (
  Provider Relation = "Provider"             // AS x attests AS y as a provider
  NonProvider Relation = "NonProvider"       // A hop that violates the provider relationship
  NoAttestation Relation = "NoAttestation"   // A hop where the relationship isn't confirmed or attested by ASPA
)

// Union of SPAS, An AS MUST list in its SPAS the union of all its Provider AS(es)
// and non-transparent RS AS(es) at which it is an RS-client
type USPASTable map[int]map[int]void


// Global types and variables
type void = common.Void

// IETF Draft: draft-ietf-sidrops-aspa-verification-19
// Direct link: https://datatracker.ietf.org/doc/draft-ietf-sidrops-aspa-verification/
//
// ASPA implementations are directly referenced from the IETF draft, as with the other protocols in the
// protocols package, protocols are used later in the either policies or routers for accepting routes,
// preferring routes etc. For close to exact implementations following IETF draft is 
// draft-ietf-sidrops-aspa-verification-19
//
// Abstract
// This document describes procedures that make use of Autonomous System. Provider Authorization (ASPA)
// objects in the Resource Public Key Infrastructure (RPKI) to verify the Border Gateway Protocol (BGP) 
// AS_PATH attribute of advertised routes.  This type of AS_PATH verification provides detection and 
// mitigation of route leaks and some forms of AS path manipulation.  It also provides protection, to 
// some degree, against prefix hijacks with forged-origin or forged-path-segment.

// ASPA record is a An ASPA record is a digitally signed object that binds a set of PAS
// numbers to a CAS number and is signed by the CAS, notaiont (AS x, {AS y1, AS y2, ...}), 
// is used to represent an ASPA object for a CAS denoted as AS x
// In this notation, the comma-separated set {AS y1, AS y2, ...} represents the Set of 
// Provider ASes (SPAS) of the CAS (AS x).  A CAS is expected to register a single ASPA 
// listing all its Provider ASes
type ASPAObject struct {
  CustomerAS  int      // CAS AS x
  ASPASet     []int    // PAS AS y1, AS y2, ...
  Signature   []byte   // Signature of the ASPA object
}

// A compliant AS should register a single ASPA, to prevent race conditions, during ASPA
// updates that might affect prefix propagaitons, this should be enforced, to accomplish
// this a helper function for checking if the AS has multiple ASPA objects and if so
// checking the U-SPAS and returning true or false for indicating statement is met or not
func IsCompliantAS(aspaList []ASPAObject) bool {
  if len(aspaList) == 1 {
    return true
  } else if len(aspaList) > 1 {
    if len(GetUnionSPAS(aspaList)) == 1 {
      return true
    }
  } else {
    return false
  }
  return false
}

func IsCryptographicallyValid(aspa int) bool {
  return true
}

// If a CAS has single cryptographically valid ASPA, then the Union SPAS (U-SPAS) for the
// CAS equals to SPAS. In case a CAS has multiple cryptographically valid ASPAs, then 
// the U-SPAS for the CAS is the union of AS listed in all SPAS of these ASPAs.
func GetUnionSPAS(aspaList []ASPAObject) map[int]void {
  unionSPAS := make(map[int]void)  
  for _, aspa := range aspaList {
    for _, as := range aspa.ASPASet {
      unionSPAS[as] = void{}
    }
  }
  return unionSPAS
}

// Let AS x and AS y represent two unique ASes.  A provider
// authorization function, authorized(AS x, AS y), checks if the ordered
// pair of ASNs, (AS x, AS y), has the property that AS y is an attested
// provider of AS x per U-SPAS of AS x.  By term "Provider+" the
// function should signal if AS y plays role of Provider, non-
// transparent RS, or mutual-transit neighbor.  This function is
// specified as follows:
//
//                            /
//                            | "No Attestation" if there is no entry
//                            |   in U-SPAS table for CAS = AS x
//                            |
// authorized(AS x, AS y) =  / Else, "Provider+" if the U-SPAS entry
//                            \   for CAS = AS x includes AS y
//                            |
//                            | Else, "Not Provider+"
//                            \
//
//               Figure 1: Provider authorization function.
//
// The "No Attestation" result is returned only when no ASPA is
// retrieved for the CAS or none of its ASPAs are cryptographically
// valid.  The provider authorization function is used in the ASPA-based
// AS_PATH verification algorithms described in Section 7.2 and
// Section 7.3.
func ProviderAuthorization(customerAS, providerAS int, uspasTable USPASTable) (Relation, error) {
	// Check if the customer AS (CAS) exists in the U-SPAS table
  log.Infof("Current received CAS is AS%d and PAS is AS%d", customerAS, providerAS)
	_, exists := uspasTable[customerAS]
	if !exists {
    log.Warnf("No attestation for CAS %d", customerAS)
		return NoAttestation, nil
	}

  // Verify that at least one ASPA is cryptographically valid
  // TODO for temporarly disabling this check
  // hasValidASPA := false
  // for provider := range uspasTable {
  //     if IsCryptographicallyValid(provider) {
  //         log.Info("Found a valid ASPA")
  //         hasValidASPA = true
  //         break
  //     }
  // }
  // if !hasValidASPA {
  //     log.Infof("Has no valid ASPA")
  //     return NoAttestation, nil
  // }

	// Check if AS y is an attested provider of AS x, out of the U-SPAS table
  // that is in the structure of map[int]map[int]void
  if _, exists := uspasTable[customerAS][providerAS]; exists {
    log.Infof("AS%d is an attested provider of AS%d", providerAS, customerAS)
    return Provider, nil
  }

	// Default case: Not Provider+
  log.Infof("AS%d is not an attested provider of AS%d", providerAS, customerAS)
	return NonProvider, nil
}

// Maximum and minimum length of up-ramp and down-ramp calculations. These functions are helper functions
// to be used in verifying functions
//
// - These represent the longest possible valid up-ramp and down-ramp based on ASPA rules.
//   If the sum of these lengths is less than N, the AS_PATH has a problem and is Invalid.
//
// - These represent the shortest possible valid up-ramp and down-ramp, considering uncertainties 
//   (e.g., "No Attestation"). If the sum of these lengths is less than N, we don’t have enough 
//   information to confirm the AS_PATH, so the result is Unknown.

// CalculateMaxUpRamp calculates the maximum up-ramp length.
func calculateUpRamp(asPath []int, uspasTable USPASTable) (maxUpRamp int, minUpRamp int) {
    N := len(asPath)
    maxUpRamp = N
    minUpRamp = N
    
    log.Infof("Calculating max up-ramp")
    // Calculate max_up_ramp
    for i := 0; i < N-1; i++ {
        providerAS := asPath[i]
        customerAS := asPath[i+1]
        log.Infof("Current <CAS, PAS> pair: <%d, %d>", customerAS, providerAS)

        // relation, err := r.ASPA.Authorize(x, y)
        relation, err := ProviderAuthorization(customerAS, providerAS, uspasTable)
        if err != nil {
            // Handle error appropriately
            fmt.Printf("Authorization error: %v\n", err)
            break
        }
        if relation != Provider {
            maxUpRamp = i
            break
        }
    }

    log.Infof("Calculating min up-ramp")
    // Calculate min_up_ramp
    for i := 0; i < N-1; i++ {
        providerAS := asPath[i]
        customerAS := asPath[i+1]
        log.Infof("Current <CAS, PAS> pair: <%d, %d>", customerAS, providerAS)
    
        // relation, err := r.ASPA.Authorize(x, y)
        relation, err := ProviderAuthorization(customerAS, providerAS, uspasTable)
        if err != nil {
            // Handle error appropriately
            fmt.Printf("Authorization error: %v\n", err)
            break
        }
        if relation == NoAttestation || relation != Provider {
            minUpRamp = i
            break
        }
    }
    return
}

// calculateDownRamp calculates max_down_ramp and min_down_ramp based on the AS_PATH.
func calculateDownRamp(asPath []int, uspasTable USPASTable) (maxDownRamp int, minDownRamp int) {
    N := len(asPath)
    maxDownRamp = N
    minDownRamp = N
    
    log.Infof("Calculating max down-ramp")
    // Calculate max_down_ramp
    for i := N - 1; i > 0; i-- {
        providerAS := asPath[i]
        customerAS := asPath[i-1]
        log.Infof("Current <CAS, PAS> pair: <%d, %d>", customerAS, providerAS)
            
        // relation, err := r.ASPA.Authorize(x, y)
        relation, err := ProviderAuthorization(customerAS, providerAS, uspasTable)
        if err != nil {
            // Handle error appropriately
            fmt.Printf("Authorization error: %v\n", err)
            break
        }
        if relation != Provider {
            maxDownRamp = N - i
            break
        }
    }
    
    log.Infof("Calculating min down-ramp")
    // Calculate min_down_ramp
    for i := N - 1; i > 0; i-- {
        providerAS := asPath[i]
        customerAS := asPath[i-1]
        log.Infof("Current <CAS, PAS> pair: <%d, %d>", customerAS, providerAS)
    
        // relation, err := r.ASPA.Authorize(x, y)
        relation, err := ProviderAuthorization(customerAS, providerAS, uspasTable)    

        if err != nil {
            // Handle error appropriately
            fmt.Printf("Authorization error: %v\n", err)
            break
        }
        if relation == NoAttestation || relation != Provider {
            minDownRamp = N - i
            break
        }
    }

    return
}


// This function compresses the AS path to remove duplicates or sequences that don't affect validation.
func CompressASPath(asPath []int) []int {
    compressed := []int{}
    for i, as := range asPath {
        if i == 0 || as != asPath[i-1] {
            compressed = append(compressed, as)
        }
    }
    return compressed
}

// VerifyUpstreamPath(asPath []int, neighborAS int, isRSClient bool, uspaspTable USPASTable) Outcome 
// UpstreamVerifyASPath applies the upstream verification algorithm.
func UpstreamVerifyASPath(originRoute int, asPath []int, uspasTable USPASTable) (Outcome, error) {
    if len(asPath) == 0 {
        return Invalid, errors.New("AS_PATH is empty")
    }

    // asPath := route.PathASNumbers()
    N := len(asPath)

    // Received AS path on up-stream calculaton
    log.Infof("Received AS path %v (US)", asPath)

    // Step 2: Check if the most recently added AS matches the neighbor AS
    // The most recently added AS is the last AS in the path
    neighborAS := asPath[0]
    if neighborAS != originRoute {
        log.Infof("Most recently added AS %d does not match receiving AS %d", neighborAS, originRoute)
        return Invalid, fmt.Errorf("Most recently added AS %d does not match receiving AS %d", neighborAS, originRoute)
    }

    // Step 3: Check for AS_SET in AS_PATH
    // Assuming AS_SET is represented differently, e.g., as a list or with special markers.
    // If AS_SET is present, halt with Invalid.
    // For simplicity, assuming AS_SET is not represented, so skipping this step.
    // Implement AS_SET detection as per your AS_PATH representation.

    // Step 4 and 5: Use ramp calculations
    log.Infof("Calculating up-ramp for AS path %v", asPath)
    maxUpRamp, minUpRamp := calculateUpRamp(asPath, uspasTable)

    if maxUpRamp < N {
        log.Infof("Max up-ramp %d is less than N %d", maxUpRamp, N)
        return Invalid, nil
    }

    if minUpRamp < N {
        log.Infof("Min up-ramp %d is less than N %d", minUpRamp, N)
        return Unknown, nil
    }

    log.Infof("ASPA validation successful, max up-ramp (%d) and min up-ramp (%d) is >= N (%d)", maxUpRamp, minUpRamp, N)
    return Valid, nil
}

// DownstreamVerifyASPath applies the downstream verification algorithm.
func DownstreamVerifyASPath(originRoute int, asPath []int, uspasTable USPASTable) (Outcome, error) {
    if len(asPath) == 0 {
        return Invalid, errors.New("AS_PATH is empty")
    }

    // asPath := route.PathASNumbers()
    N := len(asPath)

    // Received AS path on down-stream calculaton
    log.Infof("Received AS path %v (DS)", asPath)

    // Step 2: Check if the most recently added AS matches the neighbor AS
    // since the asPath is reversed instead of taking asPath[len(asPath)-1] we take asPath[0]
    neighborAS := asPath[0]

    if neighborAS != originRoute {
        log.Infof("Most recently added AS %d does not match receiving AS %d", neighborAS, originRoute)
        return Invalid, fmt.Errorf("Most recently added AS %d does not match receiving AS %d", neighborAS, originRoute)
    }

    // Step 3: Check for AS_SET in AS_PATH
    // Assuming AS_SET is represented differently, e.g., as a list or with special markers.
    // If AS_SET is present, halt with Invalid.
    // For simplicity, assuming AS_SET is not represented, so skipping this step.
    // Implement AS_SET detection as per your AS_PATH representation.

    // Step 4 and 5: Use ramp calculations
    log.Infof("Calculating down-ramp for AS path %v", asPath)
    maxUpRamp, minUpRamp := calculateUpRamp(asPath, uspasTable)
    maxDownRamp, minDownRamp := calculateDownRamp(asPath, uspasTable)

    if (maxUpRamp + maxDownRamp) < N {
        log.Infof("Max up-ramp %d and max down-ramp %d is less than N %d", maxUpRamp, maxDownRamp, N)
        return Invalid, nil
    }

    if (minUpRamp + minDownRamp) < N {
        log.Infof("Min up-ramp %d and min down-ramp %d is less than N %d", minUpRamp, minDownRamp, N)
        return Unknown, nil
    }
    
    log.Infof("ASPA validation successful, max up-ramp + max down-ramp (%d), and min up-ramp + min down-ramp (%d) is >= N (%d)", 
                maxUpRamp + maxDownRamp, 
                minUpRamp + minDownRamp, 
                N)
    return Valid, nil
}

