// @file data_api.c
// @brief Contacts background worker to request data
//
// Contains all code used to communicate with the background worker in order
// to get data and stats. No actual data processing is done in the foreground
// portion of Battery+.
//
// @author Eric D. Phillips
// @date January 16, 2015
// @bugs No known bugs

#include "data_api.h"
#include "../utility.h"

//! Main data structure
typedef struct DataAPI {

} DataAPI;


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Get the color of an alert from a table of colors based on index
GColor data_api_get_alert_color(DataAPI *data_api, uint8_t index) {
  return GColorBlack;
}

// Get the text of an alert from a table of text based on index
char *data_api_get_alert_text(DataAPI *data_api, uint8_t index) {
  return "";
}

// Get the alert level threshold in seconds, this is the time remaining when the alert goes off
int32_t data_api_get_alert_threshold(DataAPI *data_api, uint8_t index) {
  return 0;
}

// Get the number of scheduled alerts at the current time
uint8_t data_api_get_alert_count(DataAPI *data_api) {
  return 0;
}

// Refresh all alerts and schedule timers which will do the actual waking up
void data_api_refresh_all_alerts(DataAPI *data_api) {

}

// Create a new alert at a certain threshold
void data_api_schedule_alert(DataAPI *data_api, int32_t seconds) {

}

// Destroy an existing alert at a certain index
void data_api_unschedule_alert(DataAPI *data_api, uint8_t index) {

}

// Register callback for when an alert goes off
// TODO: Add this to the foreground app to get alerts when awake
void data_api_register_alert_callback(DataAPI *data_api, BatteryAPIAlertCallback callback) {

}

// Get the time the watch needs to be charged by
int32_t data_api_get_charge_by_time(DataAPI *data_api) {
  return 0;
}

//! Get the estimated time remaining in seconds
//! @param data_api A pointer to an existing DataAPI
//! @return The number of seconds of battery life remaining
int32_t data_api_get_life_remaining(DataAPI *data_api) {
  return 0;
};

//! Get the record run time of the watch
//! @param data_api A pointer to an existing DataAPI
//! @return The record run time of the watch in seconds
int32_t data_api_get_record_run_time(DataAPI *data_api) {
  return 0;
};

//! Get the run time at a certain charge cycle returns negative value if no data
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The number of seconds since the last charge
int32_t data_api_get_run_time(DataAPI *data_api, uint16_t index) {
  return 0;
};

//! Get the maximum battery life possible at a certain charge cycle (0 is current)
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the charge cycle to use (0 is current max life)
//! @return The maximum seconds of battery life
int32_t data_api_get_max_life(DataAPI *data_api, uint16_t index) {
  return 0;
};

//! Get the current percent-per-day of battery life
//! @param data_api A pointer to an existing DataAPI
//! @return The current percent-per-day discharge rate
int32_t data_api_get_percent_per_day(DataAPI *data_api) {
  return 0;
};

//! Get the current battery percentage (this is an estimate of the exact value)
//! @param data_api A pointer to an existing DataAPI
//! @return An estimate of the current exact battery percent
uint8_t data_api_get_battery_percent(DataAPI *data_api) {
  return 0;
};

//! Get a data point by its index with 0 being the most recent
//! @param data_api A pointer to an existing DataAPI
//! @param index The index of the point to get
//! @param epoch A pointer to an int32_t to which to set the epoch
//! @param percent A pointer to a uint8_t to which to set the battery percent
void data_api_get_data_point(DataAPI *data_api, uint16_t index, int32_t *epoch,
                             uint8_t *percent) {

};

//! Get the number of charge cycles which include the last x number of seconds (0 gets all points)
//! @param data_api A pointer to an existing DataAPI
//! @param seconds The number of seconds back to count
//! @return The minimum number of charge cycles to encompass that time span
uint16_t data_api_get_charge_cycle_count_including_seconds(DataAPI *data_api, int32_t seconds) {
  return 0;
};

//! Get the number of data points which include the last x number of seconds
//! @param data_api A pointer to an existing DataAPI
//! @param seconds The number of seconds back to count
//! @return The minimum number of data points to encompass that time span
uint16_t data_api_get_data_point_count_including_seconds(DataAPI *data_api, int32_t seconds) {
  return 0;
};

//! Print the data to the console in CSV format
//! @param data_api A pointer to an existing DataAPI
void data_api_print_csv(DataAPI *data_api) {

};

//! Process a BatteryChargeState structure and add it to the data
//! @param data_api A pointer to an existing DataAPI
//! @param battery_state A BatteryChargeState containing the state of the battery
void data_api_process_new_battery_state(DataAPI *data_api, BatteryChargeState
                                    battery_state) {

};

//! Destroy data and reload from persistent storage
//! @param data_api A pointer to an existing DataAPI
void data_api_reload(DataAPI *data_api) {

};

//! Initialize the data
//! @return A pointer to a newly initialized DataAPI structure
DataAPI *data_api_initialize(void) {
  return NULL;
};

//! Terminate the data
//! @param data_api A pointer to a previously initialized DataAPI type
void data_api_terminate(DataAPI *data_api) {

};
