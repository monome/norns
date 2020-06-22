T = require 'test/luaunit'
DefaultTable = dofile('lib/container/defaulttable.lua')

--
-- tests
--

function test_new_no_args()
  local dt = DefaultTable.new()
  T.assertNotNil(dt)
  T.assertNil(dt['a'])
  dt['a'] = 1
  T.assertEquals(dt['a'], 1)
end

function test_new_with_table_literal()
  local dt = DefaultTable.new({'a', 'b', 'c'})
  T.assertEquals(dt['key1'][1], 'a')
  T.assertEquals(dt['key1'][3], 'c')

  -- ensure different key also has table
  T.assertEquals(dt['key2'][3], 'c')

  -- ensure keys have different tables
  dt['key1'][3] = 'changed'
  T.assertEquals(dt['key2'][3], 'c')
  T.assertNotEquals(dt['key1'], dt['key2'])
end

function test_new_with_function()
  local initializer = function()
    return { x = 1, y = -16 }
  end

  local default = initializer()
  local dt = DefaultTable.new(initializer)

  T.assertEquals(dt['random'], default)
  dt.random.x = 100
  T.assertEquals(dt['random']['x'], 100)
end

function test_len()
  local dt = DefaultTable.new('value')
  T.assertEquals(#dt, 0)
  local v1 = dt[1]
  T.assertEquals(#dt, 1)
  local v2 = dt[2]
  T.assertEquals(#dt, 2)

  -- NB: like ipairs, # stops counting at the first index with a nil value
  local other = dt[131]
  T.assertEquals(#dt, 2)
end

function test_ipairs()
  local dt = DefaultTable.new()
  dt[1] = 'first'
  dt[2] = 'second'
  dt[3] = 'last'
  dt[34] = 'not reached'

  local iter, t, i = ipairs(dt)
  local i, v = iter(t, i)
  T.assertEquals(v, 'first')
  local i, v = iter(t, i)
  T.assertEquals(v, 'second')
  local i, v = iter(t, i)
  T.assertEquals(v, 'last')
  local i, v = iter(t, i)
  T.assertNil(v)
end

function test_pairs()
  local dt = DefaultTable.new()
  dt['a'] = 'first'
  dt[2] = 1234
  dt[{}] = 'other'

  local keys = {}
  local values = {}

  for k,v in pairs(dt) do
    table.insert(keys, k)
    table.insert(values, v)
  end

  T.assertItemsEquals(keys, {'a', 2, {}})
  T.assertItemsEquals(values, {1234, 'first', 'other'})
end

os.exit(T.LuaUnit.run())