-- UDP Server/Client application

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

dofile "T009/udp.lua"

mainframe = TitleFrame:new(0,0,180,80,nil,"Main Menu")
mainframe.startserver = mainframe:add(
  Button:new(5,25,170,25,"Start server"))
mainframe.startclient = mainframe:add(
  Button:new(5,50,170,25,"Start client"))

--------- logging component
function createLogLabel (...)
  local label = Label:new(...)
  label:setAlignment(
    Label.LABEL_ALIGNLEFT,Label.LABEL_ALIGNTOP)

  function label:reset() self.loglines = {} end

  function label:log(fmt,...)
    local line = select('#',...)==0 and tostring(fmt) 
      or fmt:format(...)
    local str = self:wrapLine(line)
    for nl,line in str:gmatch("(\n?)([^\n]*)") do
      if #nl+#line>0 then
        table.insert(self.loglines,line)
      end
    end
    self:scrollto()
  end
  function label:scrollto(line)
    line = line or #self.loglines
    local lines = {}
    for i=math.max(1,line-self:getMaxLines()+1),line do
      lines[#lines+1] = self.loglines[i]
    end
    self:setText(table.concat(lines,"\n"))
  end
  label:reset()
  return label
end

mainframe.serverlog = mainframe:add(
  createLogLabel(10,28,160,0))
 
------- moving the frame
local function window_mousePressed(self,me,contains)
  if not contains then return end
  self:getParent():moveZ(self,1) -- move to front
  if me.y<25 then
    self:lockMouse()
    self.mouselockpos = {me.x,me.y}
  end
end

function window_mouseReleased(self,me)
  if self:isMouseLocker() then self:unlockMouse() end
end

function window_mouseMoved(self,me)
  if self:isMouseLocker() then
    local x,y = self:getLocation()
    local dx,dy = 
      me.x-self.mouselockpos[1], 
      me.y-self.mouselockpos[2]
    self:setLocation(x+dx,y+dy)
  end
end

mainframe.mousePressed = window_mousePressed
mainframe.mouseReleased = window_mouseReleased
mainframe.mouseMoved = window_mouseMoved

-------- runing the server
function startserver ()
  mainframe.serverruns = true
  local function logger(...)
    mainframe.serverlog:log(...)
  end
  local function closed ()
    return not mainframe.serverruns
  end
  local function broadcast ()
  end
  server (logger,closed,broadcast)
  Timer.remove("Server")
  mainframe.serverruns = nil
end

------- start / stop the server
function mainframe.startserver:onClicked()
  if mainframe.serverruns==nil then
    mainframe:setSize(180,300)  
    mainframe.startserver:setText("Stop Server")
    mainframe.startserver:setLocation(5,245)
    mainframe.startclient:setLocation(5,270)
    mainframe.serverlog:setSize(160,210)
    
    mainframe.serverlog:scrollto()
    Timer.set("Server",startserver,50)
  else
    mainframe:setSize(180,80)
    mainframe.serverruns = false
    mainframe.startserver:setText("Start Server")
    mainframe.startserver:setLocation(5,25)
    mainframe.startclient:setLocation(5,50)
    mainframe.serverlog:setSize(160,0)
  end
end

------- start a client
function mainframe.startclient:onClicked()
  local window = TitleFrame:new(
    mainframe:getX()+mainframe:getWidth(),mainframe:getY(),
    200,150,nil,"Client")
  Container.getRootContainer():add(window,1)
  window.mousePressed = window_mousePressed
  window.mouseReleased = window_mouseReleased
  window.mouseMoved = window_mouseMoved
  
  local close = window:add(Button:new(168,4,30,20,"exit"))
  
  
  local running
  
  local conpanel = window:add(GroupFrame:new(10,40,180,80))
  conpanel:add(Label:new(10,10,160,16,"Server adress:"))
  local serveradr = conpanel:add(TextField:new(10,26,160,20))
  local connect = conpanel:add(Button:new(100,48,70,20,
    "connect"))
  serveradr:setText("localhost")
  
  local chatpanel = GroupFrame:new(4,24,194,124)
  local log = chatpanel:add(createLogLabel(5,5,170,90))
  local sendtx = chatpanel:add(TextField:new(5,95,120,20))
  local send = chatpanel:add(Button:new(122,95,40,20,"Send"))
  local bye = chatpanel:add(Button:new(160,95,30,20,"Bye"))
  
  local sendqueue = {}
  
  function send:onClicked()
    table.insert(sendqueue,sendtx:getText())
    sendtx:setText("")
  end
  
  sendtx.onAction = send.onClicked
  
  
  function bye:onClicked()
    running = false
  end
  
  function close:onClicked()
    running = false
    window:remove()
  end
  
  function connect:onClicked()
    if running~=nil then return end
    
    running = true
    
    conpanel:remove()
    window:add(chatpanel)
    
    local function closeit () return not running end
    local function receiver (...) log:log(...) end
    local function sender () 
      return table.remove(sendqueue,1)
    end
    
    TimerTask.new(
      function ()
        client(serveradr:getText(),receiver,sender,closeit)
        running = nil
        chatpanel:remove()
        window:add(conpanel)
      end,50)
  end
  
end

Container.getRootContainer():add(mainframe)

MouseCursor.showMouse(true)

-- cleanup for tutorial framework
return function() Timer.remove("Server") end