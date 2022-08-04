-- LFOs for general-purpose scripting
-- @module lib.lfo
-- inspired by contributions from @markwheeler (changes), @justmat (hnds), and @sixolet (toolkit)
-- added by @dndrks + @sixolet

local lattice = require 'lattice'

local LFO = {}
LFO.__index = LFO

local lfo_rates = {1/16,1/8,1/4,5/16,1/3,3/8,1/2,3/4,1,1.5,2,3,4,6,8,16,32,64,128,256,512,1024}
local lfo_rates_as_strings = {"1/16","1/8","1/4","5/16","1/3","3/8","1/2","3/4","1","1.5","2","3","4","6","8","16","32","64","128","256","512","1024"}

local params_per_entry = 14

local rand_values;
local scaled_min, scaled_max, mid, centroid_mid;

function LFO.init()
  if norns.lfo == nil then
    norns.lfo = {lattice = lattice:new()}
  end
end

--- construct an LFO
-- @tparam[opt] string shape The shape for this LFO (options: 'sine','saw','square','random'; default: 'sine')
-- @tparam[opt] number min The minimum bound for this LFO (default: 0)
-- @tparam[opt] number max The maximum bound for this LFO (default: 1)
-- @tparam[opt] number depth The depth of modulation between min/max (range: 0.0 to 1.0; default: 0.0)
-- @tparam[opt] string mode How to advance the LFO (options: 'clocked', 'free'; default: 'clocked')
-- @tparam[opt] number period The timing of this LFO's advancement. If mode is 'clocked', argument is in beats. If mode is 'free', argument is in seconds.
-- @tparam[opt] function action A callback function to perform as the LFO advances. This library passes both the scaled and the raw value to the callback function.
function LFO.new(shape, min, max, depth, mode, period, action)  
  local i = {}
  setmetatable(i, LFO)
  i.init()
  i.scaled = 0
  i.raw = 0
  i.phase_counter = 0
  i.shape = shape == nil and 'sine' or shape
  i.min = min == nil and 0 or min
  i.max = max == nil and 1 or max
  i.depth = depth == nil and 1 or depth
  i.enabled = 0
  i.mode = mode == nil and 'clocked' or mode
  i.period = period == nil and 4 or period
  i.reset_target = 'floor'
  i.baseline = 'min'
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
  return i
end

--- construct an LFO via table arguments
-- eg. my_lfo:add{shape = 'sine', min = 200, max = 12000}
-- @tparam[opt] string shape The shape for this LFO (options: 'sine','saw','square','random'; default: 'sine')
-- @tparam[opt] number min The minimum bound for this LFO (default: 0)
-- @tparam[opt] number max The maximum bound for this LFO (default: 1)
-- @tparam[opt] number depth The depth of modulation between min/max (range: 0.0 to 1.0; default: 0.0)
-- @tparam[opt] string mode How to advance the LFO (options: 'clocked', 'free'; default: 'clocked')
-- @tparam[opt] number period The timing of this LFO's advancement. If mode is 'clocked', argument is in beats. If mode is 'free', argument is in seconds.
-- @tparam[opt] function action A callback function to perform as the LFO advances. This library passes both the scaled and the raw value to the callback function.
function LFO:add(args)
  local shape = args.shape == nil and 'sine' or args.shape
  local min = args.min == nil and 0 or args.min
  local max = args.max == nil and 1 or args.max
  local depth = args.depth == nil and 1 or args.depth
  local mode = args.mode == nil and 'clocked' or args.mode
  local period = args.period == nil and 4 or args.period
  local action = args.action == nil and (function(scaled, raw) end) or args.action
  return self.new(shape, min, max, depth, mode, period, action)
end

-- PARAMETERS UI /
local function lfo_params_visibility(state, i)
  params[state](params, "lfo_baseline_"..i)
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

