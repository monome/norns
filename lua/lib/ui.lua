--- UI widgets module.
-- Widgets for paging, tabs, lists, etc.
--
-- @module UI
-- @release v1.0.0
-- @author Mark Eats

local UI = {}
UI.__index = UI



-------- Pages --------

UI.Pages = {}
UI.Pages.__index = UI.Pages

--- Create a new Pages object.
-- @param index Selected page, defaults to 1.
-- @param num_pages Total number of pages, defaults to 3.
-- @return Instance of Pages.
function UI.Pages.new(index, num_pages)
  local pages = {
    index = index or 1,
    num_pages = num_pages or 3
  }
  setmetatable(UI.Pages, {__index = UI})
  setmetatable(pages, UI.Pages)
  return pages
end

--- Set selected page.
-- @param index Page number.
function UI.Pages:set_index(index)
  self.index = util.clamp(index, 1, self.num_pages)
end

--- Set selected page using delta.
-- @param delta Number to move from selected page.
-- @param wrap Boolean, true to wrap pages.
function UI.Pages:set_index_delta(delta, wrap)
  local index
  if wrap then
    index = self.index % self.num_pages + delta
  else
    index = self.index + delta
  end
  self:set_index(index)
end

--- Redraw Pages.
-- Call when changed.
function UI.Pages:redraw()
  local dots_y = util.round((64 - self.num_pages * 4 - (self.num_pages - 1) * 2) * 0.5)
  for i = 1, self.num_pages do
    if i == self.index then screen.level(5)
    else screen.level(1) end
    screen.rect(127, dots_y, 1, 4)
    screen.fill()
    dots_y = dots_y + 6
  end
end


-------- Tabs --------

UI.Tabs = {}
UI.Tabs.__index = UI.Tabs

--- Create a new Tabs object.
-- @param index Selected tab, defaults to 1.
-- @param titles Table of strings for tab titles.
-- @return Instance of Tabs.
function UI.Tabs.new(index, titles)
  local tabs = {
    index = index or 1,
    titles = titles or {}
  }
  setmetatable(UI.Tabs, {__index = UI})
  setmetatable(tabs, UI.Tabs)
  return tabs
end

