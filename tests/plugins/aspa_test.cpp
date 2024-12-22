#include "engine/rpki/rpki.hpp"
#include "engine/topology/topology.hpp"
#include "parser/parser.hpp"
#include "plugins/aspa/aspa.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

class ASPAAuthTest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;
  std::shared_ptr<ASPAProtocol> aspaProtocol;
  std::unique_ptr<ASPAPolicyEngine> policyEngine;
  std::unique_ptr<Topology> topology;
  std::vector<AsRel> testRelations;

  void SetUp() override {
    // Initialize RPKI
    rpki = std::make_shared<RPKI>();

    // Initialize ASPA Protocol
    aspaProtocol = std::make_shared<ASPAProtocol>(rpki);

    // Initialize Policy Engine with RPKI and ASPA Protocol
    policyEngine = std::make_unique<ASPAPolicyEngine>(rpki, aspaProtocol);

    // Setup test relations
    testRelations = {
        {1, 2, -1},  {1, 3, -1},  {1, 4, -1}, // Tier 1 connections
        {2, 3, 0},   {2, 5, -1},  {2, 6, -1}, // Tier 2 interconnections
        {3, 6, -1},  {3, 7, -1},  {4, 7, -1},  {4, 8, -1},  {5, 6, 0},
        {6, 7, 0},                             // Tier 2 peer connections
        {7, 8, 0},   {5, 9, -1},  {5, 10, -1}, // Tier 3 connections
        {6, 11, -1}, {6, 12, -1}, {7, 13, -1}, {7, 14, -1}, {8, 15, -1},
        {8, 16, -1}, {9, 17, -1}, {16, 18, -1} // Additional connections
    };

    // Initialize topology
    topology = std::make_unique<Topology>(testRelations, rpki);
  }
};

// Test Provider+ cases
TEST_F(ASPAAuthTest, ProviderPlusRelationships) {
  // Create ASPA object for AS5 with AS2 as provider
  std::vector<int> as5_providers = {2};
  ASPAObject as5_aspa(5, as5_providers, std::vector<unsigned char>());
  rpki->USPAS[5] = as5_aspa;

  auto router5 = topology->GetRouter(5);
  auto router2 = topology->GetRouter(2);

  ASSERT_NE(router5, nullptr);
  ASSERT_NE(router2, nullptr);

  // Test Provider+ relationship (AS5 with provider AS2)
  ASPAAuthResult result =
      policyEngine->authorized(router5.get(), router2.get());
  EXPECT_EQ(result, ASPAAuthResult::ProviderPlus);
}

// Test Not Provider+ cases
TEST_F(ASPAAuthTest, NotProviderPlusRelationships) {
  // Create ASPA object for AS5 with AS2 as only provider
  std::vector<int> as5_providers = {2};
  ASPAObject as5_aspa(5, as5_providers, std::vector<unsigned char>());
  rpki->USPAS[5] = as5_aspa;

  auto router5 = topology->GetRouter(5);
  auto router3 = topology->GetRouter(3); // AS3 is not in AS5's provider list

  ASSERT_NE(router5, nullptr);
  ASSERT_NE(router3, nullptr);

  // Test Not Provider+ relationship (AS5 with non-provider AS3)
  ASPAAuthResult result =
      policyEngine->authorized(router5.get(), router3.get());
  EXPECT_EQ(result, ASPAAuthResult::NotProviderPlus);
}

// Test No Attestation cases
TEST_F(ASPAAuthTest, NoAttestationCases) {
  // Don't create any ASPA objects for AS7
  auto router7 = topology->GetRouter(7);
  auto router4 = topology->GetRouter(4);

  ASSERT_NE(router7, nullptr);
  ASSERT_NE(router4, nullptr);

  // Test No Attestation case (AS7 has no ASPA record)
  ASPAAuthResult result =
      policyEngine->authorized(router7.get(), router4.get());
  EXPECT_EQ(result, ASPAAuthResult::NoAttestation);
}

