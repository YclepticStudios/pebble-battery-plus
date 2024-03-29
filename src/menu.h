//! @file menu.h
//! @brief Code handling action menu for app
//!
//! This file contains all the code necessary to create and maintain an
//! ActionMenu. It also contains the code which executes when various
//! commands on the menu are selected.
//!
//! @author Eric D. Phillips
//! @date December 28, 2015
//! @bug No known bugs

#pragma once
#include <pebble.h>
#include "data/data_api.h"

//! Create and show the action menu
//! @param data_api A pointer to the main DataLibrary from which to get information
void menu_show(DataAPI *data_api);
