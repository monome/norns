--- experimental voice allocation module
-- @classmod voice
-- @alias Voice


-- (voice) Slot class
local Slot = {}
Slot.__index = Slot

function Slot.new(pool, id)
	local o = setmetatable({}, Slot)
	o.pool = pool
	o.id = id
	o.active = false
	o.on_release = nil
	o.on_steal = nil
	return o
end

function Slot:release()
	self.pool:release(self)
end


-- Rotate allocation class
local Rotate = {}
Rotate.__index = Rotate

function Rotate.new(polyphony, slots)
	local o = setmetatable({}, Rotate)
	o.polyphony = polyphony
	o.slots = slots
	o.n = 0
	return o
end

function Rotate:next()
	-- voices isn't used
	local i = self.n + 1
	if i > self.polyphony then
		i = 1
	end
	self.n = i
	return self.slots[i]
end


-- Random allocation class
local Random = {}
Random.__index = Random

function Random.new(polyphony, slots)
	local o = setmetatable({}, Random)
	o.polyphony = polyphony
	o.slots = slots
	return o
end

function Random:next()
	-- voices isn't used
	return self.slots[math.random(self.polyphony)]
end


-- LRU allocation class
local LRU = {}
LRU.__index = LRU

function LRU.new(polyphony, slots)
	local o = setmetatable({}, LRU)
	o.slots = slots
	o.count = 0
	for _, s in pairs(slots) do
		s.n = 0    -- yuck: add field to slot
	end
	return o
end

function LRU:next()
	local count = self.count + 1
	local next = self.slots[1]
	local free = nil

	self.count = count

	for _, slot in pairs(self.slots) do
		if not slot.active then
			if free == nil or slot.n < free.n then
				free = slot
			end
		elseif slot.n < next.n then
			next = slot
		end
	end

	-- choose free voice if possible
	if free then next = free end
	next.n = count
	return next
end

-- Voice class
local Voice = {}
Voice.__index = Voice

-- constants
Voice.MODE_ROTATE = 1
Voice.MODE_LRU = 2
Voice.MODE_RANDOM = 3

--- create a new Voice
-- @tparam number polyphony
-- @tparam number mode
-- @treturn Voice
function Voice.new(polyphony, mode)
	local o = setmetatable({}, Voice)
	o.polyphony = polyphony
	o.mode = mode
	o.will_steal = nil      -- event callback
	o.will_release = nil     -- event callback
	o.pairings = {}
	local slots = {}
	for id = 1, polyphony do
		slots[id] = Slot.new(o, id)
	end
	if mode == Voice.MODE_ROTATE then
		o.style = Rotate.new(polyphony, slots)
	elseif mode == Voice.MODE_RANDOM then
		o.style = Random.new(polyphony, slots)
	else
		o.style = LRU.new(polyphony, slots)
	end
	return o
end

--- get next available voice Slot from pool, stealing an active slot if needed
function Voice:get()
	local slot = self.style:next()
	if slot.active then
		if self.will_steal then self.will_steal(slot) end

		-- ack; nuke any existing pairings
		for key, value in pairs(self.pairings) do
			if value == slot then
				self.pairings[key] = nil
				break
			end
		end

		if slot.on_steal then 
			slot.on_steal(slot)
		elseif slot.on_release then
			slot.on_release(slot)
		end
	end
	slot.active = true
	return slot
end


--- push
function Voice:push(key, slot)
	self.pairings[key] = slot
end

--- pop
function Voice:pop(key)
	local slot = self.pairings[key]
	self.pairings[key] = nil
	return slot
end

--- return voice slot to pool
-- @param slot : a Slot obtained from get()
function Voice:release(slot)
	if slot.pool == self then
		if self.will_release then self.will_release(slot) end
		if slot.on_release then slot.on_release(slot) end
		slot.active = false
	else
		print("voice slot: ", slot, "does not belong to pool: ", self)
	end
end


return Voice
