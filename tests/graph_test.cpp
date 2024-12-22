#include <gtest/gtest.h>

#include "engine/rpki/rpki.hpp"
#include "graph/graph.hpp"
#include "plugins/base/base.hpp"

class GraphTest : public ::testing::Test {
protected:
  Graph<std::shared_ptr<Router>> graph;
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

TEST_F(GraphTest, BasicGraphOperations) {
  graph.addNode(1, r1);
  graph.addNode(2, r2);
  EXPECT_NO_THROW(graph.addEdge(1, 2, 1.0));

  auto neighbors = graph.getNeighbors(1);
  EXPECT_EQ(neighbors.size(), 1);
  EXPECT_EQ(neighbors[0].targetNodeId, 2);
};

TEST_F(GraphTest, DuplicateNodeAddition) {
  graph.addNode(1, r1);
  EXPECT_THROW(graph.addNode(1, r2), std::invalid_argument);
}

TEST_F(GraphTest, AddEdgeNonExistentNodes) {
  graph.addNode(1, r1);
  EXPECT_THROW(graph.addEdge(1, 2, 1.0), std::invalid_argument);
  EXPECT_THROW(graph.addEdge(2, 3, 1.0), std::invalid_argument);
}

TEST_F(GraphTest, MultipleNeighbors) {
  graph.addNode(1, r1);
  graph.addNode(2, r2);
  graph.addNode(3, r3);
  graph.addEdge(1, 2, 1.0);
  graph.addEdge(1, 3, 2.0);

  auto neighbors = graph.getNeighbors(1);
  EXPECT_EQ(neighbors.size(), 2);
  EXPECT_EQ(neighbors[0].targetNodeId, 2);
  EXPECT_EQ(neighbors[1].targetNodeId, 3);
}

TEST_F(GraphTest, CycleDetection) {
  graph.addNode(1, r1);
  graph.addNode(2, r2);
  graph.addNode(3, r3);
  graph.addEdge(1, 2, 1.0);
  graph.addEdge(2, 3, 1.0);
  graph.addEdge(3, 1, 1.0);

  // Assuming you have a method to detect cycles
  EXPECT_TRUE(graph.hasCycle());
}
