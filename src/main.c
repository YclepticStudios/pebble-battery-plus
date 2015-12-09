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
#include "drawing.h"
#include "utility.h"

// Main data structure
static struct {
  Window      *window;        //< The base window for the application
  Layer       *layer;         //< A pointer to the layer on which everything is drawn
} main_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Layer update procedure
static void prv_layer_update_proc_handler(Layer *layer, GContext *ctx) {
  drawing_render(layer, ctx);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Loading and Unloading
//

// Initialize the program
static void prv_initialize(void) {
  // start background worker
  app_worker_launch();
  // load data
  data_load_past_days(1);
  // initialize window
  main_data.window = window_create();
  ASSERT(main_data.window);
  window_set_background_color(main_data.window, GColorShockingPink);
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
  window_stack_push(main_data.window, true);
  // initialize tile layer
  main_data.layer = layer_create(window_bounds);
  ASSERT(main_data.layer);
  layer_set_update_proc(main_data.layer, prv_layer_update_proc_handler);
  layer_add_child(window_root, main_data.layer);
  // initialize drawing
  drawing_initialize(main_data.layer);
}

// Terminate the program
static void prv_terminate(void) {
  // destroy
  layer_destroy(main_data.layer);
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