local function scale_lfo(target)
  if target.baseline == 'min' then
    scaled_min = target.min
    scaled_max = target.min + target.percentage
    mid = util.linlin(target.min,target.max,scaled_min,scaled_max,(target.min+target.max)/2)
  elseif target.baseline == 'center' then
    mid = (target.min+target.max)/2
    local centroid_mid = math.abs(target.min-target.max) * (target.depth/2)
    scaled_min = util.clamp(mid - centroid_mid,target.min,target.max)
    scaled_max = util.clamp(mid + centroid_mid,target.min,target.max)
  elseif target.baseline == 'max' then
    mid = (target.min+target.max)/2
    scaled_min = target.max * (1-(target.depth))
    scaled_max = target.max
    mid = math.abs(util.linlin(target.min,target.max,scaled_min,scaled_max,mid))
  end
end

local function change_bound(target, which, value)
  target[which] = value
  target.percentage = math.abs(target.min-target.max) * target.depth
  scale_lfo(target)
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
  if target.pattern then
    target.pattern:set_division(1/(4*ppqn))
  end
end

local function process_lfo(id)
  local _lfo = id
  local phase

  _lfo.phase_counter = _lfo.phase_counter + (1/_lfo.ppqn)
  if _lfo.mode == "clocked" then
    phase = _lfo.phase_counter / _lfo.period
  else
    phase = _lfo.phase_counter * clock.get_beat_sec() / _lfo.period
  end
  phase = phase % 1

  if _lfo.enabled == 1 then

    local current_val;
    if _lfo.shape == 'sine' then
      current_val = (math.sin(2*math.pi*phase) + 1)/2
    elseif _lfo.shape == 'saw' then
      current_val = phase < 0.5 and phase/0.5 or 1-(phase-0.5)/(0.5)
    elseif _lfo.shape == 'square' then
      current_val = phase < 0.5 and 1 or 0
    elseif _lfo.shape == 'random' then
      current_val = (math.sin(2*math.pi*phase) + 1)/2
    end

    local min = _lfo.min
    local max = _lfo.max

    if _lfo.shape ~= 'random' then
      _lfo.raw = current_val
    end
    current_val = current_val + _lfo.offset
    local value = util.linlin(0,1,min,min + _lfo.percentage,current_val)

    if _lfo.depth > 0 then

      if _lfo.baseline == 'center' then
        value = util.linlin(0,1,scaled_min,scaled_max,current_val)
      elseif _lfo.baseline == 'max' then
        value = util.linlin(0,1,scaled_max,scaled_min,current_val)
      end

      if _lfo.shape == "sine" or  _lfo.shape == "saw" then
        value = util.clamp(value,min,max)
        _lfo.scaled = value
      elseif _lfo.shape == "square" then
        local square_value = value >= mid and max or min
        square_value = util.linlin(min,max,scaled_min,scaled_max,square_value)
        square_value = util.clamp(square_value,scaled_min,scaled_max)
        _lfo.scaled = square_value
        _lfo.raw = util.linlin(scaled_min,scaled_max,0,1,square_value)
      elseif _lfo.shape == "random" then
        local prev_value = rand_values
        rand_values = value >= mid and max or min
        local rand_value;
        if prev_value ~= rand_values then
          rand_value = util.linlin(min,max,scaled_min,scaled_max,math.random(math.floor(min*100),math.floor(max*100))/100)
          rand_value = util.clamp(rand_value,min,max)
          _lfo.scaled = rand_value
          _lfo.raw = util.linlin(scaled_min,scaled_max,0,1,rand_value)
        end
      end

      _lfo.action(_lfo.scaled, _lfo.raw)

      if _lfo.parameter_id ~= nil then
        params:set("lfo_scaled_".._lfo.parameter_id,util.round(_lfo.scaled,0.01))
        params:set("lfo_raw_".._lfo.parameter_id,util.round(_lfo.raw,0.01))
      end

    end
  end
end

--- start LFO
function LFO:start()
  if self.pattern == nil then
    self:reset_phase()
    self.pattern = norns.lfo.lattice:new_pattern{
      action=function() process_lfo(self) end,
      division=1/(4*self.ppqn),
      enabled=true,
    }
    if not norns.lfo.lattice.enabled then
      norns.lfo.lattice:start()
    end
    self.enabled = 1
  end
end

