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
  MenuLayer   *menu;          //< The main menu layer for the application
  Layer       *layer;         //< The drawing layer for the progress ring
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

// MenuLayer row draw callback
static void prv_menu_row_draw_handler(GContext *ctx, const Layer *layer, MenuIndex *index,
                                      void *context) {
  drawing_render_cell(main_data.menu, (Layer*)layer, ctx, *index);
}

// MenuLayer get row count callback
static uint16_t prv_menu_get_row_count_handler(MenuLayer *menu, uint16_t index, void *context) {
  return MENU_CELL_COUNT;
}

// MenuLayer get row height callback
static int16_t prv_menu_get_row_height_handler(MenuLayer *menu, MenuIndex *index, void *context) {
  if (menu_layer_get_selected_index(main_data.menu).row == index->row) {
    return MENU_CELL_HEIGHT_TALL;
  }
  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(menu));
  return (menu_bounds.size.h - MENU_CELL_HEIGHT_TALL) / 2;
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
  Layer *window_root = window_get_root_layer(main_data.window);
  GRect window_bounds = layer_get_bounds(window_root);
  window_stack_push(main_data.window, true);
  // initialize menu layer
  main_data.menu = menu_layer_create(grect_inset(window_bounds, GEdgeInsets1(RING_WIDTH)));
  ASSERT(main_data.menu);
  menu_layer_set_click_config_onto_window(main_data.menu, main_data.window);
  menu_layer_set_highlight_colors(main_data.menu, COLOR_MENU_BACKGROUND, GColorBlack);
  menu_layer_set_center_focused(main_data.menu, true);
  menu_layer_set_callbacks(main_data.menu, NULL, (MenuLayerCallbacks) {
    .draw_row = prv_menu_row_draw_handler,
    .get_num_rows = prv_menu_get_row_count_handler,
    .get_cell_height = prv_menu_get_row_height_handler,
  });
  layer_add_child(window_root, menu_layer_get_layer(main_data.menu));
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
  menu_layer_destroy(main_data.menu);
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
