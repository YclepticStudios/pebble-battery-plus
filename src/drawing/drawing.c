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
#include "card.h"
#include "cards/card_render.h"

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
void drawing_refresh(void) {
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    card_refresh(drawing_data.card_layer[ii]);
  }
}

// Select click handler for current card
void drawing_select_click(void) {
  // get current card
  uint8_t card_index = (DRAWING_CARD_COUNT -
    ((drawing_data.scroll_offset / drawing_data.window_bounds.size.h) % DRAWING_CARD_COUNT - 1)) %
    DRAWING_CARD_COUNT;
  // send click event
  card_select_click(drawing_data.card_layer[card_index]);
}

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
  // this must be done with the last initialized being the one which is selected first. As
  // all the cards must initially be stacked on the screen so they can render themselves.
  // When scrolling begins, they will reposition.
  drawing_data.scroll_offset = drawing_data.scroll_offset_ani = -drawing_data.window_bounds.size.h;
  drawing_data.card_layer[0] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_BAR_GRAPH,
    CARD_BACK_COLOR_BAR_GRAPH, card_render_bar_graph);
  drawing_data.card_layer[1] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_LINE_GRAPH,
    CARD_BACK_COLOR_LINE_GRAPH, card_render_line_graph);
  drawing_data.card_layer[2] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_DASHBOARD,
    CARD_BACK_COLOR_DASHBOARD, card_render_dashboard);
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
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    card_terminate(drawing_data.card_layer[ii]);
  }
}
