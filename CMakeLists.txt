# MIT License
#
# Copyright (c) 2023-2026 Kritzel Kratzel.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in 
# the Software without restriction, including without limitation the rights to 
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# ------------------------------------------------------------------------------
# CMake for luacheck
# ------------------------------------------------------------------------------
# Setup with Visual Studio 17 2022 generator for x64
# ------------------------------------------------------------------------------
# Visual Studio is a mutlti-configuration generator
# https://stackoverflow.com/questions/24460486/
#
# Basic instructions for out-of-source build
# ------------------------------------------
# mkdir build && cd build
# cmake .. -G "Visual Studio 17 2022" -A x64
# cmake --build . --config Release
# cmake --install . --config Release
# 
# Available architectures (-A ...) are: Win32, x64, ARM, ARM64
#
# Note: As luacheck does not comprise any files to be compiled and linked (just
# files to be installed properly) the mention of architecture or config is
# somewhat arbitrary.

# ------------------------------------------------------------------------------
# General definitions
cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

# ------------------------------------------------------------------------------
# find liblua installation and version info
if(NOT LUA_HINTS)
  if(WIN32)
    set(LUA_HINTS "c:/Apps")
  endif()
endif()
find_package(liblua REQUIRED CONFIG HINTS ${LUA_HINTS})
if(liblua_FOUND)
  message(STATUS "liblua version        : ${liblua_VERSION}")
  message(STATUS "liblua install prefix : ${LIBLUA_INSTALLDIR}")
  message(STATUS "liblua include dir    : ${LIBLUA_INCLUDEDIR}")
  message(STATUS "liblua lib dir        : ${LIBLUA_LIBDIR}")
else()
  message(FATAL_ERROR "Unable to find liblua version ${liblua_VERSION}.")
endif()
# Note: liblua_VERSION is set by find_package() directly. LIBLUA_INSTALLDIR,
# LIBLUA_INCLUDEDIR and LIBLUA_LIBDIR are set by libluaConfigVersion.cmake in
# <prefix>/share/cmake/liblua.

# ------------------------------------------------------------------------------
# Installation prefix directory - automatically set from find_package()
# Needs to be defined before project definition statement - for whatever reason
if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${LIBLUA_INSTALLDIR})
endif()

# ------------------------------------------------------------------------------
# Project defintion
project(luacheck CXX)

# ------------------------------------------------------------------------------
# Other settings
set(CMAKE_VERBOSE_MAKEFILE OFF)

# ------------------------------------------------------------------------------
# Setup GNU-alike installatin directories
include (GNUInstallDirs)
set(INSTALL_BINDIR
  ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_DOCDIR
  ${CMAKE_INSTALL_DOCDIR} CACHE PATH "Installation directory for documentation")
set(INSTALL_DATAROOTDIR
  ${CMAKE_INSTALL_DATAROOTDIR} CACHE PATH "Installation directory for data")
# Lua-specific installation dirs
set(INSTALL_TOP_LDIR
  ${INSTALL_DATAROOTDIR}/lua/${liblua_VERSION_MAJOR}.${liblua_VERSION_MINOR})

# ------------------------------------------------------------------------------
# luacheck Launcher (.exe instead of .bat)
if(WIN32)

  # Define absolute paths for the input Lua script and the generated C++ header
  set(LUACHECK_INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bin/luacheck.lua")
  set(LUACHECK_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/luacheck_source.h")

  # Define the generation rule: Converts the Lua script into a C-style hex array.
  # This avoids encoding issues and delimiter conflicts in C++ string literals.
  # Ensure the header is rebuilt if the Lua script or the converter script changes
  # VERBATIM ensures correct argument escaping for the Windows Shell/MSBuild
  add_custom_command(
    OUTPUT "${LUACHECK_HEADER_FILE}"
    COMMAND ${CMAKE_COMMAND} 
    -DINPUT_FILE=${LUACHECK_INPUT_FILE}
    -DOUTPUT_FILE=${LUACHECK_HEADER_FILE} 
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/convert_lua_to_hex.cmake"
    DEPENDS "${LUACHECK_INPUT_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/convert_lua_to_hex.cmake"
    COMMENT "Converting luacheck.lua to hex header..."
    VERBATIM
  )

  # Define the executable target
  add_executable(LuacheckLauncher
    logo/lua-logo-olp-dist.rc # Windows resource file (icon/metadata)
    bin/luacheck.cpp          # Main C++ launcher logic
    "${LUACHECK_HEADER_FILE}" # Listing the header here makes it visible to MSBuild's dependency scanner
  )

  # Mark the file as generated so CMake doesn't look for it during the initial configuration
  set_source_files_properties("${LUACHECK_HEADER_FILE}" PROPERTIES GENERATED TRUE)

  # Crucial for MSVC/MSBuild: Tells the compiler that luacheck.cpp MUST be recompiled 
  # whenever luacheck_source.h is updated. This bridges the gap between the custom command and
  # the C++ build.
  set_source_files_properties(luacheck.cpp PROPERTIES OBJECT_DEPENDS "${LUACHECK_HEADER_FILE}")

  # Include directories: ensure the compiler finds both Lua headers and our generated hex header
  target_include_directories(LuacheckLauncher PRIVATE ${LIBLUA_INCLUDEDIR})
  target_include_directories(LuacheckLauncher PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

  # Linker: Set path for the Lua library
  target_link_directories(LuacheckLauncher PRIVATE ${LIBLUA_LIBDIR})
  
  # Link dependencies: liblua for the interpreter and pathcch for Windows path handling
  target_link_libraries(LuacheckLauncher PRIVATE liblua.lib pathcch.lib)

  # Rename the final binary to 'luacheck.exe' (default would be LuacheckLauncher.exe)
  set_target_properties(LuacheckLauncher PROPERTIES OUTPUT_NAME "luacheck")

  # Define the installation rule for the resulting binary
  install(TARGETS LuacheckLauncher RUNTIME DESTINATION ${INSTALL_BINDIR})
endif()

# ------------------------------------------------------------------------------
# Install luacheck files and docs
install(DIRECTORY src/ DESTINATION ${INSTALL_TOP_LDIR}
  FILES_MATCHING PATTERN "*.lua"
)
install(DIRECTORY doc-html/ DESTINATION ${INSTALL_DOCDIR})