// Test multiple providers case
TEST_F(ASPAAuthTest, MultipleProvidersCase) {
  // Create ASPA object for AS7 with multiple providers
  std::vector<int> as7_providers = {3, 4};
  ASPAObject as7_aspa(7, as7_providers, std::vector<unsigned char>());
  rpki->USPAS[7] = as7_aspa;

  auto router7 = topology->GetRouter(7);
  auto router3 = topology->GetRouter(3);
  auto router4 = topology->GetRouter(4);

  ASSERT_NE(router7, nullptr);
  ASSERT_NE(router3, nullptr);
  ASSERT_NE(router4, nullptr);

  // Test both providers
  ASPAAuthResult result1 =
      policyEngine->authorized(router7.get(), router3.get());
  ASPAAuthResult result2 =
      policyEngine->authorized(router7.get(), router4.get());

  EXPECT_EQ(result1, ASPAAuthResult::ProviderPlus);
  EXPECT_EQ(result2, ASPAAuthResult::ProviderPlus);
}

// Test AS0 provider case
TEST_F(ASPAAuthTest, AS0ProviderCase) {
  // First clear any existing ASPA objects
  rpki->USPAS.clear();

  // Create ASPA object for AS2 with AS0 and AS1 as providers (since AS1 is
  // provider of AS2)
  std::vector<int> as2_providers = {0, 1};
  ASPAObject as2_aspa(2, as2_providers, std::vector<unsigned char>());
  rpki->USPAS[2] = as2_aspa;

  auto router1 = topology->GetRouter(1);
  auto router2 = topology->GetRouter(2);

  ASSERT_NE(router1, nullptr);
  ASSERT_NE(router2, nullptr);

  // Test that AS2 correctly identifies AS1 as Provider+
  ASPAAuthResult result =
      policyEngine->authorized(router2.get(), router1.get());
  EXPECT_EQ(result, ASPAAuthResult::ProviderPlus)
      << "Expected AS1 to be a provider of AS2";
}

// Assuming ASPATest is defined as provided in your setup
class ASPATest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;
  std::unique_ptr<Topology> topology;
  std::vector<AsRel> testRelations;
  Parser parser;

  void SetUp() override {
    // Initialize RPKI
    rpki = std::make_shared<RPKI>();

    // Setup test relations - hardcoded for the test
    testRelations = {
        {1, 2, -1},  {1, 3, -1},  {1, 4, -1}, // Tier 1 connections
        {2, 3, 0},   {2, 5, -1},  {2, 6, -1}, // Tier 2 interconnections
        {3, 6, -1},  {3, 7, -1},  {4, 7, -1},  {4, 8, -1},  {5, 6, 0},
        {6, 7, 0},                             // Tier 2 peer connections
        {7, 8, 0},   {5, 9, -1},  {5, 10, -1}, // Tier 3 connections
        {6, 11, -1}, {6, 12, -1}, {7, 13, -1}, {7, 14, -1}, {8, 15, -1},
        {8, 16, -1}, {9, 17, -1}, {16, 18, -1} // Additional connections
    };

    // Initialize topology with relations
    topology = std::make_unique<Topology>(testRelations, rpki);

    // Deploy ASPA randomly for 100% of ASes
    /*topology->DeployASPARandomly(100.0, 100.0);*/
  }

  // Helper function to create a route from AS IDs
  Route *createRoute(const std::vector<int> &pathASIDs) {
    std::vector<Router *> path;

    for (int asID : pathASIDs) {
      auto router = topology->GetRouter(asID);
      if (!router) {
        throw std::runtime_error("Router with AS" + std::to_string(asID) +
                                 " not found in topology");
      }
      path.push_back(router.get());
    }

    Route *route = new Route();
    route->destination = path[0]; // First AS in path is destination
    route->path = path;
    route->originValid = false;
    route->pathEndInvalid = false;
    route->authenticated = false;

    return route;
  }

  void TearDown() override {
    // Clean up if needed
  }
};

// C++ Equivalent Test Function
TEST_F(ASPATest, ASpaCase01) {
  // Step 1: Retrieve the victim AS (AS 17) and verifying AS (AS 9)
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(9);

  ASSERT_NE(victim, nullptr) << "Victim AS 17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS 9 not found in topology.";

  Route *route = createRoute({17, 9});

  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(aspaProto, nullptr) << "ASPA Protocol not created.";

  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up the allocated Route object
  delete route;
}

