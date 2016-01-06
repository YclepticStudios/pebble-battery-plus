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
#define DATA_VERSION 0                  //< The current persistent storage format version
#define DATA_BLOCK_SAVE_STATE_COUNT 50  //< Number of SaveStates that fit in one persistent write
#define DATA_EPOCH_OFFSET 1420070400    //< Jan 1, 2015 at 0:00:00, reduces size when saving data
#define LINKED_LIST_MAX_SIZE DATA_BLOCK_SAVE_STATE_COUNT * 3 //< Max size of linked list
#define CYCLE_LINKED_LIST_MAX_SIZE 9    //< The maximum number of nodes in the charge cycle list
#define PERSIST_DATA_KEY 1000           //< The persistent storage key where the data write starts
#define PERSIST_RECORD_LIFE_KEY 999     //< Persistent storage key where the record life is stored
#define DATA_LOGGING_TAG 5155346        //< Tag used to identify data once on phone
// Thresholds
#define CHARGING_MIN_LENGTH 45          //< Minimum duration while charging to register (sec)
#define DISCHARGING_MIN_LENGTH 300      //< Minimum duration when discharging to register (sec)
#define DISCONTIGUOUS_MIN_LENGTH 45     //< Minimum length of discontiguous region to register (sec)


// Structure containing compressed data in the form it will be saved in
typedef struct SaveState {
  unsigned  epoch       : 30;   //< The epoch timestamp for when the percentage changed in seconds
  unsigned  percent     : 7;    //< The battery charge percent
  bool      charging    : 1;    //< The charging state of the battery
  bool      plugged     : 1;    //< The plugged in state of the watch
  bool      contiguous  : 1;    //< Whether the worker stayed active since the last data point
} __attribute__((__packed__)) SaveState;

// Data structure used when saving to and reading from persistent storage
// Includes a header with extra stats
typedef struct SaveStateBlock {
  unsigned    data_version          : 8;            //< The data version format for this block
  signed      initial_charge_rate   : 24;           //< The charge rate after first point in block
  unsigned    padding               : 10;           //< Padding space so structure is 256 bytes
  unsigned    save_state_count      : 6;            //< The number of data points written
  SaveState save_states[DATA_BLOCK_SAVE_STATE_COUNT];  //< The actual data state points being stored
} __attribute__((__packed__)) SaveStateBlock;

// Linked list basic node type structure followed by all linked lists
typedef struct Node {
  struct Node     *next;            //< A pointer to the next node in the linked list
} Node;

// Linked list of data with uncompressed stats, used everywhere except when reading and writing data
typedef struct DataNode {
  struct DataNode *next;            //< Pointer to next node in linked list (must be first)
  int32_t         epoch;            //< Epoch timestamp for when the percentage changed in seconds
  uint8_t         percent;          //< The battery charge percent
  bool            charging;         //< The charging state of the battery
  bool            plugged;          //< The plugged in state of the watch
  bool            contiguous;       //< Whether the worker stayed active since the last data point
  int32_t         charge_rate;      //< The charge rate when at this data point
} DataNode;

// Linked list of charge cycles
typedef struct ChargeCycleNode {
  struct ChargeCycleNode *next;   //< Pointer to next node in linked list (must be first)
  int32_t       charge_epoch;     //< Epoch timestamp for when the charging started
  int32_t       discharge_epoch;  //< Epoch timestamp for when the charging stopped and run started
  int32_t       end_epoch;        //< Epoch timestamp for when charging began for the next cycle
  int32_t       avg_charge_rate;  //< The average charge rate during the discharging (negative)
} ChargeCycleNode;

// Main data structure library
typedef struct DataLibrary {
  uint16_t                node_count;               //< The number of nodes in the linked list
  uint16_t                head_node_index;          //< The index of the head node into the data
  DataNode                *head_node;               //< The head node for linked list, newest first
  uint16_t                cycle_node_count;         //< Number of nodes in charge cycle linked list
  DataNode                *cycle_head_node;         //< Charge cycle linked list head node
  bool                    data_is_contiguous;       //< Whether worker was shut off since last pt
  DataLoggingSessionRef   data_logging_session;     //< Data logging session to send data to phone
} DataLibrary;

