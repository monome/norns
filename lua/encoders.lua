--- Encoders class
-- @module encoders

local encoders = {}

local time = function()
  m,s = get_time()
  return m + s/1000000
end 

local now = time()

encoders.tick = {0,0,0}
encoders.accel = {true,true,true}
encoders.sens = {1,1,1}
encoders.time = {now,now,now}
encoders.callback = norns.none

encoders.set_accel = function(n,z)
  if n == 0 then
    for n=1,3 do
      encoders.accel[n] = z
      encoders.tick[n] = 0
    end
  else
    encoders.accel[n] = z 
    encoders.tick[n] = 0
  end 
end

encoders.set_sens = function(n,s)
  if n == 0 then
    for n=1,3 do
      encoders.sens[n] = util.clamp(s,0.01,1)
      encoders.tick[n] = 0
    end
  else
    encoders.sens[n] = util.clamp(s,0.01,1)
    encoders.tick[n] = 0
  end 
end

encoders.process = function(n,d)
  now = time()
  local diff = now - encoders.time[n]
  encoders.time[n] = now

  if encoders.accel[n] then
    if diff < 0.005 then d = d*6
    elseif diff < 0.01 then d = d*4
    elseif diff < 0.02 then d = d*3
    elseif diff < 0.03 then d = d*2
    end
  end

  d = d * encoders.sens[n]

  encoders.tick[n] = encoders.tick[n] + d
  if math.abs(encoders.tick[n]) >= 1 then
    local delta = util.round(encoders.tick[n],1)
    encoders.callback(n,delta)
    encoders.tick[n] = encoders.tick[n] - delta
  end
end 

return encoders
