-- crow asl string building helper functions
--
-- for the common case, these functions make interactions
-- w/ crow look the same on norns as in druid.
--
-- for example, common commands like:
--
-- ```
--    crow.output[1].action = { to(0,0), to(5,0.1), to(1,2) }
--    crow.output[1].action = times(4, { to(0,0), to(5,0.1), to(1,2) } )
--    crow.output[1].action = loop{ to(0,0), to(5,0.1), to(1,2) }
--    crow.output[1].action = lfo(1, 5)
--    crow.output[1].action = ar(1, 5)
--    crow.output[1].action = pulse(1, 5, 1)
--    crow.output[1].action = adsr(0.1, 0.5, 2, 2)
--    crow.output[1].action = { held{to(5.0, 1), to(1, 2)}, to(0, 1) }
-- ```
--
-- work as expected.
--
-- things differ in the "dynamic case" where a script writer wants to assign
-- custom behavior to a crow output.  for example:
--
--     `crow.output[1].action = ar(1, math.random(5))`
--
-- has different behavior executed on norns vs. crow:
--
--   * on norns, `math.random(5)` is executed once and the result assigned to the
--     envelope's release segment
--   * on crow, `math.random(5)` is executed fresh on each triggering of the action
--
-- to get the dynamic re-evaluating behavior on norns, the idiom is to send the asl
-- directly to crow like this:
--
-- ```
--    crow.send[[
--      output[1].action = ar(1, math.random(5))
--    ]]
-- ```
--
-- (note that `[[` and `]]` delimit mult-line strings in Lua.)
--
--
-- todo(pq): add luadocs for functions
-- todo(pq): consider reporting missing required args
--
local function as_arg_string(args, as_table)
  local arg_string = ''
  for _, arg in ipairs(args) do
    if arg then
      if string.len(arg_string) ~= 0 then
        arg_string = arg_string .. ', '
      end
      arg_string = arg_string .. arg
    end
  end
  if as_table then
    return '{' .. arg_string .. '}'
  else
    return '(' .. arg_string .. ')'
  end
end

local function func(fun, args, as_table)
  return fun .. as_arg_string(args, as_table)
end

function ar(attack, release, level)
  return func('ar', {attack, release, level})
end

function pulse(time, level, polarity)
  return func('pulse', {time, level, polarity})
end

function ramp(time, skew, level)
  return func('ramp', {time, skew, level})
end

function to(dest, time, shape)
  return func('to', {dest, time, shape})
end

function lfo(hz, amp)
  return func('lfo', {hz, amp})
end

function loop(asl)
  return func('loop', asl, true)
end

function adsr(attack, decay, sustain, release)
  return func('adsr', {attack, decay, sustain, release})
end

function note(noteNum, duration)
  return func('note', {noteNum, duration})
end

function n2v(n)
  return func('n2v', {n})
end

function negate(v)
  return func('negate', {v})
end

function boundgz(n)
  return func('boundgz', {n})
end

function div(n, d)
  return func('div', {n, d})
end

function times(count, asl)
  local arg_string = as_arg_string(asl, true)
  return 'times(' .. count .. ', ' .. arg_string .. ')'
end

function lock(fns)
  return func('lock', fns, true)
end

function held(fns)
  return func('held', fns, true)
end
