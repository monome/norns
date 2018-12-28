-- port of Kria from ansible_grid.c 
--
-- original kria by Tehn at monome 
--
--  
--	passing thought - the IIC stuff should be OSC instead for this?? 
--
--  plan is for this to be a library that can be included/modulated
--  in other scripts
--  see how far I get in this first go 

local scales = require 'ansible/scales'
local tabutil = require 'tabutil'

local function newempty(n,v)
	if v == nil then
		v = 0
	end
	local t = {}
	for idx = 1,n do
		t[idx] = v
	end
	return t
end

local kria = {}
kria.__index = kria

local kriadata = {}
kriadata.__index = kriadata

local kriatrack = {}
kriatrack.__index = kriatrack

local kriapattern = {}
kriapattern.__index = kriapattern

-- modes & constants
kria.mTr = 1
kria.mNote = 2
kria.mOct = 3
kria.mDur = 4
kria.mRpt = 5
kria.mAltNote = 6
kria.mGlide = 7
kria.mScale = 8
kria.mPattern = 9

-- TODO - make sure these are consistently 
-- used to make it easier to modify 
-- to change these settings
kria.NUM_PATTERNS = 16
kria.NUM_TRACKS = 4
kria.NUM_PARAMS = 7 
kria.GRID_PRESETS = 8
kria.GRID_KEY_HOLD_TIME = 3

kria.modNone = 1
kria.modLoop = 2
kria.modTime = 3
kria.modProb = 4

-- grid light level constants
kria.L0 = 4
kria.L1 = 8
kria.L2 = 12 

function kriatrack.new()
  local i = {}
  setmetatable(i, kriatrack)
	i.tr = newempty(16)
	i.oct = newempty(16,3)
	i.note = newempty(16)
	i.dur = newempty(16)
	i.rpt = newempty(16,1)
	i.alt_note = newempty(16)
	i.glide = newempty(16)
	i.p = newempty(kria.NUM_PARAMS)
	for idx = 1,kria.NUM_PARAMS do
		i.p[idx] = newempty(16,3)
	end
	i.dur_mul = 4
	i.lstart = newempty(kria.NUM_PARAMS,1)
	i.lend = newempty(kria.NUM_PARAMS,6)
	i.llen = newempty(kria.NUM_PARAMS,6)
	i.lswap = newempty(kria.NUM_PARAMS)
	i.tmul = newempty(kria.NUM_PARAMS,1)
  return i
end

function kriatrack:copy()
	local i = {}
	setmetatable(i,kriatrack)
	i.tr = newempty(16)
	i.oct = newempty(16)
	i.note = newempty(16)
	i.dur = newempty(16)
	i.rpt = newempty(16,1)
	i.alt_note = newempty(16)
	i.glide = newempty(16)
	for idx = 1,16 do
		i.tr[idx] = self.tr[idx]
		i.oct[idx] = self.oct[idx]
		i.note[idx] = self.note[idx]
		i.dur[idx] = self.dur[idx]
		i.rpt[idx] = self.rpt[idx]
		i.alt_note[idx] = self.alt_note[idx]
		i.glide[idx ] = self.glide[idx]
	end
	i.p = {}
	i.lstart = newempty(kria.NUM_PARAMS,1)
	i.lend = newempty(kria.NUM_PARAMS,6)
	i.llen = newempty(kria.NUM_PARAMS,6)
	i.lswap = newempty(kria.NUM_PARAMS)
	i.tmul = newempty(kria.NUM_PARAMS,1)
	for idx = 1,kria.NUM_PARAMS do
		i.p[idx] = {}
		for jdx = 1,16 do
			i.p[idx][jdx] = self.p[idx][jdx]
		end
		i.lstart[idx] = self.lstart[idx]
		i.lend[idx] = self.lend[idx]
		i.llen[idx] = self.llen[idx]
		i.lswap[idx] = self.lswap[idx]
		i.tmul[idx] = self.tmul[idx]
	end
	i.dur_mul = self.dur_mul
	return i
end

function kriapattern.new()
	local i = {}
	setmetatable(i,kriapattern)
  i.t = {} 
	for tdx = 1,kria.NUM_TRACKS do
		i.t[tdx] = kriatrack.new()
	end
	i.scale = 1
	return i
end

function kriapattern:copy()
	local i = {}
	setmetatable(i,kriapattern)
  i.t = {} 
	for tdx = 1,kria.NUM_TRACKS do
		i.t[tdx] = self.t[tdx]:copy()
	end
	i.scale = self.scale
	return i
end

function kriadata.new()
	local t = {}
	setmetatable(t,kriadata)
  t.p =  {}
	for idx = 1,kria.NUM_PATTERNS do 
		t.p[idx] = kriapattern.new()
	end
	t.glyph = newempty(8,0)
	for idx = 1,8 do
		t.glyph[idx] = newempty(8,0)
	end
	return t
end

-- TODO - tidy up anything I've not 
-- used - I've basically put all the 
-- global variables from the module 
-- here - they are mostly needed though I think 
function kria.new()
  local i = {}
  setmetatable(i, kria)
	i.preset_mode = false
	i.key_count = 0
	i.held_keys = {}
	i.key_times = newempty(kria.NUM_PATTERNS)
	i.clock_external = false
	i.view_clock = false 
	i.view_config = false
	i.clock_period = 60
  i.clock_count = 0
	i.clock_mul = 1
	i.ext_clock_count = 0
	i.ext_clock_phase = 0
	i.time_rough = 0
	i.time_fine = 0

	i.cue = false
	i.meta = false
	-- scales 
	i.mode = kria.mTr
	i.mod_mode = kria.modNone
	i.mode_is_alt = false
	i.kria_mutes = newempty(kria.NUM_TRACKS,0)
  i.kria_blinks = newempty(kria.NUM_TRACKS)
  i.blink_timers = {}
	i.blink = 1
	i.track = 1
  -- state stuff from ansible.c 
	i.clock_period = 60
	i.preset = 1
	i.note_sync = true 
	i.loop_sync = 2
	i.cue_div = 0
	i.cue_steps = 3
  i.presetstore = {}
	for idx = 1,kria.GRID_PRESETS do 
		i.presetstore[idx] = kriadata.new()
	end	
  -- --------------------------------------
  i.cue_sub_count = 0
  i.cue_count = 0
	i.cue_pat_next = 0
	i.pos = newempty(kria.NUM_TRACKS)
	i.pos_mul = newempty(kria.NUM_TRACKS)
	for idx = 1,kria.NUM_TRACKS do
		i.pos[idx] = newempty(kria.NUM_PARAMS,1)	
		i.pos_mul[idx] = newempty(kria.NUM_PARAMS)	
	end
	-- key state stuff 
	i.loop_edit = 0
	i.loop_first = 0
	i.loop_last = 0
	i.last_clocks = newempty(kria.NUM_TRACKS)
	i.clock_deltas = newempty(kria.NUM_TRACKS)
  i.dur = newempty(kria.NUM_TRACKS)
  i.oct = newempty(kria.NUM_TRACKS)
  i.note = newempty(kria.NUM_TRACKS)
  i.rpt = newempty(kria.NUM_TRACKS)
  i.alt_note = newempty(kria.NUM_TRACKS)
  i.glide = newempty(kria.NUM_TRACKS)
	i.pos_reset = true
	i.activeRpt = newempty(kria.NUM_TRACKS)
	i.repeats = newempty(kria.NUM_TRACKS)
	i.cur_scale = newempty(7)
	-- meta stuff
	i.meta_pat = newempty(64,1)
	i.meta_steps = newempty(64,7)
	i.meta_start = 1
	i.meta_end = 4
	i.meta_len = 4 
	i.meta_lswap = 0
  i.meta_count = 1
  i.meta_pos = 1
	i.meta_edit = 1
	i.meta_next = 0
	i.pattern = 1
  i.keytimermetro = nil
  return i
