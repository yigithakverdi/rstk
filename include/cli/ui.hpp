#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

/**
 * A simple progress bar for long-running tasks.
 */
class ProgressBar {
public:
  /**
   * @param total The total number of steps/units
   * @param width The width in characters for the displayed progress bar
   */
  ProgressBar(int total, int width = 50);

  /**
   * Update the progress bar display.
   * @param current The current progress value (0 to total).
   */
  void update(int current);

  /**
   * Complete the progress bar (sets it to 100%).
   */
  void finish();

private:
  int total_;
  int width_;
  int current_;
};

/**
 * A simple spinner to indicate an ongoing process with no known end time.
 */
class Spinner {
public:
  Spinner() : running_(false), current_frame_(0), frames_{"|", "/", "-", "\\"} {}

  void start() {
    running_ = true;
    spinner_thread_ = std::thread([this]() {
      while (running_) {
        std::cout << "\r" << frames_[current_frame_] << " Working..." << std::flush;
        current_frame_ = (current_frame_ + 1) % frames_.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
  }

  void stop() {
    running_ = false;
    if (spinner_thread_.joinable())
      spinner_thread_.join();
    // Clear the spinner line
    std::cout << "\r" << " " << "           " << "\r" << std::flush;
  }

  // Optional: Prevent copying
  Spinner(const Spinner &) = delete;
  Spinner &operator=(const Spinner &) = delete;

private:
  std::atomic<bool> running_;
  int current_frame_;
  std::vector<std::string> frames_;
  std::thread spinner_thread_;
};

class MultiProgressDisplay {
public:
  struct ProgressData {
    std::atomic<size_t> current{0};
    std::atomic<size_t> total{0};
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update;
    std::string description;
    double estimated_time_remaining{0.0};
  };

  MultiProgressDisplay(size_t num_bars);
  ~MultiProgressDisplay();

  // Initialize a specific progress bar
  void initBar(size_t index, const std::string &description, size_t total);

  // Update progress for a specific bar
  void updateProgress(size_t index, size_t current);

  // Start/stop the display refresh thread
  void start();
  void stop();

  // Force a refresh of the display
  void refresh();

private:
  static constexpr int BAR_WIDTH = 30;
  static constexpr auto REFRESH_RATE = std::chrono::milliseconds(100);

  // Thread-safe console output
  void clearLines(int count);
  void moveCursorUp(int lines);
  void displayBars();

  std::string formatTime(double seconds) const;
  std::string createBar(double percentage) const;
  std::string formatProgressBar(const ProgressData &data) const;

  std::vector<ProgressData> progress_bars_;
  std::mutex display_mutex_;
  std::atomic<bool> running_{false};
  std::unique_ptr<std::thread> refresh_thread_;
};

// Add this helper class
class ProgressDisplay {
public:

  void updateMatrixProgress(double progress, double obj_pct, double pol_pct) {
    // Save cursor position
    std::cout << "\033[s";

    // Move to start of line and clear
    std::cout << "\033[G\033[K";

    // Draw matrix progress bar
    std::cout << "Matrix Progress [";
    int width = 30;
    int pos = static_cast<int>(width * (progress / 100.0));

    for (int i = 0; i < width; ++i) {
      if (i < pos)
        std::cout << "█";
      else if (i == pos)
        std::cout << "▓";
      else
        std::cout << "░";
    }

    std::cout << "] " << std::fixed << std::setprecision(1) << progress << "% "
              << "(Object: " << obj_pct << "%, Policy: " << pol_pct << "%)";

    // Restore cursor position
    std::cout << "\033[u" << std::flush;
  }

  void updateTrialProgress(double progress, double result, int victim_as, int attacker_as) {
    std::cout << "\033[s";       // Save position
    std::cout << "\033[1B";      // Move down one line
    std::cout << "\033[G\033[K"; // Clear line

    // Draw trial progress
    std::cout << "Trial Progress  [";
    int width = 30;
    int pos = static_cast<int>(width * (progress / 100.0));

    for (int i = 0; i < width; ++i) {
      if (i < pos)
        std::cout << "█";
      else if (i == pos)
        std::cout << "▓";
      else
        std::cout << "░";
    }

    std::cout << "] " << std::fixed << std::setprecision(1) << progress << "% "
              << "| Success Rate: " << result * 100 << "% "
              << "| Victim AS" << victim_as << " / Attacker (Leaking) AS" << attacker_as;

    std::cout << "\033[u" << std::flush; // Restore position
  }
};
