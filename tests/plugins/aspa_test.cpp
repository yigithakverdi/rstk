#include "engine/rpki/rpki.hpp"
#include "engine/topology/topology.hpp"
#include "parser/parser.hpp"
#include "plugins/aspa/aspa.hpp"
#include "logger/logger.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

// Assuming ASPATest is defined as provided in your setup
class ASPATest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;
  std::unique_ptr<Topology> topology;
  std::vector<AsRel> testRelations;
  Parser parser;

  void SetUp() override {
    // Initialize logger
    LOG.setLevel(LogLevel::NONE);
    
    // Initialize RPKI
    rpki = std::make_shared<RPKI>();

    // Setup test relations - hardcoded for the test
    testRelations =
        {
            {1, 2, -1},  {1, 3, -1},  {1, 4, -1}, // Tier 1 connections
            {2, 3, 0},   {2, 5, -1},  {2, 6, -1}, // Tier 2 interconnections
            {3, 6, -1},  {3, 7, -1},  {4, 7, -1},  {4, 8, -1},
            {5, 6, 0},   {6, 7, 0},                // Tier 2 peer connections
            {7, 8, 0},   {5, 9, -1},  {5, 10, -1}, // Tier 3 connections
            {6, 11, -1}, {6, 12, -1}, {7, 13, -1}, {7, 14, -1},
            {8, 15, -1}, {8, 16, -1}, {9, 17, -1}, {16, 18, -1} // Additional connections
        };

    // Initialize topology with relations
    topology = std::make_unique<Topology>(testRelations, rpki);

    // Create and set deployment strategy (100% deployment for both objects and policies)
    auto deploymentStrategy = std::make_unique<ASPADeployment>(100.0, 100.0);
    topology->setDeploymentStrategy(std::move(deploymentStrategy));
    topology->deploy();
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
TEST_F(ASPATest, ASPACase01) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(9);

  ASSERT_NE(victim, nullptr) << "Victim AS 17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS 9 not found in topology.";

  Route *route = createRoute({17, 9});

  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);
  ASSERT_NE(aspaProto, nullptr) << "ASPA Protocol not created.";

  ASPAResult result = policyEngine->PerformASPA(*route);
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

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
  EXPECT_EQ(result, ASPAResult::Invalid) << "Path should be Invalid due to route leak by AS6";

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
  LOG.setLevel(LogLevel::DEBUG);
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
  EXPECT_EQ(result, ASPAResult::Valid) << "Path should be Valid for 6 --(Peer)--> 5 --(Provider)--> 9 --(Provider)--> 17 path";

  delete route;
}

TEST_F(ASPATest, ASPACase07) {
  LOG.setLevel(LogLevel::DEBUG);
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
      << "Path should be Invalid due to  7 --(Peer)-> 6 --(Peer)-> 5 --(Provider)-> 9"
         " relationships.";

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
  LOG.setLevel(LogLevel::DEBUG);
  // Test upstream path verification: Customer -> Provider -> Peer -> Peer
  // Path: [AS9 -> AS5 -> AS6 -> AS7], expecting Invalid

  // Step 1: Retrieve victim AS (AS9) and verifying AS (AS7)
  auto victim = topology->GetRouter(9);
  auto verifying_as = topology->GetRouter(3);

  ASSERT_NE(victim, nullptr) << "Victim AS9 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS7 not found in topology.";

  Route *route = createRoute({9, 5, 6, 3});

  // Step 5: Initialize ASPAProtocol and ASPAPolicyEngine
  auto aspaProto = std::make_unique<ASPAProtocol>(rpki);
  auto policyEngine = std::make_unique<ASPAPolicyEngine>(rpki);

  // Step 6: Perform ASPA verification
  ASPAResult result = policyEngine->PerformASPA(*route);

  // Step 7: Expect 'Invalid' due to ASPA violation
  EXPECT_EQ(result, ASPAResult::Invalid)
      << "Path should be Invalid due to 3 --(Provider)--> 6 --(Peer)--> 5 --(Provider)--> 9 "
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
  Route *route = createRoute({9, 5, 10});

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
      << "Path should be Invalid due to 10 --(Customer)--> 5 --(Peer)--> 6 "
         "relationships.";

  // Step 8: Clean up
  delete route;
}

