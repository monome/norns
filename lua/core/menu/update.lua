local m = {
  alt = false,
  install = "stable",
  dots = 0,
  -- polls with no new progress, cleared by progress and by each check
  stall_ticks = 0,
  status_path = "/tmp/norns-update-status",
  progress_path = "/tmp/norns-update-progress",
  -- named in system_cmd.cc
  update_unit = "norns-update",
  -- override, settable from the maiden repl for testing
  releases_url = nil,
  -- 0.5s ticks without installer progress before probing the unit (10 min)
  install_timeout_ticks = 1200,
  -- bumped on leaving the page, so a stale callback can be told from a live one
  visit_id = 0,
  constants = {
    CHECKING = "checking",
    MESSAGE = "message",
    CONFIRM = "confirm",
    UPDATE = "update",
    CANCEL = "cancel",
    INSTALLING = "installing",
    INSTALL_FAILED = "install failed",
  },
}

releases = {}

local UPDATER = _path.home .. "/norns/update/update.sh"

-- async callbacks, defined below their callers
local compare_releases
local handoff_update

-- the menu timer is shared, so a callback from a previous visit must not stop it
local function if_same_visit(fn)
  local visit = m.visit_id
  return function(...)
    if m.visit_id ~= visit then return end
    return fn(...)
  end
end

-- the shared menu timer, cycling the dots until the page settles
local function start_tick_timer()
  _menu.timer.time = 0.5
  _menu.timer.count = -1
  _menu.timer.event = function()
    m.dots = (m.dots % 3) + 1
    _menu.redraw()
  end
  _menu.timer:start()
end

local function check_newest()
  print("checking for update")
  if m.alt then print("stable and beta") end
  local url = m.releases_url or "https://raw.githubusercontent.com/monome/norns/main/releases.txt"
  norns.system_cmd("curl -s " .. url, if_same_visit(compare_releases))
end

local function valid_release(r)
  return type(r) == "table" and tonumber(r.version) ~= nil
end

compare_releases = function(result)
  print(result)
  releases = {}
  local manifest = load(result or "")
  -- an http error body arrives as an ordinary callback, so it must fail here
  -- rather than error mid-callback
  if not (manifest and pcall(manifest) and valid_release(releases.stable) and valid_release(releases.beta)) then
    print("update: could not read the releases list")
    _menu.timer:stop()
    m.message = "check failed. try again."
    m.stage = m.constants.MESSAGE
    _menu.redraw()
    return
  end

  _menu.timer:stop()
  if m.alt==false and tonumber(norns.version.update) >= tonumber(releases.stable.version) then
    m.message = "up to date."
    m.stage = m.constants.MESSAGE
  else
    m.stage = m.constants.CONFIRM
  end
  _menu.redraw()
end


local function get_update()
  _menu.redraw()
  pcall(cleanup) -- shut down script
  norns.script.clear()
  _menu.locked = true
  print("shutting down audio...")
  for i=1,6 do _norns.cut_enable(i,0) end -- disable softcut
  _norns.execute("sudo systemctl stop norns-sclang.service") -- disable audio
  m.dots = 0
  _menu.redraw()
  print("starting download...")
  local release = releases[m.install]
  local cmd = table.concat({
    "/bin/bash", UPDATER, "download", release.url, release.sha
  }, " ")
  print("> "..cmd)
  norns.system_cmd(cmd, if_same_visit(handoff_update))
  start_tick_timer()
end

local function read_failed_step()
  local f = io.open(m.status_path, "r")
  if not f then return nil end
  local line = f:read("l")
  f:close()
  return line and line:match("^failed:(.+)$") or nil
end

local function clear_failed_step()
  os.remove(m.status_path)
end

local function enter_install_failed(step)
  _menu.timer:stop()
  m.failed_step = step
  m.stage = m.constants.INSTALL_FAILED
  _menu.redraw()
end

local function read_install_progress()
  local f = io.open(m.progress_path, "r")
  if not f then return nil end
  local text = f:read("l")
  local beat = f:read("l")
  f:close()
  if not text or text == "" then return nil end
  return text, text .. "\n" .. (beat or "")
end

local ALIVE_STATES = {
  active = true, activating = true, reloading = true, deactivating = true,
}

local function check_updater_alive(on_result)
  norns.system_cmd("systemctl is-active " .. m.update_unit, function(out)
    -- a transport failure gives "", which reads as not alive
    local state = tostring(out or ""):match("^%s*([%a-]+)")
    on_result(state ~= nil and ALIVE_STATES[state] == true)
  end)
end

local function give_up_unless_alive(alive)
  if m.stage ~= m.constants.INSTALLING then return end
  if alive then return end
  enter_install_failed("timeout")
end

local function check_install_status()
  -- success reboots out from under this poll, so it sees progress or failure
  local step = read_failed_step()
  if step then
    enter_install_failed(step)
    return
  end
  local text, mark = read_install_progress()
  if mark and mark ~= m.last_progress then
    m.last_progress = mark
    m.message = text
    m.stall_ticks = 0
  else
    m.stall_ticks = m.stall_ticks + 1
    if m.stall_ticks >= m.install_timeout_ticks then
      -- a running updater reboots when it is done
      m.stall_ticks = 0
      check_updater_alive(if_same_visit(give_up_unless_alive))
    end
  end
  m.dots = (m.dots % 3) + 1
  _menu.redraw()
