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
  Layer       *layer;         //< Layer onto which everything is drawn
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
//  // draw dummy text
//  GRect bounds = layer_get_bounds(layer);
//  bounds.origin.y = bounds.size.h / 2 - 23;
//  graphics_context_set_text_color(ctx, GColorWhite);
//  graphics_draw_text(ctx, "Battery+", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), bounds,
//    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
//  // draw time remaining
//  char buff[64];
//  data_get_time_remaining(buff, sizeof(buff));
//  bounds.origin.y = bounds.size.h * 3 / 4 - 15;
//  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bounds,
//    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
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
  window_set_background_color(main_data.window, GColorBlack);
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
  window_stack_push(main_data.window, true);
  // initialize layer
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
