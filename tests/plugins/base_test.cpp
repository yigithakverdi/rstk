#include "engine/topology/topology.hpp"
#include "plugins/base/base.hpp"
#include "plugins/manager.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <gtest/gtest.h>
#include <memory>

class BaseProtocolTest : public ::testing::Test {
protected:
  std::unique_ptr<BaseProtocol> protocol;
  std::shared_ptr<RPKI> rpki;
  std::shared_ptr<Router> r1;
  std::shared_ptr<Router> r2;
  std::shared_ptr<Router> r3;
  std::shared_ptr<Router> r4;

  void SetUp() override {
    protocol = std::make_unique<BaseProtocol>();
    rpki = std::make_shared<RPKI>();

    // Create test routers
    r1 = std::make_shared<Router>(1, 1, std::make_unique<BaseProtocol>(), rpki);
    r2 = std::make_shared<Router>(2, 1, std::make_unique<BaseProtocol>(), rpki);
    r3 = std::make_shared<Router>(3, 1, std::make_unique<BaseProtocol>(), rpki);
    r4 = std::make_shared<Router>(4, 1, std::make_unique<BaseProtocol>(), rpki);

    // Set up neighbor relationships
    Neighbor n1to2;
    n1to2.relation = Relation::Provider;
    n1to2.router = r2.get();
    r1->neighbors_[2] = n1to2;

    Neighbor n2to1;
    n2to1.relation = Relation::Customer;
    n2to1.router = r1.get();
    r2->neighbors_[1] = n2to1;

    Neighbor n2to3;
    n2to3.relation = Relation::Peer;
    n2to3.router = r3.get();
    r2->neighbors_[3] = n2to3;
  }
};

// Protocol Name Test
TEST_F(BaseProtocolTest, ProtocolName) {
  EXPECT_EQ(protocol->getProtocolName(), "Base Protocol Implementation");
}

// Route Acceptance Tests
TEST_F(BaseProtocolTest, AcceptValidRoute) {
  Route route;
  route.destination = r1.get();
  route.path = {r2.get(), r3.get(), r1.get()};

  EXPECT_TRUE(protocol->acceptRoute(route));
}

TEST_F(BaseProtocolTest, RejectCyclicRoute) {
  Route route;
  route.destination = r1.get();
  route.path = {r1.get(), r2.get(), r1.get()};

  EXPECT_FALSE(protocol->acceptRoute(route));
}

// Route Preference Tests
TEST_F(BaseProtocolTest, PreferShorterPath) {
  Route currentRoute;
  currentRoute.destination = r1.get();
  currentRoute.path = {r3.get(), r2.get(), r1.get()};

  Route newRoute;
  newRoute.destination = r1.get();
  newRoute.path = {r2.get(), r1.get()};

  EXPECT_TRUE(protocol->preferRoute(currentRoute, newRoute));
}

TEST_F(BaseProtocolTest, PreferCustomerRoute) {
  Route customerRoute;
  customerRoute.destination = r1.get();
  customerRoute.path = {r2.get(), r1.get()}; // r2 is customer of r1

  Route peerRoute;
  peerRoute.destination = r1.get();
  peerRoute.path = {r3.get(), r1.get()}; // r3 is peer of r2

  EXPECT_TRUE(protocol->preferRoute(peerRoute, customerRoute));
}

// Route Forwarding Tests
TEST_F(BaseProtocolTest, CustomerToProviderForwarding) {
  EXPECT_TRUE(protocol->canForwardTo(Relation::Customer, Relation::Provider));
}

TEST_F(BaseProtocolTest, ProviderToCustomerForwarding) {
  EXPECT_TRUE(protocol->canForwardTo(Relation::Provider, Relation::Customer));
}

TEST_F(BaseProtocolTest, PeerToPeerForwarding) {
  EXPECT_FALSE(protocol->canForwardTo(Relation::Peer, Relation::Peer));
}

TEST_F(BaseProtocolTest, CustomerToCustomerForwarding) {
  EXPECT_FALSE(protocol->canForwardTo(Relation::Customer, Relation::Customer));
}

