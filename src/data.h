//! @file data.h
//! @brief File to handle data
//!
//! Save and load data. Also calculates data trends and statistics for data.
//!
//! @author Eric D. Phillips
//! @date November 26, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>

// Public Constants
#define SEC_IN_DAY 86400

////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

//! Get the estimated time remaining as a formatted string
//! @param buff The buffer to write the time string into
void data_get_time_remaining(char *buff);

//! Load the past X days of data
//! @param num_days The number of days of data to load
void data_load_past_days(uint8_t num_days);

//! Unload data and free memory
void data_unload(void);

//! Print the data to the console
void data_print(void);
