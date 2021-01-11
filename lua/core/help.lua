local help_topics = [[
--------------------------------------------------------------------------------
help(topic): grid, clock
--------------------------------------------------------------------------------
]]

function help(topic)
  if topic == nil then
    print(help_topics)
  elseif type(topic)=="table" then
    if topic.help then
      print(topic.help)
    end
  end
end
