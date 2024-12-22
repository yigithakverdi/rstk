#include "cli/ui.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

std::string getStatusSymbol(double value) {
  if (value >= 90)
    return " ✓ Success ";
  if (value >= 70)
    return " ⚠ Warning ";
  return " ✗ Failed  ";
}

ProgressBar::ProgressBar(int total, int width) : total_(total), width_(width), current_(0) {}

void ProgressBar::update(int current) {
  current_ = std::min(current, total_);
  float progress = static_cast<float>(current_) / total_;
  int pos = static_cast<int>(width_ * progress);

  std::cout << "\r[";
  for (int i = 0; i < width_; ++i) {
    if (i < pos)
      std::cout << "=";
    else if (i == pos)
      std::cout << ">";
    else
      std::cout << " ";
  }
  std::cout << "] " << static_cast<int>(progress * 100.0f) << "% (" << current_ << "/" << total_
            << ")" << std::flush;
}

void ProgressBar::finish() {
  update(total_);
  std::cout << std::endl;
}

MultiProgressDisplay::MultiProgressDisplay(size_t num_bars) : progress_bars_(num_bars) {}

MultiProgressDisplay::~MultiProgressDisplay() { stop(); }

void MultiProgressDisplay::initBar(size_t index, const std::string &description, size_t total) {
  if (index >= progress_bars_.size())
    return;

  auto &bar = progress_bars_[index];
  bar.description = description;
  bar.total = total;
  bar.current = 0;
  bar.start_time = std::chrono::steady_clock::now();
  bar.last_update = bar.start_time;
}

void MultiProgressDisplay::updateProgress(size_t index, size_t current) {
  if (index >= progress_bars_.size())
    return;

  auto &bar = progress_bars_[index];
  bar.current = current;
  bar.last_update = std::chrono::steady_clock::now();

  // Calculate ETA
  auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(bar.last_update - bar.start_time).count();

  if (current > 0 && elapsed > 0) {
    double rate = static_cast<double>(current) / elapsed;
    double remaining = bar.total - current;
    bar.estimated_time_remaining = remaining / rate;
  }
}

void MultiProgressDisplay::start() {
  running_ = true;
  refresh_thread_ = std::make_unique<std::thread>([this]() {
    while (running_) {
      refresh();
      std::this_thread::sleep_for(REFRESH_RATE);
    }
  });
}

void MultiProgressDisplay::stop() {
  running_ = false;
  if (refresh_thread_ && refresh_thread_->joinable()) {
    refresh_thread_->join();
  }
  // Clear the progress display
  std::lock_guard<std::mutex> lock(display_mutex_);
  clearLines(progress_bars_.size());
}

void MultiProgressDisplay::refresh() {
  std::lock_guard<std::mutex> lock(display_mutex_);
  displayBars();
}

void MultiProgressDisplay::clearLines(int count) {
  for (int i = 0; i < count; ++i) {
    std::cout << "\033[2K"; // Clear line
    if (i < count - 1) {
      std::cout << "\033[1A"; // Move up
    }
  }
  std::cout << "\r" << std::flush;
}

void MultiProgressDisplay::moveCursorUp(int lines) { std::cout << "\033[" << lines << "A"; }

std::string MultiProgressDisplay::formatTime(double seconds) const {
  int hours = static_cast<int>(seconds) / 3600;
  int minutes = (static_cast<int>(seconds) % 3600) / 60;
  int secs = static_cast<int>(seconds) % 60;

  std::stringstream ss;
  if (hours > 0) {
    ss << hours << "h ";
  }
  if (minutes > 0 || hours > 0) {
    ss << minutes << "m ";
  }
  ss << secs << "s";
  return ss.str();
}

std::string MultiProgressDisplay::createBar(double percentage) const {
  std::string bar;
  int pos = static_cast<int>(BAR_WIDTH * percentage);

  for (int i = 0; i < BAR_WIDTH; ++i) {
    if (i < pos)
      bar += "█";
    else if (i == pos)
      bar += "▓";
    else
      bar += "░";
  }
  return bar;
}

std::string MultiProgressDisplay::formatProgressBar(const ProgressData &data) const {
  double percentage = static_cast<double>(data.current) / data.total;
  std::stringstream ss;

  ss << data.description << ": [" << createBar(percentage) << "] " << std::setw(3)
     << static_cast<int>(percentage * 100) << "% "
     << "(" << data.current << "/" << data.total << ") "
     << "ETA: " << formatTime(data.estimated_time_remaining);

  return ss.str();
}

void MultiProgressDisplay::displayBars() {
  // Clear previous display
  clearLines(progress_bars_.size());

  // Display each progress bar
  for (const auto &bar : progress_bars_) {
    std::cout << formatProgressBar(bar) << "\n";
  }
  std::cout << std::flush;
}
