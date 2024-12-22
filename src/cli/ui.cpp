#include "cli/ui.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
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

Spinner::Spinner() : running_(false), current_frame_(0) { frames_ = {"|", "/", "-", "\\"}; }

void Spinner::start() { running_ = true; }

void Spinner::stop() {
  running_ = false;
  // Overwrite spinner output with blank spaces, then carriage return
  std::cout << "\r" << std::string(10, ' ') << "\r" << std::flush;
}

void Spinner::update() {
  if (!running_)
    return;

  std::cout << "\r" << frames_[current_frame_] << " Working..." << std::flush;
  current_frame_ = (current_frame_ + 1) % frames_.size();
}

bool Spinner::isRunning() const { return running_; }