// Test Case 14: Valid Route - Upstream of Upstream Sending Route
TEST_F(ASPATest, ASPACase14) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 15: Valid Route - Peer of Upstream of Upstream Sending Route
TEST_F(ASPATest, ASPACase15) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 16: Valid Route - Inverted V Shape
TEST_F(ASPATest, ASPACase16) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 17: Valid Route - Inverted V Shape Opposite Direction
TEST_F(ASPATest, ASPACase17) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 18: Valid Route - Inverted V Shape with p2p at Apex
TEST_F(ASPATest, ASPACase18) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 19: Valid Route - Inverted V Shape Opposite Direction
TEST_F(ASPATest, ASPACase19) {
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
  EXPECT_EQ(result, ASPAResult::Valid) << "ASPA verification did not return 'Valid'.";

  // Clean up
  delete route;
}

// Test Case 20: Unknown Route - Inverted V Shape, AS5 has no ASPA
TEST_F(ASPATest, ASPACase20) {
  auto victim = topology->GetRouter(17);
  auto verifying_as = topology->GetRouter(12);

  ASSERT_NE(victim, nullptr) << "Victim AS17 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS12 not found in topology.";

  // Path: [17,9,5,2,6,12]
  Route *route = createRoute({17, 9, 5, 2, 6, 12});

  // Remove ASPA object for AS5, and 
  rpki->USPAS.erase(5);
  auto router5 = topology->GetRouter(5);
  ASSERT_NE(router5, nullptr);

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
TEST_F(ASPATest, ASPACase21) {
  auto victim = topology->GetRouter(12);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS12 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [12,6,2,5,9,17]
  Route *route = createRoute({12, 6, 2, 5, 9, 17});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  auto router5 = topology->GetRouter(5);
  ASSERT_NE(router5, nullptr);

  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router5->proto.get())) {
    aspaProto->clearASPAObjects();
  }


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
TEST_F(ASPATest, ASPACase22) {
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
TEST_F(ASPATest, ASPACase23) {
  auto victim = topology->GetRouter(14);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS14 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [14,7,3,2,5,9,17]
  Route *route = createRoute({14, 7, 3, 2, 5, 9, 17});

  // Remove ASPA object for AS5
  rpki->USPAS.erase(5);

  auto router5 = topology->GetRouter(5);
  ASSERT_NE(router5, nullptr);

  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router5->proto.get())) {
    aspaProto->clearASPAObjects();
  }


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
TEST_F(ASPATest, ASPACase24) {
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
TEST_F(ASPATest, ASPACase25) {
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
TEST_F(ASPATest, ASPACase26) {
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
TEST_F(ASPATest, ASPACase27) {
  auto victim = topology->GetRouter(14);
  auto verifying_as = topology->GetRouter(17);

  ASSERT_NE(victim, nullptr) << "Victim AS14 not found in topology.";
  ASSERT_NE(verifying_as, nullptr) << "Verifying AS17 not found in topology.";

  // Path: [14,7,3,2,5,9,17]
  Route *route = createRoute({14, 7, 3, 2, 5, 9, 17});

  // Remove ASPA objects for AS5 and AS6
  rpki->USPAS.erase(5);
  rpki->USPAS.erase(6);

  auto router5 = topology->GetRouter(5);
  auto router6 = topology->GetRouter(6);
  ASSERT_NE(router5, nullptr);
  ASSERT_NE(router6, nullptr);

  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router5->proto.get())) {
    aspaProto->clearASPAObjects();
  }
  if (auto aspaProto = dynamic_cast<ASPAProtocol *>(router6->proto.get())) {
    aspaProto->clearASPAObjects();
  }


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
TEST_F(ASPATest, ASPACase30) {
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
TEST_F(ASPATest, ASPACase31) {
  LOG.setLevel(LogLevel::DEBUG);
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
TEST_F(ASPATest, ASPACase32) {
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
TEST_F(ASPATest, ASPACase33) {
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