end


function kria:keytimer()
  if self.key_count > 0 then
					for idx = 0,self.key_count-1   do
						if self.key_times[self.held_keys[idx+1]] > 0 then
							self.key_times[self.held_keys[idx+1]] = self.key_times[self.held_keys[idx+1]] - 1
							if self.key_times[self.held_keys[idx+1]] == 0 then
								if self.mode == kria.mPattern then
									local tmp = self.held_keys[idx+1] 
									if tmp < 16 then
									  print("Copy " .. self.pattern .. " to " .. tmp)
										self:k().p[tmp] = self:k().p[self.pattern]:copy()
										self.pattern = tmp
									end
								end
							end
						end
					end
	end
end

function kria:init(note_callback)
	self.callback = note_callback
  self.keytimermetro = metro.alloc(function() self:keytimer() end)
  self.keytimermetro:start(0.3)
  self.blink_timers = {}
	for idx = 1,kria.NUM_TRACKS do
		self.blink_timers[idx] = metro.alloc(function () self.kria_blinks[idx] = 0 end )
	end
	self.blink = 1
  self.blinker = metro.alloc(function() if self.blink == 1 then self.blink = 0 else self.blink = 1 end end )
  self.blinker:start(0.5)
end

-- some accessor functions to stop me 
-- going mad 

-- k() returns current preset 
function kria:k()
	return self.presetstore[self.preset]
end

-- p() returns the current pattern
-- of the current preset
function kria:p()
	return self:k().p[self.pattern]
end

--- Clocking stuff first 
--  expects clock function to be called for each 'beat' 
--  from outside the package

function kria:clock()
	self.clock_count = self.clock_count + 1
  self.cue_sub_count = self.cue_sub_count + 1

	if self.cue_sub_count >= self.cue_div + 1 then
		self.cue_sub_count = 0
		self.cue_count = self.cue_count + 1
		if self.cue_count >= self.cue_steps + 1 then  
						self.cue_count = 0

						if self.meta then  
								self.meta_count = self.meta_count + 1
								if self.meta_count > self.meta_steps[self.meta_pos] then
  									if self.meta_next > 0 then
												self.meta_pos = self.meta_next 
										elseif self.meta_pos == self.meta_end then
												self.meta_pos = self.meta_start
										else
												self.meta_pos = self.meta_pos + 1
										end
										print("changing pattern pos " .. self.meta_pos  )
										print("changing pattern pat " .. self.meta_pat[self.meta_pos] )
										self:change_pattern(self.meta_pat[self.meta_pos])
										self.meta_next = 0
										self.meta_count = 1
								end
					elseif self.cue_pat_next > 0 then 
							self:change_pattern(self.cue_pat_next);
							self.cue_pat_next = 0;
					end
		end
	end
	if self.pos_reset then
		self:reset()
	end
  for tnum = 1,4 do
		self:clock_track(tnum);
  end
end

kria.nextsteps = {
 [0] = function() return false end,
 [1] = function() return math.random() <= 0.25  end,
 [2] = function() return math.random() <= 0.5  end,
 [3] = function() return true end,
}

function kria:next_step(tnum,typ) 
	self.pos_mul[tnum][typ] = self.pos_mul[tnum][typ] + 1
	if self.pos_mul[tnum][typ] >= self:p().t[tnum].tmul[typ] then
		if self.pos[tnum][typ] == self:p().t[tnum].lend[typ] then
			self.pos[tnum][typ] = self:p().t[tnum].lstart[typ]
		else
			self.pos[tnum][typ] = self.pos[tnum][typ] + 1
			if self.pos[tnum][typ] > 16 then 
				self.pos[tnum][typ] = 1
			end
		end
		self.pos_mul[tnum][typ] = 0
		return kria.nextsteps[self:p().t[tnum].p[typ][self.pos[tnum][typ]]]()
	else
		 return false 
	end
end

function kria:clock_track(tnum)
				local now = get_time() * 0.4 
				self.clock_deltas[tnum] = now - self.last_clocks[tnum]
				self.last_clocks[tnum] = now

				if self:next_step(tnum,kria.mDur) then 
								self.dur[tnum] = self:p().t[tnum].dur[self.pos[tnum][kria.mDur]] + 1
				end
				if self:next_step(tnum,kria.mOct) then
								-- TODO - make octaves more midi friendly 
								self.oct[tnum] = self:p().t[tnum].oct[self.pos[tnum][kria.mOct]]
				end
				if self:next_step(tnum,kria.mNote) then 
								self.note[tnum] = self:p().t[tnum].note[self.pos[tnum][kria.mNote]] 
				end
				if self:next_step(tnum,kria.mRpt) then
								self.rpt[tnum] = self:p().t[tnum].rpt[self.pos[tnum][kria.mRpt]]
				end
				if self:next_step(tnum,kria.mAltNote) then
								self.alt_note[tnum] = self:p().t[tnum].alt_note[self.pos[tnum][kria.mAltNote]] 
				end
				if self:next_step(tnum,kria.mGlide) then
								self.glide[tnum] = self:p().t[tnum].glide[self.pos[tnum][kria.mGlide]]
				end
				-- and all of that is great but
				-- do we play a note or not? 
				if self:next_step(tnum,kria.mTr) then
					if self.kria_mutes[tnum] ~= 1 and self:p().t[tnum].tr[self.pos[tnum][kria.mTr]] == 1 then
						self.activeRpt[tnum] = self.rpt[tnum]
						self.repeats[tnum] = self.rpt[tnum] - 1
						local noteinscale = ((self.note[tnum] + self.alt_note[tnum]) % 7) + 1
						local octave = math.floor(((self.note[tnum] + self.alt_note[tnum]) / 7) + self.oct[tnum])
						if self.callback ~= nil then
							self.callback(tnum,noteinscale,octave,self.dur[tnum],self:p().t[tnum].tmul[kria.mTr],self.repeats[tnum],self.glide[tnum])
						end
						-- blink the track button 
						-- on trigger
						-- TODO - do we put the time estimation stuff in 
						-- maybe needed for repeats
						self.kria_blinks[tnum] = 1
						self.blink_timers[tnum]:start(math.max(self.dur[tnum],0.31),1)
					end

				end

