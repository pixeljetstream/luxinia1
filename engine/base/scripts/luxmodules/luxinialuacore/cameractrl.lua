



--[[
attribs.shiftmulti = 5.0
attribs.movemulti = 2.5
attribs.rotmulti = 1/16.0

attribs.invert = false
attribs.mousesense = 1.0
attribs.mousewheel = 1.0
attribs.fix = true
attribs.keys = {
	forward,backward,left,right,up,down,fast
}
-- internal
attribs.mdelta
attribs.guilock
attribs.fixpos = {x,y}
attribs.last = {mx,my}

]]

local function defkeys()
	return  {
		forward = "W",
		backward = "S",
		left = "A",
		right = "D",
		up = "SPACE",
		down = "C",
		fast = "SHIFT",
	}
end

local OLDSTYLE = true

local defattribs = {
	shiftmulti = 5.0,
	movemulti = 2.5,
	rotmulti = 0.03,
	invert = false,
	mousesense = 6.0,
	mousewheel = 1.0,
	fix = true,
	keys = defkeys(),
}



local function checkattribs(attribs)
	for i,v in pairs(defattribs) do
		if (attribs[i]==nil) then
			attribs[i] = i=="keys" and defkeys() or v
		end
	end
	
	for i,v in pairs(defattribs.keys) do
		if (attribs.keys[i]==nil) then
			attribs.keys[i] = v
		end
	end
end

CameraCtrl = {}

LuxModule.registerclass("luxinialuacore","CameraCtrl",
[[The CameraCtrl virtual class is a helper class for camera manipulation in editors and alike.
Various implementations exist. The attrib array is currently
as follows (values used depending on implementation):
* shiftmulti: float - speed multiplier when keys.fast is pressed
* movemulti: float - movement multiplier
* rotmulti: float - rotation multiplier
* invert: bool - mouse inverted (up/down)
* mousesense: float - multiplied on all mouse deltas
* mousewheel: float - multiplied on all wheel deltas
* fix: boolean - whether mouse position is fixed to action start (default true)
* keys: table - each value can be a string/keycode or table of such.
** forward
** backward
** left
** right
** up
** down
** fast
]],CameraCtrl,{})
	
LuxModule.registerclassfunction("new",
	[[(CameraCtrl):(table class, l3dcamera, actornode, table attribs) -
	creates the controller.]])
function CameraCtrl.new(class,cam,camact,attribs)
	local self = {}
	setmetatable(self,{__index = class})
	
	self.cam = cam
	self.camact = camact
	self.attribs = attribs or {}
	checkattribs(self.attribs)
	
	self.listeners = {}
	
	return self
end

LuxModule.registerclassfunction("createML",
	[[(MouseListener):(CameraCtrl, [Component]) -
	creates a mouselistener based on input. Pass the host component,
to get drag for action behavior. Or pass no host and assign to the 
MouseCursor itself, and the mouse position will be fixed at
center of screen and always firing (same when MouseCursor is disabled).]])


	
function CameraCtrl:createML(guicomp)
	if (guicomp) then
	return MouseListener.new(function (ml,me)
		local mx,my = me.x,me.y
		local attr = self.attribs
		
		if (me:isPressed()) then
			attr.fixpos = {guicomp:local2world(mx,my)}
			attr.last = nil
			guicomp:lockMouse()
			return
		end
		
		if (me:isDragged() and not me:isPressed() and 
			Component.getMouseLock() == guicomp) then

			self:mouseDelta(mx,my)
			attr.guilock = guicomp
			
		elseif Component.getMouseLock() == guicomp and not me:isPressed() then
			guicomp:unlockMouse()
			
			attr.fixpos = nil
		end
		end)
	else
	return MouseListener.new(function (ml,me)
		if (me:isMoved()) then
			local attr = self.attribs
			local wx,wy = window.refsize()
			wx,wy = math.floor(wx/2),math.floor(wy/2)
			attr.fixpos = {wx,wy}
			
			self:mouseDelta(me.x,me.y)
			
		end
		end)
	end
end

LuxModule.registerclassfunction("runThink",
	[[():(CameraCtrl) -
	The think performs the actual manipulation of the l3dcamera and its
actornode and calls the registered listeners. It polls the keyboard and
processes the batched mouse move events.]])

