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
ADD_EXECUTABLE(Status++ ${SOURCE} ${HEADERS} src/drawing/cards/dashboard_card.c
        src/drawing/cards/dashboard_card.h src/drawing/drawing.c src/drawing/drawing.h
        src/drawing/cards/line_graph_card.h src/drawing/cards/line_graph_card.c
        src/drawing/cards/bar_graph_card.h src/drawing/cards/bar_graph_card.c src/drawing/card.c src/drawing/card.h src/drawing/cards/card_render.h)