end

function kria:reset()
	self.clock_count = 0
  for tnum = 1,4 do
		for idx = kria.mTr,kria.mPattern do
			self.pos[tnum][idx] = self:p().t[tnum].lend[idx]
			self.pos_mul[tnum][idx] = self:p().t[tnum].tmul[idx]
		end
	end
	self.cue_count = 0
	self.cue_sub_count = 0
	self.pos_reset = false
end

function kria:change_pattern(x) 
	self.pattern = x
	self.pos_reset = true
	self:calc_scale(self:p().scale)
end


-- tables for key functions 
-- this is how Lua does switch statements
-- each entry provides a 
-- function which takes (kria,x,y,z)
-- and is called by the event function 
-- or another table entry
--
-- can't do the bitwise trickery that C allows
-- I've tended towards explicit in this code 
-- one - because I prefer that and two - hopefully
-- those that come after will find it easier to understand
function kria.inv(v) if v == 1 then return 0 else return 1 end end
-- metatable to provide a null event function 
-- not all the grid switches ignore keys that 
-- don't do anything 
kria.br_mt = { __index = function(table,key) return function(k,x,y,z) end end } 

kria.bottom_row_down = {
		[1] = function(k,x,y,z) 
			if k.mod_mode == kria.modLoop then 
				k.kria_mutes[x] = kria.inv(k.kria_mutes[x])
				print("Kria mute for " .. x .. " is " .. k.kria_mutes[x] )
			else
				k.track = x
			end
		end,
		[6] = function(k,x,y,z)
			if k.mode == kria.mTr then
					k.mode = kria.mRpt
			else
					k.mode = kria.mTr
			end
		end,
		[7] = function(k,x,y,z) 
			if k.mode == kria.mNote then
					k.mode = kria.mAltNote
			else
					k.mode = kria.mNote
			end
		end,
		[8] = function(k,x,y,z) 
			if k.mode == kria.mOct then
					k.mode = kria.mGlide
			else
					k.mode = kria.mOct
			end
		end,
		[9] = function(k,x,y,z) 
			k.mode = kria.mDur
		end,
		[11] = function(k,x,y,z) 
			k.mod_mode = kria.modLoop
			k.loop_count = 0
		end,
		[12] = function(k,x,y,z) 
			k.mod_mode = kria.modTime
		end,
		[13] = function(k,x,y,z) 
			k.mod_mode = kria.modProb
		end,
		[15] = function(k,x,y,z) 
			k.mode = kria.mScale
		end,
		[16] = function(k,x,y,z) 
			k.mode = kria.mPattern
			k.cue = true
		end,
		default = function(k,x,y,z) 

		end,
}
-- Lua likes everything explicit 
-- guess that's a good thing 
kria.bottom_row_down[2] = kria.bottom_row_down[1]
kria.bottom_row_down[3] = kria.bottom_row_down[1]
kria.bottom_row_down[4] = kria.bottom_row_down[1]
setmetatable(kria.bottom_row_down,kria.br_mt)

kria.bottom_row_up = {
		[11] = function(k,x,y,z) 
			k.mod_mode = kria.modNone
		end,
		[16] = function(k,x,y,z) 
			k.cue = false
		end,
		default = function(k,x,y,z)
		end,
}
kria.bottom_row_up[12] = kria.bottom_row_up[11]
kria.bottom_row_up[13] = kria.bottom_row_up[11]
setmetatable(kria.bottom_row_up,kria.br_mt)

-- all the pages next 
--
-- Tr page 
--
kria.tr_page = {
	[kria.modNone] = function(k,x,y,z) 
		-- why doesn't the original code constrain 
		-- this to y in top 4 rows? 
		if z == 1 and y < 5 then 
			k:p().t[y].tr[x] = kria.inv(k:p().t[y].tr[x])
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
		-- in this version only one set of triggers 
		-- on that first page
		if z == 1 and y < 5   then 
			if k.loop_count == 0 then
				k.loop_edit = y
				k.loop_first = x
				k.loop_last = -1
			else
				k.loop_last = x
				k:update_loop_start(k.loop_edit,k.loop_first,kria.mTr)
				k:update_loop_end(k.loop_edit,k.loop_last,kria.mTr)
			end
			k.loop_count = k.loop_count + 1
		elseif k.loop_edit == y then 
			k.loop_count = k.loop_count - 1
			if k.loop_count == 0 then 
				if k.loop_last == -1 then 
					if k.loop_first == k:p().t[k.loop_edit].lstart[kria.mTr] then 
						k:update_loop_start(k.loop_edit,k.loop_first,kria.mTr)
						k:update_loop_end(k.loop_edit,k.loop_first,kria.mTr)
					else
						k:update_loop_start(k.loop_edit,k.loop_first,kria.mTr)
					end
				end
			end
		end
	end,
	[kria.modTime] = function(k,x,y,z)
		if z == 1 then 
		 k:p().t[k.track].tmul[kria.mTr] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
		 k:p().t[k.track].p[kria.mTr][x] = 6 - y
		end
	end,
}

