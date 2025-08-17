-- test/lib/container/watchtable_test.lua
-- unit tests for watchtable module

local luaunit = require('lib/test/luaunit')
local WatchTable = require('lib/container/watchtable')

TestWatchTable = {}

function TestWatchTable.test_new_empty()
  local wt = WatchTable.new()
  luaunit.assertNotIsNil(wt)
  luaunit.assertIsNil(wt.foo)
  luaunit.assertIsNil(wt['bar'])
end

function TestWatchTable.test_new_with_initial_table()
  local initial = { a = 1, b = 'test' }
  local wt = WatchTable.new(initial)
  luaunit.assertEquals(wt.a, 1)
  luaunit.assertEquals(wt['b'], 'test')
  luaunit.assertIsNil(wt.missing)
end

function TestWatchTable.test_set_with_callback()
  local captured_key = nil
  local captured_value = nil
  local callback_count = 0

  local callback = function(k, v)
    captured_key = k
    captured_value = v
    callback_count = callback_count + 1
  end

  local wt = WatchTable.new(nil, callback)
  wt.test_key = 'test_value'

  luaunit.assertEquals(captured_key, 'test_key')
  luaunit.assertEquals(captured_value, 'test_value')
  luaunit.assertEquals(callback_count, 1)
  luaunit.assertEquals(wt.test_key, 'test_value')
end

function TestWatchTable.test_multiple_sets()
  local calls = {}
  local callback = function(k, v)
    table.insert(calls, { key = k, value = v })
  end

  local wt = WatchTable.new({}, callback)
  wt.first = 'one'
  wt.second = 'two'
  wt['third'] = 'three'

  luaunit.assertEquals(#calls, 3)
  luaunit.assertEquals(calls[1].key, 'first')
  luaunit.assertEquals(calls[1].value, 'one')
  luaunit.assertEquals(calls[2].key, 'second')
  luaunit.assertEquals(calls[2].value, 'two')
  luaunit.assertEquals(calls[3].key, 'third')
  luaunit.assertEquals(calls[3].value, 'three')
end

function TestWatchTable.test_overwrite_existing_key()
  local call_count = 0
  local last_value = nil

  local callback = function(_k, v)
    call_count = call_count + 1
    last_value = v
  end

  local wt = WatchTable.new({ existing = 'old' }, callback)
  luaunit.assertEquals(wt.existing, 'old')

  wt.existing = 'new'
  luaunit.assertEquals(call_count, 1)
  luaunit.assertEquals(last_value, 'new')
  luaunit.assertEquals(wt.existing, 'new')
end

function TestWatchTable.test_set_nil_value()
  local captured_key = nil
  local captured_value = 'not_nil'
  local callback_count = 0

  local callback = function(k, v)
    captured_key = k
    captured_value = v
    callback_count = callback_count + 1
  end

  local wt = WatchTable.new({ test = 'value' }, callback)
  wt.test = nil

  luaunit.assertEquals(captured_key, 'test')
  luaunit.assertIsNil(captured_value)
  luaunit.assertEquals(callback_count, 1)
  luaunit.assertIsNil(wt.test)
end

function TestWatchTable.test_no_callback_function()
  local wt = WatchTable.new()
  wt.test = 'value'
  luaunit.assertEquals(wt.test, 'value')
end

function TestWatchTable.test_callback_with_different_value_types()
  local calls = {}
  local callback = function(k, v)
    table.insert(calls, { key = k, value = v, type = type(v) })
  end

  local wt = WatchTable.new({}, callback)
  wt.string_val = 'text'
  wt.number_val = 42
  wt.table_val = { nested = true }
  wt.bool_val = false
  wt.nil_val = nil

  luaunit.assertEquals(#calls, 5)
  luaunit.assertEquals(calls[1].type, 'string')
  luaunit.assertEquals(calls[2].type, 'number')
  luaunit.assertEquals(calls[3].type, 'table')
  luaunit.assertEquals(calls[4].type, 'boolean')
  luaunit.assertEquals(calls[5].type, 'nil')
end

function TestWatchTable.test_table_separation()
  local wt1 = WatchTable.new({ shared = 'one' })
  local wt2 = WatchTable.new({ shared = 'two' })

  luaunit.assertEquals(wt1.shared, 'one')
  luaunit.assertEquals(wt2.shared, 'two')

  wt1.shared = 'modified'
  luaunit.assertEquals(wt1.shared, 'modified')
  luaunit.assertEquals(wt2.shared, 'two')
end
