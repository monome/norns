--- Utility module;
-- @module util

util = {}

--- scan directory, return file list
-- @param directory path to directory
util.scandir = function(directory)
  local i, t, popen = 0, {}, io.popen
  local pfile = popen('ls -p --group-directories-first "'..directory..'"')
  for filename in pfile:lines() do
    i = i + 1
    t[i] = filename
  end
  pfile:close()
  return t
end

--- execute os command, capture output
-- @param cmd command
-- @param raw raw output (omit for scrubbed)
-- @return ouput
util.os_capture = function(cmd, raw)
  local f = assert(io.popen(cmd, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  if raw then return s end
  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  s = string.gsub(s, '[\n\r]+', ' ')
  return s
end

--- string begins with
-- @param s string to examine
-- @param start string to search for
-- @return true or false
util.string_starts = function(s,start)
  return string.sub(s,1,string.len(start))==start
end

--- clamp values to min max
-- @param n value
-- @param min minimum
-- @param max maximum
-- @return clamped value
util.clamp = function(n, min, max)
  return math.min(max,(math.max(n,min)))
end

return util
