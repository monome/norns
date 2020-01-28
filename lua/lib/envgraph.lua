--- Envelope graph drawing module.
-- Subclass of Graph for drawing common envelope graphs. Includes DADSR, ADSR, ASR and AR (Perc).
--
-- @classmod EnvGraph
-- @release v1.0.1
-- @author Mark Eats

local EnvGraph = {}
EnvGraph.__index = EnvGraph

local Graph = require "graph"



-------- Private utility methods --------

local function new_env_graph(x_min, x_max, y_min, y_max)
  local graph = Graph.new(x_min, x_max, "lin", y_min, y_max, "lin", "line_and_point", false, false)
  setmetatable(EnvGraph, {__index = Graph})
  setmetatable(graph, EnvGraph)
  return graph
end

local function set_env_values(self, delay, attack, decay, sustain, release, level, curve)
  if not self._env then self._env = {} end
  if delay then self._env.delay = math.max(0, delay) end
  if attack then self._env.attack = math.max(0, attack) end
  if decay then self._env.decay = math.max(0, decay) end
  if sustain then self._env.sustain = util.clamp(sustain, 0, 1) end
  if release then self._env.release = math.max(0, release) end
  if level then self._env.level = util.clamp(level, self._y_min, self._y_max) end
  if curve then self._env.curve = curve end
end


-------- Public methods --------

--- Create a new DADSR EnvGraph object.
-- All arguments optional.
-- @tparam number x_min Minimum value for x axis, defaults to 0.
-- @tparam number x_max Maximum value for x axis, defaults to 1.
-- @tparam number y_min Minimum value for y axis, defaults to 0.
-- @tparam number y_max Maximum value for y axis, defaults to 1.
-- @tparam number delay Delay value, defaults to 0.1
-- @tparam number attack Attack value, defaults to 0.05.
-- @tparam number decay Decay value, defaults to 0.2.
-- @tparam number sustain Sustain value, accepts 0-1, defaults to 0.5.
-- @tparam number release Release value, defaults to 0.3.
-- @tparam number level Level value, accepts y_min to y_max, defaults to 1.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
-- @treturn EnvGraph Instance of EnvGraph.
function EnvGraph.new_dadsr(x_min, x_max, y_min, y_max, delay, attack, decay, sustain, release, level, curve)
  local graph = new_env_graph(x_min, x_max, y_min, y_max)
  set_env_values(graph, delay or 0.1, attack or 0.05, decay or 0.2, sustain or 0.5, release or 0.3, level or 1, curve or -4)
  
  graph:add_point(0, 0)
  graph:add_point(graph._env.delay, 0)
  graph:add_point(graph._env.delay + graph._env.attack, graph._env.level, graph._env.curve)
  graph:add_point(graph._env.delay + graph._env.attack + graph._env.decay, graph._env.level * graph._env.sustain, graph._env.curve)
  graph:add_point(graph._x_max - graph._env.release, graph._env.level * graph._env.sustain, graph._env.curve)
  graph:add_point(graph._x_max, 0, graph._env.curve)
  return graph
end

--- Edit a DADSR EnvGraph object.
-- All arguments optional.
-- @tparam number delay Delay value.
-- @tparam number attack Attack value.
-- @tparam number decay Decay value.
-- @tparam number sustain Sustain value, accepts 0-1.
-- @tparam number release Release value.
-- @tparam number level Level value, accepts y_min to y_max.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
function EnvGraph:edit_dadsr(delay, attack, decay, sustain, release, level, curve)
  if #self._points ~= 6 then return end
  set_env_values(self, delay, attack, decay, sustain, release, level, curve)
  
  self:edit_point(2, self._env.delay)
  self:edit_point(3, self._env.delay + self._env.attack, self._env.level, self._env.curve)
  self:edit_point(4, self._env.delay + self._env.attack + self._env.decay, self._env.level * self._env.sustain, self._env.curve)
  self:edit_point(5, self._x_max - self._env.release, self._env.level * self._env.sustain, self._env.curve)
  self:edit_point(6, nil, nil, self._env.curve)
end

--- Create a new ADSR EnvGraph object.
-- All arguments optional.
-- @tparam number x_min Minimum value for x axis, defaults to 0.
-- @tparam number x_max Maximum value for x axis, defaults to 1.
-- @tparam number y_min Minimum value for y axis, defaults to 0.
-- @tparam number y_max Maximum value for y axis, defaults to 1.
-- @tparam number attack Attack value, defaults to 0.05.
-- @tparam number decay Decay value, defaults to 0.2.
-- @tparam number sustain Sustain value, accepts 0-1, defaults to 0.5.
-- @tparam number release Release value, defaults to 0.3.
-- @tparam number level Level value, accepts y_min to y_max, defaults to 1.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
-- @treturn EnvGraph Instance of EnvGraph.
function EnvGraph.new_adsr(x_min, x_max, y_min, y_max, attack, decay, sustain, release, level, curve)
  local graph = new_env_graph(x_min, x_max, y_min, y_max)
  set_env_values(graph, nil, attack or 0.05, decay or 0.2, sustain or 0.5, release or 0.3, level or 1, curve or -4)
  
  graph:add_point(0, 0)
  graph:add_point(graph._env.attack, graph._env.level, graph._env.curve)
  graph:add_point(graph._env.attack + graph._env.decay, graph._env.level * graph._env.sustain, graph._env.curve)
  graph:add_point(graph._x_max - graph._env.release, graph._env.level * graph._env.sustain, graph._env.curve)
  graph:add_point(graph._x_max, 0, graph._env.curve)
  return graph
