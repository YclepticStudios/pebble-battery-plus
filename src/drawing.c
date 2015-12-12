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
#include "stdarg.h"
#include "utility.h"

// Data constants
#define LEVEL_LOW_THRESH_SEC 4 * SEC_IN_HR
#define LEVEL_MED_THRESH_SEC SEC_IN_DAY
// Drawing constants
#define COLOR_BACKGROUND PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define COLOR_CENTER_BORDER GColorBlack
#define COLOR_FOREGROUND GColorBlack
#define COLOR_RING_LOW GColorRed
#define COLOR_RING_MED GColorYellow
#define COLOR_RING_NORM GColorGreen
#define CENTER_STROKE_WIDTH PBL_IF_ROUND_ELSE(5, 4)
#define TEXT_TOP_BORDER_FRACTION 3 / 25
#define ANI_DURATION 300
#define STARTUP_ANI_DELAY 550


// Drawing variables
static struct {
  Layer       *layer;             //< A pointer to the layer on which everything is drawn
  GRect       center_bounds;      //< The bounds of the central disk
  int32_t     ring_level_angle;   //< The angle for the start of the green ring
  int32_t     ring_1day_angle;    //< The angle for the start of the yellow ring
  int32_t     ring_4hr_angle;     //< The angle for the start of the red ring
} drawing_data;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Cell Rendering Functions
//

// Render large cell header
static void prv_render_header_text(GRect bounds, GContext *ctx, bool large, char *text) {
  // return if in small view
  if (!large) { return; }
  // get header bounds
  GRect header_bounds = bounds;
  header_bounds.origin.y = -2;
  // draw header
  GTextAttributes *header_attr = graphics_text_attributes_create();
  graphics_text_attributes_enable_screen_text_flow(header_attr, RING_WIDTH + 5);
  graphics_context_set_text_color(ctx, GColorBulgarianRose);
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), header_bounds,
    GTextOverflowModeFill, GTextAlignmentLeft, header_attr);
  graphics_text_attributes_destroy(header_attr);
}

// Render rich text with different fonts
// Arguments are specified as alternating string pointers and fonts (char*, GFont, char*, GFont ...)
static void prv_render_rich_text(GRect bounds, GContext *ctx, uint8_t num_args, ...) {
  // get variable arguments
  va_list a_list_1, a_list_2;
  va_start(a_list_1, num_args);
  va_start(a_list_2, num_args);
  // calculate the rendered size of each string
  int16_t max_height = 0, tot_width = 0;
  GRect txt_bounds[num_args / 2];
  for (uint8_t ii = 0; ii < num_args / 2; ii++) {
    txt_bounds[ii].size = graphics_text_layout_get_content_size(va_arg(a_list_1, char*),
      va_arg(a_list_1, GFont), bounds, GTextOverflowModeFill, GTextAlignmentLeft);
    max_height = max_height > txt_bounds[ii].size.h ? max_height : txt_bounds[ii].size.h;
    tot_width += txt_bounds[ii].size.w;
  }
  // calculate positioning of each string
  int16_t base_line_y = (bounds.size.h + max_height) / 2 - max_height * TEXT_TOP_BORDER_FRACTION;
  int16_t left_line_x = (bounds.size.w - tot_width) / 2;
  for (uint8_t ii = 0; ii < num_args / 2; ii++) {
    txt_bounds[ii].origin.x = left_line_x;
    txt_bounds[ii].origin.y = base_line_y - txt_bounds[ii].size.h;
    left_line_x += txt_bounds[ii].size.w;
  }
  // draw each string
  for (uint8_t ii = 0; ii < num_args / 2; ii++) {
    graphics_draw_text(ctx, va_arg(a_list_2, char*), va_arg(a_list_2, GFont), txt_bounds[ii],
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }
  // end variable arguments
  va_end(a_list_1);
  va_end(a_list_2);
}

// Render current clock time
static void prv_cell_render_clock_time(GRect bounds, GContext *ctx, bool large) {
  // draw header
  prv_render_header_text(bounds, ctx, large, "Clock");
  // get fonts
  GFont digit_font, symbol_font;
  symbol_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  if (large) {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
  } else {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  }
  // get text
  char digit_buff[6], symbol_buff[3];
  time_t t_time = time(NULL);
  tm *tm_time = localtime(&t_time);
  strftime(digit_buff, sizeof(digit_buff), "%l:%M", tm_time);
  strftime(symbol_buff, sizeof(symbol_buff), "%p", tm_time);
  // draw text
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, 4, digit_buff, digit_font, symbol_buff, symbol_font);
}

