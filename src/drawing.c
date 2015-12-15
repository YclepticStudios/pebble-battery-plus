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
  MenuLayer   *menu;              //< A pointer to the menu which contains the information
  int32_t     ring_level_angle;   //< The angle for the start of the green ring
  int32_t     ring_1day_angle;    //< The angle for the start of the yellow ring
  int32_t     ring_4hr_angle;     //< The angle for the start of the red ring
} drawing_data;

// Different fonts used to draw
static struct {
  GFont       gothic_14;          //< FONT_KEY_GOTHIC_14
  GFont       gothic_14_bold;     //< FONT_KEY_GOTHIC_14_BOLD
  GFont       gothic_18_bold;     //< FONT_KEY_GOTHIC_18_BOLD
  GFont       gothic_24_bold;     //< FONT_KEY_GOTHIC_24_BOLD
  GFont       leco_26_bold;       //< FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM
  GFont       leco_32_bold;       //< FONT_KEY_LECO_32_BOLD_NUMBERS
  GFont       leco_36_bold;       //< FONT_KEY_LECO_36_BOLD_NUMBERS
  GFont       leco_42;            //< FONT_KEY_LECO_42_NUMBERS
} drawing_fonts;

// Cell size type
typedef enum {
  CellSizeSmall,
  CellSizeLarge,
  CellSizeFullScreen
} CellSize;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Cell Rendering Functions
//

// Render cell header
static void prv_render_header_text(GRect bounds, GContext *ctx, char *text) {
  // format text
  GTextAlignment text_alignment = GTextAlignmentLeft;
  GTextAttributes *header_attr = NULL;
  header_attr = graphics_text_attributes_create();
  graphics_text_attributes_enable_screen_text_flow(header_attr, RING_WIDTH + 5);
  // draw text
  graphics_draw_text(ctx, text, drawing_fonts.gothic_14, bounds,
    GTextOverflowModeFill, text_alignment, header_attr);
  graphics_text_attributes_destroy(header_attr);
}

// Render rich text with different fonts
// Arguments are specified as alternating string pointers and fonts (char*, GFont, char*, GFont ...)
static void prv_render_rich_text(GRect bounds, GContext *ctx, uint8_t num_args, va_list a_list_1) {
  // get variable arguments
  va_list a_list_2;
  va_copy(a_list_2, a_list_1);
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
  va_end(a_list_2);
}

// Render a cell with a title and rich formatted content
// Arguments are specified as alternating string pointers and fonts (char*, GFont, char*, GFont ...)
static void prv_render_cell(GRect bounds, GContext *ctx, char *title, uint8_t num_args, ...) {
  // get variable arguments
  va_list a_list;
  va_start(a_list, num_args);
  // draw header
  graphics_context_set_text_color(ctx, GColorBulgarianRose);
  prv_render_header_text(bounds, ctx, title);
  // draw body
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, num_args, a_list);
  // end variable arguments
  va_end(a_list);
}

// Render current clock time
static void prv_cell_render_clock_time(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get time
  time_t t_time = time(NULL);
  tm *tm_time = localtime(&t_time);
  // check draw mode
  if (cell_size == CellSizeSmall || cell_size == CellSizeLarge) {
    // get fonts
    GFont symbol_font = drawing_fonts.gothic_14_bold;
    GFont digit_font = (cell_size == CellSizeSmall) ?
      drawing_fonts.leco_26_bold : drawing_fonts.leco_32_bold;
    // get text
    char digit_buff[6], symbol_buff[3];
    strftime(digit_buff, sizeof(digit_buff), "%l:%M", tm_time);
    strftime(symbol_buff, sizeof(symbol_buff), "%p", tm_time);
    // draw text
    prv_render_cell(bounds, ctx, "Clock", 4, digit_buff, digit_font, symbol_buff, symbol_font);
  } else {
    // draw background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    // calculate hands
    GPoint hr_point = gpoint_from_polar(grect_inset(bounds, GEdgeInsets1(25)),
      GOvalScaleModeFillCircle,
      (tm_time->tm_hour % 12 * MIN_IN_HR + tm_time->tm_min) * TRIG_MAX_ANGLE / (MIN_IN_DAY / 2));
    GPoint min_point = gpoint_from_polar(grect_inset(bounds, GEdgeInsets1(15)),
      GOvalScaleModeFillCircle,
      tm_time->tm_min * TRIG_MAX_ANGLE / MIN_IN_HR);
    // draw hands
    graphics_context_set_stroke_width(ctx, 7);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, grect_center_point(&bounds), min_point);
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_line(ctx, grect_center_point(&bounds), hr_point);
  }
}

