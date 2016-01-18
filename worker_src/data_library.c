// @file data_library.c
// @brief All code for reading, writing, and processing data
//
// Contains all code used to read and write data, as well as the
// code used to process the data and store the results.
//
// @author Eric D. Phillips
// @date January 2, 2015
// @bugs No known bugs

#include "data_library.h"
#define PEBBLE_BACKGROUND_WORKER
#include "../src/data/data_shared.h"
#include "../src/utility.h"
#undef PEBBLE_BACKGROUND_WORKER

// Constants
#define DATA_VERSION 0                  //< The current persistent storage format version
#define LOW_THRESH_DEFAULT 4 * SEC_IN_HR  //< Default threshold for low level
#define MED_THRESH_DEFAULT SEC_IN_DAY   //< Default threshold for medium level
#define DATA_BLOCK_SAVE_STATE_COUNT 50  //< Number of SaveStates that fit in one persistent write
#define DATA_EPOCH_OFFSET 1420070400    //< Jan 1, 2015 at 0:00:00, reduces size when saving data
#define LINKED_LIST_MAX_SIZE DATA_BLOCK_SAVE_STATE_COUNT //< Max size of linked list
#define CYCLE_LINKED_LIST_MIN_SIZE 9    //< The minimum number of nodes in the charge cycle list
// Thresholds
#define CHARGING_MIN_LENGTH 60          //< Minimum duration while charging to register (sec)
#define DISCHARGING_MIN_FRACTION 1 / 10 //< Minimum fraction of default run time to register


// AppTimer custom data struct
typedef struct {
  AppTimer      *app_timer;       //< The AppTimer which will own this struct as data
  DataLibrary   *data_library;    //< Main data library
  uint8_t       index;            //< Index of timer/alert
} AppTimerData;

// Battery alert data structure
typedef struct {
  int32_t   thresholds[DATA_ALERT_MAX_COUNT];   //< Number of seconds before empty for alert
  uint8_t   scheduled_count;                    //< The total number of currently scheduled alerts
} AlertData;

// Structure containing compressed data in the form it will be saved in
typedef struct {
  unsigned  epoch       : 30;   //< The epoch timestamp for when the percentage changed in seconds
  unsigned  percent     : 7;    //< The battery charge percent
  bool      charging    : 1;    //< The charging state of the battery
  bool      plugged     : 1;    //< The plugged in state of the watch
  bool      contiguous  : 1;    //< Whether the worker stayed active since the last data point
} __attribute__((__packed__)) SaveState;

// Data structure used when saving to and reading from persistent storage
// Includes a header with extra stats
typedef struct {
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
  bool                    data_is_contiguous;       //< Whether worker was shut off since last pt
  uint16_t                cycle_node_count;         //< Number of nodes in charge cycle linked list
  ChargeCycleNode         *cycle_head_node;         //< Charge cycle linked list head node
  AlertData               alert_data;               //< Data for battery low alerts
  AppTimerData            app_timers[DATA_ALERT_MAX_COUNT];  //< AppTimer data for alerts
  BatteryAlertCallback    alert_callback;           //< The function to call when an alert goes off
  DataLoggingSessionRef   data_logging_session;     //< Data logging session to send data to phone
} DataLibrary;

