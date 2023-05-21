# MIT License
#
# Copyright (c) 2023 Kritzel Kratzel.
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
# Setup with Visual Studio 17 2022 generator for Win32
# ------------------------------------------------------------------------------
# Visual Studio is a mutlti-configuration generator
# https://stackoverflow.com/questions/24460486/
#
# Basic instructions for out-of-source build
# ------------------------------------------
# mkdir build
# cd build
# cmake .. -G "Visual Studio 17 2022" -A Win32
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
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# ------------------------------------------------------------------------------
# find liblua installation and version info
if(WIN32)
  set(LUA_HINTS "c:/Apps")
else()
  set(LUA_HINTS "")
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
set(CMAKE_INSTALL_PREFIX ${LIBLUA_INSTALLDIR})

# ------------------------------------------------------------------------------
# Project defintion
project(luacheck)

# ------------------------------------------------------------------------------
# Other settings
set(CMAKE_VERBOSE_MAKEFILE OFF)
# luacheck has nothing to build, only some files to install
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

# ------------------------------------------------------------------------------
# Setup GNU-alike installatin directories
include (GNUInstallDirs)
set(INSTALL_BINDIR
  ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_DOCDIR
  ${CMAKE_INSTALL_DOCDIR} CACHE PATH "Installation directory for documentation")
# Lua-specific installation dirs
set(INSTALL_TOP_LDIR
  ${LIBLUA_INSTALLDIR}/share/lua/${liblua_VERSION_MAJOR}.${liblua_VERSION_MINOR})

# ------------------------------------------------------------------------------
# Install luacheck files and docs
install(PROGRAMS bin/luacheck.lua DESTINATION ${INSTALL_BINDIR})
if(WIN32)
  # *.bat-launcher for Windows
  install(FILES bin/luacheck.bat DESTINATION ${INSTALL_BINDIR})
endif()
install(DIRECTORY src/ DESTINATION ${INSTALL_TOP_LDIR}
  FILES_MATCHING PATTERN "*.lua"
)
install(DIRECTORY doc-html/ DESTINATION ${INSTALL_DOCDIR})
