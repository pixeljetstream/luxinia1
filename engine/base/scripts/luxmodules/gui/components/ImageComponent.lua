ImageComponent = {}
setmetatable(ImageComponent,{__index = Component})
LuxModule.registerclass("gui","ImageComponent",
	[[A ImageComponent element for displaying simple images (textures, material, plain colors).]],ImageComponent,
	{},"Component")


LuxModule.registerclassfunction("new",
	[[(ImageComponent):(class, int x,y,w,h, matsurface, [autofocus],[blendmode],[matrix4x4 texmatrix]) -.]])
function ImageComponent.new (class, x,y,w,h, tex,autofocus,blend,tm)
	local self = Component.new(class,x,y,w,h)
	
	self.dataImageComponent = {
		tex = tex,
		r=1,g=1,b=1,a=1,
		autofocus = autofocus or false,
		blend = blend,
		tm = tm
	}
	
	self:createVisibles()

	return self
end

LuxModule.registerclassfunction("getL2DImage",
	[[(l2dimage):(ImageComponent) - returns the used imagecomponent]])
function ImageComponent:getL2DImage()
	return self.l2d and self.l2d.image
end

LuxModule.registerclassfunction("setL2DImage",
	[[():(ImageComponent,l2d) - sets the used l2dimage]])
function ImageComponent:setL2D (l2d)
	self.l2d.image = assert(l2d,"need a l2dvalue")
end

LuxModule.registerclassfunction("isFocusable",
	[[(boolean):(ImageComponent) - returns true if visible]])
function ImageComponent:isFocusable() return self.dataImageComponent.autofocus and true and self:isVisible() end

LuxModule.registerclassfunction("toString",
	[[(string):(ImageComponent) - returns simple component representation]])
function ImageComponent:toString ()
	return string.format("[ImageComponent tex=%s]",tostring(self.dataImageComponent.tex))
end

local function updateMo (self)
	if not self.l2d then return end
	local phi = self.dataImageComponent.uvrot or 0
	local x,y = unpack(self.dataImageComponent.uvpos or {0,0})
	local sx,sy = unpack(self.dataImageComponent.uvscale or {1,1})
	local c,s = math.cos(phi),math.sin(phi)
	
	self.l2d.image:moPos(x,y,0)
	self.l2d.image:moRotaxis(
		c*sx, s*sx, 0,
		-s*sy,c*sy, 0,
		0,0,1
	)
end
LuxModule.registerclassfunction("setUVScale", "():(ImageComponent,x,y) - sets scaling for uv")
function ImageComponent:setUVScale(x,y)
	self.dataImageComponent.uvscale = {x,y}
	updateMo(self)
end
LuxModule.registerclassfunction("getUVScale", "(x,y):(ImageComponent) - gets scaling for uv")
function ImageComponent:getUVScale()
	return unpack(self.dataImageComponent.uvscale or {1,1})
end
LuxModule.registerclassfunction("setUVPos", "():(ImageComponent,x,y) - sets uv translation")
function ImageComponent:setUVPos(x,y)
	self.dataImageComponent.uvpos = {x,y}
	updateMo(self)
end
LuxModule.registerclassfunction("getUVPos", "():(ImageComponent,x,y) - gets uv translation")
function ImageComponent:getUVPos()
	return unpack(self.dataImageComponent.uvpos or {0,0})
end
LuxModule.registerclassfunction("setUVRotRad", "():(ImageComponent,phi) - sets uv rotation angle")
function ImageComponent:setUVRotRad(phi)
	self.dataImageComponent.uvrot = phi
	updateMo(self)
end
LuxModule.registerclassfunction("getUVRotRad", "(phi):(ImageComponent) - gets uv rotation angle")
function ImageComponent:getUVRotRad()
	return self.dataImageComponent.uvrot or 0
end

--------------------------------------------------------------------------------
LuxModule.registerclassfunction("deleteVisibles", "@overloaded Component")
function ImageComponent:deleteVisibles ()
	if (not self.l2d) then return end
	for a,b in pairs(self.l2d) do
		b:delete()
	end
	self.l2d = nil
end

LuxModule.registerclassfunction("createVisibles", "@overloaded Component")
function ImageComponent:createVisibles ()
	if (self.l2d) then return end

	self.l2d = {}
	self.l2d.image = l2dimage.new("imagecomponent",self.dataImageComponent.tex)
	self.l2d.image.collectforgc = true
	self.l2d.image:parent(self.basel2d)
	
	self.l2d.image:rfNodepthmask(true)
	self.l2d.image:rfNodepthtest(true)
	self.l2d.image:scissorparent(true)
	
	local w,h = self:getSize()
	self.l2d.image:scale(w,h,1)
	
	updateMo(self)


	self.l2d.image:color(self.dataImageComponent.r,self.dataImageComponent.g,self.dataImageComponent.b,
		self.dataImageComponent.a)
	if (self.dataImageComponent.blend) then
		self.l2d.image:rfBlend(true)
		self.l2d.image:rsBlendmode(self.dataImageComponent.blend)
	end
	if (self.dataImageComponent.tm) then
		self.l2d.image:moTexmatrix(self.dataImageComponent.tm)
	end

