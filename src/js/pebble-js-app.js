////////////////////////////////////////////////////////////////////////////////////////////////////
// Timeline Pins
//

// pins
var curDate = new Date();

// "Charge By" pin
var chargeByPin = {
  "id": "battery-plus-charge-by",
  "time": curDate.toISOString(),
  "layout": {
    "type": "weatherPin",
    "title": "Charge By",
//    "shortSubtitle": "Battery+", // currently broken as of SDK 3.9.2
//    "displayTime": "none",
    "subtitle": "00:00",
    "tinyIcon": "system://images/GENERIC_WARNING",
    "largeIcon": "system://images/GENERIC_WARNING",
    "locationName": "Battery+",
    "headings": ["Plug me in!"],
    "paragraphs": ["Remember to charge your Pebble by this time."],
    "backgroundColor": "#FF5500"
  },
  "actions": [
    {
      "title": "Launch App",
      "type": "openWatchApp",
      "launchCode": 0
    }
  ]
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
//

// format a date object into a 12 hr AM/PM string
function formatAMPM(date) {
  var hours = date.getHours();
  var minutes = date.getMinutes();
  var ampm = hours >= 12 ? 'PM' : 'AM';
  hours = hours % 12;
  hours = hours ? hours : 12; // the hour '0' should be '12'
  minutes = minutes < 10 ? '0'+minutes : minutes;
  return hours + ':' + minutes + ' ' + ampm;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Timeline Library
//

// The Timeline public URL root
var API_URL_ROOT = 'https://timeline-api.getpebble.com/';
/**
 * Send a request to the Pebble public web timeline API.
 * @param pin The JSON pin to insert. Must contain 'id' field.
 * @param type The type of request, either PUT or DELETE.
 * @param callback The callback to receive the responseText after the request has completed.
 */
function timelineRequest(pin, type, callback) {
  // User or shared?
  var url = API_URL_ROOT + 'v1/user/pins/' + pin.id;

  // Create XHR
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
      console.log('timeline: response received: ' + this.responseText);
      callback(this.responseText);
  };
  xhr.open(type, url);

  // Get token
  Pebble.getTimelineToken(function (token) {
    // Add headers
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.setRequestHeader('X-User-Token', '' + token);

    // Send
    xhr.send(JSON.stringify(pin));
    console.log('timeline: request sent.');
  }, function (error) { console.log('timeline: error getting timeline token: ' + error); });
}
/**
 * Insert a pin into the timeline for this user.
 * @param pin The JSON pin to insert.
 * @param callback The callback to receive the responseText after the request has completed.
 */
function insertUserPin(pin, callback) {
  timelineRequest(pin, 'PUT', callback);
}
/**
 * Delete a pin from the timeline for this user.
 * @param pin The JSON pin to delete.
 * @param callback The callback to receive the responseText after the request has completed.
 */
function deleteUserPin(pin, callback) {
  timelineRequest(pin, 'DELETE', callback);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// App Message
//

// message received
Pebble.addEventListener('appmessage', function(e) {
  // check for key
  if (e.payload.hasOwnProperty('KEY_CHARGE_BY')){
    // check that it is valid to send pins i.e. its SDK 3.0 or greater
    if (typeof Pebble.getTimelineToken == 'function') {
      // add time
      pinDate = new Date(0);
      pinDate.setUTCSeconds(e.payload.KEY_CHARGE_BY);
      chargeByPin.time = pinDate;
      chargeByPin.layout.subtitle = formatAMPM(new Date(e.payload.KEY_CHARGE_BY * 1000));
      // insert pin
      insertUserPin(chargeByPin, function (responseText) {
        Pebble.sendAppMessage({ KEY_CHARGE_BY: 0 });
        console.log('Pin Sent Result: ' + responseText);
      });
    }
  }
});
