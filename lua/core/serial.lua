--- USB serial device interface
-- @module serial
local tab = require 'tabutil'

local serial = {
  _handlers={},
}

--- add a new serial device handler
-- @param args
-- @param args.id unique identifier for the handler
-- @param args.match function(attrs) returning true if the handler should accept the device with the given attributes, 
--   otherwise false
-- @param args.configure function(term) receiving the device's initial terminal settings and returning the new terminal
--   settings. The callback should modify the input table using bitwise operations and then return the input table
-- @param args.add function(id, name, dev) called when a new device for this handler has been connected and initialized
-- @param args.remove function(id) called when a connected device for this  handler has been disconnected
-- @param args.event function(id, data) called when a message is received from a connected device for this handler. The 
--   data argument will be no more than 255 bytes in length. Messages longer than 255 bytes will be spread out over 
--   multiple event callbacks. It's possible for multiple messages shorter than 255 bytes to appear in a single event 
--   callback.
-- @usage
-- friends = {}
--
-- serial.add_handler({
--   id = "example_id", -- can be number or string
--
--   match = function(attrs)
--     local is_match = (
--           (attrs["vendor"] == "FTDI")
--       and (attrs["model"] == "FT232R")
--       and (attrs["serial"] == "ABC123")
--       and (attrs["interface"] == "00")
--     )
--     return is_match
--   end,
--
--   configure = function(term)
--       -- set baud rate.
--       term.ispeed = term.ispeed | serial.speed.B115200
--       term.ospeed = term.ospeed | serial.speed.B115200
--
--       -- enable some flags.
--       term.iflag = term.iflag | serial.iflag.INCLR
--       term.oflag = term.oflag | serial.oflag.ONLCR
--       term.cflag = term.cflag | serial.cflag.CS8
--
--       -- disable some flags.
--       term.lflag = term.lflag & (~serial.lflag.ECHO)
--
--       -- return the copied and modified table.
--       return term
--   end,
--
--   add = function(id, name, dev)
--     print("saying hi...")
--     serial.send(dev, "hello " .. name .. "!")
--     tab.insert(friends, id, dev)
--   end,
--
--   remove = function(id)
--     print("it's too late to say goodbye.")
--     tab.remove(friends, id)
--   end,
--
--   event = function(id, data)
--     print(friends[id] .. " says:", data)
--   end
-- })
function serial.add_handler(args)
  assert(serial._handlers[args.id] == nil, "duplicate serial handler id: " .. args.id)
  serial._handlers[args.id] = args
end

--- send a message to a serial device
-- @param dev opaque device pointer
-- @param data string message to send
function serial.send(dev, data)
  _norns.serial_send(dev, data)
end

_norns.serial = {}

function _norns.serial.match(vendor, model, serial_num, interface_num)
  local attrs = tab.readonly{table={
    vendor=vendor,
    model=model,
    serial=serial_num,
    interface=interface_num,
  }}
  for id, handler in pairs(serial._handlers) do
    if handler.match(attrs) then
      return id
    end
  end
  return nil
end

function _norns.serial.configure(handler_id, tio)
  return serial._handlers[handler_id].configure(tio)
end

function _norns.serial.add(handler_id, id, name, dev)
  serial._handlers[handler_id].add(id, name, dev)
end

function _norns.serial.remove(handler_id, id)
  serial._handlers[handler_id].remove(id)
end

function _norns.serial.event(handler_id, id, data)
  serial._handlers[handler_id].event(id, data)
end

--- CC (special character) constants
-- @table cc
-- @field VINTR INTR character
-- @field VQUIT QUIT character
-- @field VERASE ERASE character
-- @field VKILL KILL character
-- @field VEOF EOF character
-- @field VTIME TIME value
-- @field VMIN MIN value
-- @field VSTART START character
-- @field VSTOP STOP character
-- @field VSUSP SUSP character
-- @field VEOL EOL character
serial.cc = tab.readonly{table = {
  VINTR = 0,
  VQUIT = 1,
  VERASE = 2,
  VKILL = 3,
  VEOF = 4,
  VTIME = 5,
  VMIN = 6,
  VSWTC = 7,
  VSTART = 8,
  VSTOP = 9,
  VSUSP = 10,
  VEOL = 11,
  VREPRINT = 12,
  VDISCARD = 13,
  VWERASE = 14,
  VLNEXT = 15,
  VEOL2 = 16,
}}

--- iflag constants
-- @table iflag
-- @field IGNBREAK Ignore break condition.
-- @field BRKINT Signal interrupt on break.
-- @field IGNPAR Ignore characters with parity errors.
-- @field PARMRK Mark parity and framing errors.
-- @field INPCK Enable input parity check.
-- @field ISTRIP Strip 8th bit off characters.
-- @field INCLR Map NL to CR on input.
-- @field IGNCR Ignore CR.
-- @field ICRNL Map CT to NL on input.
-- @field IUCLC Map uppercase characters to lowercase on input (not in POSIX).
-- @field IXON Enable start/stop output control.
-- @field IXANY Enable any character to restart output.
-- @field IXOFF Enable start/stop input control.
-- @field IMAXBEL Enable start/stop input control.
-- @field IUTF8 Input is UTF8 (not in POSIX).
serial.iflag = tab.readonly{table = {
  IGNBRK = 1,
  BRKINT = 2,
  IGNPAR = 4,
  PARMRK = 8,
  INPCK = 16,
  ISTRIP = 32,
  INCLR = 64,
  IGNCR = 128,
  ICRNL = 256,
  IUCLC = 512,
  IXON = 1024,
  IXANY = 2048,
  IXOFF = 4096,
  IMAXBEL = 8192,
  IUTF8 = 16384,
}}

