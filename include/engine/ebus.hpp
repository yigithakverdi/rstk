#pragma once
#ifndef EBUS_HPP
#define EBUS_HPP

#include <any>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <iostream>

enum class eventPriority { CRITICAL = 0, HIGH = 1, NORMAL = 2, LOW = 3 };

class IEvent {
public:
  virtual ~IEvent() = default;
  virtual std::type_index getType() const = 0;
  virtual std::string getName() const = 0;
  virtual uint64_t getTimestamp() const = 0;
};

class BaseEvent : public IEvent {
public:
  BaseEvent() : timestamp_(std::chrono::system_clock::now().time_since_epoch().count()) {}
  uint64_t getTimestamp() const override { return timestamp_; }
  std::type_index getType() const override { return std::type_index(typeid(BaseEvent)); }
  std::string getName() const override { return "init"; }

  template <typename T> void setData(const std::string &key, T value) { payload_[key] = value; }
  template <typename T> T getPayload() const { return std::any_cast<T>(payload_); }
  template <typename T> T getData(const std::string &key) const {
    auto it = payload_.find(key);
    if (it != payload_.end()) {
      return std::any_cast<T>(it->second);
    }
    throw std::runtime_error("Key not found: " + key);
  }

private:
  uint64_t timestamp_;
  std::unordered_map<std::string, std::any> payload_;
};

class ebus {
public:
  using eventCallBack = std::function<void(std::shared_ptr<IEvent>)>;
  using eventFilter = std::function<bool(std::shared_ptr<IEvent>)>;

  ebus();
  ~ebus();

  template <typename T>
  uint64_t subscribe(eventCallBack callback, eventPriority priority = eventPriority::NORMAL,
                     eventFilter filter = nullptr) {

    static_assert(std::is_base_of<IEvent, T>::value, "T must inherit from IEvent");
    std::lock_guard<std::mutex> lock(mMutex);
    auto sub = std::make_shared<subscription>(callback, priority, filter, std::type_index(typeid(T)));
    mSubscribers[std::type_index(typeid(T))].push_back(sub);

    return sub->id;
  }

  template <typename T> std::future<void> publish(std::shared_ptr<T> event, bool immediate = false) {
    static_assert(std::is_base_of<IEvent, T>::value, "T must inherit from IEvent");

    try {
      std::lock_guard<std::mutex> lock(mMutex);
      if (!mRunning) {
        throw std::runtime_error("Event bus not running");
      }

      if (immediate) {
        return std::async(std::launch::async, [this, event]() {
          this->dispatchImmediate(std::static_pointer_cast<IEvent>(event));
        });
      }

      mEventQueue.push(eventQueueItem{std::static_pointer_cast<IEvent>(event), std::type_index(typeid(T))});
      mCondition.notify_one();

      std::promise<void> promise;
      promise.set_value();
      return promise.get_future();
    } catch (const std::exception &e) {
      std::cerr << "Error publishing event: " << e.what() << std::endl;
      throw;
    }
  }

  template <typename T> void unsubscribe(uint64_t subscriptionId) {
    static_assert(std::is_base_of<IEvent, T>::value, "T must derive from IEvent");
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mSubscribers.find(std::type_index(typeid(T)));
    if (it != mSubscribers.end()) {
      auto &subscriptions = it->second;
      auto removeIt = std::remove_if(
          subscriptions.begin(), subscriptions.end(),
          [subscriptionId](const std::shared_ptr<subscription> &sub) { return sub->id == subscriptionId; });

      if (removeIt != subscriptions.end()) {
        subscriptions.erase(removeIt, subscriptions.end());
      }
    }
  }

  void start();
  void stop();
  void flush();

private:
  struct subscription {
    uint64_t id;
    eventCallBack callback;
    eventPriority priority;
    eventFilter filter;
    std::type_index type;

    static uint64_t nextID() {
      static std::atomic<uint64_t> id{0};
      return ++id;
    }

    subscription(eventCallBack cb, eventPriority p, eventFilter f, std::type_index t)
        : id(nextID()), callback(cb), priority(p), filter(f), type(t) {}
  };

  struct eventQueueItem {
    std::shared_ptr<IEvent> event;
    std::type_index type;

    bool operator<(const eventQueueItem &other) const {
      return event->getTimestamp() > other.event->getTimestamp();
    }
  };

  std::unordered_map<std::type_index, std::vector<std::shared_ptr<subscription>>> mSubscribers;
  std::priority_queue<eventQueueItem> mEventQueue;
  std::mutex mMutex;
  std::condition_variable mCondition;
  std::unique_ptr<std::thread> mDispatcherThread;
  bool mRunning;

  void dispatchLoop();
  void dispatchImmediate(std::shared_ptr<IEvent> event);
};

#endif // EBUS_HPP
