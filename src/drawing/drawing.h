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

// Constants
#define DRAWING_CARD_COUNT 3


//! Refresh all visible elements
void drawing_refresh(void);

//! Select click handler for current card
void drawing_select_click(void);

//! Select next or previous card
//! @param up True if the selection should move up, else false
void drawing_select_next_card(bool up);

//! Initialize all cards and add to window layer
//! @param window_layer The base layer of the window
void drawing_initialize(Layer *window_layer);

//! Terminate all cards and free memory
void drawing_terminate(void);
