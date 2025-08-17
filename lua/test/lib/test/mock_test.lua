-- lua/test/lib/mock_test.lua
-- test mock functionality

local luaunit = require('lib/test/luaunit')
local mock = require('lib/test/mock')

TestMock = {}

--- test `mock.stub` replaces function and returns restore function.
-- @within TestMock
function TestMock.test_stub_replaces_function()
  local obj = { func = function() return "original" end }

  local restore = mock.stub(obj, 'func', { "stubbed" })

  luaunit.assertEquals(obj.func(), "stubbed")

  restore()
  luaunit.assertEquals(obj.func(), "original")
end

--- test `mock.stub` with multiple return values.
-- @within TestMock
function TestMock.test_stub_multiple_return_values()
  local obj = { func = function() return 1 end }

  local restore = mock.stub(obj, 'func', { "a", "b", "c" })

  local a, b, c = obj.func()
  luaunit.assertEquals(a, "a")
  luaunit.assertEquals(b, "b")
  luaunit.assertEquals(c, "c")

  restore()
end

--- test `mock.stub` with no return values.
-- @within TestMock
function TestMock.test_stub_no_return_values()
  local obj = { func = function() return "original" end }

  local restore = mock.stub(obj, 'func')

  luaunit.assertIsNil(obj.func())

  restore()
  luaunit.assertEquals(obj.func(), "original")
end

--- test `mock.spy` without wrapping function.
-- @within TestMock
function TestMock.test_spy_basic()
  local spy = mock.spy()

  luaunit.assertFalse(spy.called())
  luaunit.assertEquals(spy.call_count(), 0)

  spy("arg1", "arg2")

  luaunit.assertTrue(spy.called())
  luaunit.assertEquals(spy.call_count(), 1)

  local args = spy.args(1)
  luaunit.assertEquals(args[1], "arg1")
  luaunit.assertEquals(args[2], "arg2")
end

--- test `mock.spy` wrapping existing function.
-- @within TestMock
function TestMock.test_spy_wrapping_function()
  local original = function(a, b) return a + b end
  local spy = mock.spy(original)

  local result = spy(3, 4)

  luaunit.assertEquals(result, 7)
  luaunit.assertTrue(spy.called())
  luaunit.assertEquals(spy.call_count(), 1)

  local args = spy.args(1)
  luaunit.assertEquals(args[1], 3)
  luaunit.assertEquals(args[2], 4)
end

--- test `mock.spy` tracks multiple calls.
-- @within TestMock
function TestMock.test_spy_multiple_calls()
  local spy = mock.spy()

  spy("first")
  spy("second", "call")
  spy()

  luaunit.assertEquals(spy.call_count(), 3)
  luaunit.assertTrue(spy.called(3))
  luaunit.assertFalse(spy.called(2))

  luaunit.assertEquals(spy.args(1)[1], "first")
  luaunit.assertEquals(spy.args(2)[1], "second")
  luaunit.assertEquals(spy.args(2)[2], "call")
  luaunit.assertIsNil(spy.args(3)[1])
end

--- test `mock.spy` reset functionality.
-- @within TestMock
function TestMock.test_spy_reset()
  local spy = mock.spy()

  spy("test")
  luaunit.assertTrue(spy.called())

  spy.reset()

  luaunit.assertFalse(spy.called())
  luaunit.assertEquals(spy.call_count(), 0)
  luaunit.assertIsNil(spy.args(1))
end

--- test `mock.spy` callable interface.
-- @within TestMock
function TestMock.test_spy_callable()
  local spy = mock.spy(function(x) return x * 2 end)

  -- test both call styles work
  local result1 = spy(5)
  local result2 = spy.fn(3)

  luaunit.assertEquals(result1, 10)
  luaunit.assertEquals(result2, 6)
  luaunit.assertEquals(spy.call_count(), 2)
end

--- test `mock.stub_table` with single values.
-- @within TestMock
function TestMock.test_stub_table_single_values()
  local obj = {
    func1 = function() return "orig1" end,
    func2 = function() return "orig2" end
  }

  local restore = mock.stub_table(obj, {
    func1 = "stubbed1",
    func2 = "stubbed2"
  })

  luaunit.assertEquals(obj.func1(), "stubbed1")
  luaunit.assertEquals(obj.func2(), "stubbed2")

  restore()

  luaunit.assertEquals(obj.func1(), "orig1")
  luaunit.assertEquals(obj.func2(), "orig2")
end

--- test `mock.stub_table` with multiple return values.
-- @within TestMock
function TestMock.test_stub_table_multiple_values()
  local obj = {
    func1 = function() return 1 end,
    func2 = function() return 2 end
  }

  local restore = mock.stub_table(obj, {
    func1 = { "a", "b" },
    func2 = { "x", "y", "z" }
  })

  local a, b = obj.func1()
  luaunit.assertEquals(a, "a")
  luaunit.assertEquals(b, "b")

  local x, y, z = obj.func2()
  luaunit.assertEquals(x, "x")
  luaunit.assertEquals(y, "y")
  luaunit.assertEquals(z, "z")

  restore()
end

--- test `mock.stub_table` restores in reverse order.
-- @within TestMock
function TestMock.test_stub_table_restore_order()
  local obj = {
    func1 = function() return "orig1" end,
    func2 = function() return "orig2" end
  }

  -- stub twice to test restoration order
  local restore1 = mock.stub_table(obj, { func1 = "first" })
  local restore2 = mock.stub_table(obj, { func1 = "second" })

  luaunit.assertEquals(obj.func1(), "second")

  restore2()
  luaunit.assertEquals(obj.func1(), "first")

  restore1()
  luaunit.assertEquals(obj.func1(), "orig1")
end

--- test `spy` handles functions with variable arguments.
-- @within TestMock
function TestMock.test_spy_variable_arguments()
  local spy = mock.spy(function(...)
    return select('#', ...), ...
  end)

  local count, a, b, c = spy("x", "y", "z")

  luaunit.assertEquals(count, 3)
  luaunit.assertEquals(a, "x")
  luaunit.assertEquals(b, "y")
  luaunit.assertEquals(c, "z")

  local args = spy.args(1)
  luaunit.assertEquals(args[1], "x")
  luaunit.assertEquals(args[2], "y")
  luaunit.assertEquals(args[3], "z")
end

--- test `spy` preserves nil arguments.
-- @within TestMock
function TestMock.test_spy_nil_arguments()
  local spy = mock.spy()

  spy(1, nil, 3)

  local args = spy.args(1)
  luaunit.assertEquals(args[1], 1)
  luaunit.assertIsNil(args[2])
  luaunit.assertEquals(args[3], 3)
end
