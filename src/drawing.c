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
#define ANIMATION_ANGLE_CHANGE_THRESHOLD 348
#define COLOR_BACKGROUND PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define COLOR_CENTER_BORDER GColorBlack
#define COLOR_FOREGROUND GColorBlack
#define COLOR_RING_LOW PBL_IF_COLOR_ELSE(GColorRed, GColorLightGray)
#define COLOR_RING_MED PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray)
#define COLOR_RING_NORM GColorGreen
#define CENTER_STROKE_WIDTH PBL_IF_ROUND_ELSE(5, 4)
#define TEXT_TOP_BORDER_FRACTION 3 / 25
#define ANI_DURATION 300
#define STARTUP_ANI_DELAY 550
// Clock cell constants
#define CELL_CLOCK_TICK_WIDTH 3
#define CELL_CLOCK_TICK_LENGTH 8
#define CELL_CLOCK_HR_HAND_INSET 37
#define CELL_CLOCK_MIN_HAND_INSET 23
#define CELL_CLOCK_HAND_WIDTH 7


// Drawing variables
static struct {
  Layer       *layer;             //< A pointer to the layer on which everything is drawn
  MenuLayer   *menu;              //< A pointer to the menu which contains the information
  int32_t     ring_level_angle;   //< The angle for the start of the green ring
  int32_t     ring_med_angle;     //< The angle for the start of the yellow ring
  int32_t     ring_low_angle;     //< The angle for the start of the red ring
} drawing_data;

// Rich text element
typedef struct {
  char        *text;              //< Pointer to text to draw
  char        *font;              //< Pointer to font face string to draw with
} RichTextElement;

// Cell size type
typedef enum {
  CellSizeSmall,
  CellSizeLarge,
  CellSizeFullScreen
} CellSize;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Cell Helper Functions
//

// Render cell header
static void prv_render_header_text(GRect bounds, GContext *ctx, char *text) {
  // format text
  GTextAttributes *header_attr = graphics_text_attributes_create();
  graphics_text_attributes_enable_screen_text_flow(header_attr, RING_WIDTH + 5);
  // draw text
  GFont font_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  graphics_draw_text(ctx, text, font_gothic_14, bounds, GTextOverflowModeFill,
    GTextAlignmentLeft, PBL_IF_BW_ELSE(NULL, header_attr)); // TODO: FIX SCREEN FLOW
  graphics_text_attributes_destroy(header_attr);
}

// Render rich text with different fonts
static void prv_render_rich_text(GRect bounds, GContext *ctx, uint8_t array_length,
                                 RichTextElement *rich_text) {
  // calculate the rendered size of each string
  int16_t max_height = 0, tot_width = 0;
  GRect txt_bounds[array_length];
  for (uint8_t ii = 0; ii < array_length; ii++) {
    GFont tmp_font = fonts_get_system_font(rich_text[ii].font);
    txt_bounds[ii].size = graphics_text_layout_get_content_size(rich_text[ii].text, tmp_font,
      bounds, GTextOverflowModeFill, GTextAlignmentLeft);
    max_height = (max_height > txt_bounds[ii].size.h) ? max_height : txt_bounds[ii].size.h;
    tot_width += txt_bounds[ii].size.w;
  }
  // calculate positioning of each string
  int16_t base_line_y = bounds.origin.y + (bounds.size.h + max_height) / 2 -
    max_height * TEXT_TOP_BORDER_FRACTION;
  int16_t left_line_x = bounds.origin.x + (bounds.size.w - tot_width) / 2;
  for (uint8_t ii = 0; ii < array_length; ii++) {
    txt_bounds[ii].origin.x = left_line_x;
    txt_bounds[ii].origin.y = base_line_y - txt_bounds[ii].size.h;
    left_line_x += txt_bounds[ii].size.w;
  }
  // draw each string
  for (uint8_t ii = 0; ii < array_length; ii++) {
    GFont tmp_font = fonts_get_system_font(rich_text[ii].font);
    graphics_draw_text(ctx, rich_text[ii].text, tmp_font, txt_bounds[ii], GTextOverflowModeFill,
      GTextAlignmentLeft, NULL);
  }
}

