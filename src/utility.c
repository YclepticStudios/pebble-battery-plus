// @file utility.c
// @brief File containing simple convenience functions.
//
// This file contains simple convenience functions that may be used
// in several different places. An example would be the "assert" function
// which terminates program execution based on the state of a pointer.
//
// @author Eric D. Phillips
// @date August 29, 2015
// @bugs No known bugs

#include "utility.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Convenience Functions
//

// Check pointer for null and assert if true
void assert(void *ptr, const char *file, int line) {
  if (ptr) {
    return;
  }
  APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid pointer: (%s:%d)", file, line);
  // assert
  void (*exit)(void) = NULL;
  exit();
}

// Malloc with built in pointer check
void *malloc_check(uint16_t size, const char *file, int line) {
  void *ptr = malloc(size);
  assert(ptr, file, line);
  return ptr;
}

// Get current epoch in milliseconds
uint64_t epoch(void) {
  return (uint64_t)time(NULL) * 1000 + (uint64_t)time_ms(NULL, NULL);
}

// Grab the current time and start the profiler count
uint64_t profile_time;
void profile_start(void) {
  profile_time = epoch();
}

// Detect how long the profiler has been running and print the result
void profile_print(void) {
  uint64_t duration = epoch() - profile_time;
  APP_LOG(APP_LOG_LEVEL_INFO, "Profiler: %d ms", (int)duration);
}
