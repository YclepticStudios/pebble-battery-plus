// @file pin_window.c
// @brief Popup window for entering a series of numbers
//
// Creates and displays a window with Pebble style bouncing selectors.
// It is used for choosing a number or series of numbers.
//
// @author Eric D. Phillips
// @date January 8, 2016
// @bug No known bugs

#include "pin_window.h"
#include "selection_layer.h"
#include "../../../utility.h"

// Constants
#define DEFAULT_ACTIVE_CELL_COLOR GColorMagenta
#define DEFAULT_INACTIVE_CELL_COLOR GColorDarkGray
#define SELECTION_LAYER_SIZE GSize(126, 34)
#define SELECTION_LAYER_CELL_PADDING 6

// Main data struct
typedef struct {
  TextLayer   *title_layer;             //< Pointer to title TextLayer
  TextLayer   *footer_layer;            //< Pointer to footer TextLayer
  Layer       *selection_layer;         //< Pointer to selection layer
  uint8_t     field_count;              //< Total number of fields displayed
  uint8_t     field_selected_index;     //< Index of currently selected field
  int8_t      field_values[PIN_WINDOW_MAX_FIELD_COUNT];       //< Actual values of each field
  uint8_t     field_max_values[PIN_WINDOW_MAX_FIELD_COUNT];   //< Maximum value for each field
  char        field_buffs[PIN_WINDOW_MAX_FIELD_COUNT][4];     //< Text buffer for fields
  bool        destroy_on_close;         //< Whether to auto-destroy when returning
  bool        already_returned;         //< Whether the return callback has already been called
  void        *context;                 //< Context to be returned with the completed callback
  PinWindowReturnCallback   return_callback;                  //< Return callback function
} PinWindowData;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Selection layer get text
static char* prv_selection_handle_get_text(int index, void *context) {
  PinWindowData *window_data = window_get_user_data(context);
  snprintf(window_data->field_buffs[index], sizeof(window_data->field_buffs[0]),
    window_data->field_max_values[index] < 10 ? "%d" : "%02d",
    (int)window_data->field_values[index]);
  return window_data->field_buffs[index];
}

// Selection layer complete
static void prv_selection_handle_complete(void *context) {
  Window *window = context;
  PinWindowData *window_data = window_get_user_data(window);
  bool is_handler = (bool)window_data->return_callback;
  if (is_handler) {
    window_data->already_returned = true;
    window_data->return_callback(false, window_data->field_count, window_data->field_values,
      window_data->context);
  }
}

// Selection layer increment
static void prv_selection_handle_inc(int index, uint8_t clicks, void *context) {
  PinWindowData *window_data = window_get_user_data(context);
  window_data->field_values[index]++;
  if(window_data->field_values[index] > window_data->field_max_values[index]) {
    window_data->field_values[index] = 0;
  }
}

// Selection layer decrement
static void prv_selection_handle_dec(int index, uint8_t clicks, void *context) {
  PinWindowData *window_data = window_get_user_data(context);
  window_data->field_values[index]--;
  if(window_data->field_values[index] < 0) {
    window_data->field_values[index] = window_data->field_max_values[index];
  }
}