end

--- Edit an ADSR EnvGraph object.
-- All arguments optional.
-- @tparam number attack Attack value.
-- @tparam number decay Decay value.
-- @tparam number sustain Sustain value, accepts 0-1.
-- @tparam number release Release value.
-- @tparam number level Level value, accepts y_min to y_max.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
function EnvGraph:edit_adsr(attack, decay, sustain, release, level, curve)
  if #self._points ~= 5 then return end
  set_env_values(self, nil, attack, decay, sustain, release, level, curve)
  
  self:edit_point(2, self._env.attack, self._env.level, self._env.curve)
  self:edit_point(3, self._env.attack + self._env.decay, self._env.level * self._env.sustain, self._env.curve)
  self:edit_point(4, self._x_max - self._env.release, self._env.level * self._env.sustain, self._env.curve)
  self:edit_point(5, nil, nil, self._env.curve)
end


--- Create a new ASR EnvGraph object.
-- All arguments optional.
-- @tparam number x_min Minimum value for x axis, defaults to 0.
-- @tparam number x_max Maximum value for x axis, defaults to 1.
-- @tparam number y_min Minimum value for y axis, defaults to 0.
-- @tparam number y_max Maximum value for y axis, defaults to 1.
-- @tparam number attack Attack value, defaults to 0.05.
-- @tparam number release Release value, defaults to 0.3.
-- @tparam number level Level value, accepts y_min to y_max, defaults to 1.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
-- @treturn EnvGraph Instance of EnvGraph.
function EnvGraph.new_asr(x_min, x_max, y_min, y_max, attack, release, level, curve)
  local graph = new_env_graph(x_min, x_max, y_min, y_max)
  set_env_values(graph, nil, attack or 0.05, nil, nil, release or 0.3, level or 1, curve or -4)
  
  graph:add_point(0, 0)
  graph:add_point(graph._env.attack, graph._env.level, graph._env.curve)
  graph:add_point(graph._x_max - graph._env.release, graph._env.level, graph._env.curve)
  graph:add_point(graph._x_max, 0, graph._env.curve)
  return graph
end

--- Edit an ASR EnvGraph object.
-- All arguments optional.
-- @tparam number attack Attack value.
-- @tparam number release Release value.
-- @tparam number level Level value, accepts y_min to y_max.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
function EnvGraph:edit_asr(attack, release, level, curve)
  if #self._points ~= 4 then return end
  set_env_values(self, nil, attack, nil, nil, release, level, curve)
  
  self:edit_point(2, self._env.attack, self._env.level, self._env.curve)
  self:edit_point(3, self._x_max - self._env.release, self._env.level, self._env.curve)
  self:edit_point(4, nil, nil, self._env.curve)
end

--- Create a new AR (Perc) EnvGraph object.
-- All arguments optional.
-- @tparam number x_min Minimum value for x axis, defaults to 0.
-- @tparam number x_max Maximum value for x axis, defaults to 1.
-- @tparam number y_min Minimum value for y axis, defaults to 0.
-- @tparam number y_max Maximum value for y axis, defaults to 1.
-- @tparam number attack Attack value, defaults to 0.05.
-- @tparam number release Release value, defaults to 0.3.
-- @tparam number level Level value, accepts y_min to y_max, defaults to 1.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
-- @treturn EnvGraph Instance of EnvGraph.
function EnvGraph.new_ar(x_min, x_max, y_min, y_max, attack, release, level, curve)
  local graph = new_env_graph(x_min, x_max, y_min, y_max)
  set_env_values(graph, nil, attack or 0.1, nil, nil, release or 0.9, level or 1, curve or -4)
  
  graph:add_point(0, 0)
  graph:add_point(graph._env.attack, graph._env.level, graph._env.curve)
  graph:add_point(graph._env.attack + graph._env.release, 0, graph._env.curve)
  return graph
end

--- Edit an AR (Perc) EnvGraph object.
-- All arguments optional.
-- @tparam number attack Attack value.
-- @tparam number release Release value.
-- @tparam number level Level value, accepts y_min to y_max.
-- @tparam string|number curve Curve of envelope, accepts "lin", "exp" or a number where 0 is linear and positive and negative numbers curve the envelope up and down, defaults to -4.
function EnvGraph:edit_ar(attack, release, level, curve)
  if #self._points ~= 3 then return end
  set_env_values(self, nil, attack, nil, nil, release, level, curve)
  
  self:edit_point(2, self._env.attack, self._env.level, self._env.curve)
  self:edit_point(3, self._env.attack + self._env.release, nil, self._env.curve)
end


-- Getters

--- Get delay value.
-- @treturn number Delay value.
function EnvGraph:get_delay() return self._env.delay end

--- Get attack value.
-- @treturn number Attack value.
function EnvGraph:get_attack() return self._env.attack end

--- Get decay value.
-- @treturn number Decay value.
function EnvGraph:get_decay() return self._env.decay end

--- Get sustain value.
-- @treturn number Sustain value.
function EnvGraph:get_sustain() return self._env.sustain end

--- Get release value.
-- @treturn number Release value.
function EnvGraph:get_release() return self._env.release end

--- Get level value.
-- @treturn number Level value.
function EnvGraph:get_level() return self._env.level end

--- Get curve value.
-- @treturn string|number Curve value.
function EnvGraph:get_curve() return self._env.curve end


return EnvGraph
