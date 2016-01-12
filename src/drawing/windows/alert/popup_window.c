// @file popup_window.h
// @brief Popup window for displaying simple notifications
//
// Creates a new window containing a PDC viewer and optional title and footer.
// This window does not provide user interaction, but can be set to auto-destroy when closed.
//
// @author Eric D. Phillips
// @date January 10, 2016
// @bug No known bugs

#include "popup_window.h"
#include "../../../utility.h"

// Constants
#define SEQUENCE_NEXT_FRAME_DELAY 33
#define BACKGROUND_DEFAULT_COLOR GColorMagenta

// File type options
typedef enum {
  FileTypePDCImage,
  FileTypePDCSequence,
  FileTypeUnknown
} FileType;

// Main data struct
typedef struct {
  Layer         *canvas_layer;      //< Pointer to canvas Layer
  TextLayer     *title_layer;       //< Pointer to title TextLayer
  TextLayer     *footer_layer;      //< Pointer to footer TextLayer
  AppTimer      *app_timer;         //< Animation timer for PDC sequence
  void          *file;              //< Pointer to loaded file
  FileType      file_type;          //< Type of file loaded
  uint16_t      frame_index;        //< Current frame index if animation file type
  bool          close_on_animation_end; //< Whether to automatically on the animation end time
  bool          destroy_on_close;   //< Whether to auto-destroy when returning
} PopupWindowData;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//

static void next_frame_handler(void *context) {
  Window *window = context;
  PopupWindowData *window_data = window_get_user_data(window);
  window_data->app_timer = NULL;
  // check if animation is complete/wrapping
  int num_frames = gdraw_command_sequence_get_num_frames(window_data->file);
  if (window_data->frame_index >= num_frames) {
    if (window_data->close_on_animation_end) {
      window_stack_remove(window, true);
      return;
    } else {
      window_data->frame_index = 0;
    }
  }
  // refresh
  layer_mark_dirty(window_data->canvas_layer);
  // schedule next timer
  window_data->app_timer = app_timer_register(SEQUENCE_NEXT_FRAME_DELAY, next_frame_handler,
    window);
}

// Layer update callback
static void prv_layer_update_proc(Layer *layer, GContext *ctx) {
  // get properties
  PopupWindowData **layer_data = layer_get_data(layer);
  PopupWindowData *window_data = (*layer_data);
  GRect layer_bounds = layer_get_bounds(layer);
  // check file type
  if (window_data->file_type == FileTypePDCImage) {
    // draw PDC image
    GSize image_size = gdraw_command_image_get_bounds_size(window_data->file);
    GPoint image_origin = GPoint(
      (layer_bounds.size.w - image_size.w) / 2,
      (layer_bounds.size.h - image_size.h) / 2
    );
    gdraw_command_image_draw(ctx, window_data->file, image_origin);
  } else if (window_data->file_type == FileTypePDCSequence) {
    // draw PDC sequence
    GSize sequence_bounds = gdraw_command_sequence_get_bounds_size(window_data->file);
    // get next frame
    GDrawCommandFrame *pdc_frame = gdraw_command_sequence_get_frame_by_index(window_data->file,
      window_data->frame_index);
    // draw frame if found
    if (pdc_frame) {
      gdraw_command_frame_draw(ctx, window_data->file, pdc_frame, GPoint(
        (layer_bounds.size.w - sequence_bounds.w) / 2,
        (layer_bounds.size.h - sequence_bounds.h) / 2
      ));
    }
    // advance to the next frame
    window_data->frame_index++;
  }
}

// Window appear handler
static void prv_window_appear_handler(Window *window) {
  // resume/start animation playback if animation file type
  PopupWindowData *window_data = window_get_user_data(window);
  if (window_data->file_type == FileTypePDCSequence) {
    app_timer_register(SEQUENCE_NEXT_FRAME_DELAY, next_frame_handler, window);
  }
}

// Window disappear handler
static void prv_window_disappear_handler(Window *window) {
  // pause animation
  PopupWindowData *window_data = window_get_user_data(window);
  if (window_data->file_type == FileTypePDCSequence && window_data->app_timer) {
    app_timer_cancel(window_data->app_timer);
  }
}

