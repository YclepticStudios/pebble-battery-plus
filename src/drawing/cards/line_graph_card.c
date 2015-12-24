// @file line_graph_card.c
// @brief Rendering code for line graph view
//
// File contains all drawing code for rendering the line graph
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
static void prv_dummy_render(Layer *layer, GContext *ctx) {
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "Line Graph", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
    GRect(0, 40, 144, 30), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for line graph card
void card_render_line_graph(Layer *layer, GContext *ctx) {
  prv_dummy_render(layer, ctx);
}
