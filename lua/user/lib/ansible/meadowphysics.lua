local mp = {}
mp.__index = mp

mp.MODE_POSITION = 0
mp.MODE_SPEED = 1
mp.MODE_RULES = 2

local mp_set = {}
mp_set.__index = mp_set

local gridbuf = require "gridbuf"
local gbuf = gridbuf.new(16, 8)

function mp.new()
	local m = {}
	setmetatable(m, mp)
	m.edit_row = 0
	m.key_count = 0
	m.mode = 0
	m.prev_mode = 0
	m.kcount = 0
	m.scount = {}

	m.count = {}
	m.position = {}
	m.speed = {}
	m.tick = {}
	m.min = {}
	m.max = {}
	m.trigger = {}
	m.toggle = {}
	m.rules = {}
	m.rule_dests = {}
	m.sync = {}
	m.sound = {}
	m.pushed = {}
	m.rule_dest_targets = {}
	m.smin = {}
	m.smax = {}

	for i=1,8 do
		m.count[i] = 8+i
		m.position[i] = 8+i
		m.speed[i] = 0
		m.tick[i] = 0
		m.max[i] = 8+i
		m.min[i] = 8+i
		m.trigger[i] = (1 << i)
		m.toggle[i] = 0
		m.rules[i] = 2 -- inc
		m.rule_dests[i] = i
		m.sync[i] = (1 << i)
		m.rule_dest_targets[i] = 3
		m.smin[i] = 0
		m.smax[i] = 0

		m.scount[i] = 0
	end

	m.mp_event = function(i) print("mp_event row: " .. i) end
	return m
end

function mp_set.new()
	local m = {}
	setmetatable(m, mp_set)

	m.count = {}
	m.position = {}
	m.speed = {}
	m.tick = {}
	m.min = {}
	m.max = {}
	m.trigger = {}
	m.toggle = {}
	m.rules = {}
	m.rule_dests = {}
	m.sync = {}
	m.sound = {}
	m.pushed = {}
	m.rule_dest_targets = {}
	m.smin = {}
	m.smax = {}

	for i=1,8 do
		m.count[i] = 8+i
		m.position[i] = 8+i
		m.speed[i] = 0
		m.tick[i] = 0
		m.max[i] = 8+i
		m.min[i] = 8+i
		m.trigger[i] = (1 << i)
		m.toggle[i] = 0
		m.rules[i] = 2 -- inc
		m.rule_dests[i] = i
		m.sync[i] = (1 << i)
		m.rule_dest_targets[i] = 3
		m.smin[i] = 0
		m.smax[i] = 0
	end

	return m
end

function mp:apply_rule(i)
	local rd = self.rule_dests[i]

	if self.rules[i] == 2 then -- inc
		if (self.rule_dest_targets[i] & 1) > 0 then
			self.count[rd] = self.count[rd] + 1
			if self.count[rd] > self.max[rd] then self.count[rd] = self.min[rd] end
		end

		if (self.rule_dest_targets[i] & 2) > 0 then
			self.speed[rd] = self.speed[rd] + 1
			if self.speed[rd] > self.smax[rd] then self.speed[rd] = self.smin[rd] end
		end

	elseif self.rules[i] == 3 then -- dec
		if (self.rule_dest_targets[i] & 1) > 0 then
			self.count[rd] = self.count[rd] - 1
			if self.count[rd] < self.min[rd] then self.count[rd] = self.max[rd] end
		end

		if (self.rule_dest_targets[i] & 2) > 0 then
			self.speed[rd] = self.speed[rd] - 1
			if self.speed[rd] < self.smin[rd] then self.speed[rd] = self.smax[rd] end
		end

	elseif self.rules[i] == 4 then -- max
		if (self.rule_dest_targets[i] & 1) > 0 then self.count[rd] = self.max[rd] end

		if (self.rule_dest_targets[i] & 2) > 0 then self.speed[rd] = self.smax[rd] end

	elseif self.rules[i] == 5 then -- min
		if (self.rule_dest_targets[i] & 1) > 0 then self.count[rd] = self.min[rd] end

		if (self.rule_dest_targets[i] & 2) > 0 then self.speed[rd] = self.smin[rd] end

	elseif self.rules[i] == 6 then -- rnd
		if (self.rule_dest_targets[i] & 1) > 0 then
			self.count[rd] = math.random(self.max[rd] - self.min[rd] + 1) + self.min[rd]
		end

		if (self.rule_dest_targets[i] & 2) > 0 then
			self.speed[rd] = math.random(self.smax[rd] - self.smin[rd] + 1) + self.smin[rd]
		end

	elseif self.rules[i] == 7 then -- pole
		if (self.rule_dest_targets[i] & 1) > 0 then
			if math.abs(self.count[rd] - self.min[rd]) < math.abs(self.count[rd] - self.max[rd]) then
				self.count[rd] = self.max[rd]
			else
				self.count[rd] = self.min[rd]
			end
		end

		if (self.rule_dest_targets[i] & 2) > 0 then
			if math.abs(self.speed[rd] - self.smin[rd]) < math.abs(self.speed[rd] - self.smax[rd]) then
				self.speed[rd] = self.smax[rd]
			else
				self.speed[rd] = self.smin[rd]
			end
		end

	elseif self.rules[i] == 8 then -- stop
		if (self.rule_dest_targets[i] & 1) > 0 then
			self.position[rd] = -1
		end
	end
