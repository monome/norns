--- USB serial device interface
-- @module serial

local serial = {
    _handlers={},
}

--- add a new serial device handler
function serial.handler(args)
    assert(serial._handlers[args.id] == nil, "duplicate serial handler id: " .. args.id)
    serial._handlers[args.id] = args
end

--- send a message to a serial device
function serial.send(dev, line)
    _norns.serial_send(dev, line)
end

_norns.serial = {}

function _norns.serial.config(vendor, model)
    for id, handler in pairs(serial._handlers) do
        local config = handler.configure(vendor, model)
        if config then
            return id, config
        end
    end
    return nil
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
--- START character
serial.VSTART = 8
--- STOP character
serial.VSTOP = 9
--- SUSP character
serial.VSUSP = 10
--- EOL character
serial.VEOL = 11

--- iflag constants
-- @section iflag_constants

--- Ignore break condition.
serial.IGNBRK = 0000001
--- Signal interrupt on break.
serial.BRKINT = 0000002
--- Ignore characters with parity errors.
serial.IGNPAR = 0000004
--- Mark parity and framing errors.
serial.PARMRK = 0000010
--- Enable input parity check.
serial.INPCK = 0000020
--- Strip 8th bit off characters.
serial.ISTRIP = 0000040
--- Map NL to CR on input.
serial.INLCR = 0000100
--- Ignore CR.
serial.IGNCR = 0000200
--- Map CR to NL on input.
serial.ICRNL = 0000400
--- Map uppercase characters to lowercase on input (not in POSIX).
serial.IUCLC = 0001000
--- Enable start/stop output control.
serial.IXON = 0002000
--- Enable any character to restart output.
serial.IXANY = 0004000
--- Enable start/stop input control.
serial.IXOFF = 0010000
--- Ring bell when input queue is full (not in POSIX).
serial.IMAXBEL = 0020000
--- Input is UTF8 (not in POSIX).
serial.IUTF8 = 0040000

--- oflag constants
-- @section oflag_constants

--- Post-process output.
serial.OPOST = 0000001
--- Map lowercase characters to uppercase on output. (not in POSIX).
serial.OLCUC = 0000002
--- Map NL to CR-NL on output.
serial.ONLCR = 0000004
--- Map CR to NL on output.
serial.OCRNL = 0000010
--- No CR output at column 0.
serial.ONOCR = 0000020
--- NL performs CR function.
serial.ONLRET = 0000040
--- Use fill characters for delay.
serial.OFILL = 0000100
--- Fill is DEL.
serial.OFDEL = 0000200
--- Select vertical-tab delays:
serial.VTDLY = 0040000
--- Vertical-tab delay type 0.
serial.VT0 = 0000000
--- Vertical-tab delay type 1.
serial.VT1 = 0040000

--- speed constants
-- @section speed_constants

--- hang up
serial.B0 = 0000000
--- 50 baud
serial.B50 = 0000001
--- 75 baud
serial.B75 = 0000002
--- 110 baud
serial.B110 = 0000003
--- 134.5 baud
serial.B134 = 0000004
--- 150 baud
serial.B150 = 0000005
--- 200 baud
serial.B200 = 0000006
--- 300 baud
serial.B300 = 0000007
--- 600 baud
serial.B600 = 0000010
--- 1200 baud
serial.B1200 = 0000011
--- 1800 baud
serial.B1800 = 0000012
--- 2400 baud
serial.B2400 = 0000013
--- 4800 baud
serial.B4800 = 0000014
--- 9600 baud
serial.B9600 = 0000015
--- 19200 baud
serial.B19200 = 0000016
--- 38400 baud
serial.B38400 = 0000017
--- 57600 baud
serial.B57600 = 0010001
--- 115200 baud
serial.B115200 = 0010002
--- 230400 baud
serial.B230400 = 0010003
--- 460800 baud
serial.B460800 = 0010004
--- 500000 baud
serial.B500000 = 0010005
--- 576000 baud
serial.B576000 = 0010006
--- 921600 baud
serial.B921600 = 0010007
--- 1000000 baud
serial.B1000000 = 0010010
--- 1152000 baud
serial.B1152000 = 0010011
--- 1500000 baud
serial.B1500000 = 0010012
--- 2000000 baud
serial.B2000000 = 0010013
--- 2500000 baud
serial.B2500000 = 0010014
--- 3000000 baud
serial.B3000000 = 0010015
--- 3500000 baud
serial.B3500000 = 0010016
--- 4000000 baud
serial.B4000000 = 0010017

--- cflag constants
-- @section cflag_constants

--- Character size
serial.CSIZE = 0000060
--- 5 bits.
serial.CS5 = 0000000
--- 6 bits.
serial.CS6 = 0000020
--- 7 bits.
serial.CS7 = 0000040
--- 8 bits.
serial.CS8 = 0000060
--- Send two stop bits, else one.
serial.CSTOPB = 0000100
--- Enable receiver.
serial.CREAD = 0000200
--- Parity enable.
serial.PARENB = 0000400
--- Odd parity, else even.
serial.PARODD = 0001000
--- Hang up on last close.
serial.HUPCL = 0002000
--- Ignore modem status lines.
serial.CLOCAL = 0004000

--- lflag constants
-- @section lflag_constants

-- Enable signals.
serial.ISIG = 0000001
-- Canonical input (erase and kill processing).
serial.ICANON = 0000002
-- Enable echo.
serial.ECHO = 0000010
-- Echo erase character as error-correcting backspace.
serial.ECHOE = 0000020
-- Echo KILL.
serial.ECHOK = 0000040
-- Echo NL.
serial.ECHONL = 0000100
-- Disable flush after interrupt or quit.
serial.NOFLSH = 0000200
-- Send SIGTTOU for background output.
serial.TOSTOP = 0000400
-- Enable implementation-defined input processing.
serial.IEXTEN = 0100000

return serial