// Function declarations
// Read data from persistent storage into a linked list
static void prv_persist_read_data_block(DataLibrary *data_library, uint16_t index);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// AppTimer callback for alerts
static void prv_app_timer_alert_callback(void *data) {
  AppTimerData *timer_data = data;
  DataLibrary *data_library = timer_data->data_library;
  // raise callback
  if (data_library->alert_callback) {
    data_library->alert_callback(timer_data->index);
  }
  // clean up timer
  uint8_t index = timer_data->index;
  free(timer_data);
  data_library->app_timers[index].app_timer = NULL;
}

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
      index >= data_library->head_node_index + data_library->node_count) {
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

// Filter ChargeCycleNodes removing ones with too short run times or charge times
static void prv_filter_charge_cycles(DataLibrary *data_library, bool filter_last_node) {
  // loop over nodes and find matching index
  bool pending_delete = false;
  ChargeCycleNode **tmp_node = &data_library->cycle_head_node;
  ChargeCycleNode *cur_node = data_library->cycle_head_node;
  while (cur_node && (filter_last_node || cur_node->next)) {
    // check if too short a duration
    if (cur_node->end_epoch &&
        cur_node->end_epoch - cur_node->discharge_epoch <
        cur_node->avg_charge_rate * (-100) * DISCHARGING_MIN_FRACTION) {
      pending_delete = true;
    }
    // index and/or delete nodes
    if (pending_delete) {
      (*tmp_node) = cur_node->next;
      free(cur_node);
      cur_node = (*tmp_node);
      data_library->cycle_node_count--;
      pending_delete = false;
    } else {
      tmp_node = &cur_node->next;
      cur_node = cur_node->next;
    }
  }
}

// Create a ChargeCycleNode and add it to the linked list
static ChargeCycleNode* prv_create_charge_cycle_node(DataLibrary *data_library,
                                                     int32_t charge_rate) {
  ChargeCycleNode *charge_node = MALLOC(sizeof(ChargeCycleNode));
  memset(charge_node, 0, sizeof(ChargeCycleNode));
  charge_node->avg_charge_rate = charge_rate;
  prv_linked_list_add_node_end((Node**)&data_library->cycle_head_node, (Node*)charge_node,
    &data_library->cycle_node_count);
  return charge_node;
}

// Process data and calculate charge cycles
static void prv_calculate_charge_cycles(DataLibrary *data_library, uint16_t min_cycle_count) {
  // clear any existing charge cycles
  prv_linked_list_destroy((Node**)&data_library->cycle_head_node, &data_library->cycle_node_count);
  // data type
  typedef enum DataType { TypeCharging, TypeDischarging, TypeNotContiguous, TypeFirstRun } DataType;
  DataType cur_type, lst_type = TypeFirstRun, lst_set_type = TypeFirstRun;
  // create new ChargeCycleNode
  ChargeCycleNode *charge_node = NULL;
  uint16_t charge_rate_count = 0;
  int32_t charge_rate_avg = 0;
  // loop over data
  uint16_t index = 0;
  DataNode *cur_node = prv_list_get_data_node(data_library, index++);
  SaveState cur_state, lst_state = { .epoch = 0 };
  while (cur_node && data_library->cycle_node_count < min_cycle_count + 1) {
    // calculate data type
    prv_set_save_state_from_data_node(&cur_state, cur_node);
    if (index == 0) {
      cur_type = TypeFirstRun;
    } else if (!prv_are_save_states_contiguous(cur_state, lst_state, cur_node->charge_rate)) {
      cur_type = TypeNotContiguous;
    } else if (cur_node->charging) {
      cur_type = TypeCharging;
    } else {
      cur_type = TypeDischarging;
      charge_rate_avg += cur_node->charge_rate;
      charge_rate_count++;
    } if (lst_type == TypeFirstRun) {
      lst_type = lst_set_type = cur_type;
    }
    // check if at type transition
    if (cur_type != lst_set_type) {
      // apply truth table of what to do for different transitions
      if ((lst_set_type == TypeCharging || cur_type == TypeNotContiguous) &&
        lst_set_type != TypeFirstRun) {
        if (!charge_node) {
          charge_node = prv_create_charge_cycle_node(data_library, cur_node->charge_rate);
        }
        charge_node->charge_epoch = lst_state.epoch + DATA_EPOCH_OFFSET;
      }
      if (lst_set_type == TypeNotContiguous ||
          (lst_set_type == TypeCharging && cur_type == TypeDischarging)) {
        charge_node = prv_create_charge_cycle_node(data_library, cur_node->charge_rate);
        charge_node->charge_epoch = charge_node->discharge_epoch = charge_node->end_epoch =
          lst_state.epoch + DATA_EPOCH_OFFSET;
      }
      if (lst_set_type == TypeDischarging || cur_type == TypeCharging) {
        if (!charge_node) {
          charge_node = prv_create_charge_cycle_node(data_library, cur_node->charge_rate);
        }
        charge_node->discharge_epoch = lst_state.epoch + DATA_EPOCH_OFFSET;
        charge_node->avg_charge_rate = charge_rate_avg / charge_rate_count;
        charge_rate_avg = charge_rate_count = 0;
      }
      // log change
      lst_set_type = cur_type;
      lst_type = cur_type;
    }
    // index
    lst_state = cur_state;
    cur_node = prv_list_get_data_node(data_library, index++);
    // filter to remove short cycles
    prv_filter_charge_cycles(data_library, false);
  }
  // final filter to remove last cycle if too short
  prv_filter_charge_cycles(data_library, true);
}

// Read data from persistent storage into a linked list
static void prv_persist_read_data_block(DataLibrary *data_library, uint16_t index) {
  // prep linked list
  prv_linked_list_destroy((Node**)&data_library->head_node, &data_library->node_count);
  data_library->head_node = NULL;
  data_library->head_node_index = index / DATA_BLOCK_SAVE_STATE_COUNT * DATA_BLOCK_SAVE_STATE_COUNT;
  // get persistent storage key
  uint32_t persist_key = persist_read_int(PERSIST_DATA_KEY);
  if (!persist_exists(persist_key)) { persist_key--; }
  if (persist_key <= PERSIST_DATA_KEY || !persist_exists(persist_key)) { return; }
  // offset the persistent key to where the data is stored
  SaveStateBlock save_state_block;
  persist_read_data(persist_key, &save_state_block, sizeof(SaveStateBlock));
  if (index % DATA_BLOCK_SAVE_STATE_COUNT >= save_state_block.save_state_count) {
    persist_key--;
    data_library->head_node_index += save_state_block.save_state_count;
  }
  persist_key -= index / DATA_BLOCK_SAVE_STATE_COUNT;
  // read in several blocks of data
  for (uint32_t key = persist_key;
       key > persist_key - (LINKED_LIST_MAX_SIZE / DATA_BLOCK_SAVE_STATE_COUNT) &&
         key > PERSIST_DATA_KEY && persist_exists(key); key--) {
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
  // calculate record run time
  int32_t run_time = data_get_run_time(data_library, 0);
  if (!persist_exists(PERSIST_RECORD_LIFE_KEY) ||
    persist_read_int(PERSIST_RECORD_LIFE_KEY) < run_time) {
    persist_write_int(PERSIST_RECORD_LIFE_KEY, run_time);
  }
  // add new node to start of linked list
  DataNode *new_node = MALLOC(sizeof(DataNode));
  prv_set_data_node_from_save_state(new_node, &save_state);
  new_node->next = NULL;
  DataNode *lst_node = prv_list_get_data_node(data_library, 0);
  if (lst_node) {
    SaveState lst_save_state;
    prv_set_save_state_from_data_node(&lst_save_state, lst_node);
    new_node->charge_rate = prv_calculate_charge_rate(lst_save_state, save_state,
      lst_node->charge_rate);
  } else {
    new_node->charge_rate = prv_get_default_charge_rate();
  }
  prv_linked_list_add_node_start((Node**)&data_library->head_node, (Node*)new_node,
    &data_library->node_count);
  // destroy last node
  if (data_library->node_count > DATA_BLOCK_SAVE_STATE_COUNT) {
    DataNode *old_node = prv_list_get_data_node(data_library, data_library->node_count - 2);
    if (old_node) {
      free(old_node->next);
      old_node->next = NULL;
      data_library->node_count--;
    }
  }
  // persist the data point
  prv_persist_write_data_node(data_library, new_node);
  // recalculate charge cycles
  if (lst_node) {
    if (lst_node->charging || new_node->charging ||
      !lst_node->contiguous || !new_node->contiguous) {
      prv_calculate_charge_cycles(data_library, CYCLE_LINKED_LIST_MIN_SIZE);
    }
  }
  // schedule wake-up low battery alert
  data_refresh_all_alerts(data_library);
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
  // set alerts
  data_schedule_alert(data_library, LOW_THRESH_DEFAULT);
  data_schedule_alert(data_library, MED_THRESH_DEFAULT);
  // write out starting persistent storage location
  persist_write_int(PERSIST_DATA_KEY, PERSIST_DATA_KEY + 1);
  // port any legacy data
  if (persist_exists(LEGACY_PERSIST_DATA)) {
    prv_persist_convert_legacy_data(data_library);
    // don't let the record be set by the old data
    persist_delete(PERSIST_RECORD_LIFE_KEY);
  }
  // write out the current battery state
  data_process_new_battery_state(data_library, battery_state_service_peek());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Get the alert level threshold in seconds, this is the time remaining when the alert goes off
int32_t data_get_alert_threshold(DataLibrary *data_library, uint8_t index) {
  return data_library->alert_data.thresholds[index];
}

// Get the number of scheduled alerts at the current time
//! Note: This function reads from persistent storage if a DataLibrary is NULL
uint8_t data_get_alert_count(DataLibrary *data_library) {
  if (data_library) {
    return data_library->alert_data.scheduled_count;
  } else {
    AlertData alert_data;
    persist_read_data(PERSIST_ALERTS_KEY, &alert_data, persist_get_size(PERSIST_ALERTS_KEY));
    return alert_data.scheduled_count;
  }
}

// Refresh all alerts and schedule timers which will do the actual waking up
void data_refresh_all_alerts(DataLibrary *data_library) {
  AlertData *alert_data = &data_library->alert_data;
  // load from persistent storage
  persist_read_data(PERSIST_ALERTS_KEY, alert_data, persist_get_size(PERSIST_ALERTS_KEY));
  // cancel all existing alerts and reschedule new ones
  time_t delay_time, time_remaining = data_get_life_remaining(data_library);
  for (uint8_t index = 0; index < alert_data->scheduled_count; index++) {
    // get time remaining
    delay_time = (time_remaining - alert_data->thresholds[index]) * 1000;
    // cancel, reschedule, and schedule timers
    if (data_library->app_timers[index].app_timer) {
      if (delay_time > 0) {
        // reschedule the timer
        app_timer_reschedule(data_library->app_timers[index].app_timer, delay_time);
      } else {
        // the new charge rate puts this timer in the past, so raise it's event now
        app_timer_cancel(data_library->app_timers[index].app_timer);
        prv_app_timer_alert_callback(&data_library->app_timers[index]);
      }
    } else if (delay_time > 0) {
      // schedule a new timer
      data_library->app_timers[index].data_library = data_library;
      data_library->app_timers[index].index = index;
      data_library->app_timers[index].app_timer = app_timer_register(delay_time,
        prv_app_timer_alert_callback, &data_library->app_timers[index]);
    }
  }
}

// Create a new alert at a certain threshold
void data_schedule_alert(DataLibrary *data_library, int32_t seconds) {
  AlertData *alert_data = &data_library->alert_data;
  // reload from persistent storage
  persist_read_data(PERSIST_ALERTS_KEY, alert_data, persist_get_size(PERSIST_ALERTS_KEY));
  // loop over alerts and add in proper position (sorted short to long)
  uint8_t index;
  for (index = 0; index < alert_data->scheduled_count; index++) {
    if (seconds < alert_data->thresholds[index]) { break; }
  }
  // if full delete last alert
  if (alert_data->scheduled_count >= DATA_ALERT_MAX_COUNT) {
    data_unschedule_alert(data_library, DATA_ALERT_MAX_COUNT - 1);
  }
  // move alerts to make room
  memmove(&alert_data->thresholds[index + 1], &alert_data->thresholds[index],
    (DATA_ALERT_MAX_COUNT - index - 1) * sizeof(alert_data->thresholds[0]));
  // insert new alert
  alert_data->thresholds[index] = seconds;
  alert_data->scheduled_count++;
  // write out the new data
  persist_write_data(PERSIST_ALERTS_KEY, alert_data, sizeof(AlertData));
  // send message to the foreground telling it to refresh
  AppWorkerMessage msg_data = { .data0 = 0 };
  app_worker_send_message(WorkerMessageReloadData, &msg_data);
}

// Destroy an existing alert at a certain index
void data_unschedule_alert(DataLibrary *data_library, uint8_t index) {
  AlertData *alert_data = &data_library->alert_data;
  // reload from persistent storage
  persist_read_data(PERSIST_ALERTS_KEY, alert_data, persist_get_size(PERSIST_ALERTS_KEY));
  // move memory back over that position
  memmove(&alert_data->thresholds[index], &alert_data->thresholds[index + 1],
    (DATA_ALERT_MAX_COUNT - index - 1) * sizeof(alert_data->thresholds[0]));
  alert_data->scheduled_count--;
  // cancel timer
  if (data_library->app_timers[index].app_timer) {
    app_timer_cancel(data_library->app_timers[index].app_timer);
    data_library->app_timers[index].app_timer = NULL;
  }
  // write out the new data
  persist_write_data(PERSIST_ALERTS_KEY, alert_data, sizeof(AlertData));
  // send message to the foreground telling it to refresh
  AppWorkerMessage msg_data = { .data0 = 0 };
  app_worker_send_message(WorkerMessageReloadData, &msg_data);
}

// Register callback for when an alert goes off
void data_register_alert_callback(DataLibrary *data_library, BatteryAlertCallback callback) {
  data_library->alert_callback = callback;
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

// Get the record run time of the watch
// Based off app install time if no data
int32_t data_get_record_run_time(DataLibrary *data_library) {
  // get record life
  int32_t record_run_time = 0;
  if (persist_exists(PERSIST_RECORD_LIFE_KEY)) {
    record_run_time = persist_read_int(PERSIST_RECORD_LIFE_KEY);
  }
  // check if current run time is greater than record
  if (data_get_run_time(data_library, 0) > record_run_time) {
    return data_get_run_time(data_library, 0);
  }
  return record_run_time;
}

// Get the run time at a certain charge cycle returns negative value if no data
int32_t data_get_run_time(DataLibrary *data_library, uint16_t index) {
  uint16_t load_index = index;
  if (load_index && data_library->cycle_head_node && data_library->cycle_head_node->end_epoch) {
    load_index--;
  }
  ChargeCycleNode *cur_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index(
    (Node*)data_library->cycle_head_node, load_index);
  if (!cur_node || (!index && cur_node->end_epoch) || !cur_node->discharge_epoch) {
    return -1;
  } else if (cur_node->end_epoch == 0) {
    return time(NULL) - cur_node->discharge_epoch;
  } else {
    return cur_node->end_epoch - cur_node->discharge_epoch;
  }
}

// Get the maximum battery life possible at a certain charge cycle (0 is current)
int32_t data_get_max_life(DataLibrary *data_library, uint16_t index) {
  if (index == 0) {
    // return current max life
    DataNode cur_node = prv_get_current_data_node(data_library);
    return cur_node.charge_rate * (-100);
  } else {
    if (data_library->cycle_head_node && data_library->cycle_head_node->end_epoch) {
      index--;
    }
    ChargeCycleNode *cur_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index(
      (Node*)data_library->cycle_head_node, index);
    if (!cur_node || !cur_node->avg_charge_rate) {
      return -1;
    } else {
      return cur_node->avg_charge_rate * (-100);
    }
  }
}

// Get the current percent-per-day of battery life
int32_t data_get_percent_per_day(DataLibrary *data_library) {
  return 10000 / (data_get_max_life(data_library, 0) * 100 / SEC_IN_DAY);
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
  if (percent < 1) {
    percent = 1;
  }
  return percent;
}

// Get a data point by its index with 0 being the most recent
void data_get_data_point(DataLibrary *data_library, uint16_t index, int32_t *epoch,
                         uint8_t *percent) {
  // get data node
  DataNode *data_node = prv_list_get_data_node(data_library, index);
  if (data_node) {
    (*epoch) = data_node->epoch;
    (*percent) = data_node->percent;
  }
}

// Get the number of charge cycles which include the last x number of seconds (0 gets all points)
uint16_t data_get_charge_cycle_count_including_seconds(DataLibrary *data_library, int32_t seconds) {
  time_t end_time = seconds ? time(NULL) - seconds : 0;
  uint16_t index = 0;
  ChargeCycleNode *cur_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index(
    (Node*)data_library->cycle_head_node, index);
  while (cur_node) {
    index++;
    if (cur_node->charge_epoch < end_time) { break; }
    cur_node = (ChargeCycleNode*)prv_linked_list_get_node_by_index(
      (Node*)data_library->cycle_head_node, index);
  }
  if (data_library->cycle_head_node && data_library->cycle_head_node->end_epoch) {
    index++;
  }
  if (!index) {
    index = 1;
  }
  return index;
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
  app_log(APP_LOG_LEVEL_INFO, "", 0, "-----------------------------------------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "All timestamps are in UTC epoch format.");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "'Charge Rate' represents the inverse of the rate of");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "change of the battery percentage with respect to");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "time. It is in seconds per percent.");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Any value of -1 represents an invalid statistic.");
  // print stats
  app_log(APP_LOG_LEVEL_INFO, "", 0, "--------------------- Statistics --------------------");
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Current Time:\t%d", (int)time(NULL));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Last Charged:\t%d",
    (int)(time(NULL) - data_get_run_time(data_library, 0)));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Time Remaining:\t%d",
    (int)data_get_life_remaining(data_library));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Maximum Life:\t%d",
    (int)data_get_max_life(data_library, 0));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Run Time:\t%d",
    (int)data_get_run_time(data_library, 0));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Record Life:\t%d",
    (int)data_get_record_run_time(data_library));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Battery Percent:\t%d", (int)data_get_battery_percent
    (data_library));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Percent per Day:\t%d",
    (int)data_get_percent_per_day(data_library));
  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Rate:\t%d", (int)cur_data_node.charge_rate);
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
  // send message to the foreground telling it to refresh
  AppWorkerMessage msg_data = { .data0 = 0 };
  app_worker_send_message(WorkerMessageReloadData, &msg_data);
}

