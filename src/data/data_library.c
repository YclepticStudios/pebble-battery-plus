// @file data_library.c
// @brief All code for reading, writing, and processing data
//
// Contains all code used to read and write data, as well as the
// code used to process the data and store the results. This
// file is used in both the main program and the background worker.
//
// @author Eric D. Phillips
// @date January 2, 2015
// @bugs No known bugs

#include "data_library.h"
#include "../utility.h"

// Constants
#define DATA_VERSION 0                //< The current persistent storage format version
#define DATA_BLOCK_STATE_COUNT 50     //< The number of DataStates that fit in one persistent write
#define DATA_EPOCH_OFFSET 1420070400  //< Jan 1, 2015 at 0:00:00, reduces size when saving data
#define PERSIST_DATA_KEY 1000         //< The persistent storage key where the data write starts
#define PERSIST_RECORD_LIFE_KEY 999   //< The persistent storage key where the record life is stored
#define DATA_LOGGING_TAG 5155346      //< Tag used to identify data once on phone

// Data structure of node containing only data portion
typedef struct DataState {
  unsigned  epoch       : 30;   //< The epoch timestamp for when the percentage changed in seconds
  unsigned  percent     : 7;    //< The battery charge percent
  bool      charging    : 1;    //< The charging state of the battery
  bool      plugged     : 1;    //< The plugged in state of the watch
  bool      contiguous  : 1;    //< Whether the worker stayed active since the last data point
} __attribute__((__packed__)) DataState;

// Data structure of node including "next" pointer for linked list
typedef struct DataNode {
  DataState         state;    //< The actual data for this data point
  struct DataNode   *next;    //< Pointer to next node in linked list
} __attribute__((__packed__)) DataNode;

// Data structure used when saving to and reading from persistent storage
// Includes a header with extra stats
typedef struct DataStateBlock {
  unsigned    data_version          : 8;            //< The data version format for this block
  signed      initial_charge_rate   : 24;           //< The charge rate after first point in block
  unsigned    padding               : 10;           //< Padding space so structure is 256 bytes
  unsigned    data_state_count      : 6;            //< The number of data points written
  DataState   data_states[DATA_BLOCK_STATE_COUNT];  //< The actual data state points being stored
} __attribute__((__packed__)) DataStateBlock;

// Main data structure library
typedef struct DataLibrary {
  int32_t                 charge_rate;              //< The current charge rate in sec per percent
  uint32_t                node_count;               //< The number of nodes in the linked list
  DataNode                *head_node;               //< The head node for linked list, newest first
  bool                    data_is_contiguous;       //< Whether worker was shut off since last pt
  uint16_t                loaded_data_block_count;  //< The number of DataStateBlocks loaded in RAM
  DataLoggingSessionRef   data_logging_session;     //< Data logging session to send data to phone
} DataLibrary;

// Function declarations
// Reads the next DataStateBlock (containing a max of 50 data points) into memory
static bool prv_persist_read_next_data_block(DataLibrary *data_library);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Get a DataNode at a certain index into the linked list, with 0 being most recent
static DataNode* prv_linked_list_get_data_node(DataLibrary *data_library, uint16_t index) {
  DataNode *cur_node = data_library->head_node;
  uint16_t ii = 0;
  while (cur_node) {
    // check if node
    if (index == ii) {
      return cur_node;
    }
    // load more data if next is NULL
    if (!cur_node->next) {
      prv_persist_read_next_data_block(data_library);
    }
    // index node
    ii++;
    cur_node = cur_node->next;
  }
  return NULL;
}

// Add node to start of linked list
static void prv_linked_list_add_node_start(DataLibrary *data_library, DataNode *node) {
  node->next = data_library->head_node;
  data_library->head_node = node;
  data_library->node_count++;
}

// Add node to end of linked list
static void prv_linked_list_add_node_end(DataLibrary *data_library, DataNode *node) {
  if (!data_library->head_node) {
    data_library->head_node = node;
    data_library->node_count++;
    return;
  }
  DataNode *cur_node = data_library->head_node;
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  cur_node->next = node;
  data_library->node_count++;
}

