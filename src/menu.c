// @file menu.h
// @brief Code handling action menu for app
//
// This file contains all the code necessary to create and maintain an
// ActionMenu. It also contains the code which executes when various
// commands on the menu are selected.
//
// @author Eric D. Phillips
// @date December 28, 2015
// @bug No known bugs

#include "menu.h"
#include "drawing/windows/edit/pin_window.h"
#include "utility.h"

// Action types
typedef enum {
  ActionTypeDataExport,
  ActionTypeAddAlert
} ActionType;

// Pin window context
typedef struct {
  DataLibrary   *data_library;    //< Pointer to main data library
  uint8_t       index;            //< Index of alert if editing
  bool          new_alert;        //< If true, create a new alert instead of editing one
} PinWindowContext;

// ActionMenu variables
static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level, *s_data_level, *s_alert_level;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Pin window return callback
static void prv_pin_window_return_handler(bool canceled, uint8_t value_count, int8_t *values,
                                          void *context) {
  PinWindowContext *window_context = context;
  if (!canceled) {
    if (!window_context->new_alert) {
      // destroy current alert
      data_unschedule_alert(window_context->data_library, window_context->index);
    }
    // create new alert
    int32_t new_threshold = values[0] * SEC_IN_DAY + values[1] * SEC_IN_HR;
    data_schedule_alert(window_context->data_library, new_threshold);
    // reload action menu
    menu_terminate();
    menu_initialize(window_context->data_library);
  }
  // free data
  free(window_context);
}

// Alert edit callback
static void prv_alert_edit_handler(ActionMenu *action_menu, const ActionMenuItem *item,
                                   void *context) {
  // get alert index
  uint8_t index = (int32_t)action_menu_item_get_action_data(item);
  // create pin window context
  PinWindowContext *window_context = MALLOC(sizeof(PinWindowContext));
  window_context->data_library = context;
  window_context->index = index;
  window_context->new_alert = false;
  // get current alert duration
  uint8_t cur_values[2];
  cur_values[0] = data_get_alert_threshold(context, index) / SEC_IN_DAY;
  cur_values[1] = data_get_alert_threshold(context, index) % SEC_IN_DAY / SEC_IN_HR;
  // edit duration of alert with pin window
  Window *window = pin_window_create(2, true);
  pin_window_set_field_values(window, cur_values);
  pin_window_set_max_field_values(window, (uint8_t[]) {9, 23});
  pin_window_set_text(window, "Edit Alert", "Set days and hours before empty");
  pin_window_set_context(window, window_context);
  pin_window_set_return_callback(window, prv_pin_window_return_handler);
  window_stack_push(window, true);
}

// Alert delete callback
static void prv_alert_delete_handler(ActionMenu *action_menu, const ActionMenuItem *item,
                                     void *context) {
  uint8_t index = (int32_t)action_menu_item_get_action_data(item);
  data_unschedule_alert(context, index);
  // reload action menu
  menu_terminate();
  menu_initialize(context);
}


// Data level callback
static void prv_action_performed_handler(ActionMenu *action_menu, const ActionMenuItem *item,
                                         void *context) {
  // get action type
  ActionType action_type = (ActionType)action_menu_item_get_action_data(item);
  // perform action
  switch (action_type) {
    case ActionTypeDataExport:
      // TODO: Add busy screen when printing
      data_print_csv(context);
      break;
    case ActionTypeAddAlert:;
      // create pin window context
      PinWindowContext *window_context = MALLOC(sizeof(PinWindowContext));
      window_context->data_library = context;
      window_context->new_alert = true;
      // get duration of alert with pin window
      Window *window = pin_window_create(2, true);
      pin_window_set_field_values(window, (uint8_t[]) {1, 0});
      pin_window_set_max_field_values(window, (uint8_t[]) {9, 23});
      pin_window_set_text(window, "New Alert", "Set days and hours before empty");
      pin_window_set_context(window, window_context);
      pin_window_set_return_callback(window, prv_pin_window_return_handler);
      window_stack_push(window, true);
      break;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

//! Show the action menu
void menu_show(DataLibrary *data_library) {
  // configure action menu
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorMagenta, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignTop,
    .context = data_library
  };
  // show the ActionMenu
  s_action_menu = action_menu_open(&config);
}

//! Initialize action menu
void menu_initialize(DataLibrary *data_library) {
  // create root level
  s_root_level = action_menu_level_create(2);
  // create data level
  s_data_level = action_menu_level_create(1);
  action_menu_level_add_child(s_root_level, s_data_level, "Data");
  action_menu_level_add_action(s_data_level, "Export", prv_action_performed_handler,
    (void*)ActionTypeDataExport);
  // create alert level
  uint8_t alert_count = data_get_alert_count(data_library);
  s_alert_level = action_menu_level_create(alert_count + 1);
  action_menu_level_add_child(s_root_level, s_alert_level, "Alerts");
  ActionMenuLevel *menu_level;
  for (int32_t index = 0; index < alert_count; index++) {
    menu_level = action_menu_level_create(2);
    action_menu_level_add_child(s_alert_level, menu_level, data_get_alert_text(data_library, index));
    action_menu_level_add_action(menu_level, "Edit", prv_alert_edit_handler, (void*)index);
    action_menu_level_add_action(menu_level, "Delete", prv_alert_delete_handler, (void*)index);
  }
  if (alert_count < DATA_MAX_ALERT_COUNT) {
    action_menu_level_add_action(s_alert_level, "Add Alert", prv_action_performed_handler,
      (void *) ActionTypeAddAlert);
  }
}

//! Destroy action menu
void menu_terminate(void) {
  action_menu_hierarchy_destroy(s_root_level, NULL, NULL);
}
