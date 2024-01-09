-- LFOs for general-purpose scripting
-- @module lib.lfo
-- inspired by contributions from @markwheeler (changes), @justmat (hnds), and @sixolet (toolkit)
-- added by @dndrks + @sixolet, with improvements by @Dewb and @sonoCircuit

local lattice = require 'lattice'

local LFO = {}
LFO.__index = LFO

local lfo_rates = {1/16,1/8,1/4,5/16,1/3,3/8,1/2,3/4,1,1.5,2,3,4,6,8,16,32,64,128,256,512,1024}
local lfo_rates_as_strings = {"1/16","1/8","1/4","5/16","1/3","3/8","1/2","3/4","1","1.5","2","3","4","6","8","16","32","64","128","256","512","1024"}
local lfo_shapes = {'sine','tri','square','random','up','down'}

local beat_sec = clock.get_beat_sec()

local params_per_entry = 15

function LFO.init()
  if norns.lfo == nil then
    norns.lfo = {lattice = lattice:new()}
  end
end

local function scale_lfo(target)
  if target.baseline == 'min' then
    target.scaled_min = target.min
    target.scaled_max = target.min + target.percentage
    target.mid = util.linlin(target.min,target.max,target.scaled_min,target.scaled_max,(target.min+target.max)/2)
  elseif target.baseline == 'center' then
    target.mid = (target.min+target.max)/2
    local centroid_mid = math.abs(target.min-target.max) * (target.depth/2)
    target.scaled_min = util.clamp(target.mid - centroid_mid,target.min,target.max)
    target.scaled_max = util.clamp(target.mid + centroid_mid,target.min,target.max)
  elseif target.baseline == 'max' then
    target.mid = (target.min+target.max)/2
    target.scaled_min = target.max * (1-(target.depth))
    target.scaled_max = target.max
    target.mid = math.abs(util.linlin(target.min,target.max,target.scaled_min,target.scaled_max,target.mid))
  end
end

--- construct an LFO
-- @param string shape The shape for this LFO (options: 'sine', 'tri', 'up', 'down', 'square', 'random'; default: 'sine')
-- @param number min The minimum bound for this LFO (default: 0)
-- @param number max The maximum bound for this LFO (default: 1)
-- @param number depth The depth of modulation between min/max (range: 0.0 to 1.0; default: 0.0)
-- @param string mode How to advance the LFO (options: 'clocked', 'free'; default: 'clocked')
-- @param number period The timing of this LFO's advancement. If mode is 'clocked', argument is in beats. If mode is 'free', argument is in seconds.
-- @param function action A callback function to perform as the LFO advances. This library passes both the scaled and the raw value to the callback function.
-- @param number phase The phase shift amount for this LFO (range: 0.0 to 1.0,; default: 0)
-- @param string baseline From where the LFO should start (options: 'min', 'center', 'max'; default: 'min')
function LFO.new(shape, min, max, depth, mode, period, action, phase, baseline)  
  local i = {}
  setmetatable(i, LFO)
  i.init()
  i.scaled = 0
  i.raw = 0
  i.phase_counter = 0
  if shape == 'saw' then
    shape = 'up'
  end
  i.shape = shape == nil and 'sine' or shape
  i.min = min == nil and 0 or min
  i.max = max == nil and 1 or max
  i.depth = depth == nil and 0 or depth
  i.enabled = 0
  i.mode = mode == nil and 'clocked' or mode
  i.period = period == nil and 4 or period
  i.reset_target = 'floor'
  i.baseline = baseline == nil and 'min' or baseline
  i.offset = 0
  i.ppqn = 96
  i.controlspec = {
    warp = 'linear',
    step = 0.01,
    units = '',
    quantum = 0.01,
    wrap = false,
    formatter = nil
  }
  i.action = action == nil and (function(scaled, raw) end) or action
  i.percentage = math.abs(i.min-i.max) * i.depth
  i.scaled_min = i.min
  i.scaled_max = i.max
  i.mid = 0
  i.rand_value = 0
  i.phase = phase == nil and 0 or phase
  scale_lfo(i)
  return i
end

