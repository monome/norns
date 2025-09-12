-- test/lib/container/defaulttable_test.lua
-- unit tests for defaulttable module

local luaunit = require('lib/test/luaunit')
local DefaultTable = require('lib/container/defaulttable')

TestDefaultTable = {}

function TestDefaultTable.test_new_no_args()
  local dt = DefaultTable.new()
  luaunit.assertNotIsNil(dt)
  luaunit.assertIsNil(dt['a'])
  dt['a'] = 1
  luaunit.assertEquals(dt['a'], 1)
end

function TestDefaultTable.test_new_with_table_literal()
  local dt = DefaultTable.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(dt['key1'][1], 'a')
  luaunit.assertEquals(dt['key1'][3], 'c')

  -- ensure different key also has table
  luaunit.assertEquals(dt['key2'][3], 'c')

  -- ensure keys have different tables
  dt['key1'][3] = 'changed'
  luaunit.assertEquals(dt['key2'][3], 'c')
  luaunit.assertNotEquals(dt['key1'], dt['key2'])
end

function TestDefaultTable.test_new_with_function()
  local initializer = function()
    return { x = 1, y = -16 }
  end

  local default = initializer()
  local dt = DefaultTable.new(initializer)

  luaunit.assertEquals(dt['random'], default)
  dt.random.x = 100
  luaunit.assertEquals(dt['random']['x'], 100)
end

