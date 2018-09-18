local Wifi = {}

Wifi.status = ""
Wifi.state = 0 -- 0 = OFF, 1 = HOTSPOT, 2 = ROUTER
Wifi.ip = ""
Wifi.ssid = ""
Wifi.psk = ""
Wifi.signal = ""
Wifi.scan_list = {}
Wifi.scan_count = 0
Wifi.scan_active = -1

Wifi.off = function() os.execute("~/norns/wifi.sh off &") end
Wifi.hotspot = function() os.execute("~/norns/wifi.sh hotspot &") end
Wifi.on = function() os.execute("~/norns/wifi.sh on &") end
Wifi.scan = function() os.execute("~/norns/wifi.sh scan &") end

Wifi.update = function()
  Wifi.ssid = util.os_capture("cat ~/ssid.wifi")
  Wifi.psk = util.os_capture("cat ~/psk.wifi")
  Wifi.status = util.os_capture("cat ~/status.wifi")
  Wifi.state = 0
  if Wifi.status == 'hotspot' then Wifi.state = 1
  elseif Wifi.status == 'router' then Wifi.state = 2 end
  Wifi.scan_list = {}
  for line in io.lines(home_dir.."/scan.wifi") do
    table.insert(Wifi.scan_list,line)
  end
  Wifi.scan_count = tab.count(Wifi.scan_list)
  Wifi.scan_active = tab.key(Wifi.scan_list,Wifi.ssid)
  if wifi.state > 0 then
    wifi.ip = util.os_capture("ifconfig wlan0| grep 'inet ' | awk '{print $2}'")
    wifi.signal = util.os_capture("iw dev wlan0 link | grep 'signal' | awk '{print $2}'")
  end
end

return Wifi
