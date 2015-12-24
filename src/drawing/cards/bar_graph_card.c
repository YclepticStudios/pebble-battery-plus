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
#include "../../data.h"
#include "../../utility.h"

// Drawing Constants
#define TEXT_BORDER_TOP PBL_IF_RECT_ELSE(3, 10)
#define COLOR_RUN_TIME GColorGreen
#define GRAPH_STROKE_WIDTH 3
#define GRAPH_TOP_INSET PBL_IF_RECT_ELSE(40, 45)
#define GRAPH_BOTTOM_INSET PBL_IF_RECT_ELSE(30, 35)
#define GRAPH_HORIZONTAL_INSET PBL_IF_RECT_ELSE(0, 18)
#define GRAPH_AXIS_HEIGHT 20


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render text
static void prv_render_text(GContext *ctx, GRect bounds) {
  bounds.origin.y += TEXT_BORDER_TOP;
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "Charges", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Render line and fill
static void prv_render_bars(GContext *ctx, GRect bounds) {
  // prep draw
  GRect graph_bounds = GRect(GRAPH_HORIZONTAL_INSET, GRAPH_TOP_INSET, bounds.size.w -
    GRAPH_HORIZONTAL_INSET * 2, bounds.size.h - GRAPH_TOP_INSET - GRAPH_BOTTOM_INSET);
  // get properties
  int16_t bar_width = graph_bounds.size.w / DATA_PAST_RUN_TIMES_MAX;
  int32_t graph_y_max = 0;
  for (uint16_t ii = 0; ii < DATA_PAST_RUN_TIMES_MAX; ii++) {
    if (data_get_past_run_time(ii) > graph_y_max) {
      graph_y_max = data_get_past_run_time(ii);
    }
  }
  GRect bar_bounds;
  bar_bounds.size.w = bar_width;
  // draw graph
  graphics_context_set_fill_color(ctx, COLOR_RUN_TIME);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, GRAPH_STROKE_WIDTH);
  for (uint16_t ii = 0; ii < DATA_PAST_RUN_TIMES_MAX; ii++) {
    // set bounds
    bar_bounds.origin.x = graph_bounds.origin.x + graph_bounds.size.w - (bar_width * (ii + 1));
    bar_bounds.size.h = graph_bounds.size.h * data_get_past_run_time(ii) / graph_y_max;
    bar_bounds.origin.y = graph_bounds.origin.y + graph_bounds.size.h - bar_bounds.size.h;
    // draw bar
    graphics_fill_rect(ctx, bar_bounds, 0, GCornerNone);
    graphics_draw_rect(ctx, bar_bounds);
  }
}

// Render axis
static void prv_render_axis(GContext *ctx, GRect bounds) {
  // reshape bounds
  GRect axis_bounds = bounds;
  axis_bounds.origin.y = axis_bounds.size.h - GRAPH_BOTTOM_INSET - GRAPH_AXIS_HEIGHT;
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
  // render axis text
  graphics_context_set_text_color(ctx, GColorBlack);
  GRect graph_bounds = GRect(GRAPH_HORIZONTAL_INSET, GRAPH_TOP_INSET, bounds.size.w -
    GRAPH_HORIZONTAL_INSET * 2, bounds.size.h - GRAPH_TOP_INSET - GRAPH_BOTTOM_INSET);
  int16_t bar_width = graph_bounds.size.w / DATA_PAST_RUN_TIMES_MAX;
  char buff[3];
  GRect txt_bounds = axis_bounds;
  txt_bounds.origin.y -= 2;
  txt_bounds.size.w = bar_width;
  // draw graph
  for (uint16_t ii = 0; ii < DATA_PAST_RUN_TIMES_MAX; ii++) {
    // set bounds
    txt_bounds.origin.x = graph_bounds.origin.x + graph_bounds.size.w - (bar_width * (ii + 1));
    // draw text
    snprintf(buff, sizeof(buff), "%d", ii + 1);
    graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), txt_bounds,
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for bar graph card
void card_render_bar_graph(Layer *layer, GContext *ctx, uint16_t click_count) {
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render graph line and fill
  prv_render_bars(ctx, bounds);
  // render graph axis with days of the week
  prv_render_axis(ctx, bounds);
  // render text
  prv_render_text(ctx, bounds);
}
