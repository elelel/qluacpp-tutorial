cmake_minimum_required(VERSION 3.4.0)
project(basic_dll)

include(GenerateExportHeader)

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
  ${PROJECT_BINARY_DIR}  # to include generated "cpp_dll_exports.hpp"
)

set(SOURCES
  src/basic_dll.cpp
)

set(CPP_DLL_SOURCES
  src/cpp_dll.cpp)

# MSVC does not default to C++17 yet
if (MSVC_VERSION GREATER_EQUAL "1900")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
  if (_cpp_latest_flag_supported)
    add_compile_options("/std:c++17")
  endif()
endif()

add_library(cpp_dll SHARED ${CPP_DLL_SOURCES})
generate_export_header(cpp_dll
  BASE_NAME cpp_dll
  EXPORT_MACRO_NAME cpp_dll_EXPORT
  EXPORT_FILE_NAME cpp_dll_export.hpp
  STATIC_DEFINE cpp_dll_BUILT_AS_STATIC)

add_library(lualib_basic_dll SHARED ${SOURCES})

target_link_libraries(lualib_basic_dll cpp_dll ${LUA_LIBRARIES})

# Needed for Mingw
set_target_properties(lualib_basic_dll PROPERTIES CXX_STANDARD 17)
