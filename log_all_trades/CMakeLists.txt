cmake_minimum_required(VERSION 3.4.0)
project(log_all_trades)

set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")

set(QLUACPP_LUA_LIBRARY "${LUA}/lua5.1.lib")
set(QLUACPP_LUA_INCLUDE_DIR "${LUA}/include")

include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${QLUACPP_LUA_INCLUDE_DIR}
  )

if (NOT TARGET qluacpp)
  add_subdirectory(${QLUACPP} qluacpp)
endif()

set(SOURCES
  src/log_all_trades.cpp
)

add_library(lualib_log_all_trades SHARED ${SOURCES})

target_link_libraries(lualib_log_all_trades qluacpp)
