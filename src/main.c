// @file main.c
// @brief Main logic for Battery+
//
// Contains the higher level logic code
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#include <pebble.h>
#include "data/data_api.h"
#include "drawing/drawing.h"
#include "menu.h"
#include "drawing/windows/alert/popup_window.h"
#include "phone.h"
#include "utility.h"

// Main constants
#define REFRESH_PERIOD_MIN 5
#define CLICK_LONG_PRESS_DURATION 500

// Main data structure
static struct {
  Window        *window;          //< The base window for the application
  DataAPI       *data_api;        //< The main data pointer for all data functions
} main_data;

// Function declarations
static void prv_initialize_popup(void);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Down click event for "UP" button
static void up_button_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_render_next_card(true);
}

// Up click event for "UP" button
static void up_button_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(true);
}

// Select down click handler
static void select_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_click();
  drawing_set_action_menu_dot(true);
}

// Select up click handler
static void select_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_set_action_menu_dot(false);
}

// Select long click handler
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // free memory from drawing caches
#ifdef PBL_BW
  drawing_free_caches();
#endif
  // show the action menu
  menu_show(main_data.data_api);
  drawing_set_action_menu_dot(false);
}

// Down click event for "DOWN" button
static void down_button_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_render_next_card(false);
}

// Up click event for "DOWN" button
static void down_button_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(false);
}

// Click configuration callback
static void prv_click_config_handler(void *context) {
  window_raw_click_subscribe(BUTTON_ID_UP, up_button_click_down_handler,
    up_button_click_up_handler, NULL);
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_click_down_handler,
    select_click_up_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, CLICK_LONG_PRESS_DURATION,
    select_long_click_handler, NULL);
  window_raw_click_subscribe(BUTTON_ID_DOWN, down_button_click_down_handler,
    down_button_click_up_handler, NULL);
}

// Tick Timer service for updating every minute
static void prv_tick_timer_service_handler(tm *tick_time, TimeUnits units_changed) {
  // check if at the current refresh period
  if ((time(NULL) / SEC_IN_MIN) % REFRESH_PERIOD_MIN) {
    drawing_refresh();
  }
}

// Worker message callback
static void prv_worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  // check what is being sent
  switch (type) {
    case WorkerMessageReloadData:
      data_api_reload(main_data.data_api);
      drawing_refresh();
      break;
    case WorkerMessageAlertEvent:
      prv_initialize_popup();
      break;
    default:
      return;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the pin pushing window
static void prv_initialize_pin_pushing_window(void) {
  // load data
  if (!main_data.data_api) {
    main_data.data_api = data_api_initialize();
  }
  // create popup alert window
  Window *popup_window = popup_window_create(true);
  window_set_background_color(popup_window, PBL_IF_BW_ELSE(GColorWhite, GColorVividCerulean));
  popup_window_set_text(popup_window, "Battery+", "Pushing Pins");
  popup_window_set_visual(popup_window, RESOURCE_ID_TIMELINE_SYNC_IMAGE, true);
  window_stack_push(popup_window, true);
  // start the pin sending process
  phone_connect();
  phone_send_timestamp_to_phone(data_api_get_charge_by_time(main_data.data_api));
  phone_set_window_close_on_complete(popup_window);
}

// Initialize the popup window
static void prv_initialize_popup(void) {
  // load data
  if (!main_data.data_api) {
    main_data.data_api = data_api_initialize();
  }
  // read alert index
  uint8_t alert_index = persist_read_int(WAKE_UP_ALERT_INDEX_KEY);
  persist_delete(WAKE_UP_ALERT_INDEX_KEY);
  // get text
  static char buff[16];
  int day = data_api_get_alert_threshold(main_data.data_api, alert_index) / SEC_IN_DAY;
  int hr = data_api_get_alert_threshold(main_data.data_api, alert_index) % SEC_IN_DAY / SEC_IN_HR;
  if (day && hr) {
    snprintf(buff, sizeof(buff), "%dd %dh Left", day, hr);
  } else if (day) {
    if (day > 1) {
      snprintf(buff, sizeof(buff), "%d Days Left", day);
    } else {
      snprintf(buff, sizeof(buff), "%d Day Left", day);
    }
  } else {
    if (hr > 1) {
      snprintf(buff, sizeof(buff), "%d Hours Left", hr);
    } else {
      snprintf(buff, sizeof(buff), "%d Hour Left", hr);
    }
  }
  // create popup alert window
  Window *popup_window = popup_window_create(true);
  popup_window_set_close_on_animation_end(popup_window, true);
  window_set_background_color(popup_window, PBL_IF_BW_ELSE(GColorWhite,
    data_api_get_alert_color(main_data.data_api, alert_index)));
  popup_window_set_text(popup_window, "Battery+", buff);
  popup_window_set_visual(popup_window, RESOURCE_ID_LOW_BATTERY_IMAGE, true);
  window_stack_push(popup_window, true);
  // vibrate
  vibes_short_pulse();
}

// Initialize the program
static void prv_initialize_main(void) {
  // start background worker
  bool is_running = app_worker_is_running();
  app_worker_launch();
  if (!is_running) {
    psleep(200);
  }
  // load data
  main_data.data_api = data_api_initialize();
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  Layer *window_root = window_get_root_layer(main_data.window);
  window_set_click_config_provider(main_data.window, prv_click_config_handler);
  window_stack_push(main_data.window, true);
  // initialize drawing layers
  drawing_initialize(window_root, main_data.data_api);
  // subscribe to services
  app_worker_message_subscribe(prv_worker_message_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_timer_service_handler);
}

// Terminate the program
static void prv_terminate_main(void) {
  // unsubscribe from services
  app_worker_message_unsubscribe();
  tick_timer_service_unsubscribe();
  // destroy
  drawing_terminate();
  window_destroy(main_data.window);
  // unload data
  data_api_terminate(main_data.data_api);
}

// Terminate the popup window
static void prv_terminate_popup(void) {
  // unload data
  data_api_terminate(main_data.data_api);
}

// Entry point
int main(void) {
  // set main data to 0
  memset(&main_data, 0, sizeof(main_data));
  // check launch reason
  if (launch_reason() == APP_LAUNCH_WORKER) {
    // either launch the popup or the sync screen
    if (persist_exists(WAKE_UP_ALERT_INDEX_KEY)) {
      prv_initialize_popup();
    } else {
      prv_initialize_pin_pushing_window();
    }
  } else {
    prv_initialize_main();
  }
  // main loop
  app_event_loop();
  // terminate proper part
  if (launch_reason() != APP_LAUNCH_WORKER) {
    prv_terminate_main();
  } else {
    prv_terminate_popup();
  }
}
