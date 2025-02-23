local serial = {
    _specs={},
}

function serial.spec(args)
    serial._specs[args.id] = args
end

function serial.send(dev, line)
    _norns.serial_send(dev, line)
end

_norns.serial = {}

function _norns.serial.config(vendor, model)
    for id, spec in pairs(serial._specs) do
        local config = spec.configure(vendor, model)
        if config then
            return id, config
        end
    end
    return nil
end

function _norns.serial.add(spec_id, id, name, dev)
    serial._specs[spec_id].add(id, name, dev)
end

function _norns.serial.remove(spec_id, id)
    serial._specs[spec_id].remove(id)
end

function _norns.serial.event(spec_id, id, line)
    serial._specs[spec_id].event(id, line)
end

-- c_cc characters
serial.VINTR = 0
serial.VQUIT = 1
serial.VERASE = 2
serial.VKILL = 3
serial.VEOF = 4
serial.VTIME = 5
serial.VMIN = 6
serial.VSWTC = 7
serial.VSTART = 8
serial.VSTOP = 9
serial.VSUSP = 10
serial.VEOL = 11
serial.VREPRINT = 12
serial.VDISCARD = 13
serial.VWERASE = 14
serial.VLNEXT = 15
serial.VEOL2 = 16

-- c_iflags bits
serial.IGNBRK = 0000001 -- Ignore break condition.
serial.BRKINT = 0000002 -- Signal interrupt on break.
serial.IGNPAR = 0000004 -- Ignore characters with parity errors.
serial.PARMRK = 0000010 -- Mark parity and framing errors.
serial.INPCK = 0000020 -- Enable input parity check.
serial.ISTRIP = 0000040 -- Strip 8th bit off characters.
serial.INLCR = 0000100 -- Map NL to CR on input.
serial.IGNCR = 0000200 -- Ignore CR.
serial.ICRNL = 0000400 -- Map CR to NL on input.
serial.IUCLC = 0001000 -- Map uppercase characters to lowercase on input (not in POSIX).
serial.IXON = 0002000 -- Enable start/stop output control.
serial.IXANY = 0004000 -- Enable any character to restart output.
serial.IXOFF = 0010000 -- Enable start/stop input control.
serial.IMAXBEL = 0020000 -- Ring bell when input queue is full (not in POSIX).
serial.IUTF8 = 0040000 -- Input is UTF8 (not in POSIX).

-- c_oflag bits
serial.OPOST = 0000001 -- Post-process output.
serial.OLCUC = 0000002 -- Map lowercase characters to uppercase on output. (not in POSIX).
serial.ONLCR = 0000004 -- Map NL to CR-NL on output.
serial.OCRNL = 0000010 -- Map CR to NL on output.
serial.ONOCR = 0000020 -- No CR output at column 0.
serial.ONLRET = 0000040 -- NL performs CR function.
serial.OFILL = 0000100 -- Use fill characters for delay.
serial.OFDEL = 0000200 -- Fill is DEL.
serial.VTDLY = 0040000 -- Select vertical-tab delays:
serial.VT0 = 0000000 -- Vertical-tab delay type 0.
serial.VT1 = 0040000 -- Vertical-tab delay type 1.

-- c_cflag bit meaning
serial.B0 = 0000000 -- hang up
serial.B50 = 0000001
serial.B75 = 0000002
serial.B110 = 0000003
serial.B134 = 0000004
serial.B150 = 0000005
serial.B200 = 0000006
serial.B300 = 0000007
serial.B600 = 0000010
serial.B1200 = 0000011
serial.B1800 = 0000012
serial.B2400 = 0000013
serial.B4800 = 0000014
serial.B9600 = 0000015
serial.B19200 = 0000016
serial.B38400 = 0000017
serial.B57600 = 0010001
serial.B115200 = 0010002
serial.B230400 = 0010003
serial.B460800 = 0010004
serial.B500000 = 0010005
serial.B576000 = 0010006
serial.B921600 = 0010007
serial.B1000000 = 0010010
serial.B1152000 = 0010011
serial.B1500000 = 0010012
serial.B2000000 = 0010013
serial.B2500000 = 0010014
serial.B3000000 = 0010015
serial.B3500000 = 0010016
serial.B4000000 = 0010017

-- c_cflag bits
serial.CSIZE = 0000060
serial.CS5 = 0000000
serial.CS6 = 0000020
serial.CS7 = 0000040
serial.CS8 = 0000060
serial.CSTOPB = 0000100
serial.CREAD = 0000200
serial.PARENB = 0000400
serial.PARODD = 0001000
serial.HUPCL = 0002000
serial.CLOCAL = 0004000

-- c_lflag bits
serial.ISIG = 0000001 -- Enable signals.
serial.ICANON = 0000002 -- Canonical input (erase and kill processing).
serial.ECHO = 0000010 -- Enable echo.
serial.ECHOE = 0000020 -- Echo erase character as error-correcting backspace.
serial.ECHOK = 0000040 -- Echo KILL.
serial.ECHONL = 0000100 -- Echo NL.
serial.NOFLSH = 0000200 -- Disable flush after interrupt or quit.
serial.TOSTOP = 0000400 -- Send SIGTTOU for background output.
serial.IEXTEN = 0100000 -- Enable implementation-defined input processing.

return serial
