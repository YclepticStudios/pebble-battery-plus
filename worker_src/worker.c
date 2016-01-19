// @file worker.c
// @brief Main background worker control file
//
// Background worker for Battery+. Controls acquiring new data, processing that data
// and then receiving data requests from the foreground app. All the data is stored
// in memory in the background app. The foreground app quires the background app for
// specific data metrics.
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#include <pebble_worker.h>
#include "data_library.h"
#define PEBBLE_BACKGROUND_WORKER
#include "../src/data/data_shared.h"
#include "../src/utility.c"
#undef PEBBLE_BACKGROUND_WORKER


// Main variables
static DataLibrary *data_library;

// Battery alert raised callback
static void prv_battery_alert_handler(uint8_t alert_index) {
  // write out the index of the alert
  persist_write_int(WAKE_UP_ALERT_INDEX_KEY, alert_index);
  // send message in case app is already open
  AppWorkerMessage message = {.data0 = alert_index};
  app_worker_send_message(WorkerMessageAlertEvent, &message);
  // launch main app in case it is closed
  worker_launch_app();
}

// Worker message callback
static void prv_worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  // check what was sent
  switch (type) {
    case WorkerMessageSendData:
      data_write_to_foreground(data_library, data->data0);
      break;
    case WorkerMessageScheduleAlert:;
      int32_t seconds = (data->data0 << 16) | (data->data1 & 0x0000FFFF);
      data_schedule_alert(data_library, seconds);
      break;
    case WorkerMessageUnscheduleAlert:
      data_unschedule_alert(data_library, data->data0);
      break;
    default:
      return;
  }
}

// Battery state change callback
static void prv_battery_state_change_handler(BatteryChargeState battery_state) {
  data_process_new_battery_state(data_library, battery_state);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize
static void prv_initialize(void) {
  data_library = data_initialize();
  data_register_alert_callback(data_library, prv_battery_alert_handler);
  app_worker_message_subscribe(prv_worker_message_handler);
  battery_state_service_subscribe(prv_battery_state_change_handler);
  // run initial point through
  data_process_new_battery_state(data_library, battery_state_service_peek());
}

// Terminate
static void prv_terminate(void) {
  app_worker_message_unsubscribe();
  battery_state_service_unsubscribe();
  data_terminate(data_library);
}

// Main entry point
int main(void) {
  prv_initialize();
  worker_event_loop();
  prv_terminate();
}
