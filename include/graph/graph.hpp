#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

struct Edge {
  int targetNodeId;
  double weight;

  Edge(int targetNodeId, double weight)
      : targetNodeId(targetNodeId), weight(weight) {}
};

template <typename NodeType> class Graph {
public:
  std::unordered_map<int, NodeType> nodes;

  void addNode(int nodeId, const NodeType &nodeData) {
    if (nodes.find(nodeId) != nodes.end()) {
      throw std::invalid_argument("Node with ID " + std::to_string(nodeId) +
                                  " already exists in the graph");
    }
    nodes[nodeId] = nodeData;
    adjacencyList[nodeId] = std::vector<Edge>();
  }

  void addEdge(int sourceNodeId, int targetNodeId, double weight) {
    if (nodes.find(sourceNodeId) == nodes.end() ||
        nodes.find(targetNodeId) == nodes.end()) {
      throw std::invalid_argument(
          "Both nodes must exist in the graph to add an edge.");
    }
    adjacencyList[sourceNodeId].emplace_back(targetNodeId, weight);
  }

  void printGraph() const {
    for (const auto &node : nodes) {
      int nodeId = node.first;
      const NodeType &router = node.second;

      std::cout << "Node " << nodeId << " (ASNumber: " << router->ASNumber
                << ", Tier: " << router->Tier << ")" << std::endl;

      if (adjacencyList.at(nodeId).empty()) {
        std::cout << "  -> No neighbors" << std::endl;
      } else {
        for (const Edge &edge : adjacencyList.at(nodeId)) {
          const NodeType &neighborRouter = nodes.at(edge.targetNodeId);
          std::cout << "  -> Node " << edge.targetNodeId
                    << " (ASNumber: " << neighborRouter->ASNumber
                    << ", Tier: " << neighborRouter->Tier
                    << ", Weight: " << edge.weight << ")" << std::endl;
        }
      }
    }
  }

  const std::vector<Edge> &getNeighbors(int nodeId) const {
    if (adjacencyList.find(nodeId) == adjacencyList.end()) {
      throw std::invalid_argument("Node does not exist in the graph.");
    }
    return adjacencyList.at(nodeId);
  }

  // Get node
  const NodeType &getNode(int nodeId) {
    if (nodes.find(nodeId) == nodes.end()) {
      throw std::invalid_argument("Node does not exist in the graph.");
    }
    return nodes.at(nodeId);
  }

  bool hasCycle() {
    std::unordered_map<int, bool> visited;
    std::unordered_map<int, bool> recursionStack;

    for (const auto &node : nodes) {
      visited[node.first] = false;
      recursionStack[node.first] = false;
    }

    for (const auto &node : nodes) {
      if (!visited[node.first]) {
        if (hasCycleUtil(node.first, visited, recursionStack)) {
          return true;
        }
      }
    }
    return false;
  }

private:
  bool hasCycleUtil(int nodeId, std::unordered_map<int, bool> &visited,
                    std::unordered_map<int, bool> &recursionStack) {
    visited[nodeId] = true;
    recursionStack[nodeId] = true;

    for (const Edge &edge : adjacencyList[nodeId]) {
      if (!visited[edge.targetNodeId]) {
        if (hasCycleUtil(edge.targetNodeId, visited, recursionStack)) {
          return true;
        }
      } else if (recursionStack[edge.targetNodeId]) {
        return true;
      }
    }

    recursionStack[nodeId] = false;
    return false;
  }

  std::unordered_map<int, std::vector<Edge>> adjacencyList;
};
