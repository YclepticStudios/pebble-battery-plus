// @file drawing.c
// @brief Main drawing code
//
// Contains all the drawing code for this app.
//
// @author Eric D. Phillips
// @date November 28, 2015
// @bugs No known bugs

#include "drawing.h"
#include "animation/animation.h"
#include "data.h"
#include "utility.h"

// Drawing constants
#define COLOR_BACKGROUND PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define COLOR_DISK_FILL PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)
#define COLOR_DISK_STROKE GColorBlack
#define COLOR_FOREGROUND GColorBlack
#define COLOR_RING_4HR PBL_IF_COLOR_ELSE(GColorRed, GColorWhite)
#define COLOR_RING_1DAY PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite)
#define COLOR_RING_NORM PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite)
#define RING_WIDTH PBL_IF_ROUND_ELSE(21, 19)
#define CENTER_STROKE_WIDTH 4
#define CENTER_CORNER_RAD_1 PBL_IF_ROUND_ELSE(90, 5)
#define CENTER_CORNER_RAD_2 PBL_IF_ROUND_ELSE(90, 3)
#define ANI_DURATION 300
#define STARTUP_ANI_DELAY 550

// Drawing variables
static struct {
  Layer       *layer;             //< A pointer to the layer on which everything is drawn
  GRect       center_bounds;      //< The radius of the central disk
  int32_t     ring_level_angle;   //< The angle for the start of the green ring
  int32_t     ring_1day_angle;    //< The angle for the start of the yellow ring
  int32_t     ring_4hr_angle;     //< The angle for the start of the red ring
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render progress ring and grey central disk
static void prv_render_ring(GRect bounds, GContext *ctx) {
  // calculate ring bounds size
  int32_t gr_angle = atan2_lookup(bounds.size.h, bounds.size.w);
  int32_t radius = (bounds.size.h / 2) * TRIG_MAX_RATIO / sin_lookup(gr_angle);
  bounds.origin.x += bounds.size.w / 2 - radius;
  bounds.origin.y += bounds.size.h / 2 - radius;
  bounds.size.w = bounds.size.h = radius * 2;
  // draw rings
  graphics_context_set_fill_color(ctx, COLOR_RING_4HR);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFillCircle, radius, 0,
    drawing_data.ring_4hr_angle + 10);
  graphics_context_set_fill_color(ctx, COLOR_RING_1DAY);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_4hr_angle, drawing_data.ring_1day_angle + 10);
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_1day_angle, drawing_data.ring_level_angle);
  // draw center
  graphics_context_set_fill_color(ctx, COLOR_DISK_STROKE);
  graphics_fill_rect(ctx, drawing_data.center_bounds, CENTER_CORNER_RAD_1, GCornersAll);
  graphics_context_set_fill_color(ctx, COLOR_DISK_FILL);
  graphics_fill_rect(ctx, grect_inset(drawing_data.center_bounds,
    GEdgeInsets1(CENTER_STROKE_WIDTH)), CENTER_CORNER_RAD_2, GCornersAll);
}

// Animation refresh callback
static void prv_animation_refresh_handler(void) {
  layer_mark_dirty(drawing_data.layer);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Move layout to dashboard layout
void drawing_convert_to_dashboard_layout(uint32_t delay) {
  // calculate angles for ring
  int32_t max_life_sec = data_get_max_life();
  int32_t angle_4hr = TRIG_MAX_ANGLE * 4 * SEC_IN_HR / max_life_sec;
  int32_t angle_1day = (int64_t)TRIG_MAX_ANGLE * SEC_IN_DAY / max_life_sec;
  int32_t angle_level = TRIG_MAX_ANGLE * data_get_battery_percent() / 100;
  angle_4hr = angle_level < angle_4hr ? angle_level : angle_4hr;
  angle_1day = angle_level < angle_1day ? angle_level : angle_1day;
  // animate to new values
  animation_grect_start(&drawing_data.center_bounds, grect_inset(layer_get_bounds(
    drawing_data.layer), GEdgeInsets1(RING_WIDTH)), ANI_DURATION, delay, CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_4hr_angle, angle_4hr, ANI_DURATION, delay,
    CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_1day_angle, angle_1day, ANI_DURATION, delay,
    CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_level_angle, angle_level, ANI_DURATION, delay,
    CurveSinEaseOut);
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
  // draw background
  graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  // draw rings
  prv_render_ring(bounds, ctx);
}

//! Initialize drawing variables
void drawing_initialize(Layer *layer) {
  // register animation callback
  animation_register_update_callback(prv_animation_refresh_handler);
  // store layer pointer
  drawing_data.layer = layer;
  // set starting values
  GRect bounds = layer_get_bounds(layer);
  drawing_data.center_bounds = GRect(bounds.size.w / 2 - CENTER_STROKE_WIDTH,
    bounds.size.h / 2 - CENTER_STROKE_WIDTH, CENTER_STROKE_WIDTH * 2, CENTER_STROKE_WIDTH * 2);
  drawing_data.ring_4hr_angle = drawing_data.ring_1day_angle = drawing_data.ring_level_angle = 0;
  drawing_convert_to_dashboard_layout(STARTUP_ANI_DELAY);
}
