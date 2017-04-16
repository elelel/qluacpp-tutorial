cmake_minimum_required(VERSION 3.4.0)
project(dividend_threat_test)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)

set(CURL "${TOP_DIR}/contrib/curl")
set(CATCH "${TOP_DIR}/contrib/Catch")
set(SRC "${TOP_DIR}/src")

include_directories(
  ${SRC}
  ${CURL}/include
  ${CMAKE_CURRENT_BINARY_DIR}/curl/include/curl
  ${CATCH}/include
)

if (NOT TARGET curl)
  set(BUILD_CURL_EXE OFF CACHE BOOL "Don't build curl.exe" FORCE)
  set(ENABLE_DOCS OFF CACHE BOOL "Disable docs" FORCE)
  set(ENABLE_MANUAL OFF CACHE BOOL "Disable manual" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
  add_subdirectory(${CURL} curl)
endif()

set(SOURCES
  ${SRC}/calendar.cpp)

set(TEST_SOURCES
  ${SOURCES}
  dividend_threat_test.cpp)

add_executable(dividend_threat_test
  ${TEST_SOURCES})

target_compile_definitions(dividend_threat_test PUBLIC "CATCH_CONFIG_MAIN" "CATCH_CONFIG_COLOUR_NONE")
target_link_libraries(dividend_threat_test libcurl)
add_test(CommonTests dividend_threat_test)
