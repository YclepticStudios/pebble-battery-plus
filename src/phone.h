//! @file phone.h
//! @brief Controls communication between watch and phone
//!
//! Deals with sending and receiving information between watch and phone
//! including API requests to servers and Timeline Pins
//!
//! @author Eric D. Phillips
//! @date February 25, 2016
//! @bugs No known bugs

#pragma once
#include <pebble.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// API Implementation
//

//! Close window when sending is complete or timed out
//! @param window Pointer to window to close when complete
void phone_set_window_close_on_complete(Window *window);

//! Send a timestamp to the phone to be converted to a pin
//! @param charge_by_epoch The time the pin should be created for
void phone_send_timestamp_to_phone(int32_t charge_by_epoch);

//! Establish AppMessage communication with the phone
void phone_connect(void);

//! Terminate AppMessage communication with the phone
void phone_disconnect(void);
