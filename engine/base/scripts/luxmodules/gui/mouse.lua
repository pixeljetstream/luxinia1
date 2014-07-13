MouseEvent = { -- I am currently using primes since I can find out
 -- the flags by dividing with modulo == 0
	MOUSE_MOVED = 2,
	MOUSE_CLICKED = 3,
	MOUSE_RELEASED = 5,
	MOUSE_PRESSED = 7,
	MOUSE_DRAGGED = 11,
	MOUSE_WHEEL = 13,
	MOUSE_ENTER = 17,
	MOUSE_EXIT = 19,
	MOUSE_DOUBLECLICK = 23,
	MOUSE_BTN1 = 2,
	MOUSE_BTN2 = 3,
	MOUSE_BTN3 = 5,
	doubleclicksensitivity = .275,
}

LuxModule.registerclass("gui","MouseEvent",
	[[A mouseevent is produced whenever the mouse is moved, dragged, clicked or
	hits an object. A MouseEvent cannot be modified once it was created.
	
	The x=0, y=0 coordinate is top-left, positive values go down / right. The event producing
	source might set the coordinate's origin to a different location. For example, the events that 
	come from the GUI (i.e. Component) are in local component coordinates, where the topmost-leftmost 
	corner is 0,0. The mouseevents that are created by the MouseCursor mouselisteners are in window 
	coordinates.
	]],MouseEvent,{}) 
LuxModule.registerclassfunction("MOUSE_MOVED","[int] - eventtype if the mouse was moved")
LuxModule.registerclassfunction("MOUSE_DRAGGED","[int] - eventtype if the mouse was moved while holding a button")
LuxModule.registerclassfunction("MOUSE_CLICKED","[int] - eventtype if a mousebutton was pressed and released without moving")
LuxModule.registerclassfunction("MOUSE_PRESSED","[int] - eventtype if a mousebutton is pressed down")
LuxModule.registerclassfunction("MOUSE_RELEASED","[int] - eventtype if the mouse was released")
LuxModule.registerclassfunction("MOUSE_WHEEL","[int] - eventtype if the mouse wheel was moved")
LuxModule.registerclassfunction("MOUSE_ENTER","[int] - eventtype if the mouse entered the source")
LuxModule.registerclassfunction("MOUSE_EXIT","[int] - eventtype if the mouse exited the source")
LuxModule.registerclassfunction("MOUSE_DOUBLECLICK","[int] - eventtype if the mouse event was a doubleclick")
LuxModule.registerclassfunction("doubleclicksensitivity","[float=.275] - Minimum time for doubleclick sensitivity")

LuxModule.registerclassfunction("x","{[int]} - position of the mouse when mouseevent was produced. Depending on the event producer, these can be local (component) coordinates. (0,0) is top left, (+,+) is bottom right. ")
LuxModule.registerclassfunction("y","{[int]} - position of the mouse when mouseevent was produced. Depending on the event producer, these can be local (component) coordinates.")
LuxModule.registerclassfunction("px","{[int]} - previous position of the mouse when mouseevent was produced")
LuxModule.registerclassfunction("py","{[int]} - previous position of the mouse when mouseevent was produced")
LuxModule.registerclassfunction("button","{[int]} - button that produced the mouseevent")
LuxModule.registerclassfunction("eventtype","{[int]} - eventtype of the occured mouseevent")
LuxModule.registerclassfunction("src","{[int]} - source object that produced the mouseevent if available")
LuxModule.registerclassfunction("downsince","{[int]} - since which frame the mouse button was hold down")
LuxModule.registerclassfunction("wheel","{[int]} - wheel position of mouse")
LuxModule.registerclassfunction("time","{[int]} - time in seconds when event occurred")
LuxModule.registerclassfunction("pwheel","{[int]} - previously wheel position of mouse")
LuxModule.registerclassfunction("wheelmove","{[int]} - difference between current and previous wheelposition")

LuxModule.registerclassfunction("new",
	"(MouseEvent):(int x,y,wheel,previousx,previousy,previouswheel,button,eventtype,any src,downsince,time) - creates a new mouseevent object")