--- construct an LFO via table arguments
-- eg. my_lfo:add{shape = 'sine', min = 200, max = 12000}
-- @tparam string shape The shape for this LFO (options: 'sine', 'tri', 'up', 'down', 'square', 'random'; default: 'sine')
-- @tparam number min The minimum bound for this LFO (default: 0)
-- @tparam number max The maximum bound for this LFO (default: 1)
-- @tparam number depth The depth of modulation between min/max (range: 0.0 to 1.0; default: 0.0)
-- @tparam string mode How to advance the LFO (options: 'clocked', 'free'; default: 'clocked')
-- @tparam number period The timing of this LFO's advancement. If mode is 'clocked', argument is in beats. If mode is 'free', argument is in seconds.
-- @tparam function action A callback function to perform as the LFO advances. This library passes both the scaled and the raw value to the callback function.
-- @param number phase The phase shift amount for this LFO (range: 0.0 to 1.0,; default: 0)
-- @param string baseline From where the LFO should start (options: 'min', 'center', 'max'; default: 'min')
function LFO:add(args)
  return self.new(args.shape, args.min, args.max, args.depth, args.mode, args.period, args.action, args.phase, args.baseline)
end

-- PARAMETERS UI /
local function lfo_params_visibility(state, i)
  params[state](params, "lfo_baseline_"..i)
  params[state](params, "lfo_phase_"..i)
  params[state](params, "lfo_offset_"..i)
  params[state](params, "lfo_depth_"..i)
  params[state](params, "lfo_scaled_"..i)
  params[state](params, "lfo_raw_"..i)
  params[state](params, "lfo_mode_"..i)
  if state == "show" then
    if params:get("lfo_mode_"..i) == 1 then
      params:hide("lfo_free_"..i)
      params:show("lfo_clocked_"..i)
    elseif params:get("lfo_mode_"..i) == 2 then
      params:hide("lfo_clocked_"..i)
      params:show("lfo_free_"..i)
    end
  else
    params:hide("lfo_clocked_"..i)
    params:hide("lfo_free_"..i)
  end
  params[state](params, "lfo_shape_"..i)
  params[state](params, "lfo_min_"..i)
  params[state](params, "lfo_max_"..i)
  params[state](params, "lfo_reset_"..i)
  params[state](params, "lfo_reset_target_"..i)
  _menu.rebuild_params()
end

local function build_lfo_spec(lfo,i,bound)
  params:add{
    type = 'control',
    id = 'lfo_'..bound..'_'..i,
    name = 'lfo '..bound,
    controlspec = controlspec.new(
      lfo.min,
      lfo.max,
      lfo.controlspec.warp,
      lfo.controlspec.step,
      bound == 'min' and lfo.min or lfo.max,
      lfo.controlspec.units,
      lfo.controlspec.quantum,
      lfo.controlspec.wrap
    ),
    formatter = lfo.controlspec.formatter
  }
  params:set_action('lfo_'..bound..'_'..i, function(x)
    lfo:set(bound,x)
  end)
end

local function lfo_bang(i)
  local function lb(prm)
    params:lookup_param("lfo_"..prm.."_"..i):bang()
  end
  lb('depth')
  lb('min')
  lb('max')
  lb('baseline')
  lb('offset')
  lb('mode')
  lb('clocked')
  lb('free')
  lb('shape')
  lb('reset')
  lb('reset_target')
end

-- / PARAMETERS UI

-- SCRIPTING /

local function change_bound(target, which, value)
  target[which] = value
  target.percentage = math.abs(target.min-target.max) * target.depth
  scale_lfo(target)
end

local function change_offset(id, value)
  if value == "max" then
    params:lookup_param('lfo_offset_'..id).min = -100
    params:lookup_param('lfo_offset_'..id).max = 0
  elseif value == "center" then
    params:lookup_param('lfo_offset_'..id).min = -50
    params:lookup_param('lfo_offset_'..id).max = 50
  else
    params:lookup_param('lfo_offset_'..id).min = 0
    params:lookup_param('lfo_offset_'..id).max = 100
  end
  params:set('lfo_offset_'..id, 0)
end

local function change_baseline(target, value)
  target.baseline = value
  scale_lfo(target)
end

local function change_depth(target, value)
  target.depth = value
  target.percentage = math.abs(target.min-target.max) * value
  scale_lfo(target)
end

local function change_ppqn(target, ppqn)
  target.ppqn = ppqn
  if target.sprocket then
    target.sprocket:set_division(1/(4*ppqn))
  end
end