// Render a cell with a title and rich formatted content
// Arguments are specified as two arrays of char* and GFont
static void prv_render_cell(GRect bounds, GContext *ctx, CellSize cell_size, char *title,
                            uint8_t array_length, RichTextElement *rich_text) {
  // draw header
  if (title && cell_size != CellSizeSmall) {
    graphics_context_set_text_color(ctx, GColorBulgarianRose);
    prv_render_header_text(bounds, ctx, title);
  }
  // draw body
  graphics_context_set_text_color(ctx, COLOR_FOREGROUND);
  prv_render_rich_text(bounds, ctx, array_length, rich_text);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Cell Rendering Functions
//

// Render current clock time
static void prv_cell_render_clock_time(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get time
  time_t t_time = time(NULL);
  tm *tm_time = localtime(&t_time);
  // check draw mode
  if (cell_size != CellSizeFullScreen) {
    // get fonts
    char *symbol_font = FONT_KEY_GOTHIC_14_BOLD;
    char *digit_font = (cell_size == CellSizeSmall) ?
      FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_32_BOLD_NUMBERS;
    // get text
    char digit_buff[6], symbol_buff[3], date_buff[16];
    strftime(digit_buff, sizeof(digit_buff), "%l:%M", tm_time);
    strftime(symbol_buff, sizeof(symbol_buff), "%p", tm_time);
    strftime(date_buff, sizeof(date_buff), "%a, %b %e", tm_time);
    // get bounds
    if (cell_size == CellSizeLarge) {
      bounds.size.h -= 8;
    }
    // draw text
    RichTextElement rich_text_1[] = {
      {digit_buff,  digit_font},
      {symbol_buff, symbol_font}
    };
    prv_render_cell(bounds, ctx, cell_size, "Clock", ARRAY_LENGTH(rich_text_1), rich_text_1);
    if (cell_size == CellSizeLarge) {
      bounds.origin.y += 21;
      RichTextElement rich_text_2[] = {
        {date_buff, FONT_KEY_GOTHIC_18_BOLD},
      };
      prv_render_cell(bounds, ctx, cell_size, NULL, ARRAY_LENGTH(rich_text_2), rich_text_2);
    }
  } else {
    // draw background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    // only draw if not animating
    if (!animation_any_scheduled()) {
      // draw tick marks
      GRect tick_bounds = grect_inset(bounds, GEdgeInsets1(-15));
      graphics_context_set_stroke_width(ctx, CELL_CLOCK_TICK_WIDTH);
      graphics_context_set_stroke_color(ctx, GColorWhite);
      for (int32_t angle = 0; angle < TRIG_MAX_ANGLE; angle += TRIG_MAX_ANGLE / 12) {
        graphics_draw_line(ctx, gpoint_from_polar(tick_bounds, GOvalScaleModeFillCircle, angle),
          grect_center_point(&tick_bounds));
      }
#ifdef PBL_ROUND
      graphics_fill_circle(ctx, grect_center_point(&bounds), bounds.size.w / 2 -
        CELL_CLOCK_TICK_LENGTH);
#else
      graphics_fill_rect(ctx, grect_inset(bounds, GEdgeInsets1(CELL_CLOCK_TICK_LENGTH)), 0,
        GCornerNone);
#endif
      // draw date
      GRect date_bounds = bounds;
      date_bounds.origin.y += date_bounds.size.h / 2 - 12;
      date_bounds.origin.x += date_bounds.size.w * 2 / 3;
      date_bounds.size.w /= 4;
      char date_buff[3];
      strftime(date_buff, sizeof(date_buff), "%e", tm_time);
      graphics_context_set_text_color(ctx, GColorChromeYellow);
      GFont font_gothic_18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
      graphics_draw_text(ctx, date_buff, font_gothic_18, date_bounds,
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
    // calculate hands
    GPoint hr_point = gpoint_from_polar(grect_inset(bounds, GEdgeInsets1(CELL_CLOCK_HR_HAND_INSET)),
      GOvalScaleModeFillCircle,
      (tm_time->tm_hour % 12 * MIN_IN_HR + tm_time->tm_min) * TRIG_MAX_ANGLE / (MIN_IN_DAY / 2));
    GPoint min_point = gpoint_from_polar(grect_inset(bounds,
        GEdgeInsets1(CELL_CLOCK_MIN_HAND_INSET)), GOvalScaleModeFillCircle,
      tm_time->tm_min * TRIG_MAX_ANGLE / MIN_IN_HR);
    // draw hands
    graphics_context_set_stroke_width(ctx, CELL_CLOCK_HAND_WIDTH);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, grect_center_point(&bounds), min_point);
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_line(ctx, grect_center_point(&bounds), hr_point);
  }
}

// Render run time cell
static void prv_cell_render_run_time(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  char *symbol_font = FONT_KEY_GOTHIC_18_BOLD;
  char *digit_font = (cell_size == CellSizeSmall) ?
    FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_36_BOLD_NUMBERS;
  // render run time
  GRect main_bounds = grect_inset(bounds,
    GEdgeInsets2((bounds.size.h - MENU_CELL_HEIGHT_TALL) / 2, 0));
  int32_t sec_run_time = data_get_run_time();
  int days = sec_run_time / SEC_IN_DAY;
  int hrs = sec_run_time % SEC_IN_DAY / SEC_IN_HR;
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  RichTextElement rich_text_1[] = {
    {day_buff, digit_font},
    {"d ",     symbol_font},
    {hr_buff,  digit_font},
    {"h",      symbol_font}
  };
  prv_render_cell(main_bounds, ctx, cell_size, "Run Time", ARRAY_LENGTH(rich_text_1), rich_text_1);
  // if in full-screen mode
  if (cell_size == CellSizeFullScreen) {
    // temp variables
    GRect tmp_bounds;
    int day, hour;
    char tmp_buff[16];
    // render record life
    tmp_bounds = GRect(0, MENU_CELL_FULL_SCREEN_TOP_OFFSET,
      bounds.size.w, MENU_CELL_FULL_SCREEN_SUB_HEIGHT);
    int32_t sec_record_life = data_get_record_run_time();
    day = sec_record_life / SEC_IN_DAY;
    hour = sec_record_life % SEC_IN_DAY / SEC_IN_HR;
    snprintf(tmp_buff, sizeof(tmp_buff), "%dd %02dh", day, hour);
    RichTextElement rich_text_2[] = {
      {tmp_buff, FONT_KEY_GOTHIC_24_BOLD}
    };
    prv_render_cell(tmp_bounds, ctx, cell_size, "Record", ARRAY_LENGTH(rich_text_2), rich_text_2);
    // render last charged
    tmp_bounds = GRect(0, bounds.size.h - MENU_CELL_FULL_SCREEN_SUB_HEIGHT,
      bounds.size.w, MENU_CELL_FULL_SCREEN_SUB_HEIGHT);
    time_t lst_charge_epoch = data_get_last_charge_time();
    tm *lst_charge = localtime(&lst_charge_epoch);
    strftime(tmp_buff, sizeof(tmp_buff), "%A", lst_charge);
    prv_render_cell(tmp_bounds, ctx, cell_size, "Last Charged", ARRAY_LENGTH(rich_text_2),
      rich_text_2);
  }
}

// Render battery percent cell
static void prv_cell_render_percent(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  char *symbol_font = FONT_KEY_GOTHIC_18_BOLD;
  char *digit_font = (cell_size == CellSizeSmall) ?
    FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_42_NUMBERS;
  // get text
  char buff[4];
  snprintf(buff, sizeof(buff), "%d", data_get_battery_percent());
  // draw text
  GRect main_bounds = grect_inset(bounds,
    GEdgeInsets2((bounds.size.h - MENU_CELL_HEIGHT_TALL) / 2, 0));
  RichTextElement rich_text_1[] = {
    {"   ", symbol_font},
    {buff,  digit_font},
    {"%",   symbol_font}
  };
  prv_render_cell(main_bounds, ctx, cell_size, "Percent", ARRAY_LENGTH(rich_text_1), rich_text_1);
  // if in full-screen mode
  if (cell_size == CellSizeFullScreen) {
    // temp variables
    GRect tmp_bounds;
    char tmp_buff[16];
    // render percent per day
    tmp_bounds = GRect(0, MENU_CELL_FULL_SCREEN_TOP_OFFSET,
      bounds.size.w, MENU_CELL_FULL_SCREEN_SUB_HEIGHT);
    snprintf(tmp_buff, sizeof(tmp_buff), "%d%% / day", (int) data_get_percent_per_day());
    RichTextElement rich_text_2[] = {
      {tmp_buff, FONT_KEY_GOTHIC_24_BOLD}
    };
    prv_render_cell(tmp_bounds, ctx, cell_size, "Rate", ARRAY_LENGTH(rich_text_2), rich_text_2);
  }
}

// Render time remaining cell
static void prv_cell_render_time_remaining(GRect bounds, GContext *ctx, CellSize cell_size) {
  // get fonts
  char *symbol_font = FONT_KEY_GOTHIC_18_BOLD;
  char *digit_font = (cell_size == CellSizeSmall) ?
    FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_36_BOLD_NUMBERS;
  // calculate time remaining
  int32_t sec_remaining = data_get_life_remaining();
  int days = sec_remaining / SEC_IN_DAY;
  int hrs = sec_remaining % SEC_IN_DAY / SEC_IN_HR;
  // get text
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  // draw text
  GRect main_bounds = grect_inset(bounds,
    GEdgeInsets2((bounds.size.h - MENU_CELL_HEIGHT_TALL) / 2, 0));
  RichTextElement rich_text[] = {
    { day_buff, digit_font },
    { "d ", symbol_font },
    { hr_buff, digit_font },
    { "h", symbol_font }
  };
  prv_render_cell(main_bounds, ctx, cell_size, "Remaining", ARRAY_LENGTH(rich_text), rich_text);
  // if in full-screen mode
  if (cell_size == CellSizeFullScreen) {
    // temp variables
    GRect tmp_bounds;
    int day, hour;
    char tmp_buff[16];
    // render average life
    tmp_bounds = GRect(0, MENU_CELL_FULL_SCREEN_TOP_OFFSET,
      bounds.size.w, MENU_CELL_FULL_SCREEN_SUB_HEIGHT);
    int32_t sec_avg_life = data_get_max_life();
    day = sec_avg_life / SEC_IN_DAY;
    hour = sec_avg_life % SEC_IN_DAY / SEC_IN_HR;
    snprintf(tmp_buff, sizeof(tmp_buff), "%dd %02dh", day, hour);
    RichTextElement rich_text_2[] = {
      {tmp_buff, FONT_KEY_GOTHIC_24_BOLD}
    };
    prv_render_cell(tmp_bounds, ctx, cell_size, "Avg Life", ARRAY_LENGTH(rich_text_2), rich_text_2);
  }
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
  graphics_context_set_fill_color(ctx, COLOR_RING_LOW);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius, 0,
    drawing_data.ring_low_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_MED);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_low_angle, drawing_data.ring_med_angle);
  graphics_context_set_fill_color(ctx, COLOR_RING_NORM);
  graphics_fill_radial(ctx, ring_bounds, GOvalScaleModeFillCircle, radius,
    drawing_data.ring_med_angle, drawing_data.ring_level_angle);
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

// Recalculate and animate the position the progress ring
void drawing_recalculate_progress_rings(void) {
  // calculate angles for ring
  int32_t max_life_sec = data_get_max_life();
  int32_t angle_low = TRIG_MAX_ANGLE * LEVEL_LOW_THRESH_SEC / max_life_sec;
  int32_t angle_med = (int64_t)TRIG_MAX_ANGLE * LEVEL_MED_THRESH_SEC / max_life_sec;
  int32_t angle_level = TRIG_MAX_ANGLE * data_get_battery_percent() / 100;
  angle_low = angle_level < angle_low ? angle_level : angle_low;
  angle_med = angle_level < angle_med ? angle_level : angle_med;
  // animate to new values
  if (abs(angle_low - drawing_data.ring_low_angle) >= ANIMATION_ANGLE_CHANGE_THRESHOLD) {
    animation_int32_start(&drawing_data.ring_low_angle, angle_low, ANI_DURATION,
      STARTUP_ANI_DELAY, CurveSinEaseOut);
  } else {
    drawing_data.ring_low_angle = angle_low;
  }
  if (abs(angle_med - drawing_data.ring_med_angle) >= ANIMATION_ANGLE_CHANGE_THRESHOLD) {
    animation_int32_start(&drawing_data.ring_med_angle, angle_med, ANI_DURATION,
      STARTUP_ANI_DELAY, CurveSinEaseOut);
  } else {
    drawing_data.ring_med_angle = angle_med;
  }
  if (abs(angle_level - drawing_data.ring_level_angle) >= ANIMATION_ANGLE_CHANGE_THRESHOLD) {
    animation_int32_start(&drawing_data.ring_level_angle, angle_level, ANI_DURATION,
      STARTUP_ANI_DELAY, CurveSinEaseOut);
  } else {
    drawing_data.ring_level_angle = angle_level;
  }
}

// Initialize drawing variables
void drawing_initialize(Layer *layer, MenuLayer *menu) {
  // register animation callback
  animation_register_update_callback(prv_animation_refresh_handler);
  // store layer pointer
  drawing_data.layer = layer;
  drawing_data.menu = menu;
  // set initial progress ring angles
  drawing_data.ring_low_angle = drawing_data.ring_med_angle = drawing_data.ring_level_angle = 0;
  // animate to new positions
  drawing_recalculate_progress_rings();
}
