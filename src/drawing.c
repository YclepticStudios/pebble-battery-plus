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

// Data constants
#define LEVEL_LOW_THRESH_SEC 4 * SEC_IN_HR
#define LEVEL_MED_THRESH_SEC SEC_IN_DAY
// Drawing constants
#define COLOR_BACKGROUND PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define COLOR_CENTER_BORDER GColorBlack
#define COLOR_FOREGROUND GColorBlack
#define COLOR_RING_LOW PBL_IF_COLOR_ELSE(GColorRed, GColorWhite)
#define COLOR_RING_MED PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite)
#define COLOR_RING_NORM PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite)
#define CENTER_STROKE_WIDTH PBL_IF_ROUND_ELSE(6, 4)
#define CENTER_CORNER_RAD_1 PBL_IF_ROUND_ELSE(0, 5)
#define CENTER_CORNER_RAD_2 PBL_IF_ROUND_ELSE(0, 3)
#define TILE_PERCENT_TXT_OFFSET PBL_IF_ROUND_ELSE(6, 2)
#define ANI_DURATION 300
#define STARTUP_ANI_DELAY 550

// Drawing tiles
typedef void (*TileRender)(GRect bounds, GContext*);
typedef struct {
  GRect bounds;
  TileRender render;
} DrawingTile;

// Drawing variables
static struct {
  Layer       *layer;             //< A pointer to the layer on which everything is drawn
  GRect       center_bounds;      //< The bounds of the central disk
  int32_t     ring_level_angle;   //< The angle for the start of the green ring
  int32_t     ring_1day_angle;    //< The angle for the start of the yellow ring
  int32_t     ring_4hr_angle;     //< The angle for the start of the red ring
  DrawingTile drawing_tiles[4];   //< Positions and rendering functions for visual tiles
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render battery percent tile
static void prv_tile_render_battery_percent(GRect bounds, GContext *ctx) {
  // draw background
  int32_t life_remaining_sec = data_get_life_remaining();
  GColor back_color = COLOR_RING_NORM;
  if (life_remaining_sec < LEVEL_LOW_THRESH_SEC) {
    back_color = COLOR_RING_LOW;
  } else if (life_remaining_sec < LEVEL_MED_THRESH_SEC) {
    back_color = COLOR_RING_MED;
  }
  graphics_context_set_fill_color(ctx, back_color);
  graphics_fill_rect(ctx, bounds, CENTER_CORNER_RAD_2, GCornersTop);
  // draw text
  char buff[4];
  snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
  bounds.origin.y += TILE_PERCENT_TXT_OFFSET;
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49),
    bounds, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// Render battery percent
static void prv_render_battery_percent(GRect bounds, GContext *ctx) {
  // draw background
  GRect back_bounds = drawing_data.center_bounds;
  back_bounds.size.h /= 2;
  int32_t life_remaining_sec = data_get_life_remaining();
  GColor back_color = COLOR_RING_NORM;
  if (life_remaining_sec < LEVEL_LOW_THRESH_SEC) {
    back_color = COLOR_RING_LOW;
  } else if (life_remaining_sec < LEVEL_MED_THRESH_SEC) {
    back_color = COLOR_RING_MED;
  }
  graphics_context_set_fill_color(ctx, COLOR_CENTER_BORDER);
  graphics_fill_rect(ctx, grect_inset(drawing_data.center_bounds,
    GEdgeInsets1(-CENTER_STROKE_WIDTH / 2)), CENTER_CORNER_RAD_1, GCornersAll);
  graphics_context_set_fill_color(ctx, back_color);
  graphics_fill_rect(ctx, grect_inset(back_bounds, GEdgeInsets1(CENTER_STROKE_WIDTH / 2)),
    CENTER_CORNER_RAD_2, GCornersTop);
  // if done animating
  if (!animation_check_scheduled(&drawing_data.center_bounds)) {
    // draw text
    char buff[4];
    snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
    back_bounds.origin.y += TILE_PERCENT_TXT_OFFSET;
    graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
    graphics_draw_text(ctx, buff, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49),
      back_bounds, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

// Render progress ring
static void prv_render_ring(GRect bounds, GContext *ctx) {
  // duplicate frame buffer
#ifndef PBL_ROUND
  GBitmap *bmp = graphics_capture_frame_buffer(ctx);
  uint8_t *data_old = MALLOC(gbitmap_get_bytes_per_row(bmp) * gbitmap_get_bounds(bmp).size.h);
  memcpy(data_old, gbitmap_get_data(bmp), gbitmap_get_bytes_per_row(bmp) *
    gbitmap_get_bounds(bmp).size.h);
  graphics_release_frame_buffer(ctx, bmp);
#endif
  // calculate ring bounds size
  GRect ring_bounds = bounds;
  int32_t gr_angle = atan2_lookup(ring_bounds.size.h, ring_bounds.size.w);
  int32_t radius = (ring_bounds.size.h / 2) * TRIG_MAX_RATIO / sin_lookup(gr_angle);
  ring_bounds.origin.x += ring_bounds.size.w / 2 - radius;
  ring_bounds.origin.y += ring_bounds.size.h / 2 - radius;
  ring_bounds.size.w = ring_bounds.size.h = radius * 2;
  // draw rings
  int16_t small_side = drawing_data.center_bounds.size.h < drawing_data.center_bounds.size.w ?
    drawing_data.center_bounds.size.h : drawing_data.center_bounds.size.w;
  radius = radius - small_side / 2;
  graphics_context_set_fill_color(ctx, COLOR_RING_LOW);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, 0,
    drawing_data.ring_4hr_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_MED);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_4hr_angle, drawing_data.ring_1day_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_1day_angle, drawing_data.ring_level_angle);
  graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_level_angle, TRIG_MAX_ANGLE);
  // draw center with clear hole to show tile layer
#ifdef PBL_ROUND
  graphics_context_set_fill_color(ctx, COLOR_CENTER_BORDER);
  graphics_fill_radial(ctx, grect_inset(drawing_data.center_bounds,
    GEdgeInsets1(-CENTER_STROKE_WIDTH / 2)), GOvalScaleModeFitCircle, CENTER_STROKE_WIDTH, 0,
    TRIG_MAX_ANGLE);
#else
  // copy center of frame buffer back
  bmp = graphics_capture_frame_buffer(ctx);
  uint8_t *data = gbitmap_get_data(bmp);
  for (uint16_t y = RING_WIDTH; y < gbitmap_get_bounds(bmp).size.h - RING_WIDTH; y++) {
    memcpy(&data[y * gbitmap_get_bytes_per_row(bmp) + RING_WIDTH / PBL_IF_BW_ELSE(8, 1)],
      &data_old[y * gbitmap_get_bytes_per_row(bmp) + RING_WIDTH / PBL_IF_BW_ELSE(8, 1)],
      (gbitmap_get_bounds(bmp).size.w - RING_WIDTH * 2) / PBL_IF_BW_ELSE(8, 1));
  }
  // clean up
  graphics_release_frame_buffer(ctx, bmp);
  free(data_old);
  // draw frame
  graphics_context_set_stroke_color(ctx, COLOR_CENTER_BORDER);
  graphics_context_set_stroke_width(ctx, CENTER_STROKE_WIDTH);
  graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH)));
