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
#include "data/data_shared.h"
#include "drawing/windows/edit/pin_window.h"
#include "drawing/windows/alert/popup_window.h"
#include "phone.h"
#include "utility.h"

// Action types
typedef enum {
  ActionTypeResetRecord,
  ActionTypeDataExport,
  ActionTypeAddAlert,
  ActionTypeEnableTimeline,
  ActionTypeDisableTimeline,
  ActionTypeSyncTimeline
} ActionType;

// Alert window context
typedef struct {
  DataAPI       *data_api;        //< Pointer to main data library
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
      data_api_unschedule_alert(window_context->data_api, window_context->index);
    }
    // create new alert
    int32_t new_threshold = values[0] * SEC_IN_DAY + values[1] * SEC_IN_HR;
    data_api_schedule_alert(window_context->data_api, new_threshold);
    // close pin window
    window_stack_pop(true);
    // show popup notification
    Window *popup_window = popup_window_create(true);
    popup_window_set_visual(popup_window, RESOURCE_ID_CONFIRM_SEQUENCE, false);
    popup_window_set_text(popup_window, "", "Alert Set");
#ifdef PBL_PLATFORM_APLITE
    popup_window_set_timeout(popup_window, 1500);
#endif
    window_stack_push(popup_window, true);
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
  window_context->data_api = context;
  window_context->index = index;
  window_context->new_alert = false;
  // get current alert duration
  uint8_t cur_values[2];
  cur_values[0] = data_api_get_alert_threshold(context, index) / SEC_IN_DAY;
  cur_values[1] = data_api_get_alert_threshold(context, index) % SEC_IN_DAY / SEC_IN_HR;
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
  data_api_unschedule_alert(context, index);
  // show popup notification
  Window *popup_window = popup_window_create(true);
  popup_window_set_visual(popup_window, RESOURCE_ID_DELETED_SEQUENCE, false);
  popup_window_set_text(popup_window, "", "Alert Deleted");
#ifdef PBL_PLATFORM_APLITE
  popup_window_set_timeout(popup_window, 1500);
#endif
  window_stack_push(popup_window, true);
}


// Data level callback
static void prv_action_performed_handler(ActionMenu *action_menu, const ActionMenuItem *item,
                                         void *context) {
  // get action type
  ActionType action_type = (ActionType)action_menu_item_get_action_data(item);
  // perform action
  switch (action_type) {
    case ActionTypeResetRecord:
      persist_delete(PERSIST_RECORD_LIFE_KEY);
      data_api_reload(context);
      break;
    case ActionTypeDataExport:
      // TODO: Add busy screen when printing
      data_api_print_csv(context);
      break;
    case ActionTypeAddAlert:;
      // create pin window context
      PinWindowContext *window_context = MALLOC(sizeof(PinWindowContext));
      window_context->data_api = context;
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
    case ActionTypeEnableTimeline:
      persist_write_bool(PERSIST_TIMELINE_KEY, true);
      break;
    case ActionTypeDisableTimeline:
      persist_write_bool(PERSIST_TIMELINE_KEY, false);
      break;
    case ActionTypeSyncTimeline:;
      // create popup alert window
      Window *popup_window = popup_window_create(true);
      window_set_background_color(popup_window, PBL_IF_BW_ELSE(GColorWhite, GColorVividCerulean));
      popup_window_set_text(popup_window, "Battery+", "Syncing Timeline");
      popup_window_set_visual(popup_window, RESOURCE_ID_TIMELINE_SYNC_IMAGE, true);
      window_stack_push(popup_window, true);
      // start the pin sending process
      phone_connect();
      phone_send_timestamp_to_phone(data_api_get_charge_by_time(context));
      phone_set_window_close_on_complete(popup_window);
      break;
  }
}

// Menu did close callback
static void prv_menu_did_close_handler(ActionMenu *action_menu,
                                       const ActionMenuItem *performed_action, void *context) {
  action_menu_hierarchy_destroy(s_root_level, NULL, NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Show the action menu
void menu_show(DataAPI *data_api) {
  // create root level
  s_root_level = action_menu_level_create(3);
  // create data level
  s_data_level = action_menu_level_create(2);
  action_menu_level_add_child(s_root_level, s_data_level, "Data");
  action_menu_level_add_action(s_data_level, "Export", prv_action_performed_handler,
    (void*)ActionTypeDataExport);
  action_menu_level_add_action(s_data_level, "Reset Record\nBattery Life",
    prv_action_performed_handler, (void*) ActionTypeResetRecord);
  // create alert level
  int days, hours;
  uint8_t alert_count = data_api_get_alert_count(data_api);
  static char label_buffs[DATA_ALERT_MAX_COUNT][19];
  s_alert_level = action_menu_level_create(alert_count + 1);
  action_menu_level_add_child(s_root_level, s_alert_level, "Alerts");
  ActionMenuLevel *menu_level;
  for (int32_t index = 0; index < alert_count; index++) {
    // get the label text
    days = data_api_get_alert_threshold(data_api, index) / SEC_IN_DAY;
    hours = data_api_get_alert_threshold(data_api, index) % SEC_IN_DAY / SEC_IN_HR;
    snprintf(label_buffs[index], sizeof(label_buffs[index]), "%s\n(%dd %02dh)",
      data_api_get_alert_text(data_api, index), days, hours);
    menu_level = action_menu_level_create(2);
    action_menu_level_add_child(s_alert_level, menu_level, label_buffs[index]);
    action_menu_level_add_action(menu_level, "Edit", prv_alert_edit_handler, (void*)index);
    action_menu_level_add_action(menu_level, "Delete", prv_alert_delete_handler, (void*)index);
  }
  if (alert_count < DATA_ALERT_MAX_COUNT) {
    action_menu_level_add_action(s_alert_level, "Add Alert", prv_action_performed_handler,
      (void*)ActionTypeAddAlert);
  }
  // create timeline level
  s_data_level = action_menu_level_create(2);
  action_menu_level_add_child(s_root_level, s_data_level, "Timeline");
  if (!persist_exists(PERSIST_TIMELINE_KEY) || persist_read_bool(PERSIST_TIMELINE_KEY)) {
    action_menu_level_add_action(s_data_level, "Disable Pins", prv_action_performed_handler,
      (void*)ActionTypeDisableTimeline);
  } else {
    action_menu_level_add_action(s_data_level, "Enable Pins", prv_action_performed_handler,
      (void*)ActionTypeEnableTimeline);
  }
  action_menu_level_add_action(s_data_level, "Sync Now", prv_action_performed_handler,
    (void*)ActionTypeSyncTimeline);
  // configure action menu
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorMagenta, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignTop,
    .did_close = prv_menu_did_close_handler,
    .context = data_api
  };
  // show the ActionMenu
  s_action_menu = action_menu_open(&config);
}
