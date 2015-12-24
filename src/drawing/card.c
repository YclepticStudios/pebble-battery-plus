//! @file card.c
//! @brief Base class for a rendered card
//!
//! Contains base level constructors and handlers which are
//! used across all kinds of cards. Enables rendering onto a
//! GContext and then converting that to a bitmap, which is
//! rendered from then on. This enables more advanced interfaces
//! to be animated in the card type sliding animation.
//!
//! @author Eric D. Phillips
//! @date December 23, 2015
//! @bugs No known bugs

#include "card.h"
#include "../utility.h"

// Main data structure
typedef struct {
  GBitmap             *screen_bmp;            //< Bitmap of rendered screen
  GColor              background_color;       //< Background color for layer
  CardRenderHandler   render_handler;         //< Function pointer to render specific card
} CardLayer;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

// Create screen bitmap from rendered graphics context
static void prv_create_screen_bitmap(CardLayer *card_layer, GContext *ctx) {
  // capture frame buffer
  GBitmap *frame_buff = graphics_capture_frame_buffer(ctx);
  GRect frame_bounds = gbitmap_get_bounds(frame_buff);
  GBitmapFormat frame_format = gbitmap_get_format(frame_buff);
  // create new bitmap
  if (frame_format != GBitmapFormat1Bit && frame_format != GBitmapFormat1BitPalette) {
    frame_format = GBitmapFormat8Bit;
  }
  card_layer->screen_bmp = gbitmap_create_blank(frame_bounds.size, frame_format);
  ASSERT(card_layer->screen_bmp);
  // copy image to new bitmap
#ifdef PBL_RECT
  uint32_t bmp_length = gbitmap_get_bytes_per_row(card_layer->screen_bmp) * frame_bounds.size.h;
  memcpy(gbitmap_get_data(card_layer->screen_bmp), gbitmap_get_data(frame_buff), bmp_length);
#else
  uint8_t *screen_bmp_data = gbitmap_get_data(card_layer->screen_bmp);
  GBitmapDataRowInfo row_info;
  for (uint8_t row = 0; row < frame_bounds.size.h; row++) {
    row_info = gbitmap_get_data_row_info(frame_buff, row);
    memset(screen_bmp_data, card_layer->background_color.argb, row_info.min_x);
    memcpy(screen_bmp_data + row_info.min_x, row_info.data + row_info.min_x, row_info.max_x -
      row_info.min_x);
    memset(screen_bmp_data + row_info.max_x, card_layer->background_color.argb,
      frame_bounds.size.w - row_info.max_x);
    // index
    screen_bmp_data += gbitmap_get_bytes_per_row(card_layer->screen_bmp);
  }
#endif
  // release frame buffer
  graphics_release_frame_buffer(ctx, frame_buff);
}

// Base layer callback for drawing background color
static void prv_layer_update_handler(Layer *layer, GContext *ctx) {
  // get CardLayer data
  CardLayer *card_layer = layer_get_data(layer);
  // check if existing buffer
  if (!card_layer->screen_bmp) {
    // render card
    GRect bounds = layer_get_bounds(layer);
    bounds.origin = GPointZero;
    graphics_context_set_fill_color(ctx, card_layer->background_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    card_layer->render_handler(layer, ctx);
    // TODO: Deal with this unused function...
//    // save rendered screen as bitmap
//    prv_create_screen_bitmap(card_layer, ctx);
  } else {
    // draw bitmap
    GRect bounds = layer_get_bounds(layer);
    bounds.origin = GPointZero;
    graphics_draw_bitmap_in_rect(ctx, card_layer->screen_bmp, bounds);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Interface
//

// Refresh everything on card
void card_refresh(Layer *layer) {
  // TODO: Implement this function
}

// Initialize card
Layer *card_initialize(GRect bounds, GColor background_color, CardRenderHandler render_handler) {
  // create base layer with extra data
  Layer *layer = layer_create_with_data(bounds, sizeof(CardLayer));
  ASSERT(layer);
  layer_set_update_proc(layer, prv_layer_update_handler);
  CardLayer *card_layer = layer_get_data(layer);
  card_layer->screen_bmp = NULL;
  card_layer->background_color = background_color;
  card_layer->render_handler = render_handler;
  // return layer pointer
  return layer;
}

// Terminate card
void card_terminate(Layer *layer) {
  // get layer data
  CardLayer *card_layer = layer_get_data(layer);
  // destroy layers
  if (card_layer->screen_bmp) {
    gbitmap_destroy(card_layer->screen_bmp);
  }
  layer_destroy(layer);
}
