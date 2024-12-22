#pragma once
#ifndef VERBOSITY_HPP
#define VERBOSITY_HPP

enum class VerbosityLevel {
  QUIET,   // Only show final results
  NORMAL,  // Show progress bar and key events
  VERBOSE, // Show detailed path analysis
  DEBUG    // Show everything
};

#endif // VERBOSITY_HPP