// Complex Routing Scenarios
TEST_F(BaseProtocolTest, ComplexRoutingScenario) {
  // Create a more complex route
  Route route1;
  route1.destination = r4.get();
  route1.path = {r1.get(), r2.get(), r3.get(), r4.get()};

  Route route2;
  route2.destination = r4.get();
  route2.path = {r1.get(), r3.get(), r4.get()};

  // Test acceptance
  EXPECT_TRUE(protocol->acceptRoute(route1));
  EXPECT_TRUE(protocol->acceptRoute(route2));

  // Test preference
  EXPECT_TRUE(protocol->preferRoute(route1, route2));
}

// Edge Cases
TEST_F(BaseProtocolTest, EmptyRouteHandling) {
  Route emptyRoute;
  EXPECT_TRUE(protocol->acceptRoute(emptyRoute));
}

TEST_F(BaseProtocolTest, SingleNodeRoute) {
  Route route;
  route.destination = r1.get();
  route.path = {r1.get()};
  EXPECT_TRUE(protocol->acceptRoute(route));
}

TEST_F(BaseProtocolTest, DifferentDestinationRoutes) {
  Route route1;
  route1.destination = r1.get();
  route1.path = {r2.get(), r1.get()};

  Route route2;
  route2.destination = r2.get();
  route2.path = {r3.get(), r2.get()};

  // Should not prefer routes with different destinations
  EXPECT_FALSE(protocol->preferRoute(route1, route2));
}

// Factory Creation Test
TEST_F(BaseProtocolTest, ProtocolFactory) {
  auto &factory = ProtocolFactory::Instance();

  // Test creation for even AS number
  auto protocol1 = factory.CreateProtocol(2);
  EXPECT_NE(protocol1, nullptr);
  EXPECT_EQ(protocol1->getProtocolName(), "Base Protocol Implementation");

  // Test creation for odd AS number
  auto protocol2 = factory.CreateProtocol(3);
  EXPECT_NE(protocol2, nullptr);
  EXPECT_EQ(protocol2->getProtocolName(), "Base Protocol Implementation");
}

class TopologyBaseProtocolTest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;
  std::unique_ptr<Topology> topology;

  void SetUp() override {
    rpki = std::make_shared<RPKI>();
    // Empty topology (just target router)
    //
    std::vector<AsRel> relations = {
        // Tier 1 connections
        {1, 2, -1}, // Provider
        {1, 3, -1}, // Provider
        {1, 4, -1}, // Provider

        // Tier 2 interconnections
        {2, 3, 0},  // Peer
        {2, 5, -1}, // Provider
        {2, 6, -1}, // Provider
        {3, 6, -1}, // Provider
        {3, 7, -1}, // Provider
        {4, 7, -1}, // Provider
        {4, 8, -1}, // Provider

        // Tier 2 peer connections
        {5, 6, 0}, // Peer
        {6, 7, 0}, // Peer
        {7, 8, 0}, // Peer

        // Tier 3 connections
        {5, 9, -1},  // Provider
        {5, 10, -1}, // Provider
        {6, 11, -1}, // Provider
        {6, 12, -1}, // Provider
        {7, 13, -1}, // Provider
        {7, 14, -1}, // Provider
        {8, 15, -1}, // Provider
        {8, 16, -1}, // Provider

        // Additional connections
        {9, 17, -1}, // Provider
        {16, 18, -1} // Provider
    };

    topology = std::make_unique<Topology>(relations, rpki);

    // Set deployment flag without ASPA configuration
    topology->setDeploymentTrue();
  }
};

TEST_F(TopologyBaseProtocolTest, TargetRouterWithNoNeighbors) {
  // Create a new topology with only one router (no connections)
  std::vector<AsRel> singleRouterTopology = {};
  auto isolatedTopology =
      std::make_unique<Topology>(singleRouterTopology, rpki);

  // Set deployment flag without ASPA
  isolatedTopology->setDeploymentTrue();

  // Add single router manually since it has no relations
  auto isolatedRouter =
      std::make_shared<Router>(1, 1, std::make_unique<BaseProtocol>(), rpki);
  isolatedTopology->G->addNode(1, isolatedRouter);

  // Execute route discovery
  isolatedTopology->FindRoutesTo(isolatedRouter.get());

  // Verify the router has no routing table entries
  EXPECT_TRUE(isolatedRouter->routerTable.empty());

  // Verify no other routers exist in the topology
  EXPECT_EQ(isolatedTopology->G->nodes.size(), 1);
}

