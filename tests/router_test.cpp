#include "engine/rpki/rpki.hpp"
#include "plugins/base/base.hpp"
#include <gtest/gtest.h>

class RouteTest : public ::testing::Test {
protected:
  std::shared_ptr<Router> r1;
  std::shared_ptr<Router> r2;
  std::shared_ptr<Router> r3;
  std::shared_ptr<RPKI> rpki;

  void SetUp() override {
    rpki = std::make_shared<RPKI>();
    r1 = std::make_shared<Router>(1, 1, std::make_unique<BaseProtocol>(), rpki);
    r2 = std::make_shared<Router>(2, 1, std::make_unique<BaseProtocol>(), rpki);
    r3 = std::make_shared<Router>(3, 1, std::make_unique<BaseProtocol>(), rpki);
  }
};

TEST_F(RouteTest, CycleDetection) {
  Route route;
  route.destination = r1.get();
  route.path = {r1.get(), r2.get(), r3.get()};
  EXPECT_FALSE(route.ContainsCycle());

  route.path.push_back(r1.get());
  EXPECT_TRUE(route.ContainsCycle());
}

TEST_F(RouteTest, CycleDetectionEmptyPath) {
  Route route;
  EXPECT_FALSE(route.ContainsCycle());
}

TEST_F(RouteTest, CycleDetectionSingleRouter) {
  Route route;
  route.path = {r1.get()};
  EXPECT_FALSE(route.ContainsCycle());
}

TEST_F(RouteTest, PathReversalEmptyPath) {
  Route route;
  route.ReversePath();
  EXPECT_TRUE(route.path.empty());
}

TEST_F(RouteTest, PathReversalSingleRouter) {
  Route route;
  route.path = {r1.get()};
  route.ReversePath();
  EXPECT_EQ(route.path.size(), 1);
  EXPECT_EQ(route.path[0], r1.get());
}

TEST_F(RouteTest, PathReversalComprehensive) {
  Route route;
  route.path = {r1.get(), r2.get(), r3.get(), r2.get()};
  route.ReversePath();
  std::vector<Router *> expected = {r2.get(), r3.get(), r2.get(), r1.get()};
  EXPECT_EQ(route.path, expected);
}

TEST_F(RouteTest, DefaultFlags) {
  Route route;
  EXPECT_FALSE(route.authenticated);
  EXPECT_FALSE(route.originValid);
  EXPECT_FALSE(route.pathEndInvalid);
}

TEST_F(RouteTest, FlagManipulation) {
  Route route;
  route.authenticated = true;
  route.originValid = true;
  route.pathEndInvalid = true;
  EXPECT_TRUE(route.authenticated);
  EXPECT_TRUE(route.originValid);
  EXPECT_TRUE(route.pathEndInvalid);

  route.authenticated = false;
  EXPECT_FALSE(route.authenticated);
}

TEST_F(RouteTest, PathReversal) {
  Route route;
  route.path = {r1.get(), r2.get(), r3.get()};
  auto originalPath = route.path;

  route.ReversePath();
  EXPECT_EQ(route.path[0], originalPath[2]);
  EXPECT_EQ(route.path[2], originalPath[0]);
}

class RouterTest : public ::testing::Test {
protected:
  std::shared_ptr<Router> r1;
  std::shared_ptr<Router> r2;
  std::shared_ptr<Router> r3;
  std::shared_ptr<RPKI> rpki;

  void SetUp() override {
    rpki = std::make_shared<RPKI>();
    r1 = std::make_shared<Router>(1, 1, std::make_unique<BaseProtocol>(), rpki);
    r2 = std::make_shared<Router>(2, 1, std::make_unique<BaseProtocol>(), rpki);
    r3 = std::make_shared<Router>(3, 1, std::make_unique<BaseProtocol>(), rpki);
  }
};

TEST_F(RouterTest, NeighborRelations) {
  // Set up provider-customer relationship
  Neighbor n;
  n.relation = Relation::Provider;
  n.router = r2.get();
  r1->neighbors_[2] = n;

  EXPECT_EQ(r1->GetRelation(r2.get()), Relation::Provider);
  EXPECT_EQ(r1->GetProviders().size(), 1);
  EXPECT_EQ(r1->GetCustomers().size(), 0);
}

TEST_F(RouterTest, PeerRelations) {
  Neighbor n;
  n.relation = Relation::Peer;
  n.router = r2.get();
  r1->neighbors_[2] = n;

  EXPECT_EQ(r1->GetRelation(r2.get()), Relation::Peer);
  EXPECT_EQ(r1->GetPeers().size(), 1);
  EXPECT_EQ(r1->GetProviders().size(), 0);
}

TEST_F(RouterTest, MultipleNeighborRelations) {
  Neighbor provider;
  provider.relation = Relation::Provider;
  provider.router = r2.get();

  Neighbor customer;
  customer.relation = Relation::Customer;
  customer.router = r3.get();

  r1->neighbors_[2] = provider;
  r1->neighbors_[3] = customer;

  EXPECT_EQ(r1->GetProviders().size(), 1);
  EXPECT_EQ(r1->GetCustomers().size(), 1);
  EXPECT_EQ(r1->GetPeers().size(), 0);
}

TEST_F(RouterTest, OriginateAndForwardRoute) {
  Route *origRoute = r1->OriginateRoute(r2.get());
  ASSERT_NE(origRoute, nullptr);
  EXPECT_EQ(origRoute->destination,
            r1.get()); // Changed: destination should be the originating router

  Route *forwardedRoute = r1->ForwardRoute(origRoute, r3.get());
  ASSERT_NE(forwardedRoute, nullptr);
  EXPECT_EQ(forwardedRoute->destination,
            r1.get()); // Changed: destination remains the same

  // Path verification remains the same
  std::vector<Router *> expectedPath = {r1.get(), r2.get(), r3.get()};
  EXPECT_EQ(forwardedRoute->path, expectedPath);
}

