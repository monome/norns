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

--- create and start a coroutine using the norns clock scheduler.
-- @tparam function f coroutine body function
-- @param[opt] ... any extra arguments will be passed to the body function
-- @treturn integer coroutine handle that can be used with clock.cancel
-- @see clock.cancel
clock.run = function(f, ...)
  local coro = coroutine.create(f)
  local coro_id = new_id()
  clock.threads[coro_id] = coro
  clock.resume(coro_id, ...)
  return coro_id
end

--- stop execution of a coroutine previously started using clock.run.
-- @tparam integer coro_id coroutine handle
-- @see clock.run
clock.cancel = function(coro_id)
  _norns.clock_cancel(coro_id)
  clock.threads[coro_id] = nil
end

local SCHEDULE_SLEEP = 0
local SCHEDULE_SYNC = 1

--- suspend execution of the calling coroutine and schedule resuming in specified time.
-- must be called from a coroutine previously started using clock.run.
-- @tparam float s seconds to wait for
clock.sleep = function(...)
  return coroutine.yield(SCHEDULE_SLEEP, ...)
end

--- suspend execution of the calling coroutine and schedule resuming at the next sync quantum of the specified value.
-- must be called from a coroutine previously started using clock.run.
-- @tparam float beat sync quantum. may be larger than 1
-- @tparam[opt] float offset if set, this value will be added to the sync quantum
clock.sync = function(...)
  return coroutine.yield(SCHEDULE_SYNC, ...)
end

-- todo: use c api instead
clock.resume = function(coro_id, ...)
  local coro = clock.threads[coro_id]

  local result, mode, time, offset = coroutine.resume(coro, ...)

  if coroutine.status(coro) == "dead" then
    if result then
      clock.cancel(coro_id)
    else
      error(mode)
    end
  else
    -- not dead
    if result and mode ~= nil then
      if mode == SCHEDULE_SLEEP then
        _norns.clock_schedule_sleep(coro_id, time)
      elseif mode == SCHEDULE_SYNC then
        _norns.clock_schedule_sync(coro_id, time, offset)
      else
        error('invalid clock scheduler mode')
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
  clock.tempo_change_handler = nil
end

--- select the sync source.
-- @tparam string source "internal", "midi", or "link"
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

--- get current time in beats.
clock.get_beats = function()
  return _norns.clock_get_time_beats()
end

--- get current tempo.
clock.get_tempo = function()
  return _norns.clock_get_tempo()
end

--- get length of a single beat at current tempo in seconds.
clock.get_beat_sec = function()
  return 60.0 / clock.get_tempo()
end


clock.transport = {}

--- static callback when clock transport is started;
-- user scripts can redefine
-- @static
clock.transport.start = nil

--- static callback when clock transport is stopped;
-- user scripts can redefine
-- @static
clock.transport.stop = nil

--- static callback when clock tempo is adjusted via PARAMETERS > CLOCK > tempo;
-- user scripts can redefine
-- @static
-- @param bpm : the new tempo
clock.tempo_change_handler = nil

clock.internal = {}

clock.internal.set_tempo = function(bpm)
  return _norns.clock_internal_set_tempo(bpm)
end

clock.internal.start = function()
  return _norns.clock_internal_start()
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

clock.link.start = function()
  return _norns.clock_link_set_transport_start()
end

clock.link.stop = function()
  return _norns.clock_link_set_transport_stop()
end