end

--[=[
LuxModule.registerclassfunction("showVisibles", "@overloaded Component")
function ImageComponent:showVisibles ()
	for a,b in pairs(self.l2d) do b:rfNodraw(false) end
end

LuxModule.registerclassfunction("hideVisibles", "@overloaded Component")
function ImageComponent:hideVisibles ()
	for a,b in pairs(self.l2d) do b:rfNodraw(true) end
end
]=]

LuxModule.registerclassfunction("positionUpdate","@overloaded Component")
function ImageComponent:positionUpdate(z,clip)
	if (not self.l2d) then return end
	local w,h = self:getSize()
	self.l2d.image:scale(w,h,1)
--	self.l2d.image:scissor(true)
	self.l2d.image:scissorparent(true)
	
	--[=[
	local ox,oy = self:local2world(0,0)
	self.l2d.image:pos(ox,oy,0)


	if (clip == nil) then
		for a,l2d in pairs(self.l2d) do
			l2d:rfNodraw(true)
		end
		return z+1
	end
	local vis = self:isVisible()
	for a,l2d in pairs(self.l2d) do
		l2d:scissor(true)
		l2d:scissorsize(clip[3],clip[4])
		l2d:scissorstart(clip[1],clip[2])
		l2d:rfNodraw(not vis)
		l2d:sortid(z)
	end
	]=]

	return z + 1
end

LuxModule.registerclassfunction("color","():(ImageComponent,float r,g,b,a) - color for imagecomponent")
function ImageComponent:color(r,g,b,a)	
	if (not r) then
		return self.dataImageComponent.r,self.dataImageComponent.g,self.dataImageComponent.b,
		self.dataImageComponent.a
	end
	self.dataImageComponent.r,self.dataImageComponent.g,self.dataImageComponent.b,
		self.dataImageComponent.a = r,g,b,a or 1
	if (self.l2d) then
		self.l2d.image:color(r,g,b,a or 1)
	end
end

function ImageComponent:setColor(r,g,b,a)	
	if type(r)=="table" then r,g,b,a = unpack(r) end
	return self:color(r,g,b,a)
end

function ImageComponent:getColor()
	return self:color()
end

LuxModule.registerclassfunction("blendmode","():(ImageComponent,blendmode) - blendmode for imagecomponent")
function ImageComponent:blendmode(blend)	
	self.dataImageComponent.blend = blend
	if (self.l2d) then
		if (self.dataImageComponent.blend) then
			self.l2d.image:rfBlend(true)
			self.l2d.image:rsBlendmode(self.dataImageComponent.blend)
		else
			self.l2d.image:rfBlend(false)
		end
	end
end

LuxModule.registerclassfunction("matsurface","(matsurface):(ImageComponent,matsurface) - matsurface for imagecomponent")
function ImageComponent:matsurface(tex)
	if (not tex) then return self.dataImageComponent.tex end
	self.dataImageComponent.tex = tex
	if (self.l2d) then
		self.l2d.image:matsurface(tex)
	end
end

LuxModule.registerclassfunction("texmatrix","(matrix4x4):(ImageComponent,matrix4x4) - texturematrix for imagecomponent, pass a non matrix4x4 to disable")
function ImageComponent:texmatrix(tm)
	-- FIXME tm in this case is a pointer to a matrix, but local value must be a copy !!
	if (not tm) then
		if (self.l2d) then
			self.dataImageComponent.tm = self.l2d.image:moTexmatrix()
		end
		return self.dataImageComponent.tm
	end
	self.dataImageComponent.tm = tm
	if (self.l2d and tm) then
		self.l2d.image:moTexmatrix(tm)
	end
end

LuxModule.registerclassfunction("mouseClicked","@overloaded Component")
function ImageComponent:mouseClicked(event)
	Component.mouseClicked(self,event)
	self:focus()
end

LuxModule.registerclassfunction("onGainedFocus",
	"():(from) - simply forwards the focus to the next element.")
function ImageComponent:onGainedFocus (from)
	local dir = (Keyboard.isKeyDown(Keyboard.keycode.LSHIFT) or
		Keyboard.isKeyDown(Keyboard.keycode.RSHIFT)) and -1 or 1
	return self:transferFocus(dir)
end
