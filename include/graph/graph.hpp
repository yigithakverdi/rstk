#pragma once
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "graph/operations.hpp"
#include <array>
#include <bitset>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr size_t CHUNK_SIZE = 64;
constexpr size_t INITIAL_CHUNKS = 1024;
constexpr size_t mempool_SIZE = 1024 * 1024;

template <typename T> class mempool {
  static constexpr size_t BLOCK_SIZE = sizeof(T);
  std::array<std::byte, mempool_SIZE> storage;
  std::pmr::monotonic_buffer_resource resource;
  std::pmr::polymorphic_allocator<T> allocator;

public:
  mempool() : resource(storage.data(), storage.size()), allocator(&resource) {}
  T *allocate() { return allocator.allocate(1); }
  void deallocate(T *ptr) { allocator.deallocate(ptr, 1); }
};

template <typename T> class ecompressed {
  static constexpr size_t WEIGHT_BITS = 8;
  size_t target_id_hash; // Using full size_t to store string hash
  int8_t weight_compressed;

public:
  void set(const std::string &target, float weight) {
    target_id_hash = std::hash<std::string>{}(target);
    weight_compressed = static_cast<int8_t>(weight);
  }

  std::pair<size_t, float> get() const { return {target_id_hash, static_cast<float>(weight_compressed)}; }
};

template <typename T> class echunk {
public:
  std::array<ecompressed<T>, CHUNK_SIZE> edges;
  size_t used = 0;
  std::shared_ptr<echunk<T>> next = nullptr;
  std::bitset<CHUNK_SIZE> deleted; // Track deleted edges
};

template <typename T> class epool {
  mempool<echunk<T>> chunk_pool;
  std::vector<std::shared_ptr<echunk<T>>> chunks;
  std::shared_ptr<echunk<T>> curr = nullptr;
  std::vector<std::shared_ptr<echunk<T>>> free_chunks;

public:
  epool() {
    chunks.reserve(INITIAL_CHUNKS);
    for (size_t i = 0; i < INITIAL_CHUNKS; i++) {
      chunks.push_back(std::make_shared<echunk<T>>());
    }
    curr = chunks[0];
  }

  void addEdge(int source, int target, float weight) {
    if (curr->used >= CHUNK_SIZE) {
      if (!free_chunks.empty()) {
        curr = free_chunks.back();
        free_chunks.pop_back();
      } else if (!curr->next) {
        auto newChunk = std::make_shared<echunk<T>>();
        chunks.push_back(newChunk);
        curr->next = newChunk;
      }
      curr = curr->next;
    }
    curr->edges[curr->used++].set(target, weight);
  }

  void removeEdge(echunk<T> *chunk, size_t index) {
    chunk->deleted.set(index);
    if (chunk->deleted.all()) {
      free_chunks.push_back(std::shared_ptr<echunk<T>>(chunk));
    }
  }
};

