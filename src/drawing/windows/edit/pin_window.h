//! @file pin_window.h
//! @brief Popup window for entering a series of numbers
//!
//! Creates and displays a window with Pebble style bouncing selectors.
//! It is used for choosing a number or series of numbers.
//!
//! @author Eric D. Phillips
//! @date January 8, 2016
//! @bug No known bugs

#pragma once
#include <pebble.h>

//! Constants
#define PIN_WINDOW_MAX_FIELD_COUNT 3

//! Pin window return value callback
//! Canceled shows whether or not the selection was made or the window was canceled
//! "numbers" array is freed after this callback returns
typedef void (*PinWindowReturnCallback)(bool canceled, uint8_t number_count, int8_t *numbers,
                                        void *context);


//! Set the active and inactive field colors
//! @param window The base window layer of the PinWindow
//! @param active_color The color to which to set the active cell
//! @param inactive_color The color to which to set the inactive cell
void pin_window_set_field_colors(Window *window, GColor active_color, GColor inactive_color);

//! Set the title and footer text displayed on the window
//! @param window The base window layer of the PinWindow
//! @param title_text Text to display as a title for the window
//! @param footer_text Text to display as a footer for the window
void pin_window_set_text(Window *window, char *title_text, char *footer_text);

//! Set the maximum value for each field (default of 9)
//! @param window The base window layer of the PinWindow
//! @param max_values A pointer to an array of maximum values (array length equal to field count)
void pin_window_set_max_field_values(Window *window, uint8_t *max_values);

//! Set the value for each field (default of 0)
//! @param window The base window layer of the PinWindow
//! @param values A pointer to an array of values (array length equal to field count)
void pin_window_set_field_values(Window *window, uint8_t *values);

//! Set PinWindow callback handler
//! @param window The base window layer of the PinWindow
//! @param return_callback The return callback to be called
void pin_window_set_return_callback(Window *window, PinWindowReturnCallback return_callback);

//! Set the context which will be returned in the callback
//! @param window The base window layer of the PinWindow
//! @param context The context to be returned
void pin_window_set_context(Window *window, void *context);

//! Create a PinWindow
//! @param field_count The number of selection fields to display
//! @param destroy_on_close Whether to automatically destroy the window when it closes
//! @return A pointer to the base window of the new PinWindow
Window *pin_window_create(uint8_t field_count, bool destroy_on_close);

//! Destroy a PinWindow
//! Only call this if not set to auto-destroy
//! @param window The base window layer of the PinWindow
void pin_window_destroy(Window *window);
