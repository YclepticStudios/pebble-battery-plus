//! @file data_library.h
//! @brief All code for reading, writing, and processing data
//!
//! Contains all code used to read and write data, as well as the
//! code used to process the data and store the results. This
//! file is used in both the main program and the background worker.
//!
//! @author Eric D. Phillips
//! @date January 2, 2015
//! @bugs No known bugs

//! This file is included in both the main program and the worker, so it needs different includes
#pragma once
#ifdef PEBBLE_BACKGROUND_WORKER
#include <pebble_worker.h>
#else
#include <pebble.h>
#endif

// Data constants
#define DATA_LEVEL_LOW_THRESH_SEC 4 * SEC_IN_HR
#define DATA_LEVEL_MED_THRESH_SEC SEC_IN_DAY

//! Main data structure
typedef struct DataLibrary DataLibrary;


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

//! Get the number of points of history loaded (for run time and max life)
//! @param data_library A pointer to an existing DataLibrary
//! @return The number of data points available
uint16_t data_get_history_points_count(DataLibrary *data_library);

//! Get the time the watch needs to be charged by
//! @param data_library A pointer to an existing DataLibrary
//! @return The time the watch needs to be charged as a UTC epoch
int32_t data_get_charge_by_time(DataLibrary *data_library);

//! Get the estimated time remaining in seconds
//! @param data_library A pointer to an existing DataLibrary
//! @return The number of seconds of battery life remaining
int32_t data_get_life_remaining(DataLibrary *data_library);

//! Get the time the watch was last charged
//! @param data_library A pointer to an existing DataLibrary
//! @return The time the watch was lasted charged as a UTC epoch
int32_t data_get_last_charge_time(DataLibrary *data_library);

//! Get a past run time by its index (0 is current, 1 is yesterday, etc)
//! Must be between 0 and DATA_HISTORY_INDEX_MAX
//! @param data_library A pointer to an existing DataLibrary
//! @param index The number of charge cycles into the past
//! @return The duration of that run time in seconds
int32_t data_get_past_run_time(DataLibrary *data_library, uint16_t index);

//! Get the record run time of the watch
//! @param data_library A pointer to an existing DataLibrary
//! @return The record run time of the watch in seconds
int32_t data_get_record_run_time(DataLibrary *data_library);

//! Get the current run time of the watch in seconds (if no charge data, returns app install time)
//! @param data_library A pointer to an existing DataLibrary
//! @return The number of seconds since the last charge
int32_t data_get_run_time(DataLibrary *data_library);

//! Get the current percent-per-day of battery life
//! @param data_library A pointer to an existing DataLibrary
//! @return The current percent-per-day discharge rate
int32_t data_get_percent_per_day(DataLibrary *data_library);

//! Get the current battery percentage (this is an estimate of the exact value)
//! @param data_library A pointer to an existing DataLibrary
//! @return An estimate of the current exact battery percent
uint8_t data_get_battery_percent(DataLibrary *data_library);

//! Get a past max life by its index (0 is current, 1 is yesterday, etc)
//! Must be between 0 and DATA_HISTORY_INDEX_MAX
//! @param data_library A pointer to an existing DataLibrary
//! @param index The number of charge cycles into the past
//! @return The duration of that max life in seconds
int32_t data_get_past_max_life(DataLibrary *data_library, uint16_t index);

//! Get the maximum battery life possible with the current discharge rate
//! @param data_library A pointer to an existing DataLibrary
//! @return The maximum seconds of battery life
int32_t data_get_max_life(DataLibrary *data_library);

//! Get a data point by its index with 0 being the most recent
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the point to get
//! @param epoch A pointer to an int32_t to which to set the epoch
//! @param percent A pointer to a uint8_t to which to set the battery percent
//! @return True if a point exists else false if outside data bounds
bool data_get_data_point(DataLibrary *data_library, uint16_t index, int32_t *epoch,
                                 uint8_t *percent);

//! Get the number of charge cycles which include the last x number of seconds
//! @param data_library A pointer to an existing DataLibrary
//! @param seconds The number of seconds back to count
//! @return The minimum number of charge cycles to encompass that time span
uint16_t data_get_charge_cycle_count_including_seconds(DataLibrary *data_library, int32_t seconds);

//! Get the number of data points which include the last x number of seconds
//! @param data_library A pointer to an existing DataLibrary
//! @param seconds The number of seconds back to count
//! @return The minimum number of data points to encompass that time span
uint16_t data_get_data_point_count_including_seconds(DataLibrary *data_library, int32_t seconds);

//! Print the data to the console in CSV format
//! @param data_library A pointer to an existing DataLibrary
void data_print_csv(DataLibrary *data_library);

//! Process a BatteryChargeState structure and add it to the data
//! @param data_library A pointer to an existing DataLibrary
//! @param battery_state A BatteryChargeState containing the state of the battery
void data_process_new_battery_state(DataLibrary *data_library, BatteryChargeState
                                    battery_state);

//! Destroy data and reload from persistent storage
//! @param data_library A pointer to an existing DataLibrary
void data_reload(DataLibrary *data_library);

//! Initialize the data
//! @return A pointer to a newly initialized DataLibrary structure
DataLibrary *data_initialize(void);

//! Terminate the data
//! @param data_library A pointer to a previously initialized DataLibrary type
void data_terminate(DataLibrary *data_library);
