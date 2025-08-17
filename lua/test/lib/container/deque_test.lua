-- test/lib/container/deque_test.lua
-- unit tests for deque module

local luaunit = require('lib/test/luaunit')
local Deque = require('lib/container/deque')

TestDeque = {}

function TestDeque.test_push()
  local d = Deque.new()
  d:push('a')
  luaunit.assertEquals(d._e[d.first], 'a')
  luaunit.assertEquals(d:count(), 1)
  d:push('b')
  luaunit.assertEquals(d._e[d.first], 'b')
  luaunit.assertEquals(d:count(), 2)
end

function TestDeque.test_pop()
  local d = Deque.new()
  d:push('a')
  d:push('b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:pop(), 'b')
  luaunit.assertEquals(d:count(), 1)
  luaunit.assertEquals(d:pop(), 'a')
  luaunit.assertEquals(d:count(), 0)
end

function TestDeque.test_peek()
  local d = Deque.new()
  -- empty case
  luaunit.assertEquals(d:peek(), nil)
  luaunit.assertEquals(d:peek(1), nil)
  luaunit.assertEquals(d:peek(3), nil)

  -- head
  d:push_back('a')
  luaunit.assertEquals(d:peek(), 'a')
  luaunit.assertEquals(d:count(), 1)
  luaunit.assertEquals(d:peek(2), nil)

  -- tombstone in middle
  d:push_back('b')
  d:push_back('c')
  d:remove('b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:peek(1), 'a')
  luaunit.assertEquals(d:peek(2), 'c')
end

function TestDeque.test_push_back()
  local d = Deque.new()
  d:push_back('a')
  luaunit.assertEquals(d._e[d.last], 'a')
  luaunit.assertEquals(d:count(), 1)
  d:push_back('b')
  luaunit.assertEquals(d._e[d.last], 'b')
  luaunit.assertEquals(d:count(), 2)
end

function TestDeque.test_pop_back()
  local d = Deque.new()
  d:push('a')
  d:push('b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:pop_back(), 'a')
  luaunit.assertEquals(d:count(), 1)
  luaunit.assertEquals(d:pop_back(), 'b')
  luaunit.assertEquals(d:count(), 0)
end

function TestDeque.test_peek_back()
  local d = Deque.new()
  -- empty case
  luaunit.assertEquals(d:peek_back(), nil)
  luaunit.assertEquals(d:peek_back(1), nil)
  luaunit.assertEquals(d:peek_back(3), nil)

  -- head
  d:push_back('a')
  luaunit.assertEquals(d:peek_back(), 'a')
  luaunit.assertEquals(d:count(), 1)
  luaunit.assertEquals(d:peek_back(2), nil)

  -- tombstone in middle
  d:push_back('b')
  d:push_back('c')
  d:remove('b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:peek_back(1), 'c')
  luaunit.assertEquals(d:peek_back(2), 'a')
end

function TestDeque.test_deque_new_with_elements()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(d._e[d.first], 'a')
  luaunit.assertEquals(d._e[d.last], 'c')
end

function TestDeque.test_clear()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(d:count(), 3)
  d:clear()
  luaunit.assertEquals(d:count(), 0)
  luaunit.assertEquals(d:pop(), nil)
  luaunit.assertEquals(d:pop_back(), nil)
end

function TestDeque.test_extend_back_with_list()
  local d = Deque.new()
  d:extend_back(Deque.new({ 'a', 'b', 'c' }))
  luaunit.assertEquals(d:count(), 3)
  d:extend_back(Deque.new({ 'z' }))
  luaunit.assertEquals(d:count(), 4)
  luaunit.assertEquals(d:pop_back(), 'z')
  luaunit.assertEquals(d:pop_back(), 'c')
end

function TestDeque.test_remove_front()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(d:remove('a'), 'a')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:remove('b'), 'b')
  luaunit.assertEquals(d:count(), 1)
end

function TestDeque.test_remove_back()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(d:remove('c'), 'c')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:remove('b'), 'b')
  luaunit.assertEquals(d:count(), 1)
end

function TestDeque.test_remove_middle()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertEquals(d:remove('b'), 'b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:pop(), 'a')
  luaunit.assertEquals(d:pop(), 'c')
  luaunit.assertEquals(d:count(), 0)
end

function TestDeque.test_remove_with_duplicates_in_middle()
  local d = Deque.new({ 'a', 'a', 'b', 'c', 'b' })
  luaunit.assertEquals(d:remove('a'), 'a')
  luaunit.assertEquals(d:count(), 4)
  luaunit.assertEquals(d:remove('b'), 'b')
  luaunit.assertEquals(d:remove('b'), 'b')
  luaunit.assertEquals(d:count(), 2)
  luaunit.assertEquals(d:pop(), 'a')
  luaunit.assertEquals(d:pop(), 'c')
end

local function match_note_event(a, b)
  return (a.ch == b.ch) and (a.note == b.note)
end

function TestDeque.test_remove_with_predicate()
  local na = { ch = 1, note = 30 }
  local nb = { ch = 1, note = 40 }
  local nc = { ch = 1, note = 50 }

  local d = Deque.new({ na, nb, nc })
  -- remove, matching front
  luaunit.assertEquals(d:remove(na, match_note_event), na)
  -- remove, matching back
  d:push_back(na)
  luaunit.assertEquals(d:remove(na, match_note_event), na)
  luaunit.assertEquals(d:to_array(), { nb, nc })

  -- remove, matching middle
  d:push(na)
  luaunit.assertEquals(d:remove(nb, match_note_event), nb)
  luaunit.assertEquals(d:to_array(), { na, nc })
end

function TestDeque.test_contains()
  local d = Deque.new({ 'a', 'b', 'c' })
  luaunit.assertTrue(d:contains('a'))
  luaunit.assertTrue(d:contains('c'))
  luaunit.assertTrue(d:contains('b'))
  luaunit.assertFalse(d:contains('missing'))
  local thing = d:pop()
  luaunit.assertFalse(d:contains(thing))
end

function TestDeque.test_contains_with_predicate()
  local na = { ch = 1, note = 30 }
  local nb = { ch = 1, note = 40 }
  local nc = { ch = 1, note = 50 }

  local d = Deque.new({ na, nb, nc })
  luaunit.assertTrue(d:contains(na, match_note_event))
  luaunit.assertFalse(d:contains({ ch = 2, note = 2 }))
end

function TestDeque.test_iter_empty()
  local d = Deque.new()
  for _, _ in d:ipairs() do
    luaunit.assertTrue(false) -- shouldn't get here
  end
end

function TestDeque.test_iter_simple()
  local v = { 'a', 'b', 'c' }
  local d = Deque.new(v)
  luaunit.assertEquals(d:to_array(), v)
end

function TestDeque.test_iter_after_pop()
  local d = Deque.new({ 'a', 'b', 'c' })
  d:pop()
  luaunit.assertEquals(d:to_array(), { 'b', 'c' })
  d:pop_back()
  luaunit.assertEquals(d:to_array(), { 'b' })
end

function TestDeque.test_iter_after_remove()
  local d = Deque.new({ 'a', 'b', 'b', 'c' })
  d:remove('b')
  d:remove('b')
  luaunit.assertEquals(d:to_array(), { 'a', 'c' })
end

function TestDeque.test_find()
  local d = Deque.new({ 'x', 'y', 'z' })
  luaunit.assertEquals(d:find('y'), 'y')
  luaunit.assertEquals(d:find('x'), 'x')
  luaunit.assertEquals(d:find('z'), 'z')
  luaunit.assertIsNil(d:find('missing'))
end

function TestDeque.test_find_with_predicate()
  local na = { ch = 1, note = 30 }
  local nb = { ch = 1, note = 40 }
  local nc = { ch = 2, note = 50 }

  local d = Deque.new({ na, nb, nc })
  luaunit.assertEquals(d:find(na, match_note_event), na)
  luaunit.assertEquals(d:find(nb, match_note_event), nb)
  luaunit.assertIsNil(d:find({ ch = 3, note = 60 }, match_note_event))
end

function TestDeque.test_find_after_remove()
  local d = Deque.new({ 'a', 'b', 'c', 'b' })
  d:remove('b')
  luaunit.assertEquals(d:find('b'), 'b')
  luaunit.assertEquals(d:find('a'), 'a')
  luaunit.assertEquals(d:find('c'), 'c')
end