// Window unload handler
static void prv_window_unload_handler(Window *window) {
  popup_window_destroy(window);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Set the file (PDC image/sequence) to display in the center of the screen
void popup_window_set_visual(Window *window, uint32_t resource_id) {
  PopupWindowData *window_data = window_get_user_data(window);
  // try to read the file and determine its type
  void *file = gdraw_command_image_create_with_resource(resource_id);
  if (file) {
    window_data->file = file;
    window_data->file_type = FileTypePDCImage;
    return;
  }
  file = gdraw_command_sequence_create_with_resource(resource_id);
  if (file) {
    window_data->file = file;
    window_data->file_type = FileTypePDCSequence;
    return;
  }
  window_data->file = NULL;
  window_data->file_type = FileTypeUnknown;
}

// Set the title and footer text displayed on the window
void popup_window_set_text(Window *window, char *title_text, char *footer_text) {
  PopupWindowData *window_data = window_get_user_data(window);
  text_layer_set_text(window_data->title_layer, title_text);
  text_layer_set_text(window_data->footer_layer, footer_text);
}

// Set the window's background color
void popup_window_set_background_color(Window *window, GColor color) {
  window_set_background_color(window, color);
}

// Set whether the window closes when the animation finishes
void popup_window_set_close_on_animation_end(Window *window, bool should_close) {
  PopupWindowData *window_data = window_get_user_data(window);
  window_data->close_on_animation_end = should_close;
}

// Create a Popup window
Window *popup_window_create(bool destroy_on_close) {
  // create window
  Window *window = window_create();
  ASSERT(window);
  window_set_background_color(window, BACKGROUND_DEFAULT_COLOR);
  WindowHandlers window_handlers = (WindowHandlers) {
    .appear = prv_window_appear_handler,
    .disappear = prv_window_disappear_handler,
    .unload = prv_window_unload_handler
  };
  window_set_window_handlers(window, window_handlers);
  PopupWindowData *window_data = MALLOC(sizeof(PopupWindowData));
  (*window_data) = (PopupWindowData) {
    .file_type = FileTypeUnknown,
    .close_on_animation_end = true,
    .destroy_on_close = destroy_on_close
  };
  window_set_user_data(window, window_data);
  // get window parameters
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  // create title TextLayer
  const GEdgeInsets title_text_insets = { .top = PBL_IF_RECT_ELSE(22, 27) };
  window_data->title_layer = text_layer_create(grect_inset(window_bounds, title_text_insets));
  text_layer_set_background_color(window_data->title_layer, GColorClear);
  text_layer_set_text(window_data->title_layer, "Alert Title");
  text_layer_set_font(window_data->title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(window_data->title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(window_data->title_layer));
  // create footer TextLayer
  const GEdgeInsets footer_text_insets = {.top = 125, .right = 5, .bottom = 10, .left = 5};
  window_data->footer_layer = text_layer_create(grect_inset(window_bounds, footer_text_insets));
  text_layer_set_background_color(window_data->footer_layer, GColorClear);
  text_layer_set_text(window_data->footer_layer, "This is the footer of a dummy alert!");
  text_layer_set_text_alignment(window_data->footer_layer, GTextAlignmentCenter);
  text_layer_set_font(window_data->footer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(window_data->footer_layer));
  // create canvas layer
  window_data->canvas_layer = layer_create_with_data(window_bounds, sizeof(PopupWindowData*));
  PopupWindowData **layer_data = layer_get_data(window_data->canvas_layer);
  (*layer_data) = window_data;
  layer_set_update_proc(window_data->canvas_layer, prv_layer_update_proc);
  layer_add_child(window_layer, window_data->canvas_layer);
  return window;
}

// Destroy a Popup window
void popup_window_destroy(Window *window) {
  PopupWindowData *window_data = window_get_user_data(window);
  // destroy the loaded file based on its type
  if (window_data->file_type == FileTypePDCImage) {
    gdraw_command_image_destroy(window_data->file);
  } else if (window_data->file_type == FileTypePDCSequence) {
    gdraw_command_sequence_destroy(window_data->file);
  }
  // destroy other stuff
  layer_destroy(window_data->canvas_layer);
  text_layer_destroy(window_data->footer_layer);
  text_layer_destroy(window_data->title_layer);
  window_destroy(window);
  free(window_data);
}