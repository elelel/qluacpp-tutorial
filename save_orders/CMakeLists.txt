cmake_minimum_required(VERSION 3.4.0)
project(save_orders)

set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")
set(JSON "${TOP_DIR}/contrib/json")
set(CQUEUE "${TOP_DIR}/contrib/concurrentqueue")

set(QLUACPP_LUA_LIBRARY "${LUA}/lua5.1.lib")
set(QLUACPP_LUA_INCLUDE_DIR "${LUA}/include")

include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${QLUACPP_LUA_INCLUDE_DIR}
  ${JSON}/src/
  ${CQUEUE}
)

if (NOT TARGET qluacpp)
  add_subdirectory(${QLUACPP} qluacpp)
endif()

set(SOURCES
  src/order_db.cpp
  src/log_window.cpp
  src/save_orders.cpp
)

add_library(lualib_save_orders SHARED ${SOURCES})

target_link_libraries(lualib_save_orders qluacpp)
