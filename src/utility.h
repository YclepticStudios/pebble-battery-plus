//! @file utility.h
//! @brief File containing simple convenience functions.
//!
//! This file contains simple convenience functions that may be used
//! in several different places. An example would be the "assert" function
//! which terminates program execution based on the state of a pointer.
//!
//! @author Eric D. Phillips
//! @date August 29, 2015
//! @bugs No known bugs

//! This file is included in both the main program and the worker, so it needs different includes
#pragma once
#ifdef PEBBLE_BACKGROUND_WORKER
#include <pebble_worker.h>
#else
#include <pebble.h>
#endif

// Time Constants
#define SEC_IN_MIN 60
#define SEC_IN_HR 3600
#define SEC_IN_DAY 86400
#define SEC_IN_WEEK 604800
#define MIN_IN_HR 60
#define MIN_IN_DAY 1440
#define DAY_IN_YEAR 365


//! Terminate program if null pointer
//! @param ptr The pointer to check for null
#define ASSERT(ptr) assert(ptr, __FILE__, __LINE__)

//! Malloc with failure check
//! @param size The size of the memory to allocate
#define MALLOC(size) malloc_check(size, __FILE__, __LINE__)

//! Terminate program if null pointer
//! @param ptr The pointer to check for null
//! @param file The name of the file it is called from
//! @param line The line number it is called from
void assert(void *ptr, const char *file, int line);

//! Malloc with failure check
//! @param size The size of the memory to allocate
//! @param file The name of the file it is called from
//! @param line The line number it is called from
void *malloc_check(uint16_t size, const char *file, int line);

//! Get current epoch in milliseconds
//! @return The current epoch time in milliseconds
uint64_t epoch(void);

//! Grab the current time and start the profiler count
void profile_start(void);

//! Detect how long the profiler has been running and print the result
void profile_print(void);
