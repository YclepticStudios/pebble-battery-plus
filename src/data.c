// @file data.c
// @brief File to handle data
//
// Save and load data. Also calculates data trends and statistics for data.
//
// @author Eric D. Phillips
// @date November 26, 2015
// @bugs No known bugs

#include <pebble.h>
#include "data.h"
#include "utility.h"

// Constants
#define PERSIST_DATA_LENGTH 256
#define STATS_CHARGE_RATE_KEY 999
#define DATA_PERSIST_KEY 1000

// Main data structure
typedef struct DataNode {
  uint32_t  epoch     : 31;   //< The epoch timestamp for when the percentage changed in seconds
  uint8_t   percent   : 7;    //< The battery charge percent
  bool      charging  : 1;    //< The charging state of the battery
  bool      plugged   : 1;    //< The plugged in state of the watch
  struct DataNode *next;      //< The next node in the linked list
} __attribute__((__packed__)) DataNode;

// Main variables
static DataNode *head_node = NULL; // Newest at start
static int32_t charge_rate;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Get the latest data node (returns the current battery value if no nodes)
static DataNode prv_get_latest_node(void) {
  // get current battery state
  DataNode cur_node;
  BatteryChargeState bat_state = battery_state_service_peek();
  cur_node.epoch = time(NULL);
  cur_node.percent = bat_state.charge_percent;
  cur_node.charging = bat_state.is_charging;
  cur_node.plugged = bat_state.is_plugged;
  cur_node.next = NULL;
  // return current state if no last node or not matching with last node
  if (!head_node || head_node->percent != cur_node.percent ||
    head_node->charging != cur_node.charging || head_node->plugged != cur_node.plugged) {
    return cur_node;
  }
  return *head_node;
}

// Add node to start of linked list
static void prv_list_add_node_start(DataNode *node) {
  node->next = head_node;
  head_node = node;
}

// Add node to end of linked list
static void prv_list_add_node_end(DataNode *node) {
  if (!head_node) {
    head_node = node;
    return;
  }
  DataNode *cur_node = head_node;
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  cur_node->next = node;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Get the estimated time remaining in seconds
int32_t data_get_life_remaining(void) {
  // get latest node
  DataNode tmp_node = prv_get_latest_node();
  // calculate time remaining
  return tmp_node.epoch + tmp_node.percent * -charge_rate - time(NULL);
}

// Get the time the watch was last charged
int32_t data_get_last_charge_time(void) {
  return time(NULL) - data_get_run_time();
}

// Get the current run time of the watch in seconds (if no charge data, returns app install time)
int32_t data_get_run_time(void) {
  // last charge time
  uint32_t lst_charge_time = time(NULL);
  // loop over nodes
  DataNode *cur_node = head_node;
  while (cur_node && !cur_node->charging) {
    lst_charge_time = cur_node->epoch;
    cur_node = cur_node->next;
  }
  // return time in seconds
  return time(NULL) - lst_charge_time;
}

// Get the current percent-per-day of battery life
int32_t data_get_percent_per_day(void) {
  return 10000 / (data_get_max_life() * 100 / SEC_IN_DAY);
}

// Get the current battery percentage (this is an estimate of the exact value)
uint8_t data_get_battery_percent(void) {
  // get latest node
  DataNode tmp_node = prv_get_latest_node();
  // calculate exact percent
  int32_t percent = tmp_node.percent + (time(NULL) - tmp_node.epoch) / charge_rate;
  if (percent > tmp_node.percent) {
    percent = tmp_node.percent;
  } else if (percent <= tmp_node.percent - 10) {
    percent = tmp_node.percent - 9;
  }
  if (percent < 0) {
    percent = 0;
  }
  return percent;
}

// Get the maximum battery life possible with the current discharge rate
int32_t data_get_max_life(void) {
  return charge_rate * (-100);
}

// Get the current charge rate in seconds per percent (will always be negative since discharging)
int32_t data_get_charge_rate(void) {
  return charge_rate;
}

// Load the past X days of data + at least last charge included
void data_load_past_days(uint8_t num_days) {
  if (!persist_exists(DATA_PERSIST_KEY)) {
    return;
  }
  // unload any current data first
  data_unload();
  // load current charge rate estimate
  charge_rate = persist_read_int(STATS_CHARGE_RATE_KEY);
  // prep for data
  uint32_t persist_key = persist_read_int(DATA_PERSIST_KEY);
  uint32_t worker_node_size = sizeof(DataNode) - sizeof(((DataNode*)0)->next);
  char buff[PERSIST_DATA_LENGTH];
  // loop over data
  bool has_charged_node = false;
  while (persist_key > DATA_PERSIST_KEY && persist_exists(persist_key)) {
    persist_read_data(persist_key, buff, persist_get_size(persist_key));
    for (char *ptr = buff + persist_get_size(persist_key) - worker_node_size; ptr >= buff;
         ptr -= worker_node_size) {
      // create new node
      DataNode *new_node = MALLOC(sizeof(DataNode));
      memcpy(new_node, ptr, worker_node_size);
      new_node->next = NULL;
      prv_list_add_node_end(new_node);
      // check if passed the number of days
      has_charged_node = has_charged_node || new_node->charging;
      if (new_node->epoch < time(NULL) - num_days * SEC_IN_DAY && has_charged_node) {
        return;
      }
    }
    // index key
    persist_key--;
  }
}

// Unload data and free memory
void data_unload(void) {
  DataNode *cur_node = head_node;
  DataNode *tmp_node = NULL;
  while (cur_node) {
    // index node
    tmp_node = cur_node;
    cur_node = cur_node->next;
    // destroy node
    free(tmp_node);
  }
  head_node = NULL;
}

// Print the data to the console
void data_print_csv(void) {
  // print header
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Epoch,\tPercent,\tCharging,\tPlugged,");
  // print body
  DataNode *cur_node = head_node;
  while (cur_node) {
    app_log(APP_LOG_LEVEL_INFO, "", 0, "%d,\t%d,\t%d,\t%d,", cur_node->epoch, cur_node->percent,
      cur_node->charging, cur_node->plugged);
    cur_node = cur_node->next;
  }
  // print charge rate
  printf("\nCharge Rate: %d", (int)charge_rate);
}
