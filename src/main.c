// @file main.c
// @brief Main logic for Battery+
//
// Contains the higher level logic code
//
// @author Eric D. Phillips
// @date November 22, 2015
// @bugs No known bugs

#include <pebble.h>
#include "data.h"
#include "drawing/drawing.h"
#include "utility.h"

// Main constants
#define DATA_LOAD_NUM_DAYS 7

// Main data structure
static struct {
  Window      *window;            //< The base window for the application
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Up click handler
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(true);
}

// Select click handler
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {

}

// Down click handler
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  drawing_select_next_card(false);
}

// Click configuration callback
static void prv_click_config_handler(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

// Tick Timer service for updating every minute
static void prv_tick_timer_service_handler(tm *tick_time, TimeUnits units_changed) {
  // TODO: Refresh Everything
}

// Worker message callback
static void prv_worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  // no need to read what is sent, just update the data
  data_load_past_days(DATA_LOAD_NUM_DAYS);
  // refresh the screen
  prv_tick_timer_service_handler(NULL, MINUTE_UNIT);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the program
void prv_initialize(void) {
  // start background worker
  app_worker_launch();
  // load data
  data_load_past_days(DATA_LOAD_NUM_DAYS);
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  Layer *window_root = window_get_root_layer(main_data.window);
  window_set_click_config_provider(main_data.window, prv_click_config_handler);
  window_stack_push(main_data.window, true);
  // initialize drawing layers
  drawing_initialize(window_root);
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
  drawing_terminate();
  window_destroy(main_data.window);
  // unload data
  data_unload();
}

// Entry point
int main(void) {
  prv_initialize();
  app_event_loop();
  prv_terminate();
}