TEST_F(TopologyBaseProtocolTest, AMultipleRoutesWithVaryingPreferences) {
  // Using existing topology where:
  // AS2 and AS3 both connect to AS1 as providers (preferred paths)
  // AS2 and AS3 are peers with each other
  // Both AS2 and AS3 connect to AS6 as providers (destination)

  auto targetRouter = topology->GetRouter(1);    // Target
  auto preferredRouter = topology->GetRouter(2); // First provider path
  auto altRouter = topology->GetRouter(3);       // Alternative provider path
  auto destRouter = topology->GetRouter(6);      // Destination

  ASSERT_NE(targetRouter, nullptr);
  ASSERT_NE(preferredRouter, nullptr);
  ASSERT_NE(altRouter, nullptr);
  ASSERT_NE(destRouter, nullptr);

  // Execute route discovery
  topology->FindRoutesTo(targetRouter.get());

  // Both routers should have routes to target
  auto preferredRoute = preferredRouter->routerTable[targetRouter->ASNumber];
  auto altRoute = altRouter->routerTable[targetRouter->ASNumber];

  // Verify both routes exist
  ASSERT_NE(preferredRoute, nullptr);
  ASSERT_NE(altRoute, nullptr);

  // Verify route paths
  EXPECT_EQ(preferredRoute->path.size(), 2);
  EXPECT_EQ(preferredRoute->path[0], targetRouter.get());
  EXPECT_EQ(preferredRoute->path[1], preferredRouter.get());

  EXPECT_EQ(altRoute->path.size(), 2);
  EXPECT_EQ(altRoute->path[0], targetRouter.get());
  EXPECT_EQ(altRoute->path[1], altRouter.get());

  // Verify destination router received and selected route
  auto destRoute = destRouter->routerTable[targetRouter->ASNumber];
  ASSERT_NE(destRoute, nullptr);

  // Verify path length and first hop (immediate provider)
  EXPECT_EQ(destRoute->path.size(), 3);
  EXPECT_EQ(destRoute->path[0], targetRouter.get());
  // The next hop should be either AS2 or AS3 (both are valid provider paths)
  bool validNextHop = (destRoute->path[1] == preferredRouter.get() ||
                       destRoute->path[1] == altRouter.get());
  EXPECT_TRUE(validNextHop);
  EXPECT_EQ(destRoute->path[2], destRouter.get());
}

TEST_F(TopologyBaseProtocolTest, ComprehensiveRoutePathVerification) {
  // Using existing topology where:
  // Target (AS1) is connected to AS2, AS3, AS4
  // AS2 and AS3 are connected (peers)
  // Both AS2 and AS3 connect to AS6

  auto targetRouter = topology->GetRouter(1);
  auto routerA = topology->GetRouter(2);
  auto routerB = topology->GetRouter(3);
  auto routerC = topology->GetRouter(6);

  ASSERT_NE(targetRouter, nullptr);
  ASSERT_NE(routerA, nullptr);
  ASSERT_NE(routerB, nullptr);
  ASSERT_NE(routerC, nullptr);

  // Execute route discovery
  topology->FindRoutesTo(targetRouter.get());

  // Verify direct routes from RouterA and RouterB to target
  auto routeFromA = routerA->routerTable[targetRouter->ASNumber];
  auto routeFromB = routerB->routerTable[targetRouter->ASNumber];

  // Verify direct routes exist and have correct paths
  ASSERT_NE(routeFromA, nullptr) << "RouterA should have a route to target";
  EXPECT_EQ(routeFromA->path.size(), 2);
  EXPECT_EQ(routeFromA->path[0], targetRouter.get());
  EXPECT_EQ(routeFromA->path[1], routerA.get());

  ASSERT_NE(routeFromB, nullptr) << "RouterB should have a route to target";
  EXPECT_EQ(routeFromB->path.size(), 2);
  EXPECT_EQ(routeFromB->path[0], targetRouter.get());
  EXPECT_EQ(routeFromB->path[1], routerB.get());

  // Verify RouterC's routes to target
  auto routeFromC = routerC->routerTable[targetRouter->ASNumber];
  ASSERT_NE(routeFromC, nullptr) << "RouterC should have a route to target";
  EXPECT_EQ(routeFromC->path.size(), 3);
  EXPECT_EQ(routeFromC->path[0], targetRouter.get());

  // RouterC's path should go through either RouterA or RouterB
  bool validMiddleHop = (routeFromC->path[1] == routerA.get() ||
                         routeFromC->path[1] == routerB.get());
  EXPECT_TRUE(validMiddleHop)
      << "RouterC's path should go through either RouterA or RouterB";
  EXPECT_EQ(routeFromC->path[2], routerC.get());

  // Verify routing table sizes
  EXPECT_EQ(routerA->routerTable.size(), 1) << "RouterA should have one route";
  EXPECT_EQ(routerB->routerTable.size(), 1) << "RouterB should have one route";
  EXPECT_EQ(routerC->routerTable.size(), 1) << "RouterC should have one route";
}

