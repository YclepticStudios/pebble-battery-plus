////! @file data.h
////! @brief File to handle data
////!
////! Save and load data. Also calculates data trends and statistics for data.
////!
////! @author Eric D. Phillips
////! @date November 26, 2015
////! @bugs No known bugs
//
//#pragma once
//#include <pebble.h>
//
//// Data constants
//#define DATA_LEVEL_LOW_THRESH_SEC 4 * SEC_IN_HR
//#define DATA_LEVEL_MED_THRESH_SEC SEC_IN_DAY
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
//// API Implementation
////
//
////! Get the number of points of history loaded (for run time and max life)
////! @return The number of data points available
//uint16_t data_get_history_points_count(void);
//
////! Get the time the watch needs to be charged by
////! @return The time the watch needs to be charged as a UTC epoch
//int32_t data_get_charge_by_time(void);
//
////! Get the estimated time remaining in seconds
////! @return The number of seconds of battery life remaining
//int32_t data_get_life_remaining(void);
//
////! Get the time the watch was last charged
////! @return The time the watch was lasted charged as a UTC epoch
//int32_t data_get_last_charge_time(void);
//
////! Get a past run time by its index (0 is current, 1 is yesterday, etc)
////! Must be between 0 and DATA_HISTORY_INDEX_MAX
////! @param index The number of charge cycles into the past
////! @return The duration of that run time in seconds
//int32_t data_get_past_run_time(uint16_t index);
//
////! Get the record run time of the watch
////! @return The record run time of the watch in seconds
//int32_t data_get_record_run_time(void);
//
////! Get the current run time of the watch in seconds (if no charge data, returns app install time)
////! @return The number of seconds since the last charge
//int32_t data_get_run_time(void);
//
////! Get the current percent-per-day of battery life
////! @return The current percent-per-day discharge rate
//int32_t data_get_percent_per_day(void);
//
////! Get the current battery percentage (this is an estimate of the exact value)
////! @return An estimate of the current exact battery percent
//uint8_t data_get_battery_percent(void);
//
////! Get a past max life by its index (0 is current, 1 is yesterday, etc)
////! Must be between 0 and DATA_HISTORY_INDEX_MAX
////! @param index The number of charge cycles into the past
////! @return The duration of that max life in seconds
//int32_t data_get_past_max_life(uint16_t index);
//
////! Get the maximum battery life possible with the current discharge rate
////! @return The maximum seconds of battery life
//int32_t data_get_max_life(void);
//
////! Get the current charge rate in seconds per percent (will always be negative since discharging)
////! @return The current charge rate
//int32_t data_get_charge_rate(void);
//
////! Get the current number of data points loaded in RAM
////! @return The number of data points in RAM
//uint16_t data_get_battery_data_point_count(void);
//
////! Get data points by index with 0 being the latest point
////! @param index The index of the point to retrieve
////! @param epoch A pointer to an int32_t to which to set the epoch
////! @param percent A pointer to a uint8_t to which to set the battery percent
////! @return True if a point exists else false if outside data bounds
//bool data_get_battery_data_point(uint16_t index, int32_t *epoch, uint8_t *percent);
//
////! Load the past X days of data
////! @param num_days The number of days of data to load
//void data_load_past_days(uint8_t num_days);
//
////! Unload data and free memory
//void data_unload(void);
//
////! Print the data to the console in CSV format
//void data_print_csv(void);
