--- USB serial device interface
-- @module serial
local tab = require 'tabutil'

local serial = {
  _handlers={},
}

--- add a new serial device handler
-- @param args
-- @param args.id unique identifier for the handler
-- @param args.match function(attrs) returning true if the handler should accept the device with the given attributes, otherwise false
-- @param args.configure function(term) receiving the device's initial terminal settings and returning the new terminal settings. The callback should modify the input table using bitwise operations and then return the input table
-- @param args.add function(id, name, dev) called when a new device for this handler has been connected and initialized
-- @param args.remove function(id) called when a connected device for this  handler has been disconnected
-- @param args.event function(id, line) called when a message is received from a connected device for this handler
function serial.add_handler(args)
  assert(serial._handlers[args.id] == nil, "duplicate serial handler id: " .. args.id)
  serial._handlers[args.id] = args
end

--- send a message to a serial device
-- @param dev opaque device pointer
-- @param line string message to send
function serial.send(dev, line)
  _norns.serial_send(dev, line)
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

function _norns.serial.event(handler_id, id, line)
  serial._handlers[handler_id].event(id, line)
end

--- cc constants
-- @section cc_constants

--- INTR character
serial.VINTR = 0
--- QUIT character
serial.VQUIT = 1
--- ERASE character
serial.VERASE = 2
--- KILL character
serial.VKILL = 3
--- EOF character
serial.VEOF = 4
--- TIME value
serial.VTIME = 5
--- MIN value
serial.VMIN = 6
serial.VSWTC = 7
--- START character
serial.VSTART = 8
--- STOP character
serial.VSTOP = 9
--- SUSP character
serial.VSUSP = 10
--- EOL character
serial.VEOL = 11
serial.VREPRINT = 12
serial.VDISCARD = 13
serial.VWERASE = 14
serial.VLNEXT = 15
serial.VEOL2 = 16

--- iflag constants
-- @section iflag_constants

--- Ignore break condition.
serial.IGNBRK = 1
--- Signal interrupt on break.
serial.BRKINT = 2
--- Ignore characters with parity errors.
serial.IGNPAR = 4
--- Mark parity and framing errors.
serial.PARMRK = 8
--- Enable input parity check.
serial.INPCK = 16
--- Strip 8th bit off characters.
serial.ISTRIP = 32
--- Map NL to CR on input.
serial.INLCR = 64
--- Ignore CR.
serial.IGNCR = 128
--- Map CR to NL on input.
serial.ICRNL = 256
--- Map uppercase characters to lowercase on input (not in POSIX).
serial.IUCLC = 512
--- Enable start/stop output control.
serial.IXON = 1024
--- Enable any character to restart output.
serial.IXANY = 2048
--- Enable start/stop input control.
serial.IXOFF = 4096
--- Ring bell when input queue is full (not in POSIX).
serial.IMAXBEL = 8192
--- Input is UTF8 (not in POSIX).
serial.IUTF8 = 16384

--- oflag constants
-- @section oflag_constants

--- Post-process output.
serial.OPOST = 1
--- Map lowercase characters to uppercase on output. (not in POSIX).
serial.OLCUC = 2
--- Map NL to CR-NL on output.
serial.ONLCR = 4
--- Map CR to NL on output.
serial.OCRNL = 8
--- No CR output at column 0.
serial.ONOCR = 16
--- NL performs CR function.
serial.ONLRET = 32
--- Use fill characters for delay.
serial.OFILL = 64
--- Fill is DEL.
serial.OFDEL = 128
--- Select vertical-tab delays:
serial.VTDLY = 16384
--- Vertical-tab delay type 0.
serial.VT0 = 0
--- Vertical-tab delay type 1.
serial.VT1 = 16384

--- speed constants
-- @section speed_constants

--- 50 baud
serial.B50 = 1
--- 75 baud
serial.B75 = 2
--- 110 baud
serial.B110 = 3
--- 134.5 baud
serial.B134 = 4
--- 150 baud
serial.B150 = 5
--- 200 baud
serial.B200 = 6
--- 300 baud
serial.B300 = 7
--- 600 baud
serial.B600 = 8
--- 1200 baud
serial.B1200 = 9
--- 1800 baud
serial.B1800 = 10
--- 2400 baud
serial.B2400 = 11
--- 4800 baud
serial.B4800 = 12
--- 9600 baud
serial.B9600 = 13
--- 19200 baud
serial.B19200 = 14
--- 38400 baud
serial.B38400 = 15
--- 57600 baud
serial.B57600 = 4097
--- 115200 baud
serial.B115200 = 4098
--- 230400 baud
serial.B230400 = 4099
--- 460800 baud
serial.B460800 = 4100
--- 500000 baud
serial.B500000 = 4101
--- 576000 baud
serial.B576000 = 4102
--- 921600 baud
serial.B921600 = 4103
--- 1000000 baud
serial.B1000000 = 4104
--- 1152000 baud
serial.B1152000 = 4105
--- 1500000 baud
serial.B1500000 = 4106
--- 2000000 baud
serial.B2000000 = 4107
--- 2500000 baud
serial.B2500000 = 4108
--- 3000000 baud
serial.B3000000 = 4109
--- 3500000 baud
serial.B3500000 = 4110
--- 4000000 baud
serial.B4000000 = 4111

--- cflag constants
-- @section cflag_constants

--- Character size
serial.CSIZE = 48
--- 5 bits.
serial.CS5 = 0
--- 6 bits.
serial.CS6 = 16
--- 7 bits.
serial.CS7 = 32
--- 8 bits.
serial.CS8 = 48
--- Send two stop bits, else one.
serial.CSTOPB = 64
--- Enable receiver.
serial.CREAD = 128
--- Parity enable.
serial.PARENB = 256
--- Odd parity, else even.
serial.PARODD = 512
--- Hang up on last close.
serial.HUPCL = 1024
--- Ignore modem status lines.
serial.CLOCAL = 2048

--- lflag constants
-- @section lflag_constants

-- Enable signals.
serial.ISIG = 1
-- Canonical input (erase and kill processing).
serial.ICANON = 2
-- Enable echo.
serial.ECHO = 8
-- Echo erase character as error-correcting backspace.
serial.ECHOE = 16
-- Echo KILL.
serial.ECHOK = 32
-- Echo NL.
serial.ECHONL = 64
-- Disable flush after interrupt or quit.
serial.NOFLSH = 128
-- Send SIGTTOU for background output.
serial.TOSTOP = 256
-- Enable implementation-defined input processing.
serial.IEXTEN = 32768

return serial
