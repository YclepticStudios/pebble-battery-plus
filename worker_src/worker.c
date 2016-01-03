// @file worker.c
// @brief Background worker code
//
// Background worker for Battery+.
// Saves data when battery status changes and sends data to phone via DataLogging.
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#define PEBBLE_BACKGROUND_WORKER
#include <pebble_worker.h>
#include "../src/utility.c"
#include "../src/data/data_library.c"
#undef PEBBLE_BACKGROUND_WORKER

// Main variables
static DataLibrary *data_library;


// Battery state change callback
static void prv_battery_state_change_handler(BatteryChargeState battery_state) {
  data_process_new_battery_state(data_library, battery_state);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Legacy Data Migration
//
//
//// data constants
//#define LEGACY_PERSIST_DATA 100
//#define LEGACY_PERSIST_DATA_LENGTH 24 // must be multiple of 4
//#define LEGACY_DATA_SIZE 100
//#define LEGACY_EPOCH_OFFSET 1420070400
//
//// Convert old data format to new data format
//static void prv_persist_convert_legacy_data(void) {
//  // get some values
//  uint32_t myData[LEGACY_DATA_SIZE];
//  uint8_t *ptr = (uint8_t*)&myData;
//  uint16_t step = LEGACY_PERSIST_DATA_LENGTH, size = sizeof(myData), Pers_Val = LEGACY_PERSIST_DATA;
//  // load myIndex and myCount
//  uint16_t myIndex = (uint16_t)persist_read_int(Pers_Val);
//  persist_delete(Pers_Val++);
//  uint16_t myCount = (uint16_t)persist_read_int(Pers_Val);
//  persist_delete(Pers_Val++);
//  int32_t myRecord = persist_read_int(Pers_Val);
//  persist_delete(Pers_Val++);
//  // read the array in several pieces
//  for (uint16_t delta = 0; delta < size; delta += step) {
//    persist_read_data(Pers_Val, ptr + delta,
//      (delta + step < size) ? step : (size % step));
//    persist_delete(Pers_Val++);
//  }
//  // send data to new code for processing
//  int16_t idx = -1;
//  if (myCount >= LEGACY_DATA_SIZE) {
//    idx = myIndex;
//  }
//  DataNode node;
//  // loop through data
//  for (uint16_t ii = 0; ii < 999; ii++) {
//    // index
//    idx++;
//    if (idx >= myCount && myCount >= LEGACY_DATA_SIZE) idx = 0;
//    else if (idx >= myCount || (idx == myIndex && ii > 0)) break;
//    // unpack data point
//    uint32_t val = myData[idx];
//    node.epoch = ((val / 44) * 60) + LEGACY_EPOCH_OFFSET;
//    node.percent = ((val %= 44) / 4) * 10;
//    node.charging = ((val %= 4) / 2);
//    node.plugged = (val %= 2);
//    // process data node
//    prv_process_data_node(node);
//  }
//}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize
static void prv_initialize(void) {
  // create DataLibrary
  data_library = data_initialize();
  // TODO: Check data logging if should be created in foreground and background
  // subscribe to battery service
  battery_state_service_subscribe(prv_battery_state_change_handler);
}

// Terminate
static void prv_terminate(void) {
  battery_state_service_unsubscribe();
  data_terminate(data_library);
}

// Main entry point
int main(void) {
  prv_initialize();
  worker_event_loop();
  prv_terminate();
}
