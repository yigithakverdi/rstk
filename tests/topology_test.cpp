#include "engine/rpki/rpki.hpp"
#include "engine/topology/topology.hpp"
#include "plugins/base/base.hpp"
#include "router/route.hpp"
#include "router/router.hpp"
#include <gtest/gtest.h>
#include <memory>

class TopologyTest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;
  std::vector<AsRel> testRelations;
  std::unique_ptr<Topology> topology;

  void SetUp() override {
    rpki = std::make_shared<RPKI>();

    // Build relationships according to the provided topology
    testRelations = {
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

    topology = std::make_unique<Topology>(testRelations, rpki);
  }
};

TEST_F(TopologyTest, RouterRetrieval) {
  auto router1 = topology->GetRouter(1);
  auto router2 = topology->GetRouter(2);
  auto nonExistentRouter = topology->GetRouter(999);

  ASSERT_NE(router1, nullptr);
  ASSERT_NE(router2, nullptr);
  EXPECT_EQ(nonExistentRouter, nullptr);

  EXPECT_EQ(router1->ASNumber, 1);
  EXPECT_EQ(router2->ASNumber, 2);
}

TEST_F(TopologyTest, NeighborRelations) {
  auto router1 = topology->GetRouter(1);
  auto router2 = topology->GetRouter(2);

  ASSERT_NE(router1, nullptr);
  ASSERT_NE(router2, nullptr);

  // Check if router1 has router2 as a customer
  EXPECT_EQ(router1->GetRelation(router2.get()), Relation::Customer);

  // Check inverse relation
  EXPECT_EQ(router2->GetRelation(router1.get()), Relation::Provider);
}

TEST_F(TopologyTest, TierRetrieval) {
  auto tierOne = topology->GetTierOne();
  auto tierTwo = topology->GetTierTwo();
  auto tierThree = topology->GetTierThree();

  // Initially all routers are tier 1 based on the constructor
  EXPECT_EQ(tierOne.size(), 1); // We have 4 routers in our test setup
  EXPECT_EQ(tierTwo.size(), 9);
  EXPECT_EQ(tierThree.size(), 8);
}

TEST_F(TopologyTest, RandomSampling) {
  // Test sampling with different sizes
  auto sample1 = topology->RandomSampleRouters(2);
  auto sample2 = topology->RandomSampleRouters(4);
  auto sample3 = topology->RandomSampleRouters(6);

  // With 18 total routers:
  // Sampling sizes 2, 4, and 6 should return 2, 4, and 6 routers respectively
  EXPECT_EQ(sample1.size(), 2);
  EXPECT_EQ(sample2.size(), 4);
  EXPECT_EQ(sample3.size(), 6); // Updated expectation based on 18 routers
}

TEST_F(TopologyTest, CraftRoute) {
  auto router1 = topology->GetRouter(1);
  auto router2 = topology->GetRouter(2);

  ASSERT_NE(router1, nullptr);
  ASSERT_NE(router2, nullptr);

  // Test route with different hop counts
  Route *route0 = topology->CraftRoute(router1.get(), router2.get(), 0);
  Route *route1 = topology->CraftRoute(router1.get(), router2.get(), 1);
  Route *route2 = topology->CraftRoute(router1.get(), router2.get(), 2);

  ASSERT_NE(route0, nullptr);
  ASSERT_NE(route1, nullptr);
  ASSERT_NE(route2, nullptr);

  // Check path lengths
  EXPECT_EQ(route0->path.size(), 1);
  EXPECT_EQ(route1->path.size(), 2);
  EXPECT_EQ(route2->path.size(), 3);

  // Clean up
  delete route0;
  delete route1;
  delete route2;
}

TEST_F(TopologyTest, RandomSampleExcluding) {
  auto router1 = topology->GetRouter(1);
  ASSERT_NE(router1, nullptr);

  auto samples = topology->RandomSampleExcluding(2, router1.get());

  EXPECT_EQ(samples.size(), 2);
  for (const auto &router : samples) {
    EXPECT_NE(router, router1.get());
  }
}

TEST_F(TopologyTest, TierAssignment) {
  // AS6 should be Tier 2 (has both providers and customers)
  auto router6 = topology->GetRouter(6);
  ASSERT_NE(router6, nullptr);
  EXPECT_EQ(router6->Tier, 2);

  // AS1 should be Tier 1 (no providers, has customers)
  auto router1 = topology->GetRouter(1);
  ASSERT_NE(router1, nullptr);
  EXPECT_EQ(router1->Tier, 1);

  // AS4 should be Tier 2 (has both providers and customers)
  auto router4 = topology->GetRouter(4);
  ASSERT_NE(router4, nullptr);
  EXPECT_EQ(router4->Tier, 2);
}

TEST_F(TopologyTest, TierRelationships) {
  auto router1 = topology->GetRouter(1);
  auto router5 = topology->GetRouter(5);
  auto router2 = topology->GetRouter(2);

  ASSERT_NE(router1, nullptr);
  ASSERT_NE(router5, nullptr);
  ASSERT_NE(router2, nullptr);

  // Verify that AS1 is Tier 1 (no providers, has customers)
  EXPECT_EQ(router1->Tier, 1)
      << "AS1 should be Tier 1 (no providers, has customers)";

  // Verify that AS6 is Tier 2 (has both providers and customers)
  auto router6 = topology->GetRouter(6);
  ASSERT_NE(router6, nullptr);
  EXPECT_EQ(router6->Tier, 2)
      << "AS6 should be Tier 2 (has both providers and customers)";

  // Verify that AS4 is Tier 2 (has both providers and customers)
  auto router4 = topology->GetRouter(4);
  ASSERT_NE(router4, nullptr);
  EXPECT_EQ(router4->Tier, 2)
      << "AS4 should be Tier 2 (has both providers and customers)";

  // Now, verify provider-customer relationship between AS2 and AS5
  // AS2 is the provider of AS5, AS5 is the customer of AS2
  EXPECT_EQ(router2->GetRelation(router5.get()), Relation::Customer)
      << "AS2 should have AS5 as Customer";
  EXPECT_EQ(router5->GetRelation(router2.get()), Relation::Provider)
      << "AS5 should have AS2 as Provider";

  // Additionally, verify that AS1 has AS2, AS3, AS4 as customers
  auto router1Customers = router1->GetCustomers();
  EXPECT_FALSE(router1Customers.empty()) << "AS1 should have customers";

  // Check that AS1 has AS2 as a customer using std::find_if
  bool hasAS2AsCustomer =
      std::find_if(router1Customers.begin(), router1Customers.end(),
                   [&](const Neighbor &neighbor) {
                     return neighbor.router == router2.get();
                   }) != router1Customers.end();

  EXPECT_TRUE(hasAS2AsCustomer) << "AS1 should have AS2 as a customer";
}

TEST_F(TopologyTest, DynamicTierAssignmentVerification) {
  for (const auto &[id, router] : topology->G->nodes) {
    auto customers = router->GetCustomers();
    auto providers = router->GetProviders();

    if (providers.empty()) {
      // No providers = Tier 1
      EXPECT_EQ(router->Tier, 1)
          << "AS" << id << " should be Tier 1 (no providers)";
    } else if (customers.empty()) {
      // No customers = Tier 3
      EXPECT_EQ(router->Tier, 3)
          << "AS" << id << " should be Tier 3 (no customers)";
    } else {
      // Has both providers and customers = Tier 2
      EXPECT_EQ(router->Tier, 2)
          << "AS" << id << " should be Tier 2 (has providers and customers)";
    }
  }
}
