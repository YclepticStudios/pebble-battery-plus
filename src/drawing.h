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


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

//! Move layout to dashboard layout
//! @param delay The delay before starting the animation
void drawing_convert_to_dashboard_layout(uint32_t delay);

//! Render everything on the screen
//! @param layer The layer being rendered onto
//! @param ctx The layer's drawing context
void drawing_render(Layer *layer, GContext *ctx);

//! Initialize drawing variables
//! @param layer Pointer to layer onto which everything is drawn
void drawing_initialize(Layer *layer);