// Destroy all data points from current node onward (includes current point)
static void prv_linked_list_destroy(DataLibrary *data_library, DataNode *node) {
  DataNode *cur_node = node;
  DataNode *tmp_node;
  while (cur_node) {
    // index node
    tmp_node = cur_node;
    cur_node = cur_node->next;
    // destroy node
    free(tmp_node);
    data_library->node_count--;
  }
}

// Get the latest data state (returns the current battery value if no nodes)
static DataState prv_get_current_data_state(DataLibrary *data_library) {
  // get current battery state
  DataState cur_state;
  BatteryChargeState bat_state = battery_state_service_peek();
  cur_state.epoch = time(NULL) - DATA_EPOCH_OFFSET;
  cur_state.percent = bat_state.charge_percent;
  cur_state.charging = bat_state.is_charging;
  cur_state.plugged = bat_state.is_plugged;
  // get latest existing data point
  DataNode *cur_node = prv_linked_list_get_data_node(data_library, 0);
  // return current state if no last node or not matching with last node
  if (!cur_node || cur_node->state.percent != cur_state.percent ||
    cur_node->state.charging != cur_state.charging ||
    cur_node->state.plugged != cur_state.plugged) {
    return cur_state;
  }
  return cur_node->state;
}

// Get default charge rate based on pebble type
static int32_t prv_get_default_charge_rate(void) {
  WatchInfoModel watch_model = watch_info_get_model();
  switch (watch_model) {
    case WATCH_INFO_MODEL_PEBBLE_TIME_STEEL:
      return -10 * SEC_IN_DAY / 100;
    case WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_14:
    case WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_20:
      return -2 * SEC_IN_DAY / 100;
    default:
      return -7 * SEC_IN_DAY / 100;
  }
}

// Check if two data points are contiguous
static bool prv_are_data_states_contiguous(DataState old_state, DataState new_state,
                                           int32_t charge_rate) {
  // check if listed as contiguous
  if (new_state.contiguous) { return true; }
  // otherwise predict when the new state should occur and compare to the actual time
  // Note: the epoch offset does not matter in this case
  int32_t predicted_epoch = old_state.epoch + (-10) * charge_rate;
  // value must be less than 1.5 times the difference of the old and new states
  if (new_state.epoch < predicted_epoch ||
    (new_state.epoch - predicted_epoch < (new_state.epoch - old_state.epoch)) / 2) {
    return true;
  }
  // otherwise they are not contiguous
  return false;
}

// Calculate the charge rate
static int32_t prv_calculate_charge_rate(DataState old_state, DataState new_state,
                                         int32_t charge_rate) {
  // check if qualifies for a charge rate calculation
  if (new_state.epoch <= old_state.epoch || new_state.percent >= old_state.percent ||
    new_state.charging || old_state.charging ||
    !prv_are_data_states_contiguous(old_state, new_state, charge_rate)) {
    return charge_rate;
  } else {
    return charge_rate * 4 / 5 +
      ((new_state.epoch - old_state.epoch) / (new_state.percent - old_state.percent)) / 5;
  }
}

// Reads the next DataStateBlock (containing a max of 50 data points) into memory
// Reading is done in blocks, consecutively, from most recent to oldest
// Returns the success of the reading
static bool prv_persist_read_next_data_block(DataLibrary *data_library) {
  // get location and size of data and check for valid index
  uint32_t persist_key = persist_read_int(PERSIST_DATA_KEY) -
    data_library->loaded_data_block_count;
  if (!persist_exists(persist_key)) { persist_key--; }
  if (persist_key <= PERSIST_DATA_KEY || !persist_exists(persist_key)) { return false; }
  // read the data from persistent storage
  DataStateBlock data_state_block;
  persist_read_data(persist_key, &data_state_block, sizeof(DataStateBlock));
  data_library->charge_rate = data_state_block.initial_charge_rate;
  // loop over data
  DataNode *tmp_node;
  for (uint8_t ii = 0, jj = data_state_block.data_state_count - 1;
       ii < data_state_block.data_state_count; ii++, jj--) {
    // add node to linked list
    tmp_node = MALLOC(sizeof(DataNode));
    tmp_node->state = data_state_block.data_states[jj];
    tmp_node->next = NULL;
    prv_linked_list_add_node_end(data_library, tmp_node);
    // calculate current charge rate if first block of data
    if (data_library->loaded_data_block_count == 0 &&
        ii + 1 < data_state_block.data_state_count) {
      data_library->charge_rate = prv_calculate_charge_rate(data_state_block.data_states[ii],
        data_state_block.data_states[ii + 1], data_library->charge_rate);
    }
  }
  // index number of loaded blocks
  data_library->loaded_data_block_count++;
  return true;
}

