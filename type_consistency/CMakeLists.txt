cmake_minimum_required(VERSION 3.4.0)
project(type_consistency)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
set(QLUACPP "${TOP_DIR}/contrib/qluacpp")
set(LUACPP "${TOP_DIR}/contrib/qluacpp/contrib/luacpp")

# Choose 32bit or 64bit lua library
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(LUA "${TOP_DIR}/contrib/lua32")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(LUA "${TOP_DIR}/contrib/lua64")
endif()
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

# MSVC does not default to C++17 yet
if (MSVC_VERSION GREATER_EQUAL "1900")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
  if (_cpp_latest_flag_supported)
    add_compile_options("/std:c++17")
  endif()
endif()

add_library(lualib_type_consistency SHARED ${SOURCES})

target_link_libraries(lualib_type_consistency ${LUA_LIBRARIES})