// Render run time cell
static void prv_cell_render_run_time(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  GFont symbol_font = drawing_fonts.gothic_18_bold;
  GFont digit_font = (cell_size == CellSizeSmall) ?
    drawing_fonts.leco_26_bold : drawing_fonts.leco_36_bold;
  // render run time
  int32_t sec_run_time = data_get_run_time();
  int days = sec_run_time / SEC_IN_DAY;
  int hrs = sec_run_time % SEC_IN_DAY / SEC_IN_HR;
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  prv_render_cell(bounds, ctx, "Run Time", 8, day_buff, digit_font, "d ", symbol_font, hr_buff,
    digit_font, "h", symbol_font);
  // if in full-screen mode
  if (cell_size == CellSizeFullScreen) {
    // render last charged
    int32_t lst_charge_epoch = data_get_last_charge_time();
    tm *lst_charge = localtime((time_t*)(&lst_charge_epoch));
    char lst_charge_buff[18];
    strftime(lst_charge_buff, sizeof(lst_charge_buff), "%a %e, %l:%M %p", lst_charge);
    GRect tmp_bounds = bounds;
    tmp_bounds.size.h /= 3;
    graphics_draw_text(ctx, lst_charge_buff, drawing_fonts.gothic_24_bold,
      tmp_bounds, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

// Render battery percent cell
static void prv_cell_render_percent(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  GFont symbol_font = drawing_fonts.gothic_18_bold;
  GFont digit_font = (cell_size == CellSizeSmall) ?
    drawing_fonts.leco_26_bold : drawing_fonts.leco_42;
  // get text
  char buff[4];
  snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
  // draw text
  prv_render_cell(bounds, ctx, "Percent", 6, "   ", symbol_font, buff, digit_font, "%",
    symbol_font);
}

// Render time remaining cell
static void prv_cell_render_time_remaining(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  GFont symbol_font = drawing_fonts.gothic_18_bold;
  GFont digit_font = (cell_size == CellSizeSmall) ?
    drawing_fonts.leco_26_bold : drawing_fonts.leco_36_bold;
  // calculate time remaining
  int32_t sec_remaining = data_get_life_remaining();
  int days = sec_remaining / SEC_IN_DAY;
  int hrs = sec_remaining % SEC_IN_DAY / SEC_IN_HR;
  // get text
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  // draw text
  prv_render_cell(bounds, ctx, "Remaining", 8, day_buff, digit_font, "d ", symbol_font, hr_buff,
    digit_font, "h", symbol_font);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render progress ring
static void prv_render_ring(GRect bounds, GContext *ctx) {
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
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, COLOR_RING_LOW);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, 0,
    drawing_data.ring_4hr_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_MED);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_4hr_angle, drawing_data.ring_1day_angle);
#else
  // draw grey background for aplite instead of yellow and red rings
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_1day_angle, drawing_data.ring_level_angle);
  graphics_context_set_fill_color(ctx, COLOR_BACKGROUND);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_level_angle, TRIG_MAX_ANGLE);
  // draw border around center
  graphics_context_set_stroke_color(ctx, COLOR_CENTER_BORDER);
  graphics_context_set_stroke_width(ctx, CENTER_STROKE_WIDTH);
#ifdef PBL_ROUND
  graphics_draw_circle(ctx, grect_center_point(&bounds), (small_side + CENTER_STROKE_WIDTH) / 2 -
   1);
#else
  graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH - CENTER_STROKE_WIDTH / 2)));
  // draw white rectangle to cover missing bottom of menu layer
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, grect_inset(bounds, GEdgeInsets1(RING_WIDTH)), 0, GCornerNone);
#endif
}

// Animation refresh callback
static void prv_animation_refresh_handler(void) {
  layer_mark_dirty(drawing_data.layer);
  menu_layer_reload_data(drawing_data.menu);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Render a MenuLayer cell
void drawing_render_cell(MenuLayer *menu, Layer *layer, GContext *ctx, MenuIndex index) {
  // get cell parameters
  GRect bounds = layer_get_bounds(layer);
  CellSize cell_size;
  if (bounds.size.h > MENU_CELL_HEIGHT_TALL) {
    cell_size = CellSizeFullScreen;
  } else if (menu_cell_layer_is_highlighted(layer)) {
    cell_size = CellSizeLarge;
  } else {
    cell_size = CellSizeSmall;
  }
  // detect cell
  switch (index.row) {
    case 0:
      prv_cell_render_clock_time(bounds, ctx, cell_size);
      break;
    case 1:
      prv_cell_render_run_time(bounds, ctx, cell_size);
      break;
    case 2:
      prv_cell_render_percent(bounds, ctx, cell_size);
      break;
    case 3:
      prv_cell_render_time_remaining(bounds, ctx, cell_size);
      break;
    default:
//      menu_cell_basic_draw(ctx, layer, "<empty>", NULL, NULL);
      break;
  }
}

// Render progress rings and border to the screen
void drawing_render(Layer *layer, GContext *ctx) {
  prv_render_ring(layer_get_bounds(layer), ctx);
}

// Initialize drawing variables
void drawing_initialize(Layer *layer, MenuLayer *menu) {
  // load fonts
  drawing_fonts.gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  drawing_fonts.gothic_14_bold = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  drawing_fonts.gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  drawing_fonts.gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  drawing_fonts.leco_26_bold = fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  drawing_fonts.leco_32_bold = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
  drawing_fonts.leco_36_bold = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  drawing_fonts.leco_42 = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  // register animation callback
  animation_register_update_callback(prv_animation_refresh_handler);
  // store layer pointer
  drawing_data.layer = layer;
  drawing_data.menu = menu;
  // calculate angles for ring
  drawing_data.ring_4hr_angle = drawing_data.ring_1day_angle = drawing_data.ring_level_angle = 0;
  int32_t max_life_sec = data_get_max_life();
  int32_t angle_low = TRIG_MAX_ANGLE * LEVEL_LOW_THRESH_SEC / max_life_sec;
  int32_t angle_med = (int64_t)TRIG_MAX_ANGLE * LEVEL_MED_THRESH_SEC / max_life_sec;
  int32_t angle_level = TRIG_MAX_ANGLE * data_get_battery_percent() / 100;
  angle_low = angle_level < angle_low ? angle_level : angle_low;
  angle_med = angle_level < angle_med ? angle_level : angle_med;
  // animate to new values
  animation_int32_start(&drawing_data.ring_4hr_angle, angle_low, ANI_DURATION,
    STARTUP_ANI_DELAY, CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_1day_angle, angle_med, ANI_DURATION,
    STARTUP_ANI_DELAY, CurveSinEaseOut);
  animation_int32_start(&drawing_data.ring_level_angle, angle_level, ANI_DURATION,
    STARTUP_ANI_DELAY, CurveSinEaseOut);
}
