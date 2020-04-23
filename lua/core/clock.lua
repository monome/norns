--- clock coroutines
-- @module clock

local clock = {}

clock.threads = {}

local clock_id_counter = 1
local function new_id()
  local id = clock_id_counter
  clock_id_counter = clock_id_counter + 1
  return id
end

--- create a coroutine to run but do not immediately run it;
-- @tparam function f
-- @treturn integer : coroutine ID that can be used to resume/stop it later
clock.create = function(f)
  local coro = coroutine.create(f)
  local coro_id = new_id()
  clock.threads[coro_id] = coro
  return coro_id
end

--- create a coroutine from the given function and immediately run it;
-- the function parameter is a task that will suspend when clock.sleep and clock.sync are called inside it and will wake up again after specified time.
-- @tparam function f
-- @treturn integer : coroutine ID that can be used to stop it later
clock.run = function(f, ...)
  local coro_id = clock.create(f)
  clock.resume(coro_id, ...)
  return coro_id
end

--- stop execution of a coroutine started using clock.run.
-- @tparam integer coro_id : coroutine ID
clock.cancel = function(coro_id)
  _norns.clock_cancel(coro_id)
  clock.threads[coro_id] = nil
end

local SLEEP = 0
local SYNC = 1
local SUSPEND = 2

--- yield and schedule waking up the coroutine in s seconds;
-- must be called from within a coroutine started with clock.run.
-- @tparam float s : seconds
clock.sleep = function(...)
  return coroutine.yield(SLEEP, ...)
end

--- yield and schedule waking up the coroutine at beats beat;
-- the coroutine will suspend for the time required to reach the given fraction of a beat;
-- must be called from within a coroutine started with clock.run.
-- @tparam float beats : next fraction of a beat at which the coroutine will be resumed. may be larger than 1.
clock.sync = function(...)
  return coroutine.yield(SYNC, ...)
end

--- yield and do not schedule wake up, clock must be explicitly resumed
-- must be called from within a coroutine started with clock.run.
clock.suspend = function()
  return coroutine.yield(SUSPEND)
end

-- todo: use c api instead
clock.resume = function(coro_id, ...)
  local coro = clock.threads[coro_id]

  if coro == nil then
    print('clock: ignoring resumption of canceled clock (no coroutine)')
    return
  end

  local result, mode, time = coroutine.resume(clock.threads[coro_id], ...)

  if coroutine.status(coro) == "dead" then
    if result then
      clock.cancel(coro_id)
    else
      error(mode)
    end
  else
    -- not dead
    if result and mode ~= nil then
      if mode == SLEEP then
        _norns.clock_schedule_sleep(coro_id, time)
      elseif mode == SYNC then
        _norns.clock_schedule_sync(coro_id, time)
      elseif mode == SUSPEND then
        -- nothing needed for SUSPEND
      end
    end
  end
end


clock.cleanup = function()
  for id, coro in pairs(clock.threads) do
    if coro then
      clock.cancel(id)
    end
  end

  clock.transport.start = nil
  clock.transport.stop = nil
end

--- select the sync source
-- @tparam string source : "internal", "midi", or "link"
clock.set_source = function(source)
  if type(source) == "number" then
    _norns.clock_set_source(util.clamp(source-1,0,3)) -- lua list is 1-indexed
  elseif source == "internal" then
    _norns.clock_set_source(0)
  elseif source == "midi" then
    _norns.clock_set_source(1)
  elseif source == "link" then
    _norns.clock_set_source(2)
  else
    error("unknown clock source: "..source)
  end
end


clock.get_beats = function()
  return _norns.clock_get_time_beats()
end

clock.get_tempo = function()
  return _norns.clock_get_tempo()
end

clock.get_beat_sec = function(x)
  x = x or 1
  return 60.0 / clock.get_tempo() * x
end


clock.transport = {}

clock.transport.start = nil
clock.transport.stop = nil


clock.internal = {}

clock.internal.set_tempo = function(bpm)
  return _norns.clock_internal_set_tempo(bpm)
end

clock.internal.start = function(beat)
  beat = beat or 0
  return _norns.clock_internal_start(beat)
end

clock.internal.stop = function()
  return _norns.clock_internal_stop()
end

clock.crow = {}

clock.crow.in_div = function(div)
  _norns.clock_crow_in_div(div)
end


clock.midi = {}


clock.link = {}

