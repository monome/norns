--- "just" some intonation tables
-- @module intonation
-- @alias JI

local JI = {}

--- 12-tone scales
-- @section

--- small variation of ptolemaic 5-limit with closer minor 7th.
-- taking cue from jeff snyder and calling this "normal" since it is most commonly used.
-- also known as "Duodene" by Alexander Ellis (19th c.)
-- @treturn table
function JI.normal() 
   return { 
      1/1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 15/9, 15/8
   }
end


--- ptolemaic 12-tone (5-limit) - very similar except for kinda weird m7
-- @treturn table
function JI.ptolemy() 
   return { 
      1/1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8
   }
end

--- ben johnston's overtone scale
-- identical to jeff snyder's "otonal" scale
-- @treturn table
function JI.overtone() 
   return {
      1/1, 17/16, 9/8, 19/16, 5/4, 21/16, 11/8, 3/2, 13/8, 27/16, 7/4, 15/8
   }
end

--- subharmonic mirror of the overtone scale
-- jeff calls this "utonal" after partch
-- @treturn table
function JI.undertone()
   return { 
      1/1, 16/15, 8/7, 32/27, 16/13, 4/3, 16/11, 32/21, 8/5, 32/19, 16/9, 32/17,
   }
end

--- lamonte young's 'well-tuned piano' (very eccentric)
-- @treturn table
function JI.lamonte() 
   return {
      1/1, 567/512, 9/8, 147/128, 21/16, 1323/1024, 189/128, 3/2, 49/32, 7/4, 441/256, 63/32
   }
end



--- higher-tone scales / gamuts
-- @section

--- gioseffo zarlino's 16-tone (5-limit)
-- @treturn table
function JI.zarlino() 
   return {
      1/1, 25/24, 10/9, 9/8, 32/27, 6/5, 5/4, 4/3, 25/18, 45/32, 3/2, 25/16, 5/3, 16/9, 9/5, 15/8
   }
end


--- harry partch 43-tone (11-limit, plus some)
-- @treturn table
function JI.partch() 
   return {
      1/1, 
      81/80, 
      33/32, 
      21/20, 
      16/15, 
      12/11, 
      11/10, 
      10/9, 
      9/8, 
      8/7, 
      7/6, 
      32/27, 
      6/5, 
      11/9, 
      5/4, 
      14/11, 
      9/7, 
      21/16, 
      4/3, 
      27/20, 
      11/8, 
      7/5, 
      10/7, 
      16/11, 
      40/27, 
      3/2, 
      32/21, 
      14/9, 
      11/7, 
      8/5, 
      18/11, 
      5/3, 
      27/16, 
      12/7, 
      7/4, 
      16/9, 
      9/5, 
      20/11, 
      11/6, 
      15/8, 
      40/21, 
      64/33, 
      160/81
   }
end

--- finally, jeff snyder's full 168-tone gamut
-- see his disseration for reference:
-- http://scatter.server295.com/full-dissertation.pdf
-- @treturn table

function JI.gamut()
   return {
      1/1 ,
      256/255,
      96/95,
      81/80,
      65/64,
      64/63,
      45/44,
      40/39,
      33/32,
      25/24,
      21/20,
      20/19,
      256/243,
      135/128,
      19/18,
      18/17,
      17/16,
      16/15,
      15/14,
      13/12,
      12/11,
      35/32,
      128/117,
      11/10,
      10/9,
      285/256,
      64/57,
      9/8,
      96/85,
      17/15,
      256/225,
      585/512,
      8/7,
      55/48,
      15/13,
      64/55,
      7/6,
      75/64,
      20/17,
      45/38,
      32/27,
      1215/1024,
      19/16,
      153/128,
      6/5,
      40/33,
      39/32,
      128/105,
      11/9,
      315/256,
      16/13,
      5/4,
      64/51,
      24/19,
      512/405,
      81/64,
      19/15,
      80/63,
      51/40,
      32/25,
      9/7,
      165/128,
      128/99,
      13/10,
      21/16,
      256/195,
      675/512,
      45/34,
      85/64,
      4/3,
      171/128,
      128/95,
      27/20,
      65/48,
      256/189,
      15/11,
      48/35,
      11/8,
      18/13,
      25/18,
      7/5,
      80/57,
      45/32,
      24/17,
      17/12,
      64/45,
      57/40,
      10/7,
      36/25,
      13/9,
      16/11,
      35/24,
      22/15,
      189/128,
      96/65,
      40/27,
      95/64,
      765/512,
      256/171,
      3/2,
      128/85,
      195/128,
      32/21,
      20/13,
      99/64,
      256/165,
      14/9,
      25/16,
      80/51,
      63/40,
      30/19,
      128/81,
      405/256,
      19/12,
      51/32,
      8/5,
      45/28,
      13/8,
      512/315,
      18/11,
      105/64,
      64/39,
      33/20,
      5/3,
      855/512,
      256/153,
      32/19,
      27/16,
      17/10,
      128/75,
      12/7,
      55/32,
      45/26,
      26/15,
      96/55,
      7/4,
      225/128,
      30/17,
      85/48,
      16/9,
      57/32,
      512/285,
      9/5,
      20/11,
      117/64,
      64/35,
      11/6,
      945/512,
      24/13,
      28/15,
      15/8,
      32/17,
      17/9,
      36/19,
      256/135,
      243/128,
      19/10,
      40/21,
      48/25,
      495/256,
      64/33,
      39/20,
      63/32,
      128/65,
      160/81,
      2025/1024,
      95/48,
      255/128
   }
end

return JI
