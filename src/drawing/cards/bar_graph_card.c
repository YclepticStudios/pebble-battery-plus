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
#include "../../utility.h"

// Drawing Constants
#define TEXT_BORDER_TOP PBL_IF_RECT_ELSE(3, 10)
#define COLOR_RUN_TIME PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite)
#define COLOR_MAX_LIFE PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorLightGray)
#define GRAPH_STROKE_WIDTH 3
#define GRAPH_TOP_INSET PBL_IF_RECT_ELSE(40, 45)
#define GRAPH_BOTTOM_INSET PBL_IF_RECT_ELSE(50, 60)
#define GRAPH_HORIZONTAL_INSET PBL_IF_RECT_ELSE(0, 18)
#define GRAPH_AXIS_HEIGHT 20
#define GRAPH_NUMBER_OF_BARS 9
#define CLICK_MODE_MAX 3


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render text
static void prv_render_text(GContext *ctx, GRect bounds, uint16_t click_count) {
  // get text
  char *buff;
  if (click_count % CLICK_MODE_MAX == 0) {
    buff = "Charges";
  } else if (click_count % CLICK_MODE_MAX == 1) {
    buff = "Run Time";
  } else {
    buff = "Max Life";
  }
  // draw text
  bounds.origin.y += TEXT_BORDER_TOP;
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Render line and fill
static void prv_render_bars(GContext *ctx, GRect bounds, uint16_t click_count,
                            DataAPI *data_api) {
  // prep draw
  GRect graph_bounds = GRect(GRAPH_HORIZONTAL_INSET, GRAPH_TOP_INSET, bounds.size.w -
    GRAPH_HORIZONTAL_INSET * 2, bounds.size.h - GRAPH_TOP_INSET - GRAPH_BOTTOM_INSET);
  // get properties
  int32_t avg_run_time = 0, avg_max_life = 0;
  int16_t bar_width = graph_bounds.size.w / GRAPH_NUMBER_OF_BARS;
  int32_t graph_y_max = 0;
  uint16_t cycle_point_count = data_api_get_charge_cycle_count(data_api);
  for (uint16_t ii = 0; ii < cycle_point_count; ii++) {
    avg_run_time += data_api_get_run_time(data_api, ii);
    avg_max_life += data_api_get_max_life(data_api, ii);
    if (data_api_get_run_time(data_api, ii) > graph_y_max) {
      graph_y_max = data_api_get_run_time(data_api, ii);
    }
    if (data_api_get_max_life(data_api, ii) > graph_y_max) {
      graph_y_max = data_api_get_max_life(data_api, ii);
    }
  }
  avg_max_life /= cycle_point_count;
  avg_run_time /= cycle_point_count;
  GRect bar_bounds;
  bar_bounds.size.w = bar_width;
  // draw graph
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, GRAPH_STROKE_WIDTH);
  for (uint16_t ii = 0; ii < cycle_point_count; ii++) {
    // draw max life bar
    bar_bounds.origin.x = graph_bounds.origin.x + graph_bounds.size.w - (bar_width * (ii + 1));
    bar_bounds.size.h = graph_bounds.size.h * data_api_get_max_life(data_api, ii) /
      graph_y_max;
    bar_bounds.origin.y = graph_bounds.origin.y + graph_bounds.size.h - bar_bounds.size.h;
    graphics_context_set_fill_color(ctx, COLOR_MAX_LIFE);
    graphics_fill_rect(ctx, bar_bounds, 0, GCornerNone);
    graphics_draw_rect(ctx, bar_bounds);
    // draw max life bar
    bar_bounds.size.h = graph_bounds.size.h * data_api_get_run_time(data_api, ii) /
      graph_y_max;
    bar_bounds.origin.y = graph_bounds.origin.y + graph_bounds.size.h - bar_bounds.size.h;
    graphics_context_set_fill_color(ctx, COLOR_RUN_TIME);
    graphics_fill_rect(ctx, bar_bounds, 0, GCornerNone);
    graphics_draw_rect(ctx, bar_bounds);
  }
  // render average line
  GColor avg_color;
  int16_t avg_y_value;
  int32_t avg_value;
  if (click_count % CLICK_MODE_MAX == 0) {
    return;
  } else if (click_count % CLICK_MODE_MAX == 1) {
    avg_color = GColorDarkGreen;
    avg_value = avg_run_time;
  } else {
    avg_color = GColorBlue;
    avg_value = avg_max_life;
  }
  avg_y_value = graph_bounds.origin.y + graph_bounds.size.h -
    graph_bounds.size.h * avg_value / graph_y_max;
  graphics_context_set_stroke_color(ctx, avg_color);
  graphics_draw_line(ctx, GPoint(0, avg_y_value), GPoint(bounds.size.w, avg_y_value));
  // render average text
  int days = avg_value / SEC_IN_DAY;
  int hrs = avg_value % SEC_IN_DAY / SEC_IN_HR;
  char buff[16];
  snprintf(buff, sizeof(buff), "Avg: %dd %dh", days, hrs);
  GRect txt_bounds = GRect(0, graph_bounds.origin.y + graph_bounds.size.h + 2 + GRAPH_AXIS_HEIGHT,
    bounds.size.w, 25);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), txt_bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Render axis
static void prv_render_axis(GContext *ctx, GRect bounds) {
  // reshape bounds
  GRect axis_bounds = bounds;
  axis_bounds.origin.y = axis_bounds.size.h - GRAPH_BOTTOM_INSET;
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
  int16_t bar_width = graph_bounds.size.w / GRAPH_NUMBER_OF_BARS;
  char buff[3];
  GRect txt_bounds = axis_bounds;
  txt_bounds.origin.y -= 2;
  txt_bounds.size.w = bar_width;
  // draw graph
  for (uint16_t ii = 0; ii < GRAPH_NUMBER_OF_BARS; ii++) {
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
void card_render_bar_graph(Layer *layer, GContext *ctx, uint16_t click_count,
                           DataAPI *data_api) {
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render graph line and fill
  prv_render_bars(ctx, bounds, click_count, data_api);
  // render graph axis with days of the week
  prv_render_axis(ctx, bounds);
  // render text
  prv_render_text(ctx, bounds, click_count);
}
