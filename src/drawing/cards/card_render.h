//! @file card_render.h
//! @brief Rendering code declarations for each card type
//!
//! File contains declarations of the rendering function for
//! each type of card. This header links to each of the c files
//! in question.
//!
//! @author Eric D. Phillips
//! @date December 23, 2015
//! @bugs No known bugs

#pragma once
#include <pebble.h>

// Constants
#define CARD_BACK_COLOR_DASHBOARD GColorElectricBlue
#define CARD_BACK_COLOR_LINE_GRAPH GColorMelon
#define CARD_BACK_COLOR_BAR_GRAPH GColorRichBrilliantLavender
#define CARD_PALETTE_DASHBOARD GBitmapFormat4BitPalette
#define CARD_PALETTE_LINE_GRAPH GBitmapFormat2BitPalette
#define CARD_PALETTE_BAR_GRAPH GBitmapFormat2BitPalette


//! Rendering function for dashboard card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
void card_render_dashboard(Layer *layer, GContext *ctx);

//! Rendering function for line graph card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
void card_render_line_graph(Layer *layer, GContext *ctx);

//! Rendering function for bar graph card
//! @param layer The base layer for this card
//! @param ctx The graphics context which will be rendered on
void card_render_bar_graph(Layer *layer, GContext *ctx);
