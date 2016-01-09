// @file main.c
// @brief Main logic for Battery+
//
// Contains the higher level logic code
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#include <pebble.h>
#include "data/data_library.h"
#include "drawing/drawing.h"
#include "menu.h"
#include "utility.h"

// Main constants
#define REFRESH_PERIOD_MIN 5
#define CLICK_LONG_PRESS_DURATION 500

// Main data structure
static struct {
  Window        *window;          //< The base window for the application
  DataLibrary   *data_library;    //< The main data pointer for all data functions
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Up click handler
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(true);
}

// Select down click handler
void select_click_down_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_click();
  drawing_set_action_menu_dot(true);
}

// Select up click handler
void select_click_up_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_set_action_menu_dot(false);
}

// Select long click handler
void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  menu_show(main_data.data_library);
  drawing_set_action_menu_dot(false);
}

// Down click handler
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(false);
}

// Click configuration callback
static void prv_click_config_handler(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_click_down_handler,
    select_click_up_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, CLICK_LONG_PRESS_DURATION,
    select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
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
  if (data->data0 == WorkerMessageForeground) {
    // no need to read what is sent, just update the data
    data_reload(main_data.data_library);
  }
  // refresh the screen
  drawing_refresh();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the program
void prv_initialize(void) {
  // load data
  main_data.data_library = data_initialize();
  // start background worker
  app_worker_launch();
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  Layer *window_root = window_get_root_layer(main_data.window);
  window_set_click_config_provider(main_data.window, prv_click_config_handler);
  window_stack_push(main_data.window, true);
  // initialize drawing layers
  drawing_initialize(window_root, main_data.data_library);
  // initialize action menu
  menu_initialize(main_data.data_library);
  // subscribe to services
  app_worker_message_subscribe(prv_worker_message_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_timer_service_handler);
}

// Terminate the program
static void prv_terminate(void) {
  // unsubscribe from services
  app_worker_message_unsubscribe();
  tick_timer_service_unsubscribe();
  // destroy
  menu_terminate();
  drawing_terminate();
  window_destroy(main_data.window);
  // unload data
  data_terminate(main_data.data_library);
}

// Entry point
int main(void) {
  prv_initialize();
  app_event_loop();
  prv_terminate();
}
