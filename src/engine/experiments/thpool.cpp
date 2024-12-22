// thread_pool.cpp
#include "engine/experiments/thpool.hpp"

ThreadPool::ThreadPool(size_t num_threads) : stop_(false), paused_(false) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this] { worker_loop(); });
  }
}

void ThreadPool::worker_loop() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);

      condition_.wait(lock, [this] { return stop_ || !tasks_.empty() || paused_; });

      if (paused_) {
        pause_condition_.wait(lock, [this] { return !paused_ || stop_; });
        if (stop_ && tasks_.empty()) {
          return;
        }
        continue;
      }

      if (stop_ && tasks_.empty()) {
        return;
      }

      if (!tasks_.empty()) {
        task = std::move(tasks_.front());
        tasks_.pop();
      }
    }

    if (task) {
      task();
    }
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }

  // Wake up all threads and join them
  condition_.notify_all();
  pause_condition_.notify_all();

  for (std::thread &worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::pause() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  paused_ = true;
  condition_.notify_all();
}

void ThreadPool::resume() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  paused_ = false;
  pause_condition_.notify_all();
}

void ThreadPool::stop() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
  pause_condition_.notify_all();
}

size_t ThreadPool::queue_size() const {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  return tasks_.size();
}
