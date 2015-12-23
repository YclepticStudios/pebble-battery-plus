// @file drawing_card.c
// @brief Base wrapper for controlling card layers
//
// This file contains wrapper code to manage all card layers.
// It controls refreshing, events, and everything which gets
// passed to the cards.
//
// @author Eric D. Phillips
// @date December 23, 2015
// @bugs No known bugs

#include "drawing.h"
#include "../animation/animation.h"
#include "bar_graph_card.h"
#include "dashboard_card.h"
#include "line_graph_card.h"

// Constants
#define CARD_SLIDE_ANIMATION_DURATION 100
#define CARD_BOUNCE_ANIMATION_DURATION 70
#define CARD_BOUNCE_HEIGHT 10

// Main data struct
static struct {
  Layer     *card_layer[DRAWING_CARD_COUNT];  //< An array of pointers to the base card layers
  GRect     window_bounds;                    //< The dimensions of the window
  int32_t   scroll_offset_ani;                //< The true offset of the 0'th card
  int32_t   scroll_offset;                    //< The final offset of the 0'th card after animation
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Position cards
static void prv_position_cards(void) {
  GRect bounds = drawing_data.window_bounds;
  for (int8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    bounds.origin.y = (drawing_data.scroll_offset_ani % (bounds.size.h * DRAWING_CARD_COUNT) +
      (DRAWING_CARD_COUNT + ii) * bounds.size.h) %
      (bounds.size.h * DRAWING_CARD_COUNT) - bounds.size.h;
    layer_set_bounds(drawing_data.card_layer[ii], bounds);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

static void prv_animation_handler(void) {
  // update card positions
  prv_position_cards();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Refresh all visible elements
void drawing_refresh(void);

// Select next or previous card
void drawing_select_next_card(bool up) {
  drawing_data.scroll_offset += (up ? 1 : -1) * drawing_data.window_bounds.size.h;
  // animate slide
  animation_int32_start(&drawing_data.scroll_offset_ani,
    drawing_data.scroll_offset + (up ? 1 : -1) * CARD_BOUNCE_HEIGHT,
    CARD_SLIDE_ANIMATION_DURATION, 0, CurveLinear);
  // animate bounce
  animation_int32_start(&drawing_data.scroll_offset_ani, drawing_data.scroll_offset,
    CARD_BOUNCE_ANIMATION_DURATION, CARD_SLIDE_ANIMATION_DURATION, CurveSinEaseOut);
}

// Initialize all cards and add to window layer
void drawing_initialize(Layer *window_layer) {
  // get params
  drawing_data.window_bounds = layer_get_bounds(window_layer);
  // initialize cards
  drawing_data.scroll_offset = drawing_data.scroll_offset_ani = 0;
  drawing_data.card_layer[0] = dashboard_card_initialize(drawing_data.window_bounds);
  drawing_data.card_layer[1] = line_graph_card_initialize(drawing_data.window_bounds);
  drawing_data.card_layer[2] = bar_graph_card_initialize(drawing_data.window_bounds);
  prv_position_cards();
  // add to window
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    layer_add_child(window_layer, drawing_data.card_layer[ii]);
  }
  // register animation handler
  animation_register_update_callback(prv_animation_handler);
}

// Terminate all cards and free memory
void drawing_terminate(void) {
  // destroy cards
  dashboard_card_terminate();
  line_graph_card_terminate();
  bar_graph_card_terminate();
}