local function change_period(target, new_period)
  local new_phase_counter = target.phase_counter + (1/target.ppqn)
  local new_phase
  local adjusted_phase_counter
  if target.mode == "clocked" then
    new_phase = new_phase_counter / target.period
    adjusted_phase_counter = (new_phase * new_period) - 1/target.ppqn
  else
    new_phase = new_phase_counter * clock.get_beat_sec() / target.period
    adjusted_phase_counter = (new_phase * new_period / clock.get_beat_sec()) - 1/target.ppqn
  end

  target.period = new_period
  target.phase_counter = adjusted_phase_counter
end

local function process_lfo(id)
  local _lfo = id
  local phase
  
  local curr_beat_sec = clock.get_beat_sec()
  
  -- the threshold was set arbitrarily. lower values might work too. 
  if beat_sec > curr_beat_sec + 0.2 or beat_sec < curr_beat_sec - 0.2 then
    beat_sec = curr_beat_sec
  end

  _lfo.phase_counter = _lfo.phase_counter + (1/_lfo.ppqn)
  if _lfo.mode == "clocked" then
    phase = _lfo.phase_counter / _lfo.period
  else
    phase = _lfo.phase_counter * beat_sec / _lfo.period
  end
  phase = (phase + _lfo.phase) % 1

  if _lfo.enabled == 1 then

    local current_val;
    if _lfo.shape == 'sine' then
      current_val = (math.sin(2*math.pi*phase) + 1)/2
    elseif _lfo.shape == 'tri' then
      current_val = phase < 0.5 and phase/0.5 or 1-(phase-0.5)/(0.5)
    elseif _lfo.shape == 'up' then
      current_val = phase
    elseif _lfo.shape == 'down' then
      current_val = 1-phase
    elseif _lfo.shape == 'square' then
      current_val = phase < 0.5 and 1 or 0
    elseif _lfo.shape == 'random' then
      current_val = (math.sin(2*math.pi*phase) + 1)/2
    end

    local min = _lfo.min
    local max = _lfo.max
    local offset = (_lfo.max - _lfo.min) * _lfo.offset

    if _lfo.shape ~= 'random' then
      _lfo.raw = current_val
    end
    
    local value = util.linlin(0, 1, min, min + _lfo.percentage, current_val)

    if _lfo.depth > 0 then

      if _lfo.baseline == 'center' then
        value = util.linlin(0, 1, _lfo.scaled_min, _lfo.scaled_max, current_val)
      elseif _lfo.baseline == 'max' then
        value = util.linlin(0, 1, _lfo.scaled_max, _lfo.scaled_min, current_val)
      end

      if _lfo.shape == 'sine' or  _lfo.shape == 'tri' or _lfo.shape == 'up' or _lfo.shape == 'down' then
        value = util.clamp(value, min, max)
        _lfo.scaled = value + offset
      elseif _lfo.shape == 'square' then
        local square_value = value >= _lfo.mid and max or min
        square_value = util.linlin(min, max, _lfo.scaled_min, _lfo.scaled_max, square_value)
        square_value = util.clamp(square_value ,_lfo.scaled_min, _lfo.scaled_max)
        _lfo.scaled = square_value + offset
        _lfo.raw = util.linlin(_lfo.scaled_min, _lfo.scaled_max, 0, 1, square_value)
      elseif _lfo.shape == 'random' then
        local prev_value = _lfo.rand_value
        _lfo.rand_value = value >= _lfo.mid and max or min
        local rand_value;
        if prev_value ~= _lfo.rand_value then
          rand_value = util.linlin(min, max, _lfo.scaled_min, _lfo.scaled_max, math.random(math.floor(min*100), math.floor(max*100))/100)
          rand_value = util.clamp(rand_value, min,max)
          _lfo.scaled = rand_value + offset
          _lfo.raw = util.linlin(_lfo.scaled_min, _lfo.scaled_max, 0, 1, rand_value)
        end
      end

      _lfo.action(_lfo.scaled, _lfo.raw)

      if _lfo.parameter_id ~= nil then
        params:set("lfo_scaled_".._lfo.parameter_id, util.round(_lfo.scaled, 0.01))
        params:set("lfo_raw_".._lfo.parameter_id, util.round(_lfo.raw, 0.01))
      end

    end
  end
end

