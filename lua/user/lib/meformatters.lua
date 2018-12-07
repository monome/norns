local MEFormatters = {}

function MEFormatters.format_freq(freq)
  if freq < 0.1 then
    freq = util.round(freq, 0.001) .. " Hz"
  elseif freq < 100 then
    freq = util.round(freq, 0.01) .. " Hz"
  elseif util.round(freq, 1) < 1000 then
    freq = util.round(freq, 1) .. " Hz"
  else
    freq = util.round(freq / 1000, 0.01) .. " kHz"
  end
  return freq
end

function MEFormatters.format_freq_param(param)
  local value = param:get()
  return MEFormatters.format_freq(value)
end

function MEFormatters.format_secs(secs)
  if util.round(secs, 0.01) >= 1 then
    secs = util.round(secs, 0.1)
  else
    secs = util.round(secs, 0.01)
    if string.len(secs) < 4 then secs = secs .. "0" end
  end
  return secs .. " s"
end

function MEFormatters.format_secs_param(param)
  local value = param:get()
  return MEFormatters.format_secs(value)
end

return MEFormatters
