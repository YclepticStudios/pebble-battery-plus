//! @file card.h
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

#pragma once
#include <pebble.h>
#include "../data/data_library.h"

//! Card callback type
typedef void (*CardRenderHandler)(Layer*, GContext*, uint16_t, DataLibrary*);


//! Free the rendered cache of the card if it is not visible
//! @param layer Pointer to base layer for card
void card_free_cache_if_hidden(Layer *layer);

//! Render the card and cache it the next chance possible
//! @param layer Pointer to base layer for card
void card_render(Layer *layer);

//! Send click event to current card and re-render
void card_select_click(Layer *layer);

//! Initialize card
//! @param bounds The dimensions of the window
//! @param bmp_format The GBitmapFormat to cache the rendered screen in
//! @param background_color The background fill color of the card
//! @param render_handler The function which will be called to render this card
//! @param data_library A pointer to the main data library from which to get information
//! @return A pointer to the base layer of this card
Layer *card_initialize(GRect bounds, GBitmapFormat bmp_format, GColor background_color,
                       CardRenderHandler render_handler, DataLibrary *data_library);

//! Terminate card
//! @param layer Pointer to base layer for card
void card_terminate(Layer *layer);
