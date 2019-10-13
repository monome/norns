local tabutil = require "tabutil"
local util = require "util"

--
-- constants
--

local HOTSPOT = "Hotspot"

--
-- common functions
--

local function collect_info(cmd)
  local info = {}
  local output = util.os_capture(cmd, true)
  for line in output:gmatch('([^\n]*)\n?') do
    for k, v in line:gmatch('([^:]*):(.+)') do
      if k == "Error" then
	return nil
      end
      info[k] = v
    end
  end
  return info
end

local function get_status(info)
  local state = info["GENERAL.STATE"]
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

local function get_ip4(info)
  local value = info["IP4.ADDRESS[1]"]
  if value ~= nil then
    local p = value:find("/")
    if p ~= nil then p = p - 1 end
    local ip = value:sub(1, p or -1)
    return ip
  end
  return nil
end

--
-- device (class)
--

local Device = {}
Device.__index = Device

function Device.new(name, device_type)
  local o = setmetatable({}, Device)
  o.name = name
  o.type = device_type
  o.info = {}
  o.cache = {}
  return o
end

function Device:refresh()
  local cmd = "nmcli --terse device show " .. self.name
  local info = collect_info(cmd)
  if info ~= nil then
    self.info = info
    self.cache = {}
  end
  -- return self to allow for method chaining
  return self
end

function Device:status()
  return get_status(self.info)
end

function Device:ip4()
  local value = self.cache["ip4"]
  if value ~= nil then
    return value
  end

  local ip = get_ip4(self.info)
  if ip ~= nil then
    self.cache["ip4"] = ip
  end

  return ip
end

function Device:connection_name()
  return self.info["GENERAL.CONNECTION"]
end

--
-- connection (class)
--

local Connection = {}
Connection.__index = Connection

function Connection.new(name)
  local o = setmetatable({}, Connection)
  o.name = name
  o.info = {}
  return o
end

function Connection:refresh()
  local cmd = "nmcli --terse --fields GENERAL,IP4,connection.type"
  cmd = cmd .. " connection show id '" .. self.name .. "'"

  local info = collect_info(cmd)
  if info ~= nil then
    self.info = info
    self.cache = {}
  end
  -- return self to allow for method chaining
  return self
end

function Connection:ip4()
  return get_ip4(self.info)
end

function Connection:device_name()
  return self.info["GENERAL.DEVICES"]
end

function Connection:is_wireless()
  local value = self.info["connection.type"]
  return value ~= nil and value == "802-11-wireless"
end

function Connection:status()
  --return get_status(self.info)
  return self.info["GENERAL.STATE"]
end

--
-- wifi (module)
--

local Wifi = {
  -- variables used by menu
  status = "unavailable",
  state = 0, -- 0 = OFF, 1 = HOTSPOT, 2 = ROUTER
  ip = "",
  signal = "",
  connection_name = "",

  conn_list = {},
  conn_count = 0,
  conn_active = -1,

  -- the hardware interface being manipulated by add, off
  device = Device.new("wlan0"),

  -- active connection object
  connection = nil
}

function Wifi.init()
  -- intended to be called in matron startup code.
  local conns = Wifi.connections(true) -- all connections
  print("network connections:")
  print("--------------------")
  tabutil.print(conns)
  Wifi.update()
end

function Wifi.off()
  print("do wifi off")
  os.execute("nmcli radio wifi off")
end

function Wifi.hotspot()
  print("activating hotspot")
  Wifi.ensure_radio_is_on()
  os.execute("nmcli c delete Hotspot")
  os.execute("nmcli dev wifi hotspot ifname wlan0 ssid norns password nnnnnnnn")
end

function Wifi.on(connection)
  local active = Wifi.active_connection()
  if connection == active then
    print("connection '" .. connection .. "' already enabled.")
  else
    -- clear out variables displayed by menu
    Wifi.connection_name = ""
    Wifi.ip = ""

    print("enabling connection: '" .. connection .. "'")
    Wifi.ensure_radio_is_on()
    -- change connection in bg to allow ui to update
    os.execute("nmcli --wait 2 connection up id '" .. connection .. "' &")
  end
end

function Wifi.connections(all)
  local output = util.os_capture("nmcli --terse --fields name connection show", true)
  local conns = {}
  local i = 1
  for line in output:gmatch('([^\n]*)\n?') do
    if line ~= "" and (line ~= HOTSPOT or all) then
      conns[i] = line
      i = i + 1
    end
  end
  return conns
end

function Wifi.active_connection()
  -- returns the name of the first listed active connection (of potentially multiple)
  local output = util.os_capture("nmcli --terse --fields name conn show --active", true)
  for line in output:gmatch('([^\n]*)\n?') do
    if line ~= "" then
      return line
    end
  end
  return nil
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
    os.execute("nmcli radio wifi on")
    os.execute("sleep 1.5")  -- various operations fail if called right after radio on
  end
end

function Wifi.add(ssid, psk)
  Wifi.ensure_radio_is_on()
  print("adding wifi network: " .. ssid)
  local cmd = "nmcli --wait 10 device wifi connect"
  cmd = cmd .. " '" .. ssid .. "' password '" .. psk .. "'"
  cmd = cmd .. " ifname " .. Wifi.device.name
  os.execute(cmd)
end

function Wifi.delete(name)
  -- FIXME: do we need to turn off radio if name == active_connection?
  print("deleting wifi network: " .. name)
  os.execute("nmcli connection delete id '" .. name .. "'")
end

function Wifi.devices(types)
  local devices = {}
  local i = 1
  local output = util.os_capture("nmcli --terse --fields device,type device", true)
  for line in output:gmatch('([^\n]*)\n?') do
    for device_name, device_type in line:gmatch('([^:]*):(.+)') do
      if k == "Error" then
	return nil
      end
      devices[i] = Device.new(device_name, device_type)
      i = i + 1
    end
  end
  return devices
end

function Wifi.update()
  local active = Wifi.active_connection()

  if active == nil then
    Wifi.connection_name = ""
    Wifi.ip = ""
    Wifi.signal = ""
    Wifi.state = 0
    if Wifi.connection ~= nil then
      -- let status reflect the state of the last active connection
      Wifi.status = Wifi.connection:status() or ""
    end
    return
  end

  if Wifi.connection == nil or Wifi.connection.name ~= active then
    -- connection changed; update device
    Wifi.connection = Connection.new(active)
  end

  Wifi.connection_name = active
  Wifi.connection:refresh()
  Wifi.status = Wifi.connection:status() or ""

  Wifi.state = 0
  if active ~= nil then
    if active == HOTSPOT then Wifi.state = 1 else Wifi.state = 2 end
  end

  Wifi.conn_list = Wifi.connections()
  wifi.conn_count = #Wifi.conn_list
  Wifi.conn_active = tabutil.key(Wifi.conn_list, active)

  if wifi.state > 0 then
    local ip = Wifi.connection:ip4()
    if ip ~= nil then wifi.ip = ip else wifi.ip = "" end

    if Wifi.connection:is_wireless() then
      local name = Wifi.connection:device_name()
      if name then
	wifi.signal = util.os_capture("iw dev " .. name .. " link | grep 'signal' | awk '{print $2}'")
      end
    else
      wifi.signal = ""
    end
  end
end

return Wifi