#endif
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
  int32_t angle_low = TRIG_MAX_ANGLE * LEVEL_LOW_THRESH_SEC / max_life_sec;
  int32_t angle_med = (int64_t)TRIG_MAX_ANGLE * LEVEL_MED_THRESH_SEC / max_life_sec;
  int32_t angle_level = TRIG_MAX_ANGLE * data_get_battery_percent() / 100;
  angle_low = angle_level < angle_low ? angle_level : angle_low;
  angle_med = angle_level < angle_med ? angle_level : angle_med;
  // animate to new values
  animation_grect_start(&drawing_data.center_bounds, grect_inset(layer_get_bounds(
    drawing_data.layer), GEdgeInsets1(RING_WIDTH)), ANI_DURATION, delay, CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_4hr_angle, angle_low, ANI_DURATION, delay,
    CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_1day_angle, angle_med, ANI_DURATION, delay,
    CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_level_angle, angle_level, ANI_DURATION, delay,
    CurveSinEaseOut);
}

// Render everything to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  // get properties
  GRect bounds = layer_get_bounds(layer);
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
  // set starting tile layouts
  drawing_data.drawing_tiles[0] = (DrawingTile) {
    .bounds = GRect(5, 5, 100, 60),
    .render = prv_tile_render_battery_percent,
  };
  // TODO: Initialize tiles
}