function MouseEvent.new (x,y,wheel,px,py,pwheel,button,eventtype,src,downsince,time)
	assert(src)
	assert (px and py)
	assert (x and y)
	local self = {
		x = x, y = y, wheel = wheel, pwheel = pwheel, wheelmove = wheel - pwheel,
		px = px, py = py,
		button = button or -1, eventtype = eventtype, src = src,
		downsince = downsince or -1,
		time = time or (system.getmillis()/1000),
	}
	setmetatable(self,{__index = MouseEvent, --__newindex = function() error("cannot assign new values to mouselistener") end,
		__tostring = function (self) return MouseEvent.toString(self) end})
	return self
end

LuxModule.registerclassfunction("move",
	[[(MouseEvent):(MouseEvent self, int x,y) - returns new Mouseevent
	with translated coordinates with given x,y]])
function MouseEvent:move(x,y,wheel)
	return MouseEvent.new(self.x+x,self.y+y, self.wheel + (wheel or 0),self.px+x,self.py+y,self.pwheel + (wheel or 0),
		self.button,self.eventtype,self.src,self.downsince,self.time)
end

LuxModule.registerclassfunction("set",
	[[(MouseEvent):(MouseEvent self, int x,y,[wheel]) - returns new Mouseevent
	with new coordinates set to x and y, respecting the previous
	position.]])
function MouseEvent:set(x,y, wheel)
	return MouseEvent.new(x,y,(self.wheel or wheel),
		x+self.px-self.x,
		y+self.py-self.y,
		(self.wheel or wheel) + self.wheelmove,
		self.button,self.eventtype,self.src,self.downsince,self.time)
end

LuxModule.registerclassfunction("isPressed",
	"(boolean):(MouseEvent) - true if the mouse was pressed")
function MouseEvent.isPressed (self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_PRESSED)==0
end


LuxModule.registerclassfunction("isDoubleClick",
	"(boolean):(MouseEvent) - true if the mouse was doubleclicked")
function MouseEvent.isDoubleClick(self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_DOUBLECLICK)==0
end

LuxModule.registerclassfunction("isMoved",
	"(boolean):(MouseEvent) - true if the mouse was moved")
function MouseEvent.isMoved (self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_MOVED)==0
end

LuxModule.registerclassfunction("isDragged",
	"(boolean):(MouseEvent) - true if the mouse was dragged")
function MouseEvent.isDragged (self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_DRAGGED)==0
end

LuxModule.registerclassfunction("isClicked",
	"(boolean):(MouseEvent) - true if the mouse was clicked")
function MouseEvent.isClicked (self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_CLICKED)==0
end

LuxModule.registerclassfunction("isReleased",
	"(boolean):(MouseEvent) - true if the mouse was released")
function MouseEvent.isReleased (self)
	return math.mod(self.eventtype,MouseEvent.MOUSE_RELEASED)==0
end

LuxModule.registerclassfunction("isWheeled",
	"(boolean):(MouseEvent) - true if the wheel was moved")
function MouseEvent:isWheeled()
	return math.mod(self.eventtype,MouseEvent.MOUSE_WHEEL)==0
end

LuxModule.registerclassfunction("getDiff",
	"(int x,y):(MouseEvent) - returns difference between the two frames")
function MouseEvent:getDiff()
	return self.x-self.px, self.y-self.py
end


LuxModule.registerclassfunction("toString",
	[[{string}:(MouseEvent e) - returns a string representing the mouseevent.
	This function is called if the MouseEvent is converted to a string, i.e.
	when calling 'print(mymouseevent)'.]])
function MouseEvent.toString (e)
	local ev =
	{
		math.mod(e.eventtype,MouseEvent.MOUSE_MOVED)==0 and "MOVED" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_DRAGGED)==0 and "DRAGGED" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_CLICKED)==0 and "CLICKED" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_PRESSED)==0 and "PRESSED" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_RELEASED)==0 and "RELEASED" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_EXIT)==0 and "EXIT" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_ENTER)==0 and "ENTER" or "",
		math.mod(e.eventtype,MouseEvent.MOUSE_WHEEL)==0 and "WHEEL" or "",
	}
	for i,v in ipairs(ev) do if (v=="") then table.remove(ev,i) i = i-1 end end