template <typename node> class graph {
public:
  graph() = default;

  void addNode(const std::string &id, const node &n) {
    std::unique_lock lock(mGraph);
    if (nodes.find(id) != nodes.end()) {
      throw std::invalid_argument("Node already exists: " + id);
    }
    nodes[id] = n;
    edges[id] = std::make_shared<echunk<node>>();
  }

  bool hasNode(const std::string &id) const {
    std::shared_lock lock(mGraph);
    return nodes.find(id) != nodes.end();
  }

  std::optional<node> getNode(const std::string &id) const {
    std::shared_lock lock(mGraph);
    auto it = nodes.find(id);
    if (it != nodes.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void addEdge(const std::string &source, const std::string &target, float weight) {
    registerString(source);
    registerString(target);
    std::unique_lock lock(mGraph);
    if (nodes.find(source) == nodes.end() || nodes.find(target) == nodes.end()) {
      throw std::invalid_argument("Source or target node does not exist");
    }

    auto &sourceChunk = edges[source];
    if (sourceChunk->used >= CHUNK_SIZE) {
      auto newChunk = std::make_shared<echunk<node>>();
      newChunk->next = sourceChunk;
      edges[source] = newChunk;
      sourceChunk = newChunk;
    }

    sourceChunk->edges[sourceChunk->used++].set(target, weight);
  }

  std::vector<std::pair<std::string, float>> getNeighbors(const std::string &id) const {
    std::vector<std::pair<std::string, float>> neighbors;
    {
      std::shared_lock lock(mGraph);

      auto it = edges.find(id);
      if (it == edges.end()) {
        return neighbors;
      }

      auto chunk = it->second;
      while (chunk) {
        for (size_t i = 0; i < chunk->used; i++) {
          if (!chunk->deleted[i]) {
            auto [target_hash, weight] = chunk->edges[i].get();
            neighbors.emplace_back(getStringFromHash(target_hash), weight);
          }
        }
        chunk = chunk->next;
      }
    }

    return neighbors;
  }

  // The following functions provide batch operations for traversing and processing nodes and edges
  // in a thread-safe manner. These operations are designed to handle large-scale graph data efficiently
  // while ensuring consistency and synchronization through the use of shared locks.

  // Applies a given function to every node in the graph. This function is thread-safe and uses a
  // shared lock (`std::shared_lock`) to allow concurrent read access to the graph while preventing
  // modifications during traversal.
  //
  // @tparam Func A callable type (e.g., lambda, function pointer) that accepts two arguments:
  //              the node ID and a reference to the node object.
  // @param f The function to be applied to each node.
  template <typename Func> void forEachNode(Func f) const {
    std::shared_lock lock(mGraph);
    for (const auto &[id, n] : nodes) {
      f(id, n);
    }
  }

  // Applies a given function to every edge originating from a specified source node. This function
  // is thread-safe and uses a shared lock (`std::shared_lock`) to ensure consistent access to the
  // graph's edge data. It skips deleted edges and processes only valid edges.
  //
  // @tparam Func A callable type (e.g., lambda, function pointer) that accepts three arguments:
  //              the source node ID, the target node ID, and the edge weight.
  // @param source_id The ID of the source node whose outgoing edges are to be processed.
  // @param f The function to be applied to each edge.
  template <typename Func> void forEachEdge(int source_id, Func f) const {
    std::shared_lock lock(mGraph);
    auto edge_it = edges.find(source_id);
    if (edge_it == edges.end()) {
      return;
    }

    auto chunk = edge_it->second;
    while (chunk) {
      for (size_t i = 0; i < chunk->used; i++) {
        if (!chunk->deleted[i]) {
          auto [target, weight] = chunk->edges[i].get();
          f(source_id, target, weight);
        }
      }
      chunk = chunk->next;
    }
  }

  // Represents a subgraph, which is a subset of nodes extracted from a larger graph.
  // This class is designed to provide thread-safe access to the nodes within the subgraph
  // by acquiring locks on their corresponding mutexes during construction. This ensures
  // that no other threads can modify the nodes while the subgraph is in use.
  //
  // The subgraph is defined by a set of node IDs (`nodeSet`), and it maintains a reference
  // to the original graph for accessing node-specific data and synchronization mechanisms.
  using nodeSet = std::unordered_set<std::string>;
  class subgraph {
  public:
    subgraph(const nodeSet &nodes, graph &g) : nodes_(nodes), g_(g) {
      for (const std::string &id : nodes) {
        auto &mutex = g_.getNodeMutex(id);
        mutex.lock();
      }
    }

    ~subgraph() {
      for (const std::string &id : nodes_) {
        auto &mutex = g_.getNodeMutex(id);
        mutex.unlock();
      }
    }

    subgraph(const subgraph &) = delete;
    subgraph &operator=(const subgraph &) = delete;

  private:
    const nodeSet &nodes_;
    graph &g_;
  };

  // Creates and returns a `subgraph` object for a specified set of nodes, ensuring thread-safe
  // access to those nodes by acquiring their corresponding mutexes. This function acts as a
  // convenience wrapper for constructing a `subgraph` instance, simplifying the process of
  // locking a subset of nodes within the graph.
  //
  // @param nodes The set of node IDs to be included in the subgraph.
  // @return A `subgraph` object that holds locks on the specified nodes, providing exclusive
  //         access to them for the duration of the subgraph's lifetime.
  subgraph sublock(const nodeSet &nodes) { return subgraph(nodes, *this); }

  // This class provides a comprehensive set of graph operations, including traversal,
  // analysis, and manipulation functionalities. To accommodate different performance
  // and scalability requirements, many operations have two implementations:
  //
  // 1. Normal Implementation: Standard, single-threaded execution suitable for
  //    smaller graphs or scenarios where simplicity and determinism are prioritized.
  //    These functions are named conventionally (e.g., `bfs`, `dfs`).
  //
  // 2. Parallel Implementation: Optimized for performance, leveraging multi-threading
  //    or other parallel processing techniques to handle larger graphs or computationally
  //    intensive tasks. These functions are prefixed with 'f' (e.g., `fbfs`, `fdfs`) to
  //    indicate their parallel nature.
  //
  // The following functions are organized into logical groups based on their purpose:
  // - Traversal Algorithms: Breadth-First Search (BFS), Depth-First Search (DFS), etc.
  // - Pathfinding and Analysis: Shortest path, connectivity checks, etc.
  // - Graph Manipulation: Node/edge addition/removal, subgraph extraction, etc.
  // - Utility Functions: Helper functions for common tasks like locking, validation, etc.
  //
  // Each function is documented with its purpose, parameters, and behavior to ensure
  // clarity and ease of use. Developers should choose between normal and parallel
  // implementations based on their specific performance and concurrency requirements.
  const operations<node> &ops() const;

  // Getter methods
  const std::unordered_map<std::string, node> &getNodes() const { return nodes; }

private:
  // Core data structures
  std::unordered_map<std::string, node> nodes;
  std::unordered_map<std::string, std::shared_ptr<echunk<node>>> edges;
  std::unique_ptr<operations<node>> ops_; // Operations object used for graph algorithms
  epool<node> pool;

  // Add hash-string bidirectional mapping
  std::unordered_map<size_t, std::string> hash_to_string_map;
  std::unordered_map<std::string, size_t> string_to_hash_map;
  mutable std::shared_mutex mapping_mutex;

  size_t registerString(const std::string &str) {
    std::unique_lock lock(mapping_mutex);
    auto hash = std::hash<std::string>{}(str);
    hash_to_string_map[hash] = str;
    string_to_hash_map[str] = hash;
    return hash;
  }

  std::string getStringFromHash(size_t hash) const {
    std::shared_lock lock(mapping_mutex);
    auto it = hash_to_string_map.find(hash);
    if (it == hash_to_string_map.end()) {
      throw std::runtime_error("Hash not found in mapping");
    }
    return it->second;
  }

  // Synchronization
  mutable std::shared_mutex mGraph;                   // Protects the graph data structure
  std::unordered_map<std::string, std::mutex> mNodes; // Per-node node mutexes for fine-grained locking

  // Retrieves a node-specific mutex from the list of node mutexes based on the provided node ID.
  // This function ensures thread-safe access to the node's mutex by acquiring a unique lock
  // on the graph's mutex (`mGraph`) during the lookup process. The returned mutex can be used
  // to synchronize access to resources associated with the specified node.
  //
  // @param id The ID of the node whose mutex is to be retrieved.
  // @return A reference to the mutex associated with the specified node.
  std::mutex &getNodeMutex(const std::string &id) {
    std::unique_lock lock(mGraph);
    return mNodes[id];
  }
};

#endif // GRAPH_HPP
