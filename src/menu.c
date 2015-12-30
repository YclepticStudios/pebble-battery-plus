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
#include "data.h"

// Action types
typedef enum {
  ActionTypeDataExport
} ActionType;

// ActionMenu variables
static ActionMenu *s_action_menu;
static ActionMenuLevel *s_root_level, *s_data_level;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Data level callback
static void prv_action_performed_handler(ActionMenu *action_menu, const ActionMenuItem *item,
                                         void *context) {
  // get action type
  ActionType action_type = (ActionType)action_menu_item_get_action_data(item);
  // perform action
  switch (action_type) {
    case ActionTypeDataExport:
      data_print_csv();
      break;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

//! Show the action menu
void menu_show(void) {
  // configure action menu
  ActionMenuConfig config = (ActionMenuConfig) {
    .root_level = s_root_level,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite),
      .foreground = GColorBlack,
    },
    .align = ActionMenuAlignCenter
  };
  // show the ActionMenu
  s_action_menu = action_menu_open(&config);
}

//! Initialize action menu
void menu_initialize(void) {
  // create root level
  s_root_level = action_menu_level_create(1);
  // create data level
  s_data_level = action_menu_level_create(1);
  action_menu_level_add_child(s_root_level, s_data_level, "Data");
  action_menu_level_add_action(s_data_level, "Export", prv_action_performed_handler,
    (void*)ActionTypeDataExport);
}

//! Destroy action menu
void menu_terminate(void) {
  action_menu_hierarchy_destroy(s_root_level, NULL, NULL);
}
