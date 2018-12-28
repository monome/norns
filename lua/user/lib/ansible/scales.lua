-- 
-- trying to split scales out from kria so 
-- they can be reused elsewhere 
--
-- however this is a first pass and is likely to change
-- 
-- original ansible code by Tehn 


local scales = {}
scales.__index = scales

SCALE_INT = {
				{2, 2, 1, 2, 2, 2, 1},-- ionian
				{2, 1, 2, 2, 2, 1, 2},-- dorian
				{1, 2, 2, 2, 1, 2, 2},-- phyrgian
				{2, 2, 2, 1, 2, 2, 1},-- lydian
				{2, 2, 1, 2, 2, 1, 2},-- mixolydian
				 {2, 1, 2, 2, 1, 2, 2},-- aeolian
				 {1, 2, 2, 1, 2, 2, 2} -- locrian
 }

for idx = 1,16 do
	scales[idx] = {}
	if idx < 8 then 
		-- copy in scale data for the first 7
		for sdx = 1,7 do
			scales[idx][sdx] = SCALE_INT[idx][sdx]
		end
	else
		-- for the rest just use the first for now
		for sdx = 1,7 do
			scales[idx][sdx] = SCALE_INT[1][sdx]
		end
	end
end

return scales
