/**
 * @file luacheck.cpp
 * @brief Standalone executable launcher for the Luacheck static linter tool.
 * 
 * This launcher embeds the main 'luacheck.lua' script as a hex-encoded byte array 
 * (generated via CMake) to eliminate the need for an external script file in the 
 * binary directory. 
 * 
 * Key Features:
 * - Portable Execution: Resolves the installation prefix dynamically relative
 *   to the EXE location.
 * - Environment Setup: Automatically configures Lua's 'package.path' and
 *   'package.cpath' using a custom C++/Lua bridge to find shared libraries and
 *   modules in the system-independent 'share' and 'lib' directories.
 * - Encoding Safety: Uses UTF-8 conversion for Windows WideChar paths to ensure 
 *   compatibility with the Lua interpreter.
 *
 * -----------------------------------------------------------------------------
 * MIT License
 * 
 * Copyright (c) 2026 The OneLuaPro project authors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <string>
#include <vector>

#include <lua.hpp>
#include "luacheck_source.h"

#define appName "luacheck.exe"

static std::string WideCharToUTF8(LPCWSTR text) {
  if (!text) return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
  if (size_needed <= 0) return std::string();
  std::vector<char> buffer(size_needed);
  WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), size_needed, NULL, NULL);
  return std::string(buffer.data());
}

static const char *setPaths = R"(
function setPaths(basePath, paths, cpaths)
   local cleanBase = basePath:gsub("\\+$", "")
   local function process(list)
      local result = {}
      local isRelative = false
      for _, v in ipairs(list) do
	 if v == "<RELATIVE>" then
	    isRelative = true
	 else
	    local entry = v:gsub("^\\+", ""):gsub("\\+$", "")
	    if isRelative then
	       table.insert(result, entry)
	    else
	       table.insert(result, cleanBase .. "\\" .. entry)
	    end
	 end
      end
      return table.concat(result, ";")
   end
   package.path = process(paths)
   package.cpath = process(cpaths)
end
)";

static const char* LUA_PATHS[] = {
  R"(bin\lua\?.lua)",
  R"(bin\lua\?\init.lua)",
  R"(bin\?.lua)",
  R"(bin\?\init.lua)",
  "share\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?.lua",
  "share\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?\\init.lua",
  "<RELATIVE>",		// Sentinel, relative paths from here
  R"(.\?.lua)",
  R"(.\?\init.lua)",
  NULL			// End of List
};

static const char* LUA_CPATHS[] = {
  R"(bin\?.dll)",
  "lib\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?.dll",
  R"(bin\loadall.dll)",
  "<RELATIVE>",		// Sentinel, relative paths from here
  R"(.\?.dll)",
  NULL			// End of List
};

int main(int argc, char** argv) {
  // Determine path, where appName is currently located
  WCHAR installPrefix[PATHCCH_MAX_CCH];
  if (!GetModuleFileNameW(NULL, installPrefix, PATHCCH_MAX_CCH)) {
    fprintf(stderr, "%s: Could not find executable path.\n", appName);
    return 1;
  }

  // Navigate two levels up from <INSTALL_PREFIX>/bin/luacheck.exe to <INSTALL_PREFIX>
  PathCchRemoveFileSpec(installPrefix, PATHCCH_MAX_CCH);
  PathCchRemoveFileSpec(installPrefix, PATHCCH_MAX_CCH);
  std::string utf8Prefix = WideCharToUTF8(installPrefix);

  // Create new Lua state
  lua_State *L = luaL_newstate();
  if (!L) {
    fprintf(stderr, "%s: Failed to create Lua state.\n", appName);
    return 1;
  }

  // Open standard libs
  luaL_openlibs(L);

  // hand-over command-line args to lua by setting global table arg
  lua_newtable(L);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");

  // Globally register function setPaths()
  luaL_dostring(L, setPaths);

  // Putsh function on stack
  lua_getglobal(L, "setPaths");

  // Push 1st arg (the INSTALL_PREFIX)
  lua_pushstring(L, utf8Prefix.c_str());

  // Push 2nd arg (table of paths for package.path)
  lua_newtable(L);
  for (int i = 0; LUA_PATHS[i] != NULL; ++i) {
    lua_pushstring(L, LUA_PATHS[i]);
    lua_rawseti(L, -2, i + 1);
  }

  // Push 3rd arg (table of paths for package.cpath)
  lua_newtable(L);
  for (int i = 0; LUA_CPATHS[i] != NULL; ++i) {
    lua_pushstring(L, LUA_CPATHS[i]);
    lua_rawseti(L, -2, i + 1);
  }

  // Run function
  if (lua_pcall(L, 3, 0, 0) != 0) {
    fprintf(stderr, "%s: Error setting paths: %s\n", appName, lua_tostring(L, -1));
  }
  // Lua state now fully initialized with all standard search paths

  // Use loadbuffer, because it works with byte-arrays ans sizes
  if (luaL_loadbuffer(L, (const char*)luacheck_source_bytes,
		      luacheck_source_size, "@luacheck.lua") == LUA_OK) {
    // Execute the chunk
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
      fprintf(stderr, "%s: Runtime error: %s\n", appName, lua_tostring(L, -1));
    }
  }
  else {
    fprintf(stderr, "%s: Syntax error in embedded code: %s\n", appName, lua_tostring(L, -1));
  }

  lua_close(L);
  return 0;
}