TEST_F(RouterTest, LearnRouteToSelf) {
  // Create a route where the destination is the learning router
  Route route;
  route.destination = r1.get();
  route.path = {r2.get(), r1.get()};

  // Learning a route to itself should return empty vector
  std::vector<Router *> forwardTo = r1->LearnRoute(&route);
  EXPECT_TRUE(forwardTo.empty());

  // Route should not be added to routing table
  EXPECT_EQ(r1->routerTable.find(r1->ASNumber), r1->routerTable.end());
}

TEST_F(RouterTest, LearnValidRoute) {
  // Setup: R2 is customer of R1
  Neighbor n1ToR2;
  n1ToR2.relation = Relation::Customer;
  n1ToR2.router = r2.get();
  r1->neighbors_[2] = n1ToR2;

  // Setup: R3 is also customer of R1
  Neighbor n1ToR3;
  n1ToR3.relation = Relation::Customer;
  n1ToR3.router = r3.get();
  r1->neighbors_[3] = n1ToR3;

  // Setup: Establish relationship between R2 and R3
  Neighbor n2ToR3;
  n2ToR3.relation = Relation::Customer;
  n2ToR3.router = r3.get();
  r2->neighbors_[3] = n2ToR3;

  // **New Setup: R4 is a peer of R1**
  std::shared_ptr<Router> r4 =
      std::make_shared<Router>(4, 1, std::make_unique<BaseProtocol>(), rpki);
  Neighbor n1ToR4;
  n1ToR4.relation = Relation::Peer;
  n1ToR4.router = r4.get();
  r1->neighbors_[4] = n1ToR4;

  // Establish bidirectional peer relationship
  Neighbor n4ToR1;
  n4ToR1.relation = Relation::Peer;
  n4ToR1.router = r1.get();
  r4->neighbors_[1] = n4ToR1;

  // Create route: R1 learning from R2 about path to R3
  Route route;
  route.destination = r3.get();
  route.path = {r1.get(), r2.get(), r3.get()}; // Include R1 in the path

  // Learn route
  std::vector<Router *> forwardTo = r1->LearnRoute(&route);

  // Verify route was learned
  ASSERT_NE(r1->routerTable.find(r3->ASNumber), r1->routerTable.end());

  // Since R2 is a customer advertising a route, R1 should be able to forward
  // this to its peer R4
  EXPECT_FALSE(forwardTo.empty());

  // Additionally, verify that R1 is forwarding to R4
  ASSERT_EQ(forwardTo.size(), 1);
  EXPECT_EQ(forwardTo[0], r4.get());
}

TEST_F(RouterTest, LearnBetterRoute) {
  // Create initial route with longer path
  Route *initialRoute = new Route();
  initialRoute->destination = r3.get();
  initialRoute->path = {r1.get(), r2.get(), r3.get()};
  r1->routerTable[r3->ASNumber] = initialRoute;

  // Create better route with shorter path
  Route betterRoute;
  betterRoute.destination = r3.get();
  betterRoute.path = {r1.get(), r3.get()};

  // Learn better route
  std::vector<Router *> forwardTo = r1->LearnRoute(&betterRoute);

  // Better route should replace existing route
  EXPECT_EQ(r1->routerTable[r3->ASNumber]->path.size(), 2);
}

TEST_F(RouterTest, LearnWorseRoute) {
  // Create initial good route
  Route *initialRoute = new Route();
  initialRoute->destination = r3.get();
  initialRoute->path = {r1.get(), r3.get()};
  r1->routerTable[r3->ASNumber] = initialRoute;

  // Create worse route with longer path
  Route worseRoute;
  worseRoute.destination = r3.get();
  worseRoute.path = {r1.get(), r2.get(), r3.get()};

  // Learn worse route
  std::vector<Router *> forwardTo = r1->LearnRoute(&worseRoute);

  // Original route should be kept
  EXPECT_EQ(r1->routerTable[r3->ASNumber]->path.size(), 2);
  EXPECT_TRUE(forwardTo.empty());
}

TEST_F(RouterTest, LearnRouteWithCycle) {
  // Create route with cycle
  Route route;
  route.destination = r3.get();
  route.path = {r1.get(), r2.get(), r1.get(), r3.get()};

  // Learn route with cycle
  std::vector<Router *> forwardTo = r1->LearnRoute(&route);

  // Route should be rejected
  EXPECT_TRUE(forwardTo.empty());
  EXPECT_EQ(r1->routerTable.find(r3->ASNumber), r1->routerTable.end());
}

TEST_F(RouterTest, LearnRoutePolicyRejection) {
  // Setup: R2 is peer of R1
  Neighbor n1ToR2;
  n1ToR2.relation = Relation::Peer;
  n1ToR2.router = r2.get();
  r1->neighbors_[2] = n1ToR2;

  // Establish bidirectional peer relationship
  Neighbor n2ToR1;
  n2ToR1.relation = Relation::Peer;
  n2ToR1.router = r1.get();
  r2->neighbors_[1] = n2ToR1;

  // Create route: R2 (peer) advertising path to R3
  Route route;
  route.destination = r3.get();
  route.path = {r2.get(), r3.get()};

  // Learn route from peer
  std::vector<Router *> forwardTo = r1->LearnRoute(&route);

  // Route should not be forwarded
  EXPECT_TRUE(forwardTo.empty());

  // Route should be accepted into routing table (valid for local use)
  // but not forwarded to anyone else
  EXPECT_NE(r1->routerTable.find(r3->ASNumber), r1->routerTable.end());
}