end

function mp:clock()
	for i=1,8 do
		if self.tick[i] == 0 then
			self.tick[i] = self.speed[i]

			if self.position[i] == 1 then 
				self.mp_event(i)
				self:apply_rule(i) 
				self.position[i] = self.position[i] - 1

				for n=1,8 do
					if (self.sync[i] & (1 << n)) > 0 then
						self.position[n] = self.count[n]
						self.tick[n] = self.speed[n]
					end
				end

			elseif self.position[i] > 1 then
				self.position[i] = self.position[i] - 1
			end

		else
			self.tick[i] = self.tick[i] - 1
		end
	end
end

function mp:gridevent(x, y, z)
	self.prev_mode = self.mode

	if x == 1 then
		self.kcount = self.kcount + ((z << 1)-1)
		if selfkcount < 0 then self.kcount = 0 end

		if self.kcount == 1 and z == 1 then
			self.mode = 1
		elseif self.kcount == 0 then
			self.mode = 0
			self.scount[y] = 0
		end

		if self.mode == 1 and z == 1 then 
			self.edit_row = y 
		end

	elseif x == 2 and self.mode ~= 0 then
		if self.mode == 1 and z == 1 then 
			self.mode = 2
			self.edit_row = y
		elseif self.mode == 2 and z == 0 then
			self.mode = 1
		end

	elseif self.mode == 0 then
		self.scount[y] = self.scount[y] + ((z << 1) - 1)
		if self.scount[y] < 0 then self.scount[y] = 0 end

		if z == 1 and self.scount[y] == 1 then
			self.position[y] = x
			self.count[y] = x
			self.min[y] = x
			self.max[y] = x
			self.tick[y] = self.speed[y]

			--if self.sound then self.pushed[y] = 1 end
		elseif z == 1 and self.scount[y] == 2 then
			if x < self.count[y] then
				self.min[y] = x
				self.max[y] = self.count[y]
			else
				self.max[y] = x
				self.min[y] = self.count[y]
			end
		end
	elseif self.mode == 1 then

	elseif self.mode == 2 and z == 1 then
		if x > 4 and x < 8 then
			self.rule_dests[self.edit_row] = y
			self.rule_dest_targets[self.edit_row] = x - 4
		elseif x > 7 then
			self.rules[self.edit_row] = y
		end
	end
end

function mp:gridredraw(g)
	gbuf:led_level_all(0)

	if self.mode == mp.MODE_POSITION then
		for i=1,8 do
			for j=self.min[i],self.max[i] do
				gbuf:led_level_set(j, i, 4)
			end

			gbuf:led_level_set(self.count[i], i, 8)

			if self.position[i] >= 1 then
				gbuf:led_level_set(self.position[i], i, 12)
			end
		end
	elseif self.mode == mp.MODE_SPEED then
		for i=1,8 do 
			if self.position[i] >= 1 then gbuf:led_level_set(self.position[i], i, 4) end

			if self.position[i] ~= -1 then gbuf:led_level_set(3, i, 2) end

			for j=self.smin[i],self.smax[i] do
				gbuf:led_level_set(j+9, i, 4)
			end

			gbuf:led_level_set(self.speed[i]+9, i, 8)

			--if self.sound then gbuf:led_level_set(5, i, 2) end

			if (self.toggle[self.edit_row] & (1 << i)) > 0 then
				gbuf:led_level_set(6, i, 12)
			else
				gbuf:led_level_set(6, i, 4)
			end

			if (self.trigger[self.edit_row] & (1 << i)) > 0 then
				gbuf:led_level_set(7, i, 12)
			else
				gbuf:led_level_set(7, i, 4)
			end

			if (self.toggle[self.edit_row] & (1 << i)) > 0 then
				gbuf:led_level_set(4, i, 8)
			else
				gbuf:led_level_set(4, i, 4)
			end
		end

		gbuf:led_level_set(1, self.edit_row, 8)
	elseif mode == mp.MODE_RULES then 
		for i=1,8 do 
			if self.position[i] >= 1 then gbuf:led_level_set(self.position[i], i, 4) end
		end

		gbuf:led_level_set(1, self.edit_row, 8)
		gbuf:led_level_set(2, self.edit_row, 8)

		local rd = self.rule_dests[self.edit_row]
		if self.rule_dest_targets[self.edit_row] == 1 then
			gbuf:led_level_set(5, rd, 12)
			gbuf:led_level_set(6, rd, 4)
			gbuf:led_level_set(7, rd, 4)
		elseif self.rule_dest_targets[self.edit_row] == 2 then
			gbuf:led_level_set(5, rd, 4)
			gbuf:led_level_set(6, rd, 12)
			gbuf:led_level_set(7, rd, 4)
		else
			gbuf:led_level_set(5, rd, 12)
			gbuf:led_level_set(6, rd, 12)
			gbuf:led_level_set(7, rd, 4)

			for i=8,16 do
				gbuf:led_level_set(i, self.rules[self.edit_row], 4)
			end
		end
	end

	gbuf:render(g)
	g.refresh()
end

return mp