// Write newest DataState into persistent storage
static void prv_persist_write_data_state(DataLibrary *data_library, DataState data_state) {
  // get the location and size of the data
  uint32_t persist_key = persist_read_int(PERSIST_DATA_KEY);
  // build a DataStateBlock to write into memory
  DataStateBlock data_state_block = (DataStateBlock) {
    .data_version = DATA_VERSION,
    .initial_charge_rate = data_library->charge_rate,
    .data_state_count = 0
  };
  if (persist_exists(persist_key)) {
    persist_read_data(persist_key, &data_state_block, sizeof(DataStateBlock));
  }
  data_state_block.data_states[data_state_block.data_state_count++] = data_state;
  // check if DataStateBlock is full and index to next key
  if (data_state_block.data_state_count >= DATA_BLOCK_STATE_COUNT) {
    persist_write_int(PERSIST_DATA_KEY, ++persist_key);
  }
  // get the oldest existing key
  uint32_t old_persist_key = persist_key;
  while (old_persist_key > PERSIST_DATA_KEY && persist_exists(old_persist_key - 1)) {
    old_persist_key--;
  }
  // attempt to write the data and delete old data if the write fails,
  // but do not delete any closer than the last three data blocks
  uint16_t bytes_written = persist_write_data(persist_key, &data_state_block,
    sizeof(DataStateBlock));
  while (bytes_written < sizeof(DataStateBlock) && old_persist_key + 3 < persist_key) {
    persist_delete(old_persist_key++);
    bytes_written = persist_write_data(persist_key, &data_state_block, sizeof(DataStateBlock));
  }
}

// Process a DataState structure by calculating stats and persisting
static void prv_process_data_state(DataLibrary *data_library, DataState data_state) {
  // TODO: Calculate record life
  // calculate charge rate
  DataNode *cur_node = prv_linked_list_get_data_node(data_library, 0);
  if (cur_node) {
    data_library->charge_rate = prv_calculate_charge_rate(cur_node->state, data_state,
      data_library->charge_rate);
  }
  // TODO: Schedule wake-up alert
  // persist the data point
  prv_persist_write_data_state(data_library, data_state);
  // send the data to the phone with data logging
  data_logging_log(data_library->data_logging_session, &data_state, 1);
  // add new node to start of linked list and destroy last one
  DataNode *tmp_node = MALLOC(sizeof(DataNode));
  tmp_node->state = data_state;
  tmp_node->next = NULL;
  prv_linked_list_add_node_start(data_library, tmp_node);
  // if too many nodes, destroy last fifty
  if (data_library->node_count >= 150) {
    DataNode *last_node = prv_linked_list_get_data_node(data_library,
      data_library->node_count - 50);
    prv_linked_list_destroy(data_library, last_node);
  }
}

