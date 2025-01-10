#pragma once
#ifndef OPERATIONS_HPP
#define OPERATIONS_HPP

#include <string>
#include <vector>

template <typename node> class graph;
template <typename node> class operations {
private:
  const graph<node> &g;

public:
  explicit operations(const graph<node> &graph) : g(graph) {}
  ~operations() = default;
  std::vector<node> bfs(const std::string &start) const;
  std::vector<node> dfs(const std::string &start) const;
  std::vector<node> fbfs(const std::string &start) const;
  std::vector<node> fdfs(const std::string &start) const;
};

#endif // OPERATIONS_HPP
