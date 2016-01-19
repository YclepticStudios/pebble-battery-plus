//! @file data_api.h
//! @brief Contacts background worker to request data
//!
//! Contains all code used to communicate with the background worker in order
//! to get data and stats. No actual data processing is done in the foreground
//! portion of Battery+.
//!
//! @author Eric D. Phillips
//! @date January 16, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>
#include "data_shared.h"


//! Main data structure
typedef struct DataAPI DataAPI;

//! Alert triggered callback type
typedef void(*BatteryAPIAlertCallback)(uint8_t);


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

//! Get the color of an alert from a table of colors based on index
//! Note: This function reads from persistent storage if a DataAPI is NULL
//! @param data_api Optional pointer to DataAPI to prevent persistent storage read
//! @param index The index of the alert
//! @return A GColor which represents that level of alert
GColor data_api_get_alert_color(DataAPI *data_api, uint8_t index);

//! Get the text of an alert from a table of text based on index
//! Note: This function reads from persistent storage if a DataAPI is NULL
//! @param data_api Optional pointer to DataAPI to prevent persistent storage read
//! @param index The index of the alert
//! @return A pointer to text which represents that level of alert
char *data_api_get_alert_text(DataAPI *data_api, uint8_t index);

//! Get the alert level threshold in seconds, this is the time remaining when the alert goes off
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the alert
//! @return The number of seconds the low level is set to
int32_t data_api_get_alert_threshold(DataAPI *data_api, uint8_t index);

//! Get the number of scheduled alerts at the current time
//! Note: This function reads from persistent storage if a DataAPI is NULL
//! @param data_api A pointer to an existing DataAPI
//! @return The current number of scheduled alerts
uint8_t data_api_get_alert_count(DataAPI *data_api);

//! Create a new alert at a certain threshold
//! @param data_api A pointer to an existing DataAPI
//! @param seconds The number of seconds before empty that the alert should go off
void data_api_schedule_alert(DataAPI *data_api, int32_t seconds);

//! Destroy an existing alert at a certain index
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the alert
void data_api_unschedule_alert(DataAPI *data_api, uint8_t index);

//! Get the time the watch needs to be charged by
//! @param data_api A pointer to an existing DataAPI
//! @return The time the watch needs to be charged as a UTC epoch
int32_t data_api_get_charge_by_time(DataAPI *data_api);

//! Get the estimated time remaining in seconds
//! @param data_api A pointer to an existing DataAPI
//! @return The number of seconds of battery life remaining
int32_t data_api_get_life_remaining(DataAPI *data_api);

//! Get the record run time of the watch
//! @param data_api A pointer to an existing DataAPI
//! @return The record run time of the watch in seconds
int32_t data_api_get_record_run_time(DataAPI *data_api);

//! Get the run time at a certain charge cycle returns negative value if no data
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The number of seconds since the last charge
int32_t data_api_get_run_time(DataAPI *data_api, uint16_t index);

//! Get the maximum battery life possible at a certain charge cycle (0 is current)
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The maximum seconds of battery life
int32_t data_api_get_max_life(DataAPI *data_api, uint16_t index);

//! Get the current battery percentage (this is an estimate of the exact value)
//! @param data_api A pointer to an existing DataAPI
//! @return An estimate of the current exact battery percent
uint8_t data_api_get_battery_percent(DataAPI *data_api);

//! Get a data point by its index with 0 being the most recent
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the point to get
//! @param epoch A pointer to an int32_t to which to set the epoch
//! @param percent A pointer to a uint8_t to which to set the battery percent
//! @return True if there is a data point, else false if out of points
bool data_api_get_data_point(DataAPI *data_api, uint16_t index, int32_t *epoch,
                             uint8_t *percent);

//! Get the number of charge cycles currently loaded into memory
//! @param data_api A pointer to an existing DataAPI
//! @return The number of cycles loaded into memory
uint16_t data_api_get_charge_cycle_count(DataAPI *data_api);

//! Print the data to the console in CSV format
//! @param data_api A pointer to an existing DataAPI
void data_api_print_csv(DataAPI *data_api);

//! Destroy data and reload from persistent storage
//! @param data_api A pointer to an existing DataAPI
void data_api_reload(DataAPI *data_api);

//! Initialize the data
//! @return A pointer to a newly initialized DataAPI structure
DataAPI *data_api_initialize(void);

//! Terminate the data
//! @param data_api A pointer to a previously initialized DataAPI type
void data_api_terminate(DataAPI *data_api);
