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

// Initialize
static void prv_initialize(void) {
  data_library = data_initialize();
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
