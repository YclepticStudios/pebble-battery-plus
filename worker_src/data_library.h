//! @file data_library.h
//! @brief All code for reading, writing, and processing data
//!
//! Contains all code used to read and write data, as well as the
//! code used to process the data and store the results.
//!
//! @author Eric D. Phillips
//! @date January 2, 2015
//! @bugs No known bugs

#pragma once
#include <pebble_worker.h>

//! Main data structure
typedef struct DataLibrary DataLibrary;

//! Alert triggered callback type
typedef void(*BatteryAlertCallback)(uint8_t);


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

//! Get the alert level threshold in seconds, this is the time remaining when the alert goes off
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the alert
//! @return The number of seconds the low level is set to
int32_t data_get_alert_threshold(DataLibrary *data_library, uint8_t index);

//! Get the number of scheduled alerts at the current time
//! Note: This function reads from persistent storage if a DataLibrary is NULL
//! @param data_library A pointer to an existing DataLibrary
//! @return The current number of scheduled alerts
uint8_t data_get_alert_count(DataLibrary *data_library);

//! Refresh all alerts and schedule timers which will do the actual waking up
//! @param data_library A pointer to an existing DataLibrary
void data_refresh_all_alerts(DataLibrary *data_library);

//! Create a new alert at a certain threshold
//! @param data_library A pointer to an existing DataLibrary
//! @param seconds The number of seconds before empty that the alert should go off
void data_schedule_alert(DataLibrary *data_library, int32_t seconds);

//! Destroy an existing alert at a certain index
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the alert
void data_unschedule_alert(DataLibrary *data_library, uint8_t index);

//! Register callback for when an alert goes off
//! @param data_library A pointer to an existing DataLibrary
//! @param callback The callback to register
// TODO: Add this to the foreground app to get alerts when awake
void data_register_alert_callback(DataLibrary *data_library, BatteryAlertCallback callback);

//! Get the time the watch needs to be charged by
//! @param data_library A pointer to an existing DataLibrary
//! @return The time the watch needs to be charged as a UTC epoch
int32_t data_get_charge_by_time(DataLibrary *data_library);

//! Get the estimated time remaining in seconds
//! @param data_library A pointer to an existing DataLibrary
//! @return The number of seconds of battery life remaining
int32_t data_get_life_remaining(DataLibrary *data_library);

//! Get the record run time of the watch
//! @param data_library A pointer to an existing DataLibrary
//! @return The record run time of the watch in seconds
int32_t data_get_record_run_time(DataLibrary *data_library);

//! Get the run time at a certain charge cycle returns negative value if no data
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The number of seconds since the last charge
int32_t data_get_run_time(DataLibrary *data_library, uint16_t index);

//! Get the maximum battery life possible at a certain charge cycle (0 is current)
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The maximum seconds of battery life
int32_t data_get_max_life(DataLibrary *data_library, uint16_t index);

//! Get the current percent-per-day of battery life
//! @param data_library A pointer to an existing DataLibrary
//! @return The current percent-per-day discharge rate
int32_t data_get_percent_per_day(DataLibrary *data_library);

//! Get the current battery percentage (this is an estimate of the exact value)
//! @param data_library A pointer to an existing DataLibrary
//! @return An estimate of the current exact battery percent
uint8_t data_get_battery_percent(DataLibrary *data_library);

//! Get a data point by its index with 0 being the most recent
//! @param data_library A pointer to an existing DataLibrary
//! @param index The index of the point to get
//! @param epoch A pointer to an int32_t to which to set the epoch
//! @param percent A pointer to a uint8_t to which to set the battery percent
void data_get_data_point(DataLibrary *data_library, uint16_t index, int32_t *epoch,
                         uint8_t *percent);

//! Get the number of charge cycles which include the last x number of seconds (0 gets all points)
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

//! Write the data out in chunks to the foreground app
//! @param data_library A pointer to an existing DataLibrary
//! @param data_pt_start_index The index of the data point to start with in the raw data pt section
void data_write_to_foreground(DataLibrary *data_library, uint8_t data_pt_start_index);

//! Initialize the data
//! @return A pointer to a newly initialized DataLibrary structure
DataLibrary *data_initialize(void);

//! Terminate the data
//! @param data_library A pointer to a previously initialized DataLibrary type
void data_terminate(DataLibrary *data_library);