TEST_F(TopologyBaseProtocolTest, LargeScaleRouteDiscovery) {
  // Using existing topology which has comprehensive router setup with Tiers 1-3
  auto targetRouter = topology->GetRouter(1); // Tier 1 router
  ASSERT_NE(targetRouter, nullptr);

  // Record start time for performance measurement
  auto startTime = std::chrono::high_resolution_clock::now();

  // Execute route discovery
  topology->FindRoutesTo(targetRouter.get());

  // Record end time and calculate duration
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  // Maximum acceptable duration (e.g., 1000ms)
  const int MAX_DURATION_MS = 1000;
  EXPECT_LT(duration.count(), MAX_DURATION_MS)
      << "Route discovery took too long: " << duration.count() << "ms";

  // Verify routes for all routers in the topology
  for (const auto &[asNum, router] : topology->G->nodes) {
    if (asNum == targetRouter->ASNumber)
      continue; // Skip target router itself

    auto route = router->routerTable[targetRouter->ASNumber];

    // Verify route exists
    ASSERT_NE(route, nullptr)
        << "Router AS" << asNum << " should have a route to target";

    // Verify path starts with target and ends with current router
    EXPECT_EQ(route->path[0], targetRouter.get())
        << "Path for AS" << asNum << " should start with target router";
    EXPECT_EQ(route->path.back(), router.get())
        << "Path for AS" << asNum << " should end with current router";

    // Verify no cycles in path
    EXPECT_FALSE(route->ContainsCycle())
        << "Route from AS" << asNum << " contains a cycle";

    // Verify path length is reasonable (should not exceed number of routers)
    EXPECT_LE(route->path.size(), topology->G->nodes.size())
        << "Path length for AS" << asNum << " is unexpectedly long";

    // Verify each hop in path is valid
    for (size_t i = 0; i < route->path.size() - 1; i++) {
      Router *currentHop = route->path[i];
      Router *nextHop = route->path[i + 1];

      // Verify neighboring relationship exists
      Relation rel = currentHop->GetRelation(nextHop);
      EXPECT_NE(rel, Relation::Unknown)
          << "Invalid hop between AS" << currentHop->ASNumber << " and AS"
          << nextHop->ASNumber;
    }
  }

  // Verify route count for each tier
  auto tier2Routers = topology->GetTierTwo();
  auto tier3Routers = topology->GetTierThree();

  // All Tier 2 routers should have routes
  for (const auto &router : tier2Routers) {
    EXPECT_NE(router->routerTable[targetRouter->ASNumber], nullptr)
        << "Tier 2 router AS" << router->ASNumber << " missing route to target";
  }

  // All Tier 3 routers should have routes
  for (const auto &router : tier3Routers) {
    EXPECT_NE(router->routerTable[targetRouter->ASNumber], nullptr)
        << "Tier 3 router AS" << router->ASNumber << " missing route to target";
  }

  // Verify total number of discovered routes
  size_t totalRoutes = 0;
  for (const auto &[_, router] : topology->G->nodes) {
    if (router->routerTable.find(targetRouter->ASNumber) !=
        router->routerTable.end()) {
      totalRoutes++;
    }
  }

  // Expected routes = total nodes - 1 (excluding target)
  EXPECT_EQ(totalRoutes, topology->G->nodes.size() - 1)
      << "Unexpected number of total routes discovered";
}

