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
ADD_EXECUTABLE(Status++ ${SOURCE} ${HEADERS} src src/menu.c src/menu.h)