function TestDefaultTable.test_len()
  local dt = DefaultTable.new('value')
  luaunit.assertEquals(#dt, 0)
  local _ = dt[1]
  luaunit.assertEquals(#dt, 1)
  local _ = dt[2]
  luaunit.assertEquals(#dt, 2)

  -- NB: like ipairs, # stops counting at the first index with a nil value
  local _ = dt[131]
  luaunit.assertEquals(#dt, 2)
end

function TestDefaultTable.test_ipairs()
  local dt = DefaultTable.new()
  dt[1] = 'first'
  dt[2] = 'second'
  dt[3] = 'last'
  dt[34] = 'not reached'

  local results = {}
  for i, v in ipairs(dt) do
    table.insert(results, { index = i, value = v })
  end

  luaunit.assertEquals(#results, 3)
  luaunit.assertEquals(results[1].index, 1)
  luaunit.assertEquals(results[1].value, 'first')
  luaunit.assertEquals(results[2].index, 2)
  luaunit.assertEquals(results[2].value, 'second')
  luaunit.assertEquals(results[3].index, 3)
  luaunit.assertEquals(results[3].value, 'last')
end

function TestDefaultTable.test_pairs()
  local dt = DefaultTable.new()
  dt['a'] = 'first'
  dt[2] = 1234
  dt[{}] = 'other'

  local keys = {}
  local values = {}

  for k, v in pairs(dt) do
    table.insert(keys, k)
    table.insert(values, v)
  end

  luaunit.assertItemsEquals(keys, { 'a', 2, {} })
  luaunit.assertItemsEquals(values, { 1234, 'first', 'other' })
end

function TestDefaultTable.test_unpack_behavior()
  -- demonstrates how table.unpack behaves with defaulttable vs regular tables

  -- table.unpack uses # operator to determine length, then unpacks [1] to [#t]
  -- regular table with gap has length determined by # operator
  local normal_table = { 'one' }
  normal_table[3] = 'three' -- creates gap at index 2

  -- # stops at first nil, so length is 1
  luaunit.assertEquals(#normal_table, 1)

  -- unpack uses length, so only unpacks index 1
  local n1, n2 = table.unpack(normal_table)
  luaunit.assertEquals(n1, 'one')
  luaunit.assertIsNil(n2)

  -- defaulttable with gaps behaves differently
  local dt_gap = DefaultTable.new('default')
  dt_gap[1] = 'one'
  dt_gap[2] = 'two'
  dt_gap[3] = 'three'

  -- defaulttable supports # operator properly
  luaunit.assertEquals(#dt_gap, 3)

  -- unpack works normally for contiguous defaulttable
  local d1, d2, d3 = table.unpack(dt_gap)
  luaunit.assertEquals(d1, 'one')
  luaunit.assertEquals(d2, 'two')
  luaunit.assertEquals(d3, 'three')

  -- but with actual gaps, # still stops at first nil value
  local dt_sparse = DefaultTable.new('default')
  dt_sparse[1] = 'first'
  -- skip index 2, creates actual nil (not default value in storage)
  dt_sparse[3] = 'third'

  -- __len metamethod implementation determines this behavior
  luaunit.assertEquals(#dt_sparse, 1) -- stops at first gap

  -- unpack only gets first element due to length
  local s1, s2, s3 = table.unpack(dt_sparse)
  luaunit.assertEquals(s1, 'first')
  luaunit.assertIsNil(s2) -- not unpacked due to length limit
  luaunit.assertIsNil(s3) -- not unpacked due to length limit
end

function TestDefaultTable.test_shallow_copy_isolates_top_level_fields()
  local original = { a = 1, b = { nested = 'value' } }
  local dt = DefaultTable.new(original)

  local copy1 = dt.first_key
  local copy2 = dt.second_key
  copy1.a = 999

  luaunit.assertEquals(copy1.a, 999)
  luaunit.assertEquals(copy2.a, 1)
  luaunit.assertEquals(original.a, 1)
end

function TestDefaultTable.test_shallow_copy_shares_nested_objects()
  local original = { a = 1, b = { nested = 'value' } }
  local dt = DefaultTable.new(original)

  local copy1 = dt.first_key
  local copy2 = dt.second_key
  copy1.b.nested = 'modified'

  luaunit.assertEquals(copy1.b.nested, 'modified')
  luaunit.assertEquals(copy2.b.nested, 'modified')
  luaunit.assertEquals(original.b.nested, 'modified')
  luaunit.assertTrue(copy1.b == copy2.b)
  luaunit.assertTrue(copy1.b == original.b)
end

function TestDefaultTable.test_shallow_copy_handles_all_key_types()
  local key_obj = { id = 'key' }
  local original = {
    string_key = 'string_value',
    [42] = 'number_value',
    [key_obj] = 'object_value'
  }
  local dt = DefaultTable.new(original)

  local copy = dt.test_key

  luaunit.assertEquals(copy.string_key, 'string_value')
  luaunit.assertEquals(copy[42], 'number_value')
  luaunit.assertEquals(copy[key_obj], 'object_value')
  luaunit.assertFalse(copy == original)
end

function TestDefaultTable.test_shallow_copy_handles_non_integer_keys()
  local original = { foo = 'bar', [2.5] = 'float', [''] = 'empty' }
  local dt = DefaultTable.new(original)

  local copy1 = dt.first_key
  local copy2 = dt.second_key
  copy1.foo = 'modified'

  luaunit.assertEquals(copy1.foo, 'modified')
  luaunit.assertEquals(copy2.foo, 'bar')
  luaunit.assertEquals(original.foo, 'bar')
  luaunit.assertEquals(copy1[2.5], 'float')
  luaunit.assertEquals(copy1[''], 'empty')
end

function TestDefaultTable.test_shallow_copy_handles_sparse_arrays()
  local original = {}
  original[1] = 'first'
  original[3] = 'third' -- gap at index 2
  original[10] = 'tenth'
  local dt = DefaultTable.new(original)

  local copy1 = dt.first_key
  local copy2 = dt.second_key
  copy1[1] = 'modified'

  luaunit.assertEquals(copy1[1], 'modified')
  luaunit.assertEquals(copy2[1], 'first')
  luaunit.assertEquals(original[1], 'first')
  luaunit.assertEquals(copy1[3], 'third')
  luaunit.assertEquals(copy1[10], 'tenth')
  luaunit.assertIsNil(copy1[2])
end
