--- encoders
--
-- The [norns script reference](https://monome.org/docs/norns/reference/)
-- has [examples for this module](https://monome.org/docs/norns/reference/encoders).
--
-- @module encoders

local encoders = {}

local util = require 'util'

local now = util.time()

encoders.tick = {0,0,0}
encoders.accel = {true,true,true}
encoders.sens = {1,1,1}
encoders.time = {now,now,now}
encoders.callback = norns.none

--- set acceleration
encoders.set_accel = function(n,z)
  if n == 0 then
    for k=1,3 do
      encoders.accel[k] = z
      encoders.tick[k] = 0
    end
  else
    encoders.accel[n] = z
    encoders.tick[n] = 0
  end
end

--- set sensitivity
encoders.set_sens = function(n,s)
  if n == 0 then
    for k=1,3 do
      encoders.sens[k] = util.clamp(s,1,16)
      encoders.tick[k] = 0
    end
  else
    encoders.sens[n] = util.clamp(s,1,16)
    encoders.tick[n] = 0
  end
end

--- process delta
encoders.process = function(n,d)
  encoders.tick[n] = encoders.tick[n] + d
  if encoders.accel[n] then
		local s = d<0 and -1 or 1
		d = s * math.pow(math.abs(d),1.6)
  end
  if math.abs(encoders.tick[n]) >= encoders.sens[n] then
    local val = encoders.tick[n] / encoders.sens[n]
		val = (val > 0) and math.floor(val) or math.ceil(val)
    encoders.callback(n,val)
    encoders.tick[n] = math.fmod(encoders.tick[n],encoders.sens[n])
    screen.ping()
	end
end

--- process delta, selected if _norns.adc_rev() == 0 in norns.lua
encoders.process_with_accel = function(n,d)
  now = util.time()
  local diff = now - encoders.time[n]
  encoders.time[n] = now
  if encoders.accel[n] then
    if diff < 0.005 then d = d*6
    elseif diff < 0.01 then d = d*4
    elseif diff < 0.02 then d = d*3
    elseif diff < 0.03 then d = d*2
    end
  end
  encoders.tick[n] = encoders.tick[n] + d
  if math.abs(encoders.tick[n]) >= encoders.sens[n] then
    local val = encoders.tick[n] / encoders.sens[n]
		val = (val > 0) and math.floor(val) or math.ceil(val)
    encoders.callback(n,val)
    encoders.tick[n] = math.fmod(encoders.tick[n],encoders.sens[n])
    screen.ping()
  end
end



-- script state

local accel = {true,true,true}
local sens = {2,2,2}

norns.enc = {}
norns.enc.accel = function(n,z)
  if n == 0 then
    for k=1,3 do
      accel[k] = z
    end
  else
    accel[n] = z
  end
  if(_menu.mode == false) then norns.encoders.set_accel(n,z) end
end

norns.enc.sens = function(n,s)
  if n == 0 then
    for k=1,3 do
      sens[k] = util.clamp(s,1,16)
    end
  else
    sens[n] = util.clamp(s,1,16)
  end
  if(_menu.mode == false) then norns.encoders.set_sens(n,s) end
end

norns.enc.resume = function()
  for n=1,3 do
    norns.encoders.set_accel(n,accel[n])
    norns.encoders.set_sens(n,sens[n])
  end
end


return encoders
