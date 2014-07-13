L3DIcon = {}
setmetatable(L3DIcon,{__index = Icon})
LuxModule.registerclass("gui","L3DIcon",
	[[L3DIcon are displaing l3dmodels.]],
	L3DIcon,
	{
	},"Icon")

LuxModule.registerclassfunction("new",
	[[():(class,) -
	]])
function L3DIcon.new (class, width, height, oncreate, ondelete)
	local self = Icon.new(class,width,height)

	self.oncreate, self.ondelete = oncreate,ondelete
	self.rotdeg = {0,0,0}
	self.origin = {0,0,0}
	self.scale = {1,1,1}

	return self
end

function L3DIcon.createModelIcon(class,width,height,mdl)
	return class:new(width,height,function () return l3dmodel.new("l3d",mdl) end,
		function (self,l3d) l3d:delete() end)
end

LuxModule.registerclassfunction("l3dRotdeg",
	[[():(class, float x,y,z) - sets rotation of the l3d icon]])
function L3DIcon:l3dRotdeg (x,y,z)
	self.rotdeg = {
		x or self.rotdeg[1],
		y or self.rotdeg[2],
		z or self.rotdeg[3]
	}

	if self.l2d then
		self.l2d:rotdeg(unpack(self.rotdeg))
	end
end

LuxModule.registerclassfunction("l3dPos",
	[[():(class, float x,y,z) - sets position of the l3d icon]])
function L3DIcon:l3dPos (x,y,z)
	self.origin = {
		x or self.origin[1],
		y or self.origin[2],
		z or self.origin[3]
	}
	if self.l2d then
		self.l2d:pos(
			self.x + self.origin[1],
			self.y + self.origin[2],
			self.z + self.origin[3])
	end
end

LuxModule.registerclassfunction("l3dScale",
	[[():(class, float x,y,z) - sets scaling of the l3d icon]])
function L3DIcon:l3dScale (x,y,z)
	self.scale = {
		x or self.scale[1],
		y or self.scale[2],
		z or self.scale[3]
	}
	if self.l3d and self.l3d.renderscale then
		self.l3d:renderscale(unpack(self.scale))
	elseif self.l2d then
		self.l2d:scale(unpack(self.scale))
	end
end


function L3DIcon:update(reason)
--	print "updating icon"
--	print(self.l2d,self.x,self.y)
	if (not self.l2d) then return end

	if (not self.visibleflag) then
		self.l2d:rfNodraw(true)
		return
	end

	self.l2d:rfNodraw(false)

	local x,y,w,h = math.floor(self.x),math.floor(self.y),math.floor(self.width),math.floor(self.height)
	local a,b,c = unpack (self.origin)
	self.l2d:pos(x+a,y+b,self.z+c)
	if self.l3d.renderscale then
		self.l3d:renderscale(unpack(self.scale))
	else
		self.l2d:scale(unpack(self.scale))
	end
	self.l2d:sortid(self.id)
	self.l2d:scissorparent(true)
	self.l2d:scissor(true)
	self.l2d:scissorsize(w,h)

	if self.scissor and self.scissor[1] then
		self.l2d:scissor(true)
		self.l2d:scissorparent(true)
		local x,y,w,h = unpack(self.scissor)

		local a =	Rectangle:new(x,y,w,h)
		local b = Rectangle:new(self.x,self.y,self.width,self.height)
		local rect = b:intersection(a)
	--	print(a,b,rect)
		if not rect then self.l2d:rfNodraw(true) else

			self.l2d:rfNodraw(false)
			x,y,w,h = unpack(rect)

			self.l2d:scissorsize(w,h)
			self.l2d:scissorstart(x,y)
		end
	end
end

function L3DIcon:setClip(x,y,w,h)
	self.scissor = {x,y,w,h}
	if (not self.l2d) then return end
	if (not x) then
		self.l2d:scissor(false)
	else
		self.l2d:scissor(true)
		self.l2d:scissorparent(true)
		self.l2d:scissorsize(w,h)
		self.l2d:scissorstart(x,y)
	end
end


function ImageIcon:getClip()
	return unpack(self.scissor or {})
end

function L3DIcon:createVisibles (l2dparent)
	Icon.createVisibles(self,l2dparent)
	
	if (self.l2d) then return end

	self.l3d = self:oncreate()
	self.l2d = l2dnode3d.new("iconl3d",self.l3d)
	self.l2d:link(l2dparent)

	self.l3d:uselocal(true)
	--self.l3d:localpos(unpack(self.origin))
	self.l2d:rotdeg(unpack(self.rotdeg))
	self.l2d:scale(unpack(self.scale))


	self:update(Icon.REASON.CREATEDVIS)
end

function L3DIcon:deleteVisibles()
	if (not self.l2d) then return end
	self.l2d:delete()
	self.l2d = nil
	self.ondelete(self,self.l3d)
	self.l3d = nil

	self:update(Icon.REASON.DELETEDVIS)
end

function L3DIcon:clone()
	local clone = L3DIcon:new(self.width,self.height,self.oncreate,self.ondelete)
	clone:l3dRotdeg(unpack(self.rotdeg))
	clone:l3dPos(unpack(self.origin))
	clone:l3dScale(unpack(self.scale))
	return clone
end

