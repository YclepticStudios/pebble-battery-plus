// @file record_card.c
// @brief Rendering code for record life view
//
// File contains all drawing code for rendering the record lifespan
// card onto a drawing context.
//
// @author Eric D. Phillips
// @date January 20, 2016
// @bugs No known bugs

#include "card_render.h"
#include "../../utility.h"

// Drawing Constants
#define TEXT_BORDER_TOP PBL_IF_RECT_ELSE(102, 88)
#define TEXT_TITLE_HEIGHT 10
#define IMAGE_TOP_OFFSET 15
#define PROGRESS_BAR_WIDTH PBL_IF_RECT_ELSE(50, 25)
#define LINE_STROKE_WIDTH 2



////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Render text
static void prv_render_text(GContext *ctx, GRect bounds, DataAPI *data_api) {
  // resize to fit next to progress bar
  bounds.origin.x = PROGRESS_BAR_WIDTH;
  bounds.origin.y = TEXT_BORDER_TOP;
  bounds.size.w -= PROGRESS_BAR_WIDTH + PBL_IF_RECT_ELSE(0, PROGRESS_BAR_WIDTH);
  // draw title
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, "Record", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), bounds,
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  // get text
  int32_t record_time = data_api_get_record_run_time(data_api);
  int days = record_time / SEC_IN_DAY;
  int hrs = record_time % SEC_IN_DAY / SEC_IN_HR;
  char day_buff[4], hr_buff[3];
  snprintf(day_buff, sizeof(day_buff), "%d", days);
  snprintf(hr_buff, sizeof(hr_buff), "%02d", hrs);
  if (record_time < 0) {
    snprintf(day_buff, sizeof(day_buff), "-");
    snprintf(hr_buff, sizeof(hr_buff), "-");
  }
  // draw text
  RichTextElement rich_text[] = {
    {day_buff, FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM},
    {"d ",     FONT_KEY_GOTHIC_18_BOLD},
    {hr_buff,  FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM},
    {"h",      FONT_KEY_GOTHIC_18_BOLD}
  };
  bounds.origin.y += TEXT_TITLE_HEIGHT;
  bounds.size.h -= bounds.origin.y + PBL_IF_RECT_ELSE(0, PROGRESS_BAR_WIDTH + 8);
  card_render_rich_text(ctx, bounds, ARRAY_LENGTH(rich_text), rich_text);
}

// Render the cup image
static void prv_render_image(GContext *ctx, GRect bounds, DataAPI *data_api) {
  // load from storage
  GDrawCommandImage *command_image =
    gdraw_command_image_create_with_resource(RESOURCE_ID_CUP_IMAGE);
  // if successfully loaded from memory
  if (command_image) {
    // position image
    GSize image_size = gdraw_command_image_get_bounds_size(command_image);
    bounds.origin.x += PROGRESS_BAR_WIDTH;
    bounds.origin.y = IMAGE_TOP_OFFSET + PBL_IF_RECT_ELSE(0, PROGRESS_BAR_WIDTH);
    bounds.size.w -= PROGRESS_BAR_WIDTH + PBL_IF_RECT_ELSE(0, PROGRESS_BAR_WIDTH);
    bounds = grect_inset(bounds, GEdgeInsets2(0, (bounds.size.w - image_size.w) / 2));
    // draw image
    gdraw_command_image_draw(ctx, command_image, bounds.origin);
    // clean up image
    gdraw_command_image_destroy(command_image);
  }
}

// Render progress bar
static void prv_render_progress_bar(GContext *ctx, GRect bounds, DataAPI *data_api) {
#ifdef PBL_RECT
  // get progress bar bounds
  bounds.size.w = PROGRESS_BAR_WIDTH;
  // draw background
  GRect back_bounds = bounds;
  back_bounds.size.h -= back_bounds.size.h * data_api_get_run_time(data_api, 0) /
    data_api_get_record_run_time(data_api);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
  graphics_fill_rect(ctx, back_bounds, 0, GCornerNone);
  // draw fill
  GRect fill_bounds = bounds;
  fill_bounds.origin.y = back_bounds.size.h;
  fill_bounds.size.h = bounds.size.h - back_bounds.size.h;
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_rect(ctx, fill_bounds, 0, GCornerNone);
  // draw outline
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, LINE_STROKE_WIDTH);
  graphics_draw_line(ctx, GPoint(back_bounds.size.w, 0),
    GPoint(back_bounds.size.w, bounds.size.h));
  graphics_draw_line(ctx, GPoint(0, back_bounds.size.h),
    GPoint(back_bounds.size.w, back_bounds.size.h));
#else
  // get current angle
  int32_t angle = (int64_t)TRIG_MAX_ANGLE * data_api_get_run_time(data_api, 0) /
    data_api_get_record_run_time(data_api);
  // draw background
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, PROGRESS_BAR_WIDTH,
    angle, TRIG_MAX_ANGLE);
  // draw fill
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, PROGRESS_BAR_WIDTH, 0, angle);
  // draw border
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, grect_inset(bounds, GEdgeInsets1(PROGRESS_BAR_WIDTH)),
    GOvalScaleModeFitCircle, LINE_STROKE_WIDTH * 2, 0, TRIG_MAX_ANGLE);
  // draw progress line
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, LINE_STROKE_WIDTH);
  GPoint pt1 = gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle);
  GPoint pt2 = gpoint_from_polar(grect_inset(bounds, GEdgeInsets1(PROGRESS_BAR_WIDTH)),
    GOvalScaleModeFitCircle, angle);
  graphics_draw_line(ctx, pt1, pt2);
  pt1 = gpoint_from_polar(bounds, GOvalScaleModeFitCircle, 0);
  pt2 = gpoint_from_polar(grect_inset(bounds, GEdgeInsets1(PROGRESS_BAR_WIDTH)),
    GOvalScaleModeFitCircle, 0);
  graphics_draw_line(ctx, pt1, pt2);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Rendering function for line graph card
void card_render_record_life(Layer *layer, GContext *ctx, uint16_t click_count, DataAPI *data_api) {
  graphics_context_set_antialiased(ctx, false);
  // get bounds
  GRect bounds = layer_get_bounds(layer);
  bounds.origin = GPointZero;
  // render the progress bar
  prv_render_progress_bar(ctx, bounds, data_api);
  // render the image
  prv_render_image(ctx, bounds, data_api);
  // render text
  prv_render_text(ctx, bounds, data_api);
}
