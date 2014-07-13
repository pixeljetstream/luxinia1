ImageIcon = {}
setmetatable(ImageIcon,{__index = Icon})
LuxModule.registerclass("gui","ImageIcon",
	[[ImageIcon objects are using textures to display little icons in GUIs.
	A texture can be used for multiple different icons.]],
	ImageIcon,
	{
	},"Icon")

LuxModule.registerclassfunction("new",
	[[():(class,string/texture tex, width, height, blendmode, [table iconpositions], [table color]) -
	Creates image icon from texture. The texture can be either a string
	or a loaded textures. If a string is passed the texture is being loaded
	without texture compression (if it is on anyway).

	The width and height is going to be used as size for the icon.
	The blendmode is going to be used for the the icon.

	The iconpositions is an associative array contiaining values with the
	x,y coordinates and widths and heights of the parts of the texture to be used.
	The default entry is used if no value for the selected icon is found. Components
	will use names to load certain icons in different states (hovered, pressed,
	etc.) and you can specify here where the correct icon is to be found on
	the image. If no iconposition is passed, the whole texture is used as
	icon.

	All coordinates are in pixel coordinates.

	Per default, the filtering of the icon texture is deactivated if the
	size of the window is the same as the refsize for the drawn objects.
	Otherwise the filtering value is not touched. Be aware that the
	icons will look best if they are drawn in the same size as they really are.]])
function ImageIcon.new (class, tex,width, height, blendmode, iconpositions,clr)
	local self = Icon.new(class,width,height)

	local com =rendersystem.texcompression()
	rendersystem.texcompression(false)
	self.texture = type(tex)=="string" and texture.load(tex,false,false)
		or tex
		
	assert(self.texture and type(self.texture)~="string","no texture found")

	local rw,rh = window.refsize()
	local ww,wh = window.width(), window.height()

	--if (rw==ww and rh==wh) then -- don't filter, the size of window is ok
	--	self.texture:filter(false,false)
	--else
		self.texture:filter(false,true)
	--end

	rendersystem.texcompression(com)
	self.iconcolor = clr or {1,1,1,1}
	self.iconpositions = iconpositions

	assert(not blendmode or type(blendmode)=="userdata","invalid blendmode: "..tostring(blendmode))
	self.blendmode = blendmode
	self.tw,self.th = self.texture:dimension()
	if (rendersystem.detail()<2) then
		self.tw, self.th = self.tw*2, self.th*2
	end

	return self
end

function ImageIcon:update(reason)
	if (not self.l2d) then return end

	if (not self.visibleflag) then
		self.l2d:rfNodraw(true)
		return
	end

	self.l2d:rfNodraw(false)



	local x,y,w,h = unpack(self.iconpositions and
		(self.iconpositions[self.surface] or self.iconpositions.default) or {0,0,self.tw,self.th})

	y = self.th-y-h-1
	--print(self.surface,x,y,w,h)
	x = x / self.tw
	y = y / self.th
	w = w / self.tw
	h = h / self.th

	self.l2d:moPos(x,y,0)
	self.l2d:moRotaxis(w,0,0, 0,h,0, 0,0,1)

	x,y,w,h = math.floor(self.x),math.floor(self.y),math.floor(self.width),math.floor(self.height)
	self.l2d:pos(x,y,self.z)
	self.l2d:scale(w,h,1)
	self.l2d:sortid(self.id)
	self.l2d:color(unpack(self.iconcolor))
	self.l2d:scissorparent(true)
	self.l2d:scissor(Component.isClipped())
	self.l2d:scissorsize(w,h)
	
	if (self.blendmode) then
		self.l2d:rfBlend(true)
		self.l2d:rsBlendmode(self.blendmode)
	end
end

function ImageIcon:setClip(x,y,w,h)
	self.cx,self.cy,self.cw,self.ch = x,y,w,h
	if (not self.l2d) then return end
	if (not x) then
		self.l2d:scissor(false)
	else
		self.l2d:scissorparent(true)
		self.l2d:scissor(true)
		self.l2d:scissorsize(w+1,h+1)
		self.l2d:scissorstart(x,y)
	end
end

function ImageIcon:getClip()
	return self.cx,self.cy,self.cw,self.ch
end

function ImageIcon:getColor()
	return unpack(self.iconcolor)
end

function ImageIcon:setColor(r,g,b,a)
	self.iconcolor = {r,g,b,a}
	if self.l2d then
		self.l2d:color(r,g,b,a)
	end
end

function ImageIcon:createVisibles (l2dparent)
	Icon.createVisibles(self,l2dparent)
	if (self.l2d) then return end

	self.l2d = l2dimage.new("icon",self.texture)
	self.l2d.collectforgc = true
	self.l2d:rfNodepthtest(true)
	self.l2d:rfNodepthmask(true)

	self.l2d:parent(l2dparent)
	self.l2d:color(unpack(self.iconcolor))
	self:update(Icon.REASON.CREATEDVIS)
end

function ImageIcon:deleteVisibles()
	if (not self.l2d) then return end
	self.l2d:delete()
	self.l2d = nil

	self:update(Icon.REASON.DELETEDVIS)
end

function ImageIcon:clone()
	return ImageIcon:new(self.texture,self.width,self.height,self.blendmode,self.iconpositions)
end
