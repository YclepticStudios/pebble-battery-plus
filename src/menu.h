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

//! Show the action menu
void menu_show(void);

//! Initialize action menu
void menu_initialize(void);

//! Destroy action menu
void menu_terminate(void);
