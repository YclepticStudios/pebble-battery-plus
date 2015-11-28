// @file data.c
// @brief File to handle data
//
// Save and load data. Also calculates data trends and statistics for data.
//
// @author Eric D. Phillips
// @date November 26, 2015
// @bugs No known bugs

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
static DataNode *head_node = NULL;
static int32_t charge_rate;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

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
void data_get_time_remaining(char *buff);

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
