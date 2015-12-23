// @file bar_graph_card.c
// @brief Rendering code for bar graph view
//
// File contains a base layer which has children.
// This base layer and the children are responsible for all
// graphics drawn on this layer. Click events are captured
// in the main file and passed into here.
//
// @author Eric D. Phillips
// @date December 23, 2015
// @bugs No known bugs

#include "bar_graph_card.h"

// Main constants
#define BACKGROUND_COLOR GColorRichBrilliantLavender

// Main data structure
static struct {
  Layer       *layer;       //< Base layer for card
  TextLayer   *txt_layer;   //< Initial text layer
} bar_graph_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Position cards
static void prv_position_cards(void) {
  // TODO: Implement this function
}

// Base layer callback for drawing background color
static void prv_layer_update_handler(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Refresh everything on card
void bar_graph_card_refresh(void) {

}

// Initialize card
Layer *bar_graph_card_initialize(GRect window_bounds) {
  // create base layer
  bar_graph_data.layer = layer_create(window_bounds);
  layer_set_update_proc(bar_graph_data.layer, prv_layer_update_handler);
  // create text layer
  bar_graph_data.txt_layer = text_layer_create(grect_inset(window_bounds, GEdgeInsets1(20)));
  text_layer_set_text(bar_graph_data.txt_layer, "Bar Graph");
  layer_add_child(bar_graph_data.layer, text_layer_get_layer(bar_graph_data.txt_layer));
  // return layer pointer
  return bar_graph_data.layer;
}

// Terminate card
void bar_graph_card_terminate(void){
  // destroy layers
  text_layer_destroy(bar_graph_data.txt_layer);
  layer_destroy(bar_graph_data.layer);
}