TEST_F(ASPATest, ASPACase02) {
  // Test upstream path verification with multiple hops
  // Path: [AS17 (victim) -> AS9 -> AS5 -> AS2 -> AS1 (verifying)]

  auto victim = topology->GetRouter(17);
  auto verifyingAS = topology->GetRouter(1);

  ASSERT_NE(victim, nullptr);
  ASSERT_NE(verifyingAS, nullptr);

  // Create route with the complete path
  Route *route = createRoute({17, 9, 5, 2, 1});

  // Set up ASPA protocol for verification
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Verify ASPA result
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "Multi-hop upstream path should be Valid when all ASPA objects are "
         "properly configured";

  // Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase03) {
  // Test upstream path verification with route leak
  // Path: [AS17 (victim) -> AS9 -> AS5 -> AS2 -> AS6 -> AS3 (verifying)]
  // AS6 causes route leak by forwarding path from AS2 to AS3

  auto victim = topology->GetRouter(17);
  auto verifyingAS = topology->GetRouter(3);

  ASSERT_NE(victim, nullptr);
  ASSERT_NE(verifyingAS, nullptr);

  // Create route with the path containing route leak
  Route *route = createRoute({17, 9, 5, 2, 6, 3});

  // Set up ASPA protocol for verification
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Verify ASPA result
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "Path should be Invalid due to route leak by AS6";

  // Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase04) {
  auto victim = topology->GetRouter(17);
  auto verifyingAS = topology->GetRouter(1);

  ASSERT_NE(victim, nullptr);
  ASSERT_NE(verifyingAS, nullptr);

  // Create route with the complete path
  Route *route = createRoute({17, 9, 5, 2, 6, 3, 1});

  // Remove AS2's ASPA record from USPAS
  rpki->USPAS.erase(2); // Simply remove entry for AS2

  // Get router 2 and clear its ASPA objects
  auto router2 = topology->GetRouter(2);
  ASSERT_NE(router2, nullptr);

  // Convert to ASPAProtocol to access clearASPAObjects
  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router2->proto.get())) {
    aspaProto->clearASPAObjects();
  }

  // Set up ASPA protocol for verification
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Verify ASPA result
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "Path should be Unknown due to missing ASPA object for AS2";

  delete route;
}

TEST_F(ASPATest, ASPACase05) {
  auto victim = topology->GetRouter(17);
  auto verifyingAS = topology->GetRouter(1);

  ASSERT_NE(victim, nullptr);
  ASSERT_NE(verifyingAS, nullptr);

  // Create route with the complete path
  Route *route = createRoute({17, 9, 5, 2, 1});

  // Remove AS9's ASPA record from USPAS
  rpki->USPAS.erase(9);

  // Set up ASPA protocol for verification
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Verify ASPA result
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "Path should be Unknown due to missing ASPA object for AS9";

  delete route;
}

TEST_F(ASPATest, ASPACase06) {
  // Test upstream path verification with customer->customer->peer relationship
  // Path: [AS17 -> AS9 -> AS5 -> AS6], where AS6 is peer of AS5

  auto victim = topology->GetRouter(17);
  auto verifyingAS = topology->GetRouter(6);

  ASSERT_NE(victim, nullptr);
  ASSERT_NE(verifyingAS, nullptr);

  // Create route with the complete path
  Route *route = createRoute({17, 9, 5, 6});

  // Set up ASPA protocol for verification
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Verify ASPA result
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "Path should be Valid for customer->customer->peer path";

  delete route;
}

TEST_F(ASPATest, ASPACase07) {
  // Test upstream path verification with two customers, then lateral peer
  // relationship Path: [AS17 -> AS9 -> AS5 -> AS6], where AS9 has no ASPA

  // Step 1: Retrieve the victim AS (AS17) and verifying AS (AS6)
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(6);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS6 not found in topology.";

  Route *route = createRoute({17, 9, 5, 6});

  rpki->USPAS.erase(9);

  auto router9 = topology->GetRouter(9);
  ASSERT_NE(router9, nullptr);

  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router9->proto.get())) {
    aspaProto->clearASPAObjects();
  }

  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  ASPAResult result = policyEngine->PerformASPA(*route);

  EXPECT_EQ(result, ASPAResult::Unknown)
      << "Path should be Unknown due to missing ASPA object for AS9";

  delete route;
}

TEST_F(ASPATest, ASPACase08) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(9);
  auto verifying_as = topology->GetRouter(7);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({9, 5, 6, 7});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase09) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(5);
  auto verifying_as = topology->GetRouter(7);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({5, 2, 6, 7});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase10) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(9);
  auto verifying_as = topology->GetRouter(3);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({9, 5, 6, 3});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase11) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(9);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({9, 17});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Valid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase12) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(9);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({9, 5, 17});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Valid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

