package protocols 

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

// Define the U-SPAS table as a map of AS x (Customer AS) to its attested providers.
// Simulate the U-SPAS table.
// var uspaspTable = USPASTable{
// 	99: {200, 300},   // AS 200 and 300 are attested providers for AS 100
// 	100: {201},       // AS 201 is an attested provider for AS 101
// }
// Union of SPAS, An AS MUST list in its SPAS the union of all its Provider AS(es)
// and non-transparent RS AS(es) at which it is an RS-client
type USPASTable map[int][]int


// Global types and variables
type void struct{}

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
  CustomerAS int   // CAS AS x
  ASPASet []int    // PAS AS y1, AS y2, ...
  Signature []byte    // Signature of the ASPA object
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
func GetUnionSPAS(aspaList []ASPAObject) []int {
  var unionSPAS []int
  for _, aspa := range aspaList {
    for _, as := range aspa.ASPASet {
      unionSPAS = append(unionSPAS, as)
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
func ProviderAuthorization(x, y int, uspaspTable USPASTable) (string, error) {
	// Check if the customer AS (CAS) exists in the U-SPAS table
	providers, exists := uspaspTable[x]
	if !exists {
		return "No Attestation", nil
	}

	// Verify the cryptographic validity of all ASPAs
	for _, aspa := range providers {
		if !IsCryptographicallyValid(aspa) {
			return "No Attestation", nil
		}
	}

	// Check if AS y is an attested provider of AS x
	for _, provider := range providers {
		if provider == y {
			return "Provider+", nil
		}
	}

	// Default case: Not Provider+
	return "Not Provider+", nil
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
func CalculateMaxUpRamp(asPath []int, uspaspTable USPASTable) int {
	n := len(asPath)
	for i := 0; i < n-1; i++ {
		authResult, _ := ProviderAuthorization(asPath[i], asPath[i+1], uspaspTable)
		if authResult == "Not Provider+" {
			return i
		}
	}
	return n - 1
}

// CalculateMinUpRamp calculates the minimum up-ramp length.
func CalculateMinUpRamp(asPath []int, uspaspTable USPASTable) int {
	n := len(asPath)
	for i := 0; i < n-1; i++ {
		authResult, _ := ProviderAuthorization(asPath[i], asPath[i+1], uspaspTable)
		if authResult == "Not Provider+" || authResult == "No Attestation" {
			return i
		}
	}
	return n - 1
}

// CalculateMaxDownRamp calculates the maximum down-ramp length.
func CalculateMaxDownRamp(asPath []int, uspaspTable USPASTable) int {
	n := len(asPath)
	for j := n - 1; j > 0; j-- {
		authResult, _ := ProviderAuthorization(asPath[j], asPath[j-1], uspaspTable)
		if authResult == "Not Provider+" {
			return n - j
		}
	}
	return n - 1
}

// CalculateMinDownRamp calculates the minimum down-ramp length.
func CalculateMinDownRamp(asPath []int, uspaspTable USPASTable) int {
	n := len(asPath)
	for j := n - 1; j > 0; j-- {
		authResult, _ := ProviderAuthorization(asPath[j], asPath[j-1], uspaspTable)
		if authResult == "Not Provider+" || authResult == "No Attestation" {
			return n - j
		}
	}
	return n - 1
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

// The upstream verification algorithm described here is applied when a route is received from a
// Customer or Peer, or is received by an RS from an RS-client, or is received by an RS-client 
// from an RS.  In all these cases, the receiving/validating eBGP router expects the AS_PATH 
// to have only an up-ramp (no down-ramp) for it to be Valid. Therefore, max_down_ramp and 
// min_down_ramp are set to 0.
func VerifyUpstreamPath(asPath []int, neighborAS int, isRSClient bool, uspaspTable USPASTable) Outcome {
	n := len(asPath)

	// 1. If the AS_PATH is empty
	if n == 0 {
		return Outcome("Invalid")
	}

	// 2. If the receiving AS is not an RS-client and the most recently added AS does not match the neighbor AS
	if !isRSClient && asPath[n-1] != neighborAS {
		return Outcome("Invalid")
	}

	// 3. If the AS_PATH has an AS_SET
	if HasASSet(asPath) {
		return Outcome("Invalid")
	}

	// Calculate ramp lengths
	maxUpRamp := CalculateMaxUpRamp(asPath, uspaspTable)
	minUpRamp := CalculateMinUpRamp(asPath, uspaspTable)

	// 4. If max_up_ramp < N
	if maxUpRamp < n {
		return Outcome("Invalid")
	}

	// 5. If min_up_ramp < N
	if minUpRamp < n {
		return Outcome("Unknown")
	}

	// 6. Else, the procedure halts with the outcome "Valid"
	return Outcome("Valid")
}

// The downstream verification algorithm described here is applied when a route is received from a
// provider. Both up-ramp and down-ramp are calculated. The sum of the maximum and minimum lengths 
// of the up-ramp and down-ramp is checked against the AS_PATH length
func VerifyDownstreamPath(asPath []int, neighborAS int, uspaspTable USPASTable) Outcome {
	n := len(asPath)

	// 1. If the AS_PATH is empty
	if n == 0 {
		return Outcome("Invalid")
	}

	// 2. If the most recently added AS does not match the neighbor AS
	if asPath[n-1] != neighborAS {
		return Outcome("Invalid")
	}

	// 3. If the AS_PATH has an AS_SET
	if HasASSet(asPath) {
		return Outcome("Invalid")
	}

	// Calculate ramp lengths
	maxUpRamp := CalculateMaxUpRamp(asPath, uspaspTable)
	minUpRamp := CalculateMinUpRamp(asPath, uspaspTable)
	maxDownRamp := CalculateMaxDownRamp(asPath, uspaspTable)
	minDownRamp := CalculateMinDownRamp(asPath, uspaspTable)

	// 4. If max_up_ramp + max_down_ramp < N
	if maxUpRamp+maxDownRamp < n {
		return Outcome("Invalid")
	}

	// 5. If min_up_ramp + min_down_ramp < N
	if minUpRamp+minDownRamp < n {
		return Outcome("Unknown")
	}

	// 6. Else, the procedure halts with the outcome "Valid"
	return Outcome("Valid")
}

func HasASSet(asPath []int) bool {
	// Stub function: Replace with logic to detect AS_SET in the AS_PATH.
	// This can involve checking specific encodings or markers that denote AS_SETs in the path.
	return true 
}
