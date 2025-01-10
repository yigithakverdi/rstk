#include "engine/ebus.hpp"
#include <iostream>

ebus::ebus() : mRunning(false) {}
void ebus::start() {
  std::lock_guard<std::mutex> lock(mMutex);
  if (!mRunning) {
    mRunning = true;
    try {
      mDispatcherThread = std::make_unique<std::thread>(&ebus::dispatchLoop, this);
    } catch (const std::exception &e) {
      std::cout << "!!!!!!!!!!!!!!!!" << std::endl;
      std::cout << "Error: " << e.what() << std::endl;
      mRunning = false;
      throw std::runtime_error(std::string("Failed to start dispatcher thread: ") + e.what());
    }
  }
}

ebus::~ebus() { stop(); }

void ebus::flush() {
  std::unique_lock<std::mutex> lock(mMutex);
  mCondition.wait(lock, [this]() { return mEventQueue.empty(); });
}

void ebus::stop() {
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mRunning) {
      return;
    }
    mRunning = false;
    mCondition.notify_all(); // Changed from notify_one to notify_all
  }

  std::thread *thread_to_join = nullptr;
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mDispatcherThread && mDispatcherThread->joinable()) {
      thread_to_join = mDispatcherThread.release();
    }
  }

  if (thread_to_join) {
    try {
      thread_to_join->join();
      delete thread_to_join;
    } catch (const std::system_error &e) {
      std::cerr << "Failed to join dispatcher thread: " << e.what() << std::endl;
    }
  }
}

void ebus::dispatchLoop() {
  while (mRunning) {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this]() { return !mEventQueue.empty() || !mRunning; });

    if (!mRunning) {
      break;
    }

    if (!mEventQueue.empty()) {
      eventQueueItem item = mEventQueue.top();
      mEventQueue.pop();
      lock.unlock();
      std::cout << "Processing event: " << item.event->getName() << std::endl;

      auto it = mSubscribers.find(item.type);
      if (it != mSubscribers.end()) {
        std::cout << "Found subscribers for event type" << std::endl;
        for (const auto &subscription : it->second) {
          if (!subscription->filter || subscription->filter(item.event)) {
            subscription->callback(item.event);
          }
        }
      }
    }
  }
}

void ebus::dispatchImmediate(std::shared_ptr<IEvent> event) {
  std::type_index type = event->getType();
  auto it = mSubscribers.find(type);
  if (it != mSubscribers.end()) {
    for (const auto &subscription : it->second) {
      if (!subscription->filter || subscription->filter(event)) {
        subscription->callback(event);
      }
    }
  }
}
