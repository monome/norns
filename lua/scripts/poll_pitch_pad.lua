local norns = require 'norns'
local engine = require 'engine'
local poll = require 'poll'
local input = require 'input'

local state = {
   use_pitch = false,
   pitch_poll = nil
   gamepad = nil
}

local butCode = 'BTN_SOUTH'

local buttonCallback = function(butState)
   state.use_pitch = butState
end

--- callback from polling the pitch
-- @param hz - the detected input frequency
local hzCallback = function(hz)
   if state.use_pitch then engine.hz(hz) end
end

local didGetDevices = function(devices)
   -- FIXME: loop over devices here until we find one with btn_south
   -- here just assume there's only one device   
   gamepad = devices[1]
   if gamepad:hasCode(butCode) then
      gamepad:setCallback(butCode, buttonCallback)
   else
      print("warning: connected device doesn't appear to be a gamepad")
   end
end

--- connect a game[ad device
-- @param device: an Input table
local addGamePad = function(device)
   gamepad = device
end

local didLoadEngine = function(commands)
   -- get poll for label
   local p = poll.findByLabel('pitch_l')
   if p then
      -- start the poll with callback and period
      p:start(callback, 0.25) 
      -- store the poll in our global state so we can stop it later
      state.pitch_poll = p
   else
      print("warning: couldn't find requested poll label")
   end
end


-- load the desired engine with our callback
engine.load('TestSine', didLoadEngine)

norns.cleanup = function()
   state.pitch_poll:stop
   state.gamepad:unsetCallback(butCode)
   state = nil
   butCode = nil
end
