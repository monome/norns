T = require 'test/luaunit'
Observable = dofile('lib/container/observable.lua')

--
-- Trivial "observer"
--
local Watcher = {}
Watcher.__index = Watcher

function Watcher.new()
  local o = setmetatable({}, Watcher)
  o.value = nil
  o.notify_count = 0
  return o
end

function Watcher:notify(value)
  self.value = value
  self.notify_count = self.notify_count + 1
end

--
-- tests
--

function test_new_and_value()
  T.assertNil(Observable.new():value())
  T.assertEquals(Observable.new(3):value(), 3)
  local foo = {"one", "two"}
  T.assertEquals(Observable.new(foo):value(), foo)
end

function test_call()
  T.assertNil(Observable.new()())
  T.assertEquals(Observable.new(3)(), 3)
  local bar = {"bar", "baz"}
  T.assertEquals(Observable.new(bar)(), bar)
end

function test_set_no_observers()
  local o = Observable.new(3)
  o:set(4)
  T.assertEquals(o(), 4)
end

function test_set_with_observers()
  local w1 = Watcher.new()
  local w2 = Watcher.new()
  local o = Observable.new(3)
  o:register(w1)
  o:register(w2)
  local v = 1234
  o:set(v)
  T.assertEquals(w1.value, v)
  T.assertEquals(w2.value, v)
end

function test_setting_new_fields_is_error()
  local o = Observable.new("foo")
  T.assertError(function() o.bad = 3 end)
  T.assertError(function() o["foo"] = "bar" end)
end

function test_no_double_registration()
  local w = Watcher.new()
  local o = Observable.new()
  o:register(w)
  o:register(w)
  o:set("foo")
  T.assertEquals(w.value, "foo")
  T.assertEquals(w.notify_count, 1)
end

function test_registration_with_callback()
  local count = 0
  local value = 0
  local handler = function(v)
    count = count + 1
    value = v
  end
  local observer = "foo"  -- any value key

  local o = Observable.new()
  o:register(observer, handler)
  o:register(observer, handler)
  o:set("bar")
  T.assertEquals(count, 1)
  T.assertEquals(value, "bar")
end

function test_unregister()
  local count = 0
  local value = 0
  local handler = function(v)
    count = count + 1
    value = v
  end
  local observer = "bar"  -- any value key
  local w = Watcher.new()

  local o = Observable.new()
  o:register(observer, handler)
  o:register(w)
  o:set(-23)

  -- confirm callback notification
  T.assertEquals(count, 1)
  T.assertEquals(value, -23)

  -- confirm method notification
  T.assertEquals(w.notify_count, 1)
  T.assertEquals(w.value, -23)

  T.assertTrue(o:unregister(w))
  T.assertTrue(o:unregister(observer))

  -- confirm notification stopped (previous values are the same)
  o:set(17)
  T.assertEquals(count, 1)
  T.assertEquals(value, -23)
  T.assertEquals(w.notify_count, 1)
  T.assertEquals(w.value, -23)
end

function test_unregister_of_unknown_observer()
  local w = Watcher.new()
  local o = Observable.new()
  T.assertFalse(o:unregister("foo"))
  T.assertFalse(o:unregister(w))
end

os.exit(T.LuaUnit.run())