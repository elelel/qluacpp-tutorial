cmake_minimum_required(VERSION 3.4.0)
project(type_consistency)

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
  src/type_consistency.cpp
)

add_library(lualib_type_consistency SHARED ${SOURCES})

target_link_libraries(lualib_type_consistency ${LUA_LIBRARIES})
