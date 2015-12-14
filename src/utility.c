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

// Convert epoch into fuzzy text (Yesterday, Thursday, ...)
void epoch_to_fuzzy_text(char *buff, uint8_t size, int32_t epoch) {
  // convert epoch to localized tm struct
  tm *end_time = localtime((time_t*)(&epoch));
  time_t now = time(NULL);
  tm *now_time = localtime(&now);
  // convert struct to fuzzy time
  if ((abs(now_time->tm_yday - end_time->tm_yday) <= 1 && now_time->tm_year == end_time->tm_year) ||
      (abs(now_time->tm_yday - end_time->tm_yday) == DAY_IN_YEAR)) {
    if (time(NULL) > epoch) {
      //
    }
//    if (tm_time->tm_hour < 12) {
//      // Morning
//    } else if (tm_time->tm_hour < 17) {
//      // Afternoon
//    } else {
//      // Evening
//    }
//  } else if (epoch - time(NULL) < SEC_IN_DAY) {
//    if (tm_time->tm_hour < 12) {
//      //  Morning
//    } else if (tm_time->tm_hour < 17) {
//      // This Afternoon
//    } else {
//      // This Evening
//    }
  }
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
