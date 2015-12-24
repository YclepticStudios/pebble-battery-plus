// @file card_render.c
// @brief Rendering code declarations for each card type
//
// File contains declarations of the rendering function for
// each type of card. This header links to each of the c files
// in question. Also contains helper rendering functions.
//
// @author Eric D. Phillips
// @date December 23, 2015
// @bugs No known bugs

#include "card_render.h"

// Constants
#define TEXT_TOP_BORDER_FRACTION 3 / 25


// Render rich text with different fonts onto a graphics context
void card_render_rich_text(GContext *ctx, GRect bounds, uint8_t array_length,
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