// Function declarations
// Read data from persistent storage into a linked list
static void prv_persist_read_data_block(DataLibrary *data_library, uint16_t index);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Set DataNode properties from SaveState
static void prv_set_data_node_from_save_state(DataNode *data_node, SaveState *save_state) {
  data_node->epoch = save_state->epoch + DATA_EPOCH_OFFSET;
  data_node->percent = save_state->percent;
  data_node->charging = save_state->charging;
  data_node->plugged = save_state->plugged;
  data_node->contiguous = save_state->contiguous;
}

// Set SaveState properties from DataNode
static void prv_set_save_state_from_data_node(SaveState *save_state, DataNode *data_node) {
  save_state->epoch = data_node->epoch - DATA_EPOCH_OFFSET;
  save_state->percent = data_node->percent;
  save_state->charging = data_node->charging;
  save_state->plugged = data_node->plugged;
  save_state->contiguous = data_node->contiguous;
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

// Add node to start of linked list
static void prv_linked_list_add_node_start(Node **head_node, Node *node, uint16_t *node_count) {
  node->next = (*head_node);
  (*head_node) = node;
  (*node_count)++;
}

// Add node to end of linked list
static void prv_linked_list_add_node_end(Node **head_node, Node *node, uint16_t *node_count) {
  (*node_count)++;
  if (!(*head_node)) {
    (*head_node) = node;
    return;
  }
  Node *cur_node = (*head_node);
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  cur_node->next = node;
}

// Add node after node
static void prv_linked_list_insert_node_after(DataLibrary *data_library, DataNode *insert_after,
                                              DataNode *node) {
  data_library->node_count++;
  if (!insert_after) {
    node->next = data_library->head_node;
    data_library->head_node = node;
    return;
  }
  node->next = insert_after->next;
  insert_after->next = node;
}

// Destroy all data nodes in linked list
static void prv_linked_list_destroy(Node **head_node, uint16_t *node_count) {
  Node *cur_node = (*head_node);
  Node *tmp_node;
  while (cur_node) {
    tmp_node = cur_node;
    cur_node = cur_node->next;
    free(tmp_node);
    (*node_count)--;
  }
  (*head_node) = NULL;
}

// Get the last node in the linked list
static DataNode* prv_linked_list_get_last_node(DataLibrary *data_library) {
  if (!data_library->head_node) {
    return NULL;
  }
  DataNode *cur_node = data_library->head_node;
  while (cur_node->next) {
    cur_node = cur_node->next;
  }
  return cur_node;
}

// Get a node from a certain index of a linked list
static Node* prv_linked_list_get_node_by_index(Node *head_node, uint16_t index) {
  // loop over nodes and find matching index
  Node *cur_node = head_node;
  uint16_t cur_index = 0;
  while (cur_node) {
    if (cur_index == index) {
      return cur_node;
    }
    cur_index++;
    cur_node = cur_node->next;
  }
  return NULL;
}

// Get a DataNode at a certain index into the linked list, with 0 being most recent
// If the index is outside the current cache, destroy the cache and create a new one
static DataNode* prv_list_get_data_node(DataLibrary *data_library, uint16_t index) {
  // check if outside the currently cached linked list and create new cache
  if (index < data_library->head_node_index ||
      index >= data_library->head_node_index + LINKED_LIST_MAX_SIZE) {
    // read in new data
    prv_persist_read_data_block(data_library, index);
  }
  // loop over nodes and find matching index
  DataNode *cur_node = data_library->head_node;
  uint16_t cur_index = data_library->head_node_index;
  while (cur_node) {
    if (cur_index == index) {
      return cur_node;
    }
    cur_index++;
    cur_node = cur_node->next;
  }
  return NULL;
}

// Get the latest data node (returns a fake data node with the current battery value if no nodes)
static DataNode prv_get_current_data_node(DataLibrary *data_library) {
  // get current battery state
  BatteryChargeState bat_state = battery_state_service_peek();
  DataNode fake_node = (DataNode) {
    .epoch = time(NULL),
    .percent = bat_state.charge_percent,
    .charging = bat_state.is_charging,
    .plugged = bat_state.is_plugged,
    .charge_rate = prv_get_default_charge_rate()
  };
  // get latest existing data point
  DataNode *cur_node = prv_list_get_data_node(data_library, 0);
  // return current state if no last node or not matching with last node
  if (!cur_node || cur_node->percent != fake_node.percent ||
    cur_node->charging != fake_node.charging ||
    cur_node->plugged != fake_node.plugged) {
    return fake_node;
  }
  return *cur_node;
}

// Check if two data points are contiguous
static bool prv_are_save_states_contiguous(SaveState old_state, SaveState new_state,
                                           int32_t charge_rate) {
  // check if listed as contiguous
  if (new_state.contiguous) { return true; }
  if (!old_state.contiguous || new_state.percent > old_state.percent) {return false; }
  // otherwise predict when the new state should occur and compare to the actual time
  // Note: the epoch offset does not matter in this case
  int32_t predicted_epoch = old_state.epoch + (-10) * charge_rate;
  // value must be less than 1 / 2 the difference of the old and new states
  if (new_state.epoch < predicted_epoch ||
    new_state.epoch - predicted_epoch < (new_state.epoch - old_state.epoch) / 2) {
    return true;
  }
  // otherwise they are not contiguous
  return false;
}

// Calculate the charge rate
static int32_t prv_calculate_charge_rate(SaveState old_state, SaveState new_state,
                                         int32_t charge_rate) {
  // check if qualifies for a charge rate calculation
  if (new_state.epoch <= old_state.epoch || new_state.percent >= old_state.percent ||
    new_state.charging || old_state.charging ||
    !prv_are_save_states_contiguous(old_state, new_state, charge_rate)) {
    return charge_rate;
  } else {
    return charge_rate * 4 / 5 +
      ((new_state.epoch - old_state.epoch) / (new_state.percent - old_state.percent)) / 5;
  }
}

// Create a ChargeCycleNode and add it to the linked list
static ChargeCycleNode* prv_create_charge_cycle_node(DataLibrary *data_library) {
    ChargeCycleNode *charge_node = MALLOC(sizeof(ChargeCycleNode));
    memset(charge_node, 0, sizeof(ChargeCycleNode));
    prv_linked_list_add_node_end((Node**)&data_library->cycle_head_node, (Node*)charge_node,
      &data_library->cycle_node_count);
  return charge_node;
}

// Process data and calculate charge cycles
static void prv_calculate_charge_cycles(DataLibrary *data_library) {
  // TODO: Fix the value for the "Run Stop" of the current run time
  // clear any existing charge cycles
  prv_linked_list_destroy((Node**)&data_library->cycle_head_node, &data_library->cycle_node_count);
  // data properties
  bool contiguous;
  int32_t last_transition = time(NULL) + DISCHARGING_MIN_LENGTH;
  uint16_t charge_rate_count = 0;
  int32_t charge_rate_avg = 0;
  typedef enum DataType { TypeNotContiguous, TypeCharging, TypeDischarging, TypeFirstRun } DataType;
  DataType cur_type = TypeFirstRun, next_type;
  int32_t min_length_threshold[] = { DISCHARGING_MIN_LENGTH, CHARGING_MIN_LENGTH,
    DISCHARGING_MIN_LENGTH };
  ChargeCycleNode *charge_node = NULL;
  // prep for looping over data
  uint16_t index = 0;
  DataNode *cur_node = prv_list_get_data_node(data_library, index++);
  DataNode *next_node = prv_list_get_data_node(data_library, index++);
  SaveState cur_state, next_state;
  prv_set_save_state_from_data_node(&cur_state, cur_node);
  // loop over data
  while (cur_node && next_node && data_library->cycle_node_count <= CYCLE_LINKED_LIST_MAX_SIZE) {
    // get if contiguous
    prv_set_save_state_from_data_node(&next_state, next_node);
    contiguous = prv_are_save_states_contiguous(next_state, cur_state, next_node->charge_rate);
    // get data type
    if (!contiguous) {
      next_type = TypeNotContiguous;
    } else if (next_node->charging) {
      next_type = TypeCharging;
    } else {
      next_type = TypeDischarging;
      charge_rate_avg += next_node->charge_rate;
      charge_rate_count++;
    }
    // if change in type
    if (next_type != cur_type && cur_type != TypeFirstRun) {
      // filter out small transitions
      if (last_transition - cur_node->epoch >= min_length_threshold[cur_type]) {
        // apply truth table of what to do for different transitions
        if (cur_type == TypeCharging || next_type == TypeNotContiguous) {
          if (!charge_node) {
            charge_node = prv_create_charge_cycle_node(data_library);
          }
          charge_node->charge_epoch = cur_node->epoch;
        }
        if (cur_type == TypeNotContiguous ||
          (cur_type == TypeCharging && next_type == TypeDischarging)) {
          charge_node = prv_create_charge_cycle_node(data_library);
          charge_node->end_epoch = cur_node->epoch;
        }
        if (cur_type == TypeDischarging || next_type == TypeCharging) {
          if (!charge_node) {
            charge_node = prv_create_charge_cycle_node(data_library);
          }
          charge_node->discharge_epoch = cur_node->epoch;
          charge_node->avg_charge_rate = charge_rate_avg / charge_rate_count;
          charge_rate_avg = charge_rate_count = 0;
        }
        // store new type
        cur_type = next_type;
      }
      // update last transition time
      last_transition = cur_node->epoch;
    } else if (cur_type == TypeFirstRun) {
      cur_type = next_type;
    }
    // index
    cur_state = next_state;
    cur_node = next_node;
    next_node = prv_list_get_data_node(data_library, index++);
  }
}

// Read data from persistent storage into a linked list
static void prv_persist_read_data_block(DataLibrary *data_library, uint16_t index) {
  // prep linked list
  prv_linked_list_destroy((Node**)&data_library->head_node, &data_library->node_count);
  data_library->head_node = NULL;
  data_library->head_node_index = index / DATA_BLOCK_SAVE_STATE_COUNT * DATA_BLOCK_SAVE_STATE_COUNT;
  // get persistent storage key
  uint32_t persist_key = persist_read_int(PERSIST_DATA_KEY) - index / DATA_BLOCK_SAVE_STATE_COUNT;
  if (!persist_exists(persist_key)) { persist_key--; }
  // read in several blocks of data
  SaveStateBlock save_state_block;
  for (uint32_t key = persist_key;
       key > persist_key - 3 && key > PERSIST_DATA_KEY && persist_exists(key); key--) {
    // read the data from persistent storage
    persist_read_data(key, &save_state_block, sizeof(SaveStateBlock));
    // loop over data
    int32_t tmp_charge_rate = 0;
    DataNode *tmp_node, *insert_after_node = prv_linked_list_get_last_node(data_library);
    for (uint8_t ii = 0; ii < save_state_block.save_state_count; ii++) {
      // add node to linked list
      tmp_node = MALLOC(sizeof(DataNode));
      prv_set_data_node_from_save_state(tmp_node, &save_state_block.save_states[ii]);
      tmp_node->next = NULL;
      if (ii == 0) {
        tmp_node->charge_rate = tmp_charge_rate = save_state_block.initial_charge_rate;
      } else {
        tmp_charge_rate = prv_calculate_charge_rate(save_state_block.save_states[ii - 1],
          save_state_block.save_states[ii], tmp_charge_rate);
        tmp_node->charge_rate = tmp_charge_rate;
      }
      prv_linked_list_insert_node_after(data_library, insert_after_node, tmp_node);
    }
  }
}

// Write newest DataNode into persistent storage
static void prv_persist_write_data_node(DataLibrary *data_library, DataNode *data_node) {
  // get the location and size of the data
  uint32_t persist_key = persist_read_int(PERSIST_DATA_KEY);
  // build a SaveStateBlock to write into memory
  SaveStateBlock save_state_block = (SaveStateBlock) {
    .data_version = DATA_VERSION,
    .initial_charge_rate = data_node->charge_rate,
    .save_state_count = 0
  };
  if (persist_exists(persist_key)) {
    persist_read_data(persist_key, &save_state_block, sizeof(SaveStateBlock));
  }
  prv_set_save_state_from_data_node(&save_state_block.save_states[save_state_block
    .save_state_count++], data_node);
  // get the oldest existing key
  uint32_t old_persist_key = persist_key;
  while (old_persist_key > PERSIST_DATA_KEY && persist_exists(old_persist_key - 1)) {
    old_persist_key--;
  }
  // attempt to write the data and delete old data if the write fails,
  // but do not delete any closer than the last three data blocks
  uint16_t bytes_written = persist_write_data(persist_key, &save_state_block,
    sizeof(SaveStateBlock));
  while (bytes_written < sizeof(SaveStateBlock) && old_persist_key + 3 < persist_key) {
    persist_delete(old_persist_key++);
    bytes_written = persist_write_data(persist_key, &save_state_block, sizeof(SaveStateBlock));
  }
  // check if SaveStateBlock is full and index to next key
  if (save_state_block.save_state_count >= DATA_BLOCK_SAVE_STATE_COUNT) {
    persist_write_int(PERSIST_DATA_KEY, ++persist_key);
  }
}

// Process a SaveState structure by calculating stats and persisting
static void prv_process_save_state(DataLibrary *data_library, SaveState save_state) {
  // TODO: Calculate record life
  // TODO: Schedule wake-up alert
  // add new node to start of linked list
  DataNode *new_node = MALLOC(sizeof(DataNode));
  prv_set_data_node_from_save_state(new_node, &save_state);
  new_node->next = NULL;
  DataNode *lst_node = prv_list_get_data_node(data_library, 0);
  SaveState lst_save_state;
  prv_set_save_state_from_data_node(&lst_save_state, lst_node);
  if (lst_node) {
    new_node->charge_rate = prv_calculate_charge_rate(lst_save_state, save_state,
      lst_node->charge_rate);
  } else {
    new_node->charge_rate = prv_get_default_charge_rate();
  }
  prv_linked_list_add_node_start((Node**)&data_library->head_node, (Node*)new_node,
    &data_library->node_count);
  // destroy last node
  DataNode *old_node = prv_list_get_data_node(data_library, data_library->node_count - 2);
  if (old_node) {
    free(old_node->next);
    old_node->next = NULL;
    data_library->node_count--;
  }
  // persist the data point
  prv_persist_write_data_node(data_library, new_node);
  // send the data to the phone with data logging
  data_logging_log(data_library->data_logging_session, new_node, 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// First Time Launch
//

// data constants
#define LEGACY_PERSIST_DATA 100
#define LEGACY_PERSIST_DATA_LENGTH 24 // must be multiple of 4
#define LEGACY_DATA_SIZE 100
#define LEGACY_EPOCH_OFFSET 1420070400

// Convert old data format to new data format
static void prv_persist_convert_legacy_data(DataLibrary *data_library) {
  // get some values
  uint32_t myData[LEGACY_DATA_SIZE];
  uint8_t *ptr = (uint8_t*)&myData;
  uint16_t step = LEGACY_PERSIST_DATA_LENGTH, size = sizeof(myData), Pers_Val = LEGACY_PERSIST_DATA;
  // load myIndex and myCount
  uint16_t myIndex = (uint16_t)persist_read_int(Pers_Val);
  persist_delete(Pers_Val++);
  uint16_t myCount = (uint16_t)persist_read_int(Pers_Val);
  persist_delete(Pers_Val++);
  int32_t myRecord = persist_read_int(Pers_Val);
  persist_delete(Pers_Val++);
  // read the array in several pieces
  for (uint16_t delta = 0; delta < size; delta += step) {
    persist_read_data(Pers_Val, ptr + delta,
      (delta + step < size) ? step : (size % step));
    persist_delete(Pers_Val++);
  }
  // send data to new code for processing
  int16_t idx = -1;
  if (myCount >= LEGACY_DATA_SIZE) {
    idx = myIndex;
  }
  SaveState save_state;
  // loop through data
  for (uint16_t ii = 0; ii < 999; ii++) {
    // index
    idx++;
    if (idx >= myCount && myCount >= LEGACY_DATA_SIZE) idx = 0;
    else if (idx >= myCount || (idx == myIndex && ii > 0)) break;
    // unpack data point
    uint32_t val = myData[idx];
    save_state.epoch = ((val / 44) * 60) + LEGACY_EPOCH_OFFSET - DATA_EPOCH_OFFSET;
    save_state.percent = ((val %= 44) / 4) * 10;
    save_state.charging = ((val %= 4) / 2);
    save_state.plugged = (val % 2);
    save_state.contiguous = true;
    // process data node
    prv_process_save_state(data_library, save_state);
  }
}

// Initialize to default values on first launch
static void prv_first_launch_prep(DataLibrary *data_library) {
  // write out starting persistent storage location
  persist_write_int(PERSIST_DATA_KEY, PERSIST_DATA_KEY + 1);
  // port any legacy data
  if (persist_exists(LEGACY_PERSIST_DATA)) {
    prv_persist_convert_legacy_data(data_library);
  }
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
  DataNode cur_node = prv_get_current_data_node(data_library);
  return cur_node.epoch + cur_node.percent * (-cur_node.charge_rate);
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
  // get current node
  DataNode cur_node = prv_get_current_data_node(data_library);
  // calculate exact percent
  int32_t percent = cur_node.percent +
    (time(NULL) - cur_node.epoch) / cur_node.charge_rate;
  if (percent > cur_node.percent) {
    percent = cur_node.percent;
  } else if (percent <= cur_node.percent - 10) {
    percent = cur_node.percent - 9;
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
  DataNode cur_node = prv_get_current_data_node(data_library);
  return cur_node.charge_rate * (-100);
}

// Get a data point by its index with 0 being the most recent
bool data_get_data_point(DataLibrary *data_library, uint16_t index, int32_t *epoch,
                                 uint8_t *percent) {
  // get data node
  DataNode *data_node = prv_list_get_data_node(data_library, index);
  if (data_node) {
    (*epoch) = data_node->epoch;
    (*percent) = data_node->percent;
    return true;
  } else {
    return false;
  }
}

// Get the number of charge cycles which include the last x number of seconds
uint16_t data_get_charge_cycle_count_including_seconds(DataLibrary *data_library, int32_t seconds) {
  // TODO: Implement this function
  return 0;
}

// Get the number of data points which include the last x number of seconds
uint16_t data_get_data_point_count_including_seconds(DataLibrary *data_library, int32_t seconds) {
  time_t end_time = time(NULL) - seconds;
  uint16_t index = 0;
  DataNode *cur_node = prv_list_get_data_node(data_library, index);
  while (cur_node) {
    index++;
    if (cur_node->epoch < end_time) { break; }
    cur_node = prv_list_get_data_node(data_library, index);
  }
  return index;
}

// Print the data to the console in CSV format
void data_print_csv(DataLibrary *data_library) {
  DataNode cur_data_node = prv_get_current_data_node(data_library);
  // print header
  app_log(APP_LOG_LEVEL_INFO, "", 0, "=====================================================");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Battery+ by Ycleptic Studios");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Raw Data Export, all times in UTC epoch format");
  // print stats
  app_log(APP_LOG_LEVEL_INFO, "", 0, "--------------------- Statistics --------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Current Time: %d", (int)time(NULL));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Last Charged: %d", (int)SEC_IN_DAY); // TODO: Implement
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Record Life: %d", (int)SEC_IN_DAY); // TODO: Implement
  // TODO: Implement all statistics
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Rate: %d", (int)cur_data_node.charge_rate);
  // print interpreted charge cycles
  app_log(APP_LOG_LEVEL_INFO, "", 0, "------------------- Charge Cycles -------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Start,\tRun Start,\tRun Stop,\tAvg Charge "
    "Rate,");
  uint16_t cycle_count = 0;
  ChargeCycleNode *cur_cycle_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index
    ((Node*)data_library->cycle_head_node, cycle_count);
  while (cur_cycle_node) {
    cycle_count++;
    app_log(APP_LOG_LEVEL_INFO, "", 0, "%d,\t%d,\t%d,\t%d,",
      (int)cur_cycle_node->charge_epoch, (int)cur_cycle_node->discharge_epoch,
      (int)cur_cycle_node->end_epoch, (int)cur_cycle_node->avg_charge_rate);
    cur_cycle_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index
      ((Node*)data_library->cycle_head_node, cycle_count);
  }

  // print raw data points
  app_log(APP_LOG_LEVEL_INFO, "", 0, "---------------------- Raw Data ---------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Epoch,\t\tPerc,\tChar,\tPlug,\tContig,\tCharge Rate,");
  uint16_t data_count = 0;
  DataNode *cur_node = prv_list_get_data_node(data_library, data_count);
  while (cur_node) {
    data_count++;
    app_log(APP_LOG_LEVEL_INFO, "", 0, "%d,\t%d,\t%d,\t%d,\t%d,\t%d,",
      (int)cur_node->epoch, (int)cur_node->percent, (int)cur_node->charging, (int)cur_node->plugged,
      (int)cur_node->contiguous, (int)cur_node->charge_rate);
    cur_node = prv_list_get_data_node(data_library, data_count);
  }
  app_log(APP_LOG_LEVEL_INFO, "", 0, "-----------------------------------------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Cycle Count: %d", cycle_count);
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Data Point Count: %d", data_count);
  app_log(APP_LOG_LEVEL_INFO, "", 0, "=====================================================");
}

// Process a BatteryChargeState structure and add it to the data
void data_process_new_battery_state(DataLibrary *data_library,
                                    BatteryChargeState battery_state) {
  // check if duplicate of last point
  DataNode *last_node = prv_list_get_data_node(data_library, 0);
  if (last_node && battery_state.charge_percent == last_node->percent &&
      battery_state.is_charging == last_node->charging &&
      battery_state.is_plugged == last_node->plugged) { return; }
  // create SaveState from BatteryChargeState
  SaveState save_state = (SaveState) {
    .epoch = time(NULL) - DATA_EPOCH_OFFSET,
    .percent = battery_state.charge_percent,
    .charging = battery_state.is_charging,
    .plugged = battery_state.is_plugged,
    .contiguous = data_library->data_is_contiguous
  };
  // process the new unique SaveState
  prv_process_save_state(data_library, save_state);
  // set contiguous to true for next data point
  data_library->data_is_contiguous = true;
  // send message to the other part of the app to refresh data (background->foreground) and vv.
  AppWorkerMessage msg_data = { .data0 = 0 };
  app_worker_send_message(0, &msg_data);
}

// Destroy data and reload from persistent storage
void data_reload(DataLibrary *data_library) {
  // clean up data nodes
  prv_linked_list_destroy((Node**)&data_library->cycle_head_node, &data_library->cycle_node_count);
  prv_linked_list_destroy((Node**)&data_library->head_node, &data_library->node_count);
  // prep structure
  data_library->head_node_index = 0;
  // read data from persistent storage
  if (!persist_exists(PERSIST_DATA_KEY)) {
    prv_first_launch_prep(data_library);
  } else {
    prv_persist_read_data_block(data_library, 0);
    prv_calculate_charge_cycles(data_library);
  }
}

// Initialize the data
DataLibrary *data_initialize(void) {
  // create new DataLibrary
  DataLibrary *data_library = MALLOC(sizeof(DataLibrary));
  data_library->node_count = 0;
  data_library->head_node_index = 0;
  data_library->head_node = NULL;
  data_library->data_is_contiguous = false;
  data_library->data_logging_session = data_logging_create(DATA_LOGGING_TAG,
    DATA_LOGGING_BYTE_ARRAY, sizeof(DataNode), true);
  // read data from persistent storage
  if (!persist_exists(PERSIST_DATA_KEY)) {
    prv_first_launch_prep(data_library);
  } else {
    prv_persist_read_data_block(data_library, 0);
    prv_calculate_charge_cycles(data_library);
  }
  return data_library;
}

// Terminate the data
void data_terminate(DataLibrary *data_library) {
  prv_linked_list_destroy((Node**)&data_library->cycle_head_node, &data_library->cycle_node_count);
  prv_linked_list_destroy((Node**)&data_library->head_node, &data_library->node_count);
  free(data_library);
}
