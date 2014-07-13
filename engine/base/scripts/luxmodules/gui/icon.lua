Icon = {
	REASON = {
		RESIZED="resized",
		SKINCHANGE="skinchange",
		VISCHANGE="vischange",
		CREATEDVIS = "createdvis",
		DELETEDVIS = "deletedvis",
		COLORCHANGE = "colorchange"
	},
	colorValue = {1,1,1,1}
}

LuxModule.registerclass("gui","Icon",
	[[An Icon is a small gui element for graphical symbols. An Icon in Luxinia
	could be of any type - 2D, 3D, animated and so on, so this class should
	provide some basic interface definititions and functions to create, delete
	and hiding the graphical representation of the Icon.]],
	Icon,{
		["REASON.RESIZED"] = "{[string]}=resized - resized skin",
		["REASON.SKINCHANGE"] = "{[string]}=skinchange - skinchange reason",
		["REASON.VISCHANGE"] = "{[string]}=vischange - vischange reason",
		["REASON.CREATEDVIS"] = "{[string]}=createdvis - vischange reason",
		["REASON.DELETEDVIS"] = "{[string]}=deletedvis - vischange reason",
		["REASON.COLORCHANGE"] = "{[string]}=colorchange - colorchange reason",
		x = "[float] - position of the bounds of the icon",
		y = "[float] - position of the bounds of the icon",
		z = "[float] - z position in l2d layer",
		id = "[float] - l2d sortid",
		width = "[float] - width of the icon",
		height = "[float] - height of the icon",
		visibleflag = "[boolean] - true if the icon should be visible, but depends if the visible resources exists",
		surface = "[string] - surface that should be used for this icon",
	})

local idcnt = 1
LuxModule.registerclassfunction("new",
	[[(Icon):(class, [width,height]) - Sets
	metatable index to class. Per default, a skin does not create any visual
	objects during start. These must be created by calling createVisibles. ]])
function Icon.new (class, width, height)
	local self = {x=0,y=0,z=0,width = width or 0, height = height or 0, visibleflag = true, id=0}

	setmetatable(self, {__index = class, __gc =
		function (self) self:onDestroy() end,
		__tostring = function (self) return self:toString() end})

	self.surface = "default"
	self.txid = idcnt
	idcnt = idcnt + 1

	return self
end

LuxModule.registerclassfunction("selectIcon",
	[[():(Icon, icon) - Selects an Icon. This means a different representation
	of the same icon. If the icon is used by a Skin2D object, the Skin2D object
	will automaticly forward its selectSkin parameter to this function. So an
	icon that is assigned to a skin used by a component should provide the
	"surface" that are specific for this component.

	If no surface of that name exists the surface "default" is tried out, if that
	fails too the object handles the case on its own.]])
function Icon:selectIcon (icon)
	--print("Selecting: ", icon)
	self.surface = icon
	self:update(Icon.REASON.SKINCHANGE)
end

LuxModule.registerclassfunction("update",
	[[():(Icon,reason) - function that is called if something has changed.
	The reason should be one of the constants defined in the Icon class and
	describes the reason why the icon was updated more precisly.]])
function Icon:update(reason) end

LuxModule.registerclassfunction("setLocation",
	[[():(Icon,x,y) - sets new location for icon]])
function Icon:setLocation (x,y)
	self:setBounds(x,y,self:getWidth(),self:getHeight())
end

LuxModule.registerclassfunction("color",
	[[([r,g,b,a]):(Icon,[r,g=1,b=1,a=1]) - Sets the color of the icon - might depend on implementation.]])
function Icon:color(r,g,b,a)
	if r then
		g,b,a = g or 1, b or 1, a or 1
		assert(type(r)=="number" and type(g)=="number" and type(b)=="number" and type(a)=="number",
			"r,g,b,a must be numbers")
		self.colorValue = {r,g,b,a}
		self:update(Icon.REASON.COLORCHANGE)
	else
		return unpack(self.colorValue or {1,1,1,1})
	end
end

LuxModule.registerclassfunction("setClip",
	[[():(Icon,[x,y,w,h]) - sets clipping for icon or disables it if no clipping is passed]])
function Icon:setClip(x,y,w,h) end

LuxModule.registerclassfunction("getClip",
	[[([x,y,w,h]):(Icon) - gets clipping for icon or disables it if no clipping is passed]])
function Icon:getClip() end

LuxModule.registerclassfunction("setBounds",
	[[():(Icon,x,y,w,h) - sets self.x, self.y, self.width and self.height to
	the given values and calls the update function]])
function Icon:setBounds (x,y,w,h)
	self.x,self.y,self.width,self.height = x,y,w,h
	self:update(Icon.REASON.RESIZED)
end
LuxModule.registerclassfunction("getHeight",[[(h):(Icon) - returns height of the Icon]])
function Icon:getHeight() return self.height end
LuxModule.registerclassfunction("getWidth",[[(w):(Icon) - returns width of the Icon]])
function Icon:getWidth() return self.width end
LuxModule.registerclassfunction("getSize",[[(w,h):(Icon) - returns width and height of the Icon]])
function Icon:getSize () return self:getWidth(), self:getHeight() end

LuxModule.registerclassfunction("delete",
	[[():(Icon) - deletes the icon. This function might get called multiple times so
	flag your icon once you have deleted it. Per default the deletefunction will
	call the deleteVisibles function. If your Icon does not require further
	resources you can leave it this way.]])
function Icon:delete ()
	if (not self.destroyed) then
		self:deleteVisibles()
		self.destroyed = true
	end
end
LuxModule.registerclassfunction("createVisibles",
	[[():(Icon,l2dnode parent) - Creates all visible resources required for displaying the icon.
	Might get called even if the visible information have been created already.]])
function Icon:createVisibles (l2dparent) self.l2dparent = l2dparent end
LuxModule.registerclassfunction("deleteVisibles",
	[[():(Icon) - frees resources that are required for displaying the icon.
	Might get called even if the visible information have been deleted.]])
function Icon:deleteVisibles() end

LuxModule.registerclassfunction("onDestroy",
	[[():(Icon) - called by the garbagecollector]])
function Icon:onDestroy ()
	if (not self.destroyed) then
		self:delete()
		self.destroyed = true
	end
end

LuxModule.registerclassfunction("setVisibility",
	[[():(Icon,boolean visible) - Shows or hides the icon. Only affects
	Icons that have created visible resources.]])
function Icon:setVisibility (visible)
	self.visibleflag = visible
	self:update(Icon.REASON.VISCHANGE)
end

LuxModule.registerclassfunction("clone",
	[[(Icon):(Icon) - Creates a copy of this Icon ready to be used. Must be
	overloaded, otherwise an error is thrown.]])
function Icon:clone ()
	error ("Clone function of icon subclasses must be overloaded")
end

LuxModule.registerclassfunction("toString",
	[[(string):(Icon) - called by the tostring function and returns a string
	representing the icon information]])
function Icon:toString ()
	return ("[Icon "..self.txid.."]")
end

LuxModule.registerclassfunction("contains",
	[[(boolean):(Icon, x,y)  - returns true if the specific pixel is clickable.
	Returns true per default]])
function Icon:contains (x,y) return true end

LuxModule.registerclassfunction("setSortID",
	[[():(Icon, id)  - sets sortids for l2d sorting]])
function Icon:setSortID(id)
	self.id = id
	self:update(Icon.REASON.RESIZED)
end

LuxModule.registerclassfunction("setZ",
	[[():(Icon, z)  - Z offset to l2dlayer]])
function Icon:setZ (z)
	self.z = z
	self:update(Icon.REASON.RESIZED)
end
