// thread_pool.hpp
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
  explicit ThreadPool(size_t num_threads);
  ~ThreadPool();

  // Delete copy constructor and assignment operator
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // Add task to the thread pool
  template <class F, class... Args>
  auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

  // Control methods
  void pause();
  void resume();
  void stop();
  bool is_paused() const { return paused_; }
  size_t size() const { return workers_.size(); }
  size_t queue_size() const;

private:
  // Worker thread function
  void worker_loop();

  // Need to keep track of threads so we can join them
  std::vector<std::thread> workers_;

  // Task queue
  std::queue<std::function<void()>> tasks_;

  // Synchronization
  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::condition_variable pause_condition_;

  // Control flags
  std::atomic<bool> stop_;
  std::atomic<bool> paused_;
};

// Template implementation must be in header
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Don't allow enqueueing after stopping the pool
    if (stop_) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }

    tasks_.emplace([task]() { (*task)(); });
  }
  condition_.notify_one();
  return res;
}
