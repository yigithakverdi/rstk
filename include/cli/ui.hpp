#pragma once
#include <string>
#include <vector>


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
  Spinner();

  /// Start the spinner animation
  void start();

  /// Stop the spinner animation
  void stop();

  /// Update the spinner frame (rotate symbols)
  void update();

  /// Check if the spinner is currently running
  bool isRunning() const;

private:
  bool running_;
  int current_frame_;
  std::vector<std::string> frames_;
};
