//! @file drawing.h
//! @brief Main drawing code
//!
//! Contains all the drawing code for this app.
//!
//! @author Eric D. Phillips
//! @date November 28, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>

// Public drawing constants
#define RING_WIDTH PBL_IF_ROUND_ELSE(21, 16)
#define COLOR_MENU_BACKGROUND GColorRajah
#define MENU_CELL_HEIGHT_TALL 65


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

//! Move layout to dashboard layout
//! @param delay The delay before starting the animation
void drawing_convert_to_dashboard_layout(uint32_t delay);

//! Render a MenuLayer cell
//! @param menu The MenuLayer in question
//! @param layer The layer being rendered onto
//! @param ctx The cell's drawing context
//! @param index The cell's index in the menu
void drawing_render_cell(MenuLayer *menu, Layer *layer, GContext *ctx, MenuIndex index);

//! Render everything on the screen
//! @param layer The layer being rendered onto
//! @param ctx The layer's drawing context
void drawing_render(Layer *layer, GContext *ctx);

//! Initialize drawing variables
//! @param layer Pointer to layer onto which everything is drawn
//! @param menu Pointer to menu which contains information
void drawing_initialize(Layer *layer, MenuLayer *menu);

//! Terminate drawing variables
void drawing_terminate(void);