end

handoff_update = function(result)
  _menu.timer:stop()
  local status
  for line in tostring(result or ""):gmatch("[^\n]+") do
    status = line:match("^download: (.+)$") or status
  end
  if status ~= "ok" then
    local reason = (status and status:match("checksum")) and "checksum" or "download"
    print("update failed: " .. reason .. " (" .. (status or "no download output") .. ")")
    enter_install_failed(reason)
    return
  end

  print("handing off to updater ("..UPDATER..")...")
  clear_failed_step()
  m.last_progress = nil
  m.stall_ticks = 0
  norns.system_update(function()
    enter_install_failed("launch")
  end)
  m.message = "updating"
  m.stage = m.constants.INSTALLING

  _menu.timer.time = 0.5
  _menu.timer.count = -1
  _menu.timer.event = check_install_status
  _menu.timer:start()
  _menu.redraw()
end


m.key = function(n,z)
  if (m.stage==m.constants.CHECKING or m.stage==m.constants.MESSAGE) and z==1 then
    _menu.set_page("SYSTEM")
    _menu.redraw()
  elseif m.stage==m.constants.CONFIRM then
    if n==2 and z==1 then
      _menu.set_page("SYSTEM")
      _menu.redraw()
    elseif n==3 and z==1 then
      m.stage=m.constants.UPDATE
      get_update()
    end
  elseif m.stage==m.constants.UPDATE then
    if n==2 and z==1 then
      m.stage=m.constants.CANCEL
      _menu.redraw()
    end
  elseif m.stage==m.constants.CANCEL then
    if n==2 and z==1 then
      m.stage=m.constants.UPDATE
      _menu.redraw()
    elseif n==3 and z==1 then
      _norns.reset()
    end
  elseif m.stage==m.constants.INSTALL_FAILED and z==1 then
    -- any key returns to SYSTEM, the previous install is still in place
    _menu.timer:stop()
    clear_failed_step()
    -- bring back sclang (stopped by get_update)
    _norns.execute("sudo systemctl start norns-sclang.service")
    _menu.locked = false
    _menu.set_page("SYSTEM")
    _menu.redraw()
  end
end


m.enc = function(n,delta)
  if n==2 and delta<0 then m.install = "stable" else m.install = "beta" end
  if m.stage == m.constants.CONFIRM then
    _menu.redraw()
  end
end

-- measure the full-width string so cycling dots do not shift the line
local function draw_dots(text)
  local x = 64 - screen.text_extents(text .. "...") / 2
  screen.move(x,40)
  screen.text(text .. string.rep(".", m.dots))
end

m.redraw = function()
  screen.clear()
  screen.level(15)
  screen.move(64,40)
  if m.stage == m.constants.CONFIRM then
    if m.alt==false then
      screen.text_center("update found: "..releases.stable.version)
      screen.move(64,50)
      screen.text_center("install?")
    else
      screen.move(0,30)
      screen.level(m.install=="stable" and 15 or 1)
      screen.text("stable-"..releases.stable.version)
      screen.move(0,40)
      screen.level(m.install=="beta" and 15 or 1)
      screen.text("beta-"..releases.beta.version)
    end
  elseif m.stage == m.constants.UPDATE then
    draw_dots("downloading")
  elseif m.stage == m.constants.CANCEL then
    screen.text_center("cancel?")
  elseif m.stage == m.constants.INSTALLING then
    draw_dots(m.message)
    screen.level(5)
    screen.move(64,50)
    screen.text_center("please do not power off")
  elseif m.stage == m.constants.INSTALL_FAILED then
    screen.move(64,30)
    screen.text_center("update failed")
    local pre, step = "error:", m.failed_step or "unknown"
    local x = 64 - (screen.text_extents(pre) + 4 + screen.text_extents(step)) / 2
    screen.level(10)
    screen.move(x,40)
    screen.text(pre)
    screen.level(15)
    screen.move(x + screen.text_extents(pre) + 4,40)
    screen.text(step)
    screen.level(4)
    screen.move(64,50)
    screen.text_center("press any key")
  elseif m.stage == m.constants.CHECKING then
    draw_dots("checking for update")
  elseif m.stage == m.constants.MESSAGE then
    screen.text_center(m.message)
  end
  screen.update()
end

m.init = function()
  m.install = "stable"
  m.alt = _menu.alt

  local leftover = read_failed_step()
  if leftover then
    enter_install_failed(leftover)
    return
  end

  m.stage = m.constants.CHECKING
  m.dots = 0
  _menu.redraw()
  start_tick_timer()

  local ping = util.os_capture("ping -c 1 github.com | grep failure")

  if ping ~= '' then
    _menu.timer:stop()
    m.message = "need internet."
    m.stage = m.constants.MESSAGE
    _menu.redraw()
  elseif norns.disk < 400 then
    _menu.timer:stop()
    m.message = "disk full. need 400M."
    m.stage = m.constants.MESSAGE
    _menu.redraw()
  else check_newest() end
end

m.deinit = function()
  m.visit_id = m.visit_id + 1
  _menu.timer:stop()
end

-- exposed for lua/test/core/menu/update_test.lua
m._check_install_status = check_install_status
m._handoff_update = handoff_update

return m