clock.link.set_start_stop_sync = function(enabled)
  return _norns.clock_link_set_start_stop_sync(enabled)
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
  local send_midi_clock = {}
  params:add_group("CLOCK", 27)

  params:add_option("clock_source", "source", {"internal", "midi", "link", "crow"},
    norns.state.clock.source)
  params:set_action("clock_source",
    function(x)
      if x==3 then clock.link.set_tempo(params:get("clock_tempo")) end -- for link, apply tempo before setting source
      clock.set_source(x)
      if x==4 then
        norns.crow.clock_enable()
      end
      norns.state.clock.source = x
      if x==1 then clock.internal.set_tempo(params:get("clock_tempo")) end
    end)
  params:set_save("clock_source", false)
  params:add_number("clock_tempo", "tempo", 1, 300, norns.state.clock.tempo)
  params:set_action("clock_tempo",
    function(bpm)
      local source = params:string("clock_source")
      if source == "internal" then clock.internal.set_tempo(bpm)
      elseif source == "link" then clock.link.set_tempo(bpm) end
      norns.state.clock.tempo = bpm
      if clock.tempo_change_handler ~= nil then
        clock.tempo_change_handler(bpm)
      end
    end)
  params:set_save("clock_tempo", false)
  params:add_trigger("clock_reset", "reset")
  params:set_action("clock_reset",
    function()
      local source = params:string("clock_source")
      if source == "internal" then clock.internal.start()
      elseif source == "link" then print("link reset not supported") end
    end)
  params:add_separator("link_separator", "link")
  params:add_number("link_quantum", "link quantum", 1, 32, norns.state.clock.link_quantum)
  params:set_action("link_quantum",
    function(x)
      clock.link.set_quantum(x)
      norns.state.clock.link_quantum = x
    end)
  params:set_save("link_quantum", false)
  params:add_option("link_start_stop_sync", "link start/stop sync", {"disabled", "enabled"}, norns.state.clock.link_start_stop_sync)
  params:set_action("link_start_stop_sync",
    function(x)
      clock.link.set_start_stop_sync(x == 2)
      norns.state.clock.link_start_stop_sync = x
    end)
  params:set_save("link_start_stop_sync", false)
  params:add_separator("midi_clock_out_separator", "midi clock out")
  for i = 1,16 do
    local short_name = string.len(midi.vports[i].name) <= 20 and midi.vports[i].name or util.acronym(midi.vports[i].name)
    params:add_binary("clock_midi_out_"..i, i..". "..short_name, "toggle", norns.state.clock.midi_out[i])
    params:set_action("clock_midi_out_"..i,
      function(x)
        if x == 1 then
          table.insert(send_midi_clock,i)
        else
          if tab.contains(send_midi_clock,i) then
            table.remove(send_midi_clock,tab.key(send_midi_clock, i))
          end
        end
        norns.state.clock.midi_out[i] = x
      end
    )
    if short_name ~= "none" and midi.vports[i].connected then
      params:show("clock_midi_out_"..i)
    else
      params:hide("clock_midi_out_"..i)
    end
    params:set_save("clock_midi_out_"..i, false)
  end
  params:add_separator("crow_clock_separator", "crow")
  params:add_option("clock_crow_out", "crow out",
      {"off", "output 1", "output 2", "output 3", "output 4"}, norns.state.clock.crow_out)
  params:set_action("clock_crow_out", function(x)
      --if x>1 then crow.output[x-1].action = "pulse(0.01,8)" end
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
      if crow_out > 0 then
        crow.output[crow_out].volts = 10
        clock.sleep(60/(2*clock.get_tempo()*params:get("clock_crow_out_div")))
        crow.output[crow_out].volts = 0
      end
    end
  end)

  -- executes midi out (needs a subtick)
  -- FIXME: lots of if's every tick blah
  clock.run(function()
    while true do
      clock.sync(1/24)
      for i = 1,#send_midi_clock do
        local port = send_midi_clock[i]
        midi.vports[port]:clock()
      end
    end
  end)

  -- update tempo param value
  clock.run(function()
    while true do
      if params:get("clock_source") ~= 1 then
        local external_tempo = math.floor(clock.get_tempo() + 0.5)
        local previous_val = params:get("clock_tempo")
        params:set("clock_tempo", external_tempo, true)
        if clock.tempo_change_handler ~= nil and previous_val ~= external_tempo then
          clock.tempo_change_handler(external_tempo)
        end
      end

      clock.sleep(1)
    end
  end)

end


clock.help = [[
--------------------------------------------------------------------------------
clock.run( func )             start a new coroutine with function [func]
                              (returns) created id
clock.cancel( id )            cancel coroutine [id]
clock.sleep( time )           resume in [time] seconds
clock.sync( beats )           resume at next sync quantum of value [beats]
                                following to global tempo
clock.get_beats()             (returns) current time in beats
clock.get_tempo()             (returns) current tempo
clock.get_beat_sec()          (returns) length of a single beat at current
                                tempo in seconds
--------------------------------------------------------------------------------
-- example

-- start a clock with calling function [loop]
function init()
  clock.run(loop)
end

-- this function loops forever, printing at 1 second intervals 
function loop()
  while true do
    print("so true")
    clock.sleep(1)
  end
end
--------------------------------------------------------------------------------
]]

return clock