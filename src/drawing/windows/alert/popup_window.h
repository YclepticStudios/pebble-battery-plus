//! @file popup_window.h
//! @brief Popup window for displaying simple notifications
//!
//! Creates a new window containing a file viewer and optional title and footer.
//! This window does not provide user interaction, but can be set to auto-destroy when closed.
//!
//! @author Eric D. Phillips
//! @date January 10, 2016
//! @bug No known bugs

#pragma once
#include <pebble.h>


//! Set the file (PDC image/sequence) to display in the center of the screen
//! @param window The base window layer of the Popup window
//! @param resource_id The ID of the resource to load
void popup_window_set_visual(Window *window, uint32_t resource_id);

//! Set the title and footer text displayed on the window
//! @param window The base window layer of the Popup window
//! @param title_text Text to display as a title for the window
//! @param footer_text Text to display as a footer for the window
void popup_window_set_text(Window *window, char *title_text, char *footer_text);

//! Set the window's background color
//! @param window The base window layer of the Popup window
//! @param color The new color for the window background
void popup_window_set_background_color(Window *window, GColor color);

//! Set whether the window closes when the animation finishes
//! @param window The base window layer of the Popup window
//! @param should_close Whether the window should automatically close
void popup_window_set_close_on_animation_end(Window *window, bool should_close);

//! Create a Popup window
//! @param destroy_on_close Whether to automatically destroy the window when it closes
//! @return A pointer to the base window of the new PinWindow
Window *popup_window_create(bool destroy_on_close);

//! Destroy a Popup window
//! Only call this if not set to auto-destroy
//! @param window The base window layer of the Popup window
void popup_window_destroy(Window *window);