--- stop LFO
function LFO:stop()
  if self.pattern ~= nil then
    self.pattern:destroy()
    self.pattern = nil
    self.enabled = 0
    if next(norns.lfo.lattice.patterns) == nil then
      norns.lfo.lattice:stop()
    end
  end
end

--- set LFO variable state
-- @tparam string var The variable to target (options: 'shape', 'min', 'max', 'depth', 'offset', 'mode', 'period', 'reset_target', 'baseline', 'action', 'ppqn')
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
  elseif var == 'ppqn' then -- has its own fn, maintains state, so handled separately.
    change_ppqn(self,arg)
  elseif arg == nil then
    error('scripted LFO argument required')
  elseif not self[var] then
    error("scripted LFO variable '"..var.."' not valid")
  else
    self[var] = arg
  end
end

--- get LFO variable state
-- @tparam string var The variable to query (options: 'shape', 'min', 'max', 'depth', 'offset', 'mode', 'period', 'reset_target', 'baseline', 'action', 'enabled', 'controlspec')
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
    local baseline = 1/(clock.get_beat_sec() / (1/self.period))
    if self.reset_target == "floor" then
      self.phase_counter = (baseline/4) + (baseline/2)
    else
      self.phase_counter = (baseline/4)
    end
  else
    if self.reset_target == "floor" then
      self.phase_counter = 0.75 * (self.period)
    else
      self.phase_counter = 0.25 * (self.period)
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

      params:add_option("lfo_"..id,"lfo state",{"off","on"},1)
      params:set_action("lfo_"..id,function(x)
        if x == 1 then
          lfo_params_visibility("hide", id)
          params:set("lfo_scaled_"..id,"")
          params:set("lfo_raw_"..id,"")
          self:stop()
        elseif x == 2 then
          lfo_params_visibility("show", id)
          self:start()
        end
        self:set('enabled',x-1)
        lfo_bang(id)
      end)

      params:add_option("lfo_shape_"..id, "lfo shape", {"sine","saw","square","random"},1)
      params:set_action("lfo_shape_"..id, function(x) self:set('shape', params:lookup_param("lfo_shape_"..id).options[x]) end)

      params:add_number("lfo_depth_"..id,"lfo depth",0,100,0,function(param) return (param:get().."%") end)
      params:set_action("lfo_depth_"..id, function(x)
        if x == 0 then
          params:set("lfo_scaled_"..id,"")
          params:set("lfo_raw_"..id,"")
        end
        self:set('depth',x/100)
      end)

      params:add_number('lfo_offset_'..id, 'lfo offset', -100, 100, 0, function(param) return (param:get().."%") end)
      params:set_action("lfo_offset_"..id, function(x)
        self:set('offset',x/100)
      end)

      params:add_text("lfo_scaled_"..id,"  scaled value","")
      params:add_text("lfo_raw_"..id,"  raw value","")

      build_lfo_spec(self,id,"min")
      build_lfo_spec(self,id,"max")

      local baseline_options;
      baseline_options = {"from min", "from center", "from max"}
      params:add_option("lfo_baseline_"..id, "lfo baseline", baseline_options, 1)
      params:set_action("lfo_baseline_"..id, function(x)
        self:set('baseline',string.gsub(params:lookup_param("lfo_baseline_"..id).options[x],"from ",""))
        _menu.rebuild_params()
      end)

      params:add_option("lfo_mode_"..id, "lfo mode", {"clocked","free"},1)
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

      params:add_option("lfo_clocked_"..id, "lfo rate", lfo_rates_as_strings, 9)
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
        controlspec=controlspec.new(0.1,300,'exp',0.1,1,'sec')
      }
      params:set_action("lfo_free_"..id, function(x)
        if params:string("lfo_mode_"..id) == "free" then
          self:set('period', x)
        end
      end)

      params:add_trigger("lfo_reset_"..id, "reset lfo")
      params:set_action("lfo_reset_"..id, function(x) self:reset_phase() end)

      params:add_option("lfo_reset_target_"..id, "reset lfo to", {"floor","ceiling"}, 1)
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