kria.note_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then 
			if k.note_sync then 
				if k:p().t[k.track].tr[x] and k:p().t[k.track].note[x] == 7 - y then
					k:p().t[k.track].tr[x] = 0
				else
					k:p().t[k.track].tr[x] = 1
					k:p().t[k.track].note[x] = 7 - y
				end
			else 
					k:p().t[k.track].note[x] = 7 - y
			end
		end
	end,
	[kria.modLoop] = function(k,x,y,z) 
		if z == 1 then 
			if k.loop_count == 0 then
				k.loop_first = x
				k.loop_last = -1
			else
				k.loop_last = x
				k:update_loop_start(k.track,k.loop_first,kria.mNote)
				k:update_loop_end(k.track,k.loop_last,kria.mNote)
			end
			k.loop_count = k.loop_count + 1
		else 
			k.loop_count = k.loop_count - 1
			if k.loop_count == -1 then
				if k.loop_first == k:p().t[k.track].lstart[kria.mNote] then
					k:update_loop_start(k.track,k.loop_first,kria.mNote)
					k:update_loop_end(k.track,k.loop_last,kria.mNote)
				else
					k:update_loop_start(k.track,k.loop_first,kria.mNote)
				end
			end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mNote] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mNote][x] = 6 - y
		end
	end,
}

kria.oct_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then
			-- if y > 3 then 
			k:p().t[k.track].oct[x] = 7 - y
			-- end
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
		if z == 1 then
			if k.loop_count == 0 then
				k.loop_first = x 
				k.loop_last = -1
			else
				k.loop_last = x
				k:update_loop_start(k.track,k.loop_first,kria.mOct)
				k:update_loop_end(k.track,k.loop_last,kria.mOct)
			end
			k.loop_count = k.loop_count + 1
		else
			k.loop_count = k.loop_count - 1
			if k.loop_count == 0 then
				if k.loop_last == -1 then
					if k.loop_first == k:p().t[k.track].lstart[kria.mOct] then
						k:update_loop_start(k.track,k.loop_first,kria.mOct)
						update_loop_end(k.track,k.loop_last,kria.mOct)
					else
						k:update_loop_start(k.track,k.loop_first,kria.mOct)
					end
				end
			end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mOct] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mOct][x] = 6 - y
		end
	end,
}

kria.dur_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then
			if y == 1  then 
				k:p().t[k.track].dur_mul = x 
			else 
				k:p().t[k.track].dur[x] = y - 2
			end
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
		if y > 1 then 
			if z == 1 then
				if k.loop_count == 0 then
					k.loop_first = x 
					k.loop_last = -1
				else
					k.loop_last = x
					k:update_loop_start(k.track,k.loop_first,kria.mDur)
					k:update_loop_end(k.track,k.loop_last,kria.mDur)
				end
				k.loop_count = k.loop_count + 1
			else
				k.loop_count = k.loop_count - 1
				if k.loop_count == 0 then
					if k.loop_last == -1 then
						if k.loop_first == k:p().t[k.track].lstart[kria.mDur] then
							k:update_loop_start(k.track,k.loop_first,kria.mDur)
							k:update_loop_end(k.track,k.loop_last,kria.mDur)
						else
							k:update_loop_start(k.track,k.loop_first,kria.mDur)
						end
					end
				end
			end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mDur] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mDur][x] = 6 - y
		end
	end,
}

kria.rpt_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then
			if y > 2 and y < 7  then 
				k:p().t[k.track].rpt[x] = 7 - y
			end
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
			if z == 1 then
				if k.loop_count == 0 then
					k.loop_first = x 
					k.loop_last = -1
				else
					k.loop_last = x
					k:update_loop_start(k.track,k.loop_first,kria.mRpt)
					k:update_loop_end(k.track,k.loop_last,kria.mRpt)
				end
				k.loop_count = k.loop_count + 1
			else
				k.loop_count = k.loop_count - 1
				if k.loop_count == 0 then
					if k.loop_last == -1 then
						if k.loop_first == k:p().t[k.track].lstart[kria.mRpt] then
							k:update_loop_start(k.track,k.loop_first,kria.mRpt)
							k:update_loop_end(k.track,k.loop_last,kria.mRpt)
						else
							k:update_loop_start(k.track,k.loop_first,kria.mRpt)
						end
					end
				end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mRpt] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mRpt][x] = 6 - y
		end
	end,
}

kria.alt_note_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then
				k:p().t[k.track].alt_note[x] = 7 - y
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
			if z == 1 then
				if k.loop_count == 0 then
					k.loop_first = x 
					k.loop_last = -1
				else
					k.loop_last = x
					k:update_loop_start(k.track,k.loop_first,kria.mAltNote)
					k:update_loop_end(k.track,k.loop_last,kria.mAltNote)
				end
				k.loop_count = k.loop_count + 1
			else
				k.loop_count = k.loop_count - 1
				if k.loop_count == 0 then
					if k.loop_last == -1 then
						if k.loop_first == k:p().t[k.track].lstart[kria.mAltNote] then
							k:update_loop_start(k.track,k.loop_first,kria.mAltNote)
							k:update_loop_end(k.track,k.loop_last,kria.mAltNote)
						else
							k:update_loop_start(k.track,k.loop_first,kria.mAltNote)
						end
					end
				end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mAltNote] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mAltNote][x] = 6 - y
		end
	end,
}

kria.glide_page = {
	[kria.modNone] = function(k,x,y,z)
		if z == 1 then
				k:p().t[k.track].glide[x] = 7 - y
		end
	end,
	[kria.modLoop] = function(k,x,y,z)
			if z == 1 then
				if k.loop_count == 0 then
					k.loop_first = x 
					k.loop_last = -1
				else
					k.loop_last = x
					k:update_loop_start(k.track,k.loop_first,kria.mGlide)
					k:update_loop_end(k.track,k.loop_last,kria.mGlide)
				end
				k.loop_count = k.loop_count + 1
			else
				k.loop_count = k.loop_count - 1
				if k.loop_count == 0 then
					if k.loop_last == -1 then
						if k.loop_first == k:p().t[k.track].lstart[kria.mGlide] then
							k:update_loop_start(k.track,k.loop_first,kria.mGlide)
							k:update_loop_end(k.track,k.loop_last,kria.mGlide)
						else
							k:update_loop_start(k.track,k.loop_first,kria.mGlide)
						end
					end
				end
		end
	end,
	[kria.modTime] = function(k,x,y,z) 
		if z == 1 then
			k:p().t[k.track].tmul[kria.mGlide] = x 
		end
	end,
	[kria.modProb] = function(k,x,y,z)
		if z == 1 and y > 2 and y < 7 then 
			k:p().t[k.track].p[kria.mGlide][x] = 6 - y
		end
	end,
}

function kria.scale_page(k,x,y,z) 
	if z == 1 then 
		if x < 9 then 
			-- yes you can do fancy calculations 
			-- to do this in a one-er 
			-- but can't be bothered debuging 
			-- off by one errors
			if y == 6 then
				k:p().scale = x
			elseif y == 7 then
				k:p().scale = x + 8
			end
		else
			scales[k:p().scale][8-y]=x-8
		end

	end
