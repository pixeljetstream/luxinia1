ContainerMover = {}
LuxModule.registerclass("gui","ContainerMover",
	[[A component that is moving its parent frame if clicked and dragged.]],ContainerMover,{}
	,"Component")
setmetatable(ContainerMover,{__index = Component})


local prv = ContainerMover

LuxModule.registerclassfunction("new", 
	[[(ContainerMover):(table class, int x,y,w,h) - 
	Creates a ContainerMover, which is initially not visible]])
function ContainerMover.new (class,x,y,w,h)
	local self = Component.new(class,x,y,w,h)
	self:init()
	return self
end

local function mouseMoved (self,me)
	self[prv].sx,self[prv].sy = self[prv].sx or me.x,self[prv].sy or me.y

	if me:isDragged() then
		self:lockMouse()
		local sx,sy = self[prv].sx,self[prv].sy
		local dx,dy = math.floor(me.x-sx+.5),math.floor(me.y-sy+.5)

		
		local p =self:getParent()
		p:setLocation(p:getX()+dx,p:getY()+dy)
	else
		self:unlockMouse()
		self[prv].sx,self[prv].sy = me.x,me.y

	end
end


function ContainerMover:init ()
	if self.mouseMoved == mouseMoved then return end
	self[prv] = self[prv] or {}
	self[prv].mouseMoved = rawget(self,"mouseMoved")
	self.mouseMoved = mouseMoved
end