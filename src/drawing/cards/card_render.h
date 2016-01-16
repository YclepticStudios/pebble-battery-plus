//! @file card_render.h
//! @brief Rendering code declarations for each card type
//!
//! File contains declarations of the rendering function for
//! each type of card. This header links to each of the c files
//! in question. Also contains helper rendering functions.
//!
//! @author Eric D. Phillips
//! @date December 23, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>
#include "../../data/data_library.h"

// Constants
#define CARD_BACK_COLOR_DASHBOARD PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack)
#define CARD_BACK_COLOR_LINE_GRAPH GColorMelon
#define CARD_BACK_COLOR_BAR_GRAPH GColorElectricBlue
#define CARD_PALETTE_DASHBOARD GBitmapFormat4BitPalette
#define CARD_PALETTE_LINE_GRAPH GBitmapFormat2BitPalette
#define CARD_PALETTE_BAR_GRAPH GBitmapFormat4BitPalette

//! Rich text element
typedef struct {
  char      *text;        //!< Pointer to text to draw
  char      *font;        //!< Pointer to font face string to draw with
} RichTextElement;


//! Render rich text with different fonts onto a graphics context
//! @param ctx The graphics context onto which to render
//! @param bounds The bounds in which to render the text
//! @param array_length The number of RichTextElements in the following array
//! @param rich_text Pointer to array of RichTextElements which will be rendered
void card_render_rich_text(GContext *ctx, GRect bounds, uint8_t array_length,
                           RichTextElement *rich_text);


//! Rendering function for dashboard card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
//! @param click_count Number of select click events on this card
//! @param data_library A pointer to the main data library
void card_render_dashboard(Layer *layer, GContext *ctx, uint16_t click_count,
                           DataLibrary *data_library);

//! Rendering function for line graph card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
//! @param click_count Number of select click events on this card
//! @param data_library A pointer to the main data library
void card_render_line_graph(Layer *layer, GContext *ctx, uint16_t click_count,
                            DataLibrary *data_library);

//! Rendering function for bar graph card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
//! @param click_count Number of select click events on this card
//! @param data_library A pointer to the main data library
void card_render_bar_graph(Layer *layer, GContext *ctx, uint16_t click_count,
                           DataLibrary *data_library);
