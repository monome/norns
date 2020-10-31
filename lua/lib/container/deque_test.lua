T = dofile('lib/test/luaunit.lua')
Deque = dofile('lib/container/deque.lua')

function test_push()
  local d = Deque.new()
  d:push('a')
  T.assertEquals(d._e[d.first], 'a')
  T.assertEquals(d:count(), 1)
  d:push('b')
  T.assertEquals(d._e[d.first], 'b')
  T.assertEquals(d:count(), 2)
end

function test_pop()
  local d = Deque.new()
  d:push('a')
  d:push('b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:pop(), 'b')
  T.assertEquals(d:count(), 1)
  T.assertEquals(d:pop(), 'a')
  T.assertEquals(d:count(), 0)
end

function test_peek()
  local d = Deque.new()
  -- empty case
  T.assertEquals(d:peek(), nil)
  T.assertEquals(d:peek(1), nil)
  T.assertEquals(d:peek(3), nil)

  -- head
  d:push_back('a')
  T.assertEquals(d:peek(), 'a')
  T.assertEquals(d:count(), 1)
  T.assertEquals(d:peek(2), nil)

  -- tombstone in middle
  d:push_back('b')
  d:push_back('c')
  d:remove('b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:peek(1), 'a')
  T.assertEquals(d:peek(2), 'c')
end

function test_push_back()
  local d = Deque.new()
  d:push_back('a')
  T.assertEquals(d._e[d.last], 'a')
  T.assertEquals(d:count(), 1)
  d:push_back('b')
  T.assertEquals(d._e[d.last], 'b')
  T.assertEquals(d:count(), 2)
end

function test_pop_back()
  local d = Deque.new()
  d:push('a')
  d:push('b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:pop_back(), 'a')
  T.assertEquals(d:count(), 1)
  T.assertEquals(d:pop_back(), 'b')
  T.assertEquals(d:count(), 0)
end

function test_peek_back()
  local d = Deque.new()
  -- empty case
  T.assertEquals(d:peek_back(), nil)
  T.assertEquals(d:peek_back(1), nil)
  T.assertEquals(d:peek_back(3), nil)

  -- head
  d:push_back('a')
  T.assertEquals(d:peek_back(), 'a')
  T.assertEquals(d:count(), 1)
  T.assertEquals(d:peek_back(2), nil)

  -- tombstone in middle
  d:push_back('b')
  d:push_back('c')
  d:remove('b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:peek_back(1), 'c')
  T.assertEquals(d:peek_back(2), 'a')
end

function test_deque_new_with_elements()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertEquals(d._e[d.first], 'a')
  T.assertEquals(d._e[d.last], 'c')
end

function test_clear()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertEquals(d:count(), 3)
  d:clear()
  T.assertEquals(d:count(), 0)
  T.assertEquals(d:pop(), nil)
  T.assertEquals(d:pop_back(), nil)
end

function test_extend_back_with_list()
  local d = Deque.new()
  d:extend_back(Deque.new({'a', 'b', 'c'}))
  T.assertEquals(d:count(), 3)
  d:extend_back(Deque.new({'z'}))
  T.assertEquals(d:count(), 4)
  T.assertEquals(d:pop_back(), 'z')
  T.assertEquals(d:pop_back(), 'c')
end

function test_remove_front()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertEquals(d:remove('a'), 'a')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:remove('b'), 'b')
  T.assertEquals(d:count(), 1)
end

function test_remove_back()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertEquals(d:remove('c'), 'c')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:remove('b'), 'b')
  T.assertEquals(d:count(), 1)
end

function test_remove_middle()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertEquals(d:remove('b'), 'b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:pop(), 'a')
  T.assertEquals(d:pop(), 'c')
  T.assertEquals(d:count(), 0)
end

function test_remove_with_duplicates_in_middle()
  local d = Deque.new({'a', 'a', 'b', 'c', 'b'})
  T.assertEquals(d:remove('a'), 'a')
  T.assertEquals(d:count(), 4)
  T.assertEquals(d:remove('b'), 'b')
  T.assertEquals(d:remove('b'), 'b')
  T.assertEquals(d:count(), 2)
  T.assertEquals(d:pop(), 'a')
  T.assertEquals(d:pop(), 'c')
end

local function match_note_event(a, b)
  return (a.ch == b.ch) and (a.note == b.note)
end

function test_remove_with_predicate()
  local na = { ch = 1, note = 30 }
  local nb = { ch = 1, note = 40 }
  local nc = { ch = 1, note = 50 }

  local d = Deque.new({ na, nb, nc })
  -- remove, matching front
  T.assertEquals(d:remove(na, match_note_event), na)
  -- remove, matching back
  d:push_back(na)
  T.assertEquals(d:remove(na, match_note_event), na)
  T.assertEquals(d:to_array(), { nb, nc })

  -- remove, matching middle
  d:push(na)
  T.assertEquals(d:remove(nb, match_note_event), nb)
  T.assertEquals(d:to_array(), { na, nc })
end

function test_contains()
  local d = Deque.new({'a', 'b', 'c'})
  T.assertTrue(d:contains('a'))
  T.assertTrue(d:contains('c'))
  T.assertTrue(d:contains('b'))
  T.assertFalse(d:contains('missing'))
  local thing = d:pop()
  T.assertFalse(d:contains(thing))
end

function test_contains_with_predicate()
  local na = { ch = 1, note = 30 }
  local nb = { ch = 1, note = 40 }
  local nc = { ch = 1, note = 50 }

  local d = Deque.new({ na, nb, nc })
  T.assertTrue(d:contains(na, match_note_event))
  T.assertFalse(d:contains({ ch = 2, note = 2 }))
end

function test_iter_empty()
  local d = Deque.new()
  for i, v in d:ipairs() do
    T.assertTrue(false) -- shouldn't get here
  end
end

function test_iter_simple()
  local v = {'a', 'b', 'c'}
  local d = Deque.new(v)
  T.assertEquals(d:to_array(), v)
end

function test_iter_after_pop()
  local d = Deque.new({'a', 'b', 'c'})
  d:pop()
  T.assertEquals(d:to_array(), {'b', 'c'})
  d:pop_back()
  T.assertEquals(d:to_array(), {'b'})
end

function test_iter_after_remove()
  local d = Deque.new({'a', 'b', 'b', 'c'})
  d:remove('b')
  d:remove('b')
  T.assertEquals(d:to_array(), {'a', 'c'})
end

os.exit(T.LuaUnit.run())