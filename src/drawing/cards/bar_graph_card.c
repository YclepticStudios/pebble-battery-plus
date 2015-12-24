// @file bar_graph_card.c
// @brief Rendering code for bar graph view
//
// File contains all drawing code for rendering the bar graph
// view onto a card.
//
// @author Eric D. Phillips
// @date December 23, 2015
// @bugs No known bugs

#include "card_render.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Dummy rendering function
static void prv_dummy_render(GContext *ctx, Layer *layer) {
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "Bar Graph", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
    GRect(0, 40, 144, 30), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for bar graph card
void card_render_bar_graph(Layer *layer, GContext *ctx, uint16_t click_count) {
  prv_dummy_render(ctx, layer);
}
