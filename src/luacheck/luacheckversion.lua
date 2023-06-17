-- Moved _VERSION into separate module in order to prevent C Stack Overflows
-- with Lua 5.4 on Windows.
local luacheckversion = {
   _VERSION = "1.1.1"
}
return luacheckversion
