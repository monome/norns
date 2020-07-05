-- observable - an observable value
-- @module observable
-- @alias Observable

local WeakTable = require 'container/weaktable'

--
-- reference: https://en.wikipedia.org/wiki/Observer_pattern)
--

local Observable = {}
Observable.__index = Observable

--- Create an Observable value (object).
--
-- An observable value notifies registered observers each time the value is set.
--
-- @tparam anything initial Initial value, defaults to nil
-- @treturn Observable instance.
function Observable.new(initial)
  local o = {}
  o._observers = WeakTable.new()
  o._value = initial
  setmetatable(o, Observable)
  return o
end

function Observable:__newindex()
  error("Use the set() method to change the observable value")
end

--- Set the value
-- @tparam anything value New value for observable.
-- @tparam boolean quiet If true, do not notify observers.
function Observable:set(value, quiet)
  -- use rawset to avoid __newindex if initial value of _value is nil
  rawset(self, '_value', value)
  if not quiet then
    self:notify_observers()
  end
end

--- Return the current value (short hand for the 'value' method)
-- @treturn anything
function Observable:__call()
  return self._value
end

--- Return the current value
-- @treturn anything
function Observable:value()
  return self._value
end

--- Register an observer of value changes
--
-- An observer is identified by a key which can be any type (number, string,
-- table, etc). Registering an observer replaces any previous registration
-- associated with that key.
--
-- The handler is a function which is passed one argument, the new value. If
-- `handler` is not provided it is assumed that the `observer` is an object with
-- a `notify` method and the `notify` method is called with the new value.
--
-- @tparam anything observer Identity of observer.
-- @tparam function handler New value callback, optional.
function Observable:register(observer, handler)
  if handler ~= nil then
    self._observers[observer] = handler
  else
    self._observers[observer] = function(value)
      observer:notify(value)
    end
  end
end

--- Unregister an observer of value changes
-- @tparam anything observer Identity of observer.
-- @treturn boolean true if the observer was known and unregistered.
function Observable:unregister(observer)
  local existing = self._observers[observer]
  if existing ~= nil then
    self._observers[observer] = nil
    return true
  end
  return false
end

--- Notify registered observers of current value (normally called by `set`).
function Observable:notify_observers()
  for o, f in pairs(self._observers) do
    -- TODO: determine whether this really should be a pcall or not
    pcall(f, self._value)
  end
end

return Observable