// Render battery percent cell
static void prv_cell_render_percent(GRect bounds, GContext *ctx, bool large) {
  // draw header
  prv_render_header_text(bounds, ctx, large, "Percent");
  // get fonts
  GFont digit_font, symbol_font;
  symbol_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  if (large) {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  } else {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  }
  // get text
  char buff[4];
  snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
  // draw text
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, 6, "   ", symbol_font, buff, digit_font, "%", symbol_font);
}

// Render time remaining cell
static void prv_cell_render_time_remaining(GRect bounds, GContext *ctx, bool large) {
  // draw header
  prv_render_header_text(bounds, ctx, large, "Remaining");
  // get fonts
  GFont digit_font, symbol_font;
  symbol_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  if (large) {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  } else {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  }
  // calculate time remaining
  int32_t sec_remaining = data_get_life_remaining();
  int days = sec_remaining / SEC_IN_DAY;
  int hrs = sec_remaining % SEC_IN_DAY / SEC_IN_HR;
  // get text
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  // draw text
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, 8, day_buff, digit_font, "d ", symbol_font, hr_buff,
    digit_font, "h", symbol_font);
}

// Render run time cell
static void prv_cell_render_run_time(GRect bounds, GContext *ctx, bool large) {
  // draw header
  prv_render_header_text(bounds, ctx, large, "Run Time");
  // get fonts
  GFont digit_font, symbol_font;
  symbol_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  if (large) {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  } else {
    digit_font = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  }
  // calculate time remaining
  int32_t sec_run_time = data_get_run_time();
  int days = sec_run_time / SEC_IN_DAY;
  int hrs = sec_run_time % SEC_IN_DAY / SEC_IN_HR;
  // get text
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  // draw text
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, 8, day_buff, digit_font, "d ", symbol_font, hr_buff,
    digit_font, "h", symbol_font);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

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
  // draw grey background for aplite
#ifdef PBL_BW
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
  // draw rings
  int16_t small_side = drawing_data.center_bounds.size.h < drawing_data.center_bounds.size.w ?
    drawing_data.center_bounds.size.h : drawing_data.center_bounds.size.w;
  radius = radius - small_side / 2;
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, COLOR_RING_LOW);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, 0,
    drawing_data.ring_4hr_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_MED);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_4hr_angle, drawing_data.ring_1day_angle);
#endif
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_1day_angle, drawing_data.ring_level_angle);
  graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_level_angle, TRIG_MAX_ANGLE);
#ifdef PBL_ROUND
  // draw center with clear hole to show tile layer
  graphics_context_set_fill_color(ctx, COLOR_CENTER_BORDER);
  graphics_fill_radial(ctx, grect_inset(drawing_data.center_bounds,
    GEdgeInsets1(-CENTER_STROKE_WIDTH)), GOvalScaleModeFitCircle, CENTER_STROKE_WIDTH, 0,
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
  graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH - CENTER_STROKE_WIDTH / 2)));
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

// Render a MenuLayer cell
void drawing_render_cell(MenuLayer *menu, Layer *layer, GContext *ctx, MenuIndex index) {
  // get cell bounds
  GRect bounds = layer_get_bounds(layer);
  bool selected = menu_layer_get_selected_index(menu).row == index.row;
  // detect cell
  switch (index.row) {
    case 0:
      prv_cell_render_clock_time(bounds, ctx, selected);
      break;
    case 1:
      prv_cell_render_run_time(bounds, ctx, selected);
      break;
    case 2:
      prv_cell_render_percent(bounds, ctx, selected);
      break;
    case 3:
      prv_cell_render_time_remaining(bounds, ctx, selected);
      break;
    default:
      menu_cell_basic_draw(ctx, layer, "<empty>", "unused cell", NULL);
      break;
  }
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
}
