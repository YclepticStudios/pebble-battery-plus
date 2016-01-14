//! @file drawing_card.h
//! @brief Base wrapper for controlling card layers
//!
//! This file contains wrapper code to manage all card layers.
//! It controls refreshing, events, and everything which gets
//! passed to the cards.
//!
//! @author Eric D. Phillips
//! @date December 23, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>
#include "../data/data_library.h"

// Constants
#define DRAWING_CARD_COUNT 3


//! Refresh current card
void drawing_refresh(void);

//! Set the visible state of the action menu dot
//! @param visible The state the action menu dot should be set to
void drawing_set_action_menu_dot(bool visible);

//! Select click handler for current card
void drawing_select_click(void);

//! Render the next or previous card
//! @param up True if the selection should move up, else false
void drawing_render_next_card(bool up);

//! Select next or previous card
//! @param up True if the selection should move up, else false
void drawing_select_next_card(bool up);

//! Initialize all cards and add to window layer
//! @param window_layer The base layer of the window
//! @param data_library Pointer to the main data library from which to get information
void drawing_initialize(Layer *window_layer, DataLibrary *data_library);

//! Terminate all cards and free memory
void drawing_terminate(void);