--- start LFO
function LFO:start(from_parameter)
  if self.sprocket == nil then
    self:reset_phase()
    self.sprocket = norns.lfo.lattice:new_sprocket{
      action=function() process_lfo(self) end,
      division=1/(4*self.ppqn),
      enabled=true,
    }
    if not norns.lfo.lattice.enabled then
      norns.lfo.lattice:start()
    end
    self.enabled = 1
    if not from_parameter and self.parameter_id ~= nil then
      params:set('lfo_'..self.parameter_id,2)
    end
  end
end

--- stop LFO
function LFO:stop()
  if self.sprocket ~= nil then
    self.sprocket:destroy()
    self.sprocket = nil
    self.enabled = 0
    if next(norns.lfo.lattice.sprockets) == nil then
      norns.lfo.lattice:stop()
    end
  end
end

--- set LFO variable state
-- @tparam string var The variable to target (options: 'shape', 'min', 'max', 'depth', 'offset', 'phase', 'mode', 'period', 'reset_target', 'baseline', 'action', 'ppqn')
-- @tparam various arg The argument to pass to the target (often numbers + strings, but 'action' expects a function)
function LFO:set(var, arg)
  if var == nil then
    error('scripted LFO variable required')
  elseif var == 'min' or var == 'max' then
    change_bound(self, var, arg)
  elseif var == 'depth' then
    change_depth(self, arg)
  elseif var == 'baseline' then
    change_baseline(self, arg)
  elseif var == 'ppqn' then
    change_ppqn(self, arg)
  elseif var == 'period' then
    change_period(self, arg)
  elseif arg == nil then
    error('scripted LFO argument required')
  elseif not self[var] then
    error("scripted LFO variable '"..var.."' not valid")
  else
    self[var] = arg
  end
end

--- get LFO variable state
-- @tparam string var The variable to query (options: 'shape', 'min', 'max', 'depth', 'offset', 'phase', 'mode', 'period', 'reset_target', 'baseline', 'action', 'enabled', 'controlspec')
function LFO:get(var)
  if var == nil then
    error('scripted LFO variable required')
  elseif not self[var] then
    error("scripted LFO variable '"..var.."' not valid")
  else
    return self[var]
  end
end

--- reset the LFO's phase
function LFO:reset_phase()
  if self.mode == "free" then
    local baseline = clock.get_beat_sec()/self.period
    if self.reset_target == "floor" then
      self.phase_counter = 0.75/baseline
    elseif self.reset_target == "ceiling" then
      self.phase_counter = 0.25/baseline
    elseif self.reset_target == "mid: falling" then
      self.phase_counter = 0.5/baseline
    elseif self.reset_target == "mid: rising" then
      self.phase_counter = baseline
    end
  else
    if self.reset_target == "floor" then
      self.phase_counter = 0.75 * (self.period)
    elseif self.reset_target == "ceiling" then
      self.phase_counter = 0.25 * (self.period)
    elseif self.reset_target == "mid: falling" then
      self.phase_counter = 0.5 * (self.period)
    elseif self.reset_target == "mid: rising" then
      self.phase_counter = self.period
    end
  end
end

-- / SCRIPT LFOS

