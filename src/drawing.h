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
#define RING_WIDTH PBL_IF_ROUND_ELSE(18, 16)
#define COLOR_MENU_BACKGROUND GColorRajah
#define MENU_CELL_HEIGHT_TALL 65
#define MENU_CELL_FULL_SCREEN_SUB_HEIGHT 49
#define MENU_CELL_FULL_SCREEN_TOP_OFFSET PBL_IF_ROUND_ELSE(3, 0)


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

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

//! Recalculate and animate the position the progress ring
void drawing_recalculate_progress_rings(void);

//! Initialize drawing variables
//! @param layer Pointer to layer onto which everything is drawn
//! @param menu Pointer to menu which contains information
void drawing_initialize(Layer *layer, MenuLayer *menu);
