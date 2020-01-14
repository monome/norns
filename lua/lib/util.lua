--- Utility module
-- @module util

util = {}

--- db to amp.
-- @tparam number db
-- @treturn number amp
util.dbamp = function(db)
  if db < -80 then db = -math.huge end
  return math.pow(10,db*0.05)
end

--- get system time in s+us.
-- @return time
util.time = function()
  local us,s = _norns.get_time()
  return us + s/1000000
end


--- scan directory, return file list.
-- @tparam string directory path to directory
-- @treturn table
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

--- check if file exists.
-- @tparam string name filepath
-- @treturn boolean true/false
util.file_exists = function(name)
  local f=io.open(name,"r")
  if f~=nil then
    io.close(f)
    return true
  else
    return false
  end
end

--- make directory (with parents as needed).
-- @tparam string path
util.make_dir = function(path)
  os.execute("mkdir -p " .. path)
end


--- execute os command, capture output.
-- @tparam string cmd command
-- @param raw raw output (omit for scrubbed)
-- @return output
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

--- string begins with.
-- @tparam string s string to examine
-- @tparam string start string to search for
-- @treturn boolean true or false
util.string_starts = function(s,start)
  return string.sub(s,1,string.len(start))==start
end

--- trim string to a display width
-- @tparam string s string to trim
-- @tparam number width maximum width
-- @treturn string trimmed string
util.trim_string_to_width = function(s, width)
  local first_iter = true
  while _norns.screen_extents(s) > width do
    if first_iter then
      s = s .. "..."
      first_iter = false
    end
    s = string.sub(s, 1, string.len(s) - 4)
    s = string.gsub(s, "[%p%s]+$", "") .. "..."
  end
  return s
end

--- clamp values to min max.
-- @tparam number n value
-- @tparam number min minimum
-- @tparam number max maximum
-- @treturn number clamped value
util.clamp = function(n, min, max)
  return math.min(max,(math.max(n,min)))
end

-- linlin, linexp, explin, expexp ripped from SC source code
-- https://github.com/supercollider/supercollider/blob/cca12ff02a774a9ea212e8883551d3565bb24a6f/lang/LangSource/MiscInlineMath.h

--- convert a linear range to an exponential range
-- @tparam number slo lower limit of input range
-- @tparam number shi upper limit of input range
-- @tparam number dlo lower limit of output range (must be non-zero and of the same sign as dhi)
-- @tparam number dhi upper limit of output range (must be non-zero and of the same sign as dlo)
-- @tparam number f input to convert
-- @treturn number
function util.linexp(slo, shi, dlo, dhi, f)
  if f <= slo then
    return dlo
  elseif f >= shi then
    return dhi
  else
    return math.pow( dhi/dlo, (f-slo) / (shi-slo) ) * dlo
  end
end

--- map a linear range to another linear range.
-- @tparam number slo lower limit of input range
-- @tparam number shi upper limit of input range
-- @tparam number dlo lower limit of output range
-- @tparam number dhi upper limit of output range
-- @tparam number f input to convert
-- @treturn number
function util.linlin(slo, shi, dlo, dhi, f)
  if f <= slo then
    return dlo
  elseif f >= shi then
    return dhi
  else
    return (f-slo) / (shi-slo) * (dhi-dlo) + dlo
  end
end

--- convert an exponential range to a linear range.
-- @tparam number slo lower limit of input range (must be non-zero and of the same sign as shi)
-- @tparam number shi upper limit of input range (must be non-zero and of the same sign as slo)
-- @tparam number dlo lower limit of output range
-- @tparam number dhi upper limit of output range
-- @tparam number f input to convert
-- @treturn number
function util.explin(slo, shi, dlo, dhi, f)
  if f <= slo then
    return dlo
  elseif f >= shi then
    return dhi
  else
    return math.log(f/slo) / math.log(shi/slo) * (dhi-dlo) + dlo
  end
end

--- map an exponential range to another exponential range.
-- @tparam number slo lower limit of input range (must be non-zero and of the same sign as shi)
-- @tparam number shi upper limit of input range (must be non-zero and of the same sign as slo)
-- @tparam number dlo lower limit of output range (must be non-zero and of the same sign as dhi)
-- @tparam number dhi upper limit of output range (must be non-zero and of the same sign as dlo)
-- @tparam number f input to convert
-- @treturn number
function util.expexp(slo, shi, dlo, dhi, f)
  if f <= slo then
    return dlo
  elseif f >= shi then
    return dhi
  else
    return math.pow(dhi/dlo, math.log(f/slo) / math.log(shi/slo)) * dlo
  end
end

--- round to a multiple of a number
-- @tparam number number to round
-- @tparam number quant precision to round to
function util.round(number, quant)
  if quant == 0 then
    return number
  else
    return math.floor(number/(quant or 1) + 0.5) * (quant or 1)
  end
end

--- round up to a multiple of a number.
-- @tparam number number to round
-- @tparam number quant precision to round to
function util.round_up(number, quant)
  if quant == 0 then
    return number
  else
    return math.ceil(number/(quant or 1) + 0.5) * (quant or 1)
  end
end

--- format string, seconds to h:m:s.
-- @tparam number s seconds
-- @treturn string seconds : seconds in h:m:s
function util.s_to_hms(s)
  local m = math.floor(s/60)
  local h = math.floor(m/60)
  m = m%60
  s = s%60
  return h ..":".. string.format("%02d",m) ..":".. string.format("%02d",s)
end

return util
