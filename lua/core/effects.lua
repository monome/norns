local Effects = {}

function Effects.aux_fx_on()
   set_aux_fx_on()
end

function Effects.aux_fx_off()
   set_aux_fx_off()
end

function Effects.aux_fx_input_level(chan, val)
   set_aux_fx_input_level(chan, val)
end

function Effects.aux_fx_input_pan(chan, val)
   set_aux_fx_input_pan(chan, val)
end

function Effects.aux_fx_output_level(val)
   set_aux_fx_output_level(val)
end

function Effects.aux_fx_return_level(val)
   set_aux_fx_return_level(val)
end

function Effects.aux_fx_param(name, val)
   set_aux_fx_param(name, val)
end

function Effects.insert_fx_on()
   set_insert_fx_on()
end

function Effects.insert_fx_off()
   set_insert_fx_off()
end

function Effects.insert_fx_mix(val)
   set_insert_fx_mix(val)
end

function Effects.insert_fx_param(name, val)
   set_insert_fx_param(name, val)
end

return Effects
