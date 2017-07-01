cmake_minimum_required(VERSION 3.4.0)
project(draw_candles)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")
set(RWQUEUE "${TOP_DIR}/contrib/readerwriterqueue")

set(LUA_LIBRARIES "${LUA}/lua5.1.lib")
set(LUA_INCLUDE_DIR "${LUA}/include")

include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${LUA_INCLUDE_DIR}
  ${RWQUEUE}
)

set(SOURCES
  src/draw_candles.cpp
  src/model.cpp
  src/gui.cpp
)

add_library(lualib_draw_candles SHARED ${SOURCES})

target_link_libraries(lualib_draw_candles ${LUA_LIBRARIES})
