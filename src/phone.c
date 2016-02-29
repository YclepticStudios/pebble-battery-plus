// @file phone.c
// @brief Controls communication between watch and phone
//
// Deals with sending and receiving information between watch and phone
// including API requests to servers and Timeline Pins
//
// @author Eric D. Phillips
// @date February 25, 2016
// @bugs No known bugs

#include "phone.h"

// Constants
#define KEY_CHARGE_BY 837502
#define MESSAGE_RESEND_DELAY_MS 500
#define MESSAGE_RESEND_MAX_ATTEMPTS 5
#define WINDOW_FORCE_CLOSE_TIME 3000

// AppTimer for resending failed messages
static AppTimer *app_timer = NULL, *close_timer = NULL;
static uint8_t send_fail_count = 0;
static Window *window_close_on_complete = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks
//

// Close all windows and exit the program
static void prv_exit_app(void *data) {
  // destroy timers
  app_timer_cancel(app_timer);
  app_timer_cancel(close_timer);
  app_timer = NULL;
  close_timer = NULL;
  // close window
  phone_disconnect();
  window_stack_remove(window_close_on_complete, true);
  window_close_on_complete = NULL;
}

// AppTimer callback
static void prv_app_timer_callback(void *data) {
  app_timer = NULL;
  // attempt to resend message
  if (send_fail_count < MESSAGE_RESEND_MAX_ATTEMPTS) {
    send_fail_count++;
    phone_send_timestamp_to_phone((int32_t)data);
  } else {
    send_fail_count = 0;
    prv_exit_app(NULL);
  }
}

// AppMessage inbox received callback
static void prv_inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // for now there is only one thing this will do, so no need to check for a key
  prv_exit_app(NULL);
}

// AppMessage sent callback
static void prv_outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  // reset send count
  send_fail_count = 0;
}

// AppMessage inbox dropped callback
static void prv_inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox message dropped!");
  prv_exit_app(NULL);
}

// AppMessage outbox send failed callback
static void prv_outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason,
                                       void *context) {
  // log message failure
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", (int)reason);
  // attempt to resend message
  if (!app_timer) {
    app_timer_register(MESSAGE_RESEND_DELAY_MS, prv_app_timer_callback, context);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

// Close window when sending is complete or timed out
void phone_set_window_close_on_complete(Window *window) {
  window_close_on_complete = window;
  app_timer_register(WINDOW_FORCE_CLOSE_TIME, prv_exit_app, NULL);
}

// Send timestamp to phone
void phone_send_timestamp_to_phone(int32_t charge_by_epoch) {
  // begin iterator
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  // write data
  dict_write_int32(iter, KEY_CHARGE_BY, charge_by_epoch);
  dict_write_end(iter);
  // send
  AppMessageResult result = app_message_outbox_send();
  // resend message if failed
  app_message_set_context((void*)charge_by_epoch);
  if (result != APP_MSG_OK && !app_timer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dictionary send failed: %d", (int)result);
    app_timer_register(MESSAGE_RESEND_DELAY_MS, prv_app_timer_callback, (void*)charge_by_epoch);
  }
}

//! Establish AppMessage communication with the phone
void phone_connect(void) {
  // register callbacks
  app_message_register_inbox_received(prv_inbox_received_callback);
  app_message_register_outbox_sent(prv_outbox_sent_callback);
  app_message_register_inbox_dropped(prv_inbox_dropped_callback);
  app_message_register_outbox_failed(prv_outbox_failed_callback);
  // open AppMessage
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Ram: %d", (int)heap_bytes_free());
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

//! Terminate AppMessage communication with the phone
void phone_disconnect(void) {
  // deregister callbacks
  app_message_deregister_callbacks();
}