// Write the data out in chunks to the foreground app
void data_write_to_foreground(DataLibrary *data_library, uint8_t data_pt_start_index) {
  // get some stats
  DataNode cur_node = prv_get_current_data_node(data_library);
  int32_t lst_charge_time = data_get_run_time(data_library, 0);
  if (lst_charge_time > 0) {
    lst_charge_time = time(NULL) - lst_charge_time;
  }
  // create data blob to write
  DataAPI data_api = (DataAPI) {
    .charge_rate = cur_node.charge_rate,
    .charge_by_time = data_get_charge_by_time(data_library),
    .last_charged_time = lst_charge_time,
    .record_run_time = data_get_record_run_time(data_library),
    .data_pt_start_index = data_pt_start_index,
    .alert_count = data_get_alert_count(data_library),
    .cycle_count = data_get_charge_cycle_count_including_seconds(data_library, 0) - 1,
    .data_pt_count = 0
  };
  if (data_api.cycle_count > CHARGE_CYCLE_MAX_COUNT) {
    data_api.cycle_count = CHARGE_CYCLE_MAX_COUNT;
  }
  for (uint8_t ii = 0; ii < data_api.alert_count; ii++) {
    data_api.alert_threshold[ii] = data_get_alert_threshold(data_library, ii);
  }
  for (uint8_t ii = 0; ii < data_api.cycle_count; ii++) {
    data_api.run_times[ii] = data_get_run_time(data_library, ii + 1);
    data_api.max_lives[ii] = data_get_max_life(data_library, ii + 1);
  }
  DataNode *tmp_node;
  for (uint8_t ii = 0; ii < DATA_POINT_MAX_COUNT; ii++) {
    tmp_node = prv_list_get_data_node(data_library, data_api.data_pt_start_index + ii);
    if (!tmp_node) { break; }
    data_api.data_pt_epochs[ii] = tmp_node->epoch;
    data_api.data_pt_percents[ii] = tmp_node->percent;
    data_api.data_pt_count++;
  }
  // delete any old data that may be loaded
  persist_delete(TEMP_LOCK_KEY);
  persist_delete(TEMP_COMMUNICATION_KEY);
  // loop and wait
  // TODO: Prevent this from going into an endless loop somehow
  uint16_t bytes_written = 0, bytes_to_write = 0;
  while (1) {
    // check if data key exists
    if (!persist_exists(TEMP_LOCK_KEY)) {
      // write out data
      bytes_to_write = sizeof(DataAPI) - bytes_written;
      if (bytes_to_write > PERSIST_DATA_MAX_LENGTH) {
        bytes_to_write = PERSIST_DATA_MAX_LENGTH;
      }
      bytes_written += persist_write_data(TEMP_COMMUNICATION_KEY,
        ((char*)&data_api + bytes_written), bytes_to_write);
      persist_write_bool(TEMP_LOCK_KEY, false);
      // check if done
      if (bytes_written >= sizeof(DataAPI)) {
        break;
      }
    }
    // wait
    psleep(10);
  }
}

// Initialize the data
DataLibrary *data_initialize(void) {
  // create new DataLibrary
  DataLibrary *data_library = MALLOC(sizeof(DataLibrary));
  memset(data_library, 0, sizeof(DataLibrary));
  data_library->data_logging_session = data_logging_create(DATA_LOGGING_TAG,
    DATA_LOGGING_BYTE_ARRAY, sizeof(DataNode), true);
  // read data from persistent storage
  if (!persist_exists(PERSIST_DATA_KEY)) {
    prv_first_launch_prep(data_library);
  } else {
    prv_persist_read_data_block(data_library, 0);
    prv_calculate_charge_cycles(data_library, CYCLE_LINKED_LIST_MIN_SIZE);
    data_refresh_all_alerts(data_library);
  }
  return data_library;
}

// Terminate the data
void data_terminate(DataLibrary *data_library) {
  // free other data
  prv_linked_list_destroy((Node**)&data_library->cycle_head_node, &data_library->cycle_node_count);
  prv_linked_list_destroy((Node**)&data_library->head_node, &data_library->node_count);
  free(data_library);
}
