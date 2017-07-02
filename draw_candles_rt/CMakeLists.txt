cmake_minimum_required(VERSION 3.4.0)
project(draw_candles_rt)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")

set(LUA_LIBRARIES "${LUA}/lua5.1.lib")
set(LUA_INCLUDE_DIR "${LUA}/include")

include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${LUA_INCLUDE_DIR}
)

set(SOURCES
  src/draw_candles_rt.cpp
  src/model.cpp
  src/gui.cpp
)

add_library(lualib_draw_candles_rt SHARED ${SOURCES})

target_link_libraries(lualib_draw_candles_rt ${LUA_LIBRARIES})
