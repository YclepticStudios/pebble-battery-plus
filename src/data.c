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
  DataNode tmp_node;
  if (!head_node) {
    BatteryChargeState bat_state = battery_state_service_peek();
    tmp_node.epoch = time(NULL);
    tmp_node.percent = bat_state.charge_percent;
    tmp_node.charging = bat_state.is_charging;
    tmp_node.plugged = bat_state.is_plugged;
    tmp_node.next = NULL;
  } else {
    tmp_node = *head_node;
  }
  return tmp_node;
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

// Get the estimated time remaining as a formatted string
void data_get_time_remaining(char *buff, uint16_t length) {
  // get latest node
  DataNode tmp_node = prv_get_latest_node();
  // calculate time remaining
  int32_t sec_remaining = tmp_node.epoch + tmp_node.percent * charge_rate - time(NULL);
  int days = sec_remaining / SEC_IN_DAY;
  int hrs = sec_remaining % SEC_IN_DAY / SEC_IN_HR;
  // format and print
  snprintf(buff, length, "%d days %d hours", days, hrs);
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
  return charge_rate * 100;
}

// Get the current charge rate in seconds per percent (will always be negative since discharging)
int32_t data_get_charge_rate(void) {
  return charge_rate;
}

// Load the past X days of data
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
  char buff[PERSIST_DATA_MAX_LENGTH];
  // loop over data
  while (persist_key > DATA_PERSIST_KEY && persist_exists(persist_key)) {
    persist_read_data(persist_key, buff, persist_get_size(persist_key));
    for (char *ptr = buff + persist_get_size(persist_key) - worker_node_size; ptr >= buff;
         ptr -= worker_node_size) {
      // create new node
      DataNode *new_node = MALLOC(sizeof(DataNode));
      memcpy(new_node, ptr, worker_node_size);
      new_node->next = NULL;
      // check if passed the number of days
      if (new_node->epoch < time(NULL) - num_days * SEC_IN_DAY) {
        free(new_node);
        return;
      }
      prv_list_add_node_end(new_node);
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
void data_print(void) {
  DataNode *cur_node = head_node;
  while (cur_node) {
    printf("\nEpoch: %d\nPercent: %d\nCharging: %d\nPlugged: %d\n", cur_node->epoch,
      cur_node->percent, cur_node->charging, cur_node->plugged);
    cur_node = cur_node->next;
  }
  // print charge rate
  printf("\nCharge Rate: %d", (int)charge_rate);
}
