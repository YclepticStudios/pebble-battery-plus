// @file worker.c
// @brief Background worker code
//
// Background worker for Battery+.
// Saves data when battery status changes and sends data to phone via DataLogging
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#include <pebble_worker.h>

// Constants
#define DATA_PERSIST_KEY 1000
#define DATA_LOGGING_TAG 5155346

// Main data structure
typedef struct {
  uint32_t  epoch     : 31;   //< The epoch timestamp for when the percentage changed in seconds
  uint8_t   percent   : 7;    //< The battery charge percent
  bool      charging  : 1;    //< The charging state of the battery
  bool      plugged   : 1;    //< The plugged in state of the watch
} __attribute__((__packed__)) DataNode;

// Current data logging session
DataLoggingSessionRef data_log_session;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Convert old data format to new data format
static void prv_persist_convert_legacy_data(void) {
  // TODO: Implement this to maintain old data
}

// Save a new node to persistent storage
static void prv_persist_data_node(DataNode node) {
  // get the current persistent key
  uint32_t persist_key = DATA_PERSIST_KEY;
  if (persist_exists(DATA_PERSIST_KEY)) {
    persist_key = persist_read_int(DATA_PERSIST_KEY);
  }
  // index to next location if key is too full
  if (!persist_exists(persist_key) ||
    persist_get_size(persist_key) + sizeof(DataNode) > PERSIST_DATA_MAX_LENGTH) {
    persist_key++;
    persist_write_int(DATA_PERSIST_KEY, persist_key);
  }
  // read existing data
  int persist_size = persist_get_size(persist_key);
  if (!persist_exists(persist_key)) {
    persist_size = 0;
  }
  char *buff = malloc(persist_size + sizeof(DataNode));
  if (!buff) {
    return;
  }
  persist_read_data(persist_key, buff, persist_size);
  memcpy(&buff[persist_size], &node, sizeof(node));
  persist_write_data(persist_key, buff, sizeof(buff));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Battery state change callback
static void prv_battery_state_change_handler(BatteryChargeState battery_state) {
  // create DataNode from data
  DataNode node = (DataNode) {
    .epoch = time(NULL),
    .percent = battery_state.charge_percent,
    .charging = battery_state.is_charging,
    .plugged = battery_state.is_plugged,
  };
  // persist the data node
  prv_persist_data_node(node);
  // log the data from this sample
  data_logging_log(data_log_session, &node, 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize
static void prv_initialize(void) {
  // create data logging session
  data_log_session = data_logging_create(DATA_LOGGING_TAG, DATA_LOGGING_BYTE_ARRAY,
                                         sizeof(DataNode), true);
  // subscribe to battery service
  battery_state_service_subscribe(prv_battery_state_change_handler);
}

// Terminate
static void prv_terminate(void) {
  accel_data_service_unsubscribe();
}

// Main entry point
int main(void) {
  prv_initialize();
  worker_event_loop();
  prv_terminate();
}