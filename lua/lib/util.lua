--- Utility module
-- @module util

util = {}

--- db to amp.
-- @return amp
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

--- check if file exists.
-- @param name filepath
-- @return true/false
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
-- @param path
util.make_dir = function(path)
  os.execute("mkdir -p " .. path)
end


--- execute os command, capture output.
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

--- string begins with.
-- @param s string to examine
-- @param start string to search for
-- @return true or false
util.string_starts = function(s,start)
  return string.sub(s,1,string.len(start))==start
end

--- clamp values to min max.
-- @param n value
-- @param min minimum
-- @param max maximum
-- @return clamped value
util.clamp = function(n, min, max)
  return math.min(max,(math.max(n,min)))
end

-- linlin, linexp, explin, expexp ripped from SC source code
-- https://github.com/supercollider/supercollider/blob/cca12ff02a774a9ea212e8883551d3565bb24a6f/lang/LangSource/MiscInlineMath.h

--- convert a linear range to an exponential range
-- @param slo lower limit of input range
-- @param shi upper limit of input range
-- @param dlo lower limit of output range (must be non-zero and of the same sign as dhi)
-- @param dhi upper limit of output range (must be non-zero and of the same sign as dlo)
-- @param f input to convert
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
-- @param slo lower limit of input range
-- @param shi upper limit of input range
-- @param dlo lower limit of output range
-- @param dhi upper limit of output range
-- @param f input to convert
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
-- @param slo lower limit of input range (must be non-zero and of the same sign as shi)
-- @param shi upper limit of input range (must be non-zero and of the same sign as slo)
-- @param dlo lower limit of output range
-- @param dhi upper limit of output range
-- @param f input to convert
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
-- @param slo lower limit of input range (must be non-zero and of the same sign as shi)
-- @param shi upper limit of input range (must be non-zero and of the same sign as slo)
-- @param dlo lower limit of output range (must be non-zero and of the same sign as dhi)
-- @param dhi upper limit of output range (must be non-zero and of the same sign as dlo)
-- @param f input to convert
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
-- @param number number to round
-- @param quant precision to round to
function util.round(number, quant)
  if quant == 0 then
    return number
  else
    return math.floor(number/(quant or 1) + 0.5) * (quant or 1)
  end
end

--- round up to a multiple of a number.
-- @param number number to round
-- @param quant precision to round to
function util.round_up(number, quant)
  if quant == 0 then
    return number
  else
    return math.ceil(number/(quant or 1) + 0.5) * (quant or 1)
  end
end

--- format string, seconds to h:m:s.
-- @param s seconds
-- @treturn string seconds : seconds in h:m:s
function util.s_to_hms(s)
  local m = math.floor(s/60)
  local h = math.floor(m/60)
  m = m%60
  s = s%60
  return h ..":".. string.format("%02d",m) ..":".. string.format("%02d",s)
end

return util
