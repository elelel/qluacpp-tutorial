cmake_minimum_required(VERSION 3.4.0)
project(basic_tutorial)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
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
  src/basic_tutorial.cpp
)

add_library(lualib_basic_tutorial SHARED ${SOURCES})

target_link_libraries(lualib_basic_tutorial qluacpp)