end

function kria.pattern_page(k,x,y,z)
		if y > 2 and y < 7 then 
			if z == 1 and k.cue then
				k.meta_next = (y - 3) * 16 + x 
				if k.meta_next > k.meta_end then 
						self.meta_next = 1
				end
			elseif k.mod_mode == kria.modLoop then 
					if z == 1 then 
						if k.loop_count == 0 then
							k.loop_first = (y - 3 ) * 16 + x
							k.loop_last = -1
						else
							k.loop_last = (y - 3) * 16 + x
							k:update_meta_start(k.loop_first)
							k:update_meta_end(k.loop_last)
  					end
						k.loop_count = k.loop_count + 1
					else 
						k.loop_count = k.loop_count - 1
						if k.loop_count == 0 then 
							if k.loop_last == -1 then 
								if k.loop_first == k.meta_first then 
									k:update_meta_start(k.loop_first)
									k:update_meta_end(k.loop_first)
								else
									k:update_meta_start(k.loop_first)
							  end
							end
						end
					end
		  elseif z == 1 then 
				k.meta_edit = (y - 3) * 16 + x
			end
		elseif z == 1 and y == 7 then 
			if k.cue then 
				k.meta = kria.inv(k.meta)
			else
				k.meta_steps[k.meta_edit] = x
			end
		elseif z == 1 and y == 2 then 
			if k.mod_mode == kria.modTime then 
				k.cue_div = x - 1
			else
				k.cue_steps = x - 1
			end
		end
end

kria.page_events = {
	[kria.mTr] = function(k,x,y,z) 
		return k.tr_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mNote] = function(k,x,y,z)
		return k.note_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mOct] = function(k,x,y,z)
		return k.oct_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mDur] = function(k,x,y,z)
		return k.dur_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mRpt] = function(k,x,y,z)
		return k.rpt_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mAltNote] = function(k,x,y,z) 
		return k.alt_note_page[k.mod_mode](k,x,y,z)
	end, 
	[kria.mGlide] = function(k,x,y,z) 
		return k.glide_page[k.mod_mode](k,x,y,z)
	end,
	[kria.mScale] = function(k,x,y,z)
		return k.scale_page(k,x,y,z)
	end,
	[kria.mPattern] = function(k,x,y,z) 
		return k.pattern_page(k,x,y,z)
	end, 


}

function kria:event(x,y,z)
	local index = (y - 1) *16 + x
	-- track long presses
	if z == 1 then 
		self.held_keys[self.key_count+1] = index 
		self.key_count = self.key_count + 1
		self.key_times[index] = kria.GRID_KEY_HOLD_TIME
	else
		local found = false
		for idx = 0,self.key_count do
			if self.held_keys[idx+1] == index then
							found = true
			end
			if found then
				table.remove(self.held_keys,idx+1)
				break
			end
		end
		self.key_count = self.key_count - 1
		-- fast press 
		if self.key_times[index] > 0 then
			if self.preset_mode then
				if x == 1 then
					if y ~= self.preset then
						self.preset = y
					else
						-- in the module 
						-- this recalls the preset as saved
						-- but I'm not working like that for 
						-- the moment 
						--
					end
				  self.preset_mode = false
				end
			elseif self.mode == kria.mPattern then
				if y == 1 then
					if self.meta ~= 1 then
						if self.cue then 
							self.cue_pat_next = x
						else
							self:change_pattern(x)
						end
					else
						self.meta_pat[self.meta_edit] = x
					end
				end
			end
		end
	end
	-- config and clock modes are elsewhere 
	-- so just the regular screens here
	if self.preset_mode then 
		if x > 8 and z == 1 then 
			self:k().glyph[x-8][y] = kria.inv(self:k().glyph[x-8][y])
		end
	elseif y == 8 then
			if z == 1 then
				kria.bottom_row_down[x](self,x,y,z)
			else
				kria.bottom_row_up[x](self,x,y,z)
			end
			if self.mode == kria.mRpt or self.mode == kria.mAltNote or self.mode == kria.mGlide then
				self.mode_is_alt = true
			else
				self.mode_is_alt = false
			end
	else
		kria.page_events[self.mode](self,x,y,z)	   		
	end

end

function kria:update_meta_start(x)
	print("Update meta start " .. x )
	local temp

	temp = self.meta_pos - self.meta_start + x
	if temp < 0 then 
			temp = temp + 64
	elseif temp > 63 then
			temp = temp - 64
	end
	self.meta_next = temp + 1
	self.meta_start = x
	temp = x + self.meta_len - 1
	if temp > 63 then 
			self.meta_end = temp - 64
			self.meta_lswap = 1
	else
			self.meta_end = temp 
			self.lswap = 0
	end
end

function kria:update_meta_end(x)
	print("Update meta end " .. x )
	local temp

	self.meta_end = x
	temp = self.meta_end - self.meta_start 
	if temp < 0 then 
		 self.meta_len = temp + 65
		 self.meta_lswap = 1
	else
		 self.meta_len = temp + 1
		 self.meta_lswap = 0
	end
	temp = self.meta_pos
	if self.meta_lswap == 1 then
		if temp < self.meta_start and temp > self.meta_end then
				self.meta_next = self.meta_start + 1
		end 
	else
		if temp < self.meta_start or temp > self.meta_end then 
				self.meta_next = self.meta_start + 1
		end
	end
	print("Meta len " .. self.meta_len )
	print("Meta lswap " .. self.meta_lswap )
	print("Meta next " .. self.meta_next )
end

function kria:adjust_loop_start(t,x,m) 
	local temp = self.pos[t][m] - self:p().t[t].lstart[m] + x
	if temp < 1 then 
		temp = temp + 16
	elseif temp > 16 then
		temp = temp - 16
	end
	self.pos[t][m] = temp
	self:p().t[t].lstart[m] = x
	temp = x + self:p().t[t].llen[m] - 1
	if temp > 16 then 
		self:p().t[t].lend[m] = temp - 16
		self:p().t[t].lswap[m] = 1
	else
		self:p().t[t].lend[m] = temp 
		self:p().t[t].lswap[m] = 0
	end
end

function kria:adjust_loop_end(t,x,m)
	self:p().t[t].lend[m] = x
	local temp = self:p().t[t].lend[m] - self:p().t[t].lstart[m]
	if temp < 1 then
		self:p().t[t].llen[m] = temp + 17
		self:p().t[t].lswap[m] = 1
	else
		self:p().t[t].llen[m] = temp + 1
		self:p().t[t].lswap[m] = 0
	end
	temp = self.pos[t][m]
	if self:p().t[t].lswap[m] == 1 then
		if temp < self:p().t[t].lstart[m] and temp > self:p().t[t].lend[m] then
			self.pos[t][m] = self:p().t[t].lstart[m]
		end
	else
		if temp < self:p().t[t].lstart[m] or temp > self:p().t[t].lend[m] then
			self.pos[t][m] = self:p().t[t].lstart[m]
		end
	end