--- oflag constants
-- @table oflag
-- @field OPOST Post-process output.
-- @field OLCUC Map lowercase characters to uppercase on output. (not in POSIX).
-- @field ONLCR Map NL to CR-NL on output.
-- @field OCRNL Map CR to NL on output.
-- @field ONOCR No CR output at column 0.
-- @field ONLRET NL performs CR function.
-- @field OFILL Use fill characters for delay.
-- @field OFDEL Fill is DEL.
-- @field VTDLY Select vertical-tab delays:
-- @field VT0 Vertical-tab delay type 0.
-- @field VT1 Vertical-tab delay type 1.
serial.oflag = tab.readonly{table = {
  OPOST = 1,
  OLCUC = 2,
  ONLCR = 4,
  OCRNL = 8,
  ONOCR = 16,
  ONLRET = 32,
  OFILL = 64,
  OFDEL = 128,
  VTDLY = 16384,
  VT0 = 0,
  VT1 = 16384,
}}


--- cflag constants
-- @table cflag
-- @field CSIZE Character size
-- @field CS5 5-bits per character / symbol.
-- @field CS6 6-bits per character / symbol.
-- @field CS7 7-bits per character / symbol.
-- @field CS8 8-bits per character / symbol.
-- @field CSTOPB Send two stop bits, else one.
-- @field CREAD Enable receiver.
-- @field PARENB Parity enable.
-- @field PARODD Odd parity, else even.
-- @field HUPCL Hang up on last close.
-- @field CLOCAL Ignore modem status lines.
serial.cflag = tab.readonly{table = {
  CSIZE = 48,
  CS5 = 0,
  CS6 = 16,
  CS7 = 32,
  CS8 = 48,
  CSTOPB = 64,
  CREAD = 128,
  PARENB = 256,
  PARODD = 512,
  HUPCL = 1024,
  CLOCAL = 2048,
}}

--- lflag constants
-- @table lflag
-- @field ISIG Enable signals.
-- @field ICANON Canonical input (erase and kill processing).
-- @field ECHO Enable echo.
-- @field ECHOE Echo erase character as error-correcting backspace.
-- @field ECHOK Echo KILL.
-- @field ECHONL Echo NL.
-- @field NOFLSH Disable flush after interrupt or quit.
-- @field TOSTOP Send SIGTTOU for background output.
-- @field IEXTEN Enable implementation-defined input processing.
serial.lflag = tab.readonly{table = {
  ISIG = 1,
  ICANON = 2,
  ECHO = 8,
  ECHOE = 16,
  ECHOK = 32,
  ECHONL = 64,
  NOFLSH = 128,
  TOSTOP = 256,
  IEXTEN = 32768,
}}

--- speed constants
-- @table speed
-- @field B50 50 baud
-- @field B75 75 baud
-- @field B110 110 baud
-- @field B134 134 baud
-- @field B150 150 baud
-- @field B200 200 baud
-- @field B300 300 baud
-- @field B600 600 baud
-- @field B1200 1200 baud
-- @field B1800 1800 baud
-- @field B2400 2400 baud
-- @field B4800 4800 baud
-- @field B9600 9600 baud
-- @field B19200 19200 baud
-- @field B38400 38400 baud
-- @field B57600 57600 baud
-- @field B115200 115200 baud
-- @field B230400 230400 baud
-- @field B460800 460800 baud
-- @field B500000 500000 baud
-- @field B576000 576000 baud
-- @field B921600 921600 baud
-- @field B1000000 1000000 baud
-- @field B1152000 1152000 baud
-- @field B1500000 1500000 baud
-- @field B2000000 2000000 baud
-- @field B2500000 2500000 baud
-- @field B3000000 3000000 baud
-- @field B3500000 3500000 baud
-- @field B4000000 4000000 baud
serial.speed = tab.readonly{table = {
  B50 = 1,
  B75 = 2,
  B110 = 3,
  B134 = 4,
  B150 = 5,
  B200 = 6,
  B300 = 7,
  B600 = 8,
  B1200 = 9,
  B1800 = 10,
  B2400 = 11,
  B4800 = 12,
  B9600 = 13,
  B19200 = 14,
  B38400 = 15,
  B57600 = 4097,
  B115200 = 4098,
  B230400 = 4099,
  B460800 = 4100,
  B500000 = 4101,
  B576000 = 4102,
  B921600 = 4103,
  B1000000 = 4104,
  B1152000 = 4105,
  B1500000 = 4106,
  B2000000 = 4107,
  B2500000 = 4108,
  B3000000 = 4109,
  B3500000 = 4110,
  B4000000 = 4111,
}}

return serial