function CameraCtrl:runThink()
	-- don't do stuff while in console
	-- a little check to prevent multiple 
	-- thinks run at same frame 
	local curframe = system.frame()
	self.lframe = self.lframe or (curframe-1)
	
	if (curframe ~= self.lframe and self.think 
		and not console.active()) then self.think() end
		
	self.lframe = curframe
end

LuxModule.registerclassfunction("reset",
	[[():(CameraCtrl) -
	Resets internal caches of mouse movement and alike.]])
function CameraCtrl:reset()
	local attr = self.attribs
	attr.fixpos = nil
	attr.last = nil
	attr.mdelta = nil
end

LuxModule.registerclassfunction("addListener",
	[[():(CameraCtrl,fn) -
	registers a function to be called fn(l3dcamera,actornode) on changes]])

function CameraCtrl:addListener(func)
	self.listeners[func] = func
end

LuxModule.registerclassfunction("removeListener",
	[[():(CameraCtrl,fn) -
	unregisters the function.]])
	
function CameraCtrl:removeListener(func)
	self.listeners[func] = nil
end

LuxModule.registerclassfunction("runListeners",
	[[():(CameraCtrl,fn) -
	enforces call of all listeners.]])
	
function CameraCtrl:runListeners()
	for i,v in pairs(self.listeners) do
		v(self.cam,self.camact)
	end
end

-- internal
function CameraCtrl:mouseDelta(mx,my,time)
	local attr = self.attribs
	
	local delta =  {0,0,0} --attr.mdelta or
	local lx,ly = unpack(attr.fix and attr.fixpos or attr.last or attr.fixpos)
	local dx,dy = lx-mx,ly-my
	
	attr.mdelta = {delta[1]+dx,delta[2]+dy,delta[3]+1}
end

function CameraCtrl:mouseFunc(fn)

	local attr = self.attribs
	local mx,my = input.mousepos() 
	
	if not MouseCursor.enable() then 
		local wx,wy = window.refsize()
		wx,wy = math.floor(wx/2),math.floor(wy/2)
		attr.fixpos = {wx,wy}
		

		for rx,ry,btn,down,when in input.popmouse do
			self:mouseDelta(rx,ry,when)
		end
		
		self:mouseDelta(mx,my)
	elseif(attr.fixpos and input.ismousedown(0)) then
		--print("last",unpack(attr.last or {}))
		--print("now",mx,my)
		attr.mdelta = nil
		
		self:mouseDelta(mx,my)
	end

	if (attr.mdelta) then
		local dx,dy,cnt = unpack(attr.mdelta)
		local cur,ldiff,diff = system.time()
		
		if (dx~= 0 or dy~=0) then
			local rate = math.sqrt(dx*dx + dy*dy)/diff
			local accel = 0
			local spd = attr.mousesense + rate*accel
			
			--spd = spd/cnt
			
			fn(dx*spd,dy*spd)
			
		end
		
		if (attr.fix and attr.fixpos) then
			local lx,ly = unpack(attr.fixpos)
			
			input.mousepos(lx,ly)
		end
		attr.mdelta = nil
		attr.guilock = nil
	end
	attr.last = {mx,my}
end

-------------------------

local function checkkeys(tab)
	if type(tab)=="table" then
		for i,k in pairs(tab) do
			if Keyboard.isKeyDown(k) then return true end
		end
	else
		return Keyboard.isKeyDown(tab)
	end
	return false
end


