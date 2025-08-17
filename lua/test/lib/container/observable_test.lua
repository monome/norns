-- test/lib/container/observable_test.lua
-- unit tests for observable module

local luaunit = require('lib/test/luaunit')
local Observable = require('lib/container/observable')

--- test observer for testing observable notifications.
local TestWatcher = {}
TestWatcher.__index = TestWatcher

function TestWatcher.new()
  local o = setmetatable({}, TestWatcher)
  o.value = nil
  o.notify_count = 0
  return o
end

function TestWatcher:notify(value)
  self.value = value
  self.notify_count = self.notify_count + 1
end

TestObservable = {}

function TestObservable.test_new_and_value()
  luaunit.assertIsNil(Observable.new():value())
  luaunit.assertEquals(Observable.new(3):value(), 3)
  local foo = { "one", "two" }
  luaunit.assertEquals(Observable.new(foo):value(), foo)
end

function TestObservable.test_call()
  luaunit.assertIsNil(Observable.new()())
  luaunit.assertEquals(Observable.new(3)(), 3)
  local bar = { "bar", "baz" }
  luaunit.assertEquals(Observable.new(bar)(), bar)
end

function TestObservable.test_set_no_observers()
  local o = Observable.new(3)
  o:set(4)
  luaunit.assertEquals(o(), 4)
end

function TestObservable.test_set_with_observers()
  local w1 = TestWatcher.new()
  local w2 = TestWatcher.new()
  local o = Observable.new(3)
  o:register(w1)
  o:register(w2)
  local v = 1234
  o:set(v)
  luaunit.assertEquals(w1.value, v)
  luaunit.assertEquals(w2.value, v)
end

function TestObservable.test_setting_new_fields_is_error()
  local o = Observable.new("foo")
  luaunit.assertError(function() o.bad = 3 end)
  luaunit.assertError(function() o["foo"] = "bar" end)
end

function TestObservable.test_no_double_registration()
  local w = TestWatcher.new()
  local o = Observable.new()
  o:register(w)
  o:register(w)
  o:set("foo")
  luaunit.assertEquals(w.value, "foo")
  luaunit.assertEquals(w.notify_count, 1)
end

function TestObservable.test_registration_with_callback()
  local count = 0
  local value = 0
  local handler = function(v)
    count = count + 1
    value = v
  end
  local observer = "foo" -- any value key

  local o = Observable.new()
  o:register(observer, handler)
  o:register(observer, handler)
  o:set("bar")
  luaunit.assertEquals(count, 1)
  luaunit.assertEquals(value, "bar")
end

function TestObservable.test_unregister()
  local count = 0
  local value = 0
  local handler = function(v)
    count = count + 1
    value = v
  end
  local observer = "bar" -- any value key
  local w = TestWatcher.new()

  local o = Observable.new()
  o:register(observer, handler)
  o:register(w)
  o:set(-23)

  -- confirm callback notification
  luaunit.assertEquals(count, 1)
  luaunit.assertEquals(value, -23)

  -- confirm method notification
  luaunit.assertEquals(w.notify_count, 1)
  luaunit.assertEquals(w.value, -23)

  luaunit.assertTrue(o:unregister(w))
  luaunit.assertTrue(o:unregister(observer))

  -- confirm notification stopped (previous values are the same)
  o:set(17)
  luaunit.assertEquals(count, 1)
  luaunit.assertEquals(value, -23)
  luaunit.assertEquals(w.notify_count, 1)
  luaunit.assertEquals(w.value, -23)
end

function TestObservable.test_unregister_of_unknown_observer()
  local w = TestWatcher.new()
  local o = Observable.new()
  luaunit.assertFalse(o:unregister("foo"))
  luaunit.assertFalse(o:unregister(w))
end

function TestObservable.test_set_quiet_mode()
  local w = TestWatcher.new()
  local o = Observable.new(5)
  o:register(w)

  -- normal set triggers notification
  o:set(10)
  luaunit.assertEquals(w.notify_count, 1)
  luaunit.assertEquals(w.value, 10)

  -- quiet set does not trigger notification
  o:set(20, true)
  luaunit.assertEquals(w.notify_count, 1)
  luaunit.assertEquals(w.value, 10)   -- observer not notified
  luaunit.assertEquals(o:value(), 20) -- but value was updated
end