--	for i,v in ev do _print(i,": ",v) end
	ev = table.concat(ev,",")
	return string.format("[MouseEvent: (%d, %d, %d),(%d,%d,%d) btn=%d eventtype=%s src=%s since=%d]",
		e.x,e.y,e.wheel,e.px,e.py,e.pwheel,e.button,ev,tostring(e.src),e.downsince)
end


--------------------------------------------------------------------------------
MouseListener = {}
LuxModule.registerclass("gui","MouseListener",
	[[A mouselistener reacts on certain type of events and call a eventfunction.]],MouseListener,
	{
		["1"] = "{[function]} - listener function",
		["2"] = "{[int]} - eventmask",
		onMoved = "{[boolean]} - true if the mouselistener reacts on moves",
		onClicked = "{[boolean]} - true if the mouselistener reacts on clicks",
		onReleased = "{[boolean]} - true if the mouselistener reacts on releases",
		onPressed = "{[boolean]} - true if the mouselistener reacts on presses",
		onDragged = "{[boolean]} - true if the mouselistener reacts on dragging",
		onEnter = "{[boolean]} - true if the mouselistener reacts on enterings",
		onExit = "{[boolean]} - true if the mouselistener reacts on mouseexists",
		onWheel = "{[boolean]} - true if the mouselistener reacts on wheelmoves",
	})

LuxModule.registerclassfunction("new",
	[[
	(MouseListener):(function callback, [int eventFilter]) -
	creates a new MouseListener. The eventFilter is optional and can be
	created by creating the product of the specific events of interest (i.e.
	MouseEvent.MOUSE_MOVED*MouseEvent.MOUSE_CLICKED). If eventFilter is not
	given, the listenerfunction will be called on every mouseevent.

	The callback function's signature is
	 function listener (mouselistener, mouseevent)
	]])
function MouseListener.new(callbackFN, eventFilter)
	eventFilter = eventFilter or
		(MouseEvent.MOUSE_MOVED * MouseEvent.MOUSE_CLICKED * MouseEvent.MOUSE_RELEASED *
		MouseEvent.MOUSE_PRESSED * MouseEvent.MOUSE_DRAGGED * MouseEvent.MOUSE_ENTER *
		MouseEvent.MOUSE_EXIT * MouseEvent.MOUSE_WHEEL)
	local self = {callbackFN,eventFilter,
		onMoved = math.mod(eventFilter, MouseEvent.MOUSE_MOVED)==0,
		onClicked = math.mod(eventFilter, MouseEvent.MOUSE_CLICKED)==0,
		onReleased = math.mod(eventFilter, MouseEvent.MOUSE_RELEASED)==0,
		onPressed = math.mod(eventFilter, MouseEvent.MOUSE_PRESSED)==0,
		onDragged = math.mod(eventFilter, MouseEvent.MOUSE_DRAGGED)==0,
		onEnter = math.mod(eventFilter, MouseEvent.MOUSE_ENTER)==0,
		onExit = math.mod(eventFilter, MouseEvent.MOUSE_EXIT)==0,
		onWheel = math.mod(eventFilter, MouseEvent.MOUSE_WHEEL)==0,}
	setmetatable(self,{__index = MouseListener})
	return self
end

LuxModule.registerclassfunction("matches",
	[[(boolean):(MouseListener self, MouseEvent e) - returns true if the mouselistener's
	eventmask matches the eventmask of the MouseEvent.]])
function MouseListener:matches (e)
	e = e.eventtype
	local m = math.mod
	return
		(self.onMoved and m(e,MouseEvent.MOUSE_MOVED)==0) or
		(self.onClicked and m(e,MouseEvent.MOUSE_CLICKED)==0) or
		(self.onReleased and m(e,MouseEvent.MOUSE_RELEASED)==0) or
		(self.onPressed and m(e,MouseEvent.MOUSE_PRESSED)==0) or
		(self.onDragged and m(e,MouseEvent.MOUSE_DRAGGED)==0) or
		(self.onEnter and m(e,MouseEvent.MOUSE_ENTER)==0) or
		(self.onExit and m(e,MouseEvent.MOUSE_EXIT)==0) or
		(self.onWheel and m(e,MouseEvent.MOUSE_WHEEL)==0)