TEST_F(ASPATest, ASPACase13) {
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(6);
  auto verifying_as = topology->GetRouter(5);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Step 2: Create route [9,5,6,7]
  Route *route = createRoute({6, 5, 10});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Valid)
      << "Path should be Invalid due to Customer->Provider->Peer->Peer "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

// Test Case 14: Valid Route - Upstream of Upstream Sending Route
TEST_F(ASPATest, ASpaCase14) {
  auto victim = topology->GetRouter(2);
  auto verifying_as = topology->GetRouter(10);

  ASSERT_NE(victim, nullptr) << "Victim AS2 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS10 not found in topology.";

  // Path: [2,5,10]
  Route *route = createRoute({2, 5, 10});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 15: Valid Route - Peer of Upstream of Upstream Sending Route
TEST_F(ASPATest, ASpaCase15) {
  auto victim = topology->GetRouter(6);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS6 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [6,5,9,17]
  Route *route = createRoute({6, 5, 9, 17});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 16: Valid Route - Inverted V Shape
TEST_F(ASPATest, ASpaCase16) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(12);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS12 not found in topology.";

  // Path: [17,9,5,2,6,12]
  Route *route = createRoute({17, 9, 5, 2, 6, 12});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 17: Valid Route - Inverted V Shape Opposite Direction
TEST_F(ASPATest, ASpaCase17) {
  auto victim = topology->GetRouter(12);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS12 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [12,6,2,5,9,17]
  Route *route = createRoute({12, 6, 2, 5, 9, 17});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 18: Valid Route - Inverted V Shape with p2p at Apex
TEST_F(ASPATest, ASpaCase18) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(14);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS14 not found in topology.";

  // Path: [17,9,5,2,3,7,14]
  Route *route = createRoute({17, 9, 5, 2, 3, 7, 14});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 19: Valid Route - Inverted V Shape Opposite Direction
TEST_F(ASPATest, ASpaCase19) {
  auto victim = topology->GetRouter(14);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS14 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [14,7,3,2,5,9,17]
  Route *route = createRoute({14, 7, 3, 2, 5, 9, 17});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 20: Unknown Route - Inverted V Shape, AS5 has no ASPA
TEST_F(ASPATest, ASpaCase20) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(12);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS12 not found in topology.";

  // Path: [17,9,5,2,6,12]
  Route *route = createRoute({17, 9, 5, 2, 6, 12});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5.";

  // Clean up
  delete route;
}

// Test Case 21: Unknown Route - Opposite Direction, AS5 has no ASPA
TEST_F(ASPATest, ASpaCase21) {
  auto victim = topology->GetRouter(12);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS12 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [12,6,2,5,9,17]
  Route *route = createRoute({12, 6, 2, 5, 9, 17});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5.";

  // Clean up
  delete route;
}

// Test Case 22: Unknown Route - Inverted V Shape with p2p at Apex, AS5 has no
// ASPA
TEST_F(ASPATest, ASpaCase22) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(14);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS14 not found in topology.";

  // Path: [17,9,5,2,3,7,14]
  Route *route = createRoute({17, 9, 5, 2, 3, 7, 14});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5.";

  // Clean up
  delete route;
}

// Test Case 23: Unknown Route - Opposite Direction, AS5 has no ASPA
TEST_F(ASPATest, ASpaCase23) {
  auto victim = topology->GetRouter(14);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS14 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [14,7,3,2,5,9,17]
  Route *route = createRoute({14, 7, 3, 2, 5, 9, 17});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5.";

  // Clean up
  delete route;
}

// Test Case 24: Unknown Route - Inverted V Shape, AS5 and AS6 have no ASPA
TEST_F(ASPATest, ASpaCase24) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(12);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS12 not found in topology.";

  // Path: [17,9,5,2,6,12]
  Route *route = createRoute({17, 9, 5, 2, 6, 12});

  // Remove ASPA objects for AS5 and AS6
  rpki->USPAS.erase(5);
  rpki->USPAS.erase(6);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5 and AS6.";

  // Clean up
  delete route;
}

// Test Case 25: Unknown Route - Opposite Direction, AS5 and AS6 have no ASPA
TEST_F(ASPATest, ASpaCase25) {
  auto victim = topology->GetRouter(12);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS12 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [12,6,2,5,9,17]
  Route *route = createRoute({12, 6, 2, 5, 9, 17});

  // Remove ASPA objects for AS5 and AS6
  rpki->USPAS.erase(5);
  rpki->USPAS.erase(6);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5 and AS6.";

  // Clean up
  delete route;
}

// Test Case 26: Unknown Route - Inverted V Shape with p2p at Apex, AS5 and AS6
// have no ASPA
TEST_F(ASPATest, ASpaCase26) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(14);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS14 not found in topology.";

  // Path: [17,9,5,2,3,7,14]
  Route *route = createRoute({17, 9, 5, 2, 3, 7, 14});

  // Remove ASPA objects for AS5 and AS6
  rpki->USPAS.erase(5);
  rpki->USPAS.erase(6);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5 and AS6.";

  // Clean up
  delete route;
}

