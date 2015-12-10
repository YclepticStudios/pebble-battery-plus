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
#define SEC_IN_HR 3600
#define SEC_IN_MIN 60

////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

//! Get the estimated time remaining as a formatted string
//! @param buff The buffer to write the time string into
//! @param length The size of the buffer in bytes
void data_get_formated_time_remaining(char *buff, uint16_t length);

//! Get the estimated time remaining in seconds
//! @return The number of seconds of battery life remaining
int32_t data_get_life_remaining(void);

//! Get the current battery percentage (this is an estimate of the exact value)
//! @return An estimate of the current exact battery percent
uint8_t data_get_battery_percent(void);

//! Get the maximum battery life possible with the current discharge rate
//! @return The maximum seconds of battery life
int32_t data_get_max_life(void);

//! Get the current charge rate in seconds per percent (will always be negative since discharging)
//! @return The current charge rate
int32_t data_get_charge_rate(void);

//! Load the past X days of data
//! @param num_days The number of days of data to load
void data_load_past_days(uint8_t num_days);

//! Unload data and free memory
void data_unload(void);

//! Print the data to the console in CSV format
void data_print_csv(void);