--- Build parameter menu UI for an already-constructed LFO.
-- @tparam string id The parameter ID to use for this LFO.
-- @tparam[opt] string separator A separator name for the LFO parameters.
-- @tparam[opt] string group A group name for the LFO parameters.
function LFO:add_params(id,sep,group)
  if id ~= nil then
    if params.lookup["lfo_"..id] == nil then

      if group then
        if sep ~= nil then
          params:add_group("lfo_group_"..id,group,params_per_entry+1)
        else
          params:add_group("lfo_group_"..id,group,params_per_entry)
        end
      end

      if sep then
        params:add_separator("lfo_sep_"..sep,sep)
      end

      params:add_option("lfo_"..id,"lfo state",{"off","on"},self:get('enabled')+1)
      params:set_action("lfo_"..id,function(x)
        if x == 1 then
          lfo_params_visibility("hide", id)
          params:set("lfo_scaled_"..id,"")
          params:set("lfo_raw_"..id,"")
          self:stop()
        elseif x == 2 then
          lfo_params_visibility("show", id)
          self:start(true)
        end
        self:set('enabled',x-1)
        lfo_bang(id)
      end)

      params:add_option("lfo_shape_"..id, "lfo shape", lfo_shapes, tab.key(lfo_shapes,self:get('shape')))
      params:set_action("lfo_shape_"..id, function(x) self:set('shape', params:lookup_param("lfo_shape_"..id).options[x]) end)

      params:add_number("lfo_depth_"..id,"lfo depth",0,100,self:get('depth')*100,function(param) return (param:get().."%") end)
      params:set_action("lfo_depth_"..id, function(x)
        if x == 0 then
          params:set("lfo_scaled_"..id,"")
          params:set("lfo_raw_"..id,"")
        end
        self:set('depth',x/100)
      end)

      params:add_number('lfo_phase_'..id, 'lfo phase', 0, 100, self:get('phase') * 100, function(param) return (param:get()..'%') end)
      params:set_action('lfo_phase_'..id, function(x)
        self:set('phase',x/100)
      end)

      params:add_number('lfo_offset_'..id, 'lfo offset', -100, 100, self:get('offset')*100, function(param) return (param:get().."%") end)
      params:set_action("lfo_offset_"..id, function(x)
        self:set('offset',x/100)
      end)

      params:add_text("lfo_scaled_"..id,"  scaled value","")
      params:add_text("lfo_raw_"..id,"  raw value","")

      build_lfo_spec(self,id,"min")
      build_lfo_spec(self,id,"max")

      local baseline_options = {"from min", "from center", "from max"}
      params:add_option("lfo_baseline_"..id, "lfo baseline", baseline_options, tab.key(baseline_options,'from '..self:get('baseline')))
      params:set_action("lfo_baseline_"..id, function(x)
        self:set('baseline',string.gsub(params:lookup_param("lfo_baseline_"..id).options[x],"from ",""))
        change_offset(id, self:get('baseline'))
        _menu.rebuild_params()
      end)
      
      local mode_options = {'clocked', 'free'}
      params:add_option("lfo_mode_"..id, "lfo mode", mode_options, tab.key(mode_options,self:get('mode')))
      params:set_action("lfo_mode_"..id,
        function(x)
          self:set('mode',params:lookup_param("lfo_mode_"..id).options[x])
          self:set(
            'period',
            x == 1 and (lfo_rates[params:get('lfo_clocked_'..id)] * 4) or params:get('lfo_free_'..id)
          )
          if x == 1 and params:string("lfo_"..id) == "on" then
            params:hide("lfo_free_"..id)
            params:show("lfo_clocked_"..id)
          elseif x == 2 and params:string("lfo_"..id) == "on" then
            params:hide("lfo_clocked_"..id)
            params:show("lfo_free_"..id)
          end
          _menu.rebuild_params()
        end
        )

      local current_period_as_rate = self:get('mode') == 'clocked' and lfo_rates[lfo_rates_as_strings[self:get('period')]] or self:get('period')
      local rate_index = tab.key(lfo_rates,self:get('period'))
      params:add_option("lfo_clocked_"..id, "lfo rate", lfo_rates_as_strings, rate_index)
      params:set_action("lfo_clocked_"..id,
        function(x)
          if params:string("lfo_mode_"..id) == "clocked" then
            self:set('period',lfo_rates[x] * 4)
          end
        end
      )

      params:add{
        type='control',
        id="lfo_free_"..id,
        name="lfo rate",
        controlspec=controlspec.new(0.1,300,'exp',0.1,current_period_as_rate,'sec')
      }
      params:set_action("lfo_free_"..id, function(x)
        if params:string("lfo_mode_"..id) == "free" then
          self:set('period', x)
        end
      end)

      params:add_trigger("lfo_reset_"..id, "reset lfo")
      params:set_action("lfo_reset_"..id, function(x) self:reset_phase() end)

      local reset_destinations = {"floor", "ceiling", "mid: rising", "mid: falling"}
      params:add_option("lfo_reset_target_"..id, "reset lfo to", reset_destinations, tab.key(reset_destinations,self:get('reset_target')))
      params:set_action("lfo_reset_target_"..id, function(x)
        self:set('reset_target', params:lookup_param("lfo_reset_target_"..id).options[x])
      end)
      
      params:hide("lfo_free_"..id)

      params:lookup_param("lfo_"..id):bang()
      self.parameter_id = id
    else
      print('! params for LFO '..id..' already added !')
    end
  else
    print('! parameter id is required to add LFO parameters')
  end
end

return LFO
