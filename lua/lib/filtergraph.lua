--- Filter graph drawing module.
-- Subclass of Graph for drawing common filter graphs. Can draw approximations of low-pass, band-pass, notch and high-pass filters.
--
-- @classmod FilterGraph
-- @release v1.0.0
-- @author Mark Eats

local FilterGraph = {}
FilterGraph.__index = FilterGraph

local Graph = require "graph"



-------- Private utility methods --------

local function new_filter_graph(x_min, x_max, y_min, y_max)
  local graph = Graph.new(x_min or 10, x_max or 24000, "exp", y_min or -60, y_max or 30, "lin", "spline", true, true)
  setmetatable(FilterGraph, {__index = Graph})
  setmetatable(graph, FilterGraph)
  return graph
end

local function add_points(self)
  
  local filter_type = self._filter.filter_type
  local num_to_add
  
  if filter_type == "bandpass" then
    num_to_add = 3
  elseif filter_type == "notch" then
    num_to_add = 7
  else -- lowpass, highpass
    num_to_add = 5
  end
  
  for i = 1, num_to_add do
    self:add_point(self._x_min, 0)
  end
end

local function update_points(self)
  
  local filter_type = self._filter.filter_type
  local slope = self._filter.slope
  local freq = self._filter.freq
  local min_freq_l = freq / (math.abs(self._y_min) / slope)
  local min_freq_u = freq * (math.abs(self._y_min) / slope)
  local resonance = self._filter.resonance
  local RESO_MIN = -6
  
  if filter_type == "bandpass" then
    self:edit_point(1, min_freq_l, self._y_min)
    self:edit_point(2, freq, util.linlin(0, 1, self._y_max * 0.55, self._y_max * 2.1, resonance))
    self:edit_point(3, min_freq_u, self._y_min)
    
  elseif filter_type == "notch" then
    self:edit_point(1, self._x_min, 0)
    self:edit_point(2, util.linexp(0, 1, min_freq_l * 0.5, min_freq_l * 2, resonance), 0)
    self:edit_point(3, util.linexp(0, 1, min_freq_l * 2, min_freq_l * 4, resonance), util.linlin(0, 1, 0, 0, resonance))
    self:edit_point(4, freq, self._y_min * 1.5)
    self:edit_point(5, util.linexp(0, 1, min_freq_u * 0.5, min_freq_u * 0.25, resonance), util.linlin(0, 1, 0, 0, resonance))
    self:edit_point(6, util.linexp(0, 1, min_freq_u * 2, min_freq_u * 0.5, resonance), 0)
    self:edit_point(7, self._x_max, 0)
    
  elseif filter_type == "highpass" then
    self:edit_point(1, min_freq_l, self._y_min)
    self:edit_point(2, freq, util.linlin(0, 1, RESO_MIN, self._y_max * 1.5, resonance))
    self:edit_point(3, freq * 1.5, 0)
    self:edit_point(4, freq * 4, 0)
    self:edit_point(5, self._x_max, 0)
    
  else -- lowpass
    self:edit_point(2, freq * 0.25, 0)
    self:edit_point(3, freq * 0.666666, 0)
    self:edit_point(4, freq, util.linlin(0, 1, RESO_MIN, self._y_max * 1.5, resonance))
    self:edit_point(5, min_freq_u, self._y_min)
  end
  
end

local function set_filter_values(self, filter_type, slope, freq, resonance)
  if not self._filter then self._filter = {} end
  if filter_type then
    self._filter.filter_type = filter_type
    self:remove_all_points()
    add_points(self)
  end
  if slope then self._filter.slope = util.clamp(math.abs(slope), 6, 48) end
  if freq then self._filter.freq = util.clamp(freq, self._x_min, self._x_max) end
  if resonance then self._filter.resonance = util.clamp(resonance, 0, 1) end
  
  update_points(self)
end


-------- Public methods --------

--- Create a new FilterGraph object.
-- All arguments optional.
-- @tparam number x_min Minimum frequency value in Hz for x axis, defaults to 20.
-- @tparam number x_max Maximum frequency value in Hz for x axis, defaults to 24000.
-- @tparam number y_min Minimum amplitude value in dB for y axis, defaults to -60.
-- @tparam number y_max Maximum amplitude value in dB for y axis, defaults to 30.
-- @tparam string filter_type Type of filter, accepts "lowpass", "bandpass", "notch" or "highpass", defaults to "lowpass".
-- @tparam number slope Slope value in decibels per octave, defaults to 12.
-- @tparam number freq Frequency value in Hz, defaults to 2000.
-- @tparam number resonance Resonance value 0-1, defaults to 0.
-- @treturn FilterGraph Instance of FilterGraph.
function FilterGraph.new(x_min, x_max, y_min, y_max, filter_type, slope, freq, resonance)
  local graph = new_filter_graph(x_min, x_max, y_min, y_max)
  set_filter_values(graph, filter_type or "lowpass", slope or 12, freq or 2000, resonance or 0)
  return graph
end

--- Edit a FilterGraph object.
-- All arguments optional.
-- @tparam string filter_type Type of filter, accepts "lowpass", "bandpass", "notch" or "highpass", defaults to "lowpass".
-- @tparam number slope Slope value in decibels per octave, defaults to 12.
-- @tparam number freq Frequency value in Hz, defaults to 2000.
-- @tparam number resonance Resonance value 0-1, defaults to 0.
function FilterGraph:edit(filter_type, slope, freq, resonance)
  set_filter_values(self, filter_type, slope, freq, resonance)
end


-- Getters

--- Get filter type string.
-- @treturn string Filter type string.
function FilterGraph:filter_type() return self._filter.filter_type end

--- Get slope value.
-- @treturn number Slope value.
function FilterGraph:get_slope() return self._filter.slope end

--- Get frequency value.
-- @treturn number Frequency value.
function FilterGraph:get_freq() return self._filter.freq end

--- Get resonance value.
-- @treturn number Resonance value.
function FilterGraph:get_resonance() return self._filter.resonance end


return FilterGraph
