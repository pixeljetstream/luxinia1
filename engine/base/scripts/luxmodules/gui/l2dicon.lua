L2DIcon = {}
setmetatable(L2DIcon,{__index = Icon})
LuxModule.registerclass("gui","L2DIcon",
	[[]],
	L2DIcon,
	{
	},"Icon")

LuxModule.registerclassfunction("new",
	[[():(class,width, height, oncreate, ondelete) -
	]])
function L2DIcon.new (class, width, height, oncreate,ondelete)
	local self = Icon.new(class,width,height)
	
	self.oncreate,self.ondelete = oncreate,ondelete
	
	return self
end

function L2DIcon:update(reason)
	if (not self.l2d) then return end

	if (not self.visibleflag) then
		self.l2d:rfNodraw(true)
		return
	end

	self.l2d:rfNodraw(false)


	local x,y,w,h = math.floor(self.x),math.floor(self.y),math.floor(self.width),math.floor(self.height)
	self.l2d:pos(x,y,self.z)
	self.l2d:scale(w,h,1)
	self.l2d:sortid(self.id)

end

function L2DIcon:setClip(x,y,w,h)
	self.cx,self.cy,self.cw,self.ch = x,y,w,h
	if (not self.l2d) then return end
	if (not x) then
		self.l2d:scissor(false)
	else
		self.l2d:scissor(Component.isClipped())
		self.l2d:scissorparent(true)
		self.l2d:scissorsize(w,h)
		self.l2d:scissorstart(x,y)
	end
end

function L2DIcon:getClip()
	return self.cx,self.cy,self.cw,self.ch
end

function L2DIcon:createVisibles (l2dparent)
	Icon.createVisibles(self,l2dparent)
	if (self.l2d) then return end

	self.l2d = l2dnode.new("icon")
	

	self.l2d:link(l2dparent)
	self.l2d:rfNodepthtest(true)
	self.l2d:scissor(Component.isClipped())
	self.l2d:scissorparent(true)
	
	self:oncreate(self.l2d)

	self:update(Icon.REASON.CREATEDVIS)
end

function L2DIcon:deleteVisibles()
	if (not self.l2d) then return end
	self:ondelete()

	self.l2d:delete()
	self.l2d = nil
	
	
	self:update(Icon.REASON.DELETEDVIS)
end

function L2DIcon:clone()
	return L2DIcon:new(self.width,self.height,self.oncreate,self.ondelete)
end

