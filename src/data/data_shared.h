//! @file data_shared.h
//! @brief Contains main shared data for different data controllers
//!
//! Contains some global constants and persistent storage keys which are
//! used in both the foreground and background data sections of the app.
//!
//! @author Eric D. Phillips
//! @date January 18, 2015
//! @bugs No known bugs

//! This file is included in both the main program and the worker, so it needs different includes
#pragma once
#ifdef PEBBLE_BACKGROUND_WORKER
#include <pebble_worker.h>
#else
#include <pebble.h>
#endif


//! Constants
#define DATA_ALERT_MAX_COUNT 4          //< The maximum number of alerts to allow
#define CHARGE_CYCLE_MAX_COUNT 9        //< The maximum number of charge cycles to load
#define DATA_POINT_MAX_COUNT 50         //< The maximum number of data points to load
#define BATTERY_PERCENTAGE_OFFSET 10    //< Percentage to add to the battery to increase accuracy

//! Persistent Keys
#define PERSIST_DATA_KEY 1000           //< The persistent storage key where the data write starts
#define PERSIST_RECORD_LIFE_KEY 999     //< Persistent storage key where the record life is stored
#define PERSIST_ALERTS_KEY 998          //< Persistent storage key for number of alerts scheduled
#define WAKE_UP_ALERT_INDEX_KEY 997     //< Persistent storage key for alerts
#define TEMP_COMMUNICATION_KEY 996      //< Key used when writing data for the foreground
#define TEMP_LOCK_KEY 995               //< Key used when writing data for the foreground
#define PERSIST_TIMELINE_KEY 994        //< Persistent storage key where timeline enabled is stored
#define DATA_LOGGING_TAG 5155346        //< Tag used to identify data once on phone

//! Data structure for foreground
typedef struct DataAPI {
  int32_t     alert_threshold[DATA_ALERT_MAX_COUNT];    //< The threshold for an alert to go off at
  int32_t     charge_rate;        //< The current charge rate
  int32_t     charge_by_time;     //< The time the watch will hit 0% in seconds since epoch
  int32_t     last_charged_time;  //< The time the watch was last charged
  int32_t     record_run_time;    //< The record length between charging and charging again
  int32_t     run_times[CHARGE_CYCLE_MAX_COUNT];        //< Array of past x run times
  int32_t     max_lives[CHARGE_CYCLE_MAX_COUNT];        //< Array of past x max lives
  int32_t     data_pt_epochs[DATA_POINT_MAX_COUNT];     //< Array of past x data point times
  uint8_t     data_pt_percents[DATA_POINT_MAX_COUNT];   //< Array of past x data point battery %'s
  uint16_t    data_pt_start_index;                      //< The index the data starts at
  uint8_t     alert_count;        //< The total number of alerts scheduled
  uint8_t     cycle_count;        //< The total number of charge cycles loaded
  uint8_t     data_pt_count;      //< The total number of raw data points loaded
} DataAPI;

//! App worker message commands
typedef enum {
  WorkerMessageSendData,
  WorkerMessageReloadData,
  WorkerMessageScheduleAlert,
  WorkerMessageUnscheduleAlert,
  WorkerMessageAlertEvent,
  WorkerMessageExportData
} WorkerMessage;
