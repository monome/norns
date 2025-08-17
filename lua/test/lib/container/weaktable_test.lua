-- test/lib/container/weaktable_test.lua
-- unit tests for weaktable module

local luaunit = require('lib/test/luaunit')
local WeakTable = require('lib/container/weaktable')

TestWeakTable = {}

function TestWeakTable.test_new_empty()
  local wt = WeakTable.new()
  luaunit.assertNotIsNil(wt)
  luaunit.assertEquals(#wt, 0)
end

function TestWeakTable.test_new_with_initial_table()
  local key1 = { name = 'key1' }
  local key2 = { name = 'key2' }
  local initial = {}
  initial[key1] = 'value1'
  initial[key2] = 'value2'

  local wt = WeakTable.new(initial)
  luaunit.assertEquals(wt[key1], 'value1')
  luaunit.assertEquals(wt[key2], 'value2')
end

function TestWeakTable.test_basic_table_operations()
  local wt = WeakTable.new()
  local key = { id = 'test' }

  wt[key] = 'test_value'
  luaunit.assertEquals(wt[key], 'test_value')

  wt[key] = 'updated_value'
  luaunit.assertEquals(wt[key], 'updated_value')

  wt[key] = nil
  luaunit.assertIsNil(wt[key])
end

function TestWeakTable.test_string_keys_work_normally()
  local wt = WeakTable.new()
  wt['string_key'] = 'string_value'
  luaunit.assertEquals(wt['string_key'], 'string_value')
end

function TestWeakTable.test_number_keys_work_normally()
  local wt = WeakTable.new()
  wt[42] = 'number_value'
  luaunit.assertEquals(wt[42], 'number_value')
end

function TestWeakTable.test_multiple_object_keys()
  local wt = WeakTable.new()
  local key1 = { type = 'first' }
  local key2 = { type = 'second' }
  local key3 = { type = 'third' }

  wt[key1] = 'first_value'
  wt[key2] = 'second_value'
  wt[key3] = 'third_value'

  luaunit.assertEquals(wt[key1], 'first_value')
  luaunit.assertEquals(wt[key2], 'second_value')
  luaunit.assertEquals(wt[key3], 'third_value')
end

function TestWeakTable.test_metamethod_mode_is_set()
  local wt = WeakTable.new()
  local mt = getmetatable(wt)
  -- verify weak table uses __mode = 'k' for key garbage collection.
  luaunit.assertEquals(mt.__mode, 'k')
end

function TestWeakTable.test_weak_reference_behavior()
  local wt = WeakTable.new()

  do
    local temp_key = { data = 'temporary' }
    wt[temp_key] = 'temp_value'
    luaunit.assertEquals(wt[temp_key], 'temp_value')
  end

  -- force garbage collection
  collectgarbage('collect')

  -- after gc, the key should be gone since it was only referenced by the weak table
  local count = 0
  for _, _ in pairs(wt) do
    count = count + 1
  end
  luaunit.assertEquals(count, 0)
end

function TestWeakTable.test_weak_reference_with_strong_reference()
  local wt = WeakTable.new()
  local persistent_key = { data = 'persistent' }

  wt[persistent_key] = 'persistent_value'

  do
    local temp_key = { data = 'temporary' }
    wt[temp_key] = 'temp_value'
  end

  -- force garbage collection
  collectgarbage('collect')

  -- persistent_key should still exist since we hold a strong reference
  luaunit.assertEquals(wt[persistent_key], 'persistent_value')

  local count = 0
  for _, _ in pairs(wt) do
    count = count + 1
  end
  luaunit.assertEquals(count, 1)
end

function TestWeakTable.test_pairs_iteration()
  local wt = WeakTable.new()
  local key1 = { id = 1 }
  local key2 = { id = 2 }

  wt[key1] = 'value1'
  wt[key2] = 'value2'
  wt['string_key'] = 'string_value'

  local count = 0
  local found_values = {}

  for _, v in pairs(wt) do
    count = count + 1
    found_values[v] = true
  end

  luaunit.assertEquals(count, 3)
  luaunit.assertTrue(found_values['value1'])
  luaunit.assertTrue(found_values['value2'])
  luaunit.assertTrue(found_values['string_value'])
end
