--
-- Page (base class)
--

local Page = sky.Object:extend()

function Page:new(model)
  self.model = model or {}
end

function Page:enter(props)
end

function Page:exit()
end

function Page:process(event, output, state, props)
  output(event) -- default behaivor is to just pass the event along
end

function Page:draw(event, props)
end


--
-- PageRouter
--

--[[

ui = sky.PageRouter{
  initial = 'play',
  pages = {
    macro = tambla.MacroPage(model),
    play = tambla.PlayPage(model),
    edit = tambla.EditPage(model),
  }
}

display = sky.Chain{
  sky.NornsDisplay{
    screen.clear,
    ui:draw_router(),
    screen.update,
  }
}

controls = sky.NornsInput{
  chain = sky.Chain{
    sky.Logger{},
    ui:process_router(),
    sky.Forward(display)
  }
}


]]--

local PageRouter = sky.Object:extend()

function PageRouter:new(props)
  self.pages = {}
  for k, v in pairs(props.pages) do
    self.pages[k] = v
  end

  self.active = nil

  self.initial = props.initial
  if self.initial ~= nil then
    self.active = self.pages[self.initial]
    if self.active == nil then
      error('initial page: ' .. self.initial .. ' did not match any of the provided pages')
    end
    self.active:enter({})
  end

  self._props = {
    select_page = function(name, ...) self:select_page(name, ...) end,
  }
end

function PageRouter:select_page(name, props)
  local p = self.pages[name]
  if p == nil then
    error('select_page failed: unknown page: ' .. name)
  end
  self.active = p
  self.active:enter(props or {})
end

function PageRouter:event_router()
  return function(event, output, state)
    if self.active ~= nil then
      self.active:process(event, output, state, self._props)
    end
  end
end

function PageRouter:draw_router()
  local this = self
  local router = sky.Object:extend()
  function router:render(event, props)
    if this.active ~= nil then
      this.active:draw(event, props)
    end
  end
  return router
end


--
-- module
--
return {
  Page = Page,
  PageRouter = PageRouter,
}