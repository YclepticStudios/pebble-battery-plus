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
#include "../../utility.h"

// Drawing Constants
#define GRAPH_STROKE_WIDTH 2
#define GRAPH_VERTICAL_INSET PBL_IF_RECT_ELSE(30, 30)
#define GRAPH_HORIZONTAL_INSET PBL_IF_RECT_ELSE(0, 18)
#define GRAPH_AXIS_HEIGHT 20
#define GRAPH_X_RANGE SEC_IN_WEEK


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render axis
static void prv_render_axis(GRect bounds, GContext *ctx) {
  // reshape bounds
  GRect axis_bounds = bounds;
  axis_bounds.origin.y = axis_bounds.size.h - GRAPH_VERTICAL_INSET - GRAPH_AXIS_HEIGHT;
  axis_bounds.size.h = GRAPH_AXIS_HEIGHT;
  // draw background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, axis_bounds, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, GRAPH_STROKE_WIDTH);
  graphics_draw_line(ctx, axis_bounds.origin, GPoint(axis_bounds.origin.x + axis_bounds.size.w,
    axis_bounds.origin.y));
  graphics_draw_line(ctx, GPoint(axis_bounds.origin.x, axis_bounds.origin.y + axis_bounds.size.h),
    GPoint(axis_bounds.origin.x + axis_bounds.size.w, axis_bounds.origin.y + axis_bounds.size.h));
  // resize bounds
  axis_bounds.origin.x += GRAPH_HORIZONTAL_INSET;
  axis_bounds.size.w -= GRAPH_HORIZONTAL_INSET * 2;
  // draw days of the week
  graphics_context_set_text_color(ctx, GColorBlack);
  time_t t_time = time(NULL);
  tm tm_time = *localtime(&t_time);
  time_t l_time = t_time + tm_time.tm_gmtoff;
  int8_t day_of_week = tm_time.tm_wday;
  GRect txt_bounds = axis_bounds;
  txt_bounds.origin.y -= 2;
  txt_bounds.size.w = axis_bounds.size.w * SEC_IN_DAY / GRAPH_X_RANGE;
  uint8_t ii = 0;
  char *dow_buff;
  do {
    // calculate x offset
    txt_bounds.origin.x = axis_bounds.origin.x + axis_bounds.size.w -
      ((l_time % SEC_IN_DAY + ii * SEC_IN_DAY) * axis_bounds.size.w / GRAPH_X_RANGE);
    // draw text
    dow_buff = &("S\0M\0T\0W\0T\0F\0S\0")[day_of_week * 2];
    graphics_draw_text(ctx, dow_buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), txt_bounds,
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    // index
    ii++;
    if (--day_of_week < 0) {
      day_of_week = 6;
    }
  } while (txt_bounds.origin.x + txt_bounds.size.w > 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for line graph card
void card_render_line_graph(Layer *layer, GContext *ctx) {
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render graph axis with days of the week
  prv_render_axis(bounds, ctx);
}