end


function kria:update_loop_start(t,x,m)
	if self.loop_sync == 1 then 
		for idx = 1,kria.NUM_PARAMS do
			self:adjust_loop_start(t,x,idx)
		end
	elseif self.loop_sync == 2 then 
		for tnum = 1,4 do 
			for idx = 1,kria.NUM_PARAMS do
				self:adjust_loop_start(tnum,x,idx)
			end
		end
	else 
			self:adjust_loop_start(t,x,m)
	end
end

function kria:update_loop_end(t,x,m)
	if self.loop_sync == 1 then 
		for idx = 1,kria.NUM_PARAMS do
			self:adjust_loop_end(t,x,idx)
		end
	elseif self.loop_sync == 2 then 
		for tnum = 1,4 do 
			for idx = 1,kria.NUM_PARAMS do
				self:adjust_loop_end(tnum,x,idx)
			end
		end
	else 
			self:adjust_loop_end(t,x,m)
	end
end



kria.activeModeIndex = { 
	[kria.mTr] = 6,
	[kria.mRpt] = 6,
	[kria.mNote] = 7,
	[kria.mAltNote] = 7,
	[kria.mOct] = 8,
	[kria.mGlide] = 8,
	[kria.mDur] = 9,
	[kria.mScale] = 15,
	[kria.mPattern] = 16,
}


