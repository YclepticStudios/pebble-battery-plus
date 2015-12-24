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
#include "../../data.h"
#include "../../utility.h"

// Drawing Constants
#define TEXT_BORDER_TOP PBL_IF_RECT_ELSE(3, 10)
#define GRAPH_STROKE_WIDTH 2
#define GRAPH_TOP_INSET PBL_IF_RECT_ELSE(40, 45)
#define GRAPH_BOTTOM_INSET PBL_IF_RECT_ELSE(30, 35)
#define GRAPH_HORIZONTAL_INSET PBL_IF_RECT_ELSE(0, 18)
#define GRAPH_AXIS_HEIGHT 20
#define GRAPH_X_RANGE SEC_IN_WEEK
#define GRAPH_Y_RANGE 100


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render text
static void prv_render_text(GContext *ctx, GRect bounds) {
  bounds.origin.y += TEXT_BORDER_TOP;
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "Percent", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Render line and fill
static void prv_render_line(GContext *ctx, GRect bounds) {
  // prep draw
  GRect graph_bounds = GRect(GRAPH_HORIZONTAL_INSET, GRAPH_TOP_INSET, bounds.size.w -
    GRAPH_HORIZONTAL_INSET * 2, bounds.size.h - GRAPH_TOP_INSET - GRAPH_BOTTOM_INSET);
  // draw graph
  uint16_t index = 0;
  int32_t node_epoch;
  uint8_t node_percent;
  int32_t cur_epoch = time(NULL);
  GPoint data_points[data_get_battery_data_point_count() + 2];
  while (data_get_battery_data_point(index, &node_epoch, &node_percent)) {
    // calculate screen location
    data_points[index].x = graph_bounds.origin.x + graph_bounds.size.w -
      graph_bounds.size.w * (cur_epoch - node_epoch) / GRAPH_X_RANGE;
    data_points[index].y = graph_bounds.origin.y + graph_bounds.size.h -
      graph_bounds.size.h * node_percent / GRAPH_Y_RANGE;
    index++;
  }
  // add two last points along bottom of data to fill data
  data_points[index] = GPoint(data_points[index - 1].x,
    graph_bounds.origin.y + graph_bounds.size.h);
  data_points[index + 1] = GPoint(data_points[0].x,
    graph_bounds.origin.y + graph_bounds.size.h);
  // draw graph fill
  GPathInfo path_info = { .num_points = ARRAY_LENGTH(data_points), .points = data_points };
  GPath *path = gpath_create(&path_info);
  graphics_context_set_fill_color(ctx, GColorGreen);
  graphics_context_set_antialiased(ctx, false);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
  // draw graph stroke
  path_info.num_points -= 2;
  path = gpath_create(&path_info);
  graphics_context_set_stroke_width(ctx, GRAPH_STROKE_WIDTH);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline_open(ctx, path);
  gpath_destroy(path);
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
void card_render_line_graph(Layer *layer, GContext *ctx, uint16_t click_count) {
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render graph line and fill
  prv_render_line(ctx, bounds);
  // render graph axis with days of the week
  prv_render_axis(ctx, bounds);
  // render text
  prv_render_text(ctx, bounds);
}
