cmake_minimum_required(VERSION 3.4.0)
project(dividend_threat)

set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")
set(LUA "${TOP_DIR}/contrib/lua")
set(CURL "${TOP_DIR}/contrib/curl")

set(QLUACPP_LUA_LIBRARY "${LUA}/lua5.1.lib")
set(QLUACPP_LUA_INCLUDE_DIR "${LUA}/include")

include_directories(
  ${QLUACPP}/include
  ${LUACPP}/include
  ${QLUACPP_LUA_INCLUDE_DIR}
  ${CURL}/include
  ${CMAKE_CURRENT_BINARY_DIR}/curl/include/curl
)

if (NOT TARGET qluacpp)
  add_subdirectory(${QLUACPP} qluacpp)
endif()

if (NOT TARGET curl)
  set(BUILD_CURL_EXE OFF CACHE BOOL "Don't build curl.exe" FORCE)
  set(ENABLE_DOCS OFF CACHE BOOL "Disable docs" FORCE)
  set(ENABLE_MANUAL OFF CACHE BOOL "Disable manual" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
  add_subdirectory(${CURL} curl)
endif()

set(SOURCES
  src/calendar.cpp
  src/dividends_table.cpp
  src/dividend_threat.cpp
)

add_library(lualib_dividend_threat SHARED ${SOURCES})

target_link_libraries(lualib_dividend_threat qluacpp libcurl)