CameraEgoMKCtrl = {}
LuxModule.registerclass("luxinialuacore","CameraEgoMKCtrl",
[[Uses Mouse and Keyboard. This controller modifies camera like a first person spectator. Default 
is WASD + SPACE/C for movement and mouse for looking. 
]],CameraEgoMKCtrl,{},"CameraCtrl")
setmetatable(CameraEgoMKCtrl,{__index = CameraCtrl})
function CameraEgoMKCtrl.new(class,cam,camact,attribs)
	self = CameraCtrl.new(class,cam,camact,attribs)
	
	self.think = function()
		
		local attr = self.attribs
		local keys = attr.keys
		local act = self.camact
		local cur,ldiff,diff = system.time()
		local change = false
		
		-- keyboard updates (MOVE)
		local sd = (checkkeys(keys.right) and 1 or 0) - (checkkeys(keys.left) and 1 or 0)
		local fw = (checkkeys(keys.forward) and 1 or 0) - (checkkeys(keys.backward) and 1 or 0)
		local up = (checkkeys(keys.up) and 1 or 0) - (checkkeys(keys.down) and 1 or 0)

		
		if (sd ~= 0 or fw ~= 0 or up ~= 0) then
			local spd = attr.movemulti * (checkkeys(keys.fast) and attr.shiftmulti or 1.0)
			spd = spd * diff * 0.0166
			local mat = act:matrix()
			local delta = {sd*spd,fw*spd,up*spd}
			
			act:pos(mat:transform(unpack(delta)))
			
			change = true
		end

		-- mouse updates
		self:mouseFunc(function(dx,dy)
			local rot = {act:rotdeg()}
			rot[3] = rot[3] + ((dx)*attr.rotmulti )
			rot[1] = rot[1] + ((dy)*attr.rotmulti * (attr.invert and -1.0 or 1.0))
			rot[1] = math.max(-90,math.min(90,rot[1]))
			act:rotdeg(unpack(rot))
			
			change = true
		end)

		if (change) then
			self:runListeners()
		end
	end
	
	return self
end

CameraEgoMCtrl = {}
LuxModule.registerclass("luxinialuacore","CameraEgoMCtrl",
[[Uses Mouse mostly. This controller modifies camera like a first person spectator. Rotation
is done with leftmouse drag, while translation with rightmouse (forward/side) and
right+left or middle (up/side).
]],CameraEgoMCtrl,{},"CameraCtrl")

setmetatable(CameraEgoMCtrl,{__index = CameraCtrl})
function CameraEgoMCtrl.new(class,cam,camact,attribs)
	self = CameraCtrl.new(class,cam,camact,attribs)
	self.think = function()
		local attr = self.attribs
		local keys = attr.keys
		local act = self.camact
		local cur,diff = system.time()

		-- mouse updates
		self:mouseFunc(function(dx,dy)
			change = false
			
			if (input.ismousedown(0) and not input.ismousedown(1)) then
				local rot = {act:rotdeg()}
				rot[3] = rot[3] + ((dx)*attr.rotmulti)
				rot[1] = rot[1] + ((dy)*attr.rotmulti * (attr.invert and -1.0 or 1.0))
				rot[1] = math.max(-90,math.min(90,rot[1]))
				act:rotdeg(unpack(rot))
				
				change = true
			end
			
			local sd,fw,up = 0,0,0
			if (input.ismousedown(1) and not input.ismousedown(0)) then
				sd = -dx
				fw = dy
				
				dx = 0
			end
			if (input.ismousedown(2) or (input.ismousedown(1) and input.ismousedown(0))) then
				sd = sd-dx
				up = dy
			end
			
			if (sd ~= 0 or fw ~= 0 or up ~= 0) then
				local spd = attr.movemulti * (checkkeys(keys.fast) and attr.shiftmulti or 1.0)
				spd = spd/50
				local mat = act:matrix()
				local delta = {sd*spd,fw*spd,up*spd}
				act:pos(mat:transform(unpack(delta)))
				
				change = true
			end
			
			if (change) then
				self:runListeners()
			end
		end)
	end
	
	return self
end

--[[
CameraOrbitMouseCtrl = {}
setmetatable(CameraOrbitMouseCtrl,{__index = CameraCtrl})
function CameraOrbitMouseCtrl.new(class,cam,camact,attribs)
	self = CameraCtrl.new(class,cam,camact,attribs)
	

	return self
end

function CameraOrbitMouseCtrl:createML(comp)
	return MouseListener.new(function(ml,mev)
	
	end)
end



CameraMoveMouseCtrl = {}
setmetatable(CameraMoveMouseCtrl,{__index = CameraCtrl})
function CameraMoveMouseCtrl.new(class,cam,camact,attribs)
	self = CameraCtrl.new(class,cam,camact,attribs)
	
	return self
end

function CameraMoveMouseCtrl:createML(comp)
	return MouseListener.new(function(ml,mev)
	
	end)
end
]]