--- Set selected tab.
-- @param index Tab number.
function UI.Tabs:set_index(index)
  self.index = util.clamp(index, 1, #self.titles)
end

--- Set selected tab using delta.
-- @param delta Number to move from selected tab.
-- @param wrap Boolean, true to wrap tabs.
function UI.Tabs:set_index_delta(delta, wrap)
  local index
  if wrap then
    index = self.index % #self.titles + delta
  else
    index = self.index + delta
  end
  self:set_index(index)
end

--- Redraw Tabs.
-- Call when changed.
function UI.Tabs:redraw()
  local MARGIN = 8
  local GUTTER = 14
  local col_width = (128 - (MARGIN * 2) - GUTTER * (#self.titles - 1)) / #self.titles
  for i = 1, #self.titles do
    if i == self.index then screen.level(15)
    else screen.level(3) end
    screen.move(MARGIN + col_width * 0.5 + ((col_width + GUTTER) * (i - 1)), 6)
    screen.text_center(self.titles[i])
  end
end


-------- List --------

UI.List = {}
UI.List.__index = UI.List

--- Create a new List object.
-- @param x X position.
-- @param y Y position.
-- @param index Selected entry, defaults to 1.
-- @param entries Table of strings for list entries.
-- @return Instance of List.
function UI.List.new(x, y, index, entries)
  local list = {
    x = x or 0,
    y = y or 0,
    index = index or 1,
    entries = entries or {},
    active = true
  }
  setmetatable(UI.List, {__index = UI})
  setmetatable(list, UI.List)
  return list
end

--- Set selected entry.
-- @param index Entry number.
function UI.List:set_index(index)
  self.index = util.clamp(index, 1, #self.entries)
end

-- @param delta Number to move from selected entry.
-- @param wrap Boolean, true to wrap list.
function UI.List:set_index_delta(delta, wrap)
  local index
  if wrap then
    index = self.index % #self.entries + delta
  else
    index = self.index + delta
  end
  self:set_index(index)
end

--- Redraw List.
-- Call when changed.
function UI.List:redraw()
  for i = 1, #self.entries do
    if self.active and i == self.index then screen.level(15)
    else screen.level(3) end
    screen.move(self.x, self.y + (i - 1) * 11)
    local entry = self.entries[i] or ""
    screen.text(entry)
  end
end


-------- ScrollingList --------

UI.ScrollingList = {}
UI.ScrollingList.__index = UI.ScrollingList

--- Create a new ScrollingList object.
-- @param x X position.
-- @param y Y position.
-- @param index Selected entry, defaults to 1.
-- @param entries Table of strings for list entries.
-- @return Instance of ScrollingList.
function UI.ScrollingList.new(x, y, index, entries)
  local list = {
    x = x or 0,
    y = y or 0,
    index = index or 1,
    entries = entries or {},
    active = true
  }
  setmetatable(UI.ScrollingList, {__index = UI})
  setmetatable(list, UI.ScrollingList)
  return list
end

--- Set selected entry.
-- @param index Entry number.
function UI.ScrollingList:set_index(index)
  self.index = util.clamp(index, 1, #self.entries)
end

-- @param delta Number to move from selected entry.
-- @param wrap Boolean, true to wrap list.
function UI.ScrollingList:set_index_delta(delta, wrap)
  local index
  if wrap then
    index = self.index % #self.entries + delta
  else
    index = self.index + delta
  end
  self:set_index(index)
end

--- Redraw ScrollingList.
-- Call when changed.
function UI.ScrollingList:redraw()
  local index_offset = math.max(self.index - (#self.entries - 2), 0)
  for i = 1, 5 do
    if self.active and i == math.min(2, #self.entries - 1) + index_offset then screen.level(15)
    else screen.level(3) end
    screen.move(self.x, self.y + (i - 1) * 11)
    local entry = self.entries[i - 2 + util.clamp(self.index, 1, #self.entries - math.min(2, #self.entries - 1))] or ""
    screen.text(entry)
  end
end


-------- Message --------

UI.Message = {}
UI.Message.__index = UI.Message

--- Create a new Message object.
-- @param text_array Array of lines of text.
-- @return Instance of Message.
function UI.Message.new(text_array)
  local message = {
    text = text_array or {},
    active = true
  }
  setmetatable(UI.Message, {__index = UI})
  setmetatable(message, UI.Message)
  return message
end

--- Redraw Message.
-- Call when changed.
function UI.Message:redraw()
  local LINE_HEIGHT = 11
  local y = util.round(34 - LINE_HEIGHT * (#self.text - 1) * 0.5)
  for i = 1, #self.text do
    if self.active then screen.level(15)
    else screen.level(3) end
    screen.move(64, y)
    screen.text_center(self.text[i])
    y = y + 11
  end
end


-------- PlaybackIcon --------

UI.PlaybackIcon = {}
UI.PlaybackIcon.__index = UI.PlaybackIcon

--- Create a new PlaybackIcon object.
-- @param x X position.
-- @param y Y position.
-- @param size Icon size, defaults to 6.
-- @param status Status number. 1 = Play, 2 = Reverse Play, 3 = Pause, 4 = Stop. Defaults to 1.
-- @return Instance of PlaybackIcon.
function UI.PlaybackIcon.new(x, y, size, status)
  local playback_icon = {
    x = x or 0,
    y = y or 0,
    size = size or 6,
    status = status or 1,
    active = true
  }
  setmetatable(UI.PlaybackIcon, {__index = UI})
  setmetatable(playback_icon, UI.PlaybackIcon)
  return playback_icon
end

--- Redraw PlaybackIcon.
-- Call when changed.
function UI.PlaybackIcon:redraw()
  if self.active then screen.level(15)
  else screen.level(3) end
  -- Play
  if self.status == 1 then
    screen.move(self.x, self.y)
    screen.line(self.x + self.size, self.y + self.size * 0.5)
    screen.line(self.x, self.y + self.size)
    screen.close()
  -- Reverse Play
  elseif self.status == 2 then
    screen.move(self.x + self.size, self.y)
    screen.line(self.x, self.y + self.size * 0.5)
    screen.line(self.x + self.size, self.y + self.size)
    screen.close()
  -- Pause
  elseif self.status == 3 then
    screen.rect(self.x, self.y, util.round(self.size * 0.4), self.size)
    screen.rect(self.x + util.round(self.size * 0.6), self.y, util.round(self.size * 0.4), self.size)
  -- Stop
  else
    screen.rect(self.x, self.y, self.size, self.size)
  end
  screen.fill()
end


return UI