clock.link.set_tempo = function(bpm)
  return _norns.clock_link_set_tempo(bpm)
end

clock.link.set_quantum = function(quantum)
  return _norns.clock_link_set_quantum(quantum)
end


_norns.clock.start = function()
  if clock.transport.start ~= nil then
    clock.transport.start()
  end
end

_norns.clock.stop = function()
  if clock.transport.stop ~= nil then
    clock.transport.stop()
  end
end


function clock.add_params()
  params:add_group("CLOCK",8)

  params:add_option("clock_source", "source", {"internal", "midi", "link", "crow"},
    norns.state.clock.source)
  params:set_action("clock_source",
    function(x)
      clock.set_source(x)
      if x==4 then
        crow.input[1].change = function() end
        crow.input[1].mode("change",2,0.1,"rising")
      end
      norns.state.clock.source = x
      if x==1 then clock.internal.set_tempo(params:get("clock_tempo"))
      elseif x==3 then clock.link.set_tempo(params:get("clock_tempo")) end
    end)
  params:set_save("clock_source", false)
  params:add_number("clock_tempo", "tempo", 1, 300, norns.state.clock.tempo)
  params:set_action("clock_tempo",
    function(bpm)
      local source = params:string("clock_source")
      if source == "internal" then clock.internal.set_tempo(bpm)
      elseif source == "link" then clock.link.set_tempo(bpm) end
      norns.state.clock.tempo = bpm
    end)
  params:set_save("clock_tempo", false)
  params:add_trigger("clock_reset", "reset")
  params:set_action("clock_reset",
    function()
      local source = params:string("clock_source")
      if source == "internal" then clock.internal.start(bpm)
      elseif source == "link" then print("link reset not supported") end
    end)
  params:add_number("link_quantum", "link quantum", 1, 32, norns.state.clock.link_quantum)
  params:set_action("link_quantum",
    function(x)
      clock.link.set_quantum(x)
      norns.state.clock.link_quantum = x
    end)
  params:set_save("link_quantum", false)

  params:add_option("clock_midi_out", "midi out",
      {"off", "port 1", "port 2", "port 3", "port 4"}, norns.state.clock.midi_out)
  params:set_action("clock_midi_out", function(x) norns.state.clock.midi_out = x end)
  params:set_save("clock_midi_out", false)
  params:add_option("clock_crow_out", "crow out",
      {"off", "output 1", "output 2", "output 3", "output 4"}, norns.state.clock.crow_out)
  params:set_action("clock_crow_out", function(x)
      if x>1 then crow.output[x-1].action = "pulse(0.05,8)" end
      norns.state.clock.crow_out = x
    end)
  params:set_save("clock_crow_out", false)
  params:add_number("clock_crow_out_div", "crow out div", 1, 32,
    norns.state.clock.crow_out_div)
  params:set_action("clock_crow_out_div",
    function(x) norns.state.clock.crow_out_div = x end)
  params:set_save("clock_crow_out_div", false)
  params:add_number("clock_crow_in_div", "crow in div", 1, 32,
    norns.state.clock.crow_in_div)
  params:set_action("clock_crow_in_div",
    function(x)
      clock.crow.in_div(x)
      norns.state.clock.crow_in_div = x
    end)
  params:set_save("clock_crow_in_div", false)
  --params:add_trigger("crow_clear", "crow clear")
  --params:set_action("crow_clear",
    --function() crow.reset() crow.clear() end)

  params:bang("clock_tempo")

  -- executes crow sync
  clock.run(function()
    while true do
      clock.sync(1/params:get("clock_crow_out_div"))
      local crow_out = params:get("clock_crow_out")-1
      if crow_out > 0 then crow.output[crow_out]() end
    end
  end)

  -- executes midi out (needs a subtick)
  -- FIXME: lots of if's every tick blah
  clock.run(function()
    while true do
      clock.sync(1/24)
      local midi_out = params:get("clock_midi_out")-1
      if midi_out > 0 then
        if midi.vports[midi_out].name ~= "none" then
          midi.vports[midi_out]:clock()
        end
      end
    end
  end)

  -- update tempo param value
  clock.run(function()
    while true do
      if params:get("clock_source") ~= 1 then
        local external_tempo = math.floor(clock.get_tempo() + 0.5)
        params:set("clock_tempo", external_tempo, true)
      end

      clock.sleep(1)
    end
  end)

end


return clock
