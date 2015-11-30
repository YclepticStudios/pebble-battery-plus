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
#define STATS_CHARGE_RATE_KEY 999
#define DATA_PERSIST_KEY 1000
#define DATA_LOGGING_TAG 5155346
#define SEC_IN_DAY 86400

// Main data structure
typedef struct {
  uint32_t  epoch     : 31;   //< The epoch timestamp for when the percentage changed in seconds
  uint8_t   percent   : 7;    //< The battery charge percent
  bool      charging  : 1;    //< The charging state of the battery
  bool      plugged   : 1;    //< The plugged in state of the watch
} __attribute__((__packed__)) DataNode;

// Current data logging session
DataLoggingSessionRef data_log_session;

// Last battery state node
DataNode last_node;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Calculate current estimated charge rate (for predictions)
static void prv_calculate_charge_rate(DataNode new_node) {
  // calculate seconds per percent between two nodes and add to exponential moving average
  if (last_node.percent <= new_node.percent) { return; }
  int32_t charge_rate = persist_read_int(STATS_CHARGE_RATE_KEY);
  charge_rate = charge_rate * 4 / 5 + ((new_node.epoch - last_node.epoch) / (new_node.percent -
    last_node.percent)) / 5;
  persist_write_int(STATS_CHARGE_RATE_KEY, charge_rate);
}

// Load last data node
static void prv_load_last_data_node(void) {
  // give dummy value in case of failure
  last_node = (DataNode) { .epoch = 0, .percent = 0, .charging = true, .plugged = false };
  // check for data
  uint32_t persist_key = persist_read_int(DATA_PERSIST_KEY);
  if (!persist_exists(persist_key)) { return; }
  // read data
  size_t persist_size = persist_get_size(persist_key);
  char *buff = malloc(persist_size);
  if (!buff) { return; }
  persist_read_data(persist_key, buff, persist_size);
  memcpy(&last_node, &buff[persist_size - sizeof(DataNode)], sizeof(DataNode));
  free(buff);
}

// Convert old data format to new data format
static void prv_persist_convert_legacy_data(void) {
  // TODO: Implement this to maintain old data
}

// Save a new node to persistent storage
static void prv_persist_data_node(DataNode node) {
  // get the current persistent key
  uint32_t persist_key = persist_read_int(DATA_PERSIST_KEY);
  int persist_size = persist_exists(persist_key) ? persist_get_size(persist_key) : 0;
  // index to next storage location if too full
  if (persist_size + sizeof(DataNode) > PERSIST_DATA_MAX_LENGTH) {
    persist_key++;
    persist_write_int(DATA_PERSIST_KEY, persist_key);
  }
  // read existing data
  size_t buff_size = persist_size + sizeof(DataNode);
  char *buff = malloc(buff_size);
  if (!buff) { return; }
  persist_read_data(persist_key, buff, persist_size);
  // write data
  memcpy(&buff[persist_size], &node, sizeof(node));
  uint32_t bytes_written = persist_write_data(persist_key, buff, buff_size);
  // check for success
  if (bytes_written < buff_size) {
    // remove oldest data
    uint32_t temp_key = persist_key - 1;
    while (temp_key > DATA_PERSIST_KEY && persist_exists(temp_key)) {
      temp_key--;
    }
    // delete that data
    persist_delete(temp_key + 1);
    // attempt to write again
    persist_write_data(persist_key, buff, buff_size);
  }
  free(buff);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Battery state change callback
static void prv_battery_state_change_handler(BatteryChargeState battery_state) {
  // prevent this being called with the same battery data
  if (last_node.percent == battery_state.charge_percent &&
      last_node.charging == battery_state.is_charging &&
      last_node.plugged == battery_state.is_plugged) { return; }
  // create DataNode from data
  DataNode node = (DataNode) {
    .epoch = time(NULL),
    .percent = battery_state.charge_percent,
    .charging = battery_state.is_charging,
    .plugged = battery_state.is_plugged,
  };
  // calculate charge rate
  prv_calculate_charge_rate(node);
  // persist the data node
  prv_persist_data_node(node);
  // log the data from this sample
  data_logging_log(data_log_session, &node, 1);
  // set as last node
  last_node = node;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// First launch prepping
static void prv_first_launch(void) {
  // write out starting persistent storage location
  persist_write_int(DATA_PERSIST_KEY, DATA_PERSIST_KEY + 1);
  // write out initial guess for battery life
  uint32_t charge_rate;
  WatchInfoModel watch_model = watch_info_get_model();
  switch (watch_model) {
    case WATCH_INFO_MODEL_PEBBLE_TIME_STEEL:
      charge_rate = -10 * SEC_IN_DAY / 100;
      break;
    case WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_14:
    case WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_20:
      charge_rate = -2 * SEC_IN_DAY / 100;
      break;
    default:
      charge_rate = -7 * SEC_IN_DAY / 100;
  }
  persist_write_int(STATS_CHARGE_RATE_KEY, charge_rate);
  // set dummy last_node in an impossible configuration to ensure a battery update call
  last_node = (DataNode) { .epoch = 0, .percent = 0, .charging = true, .plugged = false };
  // initialize starting data point
  prv_battery_state_change_handler(battery_state_service_peek());
}

// Initialize
static void prv_initialize(void) {
  // check if previously launched
  if (persist_exists(DATA_PERSIST_KEY)) {
    prv_load_last_data_node();
  } else {
    prv_first_launch();
  }
  // create data logging session
  data_log_session = data_logging_create(DATA_LOGGING_TAG, DATA_LOGGING_BYTE_ARRAY,
    sizeof(DataNode), true);
  // subscribe to battery service
  battery_state_service_subscribe(prv_battery_state_change_handler);
}

// Terminate
static void prv_terminate(void) {
  battery_state_service_unsubscribe();
}

// Main entry point
int main(void) {
  prv_initialize();
  worker_event_loop();
  prv_terminate();
}
