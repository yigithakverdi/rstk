// logger.hpp
#pragma once
#include <iostream>

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  NONE // Use this to disable logging
};

class Logger {
public:
  static Logger &Instance() {
    static Logger instance;
    return instance;
  }

  void setLevel(LogLevel level) { currentLevel = level; }
  LogLevel getLevel() const { return currentLevel; }

  template <typename... Args> void debug(Args... args) {
    log(LogLevel::DEBUG, "[DEBUG] ", args...);
  }

  template <typename... Args> void info(Args... args) {
    log(LogLevel::INFO, "[INFO] ", args...);
  }

  template <typename... Args> void warning(Args... args) {
    log(LogLevel::WARNING, "[WARNING] ", args...);
  }

  template <typename... Args> void error(Args... args) {
    log(LogLevel::ERROR, "[ERROR] ", args...);
  }

private:
  Logger() : currentLevel(LogLevel::INFO) {}
  LogLevel currentLevel;

  template <typename T> void print(const T &msg) { std::cout << msg; }

  template <typename T, typename... Args>
  void print(const T &first, const Args &...args) {
    std::cout << first;
    print(args...);
  }

  template <typename... Args>
  void log(LogLevel level, const char *prefix, Args... args) {
    if (level >= currentLevel) {
      std::cout << prefix;
      print(args...);
      std::cout << std::endl;
    }
  }
};

// Convenience macro
#define LOG Logger::Instance()