// Initialize to default values on first launch
static void prv_first_launch_prep(DataLibrary *data_library) {
  // TODO: Port legacy data (treat all as contiguous)
  // write out starting persistent storage location
  persist_write_int(PERSIST_DATA_KEY, PERSIST_DATA_KEY + 1);
  // write out the current battery state
  data_process_new_battery_state(data_library, battery_state_service_peek());
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Get the number of points of history loaded (for run time and max life)
uint16_t data_get_history_points_count(DataLibrary *data_library) {
  // TODO: Implement this function
  return 9;
}

// Get the time the watch needs to be charged by
int32_t data_get_charge_by_time(DataLibrary *data_library) {
  DataState cur_state = prv_get_current_data_state(data_library);
  return cur_state.epoch + DATA_EPOCH_OFFSET + cur_state.percent * (-data_library->charge_rate);
}

// Get the estimated time remaining in seconds
int32_t data_get_life_remaining(DataLibrary *data_library) {
  return data_get_charge_by_time(data_library) - time(NULL);
}

// Get the time the watch was last charged
// Based off app install time if no data
int32_t data_get_last_charge_time(DataLibrary *data_library) {
  // TODO: Implement this function
  return time(NULL) - SEC_IN_DAY;
}

// Get a past run time by its index (0 is current, 1 is yesterday, etc)
// Must be between 0 and DATA_HISTORY_INDEX_MAX
int32_t data_get_past_run_time(DataLibrary *data_library, uint16_t index) {
  // TODO: Implement this function
  return SEC_IN_DAY;
}

// Get the record run time of the watch
// Based off app install time if no data
int32_t data_get_record_run_time(DataLibrary *data_library) {
  // TODO: Implement this function
  // check if current run time is greater than record
  if (data_get_run_time(data_library) > SEC_IN_DAY) { // SEC_IN_DAY should be record life
    return data_get_run_time(data_library);
  }
  return SEC_IN_DAY; // SEC_IN_DAY should be record life
}

// Get the current run time of the watch in seconds
// If no charge data, returns app install time
int32_t data_get_run_time(DataLibrary *data_library) {
  // TODO: Implement this function
  return time(NULL) - SEC_IN_DAY; // SEC_IN_DAY should be last charged
}

// Get the current percent-per-day of battery life
int32_t data_get_percent_per_day(DataLibrary *data_library) {
  return 10000 / (data_get_max_life(data_library) * 100 / SEC_IN_DAY);
}

// Get the current battery percentage (this is an estimate of the exact value)
uint8_t data_get_battery_percent(DataLibrary *data_library) {
  // get current state
  DataState cur_state = prv_get_current_data_state(data_library);
  // calculate exact percent
  int32_t percent = cur_state.percent + (time(NULL) - (cur_state.epoch + DATA_EPOCH_OFFSET)) /
    data_library->charge_rate;
  if (percent > cur_state.percent) {
    percent = cur_state.percent;
  } else if (percent <= cur_state.percent - 10) {
    percent = cur_state.percent - 9;
  }
  if (percent < 0) {
    percent = 0;
  }
  return percent;
}

// Get a past max life by its index (0 is current, 1 is yesterday, etc)
int32_t data_get_past_max_life(DataLibrary *data_library, uint16_t index) {
  // TODO: Implement this function
  return data_get_max_life(data_library);
}

// Get the maximum battery life possible with the current discharge rate
int32_t data_get_max_life(DataLibrary *data_library) {
  return data_library->charge_rate * (-100);
}

// Get a data point by its index with 0 being the most recent
bool data_get_data_point(DataLibrary *data_library, uint16_t index, int32_t *epoch,
                                 uint8_t *percent) {
  // get data node
  DataNode *data_node = prv_linked_list_get_data_node(data_library, index);
  if (data_node) {
    (*epoch) = data_node->state.epoch + DATA_EPOCH_OFFSET;
    (*percent) = data_node->state.percent;
    return true;
  } else {
    return false;
  }
}

// Get the number of data points which include the last x number of seconds
uint16_t data_get_data_point_count_including_seconds(DataLibrary *data_library, int32_t seconds) {
  time_t end_time = time(NULL) - seconds;
  DataNode *cur_node = data_library->head_node;
  uint16_t count = 0;
  while (cur_node) {
    // index count
    count++;
    // check if node
    if (cur_node->state.epoch + DATA_EPOCH_OFFSET < end_time) {
      return count;
    }
    // load more data if next is NULL
    if (!cur_node->next) {
      prv_persist_read_next_data_block(data_library);
    }
    // index node
    cur_node = cur_node->next;
  }
  return count;
}

// Print the data to the console in CSV format
void data_print_csv(DataLibrary *data_library) {
  // print header
  app_log(APP_LOG_LEVEL_INFO, "", 0, "======================================");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Battery+ by Ycleptic Studios");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Raw Data Export, all times in UTC epoch format");
  // print stats
  app_log(APP_LOG_LEVEL_INFO, "", 0, "------------- Statistics -------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Last Charged: %d", (int)SEC_IN_DAY); // TODO: Implement
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Record Life: %d", (int)SEC_IN_DAY); // TODO: Implement
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Rate: %d", (int)data_library->charge_rate);
  // print body
  app_log(APP_LOG_LEVEL_INFO, "", 0, "------------- Raw Data -------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Epoch,\t\t%%,\tCharge,\tPlugged,");
  DataNode *cur_node = data_library->head_node;
  while (cur_node) {
    app_log(APP_LOG_LEVEL_INFO, "", 0, "%d,\t%d,\t%d,\t%d,",
      cur_node->state.epoch + DATA_EPOCH_OFFSET, cur_node->state.percent, cur_node->state.charging,
      cur_node->state.plugged);
    cur_node = cur_node->next;
  }
  app_log(APP_LOG_LEVEL_INFO, "", 0, "=====================================");
}