function kria.draw_tr(k,g)
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[kria.mTr],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[kria.mTr][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[kria.mTr][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for tnum = 1,4 do 
			ledbuf[tnum] = newempty(16)
			for idx = 1,16 do 
				if k:p().t[tnum].tr[idx] > 0 then
					ledbuf[tnum][idx] = 3
				end
			end
			ledbuf[tnum][k.pos[tnum][kria.mTr]] = ledbuf[tnum][k.pos[tnum][kria.mTr]] + 4
	  end
    for tnum = 1,4 do 
			local extra = 0 
			if k.mod_mode == kria.modLoop then extra = 1 end
			if k:p().t[tnum].lswap[kria.mTr] == 1 then
				for idx = 1,k:p().t[tnum].llen[kria.mTr] do
					ledbuf[tnum][(idx + k:p().t[tnum].lstart[kria.mTr]) % 16] = ledbuf[tnum][idx] + 2 + extra
				end
			else
				for idx = k:p().t[tnum].lstart[kria.mTr],k:p().t[tnum].lend[kria.mTr] do
					ledbuf[tnum][idx] = ledbuf[tnum][idx] + 2 + extra
				end
			end
		end
		-- and write out the whole buffer 
		-- onto the actual LEDs
		for tnum = 1,4 do
			for idx = 1,16 do
				g.led(idx,tnum,ledbuf[tnum][idx])
			end
		end
	end
end

function kria.draw_note(k,g,alt)
	local noteMode 
	local notesArray
	if alt then 
		noteMode = kria.mAltNote
		notesArray = k:p().t[k.track].alt_note
	else
		noteMode = kria.mNote
		notesArray = k:p().t[k.track].note
	end
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[noteMode],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[noteMode][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[noteMode][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for idx = 1,16 do
			ledbuf[idx] = newempty(8)
		end
		if isAlt ~= true and k.note_sync then 
			for idx = 1,16 do
		 	   ledbuf[idx][7-notesArray[idx]] = k:p().t[k.track].tr[idx] * 3	
			end
		else
			for idx = 1,16 do
		 	   ledbuf[idx][7-notesArray[idx]] = 3
			end
		end
		ledbuf[k.pos[k.track][noteMode]][7-notesArray[k.pos[k.track][noteMode]]] = ledbuf[k.pos[k.track][noteMode]][7-notesArray[k.pos[k.track][noteMode]]] + 4 
		if k:p().t[k.track].lswap[noteMode] == 1 then
			local extra = 0
			if k.mod_mode == kria.modLoop then 
					extra = 1
			end
			for xdx = 1,k:p().t[k.track].llen[noteMode] do
				ledbuf[(xdx + k:p().t[k.track].lstart[noteMode]) %16 ][7-notesArray[xdx]] = ledbuf[(xdx + k:p().t[k.track].lstart[noteMode]) %16 ][7-notesArray[xdx]] + 2 + extra
			end
		else
			local extra = 0
			if k.mod_mode == kria.modLoop then 
					extra = 2
			end
			for xdx = k:p().t[k.track].lstart[noteMode],k:p().t[k.track].lend[noteMode] do
				ledbuf[xdx][7-notesArray[xdx]] = ledbuf[xdx][7-notesArray[xdx]] + 3 + extra
			end
		end
		-- and write out the whole buffer 
		-- onto the actual LEDs
		for ydx = 1,7 do
			for xdx = 1,16 do
				g.led(xdx,ydx,ledbuf[xdx][ydx])
			end
		end

	end
end

function kria.draw_alt_note(k,g)
	kria.draw_note(k,g,true)
end

function kria.draw_oct(k,g)
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[kria.mOct],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[kria.mOct][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[kria.mOct][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for idx = 1,16 do
			ledbuf[idx] = newempty(8)
		end
	  for xdx = 1,16 do
				for ydx = 7-k:p().t[k.track].oct[xdx],7 do
 	   			ledbuf[xdx][ydx] = kria.L0
				end
				if xdx == k.pos[k.track][kria.mOct] then
					ledbuf[xdx][7 - k:p().t[k.track].oct[xdx]] = ledbuf[xdx][7 - k:p().t[k.track].oct[xdx]] + 4 
				end
		end
		if k:p().t[k.track].lswap[kria.mOct] == 1 then
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mOct] and xdx > k:p().t[k.track].lend[kria.mOct] then
					for ydx = 0,k:p().t[k.track].oct[xdx] do
						ledbuf[xdx][7 - ydx] = ledbuf[xdx][7 - ydx] - 2
					end
				end
			end
		else
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mOct] or xdx > k:p().t[k.track].lend[kria.mOct] then
					for ydx = 0,k:p().t[k.track].oct[xdx] do
						ledbuf[xdx][7 - ydx] = ledbuf[xdx][7 - ydx] - 2
					end
				end
			end
		end
		-- and write out the whole buffer 
		-- onto the actual LEDs
		for ydx = 1,7 do
			for xdx = 1,16 do
				g.led(xdx,ydx,ledbuf[xdx][ydx])
			end
		end
	end
end

function kria.draw_dur(k,g)
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[kria.mDur],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[kria.mDur][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[kria.mDur][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for idx = 1,16 do
			ledbuf[idx] = newempty(8)
		end
		ledbuf[k:p().t[k.track].dur_mul][1] = kria.L1
	  for xdx = 1,16 do
				for ydx = 0,k:p().t[k.track].dur[xdx]  do
 	   			ledbuf[xdx][ydx + 2] = kria.L0
				end
				if xdx == k.pos[k.track][kria.mDur] then
					ledbuf[xdx][k:p().t[k.track].dur[xdx] + 2] = ledbuf[xdx][k:p().t[k.track].dur[xdx] + 2] + 4 
				end
		end
		if k:p().t[k.track].lswap[kria.mDur] == 1 then
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mDur] and xdx > k:p().t[k.track].lend[kria.mDur] then
					for ydx = 0,k:p().t[k.track].dur[xdx] do
						ledbuf[xdx][ydx + 2] = ledbuf[xdx][ydx + 2] - 2
					end
				end
			end
		else
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mDur] or xdx > k:p().t[k.track].lend[kria.mDur] then
					for ydx = 0,k:p().t[k.track].dur[xdx] do
						ledbuf[xdx][ydx + 2] = ledbuf[xdx][ydx + 2] - 2
					end
				end
			end
		end

		-- and write out the whole buffer 
		-- onto the actual LEDs
		for ydx = 1,7 do
			for xdx = 1,16 do
				g.led(xdx,ydx,ledbuf[xdx][ydx])
			end
		end
	end
end

function kria.draw_rpt(k,g)
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[kria.mRpt],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[kria.mRpt][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[kria.mRpt][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for idx = 1,16 do
			ledbuf[idx] = newempty(8)
		end
		for xdx = 1,16 do
			for ydx = 1,k:p().t[k.track].rpt[xdx] do
				ledbuf[xdx][8 - (ydx+1)] = kria.L0
			end
			if xdx == k.pos[k.track][kria.mRpt] then
				ledbuf[xdx][7 - (k.activeRpt[k.track] - k.repeats[k.track])] = ledbuf[xdx][7 - (k.activeRpt[k.track] - k.repeats[k.track])] + 4 
			end
		end
		if k:p().t[k.track].lswap[kria.mRpt] == 1 then
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mRpt] and xdx > k:p().t[k.track].lend[kria.mRpt] then
					for ydx = 1,k:p().t[k.track].rpt[xdx] do
						ledbuf[xdx][8 - (ydx + 1)] = ledbuf[xdx][8 - (ydx + 1)] - 2
					end
				end
			end
		else
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mRpt] or xdx > k:p().t[k.track].lend[kria.mRpt] then
					for ydx = 1,k:p().t[k.track].rpt[xdx] do
						ledbuf[xdx][8 - (ydx + 1)] = ledbuf[xdx][8 - (ydx + 1)] - 2
					end
				end
			end
		end
		-- and write out the whole buffer 
		-- onto the actual LEDs
		for ydx = 1,7 do
			for xdx = 1,16 do
				g.led(xdx,ydx,ledbuf[xdx][ydx])
			end
		end
	end
end

function kria.draw_glide(k,g)
	if k.mod_mode == kria.modTime then 
		for idx = 1,16 do
		   g.led(idx,2,3)
		end
		g.led(k:p().t[k.track].tmul[kria.mGlide],2,kria.L1)
	elseif k.mod_mode == kria.modProb then 
		for idx = 1,16 do
		   g.led(idx,6,3)
		end
		for idx = 1,16 do
			 if k:p().t[k.track].p[kria.mGlide][idx] > 0  then 
					local row = 6 - k:p().t[k.track].p[kria.mGlide][idx]
					g.led(idx,row,6)	
			 end
		end
	else 
		-- the ansible version has a buffer 
		-- for grid states that it queries
		-- we mostly don't need that but replicate 
		-- here because it is  less fiddly 
		local ledbuf = {} 
		for idx = 1,16 do
			ledbuf[idx] = newempty(8)
		end
		for xdx = 1,16 do
			for ydx = 0,k:p().t[k.track].glide[xdx] do
				ledbuf[xdx][7 - ydx] = kria.L0 - (k:p().t[k.track].glide[xdx] - ydx)
			end
			if xdx == k.pos[k.track][kria.mGlide] then
				ledbuf[xdx][7 - k:p().t[k.track].glide[xdx]] = ledbuf[xdx][7 - k:p().t[k.track].glide[xdx]] + 4 
			end
		end
		if k:p().t[k.track].lswap[kria.mGlide] == 1 then
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mGlide] and xdx > k:p().t[k.track].lend[kria.mGlide] then
					for ydx = 1,k:p().t[k.track].glide[xdx] do
						ledbuf[xdx][7 - ydx] = ledbuf[xdx][7 - ydx] - 2
					end
				end
			end
		else
			for xdx = 1,16 do
				if xdx < k:p().t[k.track].lstart[kria.mGlide] or xdx > k:p().t[k.track].lend[kria.mGlide] then
					for ydx = 0,k:p().t[k.track].glide[xdx] do
						ledbuf[xdx][7 - ydx] = ledbuf[xdx][7 - ydx] - 2
					end
				end
			end
		end

		-- and write out the whole buffer 
		-- onto the actual LEDs
		for ydx = 1,7 do
			for xdx = 1,16 do
				g.led(xdx,ydx,ledbuf[xdx][ydx])
			end
		end
	end
end

function kria.draw_scale(k,g)
	-- obviously dropping the teletype clocking stuff
	for ydx = 1,8 do
		g.led(9,ydx,kria.L0)
	end
	for xdx = 1,8 do
		g.led(xdx,6,2)
		g.led(xdx,7,2)
	end
	-- scale is 1-16 
	if k:p().scale < 9 then
	  g.led(k:p().scale,6,kria.L1)
	else
	  g.led(k:p().scale - 8,7,kria.L1)
	end
	for ydx = 1,7 do 
		g.led(8 + scales[k:p().scale][ydx],8 - ydx,kria.L1) 
	end
	for tnum = 1,kria.NUM_TRACKS do
		if k:p().t[tnum].tr[k.pos[tnum][kria.mTr]] > 0 then
		   g.led(8+ scales[k:p().scale][k.note[tnum] + 1 ],8 - (k.note[tnum] + 1),kria.L1 + 1 )
		end
	end

end

function kria.pat_pos(n)
	return (n % 16), math.floor( n / 16 )
end

function kria.draw_pattern(k,g)
	if k.meta ~= 1 then 
		for xdx = 1,16 do
			g.led(xdx,1,3)
		end
		g.led(k.pattern,1,kria.L1)
	else
		-- duration bar thing
		for xdx = 1,k.meta_steps[k.meta_pos] do
			g.led(xdx,7,3)
		end
		g.led(k.meta_count ,7,kria.L1)
		g.led(k.meta_steps[k.meta_edit],7,kria.L2)
		-- patterns on the top row
		g.led(k.pattern,1,kria.L0)
		g.led(k.meta_pat[k.meta_edit],1,kria.L1)
		-- and then the meta sequence
		local xpos,ypos
		if k.meta_lswap ~= 1 then
			for idx = k.meta_start - 1,k.meta_end - 1 do
				xpos,ypos = kria.pat_pos(idx)
				g.led(xpos + 1 , 3 + ypos ,3)
			end
		else
			for idx = 0,k.meta_end do
				xpos,ypos = kria.pat_pos(idx)
				g.led(xpos + 1 , 3 + ypos ,3)
			end
			for idx = k.meta_start - 1,64 - k.meta_start do
				xpos,ypos = kria.pat_pos(idx)
				g.led(xpos + 1 , 3 + ypos ,3)
			end
		end
		xpos,ypos = kria.pat_pos(k.meta_pos)
		g.led(xpos ,ypos + 3,kria.L1)
		xpos,ypos = kria.pat_pos(k.meta_edit)
		g.led(xpos ,ypos + 3,kria.L2)
		if k.meta_next > 0 then
			xpos,ypos = kria.pat_pos(k.meta_next)
			local brt = kria.L1
			if k.blink == 1 then
				brt = kria.L2
			end
			g.led(xpos ,ypos + 3,brt)
			g.led(k.meta_pat[k.meta_next],1,brt)
		end
	end
	if k.cue_pat_next > 0 then
		g.led(k.cue_pat_next,1,kria.L2)
	end
	if k.mod_mode == kria.modTime then
		g.led(k.cue_count+1,2,kria.L0)
		g.led(k.cue_div+1,2,kria.L1)
	else
		g.led(k.cue_steps+1,2,kria.L0)
		g.led(k.cue_count+1,2,kria.L1)
	end
end

kria.draw_pages = {
	[kria.mTr] = kria.draw_tr,
	[kria.mNote] = kria.draw_note,
	[kria.mAltNote] = kria.draw_alt_note,
	[kria.mOct] = kria.draw_oct,
	[kria.mDur] = kria.draw_dur,
	[kria.mRpt] = kria.draw_rpt,
	[kria.mGlide] = kria.draw_glide,
	[kria.mScale] = kria.draw_scale,
	[kria.mPattern] = kria.draw_pattern,
}

function kria:draw(g)
	-- this is kind of dodgy 
	-- if this function is being called then preset
	-- mode is false
	self.preset_mode = false
	g.all(0)
	-- bottom strip 
  g.led(6,8,kria.L0)
  g.led(7,8,kria.L0)
  g.led(8,8,kria.L0)
  g.led(9,8,kria.L0)

  g.led(11,8,kria.L0)
  g.led(12,8,kria.L0)
  g.led(13,8,kria.L0)
  g.led(15,8,kria.L0)
  g.led(16,8,kria.L0)

	--  track 
	for tnum = 1,kria.NUM_TRACKS do
		if self.kria_mutes[tnum] == 1 then
			if tnum == self.track then
				g.led(tnum,8,kria.L1)
			else
				g.led(tnum,8,2)
			end
		else
			local blink = self.kria_blinks[tnum] * 2
			if tnum == self.track then
				g.led(tnum,8,kria.L2 + blink)
			else
				g.led(tnum,8,kria.L0 + blink)
			end
		end
	end
	-- mode 
  local modeidx = kria.activeModeIndex[self.mode]
	if self.mode_is_alt then 
		if self.blink == 1 then
			 g.led(modeidx,8,kria.L2)
		else
			 g.led(modeidx,8,kria.L1)
		end
	else
	  g.led(modeidx,8,kria.L1)
	end
	-- mod 
	if self.mod_mode == kria.modLoop then 
		g.led(11,8,kria.L1)
	end
	if self.mod_mode == kria.modTime then 
		g.led(12,8,kria.L1)
	end
	if self.mod_mode == kria.modProb then 
		g.led(13,8,kria.L1)
	end
	kria.draw_pages[self.mode](self,g)
  g.refresh()
end

function kria:calc_scale(x)
	self.cur_scale[1] = scales[x][1]
	for idx = 2,7 do
		self.cur_scale[idx] = self.cur_scale[idx-1] + scales[x][idx]
	end
end

-- presets are kind of separate 
-- copying the module for now 
-- but this feels clunky 
-- will come back to this after the initial release 
-- 
-- this function should be called separately 
-- nothing in this file calls this 
--

function kria:draw_presets(g)
	-- this is kind of dodgy 
	-- if this function is being called then preset
	-- mode is true
	self.preset_mode = true
	g.all(0)
	-- I want the out of use presets 
	-- to be dimly glowing 
	for idx = 1,8 do
		g.led(1,idx,3)
	end
	g.led(1,self.preset,11)
	for xdx = 1,8 do 
		for ydx = 1,8 do 
			local brt = self:k().glyph[xdx][ydx] * 13
			g.led(xdx + 8,ydx,brt)
		end
	end
	g.refresh()
end


-- this function looks to see if 
-- the data file exists - and if so
-- loads itself from it. Failing that
-- it just makes a new one

function kria.loadornew(fname)
	local ret
  ret = tabutil.load(data_dir .. fname)
	if ret == nil then
		ret = kria.new()
	else
  	setmetatable(ret, kria)
		for idx = 1,kria.GRID_PRESETS do 
			setmetatable(ret.presetstore[idx],kriadata)
			for jdx = 1,kria.NUM_PATTERNS do 
				setmetatable(ret.presetstore[idx].p[jdx],kriapattern)
				for tdx = 1,kria.NUM_TRACKS do
					setmetatable(ret.presetstore[idx].p[jdx].t[tdx],kriatrack)
				end
			end
		end	
	end
	return ret
end

function kria:save(fname)
	print("Save")
	for k,v in ipairs(self) do
		print("Save " .. k)
	end
	tabutil.save(self,data_dir .. fname)
end


return kria