// Window unload handler
static void prv_window_unload_handler(Window *window) {
  // destroy pin window
  pin_window_destroy(window);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Set the active and inactive field colors
void pin_window_set_field_colors(Window *window, GColor active_color, GColor inactive_color) {
  PinWindowData *window_data = window_get_user_data(window);
  selection_layer_set_active_bg_color(window_data->selection_layer, active_color);
  selection_layer_set_inactive_bg_color(window_data->selection_layer, inactive_color);
}

// Set the title and footer text displayed on the window
void pin_window_set_text(Window *window, char *title_text, char *footer_text) {
  PinWindowData *window_data = window_get_user_data(window);
  text_layer_set_text(window_data->title_layer, title_text);
  text_layer_set_text(window_data->footer_layer, footer_text);
}

// Set the maximum value for each field (default of 9)
void pin_window_set_max_field_values(Window *window, uint8_t *max_values) {
  PinWindowData *window_data = window_get_user_data(window);
  for (uint8_t index = 0; index < window_data->field_count; index++) {
    window_data->field_max_values[index] = max_values[index];
  }
}

// Set the value for each field (default of 0)
void pin_window_set_field_values(Window *window, uint8_t *values) {
  PinWindowData *window_data = window_get_user_data(window);
  for (uint8_t index = 0; index < window_data->field_count; index++) {
    window_data->field_values[index] = values[index];
  }
}

// Set PinWindow callback handler
void pin_window_set_return_callback(Window *window, PinWindowReturnCallback return_callback) {
  PinWindowData *window_data = window_get_user_data(window);
  window_data->return_callback = return_callback;
}

// Set the context which will be returned in the callback
void pin_window_set_context(Window *window, void *context) {
  PinWindowData *window_data = window_get_user_data(window);
  window_data->context = context;
}

// Create a PinWindow
Window *pin_window_create(uint8_t field_count, bool destroy_on_close) {
  // create window
  Window *window = window_create();
  ASSERT(window);
  WindowHandlers window_handlers = (WindowHandlers) {
    .unload = prv_window_unload_handler
  };
  window_set_window_handlers(window, window_handlers);
  PinWindowData *window_data = MALLOC(sizeof(PinWindowData));
  (*window_data) = (PinWindowData) {
    .field_count = field_count,
    .destroy_on_close = destroy_on_close
  };
  window_set_user_data(window, window_data);
  pin_window_set_max_field_values(window, (uint8_t[]) {9, 9, 9});
  // get window parameters
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  // create title TextLayer
  const GEdgeInsets title_text_insets = { .top = PBL_IF_RECT_ELSE(22, 27) };
  window_data->title_layer = text_layer_create(grect_inset(window_bounds, title_text_insets));
  text_layer_set_font(window_data->title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(window_data->title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(window_data->title_layer));
  // create footer TextLayer
  const GEdgeInsets sub_text_insets = {.top = 112, .right = 5, .bottom = 10, .left = 5};
  window_data->footer_layer = text_layer_create(grect_inset(window_bounds, sub_text_insets));
  text_layer_set_text_alignment(window_data->footer_layer, GTextAlignmentCenter);
  text_layer_set_font(window_data->footer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(window_data->footer_layer));
  // create selection layer
  const GEdgeInsets selection_insets = GEdgeInsets(
    (window_bounds.size.h - SELECTION_LAYER_SIZE.h) / 2,
    (window_bounds.size.w - SELECTION_LAYER_SIZE.w) / 2);
  GRect selection_bounds = grect_inset(window_bounds, selection_insets);
  window_data->selection_layer = selection_layer_create(selection_bounds, field_count);
  for (int ii = 0; ii < field_count; ii++) {
    selection_layer_set_cell_width(window_data->selection_layer, ii,
      SELECTION_LAYER_SIZE.w / field_count - SELECTION_LAYER_CELL_PADDING / 2);
  }
  selection_layer_set_cell_padding(window_data->selection_layer, SELECTION_LAYER_CELL_PADDING);
  selection_layer_set_active_bg_color(window_data->selection_layer, DEFAULT_ACTIVE_CELL_COLOR);
  selection_layer_set_inactive_bg_color(window_data->selection_layer, DEFAULT_INACTIVE_CELL_COLOR);
  selection_layer_set_click_config_onto_window(window_data->selection_layer, window);
  selection_layer_set_callbacks(window_data->selection_layer, window, (SelectionLayerCallbacks) {
    .get_cell_text = prv_selection_handle_get_text,
    .complete = prv_selection_handle_complete,
    .increment = prv_selection_handle_inc,
    .decrement = prv_selection_handle_dec,
  });
  layer_add_child(window_get_root_layer(window), window_data->selection_layer);
  return window;
}

// Destroy a PinWindow
void pin_window_destroy(Window *window) {
  PinWindowData *window_data = window_get_user_data(window);
  // raise callback if not returned already
  if (!window_data->already_returned) {
    window_data->return_callback(true, window_data->field_count, window_data->field_values,
      window_data->context);
  }
  // destroy stuff
  selection_layer_destroy(window_data->selection_layer);
  text_layer_destroy(window_data->footer_layer);
  text_layer_destroy(window_data->title_layer);
  window_destroy(window);
  free(window_data);
}
