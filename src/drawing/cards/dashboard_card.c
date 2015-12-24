// @file dashboard_card.c
// @brief Rendering code for dashboard view
//
// File contains all drawing code for rendering the dashboard
// view onto a card.
//
// @author Eric D. Phillips
// @date December 23, 2015
// @bugs No known bugs

#include "card_render.h"
#include "../../data.h"

// Constants
#define COLOR_RING_LOW PBL_IF_COLOR_ELSE(GColorRed, GColorLightGray)
#define COLOR_RING_MED PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray)
#define COLOR_RING_NORM GColorGreen
#define COLOR_RING_EMPTY GColorLightGray
#define CENTER_STROKE_WIDTH 3
#define RING_WIDTH PBL_IF_ROUND_ELSE(18, 16)


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render battery percent text
static void prv_render_battery_percent(GContext *ctx, GRect bounds) {
  // get text
  char buff[4];
  snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
  // get bounds
  GRect txt_bounds = grect_inset(bounds, GEdgeInsets1(RING_WIDTH));
  txt_bounds.size.h /= 2;
  // draw text
  RichTextElement rich_text[] = {
    {"   ", FONT_KEY_GOTHIC_18_BOLD},
    {buff,  FONT_KEY_LECO_42_NUMBERS},
    {"%",   FONT_KEY_GOTHIC_18_BOLD}
  };
  graphics_context_set_text_color(ctx, GColorBlack);
  card_render_rich_text(ctx, txt_bounds, ARRAY_LENGTH(rich_text), rich_text);
}

// Render selected text showing times for colors on ring
static void prv_render_selected_text(GContext *ctx, GRect bounds) {

}

// Render progress ring
static void prv_render_ring(GContext *ctx, GRect bounds) {
  // calculate angles for ring color change positions
  int32_t max_life_sec = data_get_max_life();
  int32_t angle_low = TRIG_MAX_ANGLE * DATA_LEVEL_LOW_THRESH_SEC / max_life_sec;
  int32_t angle_med = (int64_t)TRIG_MAX_ANGLE * DATA_LEVEL_MED_THRESH_SEC / max_life_sec;
  int32_t angle_level = TRIG_MAX_ANGLE * data_get_battery_percent() / 100;
  angle_low = angle_level < angle_low ? angle_level : angle_low;
  angle_med = angle_level < angle_med ? angle_level : angle_med;
  // calculate outer ring bounds
  GRect ring_bounds = bounds;
  int32_t gr_angle = atan2_lookup(ring_bounds.size.h, ring_bounds.size.w);
  int32_t radius = (ring_bounds.size.h / 2) * TRIG_MAX_RATIO / sin_lookup(gr_angle);
  ring_bounds.origin.x += ring_bounds.size.w / 2 - radius;
  ring_bounds.origin.y += ring_bounds.size.h / 2 - radius;
  ring_bounds.size.w = ring_bounds.size.h = radius * 2;
  // calculate inner ring radius
  GRect ring_in_bounds = grect_inset(bounds, GEdgeInsets1(RING_WIDTH));
  int16_t small_side = ring_in_bounds.size.h < ring_in_bounds.size.w ?
    ring_in_bounds.size.h : ring_in_bounds.size.w;
  radius -= small_side / 2;
  // draw rings
  graphics_context_set_fill_color(ctx, COLOR_RING_LOW);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, 0, angle_low);
  graphics_context_set_fill_color(ctx, COLOR_RING_MED);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, angle_low, angle_med);
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, angle_med, angle_level);
  graphics_context_set_fill_color(ctx, COLOR_RING_EMPTY);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, angle_level,
    TRIG_MAX_ANGLE);
  // draw border and center
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, CENTER_STROKE_WIDTH);
#ifdef PBL_ROUND
  graphics_fill_circle(ctx, grect_center_point(&bounds), (small_side + CENTER_STROKE_WIDTH) / 2 -
   1);
  graphics_draw_circle(ctx, grect_center_point(&bounds), (small_side + CENTER_STROKE_WIDTH) / 2 -
   1);
#else
  graphics_fill_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH)), 0, GCornerNone);
  graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH - CENTER_STROKE_WIDTH / 2)));
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for dashboard card
void card_render_dashboard(Layer *layer, GContext *ctx, uint16_t click_count) {
  // turn off anti-aliasing
  graphics_context_set_antialiased(ctx, false);
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render to graphics context
  prv_render_ring(ctx, bounds);
  // render battery percent text
  prv_render_battery_percent(ctx, bounds);
  // render selected text
  prv_render_selected_text(ctx, bounds);
}