end

LuxModule.registerclassfunction("eventcall",
	[[(boolean):(MouseListener self, MouseEvent e) - same as MouseListener.matches,
	but calls the listener's function instantly.]]) --']]
function MouseListener:eventcall (ev)
	local e = ev.eventtype
	local m = math.mod
	if	((self.onMoved and m(e,MouseEvent.MOUSE_MOVED)==0) or
		(self.onClicked and m(e,MouseEvent.MOUSE_CLICKED)==0) or
		(self.onReleased and m(e,MouseEvent.MOUSE_RELEASED)==0) or
		(self.onPressed and m(e,MouseEvent.MOUSE_PRESSED)==0) or
		(self.onDragged and m(e,MouseEvent.MOUSE_DRAGGED)==0) or
		(self.onEnter and m(e,MouseEvent.MOUSE_ENTER)==0) or
		(self.onExit and m(e,MouseEvent.MOUSE_EXIT)==0) or
		(self.onWheel and m(e,MouseEvent.MOUSE_WHEEL)==0) )
	then
		self[1](self,ev)
		return true
	end
	return false
end



--------------------------------------------------------------------------------
local defaultmouse

-- preload default texture into core
reschunk.usecore(true)
defmousetex = texture.load("nocompress;defaultmouse.png",false)
reschunk.usecore(false)

defaultmouse = l2dimage.new("mouse",defmousetex)--l2dtext.new("mouse",string.char(127),0)
defaultmouse:scale(16,32,1)
defaultmouse:rfNodraw(true)
defaultmouse:rfBlend(true)
defaultmouse:rsBlendmode(blendmode.decal())
defaultmouse:rfNodepthtest(true)
defaultmouse:sortid(1000000)
defaultmouse:parent(l2dlist.getroot())

local mouse = defaultmouse

local mouselisteners = {}
local mb1,mb2,mb3 = false,false,false -- mousebutton 1-3
local pmb1,pmb2,pmb3 = false,false,false --previouse buttons
local mbd1,mbd2,mbd3 = false,false,false -- mousebutton 1-3 dragged?
local mbds1,mbds2,mbds3 = -1,-1,-1 -- since when are the mousebuttons pressed?
					-- fyi: mbds = mouse button down since
local downx,downy = 0,0 -- coordinate of mouseclick

MouseCursor = {}

local noqueue = false

LuxModule.registerclass("gui","MouseCursor",
	[[
	The mousecursor is a visual representation of the mouse. The graphical
	appearance can be customized.
	]],MouseCursor,{})


LuxModule.registerclassfunction("mouseL2D",
	[[(l2dnode):([l2dnode],[hotspotx,hotspoty]) - returns the l2d that is used as
	mousecursor. If a l2dnode is passed as argument, the current
	mousecursor is replaced by the argument and the old l2dnode is
	returned. This might be the defaultmousecursor.]])
local hx,hy = 2,2
function MouseCursor.mouseL2D (l2d,x,y)
	if (l2d==nil) then return mouse end
	local prev = mouse
	hx,hy = x or 0, y or 0
	mouse = l2d
	mouse:rfNodraw(prev:rfNodraw())
	prev:rfNodraw(true)
	return prev
end

LuxModule.registerclassfunction("wasButtonPressed",
	[[(boolean):(int button) - returns true if the button (0-2)
	was pressed during the last frame]])
function MouseCursor.wasButtonPressed (btn)
	if (btn==0) then return pmb1
	elseif(btn==1) then return pmb2
	elseif(btn==2) then return pmb3 end
end

LuxModule.registerclassfunction("addMouseListener",
	[[():(MouseListener ml) - adds a mouselistener which will be called
	if the eventmask fits the mouselistener's eventmask.]])
function MouseCursor.addMouseListener (ml)
	table.insert(mouselisteners,ml)
end

LuxModule.registerclassfunction("removeMouseListener",
	[[():(MouseListener ml) - removes a mouselistener]])
function MouseCursor.removeMouseListener (ml)
	for i,v in ipairs(mouselisteners) do
		if (v == ml) then
			table.remove(mouselisteners,i)
			i = i-1
		end
	end
end

local mouseenabled = false
LuxModule.registerclassfunction("enable",
	[[([boolean enabled]):([boolean enable]) - enables or disables or
	returns current state of mousecursor. Only a enabled mouse produces
	clickevents and movementevents. If the mousecursor of the operating
	system is not shown (see MouseCursor.showMouse), a custom mousecursor
	is shown instead.]])
function MouseCursor.enable (enable)
	if (enable == nil) then return mouseenabled end
	mouse:rfNodraw(not enable)
	mouseenabled = enable
end

LuxModule.registerclassfunction("showMouse",
	[[([boolean]):([boolean]) - same as input.showMouse. If the mouse is visible,
	MouseCursor.enable will be true.]])
function MouseCursor.showMouse(show)
	if show==nil then return input.showmouse() end
	if show == input.showmouse() then return end
	mouseenabled = show
	input.showmouse( show )
end

local function rm () return MouseCursor.showMouse() end

local wheel,pwheel = input.mousewheel(),input.mousewheel()

local function mouseMoved(x,y,px,py)
	if mb1 or mb2 or mb3 then
		local dx,dy = x-downx,y-downy
		return mbd1 or mbd2 or mbd3 or dx*dx+dy*dy>4
	else
		return x~=px or y~=py
	end
	--return px~=x or py~=y
end

LuxModule.registerclassfunction("pos",
	[[([int x,y]):([int x,y],[boolean nomove]) - returns or sets current mouse cursor position.
	Generates a mouseevent that is delegated to the mousecursor's mouselistener list.
	If nomove is true, input.mousepos(x,y) is NOT called, leaving the OS cursor where it is.
	]])
local noevent
local roundoff = {0,0}
local lmx,lmy = input.mousepos()
function MouseCursor.pos(x,y,nomove,when)
	--if x and y and input.showmouse() then input.mousePos(x,y) end
	local mpx,mpy = mouse:pos()
	mpx,mpy = input.mousepos()
	if input.showmouse() or (not input.fixmouse()) then
		--mpx,mpy = input.mousepos()
		--mpx,mpy = input.mousepos()
		mpx,mpy = mpx - roundoff[1],mpy-roundoff[2]
	else
		mpx,mpy = mpx + hx, mpy + hy
	end
	local w = input.mousewheel()
	local wevent = w~=pwheel and MouseEvent.MOUSE_WHEEL or 1
	if (x and y) then
		--print("set:",x,y,lmx,lmy)
		local rw,rh = window.refsize()
--		_print(x,rw)
		--x = x<0 and 0 or x>rw and rw or x
		--y = y<0 and 0 or y>rh and rh or y
		mouse:pos(x-hx,y-hy,0)
		if input.showmouse()  then
			if not nomove then 
				input.mousepos(x,y)
			end
			mouse:pos(x,y,0)
			roundoff[1],roundoff[2] = x%1,y%1
		end

		if (mouseMoved(x,y,lmx,lmy) or wheel~=w) and not noevent then

		--print(("%d,%d | %d,%d"):format(x,y,downx,downy))
			local filter = ((mb1 or mb2 or mb3) and MouseEvent.MOUSE_DRAGGED or
				MouseEvent.MOUSE_MOVED) * wevent
			mbd1 = mbd1 or mb1
			mbd2 = mbd2 or mb2
			mbd3 = mbd3 or mb3
			
			local mouseevent = MouseEvent.new(x,y,w,lmx,lmy,wheel,-1,filter,MouseCursor,nil,when)
			lmx,lmy = x,y
			
			--local t =os.clock()
			--mouseevent.gcwatch = gcwatch(function() print(t) end)
			noevent = true
			for i,v in ipairs(mouselisteners) do
				xpcall(function ()
						if (math.mod(v[2],filter)==0) then
							v[1](v,mouseevent)
						end
					end,
					function (err)
						print("Mouselistener error:")
						print(err)
						print("----------")
						print(debug.traceback())
					end
				)
			end
			noevent = false
		end
		pwheel,wheel = wheel,w
	else
		return lmx,lmy
	end
end

function MouseCursor.noEvent(state)
	noevent = state
end

LuxModule.registerclassfunction("pollOnce",
	[[():(boolean state) - instead of using the mouse event queue, 
	the state of the mouse is polled only once per frame and appropriate MouseEvents are generated.]])
function MouseCursor.pollOnce(state)
	noqueue = state
end

local px,py = input.mousepos()
local dpx,dpy = 0,0

LuxModule.registerclassfunction("prevpos",
	[[(int x,y):() - Mouseposition in previous frame]])
function MouseCursor.prevPos ()
	local X,Y = MouseCursor.pos()
	return X-dpx,Y-dpy
end

local lastclickt=0
local lastclickx,lastclicky=-1,-1

local function cursorthink ()
	if (not MouseCursor.enable()) then return end

	local realmouse = input.showmouse() or not input.fixmouse()
	local n = 0
	local function handleevent(rx,ry,btn,down,wheelpos,when)
		n=n+1
		pmb1,pmb2,pmb3 = mb1,mb2,mb3
		local b1,b2,b3 = mb1,mb2,mb3 --down and btn==1,down and btn == 2, down and btn == 3
		if btn == 0 then b1 = down end
		if btn == 1 then b2 = down end
		if btn == 2 then b3 = down end
		
		if (noqueue) then
			b1 = input.ismousedown(0)
			b2 = input.ismousedown(1)
			b3 = input.ismousedown(2)
		end
		
		local c1,c2,c3 = mb1~=b1, mb2~=b2, mb3~=b3
		
		if (noqueue) then
			local cw = input.mousewheel()~=pwheel
			local cx = math.abs(px-rx)
			local cy = math.abs(py-ry)
			if (not (cx > 0 or cy > 0 or c1 or c2 or c3 or cw)) then
				return
			end
		end
		
		--print(("%.3d,%.3d,%d,%d,%d"):format(x,y,c1 and 1 or 0,mb2 and 1 or 0,mb3 and 1 or 0))
		local dx,dy
		dx,dy = (rx-px),(ry-py)
		dpx,dpy = dx,dy
		local x,y = MouseCursor.pos()

		if (realmouse)or input.fixmouse() then
			x,y = rx,ry--input.mousepos()

			MouseCursor.pos(x,y,true,when)
			--input.mousePos(x,y)
			--print(x,y,input.mousepos())
		end



		if not mb1 and not mb2 and not mb3 then
			downx,downy = rx,ry
		end


		--px,py,x,y = x,y, x+dx,y+dy


		if not realmouse then MouseCursor.pos(x,y,true,when) end



		local function btnchange (btn,now,dragged,since)
		--print(now)
			local doubleclick = 1
			if not now then
				local tdif  = os.clock()-lastclickt
				if x==lastclickx and lastclicky==y and tdif<MouseEvent.doubleclicksensitivity then
					doubleclick = MouseEvent.MOUSE_DOUBLECLICK
				end
				--print(tdif,x,y,lastclickx,lastclicky)
				lastclickx,lastclicky = x,y
				lastclickt = os.clock()
			end

			local f1 = (now and MouseEvent.MOUSE_PRESSED or MouseEvent.MOUSE_RELEASED)
			local f2 = not dragged and not now and MouseEvent.MOUSE_CLICKED*MouseEvent.MOUSE_RELEASED or 127
			local filter =
				not now and (not dragged and
						MouseEvent.MOUSE_RELEASED*MouseEvent.MOUSE_CLICKED*doubleclick
					or MouseEvent.MOUSE_RELEASED) or
				MouseEvent.MOUSE_PRESSED
			local mouseevent = MouseEvent.new(rx,ry,wheel,px,py,pwheel,btn,filter,MouseCursor,since,when)
			--print(mouseevent:isDoubleClick(),mouseevent:isClicked())
--print(mouseevent:isClicked())
			for i,v in ipairs(mouselisteners) do
				if (math.mod(v[2],f1)==0 or math.mod(v[2],f2)==0) then
					--v[1](v,mouseevent)
					xpcall(function () v[1](v,mouseevent) end,
						function (err)
							print("Mouselistener error:")
							print(err)
							print("----------")
							print(debug.traceback())
						end
					)
				end
			end
		end
		if (c1) then btnchange(0,b1,mbd1,mbds1) mbd1 = false end
		if (c2) then btnchange(1,b2,mbd2,mbds2) mbd2 = false end
		if (c3) then btnchange(2,b3,mbd3,mbds3) mbd3 = false end

		local frame = when --Timer.frame()
		mbds1,mbds2,mbds3 = b1 and mbds1 or frame, b2 and mbds2 or frame, b3 and mbds3 or frame
		mb1,mb2,mb3 = b1,b2,b3

		px,py = x,y
	end


	local ev
	local n = 0
	
	if (noqueue) then
		for rx,ry,btn,down,when in input.popmouse do
			n = n+1
			assert(n<256,"Internal Mousequeue error, inform devs please")
		end
		
		local rx,ry = input.mousepos()
		handleevent(rx,ry,0,false,nil,system.getmillis()/1000)
	else
		for rx,ry,btn,down,when in input.popmouse do
			if (not noqueue) then
				handleevent(rx,ry,btn,down,nil,when)
			end
			ev = true
			n = n+1
			assert(n<256,"Internal Mousequeue error, inform devs please")
		end
		
		if not ev and input.mousewheel()~=pwheel then
			local rx,ry = input.mousepos()
			handleevent(rx,ry,0,false,nil,system.getmillis()/1000)
		end
	end

	--iprint(n)
--[=[
	pmb1,pmb2,pmb3 = mb1,mb2,mb3
	local b1,b2,b3 = input.ismousedown(0),input.ismousedown(1),input.ismousedown(2)
	local c1,c2,c3 = mb1~=b1, mb2~=b2, mb3~=b3
	mb1,mb2,mb3 = b1,b2,b3



	local realmouse = input.showmouse() or not input.fixmouse()
	local x,y = input.mousepos()
	local dx,dy
	dx,dy,px,py = (x-px),(y-py),x,y
	dpx,dpy = dx,dy
--	_print(x,px,y,py)
	local x,y = MouseCursor.pos()

	if (realmouse) then
		x,y = input.mousepos()

		MouseCursor.pos(x,y)
		--input.mousepos(x,y)
		--print(x,y,input.mousepos())
	end

	local px,py,x,y = x,y, x+dx,y+dy
	if not realmouse then MouseCursor.pos(x,y) end

	if not mb1 and not mb2 and not mb3 then
		downx,downy = x,y
	end

	local function btnchange (btn,now,dragged,since)

		local f1 = now and MouseEvent.MOUSE_PRESSED or MouseEvent.MOUSE_RELEASED
		local f2 = not dragged and not now and MouseEvent.MOUSE_CLICKED*MouseEvent.MOUSE_RELEASED or 127
		local filter =
			not now and (not dragged and
					MouseEvent.MOUSE_RELEASED*MouseEvent.MOUSE_CLICKED
				or MouseEvent.MOUSE_RELEASED) or
			MouseEvent.MOUSE_PRESSED
		local mouseevent = MouseEvent.new(x,y,wheel,px,py,pwheel,btn,filter,MouseCursor,since)


		for i,v in ipairs(mouselisteners) do
			if (math.mod(v[2],f1)==0 or math.mod(v[2],f2)==0) then
				--v[1](v,mouseevent)
				xpcall(function () v[1](v,mouseevent) end,
					function (err)
						print("Mouselistener error:")
						print(err)
						print("----------")
						print(debug.traceback())
					end
				)
			end
		end
	end
	if (c1) then btnchange(0,b1,mbd1,mbds1) mbd1 = false end
	if (c2) then btnchange(1,b2,mbd2,mbds2) mbd2 = false end
	if (c3) then btnchange(2,b3,mbd3,mbds3) mbd3 = false end

	local frame = Timer.frame()
	mbds1,mbds2,mbds3 = b1 and mbds1 or frame, b2 and mbds2 or frame, b3 and mbds3 or frame
]=]
end

Timer.set("_gui_mousecursor",cursorthink)