TEST_F(TopologyBaseProtocolTest, RoutePathLengthConstraints) {
  // Using existing topology, focusing on path:
  // AS1 -> AS2 -> AS5 -> AS9 -> AS17
  // This gives us a chain of 5 routers to test path length constraints

  auto targetRouter = topology->GetRouter(1);
  auto routerA = topology->GetRouter(2);
  auto routerB = topology->GetRouter(5);
  auto routerC = topology->GetRouter(9);
  auto routerD = topology->GetRouter(17);

  ASSERT_NE(targetRouter, nullptr);
  ASSERT_NE(routerA, nullptr);
  ASSERT_NE(routerB, nullptr);
  ASSERT_NE(routerC, nullptr);
  ASSERT_NE(routerD, nullptr);

  // Execute route discovery
  topology->FindRoutesTo(targetRouter.get());

  // Check first hop route (target -> A)
  auto routeFromA = routerA->routerTable[targetRouter->ASNumber];
  ASSERT_NE(routeFromA, nullptr);
  EXPECT_EQ(routeFromA->path.size(), 2);
  EXPECT_EQ(routeFromA->path[0], targetRouter.get());
  EXPECT_EQ(routeFromA->path[1], routerA.get());

  // Check two-hop route (target -> A -> B)
  auto routeFromB = routerB->routerTable[targetRouter->ASNumber];
  ASSERT_NE(routeFromB, nullptr);
  EXPECT_EQ(routeFromB->path.size(), 3);
  EXPECT_EQ(routeFromB->path[0], targetRouter.get());
  EXPECT_EQ(routeFromB->path[1], routerA.get());
  EXPECT_EQ(routeFromB->path[2], routerB.get());

  // Check three-hop route (target -> A -> B -> C)
  auto routeFromC = routerC->routerTable[targetRouter->ASNumber];
  ASSERT_NE(routeFromC, nullptr);
  EXPECT_EQ(routeFromC->path.size(), 4);
  EXPECT_EQ(routeFromC->path[0], targetRouter.get());
  EXPECT_EQ(routeFromC->path[1], routerA.get());
  EXPECT_EQ(routeFromC->path[2], routerB.get());
  EXPECT_EQ(routeFromC->path[3], routerC.get());

  // Check four-hop route (target -> A -> B -> C -> D)
  auto routeFromD = routerD->routerTable[targetRouter->ASNumber];
  ASSERT_NE(routeFromD, nullptr);
  EXPECT_EQ(routeFromD->path.size(), 5);
  EXPECT_EQ(routeFromD->path[0], targetRouter.get());
  EXPECT_EQ(routeFromD->path[1], routerA.get());
  EXPECT_EQ(routeFromD->path[2], routerB.get());
  EXPECT_EQ(routeFromD->path[3], routerC.get());
  EXPECT_EQ(routeFromD->path[4], routerD.get());

  // Verify path lengths are monotonically increasing
  size_t previousLength = 0;
  for (const auto &router : {routerA, routerB, routerC, routerD}) {
    auto route = router->routerTable[targetRouter->ASNumber];
    ASSERT_NE(route, nullptr);
    EXPECT_GT(route->path.size(), previousLength);
    previousLength = route->path.size();
  }

  // Verify no cycles in any path
  for (const auto &router : {routerA, routerB, routerC, routerD}) {
    auto route = router->routerTable[targetRouter->ASNumber];
    EXPECT_FALSE(route->ContainsCycle())
        << "Cycle detected in path to AS" << router->ASNumber;
  }
}
