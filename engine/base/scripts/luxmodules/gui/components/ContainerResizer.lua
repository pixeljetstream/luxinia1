ContainerResizer = {}
LuxModule.registerclass("gui","ContainerResizer",
	[[A component that is resizing its parent frame if clicked and dragged.]],ContainerResizer,{}
	,"Component")
setmetatable(ContainerResizer,{__index = Component})


local prv = ContainerResizer

LuxModule.registerclassfunction("new", 
	[[(ContainerResizer):(table class, int x,y,w,h) - 
	Creates a ContainerResizer, which is initially not visible]])
function ContainerResizer.new (class,x,y,w,h)
	local self = Component.new(class,x,y,w,h)
	self:init()
	return self
end

function ContainerResizer:setAxislock(x,y)
	self[prv] = self[prv] or {}
	self[prv].xlock= x
	self[prv].ylock = y
end

function ContainerResizer:setFixedPoint (x,y)
	self[prv] = self[prv] or {}
	self[prv].left,self[prv].right = left,top
end

function ContainerResizer:setMinParentSize(mw,mh)
	self[prv].minw,self[prv].minh = mw,mh
end

local function mousePressed(comp,me)
	comp[prv].sx,comp[prv].sy = comp[prv].sx or me.x,comp[prv].sy or me.y
	comp:lockMouse()
end
local function mouseReleased(comp,me)
	comp[prv].sx,comp[prv].sy = comp[prv].sx or me.x,comp[prv].sy or me.y
	comp:unlockMouse()
end

local function mouseMoved (comp,me)
	comp[prv].sx,comp[prv].sy = comp[prv].sx or me.x,comp[prv].sy or me.y
	if me:isDragged() then
		comp:lockMouse()
		local parent = comp:getParent()
		local x,y = comp:getLocation()
		
		local sx,sy = comp[prv].sx,comp[prv].sy
		local dx,dy = math.floor(me.x-sx+.5),math.floor(me.y-sy+.5)
		local fx,fy = comp[prv].left or 0, comp[prv].top or 0
		local lx,ly = comp[prv].xlock, comp[prv].ylock
		local mx,my = 0,0
		local pw,ph = parent:getSize()
		local px,py = parent:getLocation()
		if not lx then
			if fx==0 then
				pw = pw + dx
				mx = dx
			else
				px = px + dx
				pw = pw - dx
			end
		end
		if not ly then
			if fy==0 then
				ph = ph + dy
				my = dy
			else
				py = py + dy
				ph = ph - dy
			end
		end
		local minw,minh = comp[prv].minw or 10,comp[prv].minh or 10
		local tw,th = math.max(pw,minw),math.max(ph,minh)
		
		if mx~=0 then mx = mx - (pw-tw) end
		if my~=0 then my = my - (ph-th) end
		
		comp:setLocation(comp:getX()+mx,comp:getY()+my)
		parent:setLocation(px,py)
		parent:setSize(tw,th)
		
	else
		comp:unlockMouse()
		comp[prv].sx,comp[prv].sy = me.x,me.y
	end
	
end

function ContainerResizer:init ()
	if self.mouseMoved == mouseMoved then return end
	self[prv] = self[prv] or {}
	self[prv].mouseMoved = rawget(self,"mouseMoved")
	self[prv].mousePressed = rawget(self,"mousePressed")
	self[prv].mouseReleased = rawget(self,"mouseReleased")
	self.mouseMoved = mouseMoved
	self.mouseReleased = mouseReleased
	self.mousePressed = mousePressed
end
