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
#include "../utility.h"

// Constants
#define CARD_SLIDE_ANIMATION_DURATION 100
#define CARD_BOUNCE_ANIMATION_DURATION 70
#define CARD_BOUNCE_HEIGHT 10
#define ACTION_DOT_RADIUS 15
#define ACTION_DOT_OPEN_INSET PBL_IF_RECT_ELSE(5, 9)
#define ACTION_DOT_CLOSE_DURATION 150

// Main data struct
static struct {
  Layer     *card_layer[DRAWING_CARD_COUNT];  //< An array of pointers to the base card layers
  Layer     *top_layer;                       //< Topmost layer for drawing any extra content on
  GRect     window_bounds;                    //< The dimensions of the window
  int32_t   scroll_offset_ani;                //< The true offset of the 0'th card
  int32_t   scroll_offset;                    //< The final offset of the 0'th card after animation
  int32_t   action_dot_inset_ani;             //< The inset from the left of the screen for the
  // dot
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

// Topmost layer update proc handler
static void prv_top_layer_update_proc_handler(Layer *layer, GContext *ctx) {
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  GPoint center;
  center.x = bounds.size.w + ACTION_DOT_RADIUS - drawing_data.action_dot_inset_ani;
  center.y = bounds.size.h / 2;
  // draw action dot
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, ACTION_DOT_RADIUS);
}

// Animation callback handler
static void prv_animation_handler(void) {
  // update card positions
  prv_position_cards();
  // check if out of view
  for (int8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    card_free_cache_if_hidden(drawing_data.card_layer[ii], false);
  }
  // redraw topmost layer
  layer_mark_dirty(drawing_data.top_layer);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Refresh current card
void drawing_refresh(void) {
  uint8_t card_index = (DRAWING_CARD_COUNT -
    ((drawing_data.scroll_offset / drawing_data.window_bounds.size.h) % DRAWING_CARD_COUNT - 1)) %
    DRAWING_CARD_COUNT;
  card_render(drawing_data.card_layer[card_index]);
}

// Free all card caches
void drawing_free_caches(void) {
  // loop over cards and force them to free their cache
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    card_free_cache_if_hidden(drawing_data.card_layer[ii], true);
  }
}

// Set the visible state of the action menu dot
void drawing_set_action_menu_dot(bool visible) {
  if (visible) {
    drawing_data.action_dot_inset_ani = ACTION_DOT_OPEN_INSET;
  } else {
    animation_int32_start(&drawing_data.action_dot_inset_ani, 0,
      ACTION_DOT_CLOSE_DURATION, 0, CurveSinEaseOut);
  }
  layer_mark_dirty(drawing_data.top_layer);
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


// Render the next or previous card
void drawing_render_next_card(bool up) {
  // render the next card
  uint8_t cur_card_index = (DRAWING_CARD_COUNT -
    ((drawing_data.scroll_offset / drawing_data.window_bounds.size.h) % DRAWING_CARD_COUNT - 1)) %
    DRAWING_CARD_COUNT;
  uint8_t next_card_index = (cur_card_index + (up ? -1 : 1) + DRAWING_CARD_COUNT) %
    DRAWING_CARD_COUNT;
  // free the previous cache on Aplite
#ifdef PBL_BW
  card_free_cache_if_hidden(drawing_data.card_layer[cur_card_index], true);
#endif
  // render the next card
  card_render(drawing_data.card_layer[next_card_index]);
  // move next card underneath current card to hide rendering
  layer_insert_below_sibling(drawing_data.card_layer[next_card_index],
    drawing_data.card_layer[cur_card_index]);
}

// Select next or previous card
void drawing_select_next_card(bool up) {
  // move target card position
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
void drawing_initialize(Layer *window_layer, DataAPI *data_api) {
  // get params
  drawing_data.window_bounds = layer_get_bounds(window_layer);
  // initialize cards
  // this must be done with the last initialized being the one which is selected first. As
  // all the cards must initially be stacked on the screen so they can render themselves.
  // When scrolling begins, they will reposition.
  drawing_data.scroll_offset = drawing_data.scroll_offset_ani = -drawing_data.window_bounds.size.h;
  drawing_data.card_layer[0] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_RECORD_LIFE,
    CARD_BACK_COLOR_RECORD_LIFE, card_render_record_life, data_api);
  drawing_data.card_layer[1] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_LINE_GRAPH,
    CARD_BACK_COLOR_LINE_GRAPH, card_render_line_graph, data_api);
  drawing_data.card_layer[2] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_DASHBOARD,
    CARD_BACK_COLOR_DASHBOARD, card_render_dashboard, data_api);
  drawing_data.card_layer[3] = card_initialize(drawing_data.window_bounds, CARD_PALETTE_BAR_GRAPH,
    CARD_BACK_COLOR_BAR_GRAPH, card_render_bar_graph, data_api);
  prv_position_cards();
  // add to window
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    layer_add_child(window_layer, drawing_data.card_layer[ii]);
  }
  // create topmost layer
  drawing_data.top_layer = layer_create(drawing_data.window_bounds);
  ASSERT(drawing_data.top_layer);
  layer_set_update_proc(drawing_data.top_layer, prv_top_layer_update_proc_handler);
  layer_add_child(window_layer, drawing_data.top_layer);
  // register animation handler
  animation_register_update_callback(prv_animation_handler);
}

// Terminate all cards and free memory
void drawing_terminate(void) {
  // destroy topmost layer
  layer_destroy(drawing_data.top_layer);
  // destroy cards
  for (uint8_t ii = 0; ii < DRAWING_CARD_COUNT; ii++) {
    card_terminate(drawing_data.card_layer[ii]);
  }
}
