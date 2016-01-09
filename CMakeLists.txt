cmake_minimum_required(VERSION 3.2)
PROJECT (Status+)

FILE(GLOB SOURCE
  "src/*.h"
  "src/*.c"
  "src/js/*.js"
)

FILE(GLOB HEADERS
  "~/Development/Pebble/Resources/SDKs/PebbleSDK-3.6.2/Pebble/chalk/include/*.h"
)

INCLUDE_DIRECTORIES(~/Development/Pebble/Resources/SDKs/PebbleSDK-3.6.2/Pebble/chalk/include)
ADD_EXECUTABLE(Status++ ${SOURCE} ${HEADERS} src src/menu.c src/menu.h src/data/data_library.c src/data/data_library.h src/drawing/windows/alert/alert_window.c src/drawing/windows/alert/alert_window.h src/drawing/windows/edit/pin_window.c src/drawing/windows/edit/pin_window.h src/drawing/windows/edit/selection_layer.c src/drawing/windows/edit/selection_layer.h)
