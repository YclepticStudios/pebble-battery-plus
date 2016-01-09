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
#include "data/data_library.h"

//! Show the action menu
//! @param data_library A pointer to the main DataLibrary from which to get information
void menu_show(DataLibrary *data_library);

//! Initialize action menu
//! @param data_library A pointer to the main DataLibrary from which to get information
void menu_initialize(DataLibrary *data_library);

//! Destroy action menu
void menu_terminate(void);
