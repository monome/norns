local tabutil = require "tabutil"
local util = require "util"

--
-- constants
--

local HOTSPOT = "HOTSPOT"
local HOTSPOT_SSID = "norns"
local HOTSPOT_PSK = "nnnnnnnn"

--
-- device (class)
--

local Device = {}
Device.__index = Device

function Device.new(name)
  local o = setmetatable({}, Device)
  o.name = name
  o.info = {}
  o.cache = {}
  return o
end

function Device:refresh()
  local info = {}
  local output = util.os_capture("nmcli --terse device show " .. self.name, true)
  for line in output:gmatch('([^\n]*)\n?') do
    for k, v in line:gmatch('([^:]*):(.+)') do
      if k == "Error" then
	return self
      end
      info[k] = v
    end
  end
  self.info = info
  self.cache = {}
  -- return self to allow for method chaining
  return self
end

function Device:status()
  local state = self.info["GENERAL.STATE"]
  if state == nil then
    return "unavailable"
  end
  for code, msg in state:gmatch('(%d+)%s+(.+)') do
    code = tonumber(code)
    if code == 20 then
      return "off"
    elseif code == 30 then
      return "disconnected"
    elseif code == 50 then
      return "connecting..."
    elseif code == 100 then
      return "connected"
    end
  end
  return "unknown"
end

function Device:ip4()
  local value = self.cache["ip4"]
  if value ~= nil then
    return value
  end

  value = self.info["IP4.ADDRESS[1]"]
  if value ~= nil then
    local p = value:find("/")
    if p ~= nil then p = p - 1 end
    local ip = value:sub(1, p or -1)
    self.cache["ip4"] = ip
    return ip
  end
  return nil
end

function Device:connection()
  local value = self.info["GENERAL.CONNECTION"]
  if value ~= nil then
    return value
  end
  return nil
end

--
-- wifi (module)
--

local Wifi = {
  -- variables used by menu
  status = "",
  state = 0, -- 0 = OFF, 1 = HOTSPOT, 2 = ROUTER
  ip = "",
  signal = "",
  connection = "",

  conn_list = {},
  conn_count = 0,
  conn_active = -1,

  -- the hardware interface being manipulated
  device = Device.new("wlan0"),
}

function Wifi.init()
  -- one time initialization to ensure HOTSPOT connection exists,
  -- intended to be called in matron startup code.
  local conns = Wifi.connections(true) -- all connections
  print("network connections:")
  print("--------------------")
  tabutil.print(conns)
  local exists = tabutil.key(conns, HOTSPOT)
  if exists == nil then
    print("defining wifi hotspot connection: " .. HOTSPOT)
    Wifi.ensure_radio_is_on()
    local cmd = "sudo nmcli device wifi hotspot ifname wlan0 con-name " .. HOTSPOT
    cmd = cmd .. " ssid '" .. HOTSPOT_SSID .. "'"
    cmd = cmd .. " password '" .. HOTSPOT_PSK .. "'"
    os.execute(cmd)
  end
end

function Wifi.off()
  print("do wifi off")
  os.execute("sudo nmcli radio wifi off")
end

function Wifi.hotspot()
  Wifi.on(HOTSPOT)
end

function Wifi.on(connection)
  local active = Wifi.active_connection()
  if connection == active then
    print("connection '" .. connection .. "' already enabled.")
  else
    -- clear out variables displayed by menu
    Wifi.connection = ""
    Wifi.ip = ""

    print("enabling connection: '" .. connection .. "'")
    Wifi.ensure_radio_is_on()
    -- change connection in bg to allow ui to update
    os.execute("sudo nmcli --wait 5 connection up id '" .. connection .. "' &")
  end
end

function Wifi.connections(all)
  local output = util.os_capture("nmcli --terse --fields name connection show", true)
  local conns = {}
  local i = 1
  for line in output:gmatch('([^\n]*)\n?') do
    if line ~= "" and (line ~= HOTSPOT or all) then
      conns[i] = line
      i = 1 + 1
    end
  end
  return conns
end

function Wifi.active_connection()
  local output = util.os_capture("nmcli --terse --fields name conn show --active")
  if output == "" then
    return nil
  end
  return output
end

function Wifi.ssids()
  local output = util.os_capture("nmcli --terse --fields ssid device wifi list", true)
  local ssids = {}
  local i = 1
  for line in output:gmatch('([^\n]*)\n?') do
    if line ~= "" then
      ssids[i] = line
      i = i + 1
    end
  end
  return ssids
end

function Wifi.radio_state()
  return util.os_capture("nmcli --terse radio wifi")
end

function Wifi.ensure_radio_is_on()
  if Wifi.radio_state() ~= "enabled" then
    os.execute("sudo nmcli radio wifi on")
    os.execute("sleep 1.5")  -- various operations fail if called right after radio on
  end
end


function Wifi.update()
  Wifi.device:refresh()

  local active = Wifi.active_connection()
  if active ~= nil then Wifi.connection = active else Wifi.connection = "" end

  Wifi.status = Wifi.device:status()

  Wifi.state = 0
  if active ~= nil then
    if active == HOTSPOT then Wifi.state = 1 else Wifi.state = 2 end
  end

  Wifi.conn_list = Wifi.connections()
  wifi.conn_count = tabutil.count(Wifi.conn_list)
  Wifi.conn_active = tabutil.key(Wifi.conn_list, active)

  if wifi.state > 0 then
    local ip = Wifi.device:ip4()
    if ip ~= nil then wifi.ip = ip else wifi.ip = "" end

    wifi.signal = util.os_capture("iw dev wlan0 link | grep 'signal' | awk '{print $2}'")
  end
end

return Wifi