// Process a BatteryChargeState structure and add it to the data
void data_process_new_battery_state(DataLibrary *data_library,
                                    BatteryChargeState battery_state) {
  // check if duplicate of last point
  DataNode *last_node = prv_linked_list_get_data_node(data_library, 0);
  if (last_node && battery_state.charge_percent == last_node->state.percent &&
      battery_state.is_charging == last_node->state.charging &&
      battery_state.is_plugged == last_node->state.plugged) { return; }
  // create DataState from BatteryChargeState
  DataState data_state = (DataState) {
    .epoch = time(NULL) - DATA_EPOCH_OFFSET,
    .percent = battery_state.charge_percent,
    .charging = battery_state.is_charging,
    .plugged = battery_state.is_plugged,
    .contiguous = data_library->data_is_contiguous
  };
  // process the new unique DataState
  prv_process_data_state(data_library, data_state);
  // set contiguous to true for next data point
  data_library->data_is_contiguous = true;
  // send message to the other part of the app to refresh data (background->foreground) and vv.
  AppWorkerMessage msg_data = { .data0 = 0 };
  app_worker_send_message(0, &msg_data);
}

// Destroy data and reload from persistent storage
void data_reload(DataLibrary *data_library) {
  // clean up data nodes
  prv_linked_list_destroy(data_library, data_library->head_node);
  // prep structure
  data_library->node_count = 0;
  data_library->head_node = NULL;
  data_library->loaded_data_block_count = 0;
  // read data from persistent storage
  if (!persist_exists(PERSIST_DATA_KEY)) {
    prv_first_launch_prep(data_library);
    data_library->loaded_data_block_count = 1;
  } else {
    prv_persist_read_next_data_block(data_library);
  }
}

// Initialize the data
DataLibrary *data_initialize(void) {
  // create new DataLibrary
  DataLibrary *data_library = MALLOC(sizeof(DataLibrary));
  data_library->charge_rate = prv_get_default_charge_rate();
  data_library->node_count = 0;
  data_library->head_node = NULL;
  data_library->data_is_contiguous = false;
  data_library->loaded_data_block_count = 0;
  data_library->data_logging_session = data_logging_create(DATA_LOGGING_TAG,
    DATA_LOGGING_BYTE_ARRAY, sizeof(DataState), true);
  // read data from persistent storage
  if (!persist_exists(PERSIST_DATA_KEY)) {
    prv_first_launch_prep(data_library);
    data_library->loaded_data_block_count = 1;
  } else {
    prv_persist_read_next_data_block(data_library);
  }
  return data_library;
}

// Terminate the data
void data_terminate(DataLibrary *data_library) {
  prv_linked_list_destroy(data_library, data_library->head_node);
  free(data_library);
}