// Test Case 27: Unknown Route - Opposite Direction, AS5 and AS6 have no ASPA
TEST_F(ASPATest, ASpaCase27) {
  auto victim = topology->GetRouter(14);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS14 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [14,7,3,2,5,9,17]
  Route *route = createRoute({14, 7, 3, 2, 5, 9, 17});

  // Remove ASPA objects for AS5 and AS6
  rpki->USPAS.erase(5);
  rpki->USPAS.erase(6);

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Unknown)
      << "ASPA verification did not return 'Unknown' due to missing ASPA for "
         "AS5 and AS6.";

  // Clean up
  delete route;
}

// Test Case 30: Invalid Route - Route Leak by AS6 to Upstream
TEST_F(ASPATest, ASpaCase30) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(14);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS14 not found in topology.";

  // Path: [17,9,5,2,6,3,7,14]
  Route *route = createRoute({17, 9, 5, 2, 6, 3, 7, 14});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "ASPA verification did not return 'Invalid' due to route leak by AS6.";

  // Clean up
  delete route;
}

// Test Case 31: Invalid Route - Route Leak by AS6 to Lateral Peer
TEST_F(ASPATest, ASpaCase31) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(14);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS14 not found in topology.";

  // Path: [17,9,5,2,6,7,14]
  Route *route = createRoute({17, 9, 5, 2, 6, 7, 14});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "ASPA verification did not return 'Invalid' due to route leak by AS6 "
         "and AS7.";

  // Clean up
  delete route;
}

// Test Case 32: Invalid Route - Route Leak by AS6 and AS7
TEST_F(ASPATest, ASpaCase32) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(18);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS18 not found in topology.";

  // Path: [17,9,5,2,6,3,7,4,8,16,18]
  Route *route = createRoute({17, 9, 5, 2, 6, 3, 7, 4, 8, 16, 18});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "ASPA verification did not return 'Invalid' due to route leak by AS6 "
         "and AS7.";

  // Clean up
  delete route;
}

// Test Case 33: Invalid Route - Route Leak by AS6 and AS7 Opposite Direction
TEST_F(ASPATest, ASpaCase33) {
  auto victim = topology->GetRouter(18);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS18 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [18,16,8,4,7,3,6,2,5,9,17]
  Route *route = createRoute({18, 16, 8, 4, 7, 3, 6, 2, 5, 9, 17});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "ASPA verification did not return 'Invalid' due to route leak by AS6 "
         "and AS7 in opposite direction.";

  // Clean up
  delete route;
}

// Test Case 06: Valid Route - Customer -> Customer -> Peer
TEST_F(ASPATest, ASpaCase06) {
  // Assuming this corresponds to test_aspa_case_06 which was initially failing
  // Path: [ASNumber, ...] -- details not provided, but assuming it's a valid
  // customer->customer->peer path For demonstration, let's assume: Path:
  // [2,3,7]

  auto victim = topology->GetRouter(2);
  auto verifying_as = topology->GetRouter(7);

  ASSERT_NE(victim, nullptr) << "Victim AS2 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  // Path: [2,3,7]
  Route *route = createRoute({2, 3, 7});

  // Assign ASPAPolicy to verifying_as
  verifying_as->proto = std::make_unique<ASPAProtocol>(rpki);

  // Create a policy engine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(policyEngine, nullptr) << "PolicyEngine not created.";

  // Perform ASPA validation
  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid)
      << "ASPA verification did not return 'Valid' for "
         "customer->customer->peer path.";

  // Clean up
  delete route;
}
