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

#include <pebble.h>
#include "data_api.h"
#include "data_shared.h"
#include "../utility.h"

// Alert colors and text for different counts and indices, accessed as [count][index]
// smaller index is closer to empty time (smaller threshold)
#ifdef PBL_COLOR
static uint8_t prv_alert_colors[][4] = {
  { GColorRedARGB8 },
  { GColorRedARGB8, GColorYellowARGB8 },
  { GColorRedARGB8, GColorOrangeARGB8, GColorYellowARGB8 },
  { GColorRedARGB8, GColorOrangeARGB8, GColorChromeYellowARGB8, GColorYellowARGB8 }
};
#else
static uint8_t prv_alert_colors[][4] = {
  { GColorLightGrayARGB8 },
  { GColorWhiteARGB8, GColorLightGrayARGB8 },
  { GColorLightGrayARGB8, GColorWhiteARGB8, GColorLightGrayARGB8 },
  { GColorWhiteARGB8, GColorLightGrayARGB8, GColorWhiteARGB8, GColorLightGrayARGB8 }
};
#endif
static char *prv_alert_text[][4] = {
  { "Low Alert" },
  { "Low Alert", "Med Alert" },
  { "Low Alert", "Med Alert", "1st Alert" },
  { "Low Alert", "Med Alert", "2nd Alert", "1st Alert" }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Sit and wait until data is loaded from the background
static void prv_load_data_from_background(DataAPI *data_api, uint16_t data_pt_start_index) {
  // delete any old data that may be loaded
  persist_delete(TEMP_LOCK_KEY);
  persist_delete(TEMP_COMMUNICATION_KEY);
  // send message to background to start writing data
  AppWorkerMessage message = {.data0 = data_pt_start_index};
  app_worker_send_message(WorkerMessageSendData, &message);
  // loop and wait
  uint16_t bytes_read = 0;
  time_t end_time = time(NULL) + 1;
  while (time(NULL) <= end_time) {
    // check if data key exists
    if (persist_exists(TEMP_LOCK_KEY)) {
      // read in data
      bytes_read += persist_read_data(TEMP_COMMUNICATION_KEY, ((char*)data_api + bytes_read),
        persist_get_size(TEMP_COMMUNICATION_KEY));
      persist_delete(TEMP_COMMUNICATION_KEY);
      persist_delete(TEMP_LOCK_KEY);
      // check if done
      if (bytes_read >= sizeof(DataAPI)) {
        break;
      }
    }
    // wait
    psleep(1);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Get the color of an alert from a table of colors based on index
GColor data_api_get_alert_color(DataAPI *data_api, uint8_t index) {
  return (GColor){ .argb = prv_alert_colors[data_api_get_alert_count(data_api) - 1][index] };
}

// Get the text of an alert from a table of text based on index
char *data_api_get_alert_text(DataAPI *data_api, uint8_t index) {
  return prv_alert_text[data_api_get_alert_count(data_api) - 1][index];
}

// Get the alert level threshold in seconds, this is the time remaining when the alert goes off
int32_t data_api_get_alert_threshold(DataAPI *data_api, uint8_t index) {
  return data_api->alert_threshold[index];
}

// Get the number of scheduled alerts at the current time
uint8_t data_api_get_alert_count(DataAPI *data_api) {
  return data_api->alert_count;
}

// Create a new alert at a certain threshold
void data_api_schedule_alert(DataAPI *data_api, int32_t seconds) {
  AppWorkerMessage message = (AppWorkerMessage){
    .data0 = (seconds & 0xFFFF0000) >> 16,
    .data1 = seconds & 0x0000FFFF
  };
  app_worker_send_message(WorkerMessageScheduleAlert, &message);
}

// Destroy an existing alert at a certain index
void data_api_unschedule_alert(DataAPI *data_api, uint8_t index) {
  AppWorkerMessage message = {.data0 = index};
  app_worker_send_message(WorkerMessageUnscheduleAlert, &message);
}

// Get the time the watch needs to be charged by
int32_t data_api_get_charge_by_time(DataAPI *data_api) {
  return data_api->charge_by_time;
}

// Get the estimated time remaining in seconds
int32_t data_api_get_life_remaining(DataAPI *data_api) {
  return data_api->charge_by_time - time(NULL);
};

// Get the record run time of the watch
int32_t data_api_get_record_run_time(DataAPI *data_api) {
  if (data_api_get_run_time(data_api, 0) > data_api->record_run_time) {
    return data_api_get_run_time(data_api, 0);
  }
  return data_api->record_run_time;
};

// Get the run time at a certain charge cycle returns negative value if no data
int32_t data_api_get_run_time(DataAPI *data_api, uint16_t index) {
  if (!index) {
    if (data_api->last_charged_time > 0) {
      return time(NULL) - data_api->last_charged_time;
    } else {
      return -1;
    }
  } else {
    return data_api->run_times[index - 1];
  }
};

// Get the maximum battery life possible at a certain charge cycle (0 is current)
int32_t data_api_get_max_life(DataAPI *data_api, uint16_t index) {
  if (!index) {
    return data_api->charge_rate * (-100);
  } else {
    return data_api->max_lives[index - 1];
  }
};

// Get the current battery percentage (this is an estimate of the exact value)
uint8_t data_api_get_battery_percent(DataAPI *data_api) {
  // calculate exact percent
  int32_t percent = data_api->data_pt_percents[0] +
    (time(NULL) - data_api->data_pt_epochs[0]) / data_api->charge_rate;
  // restrict it to a valid range
  BatteryChargeState battery_state = battery_state_service_peek();
  if (percent > battery_state.charge_percent) {
    percent = battery_state.charge_percent;
  } else if (percent <= battery_state.charge_percent - 10) {
    percent = battery_state.charge_percent - 9;
  }
  if (percent < 1) {
    percent = 1;
  }
  return percent;
};

// Get a data point by its index with 0 being the most recent
bool data_api_get_data_point(DataAPI *data_api, uint16_t index, int32_t *epoch,
                             uint8_t *percent) {
  // check if out of range
  if (index < data_api->data_pt_start_index ||
    index >= data_api->data_pt_start_index + data_api->data_pt_count) {
    // check if should load more or if no more data available
    if (data_api->data_pt_count < DATA_POINT_MAX_COUNT &&
      index >= data_api->data_pt_start_index + data_api->data_pt_count) {
      return false;
    }
    // load in a new data range
    prv_load_data_from_background(data_api, index);
  }
  // set values
  (*epoch) = data_api->data_pt_epochs[index - data_api->data_pt_start_index];
  (*percent) = data_api->data_pt_percents[index - data_api->data_pt_start_index];
  return true;
};

// Get the number of charge cycles currently loaded into memory
uint16_t data_api_get_charge_cycle_count(DataAPI *data_api) {
  return data_api->cycle_count + 1;
}

// Print the data to the console in CSV format
void data_api_print_csv(DataAPI *data_api) {
  // TODO: Implement this function
};

// Destroy data and reload from persistent storage
void data_api_reload(DataAPI *data_api) {
  prv_load_data_from_background(data_api, 0);
};

// Initialize the data
DataAPI *data_api_initialize(void) {
  // create DataAPI
  DataAPI *data_api = MALLOC(sizeof(DataAPI));
  memset(data_api, 0, sizeof(DataAPI));
  // load data into it from the background worker
  prv_load_data_from_background(data_api, 0);
  return data_api;
};

// Terminate the data
void data_api_terminate(DataAPI *data_api) {
  free(data_api);
};
