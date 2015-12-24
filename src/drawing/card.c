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

#include <pebble.h>
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
static void prv_create_screen_bitmap(CardLayer *card_layer, GContext *ctx, GBitmapFormat format) {
  // capture frame buffer and get properties
  GBitmap *old_bmp = graphics_capture_frame_buffer(ctx);
  GRect bmp_bounds = gbitmap_get_bounds(old_bmp);
  // copy to new bitmap and save
#ifdef PBL_BW
  GBitmapFormat bmp_format = gbitmap_get_format(old_bmp);
  card_layer->screen_bmp = gbitmap_create_blank(bmp_bounds.size, bmp_format);
  ASSERT(card_layer->screen_bmp);
  uint32_t bmp_length = gbitmap_get_bytes_per_row(card_layer->screen_bmp) * bmp_bounds.size.h;
  memcpy(gbitmap_get_data(card_layer->screen_bmp), gbitmap_get_data(old_bmp), bmp_length);
#else
  // choose new bitmap format
  uint8_t bmp_bits_per_pixel;
  if (format == GBitmapFormat1BitPalette) {
    bmp_bits_per_pixel = 1;
  } else if (format == GBitmapFormat2BitPalette) {
    bmp_bits_per_pixel = 2;
  } else if (format == GBitmapFormat4BitPalette) {
    bmp_bits_per_pixel = 4;
  } else {
    bmp_bits_per_pixel = 8;
  }
  // create new bitmap
  uint8_t palette_colors = 0;
  uint8_t palette_max_colors = (bmp_bits_per_pixel == 1) ? 2 :
    (bmp_bits_per_pixel * bmp_bits_per_pixel);
  GColor *palette = MALLOC(sizeof(GColor) * palette_max_colors);
  card_layer->screen_bmp = gbitmap_create_blank_with_palette(bmp_bounds.size, format,
    palette, true);
  ASSERT(card_layer->screen_bmp);
  // loop over image rows
  GBitmapDataRowInfo old_row_info, new_row_info;
  for (uint8_t row = 0; row < bmp_bounds.size.h; row++) {
    // get row info
    old_row_info = gbitmap_get_data_row_info(old_bmp, row);
    new_row_info = gbitmap_get_data_row_info(card_layer->screen_bmp, row);
    // get the start of the row data
    // NOTE: if the new bitmap is square and old is round, we must add the old bitmap's byte offset
    // so data is copied from and to the same screen coordinates
    uint8_t *old_byte = old_row_info.data + old_row_info.min_x;
    uint8_t *new_byte = new_row_info.data + old_row_info.min_x / (8 / bmp_bits_per_pixel);
    // loop over all bytes in that row (max_x is inclusive)
    for ( ; old_byte <= old_row_info.data + old_row_info.max_x; old_byte++) {
      // palette index for color in old bitmap
      uint8_t palette_index = 0;
      // loop over the palette and find the index of the current color. if not found, add the color
      do {
        // palette index reaches palette size if no matching color is found in the palette
        if (palette_index == palette_colors) {
          // add the current color to the palette and break (the index matches this color's index)
          palette[palette_index] = (GColor){ .argb = (*old_byte) };
          palette_colors++;
          break;
        } else if (palette[palette_index].argb == (*old_byte)) {
          break;
        }
        palette_index++;
      } while (palette_index < palette_max_colors);
      // get the number of pixels from the right of the new bitmap
      uint16_t new_bmp_pix = (old_byte - (old_row_info.data + old_row_info.min_x)) +
        old_row_info.min_x % (8 / bmp_bits_per_pixel);
      // get the number of bits into the new byte from the right, MSB first
      // (0b00000011 = 0, 0b00001100 = 2, etc)
      uint8_t bit_index = (8 - bmp_bits_per_pixel) - ((new_bmp_pix % (8 / bmp_bits_per_pixel)) *
        bmp_bits_per_pixel);
      // set the bits in the new byte which correspond to the old byte (max_colors - 1 is bitmask)
      (*new_byte) |= (((uint8_t)(palette_max_colors - 1) << bit_index) &
        (palette_index << bit_index));
      // index the new byte once it is full and zero the new location
      if (bit_index == 0) {
        new_byte++;
      }
    }
  }
#endif
  // release frame buffer
  graphics_release_frame_buffer(ctx, old_bmp);
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
    // save rendered screen as bitmap
    if (card_layer->background_color.argb == GColorMelonARGB8) {
      prv_create_screen_bitmap(card_layer, ctx, GBitmapFormat2BitPalette);
    }
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
