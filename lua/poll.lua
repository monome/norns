--- Poll class;
-- API for receiving values from audio system.
-- @module poll
-- @alias Poll

local tab = require 'tabutil'

norns.version.poll = '0.0.2'

local Poll = {}
Poll.__index = Poll

--- poll objects (index by name)
Poll.polls = {}
--- poll names (indexed by int) - for reverse lookup
Poll.poll_names = {}

-- constructor
function Poll.new(props)
  local p = {}
    if props then
    if props.id and props.name then
   p.props = props
    end
  else
    props = {}
    print("warning: Poll constructor requires at least name and id properties")
  end
  setmetatable(p, Poll)
  return p
end

--- Instance Methods
-- @section instance

--- start a poll
function Poll:start()
  start_poll(self.props.id)
end

--- stop a poll
function Poll:stop()
  stop_poll(self.props.id)
end


--- custom setters;
-- `.time` and `.callback` set the corresponding private properties and perform approriate actions.
function Poll:__newindex(idx, val)
  if idx == 'time' then
    self.props.time = val
    set_poll_time(self.props.id, val)
  elseif idx == 'callback' then
    self.props.callback = val
  end
  -- oher properties are not settable!
end

--- custom getters;
-- available properties: name, callback, start, stop
function Poll:__index(idx)
  if idx == 'id' then return self.props.id
  elseif idx == 'name' then return self.props.name
  elseif idx == 'callback' then return self.props.callback
  elseif idx == 'start' then return Poll.start
  elseif idx == 'stop' then return Poll.stop
  elseif idx == 'perform' then return Poll.perform
  else
    return rawget(self, idx)
  end
end

-- perform the poll's assigned callback function, if it exists
-- (fixme: this all seems a little over-complicated)
function Poll:perform(value)
  if p.props then
    if p.props.callback then
      if type(p.props.callback) == "function" then
        p.props.callback(value)
      end
    else
      -- print("no callback") -- ok
    end
  else
    print("error: poll has no properties!") assert(false)
  end
end

--- Static Methods
-- @section static

--- called with OSC data from norns callback to register all available polls
-- @param data - table from OSC; each entry is { id (int), name (string) }
-- @tparam integer count - size of table
Poll.register = function(data, count)
  Poll.polls = {}
  Poll.poll_names = {}
  local props
  for i=1,count do
    props = {
      id = data[i][1],
      name = data[i][2]
    }
    -- print(props.id, props.name)
    Poll.poll_names[props.id] = props.name
    Poll.polls[props.name] = Poll.new(props)
  end
end

Poll.list_names = function()
  print('--- polls ---')
  local names = tab.sort(Poll.polls)
  for i,n in ipairs(names) do print(n) end
  print('------\n')
end

--- set callback function for registered Poll object by name
-- @tparam string name
-- @param callback function to call with value on each poll
Poll.set = function(name, callback)
  local p = Poll.polls[name]
  if(p) then
    p.props.callback = callback
  end
  return p
end


return Poll
