//! @file line_graph_card.h
//! @brief Rendering code for line graph view
//!
//! File contains a base layer which has children.
//! This base layer and the children are responsible for all
//! graphics drawn on this layer. Click events are captured
//! in the main file and passed into here.
//!
//! @author Eric D. Phillips
//! @date December 23, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>


//! Refresh everything on card
void line_graph_card_refresh(void);

//! Initialize card
//! @param window_bounds The dimensions of the window
//! @return A pointer to the base layer of this card
Layer *line_graph_card_initialize(GRect window_bounds);

//! Terminate card
void line_graph_card_terminate(void);
