local check = require "luacheck.check"
local filter = require "luacheck.filter"
local options = require "luacheck.options"
local format = require "luacheck.format"
local utils = require "luacheck.utils"

local init = {}
local luacheck = {
   _VERSION = "1.1.2"
}

local function raw_validate_options(fname, opts, stds, context)
   local ok, err = options.validate(options.all_options, opts, stds)

   if not ok then
      if context then
         error(("bad argument #2 to '%s' (%s: %s)"):format(fname, context, err))
      else
         error(("bad argument #2 to '%s' (%s)"):format(fname, err))
      end
   end
end

local function validate_options(fname, items, opts, stds)
   raw_validate_options(fname, opts)

   if opts ~= nil then
      for i in ipairs(items) do
         raw_validate_options(fname, opts[i], stds, ("invalid options at index [%d]"):format(i))

         if opts[i] ~= nil then
            for j, nested_opts in ipairs(opts[i]) do
               raw_validate_options(fname, nested_opts, stds, ("invalid options at index [%d][%d]"):format(i, j))
            end
         end
      end
   end
end

-- Returns report for a string.
function init.get_report(src)
   local msg = ("bad argument #1 to 'init.get_report' (string expected, got %s)"):format(type(src))
   assert(type(src) == "string", msg)
   return check(src)
end

-- Applies options to reports. Reports with .fatal field are unchanged.
-- Options are applied to reports[i] in order: options, options[i], options[i][1], options[i][2], ...
-- Returns new array of reports, adds .warnings, .errors and .fatals fields to this array.
function init.process_reports(reports, opts, stds)
   local msg = ("bad argument #1 to 'init.process_reports' (table expected, got %s)"):format(type(reports))
   assert(type(reports) == "table", msg)
   validate_options("init.process_reports", reports, opts, stds)
   local report = filter.filter(reports, opts, stds)
   report.warnings = 0
   report.errors = 0
   report.fatals = 0

   for _, file_report in ipairs(report) do
      if file_report.fatal then
         report.fatals = report.fatals + 1
      else
         for _, event in ipairs(file_report) do
            if event.code:sub(1, 1) == "0" then
               report.errors = report.errors + 1
            else
               report.warnings = report.warnings + 1
            end
         end
      end
   end

   return report
end

-- Checks strings with options, returns report.
-- Tables with .fatal field are unchanged.
function init.check_strings(srcs, opts)
   local msg = ("bad argument #1 to 'init.check_strings' (table expected, got %s)"):format(type(srcs))
   assert(type(srcs) == "table", msg)

   for _, item in ipairs(srcs) do
      msg = ("bad argument #1 to 'init.check_strings' (array of strings or tables expected, got %s)"):format(
         type(item))
      assert(type(item) == "string" or type(item) == "table", msg)
   end

   validate_options("init.check_strings", srcs, opts)

   local reports = {}

   for i, src in ipairs(srcs) do
      if type(src) == "table" and src.fatal then
         reports[i] = src
      else
         reports[i] = init.get_report(src)
      end
   end

   return init.process_reports(reports, opts)
end

function init.check_files(files, opts)
   local msg = ("bad argument #1 to 'init.check_files' (table expected, got %s)"):format(type(files))
   assert(type(files) == "table", msg)

   for _, item in ipairs(files) do
      msg = ("bad argument #1 to 'init.check_files' (array of paths or file handles expected, got %s)"):format(
         type(item))
      assert(type(item) == "string" or io.type(item) == "file", msg
      )
   end

   validate_options("init.check_files", files, opts)

   local srcs = {}

   for i, file in ipairs(files) do
      local src, err = utils.read_file(file)
      srcs[i] = src or {fatal = "I/O", msg = err}
   end

   return init.check_strings(srcs, opts)
end

function init.get_message(issue)
   local msg = ("bad argument #1 to 'init.get_message' (table expected, got %s)"):format(type(issue))
   assert(type(issue) == "table", msg)
   return format.get_message(issue)
end

setmetatable(init, {__call = function(_, ...)
   return init.check_files(...)
end})

return init
