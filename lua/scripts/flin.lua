-- @txt cyclic poly-rhythm music box
engine = 'PolySub'

local GRID_HEIGHT = 8
local DURATION_1 = 1 / 20
local FRAMERATE = 1 / 60

-- from grid_poly.lua
local freqs = {
  55.0, 61.88, 66.0, 68.75, 73.33, 82.5, 92.81, 97.77,
  110.0, 123.75, 132.0, 137.5, 146.66, 165.0, 185.625, 195.55,
}

local cols = {}
for i=1, 16 do
  cols[i] = { running = false, keys_pressed = 0, speed = 1, length = 1, leds = {} }

  for j = 1, GRID_HEIGHT * 4 do
    cols[i].leds[j] = 0
  end
end

function draw_col(x, stage)
  leds = cols[x].leds

  if g ~= nil then 
    for y=1, GRID_HEIGHT do
      g:led(x, y, cols[x].running and 5 or 0)
    end

    if cols[x].running then
      for y=1, GRID_HEIGHT do
        if leds[y] > 0 then
          g:led(x, y, 15)
        end
      end
    end
  end
end

function update_col(x, stage)
  leds = cols[x].leds
  for i=1, GRID_HEIGHT * 4 do leds[i] = 0 end

  index = (stage - 1) % (GRID_HEIGHT * 2) + 1

  for i=1, cols[x].length do
    leds[index] = 1
    index = index - 1
  end
end

function start_col(x, speed, length)
  local timer = metro[x]

  timer.callback = function(stage)
    update_col(x, stage)
    draw_col(x, stage)

    local on = cols[x].leds[1] == 1
    if on then
      e.start(x, freqs[x])
    else
      e.stop(x)
    end
  end

  timer:start(DURATION_1 * speed, -1, 1)
  cols[x].running = true
end

function stop_col(x)
  local timer = metro[x]
  timer:stop()
  cols[x].running = false
  draw_col(x)
  e.stop(x)
end

init = function()

  poll.listNames();
  local refresh_metro = metro[29] -- TODO: choose this so we don't grab menu timers
  refresh_metro.time = FRAMERATE

  refresh_metro.callback = function (stage)
    if g ~= nil then 
      g:refresh()
    end
  end

  refresh_metro:start()
end

gridkey = function(x, y, s)
  if y == GRID_HEIGHT then
    if s == 1 then stop_col(x) end
    return
  else
    if s == 1 then
      cols[x].keys_pressed = cols[x].keys_pressed + 1

      if cols[x].keys_pressed == 1 then
        stop_col(x)
        cols[x].speed = y
        cols[x].length = 1
      else
        cols[x].length = y
      end
    else
      cols[x].keys_pressed = cols[x].keys_pressed - 1

      if cols[x].keys_pressed == 0 then
        start_col(x, cols[x].speed, cols[x].length)
      end
    end
  end
end

