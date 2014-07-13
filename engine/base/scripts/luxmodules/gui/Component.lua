local focuselement
local focusorder = {}
local collectit = "collectforgc"
--setmetatable(pder,{, __newindex = function (self,key,value)
	---- if the table is broken, let's fix it
--end})

local mouselock -- element which will receive all mouseevents
local mlockpos

--[=[local l2dngc,l2dfgc,l2digc,l2dtgc = l2dnode.__gc,l2dflag.__gc,l2dimage.__gc,l2dtext.__gc
function l2dnode:__gc ()
	if self.collectforgc then
		xpcall(function () self:delete() end,function (err) end)
	end
	l2dngc(self)

--	print("flag del")
end
function l2dflag:__gc ()
	if self.collectforgc then
		xpcall(function () self:delete() end,function (err) end)
	end
	l2dfgc(self)

--	print("flag del")
end
function l2dimage:__gc ()
	if self.collectforgc then
		xpcall(function () self:delete()end,function (err) end)
	end
	l2digc(self)

--	print("ima del")
end
function l2dtext:__gc ()
	if self.collectforgc then
		xpcall(function () self:delete() end,function (err) end)
	end
	l2dtgc(self)
--	print("text del")
end]=]


reschunk.usecore(true)
local txcom = rendersystem.texcompression()
rendersystem.texcompression(false)

local white = texture.load("?white2D")
local sgui = texture.load("stretchguiskin.tga",false)
texture.load("base/textures/colorhue.tga")
texture.load("base/textures/colorgradientgrey.tga")
texture.load("fonts/arial11-16x16.tga")
texture.load("fonts/lucidasanstypewriter11-16x16.tga")

rendersystem.texcompression(txcom)

local framedefskin,defaulticon16x16set = dofile("base/skins/lux98.lua")
_G.defaulticon16x16set = defaulticon16x16set

reschunk.usecore(false)


Component = {
	clipping = true,
	zorderoffset = 10000,
	white = white,
	colors = {
		border1 = {0,0,0,1},
		border2 = {0.8,0.8,0.8,1},
		focusborder = {0.8,0.8,0.8,1},
		textbackground = {0.95,0.95,0.95,1},
		background = {0.8,0.8,0.8,1},
		text = {0,0,0,1},
		cursor = {0.6,0.6,0.6,1},
		selection = {0.4,0.4,0.4,0.3},
		highlight = {0.9,0.9,0.9,1},
		shadow = {0.3,0.3,0.3,1},
	},
	defaultfontmono = "lucidasanstypewriter11-16x16",
	defaultfont = "arial11-16x16",
	focuscolors = {
		textbackground = {1,1,1,1},
		focusborder = {0.3,0.3,0.3,1},
		border2 = {0.3,0.3,0.3,1},
		border1 = {1,0,0,1},
		cursor = function()
			---local time = system.time()
			--time = (math.sin(time*0.01)+1)*0.5
			--local c = time>0.5 and 1 or 0
			--return unpack({c,c,c,1})
			return 1,0,0.3,1
		end,
	},
	presscolors = {
		background = {0.9,0.9,0.9,1},
		focusborder = {0.9,0.9,0.9,1},
	},
	mouseovercolors = {
		background = {0.86,0.86,0.86,1},
		focusborder = {0.86,0.86,0.86,1},
	},
	clipinsets = {0,0,0,0},
	index = 1,
	zsortid = 1,

	skinnames = {
		pressedskin = "component_pressed",
		hoveredskin = "component_hover",
		defaultskin = "component",
		focusedskin = "component_focus",
	}
}
setmetatable(Component.focuscolors,{__index = Component.colors})

LuxModule.registerclass("gui","Component",
	[[A Component is a 2D object on the screen that allows interaction with
	the mouse and keyboard of the user.

	All coordinates are always relative to the parent's Component.

	All components will propagate mouseevents to its parents! If the mouse is
	moved on a child, the parent that cannot "see" the mouse will receive
	events, too. This change was done in 0.93a. The reason is, that it is easier
	for the parent components to trace the mouse actions (i.e. mousewheel)
	in an easy manner. Most mouse actions are now receiving a second argument
	that tells, if the component is the owner of the mouse, which means that it
	is the topmost component at the mouse cursor's position.
	]],Component,
	{
		zorderoffset = "[int] - sortid offset for l2dnodes",
		white = "[texture] - a 2x2 sized white texture for general purpose",
		isDisplayedFlag = "{[boolean]} - true if the component is on the rootpane",
		isVisibleFlag = "{[boolean]} - true if the component is visible",
		mouselisteners = "{[table]} - list of mouselisteners",
		keylisteners = "{[table]} - list of keylisteners",
		colors = "[table] - array containing all defaultcolors",
		focuscolors = "[table] - array containing all defaultcolors on focus",
		clipinsets ="[table] - top, right, bottom, left inset values, default = 0,0,0,0",
		index = "{[int]} - index id of the component",
		["skinnames.pressedskin"] = "{[string]} = component_pressed - default skin-string for pressed cases",
		["skinnames.hoveredskin"] = "{[string]} = component_hover - default skin-string for hovered cases",
		["skinnames.defaultskin"] = "{[string]} = component - default skin-string for default cases",
		["skinnames.focusedskin"] = "{[string]} = component_focus - default skin-string for focused cases",
	})
local idcnt = 0
LuxModule.registerclassfunction("new",
	[[(Component):(table class, int x,y,w,h, [Skin2D skin]) -
	creates a component with the given bounds of type class.
	A Component may optionally be initialized with a Skin2D skin, a visual
	representation of the Component. ]])
function Component.new (class, x,y,w,h,skin,acceptskinsize)
	x,y,w,h = x or 0, y or 0, w or 0, h or 0
	local self = {
		bounds = Rectangle:new(x,y,w,h),
		mouselisteners = {},
		keylisteners = {},
		skin = skin,
		acceptsSkinBounds = acceptskinsize==nil and true or acceptskinsize
	}
	setmetatable(self,{
		__index = class,
		__tostring = function (self) return self:toString() end,
		__gcwatch = gcwatch(function() if not self.destroyed then self:onDestroy() end end)
	})
	idcnt = idcnt + 1
	self.__id = idcnt

	self.basel2d = l2dnode.new("component")
	self.basel2d:pos(math.floor(x+0.5),math.floor(y+0.5),0)
	self.basel2d.collectforgc = true
	self.basel2d:scissor(self:isClipped())
	self.basel2d:scissorparent(self:isClipped())
	self.basel2d:scissorlocal(true)
	self.basel2d:scissorsize(w+1,h+1)

	self.isDisplayedFlag = false
	self.isVisibleFlag = true
	self:invalidate()

	--table.insert(focusorder,self)

	return self
end

function Component.setGlobalGUITex(path)
	sgui:reload(path)
end

LuxModule.registerclassfunction("setClip",
	[[():([Component],boolean on) - Disables/Enables the clipping of the component(s).
	If no Component is passed, this is used as a GLOBAL parameter, not per component.
	It will affect only components that are going to be created.
	Clipping is done using the scissortest of OpenGL.
	These areas however can only be rectangular and may produce unwanted sideeffects
	if you use a different window.refsize than the screen resolution or if
	you want to rotate the components (which is possible but yet supported).
	Setting the clip to nil for a component will make it use the global parameter
	value. Currently, this parameter is only working correctly if disabled globally. ]])
function Component:setClip(on)
	if type(self) ~= "table" then
		Component.clipping = self
	else
		self.clipping = on
		self.basel2d:scissor(true)
		self.basel2d:scissorparent(true)
	end
end

LuxModule.registerclassfunction("isClipped",
	[[(boolean):([Component]) - returns state of clipping, is the global or of the
	component]])
function Component:isClipped ()
	return self and self.clipping or Component.clipping or false
end

LuxModule.registerclassfunction("setFont",
	[[():(Component,[string font]) - sets fontname to be used. Pass noting if
	you want to use the defaultfont again.]])
function Component:setFont (font)
	if self.skin then self.skin:setFont(font) end
	self.font = font
	self:invalidate()
end

LuxModule.registerclassfunction("acceptSkinBounds",
	[[([accepts]):(Component,[accepts]) - sets or gets behaviour of sizing.
	A skin may have other preferred sizes than chosen by the component. In
	that case, the component may have different preferred sizes than expected.
	Per default, this value is true.

	If no skin is set, this value has no meaning]])
function Component:acceptSkinBounds (accept)
	if (accept == nil) then return self.acceptsSkinBounds end
	if (self.acceptsSkinBounds~=accept) then
		self.acceptsSkinBounds = accept
		self:invalidate()
	end
end

LuxModule.registerclassfunction("setDefaultUI",
	[[():([skin],[icons]) - sets the skin and iconset to be used as default.
	this will not affect existing GUI components, only new GUI elements are
	affected.]])
function Component.setDefaultUI (skin,icons)
	framedefskin = skin or framedefskin
	defaulticon16x16set	= icons or defaulticon16x16set
end

LuxModule.registerclassfunction("setSkin",
	[[():(Component, [Skin2D,[boolean copy] ]) - sets a skin to be used for displaying this
	component. If no value is set, the skin is removed. Invalidates the
	Component.

	If then skin is a string named "default" a default skin is used.

	If copy is true, the skin's cloning function is called to create a copy of
	that skin.

	The Component class is using following skin surface mappings:
	* component - simple default surface
	* component_hover - highlighted surface if mouse is over the component
	* component_pressed - if the mouse is clicked on the component
	]])
function Component:setSkin (skin,copy)
	if (skin == "default") then skin,copy = framedefskin,true end
	local caption = self.skin and self.skin:getLabelText()
	local icon = self.skin and self.skin:getIcon()
	local iconskin = self.skin and self.skin:getIconSkinSelection()
	local skinselection = self.skin and self.skin:getSelectedSkin()
	if (self.skin == skin) then return end
	if (self.skin) then
		self.skin:deleteVisibles()
	end

	self.skin = copy and skin:clone() or skin

	if skin then
		if (self.isDisplayedFlag) then
			self.skin:createVisibles(self.basel2d)
		else
			self.skin:deleteVisibles()
		end
		--self.skin:setVisibility(self:isVisible())
		self.skin:selectSkin(self:getCurrentSkinName())
		self.skin:setLabelText(caption)
		self.skin:setIcon(icon)
		self.skin:setIconSkinSelection(iconskin)
		self.skin:setBounds(0,0,self.bounds[3],self.bounds[4])

		--self.skin:selectSkin(skinselection)
	end

	--self:invalidate(Rectangle:new(self.bounds))
end


function Component:getCurrentSkinName ()
	return
			(self.mouseisinside or self:isMouseLocker()) and
			self.skinnames.hoveredskin or self:hasFocus() and
			self.skinnames.focusedskin or self.skinnames.defaultskin
end

LuxModule.registerclassfunction("getSkin",
	[[([Skin2D]):(Component) - returns the skin used for this component]])
function Component:getSkin ()
	return self.skin
end

LuxModule.registerclassfunction("setBounds",
	[[():(Component self, float x,y,w,h) - sets bounds of the Component. Changes
	will be done when validate is called, which is done every frame once for
	all (drawn) components.]])
function Component:setBounds (x,y,w,h)
	if (self.skin and self.acceptSkinBounds) then
		local gx,gy = self:local2world(0,0)
		x,y,w,h = self.skin:getPreferredBounds(x-gx,y-gy,w,h)
		x,y = x+gx, y+gy
	end
	x,y = math.floor(x+0.5),math.floor(y+0.5)
	self.basel2d:pos(x,y,0)
	self.basel2d:scissor(self:isClipped())
	self.basel2d:scissorlocal(true)
	self.basel2d:scissorparent(true)
	self.basel2d:scissorsize(w+1,h+1)
	self.bounds[1],self.bounds[2],self.bounds[3],self.bounds[4] =
		x,y,w,h
	if (self.skin) then
			--print("Skin update",self)
			--z = self.skin:setSortID(z)
		--self.skin:setVisibility(isvis)
		self.skin:setBounds(0,0,self.bounds[3],self.bounds[4])
		self.basel2d:scissorsize(self.bounds[3]+1,self.bounds[4]+1)
		self.skin:selectSkin(self:getCurrentSkinName ())
	end

	self:positionUpdate(0,self.bounds)
	self.basel2d:rfNodraw(not self.isVisibleFlag)
	--self.dirt = Rectangle:new(x,y,w,h)
	--print(x,y,w,h,self)
	--self:invalidate(Rectangle:new(x,y,w,h))
end


LuxModule.registerclassfunction("lockMouse",
	[[():(Component) - locks the mouse to this component. As long as the
	mouse is locked, no other component will receive mouseevents. This
	does not affect MouseListeners that are attached to the MouseCursor. The current
	MouseCursor coordinates are stored and can be queried later with Component.getMouseLockPos.]])
function Component:lockMouse (x,y)
--print "LOCK"
	mouselock = self
	mlockpos = {MouseCursor.pos()}
end

LuxModule.registerclassfunction("unlockMouse",
	[[():(Component) - releases the mouselock,
	but only if the Component is also the locker of the mouse.]])
function Component:unlockMouse ()
	if (self == mouselock) then
	--print "UNLOCK"
		mouselock = nil
		mlockpos = nil
		local root = Container.getRootContainer()
		root:mouseEvent(root:getLastMouseEvent())
	end
end

LuxModule.registerclassfunction("getMouseLockPos",
	[[([x,y]):([Component]) - returns the coordinates of the MouseCursor when lock occured in component's space or nil.]])
function Component.getMouseLockPos (c)
	if (not mlockpos) then return end
	if (c) then
		return c:world2local(mlockpos[1],mlockpos[2])
	else
		return mlockpos[1],mlockpos[2]
	end
end

LuxModule.registerclassfunction("getMouseLock",
	[[(Component):() - returns the Component that is currently locking the mouse.]])
function Component.getMouseLock ()
	return mouselock
end

LuxModule.registerclassfunction("isMouseLocker",
	"(boolean):(Component) - returns true if the component is locking the mouse")
function Component:isMouseLocker ()
	return mouselock == self
end

LuxModule.registerclassfunction("setTooltip",
	[[():(Component self, [nil / string text / Component component]) - Sets tooltip
	information for this component, which is shown if the mouse holding its position.
	Passing nil will delete the tooltip information, setting a string will show the
	string as a simple textlabel, which is multilinecapable. If a component is
	passed, this component will be shown in the quickinfobox or whatever is
	showing the results. Setting the tooltip while it is shown will not affect the
	way it is displayed]])
function Component:setTooltip (info)
	assert(self and self.basel2d,"not a component object (messed up . and : ?)")
	self.component_tooltipinfo = info
end

LuxModule.registerclassfunction("getTooltip",
	[[([string text / Component component]):(Component self) - returns the
	set tooltip information or nil if not present]])
function Component:getTooltip ()
	assert(self and self.basel2d,"not a component object (messed up . and : ?)")
	return self.component_tooltipinfo
end

LuxModule.registerclassfunction("addTooltipListener",
	[[(function):(Component self, function) - adds a tooltiplistener. The
	function will receive a Tooltipevent as only argument when fired.]])
function Component:addTooltipListener (func)
	assert(self and self.basel2d,"not a component object (messed up . and : ?)")
	self.component_tooltiplisteners = self.component_tooltiplisteners or { n = 0 }
	table.insert(self.component_tooltiplisteners,func)
	return func
end

LuxModule.registerclassfunction("removeTooltipListener",
	[[():(Component self, function) - removes all functions that are equal
	to entries in the tooltiplistener list]])
function Component:removeTooltipListener (func)
	assert(self and self.basel2d,"not a component object (messed up . and : ?)")
	self.component_tooltiplisteners = self.component_tooltiplisteners or {}
	local list = self.component_tooltiplisteners
	local i = 1
	while i<#list do
		if list[i] == func then table.remove(list,i)
		else i = i + 1 end
	end
end
LuxModule.registerclassfunction("showTooltipInformation",
	[[() : (Component self) - triggers displaying the tooltip information.
	It will look if the component has a function set named ]])
function Component:showTooltipInformation (tooltipevent)
	assert(self and self.basel2d,"not a component object (messed up . and : ?)")
	local event = tooltipevent or new(Tooltipevent,self)
	local function fireevent (cmp)
		for i,listener in ipairs(cmp.component_tooltiplisteners or {}) do
			listener(event)
		end
		if cmp.parent then
			fireevent(cmp.parent)
		end
	end
	fireevent(self)
end

LuxModule.registerclassfunction("setLocation",
	[[():(Component self, float x,y) - sets location of the Component]])
function Component:setLocation (x,y)
	local bounds = self:getBounds()
	self:setBounds(x,y,bounds[3],bounds[4])
end

LuxModule.registerclassfunction("setSize",
	[[():(Component self, float w,h) - sets size of the Component]])
function Component:setSize (w,h)
	local bounds = self:getBounds()
	self:setBounds(bounds[1],bounds[2],w,h)
end

LuxModule.registerclassfunction("getMinSize",
	[[(Rectangle):(Component self) - returns minimum size of component]])
function Component:getMinSize()
	return self.bounds[3],self.bounds[4]
end

LuxModule.registerclassfunction("getSize",
	[[(float w,h):(Component self) - gets size of the Component]])
function Component:getSize ()
	local bound = self:getBounds()
	return bound[3],bound[4]
end

LuxModule.registerclassfunction("getBounds",
	[[(Rectangle rect):() - returns a copy of the current bounds, EVEN
	if the component hasn't been validated yet.]]) --']]
function Component:getBounds ()
	return Rectangle:new(type(self.dirt)=="table" and self.dirt or self.bounds)
end

LuxModule.registerclassfunction("getLocation",
	"(int x,y):() - returns current Location")
function Component:getLocation()
	local b = self:getBounds()
	return b[1],b[2]
end

LuxModule.registerclassfunction("getParent",
	"(Container):() - returns parent container if exists")
function Component:getParent()
	return self.parent
end

LuxModule.registerclassfunction("getHeight", "(int):() - returns current height")
function Component:getHeight () return self:getBounds()[4] end
LuxModule.registerclassfunction("getWidth", "(int):() - returns current width")
function Component:getWidth () return self:getBounds()[3] end
LuxModule.registerclassfunction("getBottom", "(int):() - returns y+height, which is the bottomline of the component")
function Component:getBottom () return self:getBounds()[4]+self:getBounds()[2] end
LuxModule.registerclassfunction("getRight", "(int):() - returns x+width, which is the rightmostline of the component")
function Component:getRight () return self:getBounds()[3]+self:getBounds()[1] end
LuxModule.registerclassfunction("getX", "(int):() - returns current local x origin")
function Component:getX () return self:getBounds()[1] end
LuxModule.registerclassfunction("getY", "(int):() - returns current local y origin")
function Component:getY () return self:getBounds()[2] end

LuxModule.registerclassfunction("local2world",
	[[(int worldx,worldy):(int localx,localy) - returns world coordinates]])
function Component:local2world (x,y)
	local bounds = type(self.dirt)=="table" and self.dirt or self.bounds
	if (self.parent) then return self.parent:local2world(x+bounds[1],y+bounds[2]) end
	return self.bounds[1]+x, self.bounds[2]+y
end

LuxModule.registerclassfunction("world2local",
	[[(int localx,localy):(int worldx,worldy) - returns local coordinates]])
function Component:world2local (x,y)
	local bounds = type(self.dirt)=="table" and self.dirt or self.bounds
	if (self.parent) then return self.parent:world2local(x-bounds[1],y-bounds[2]) end
	return x-self.bounds[1], y-self.bounds[2]
end

LuxModule.registerclassfunction("getClipRect",
	[[(Rectangle):(Component) - returns clipping of the component. The function
	traverses to the root to find out the clipping region until the component
	has no parent any more.]])
function Component:getClipRect()
	local clip = Rectangle:new (
		self.bounds[1]+self.clipinsets[4],
		self.bounds[2]+self.clipinsets[1],
		self.bounds[3]-self.clipinsets[2],
		self.bounds[4]-self.clipinsets[3])
	clip:translate(self:local2world(-self.bounds[1],-self.bounds[2]))
	if (self.parent) then
		return Rectangle.intersection(clip,self.parent:getClipRect())
	else
		return clip
	end
end

LuxModule.registerclassfunction("delete",
	[[():(Component self) - deletes the component. Removes the component from
	any parent object and calls Component.onDestroy, that is also called when
	the Component is garbagecollected. Never overload delete, use onDestroy for custom actions instead.]])
function Component:delete ()
	if (self.destroyed) then return nil end
	local i = self:getFocusIndex()
	if (focuselement == self) then
		self:transferFocus(1)
	end
	if (focuselement == self) then
		focuselement = nil
	end
	if (self.skin) then self.skin:deleteVisibles() end
	self.skin = nil -- let the gc do the work from here on for the skin
	if (i and i>0) then
		table.remove(focusorder,i)
	end
	self:unlockMouse()
	self:remove()
	self:onDestroy()
	self.destroyed = true
end

LuxModule.registerclassfunction("onDestroy",
	[[():(Component self) - called if the garbage collector collects the
	object or if the component was deleted. Overload this method for
	custom actions, i.e. deleting all l2dnodes etc.]])
function Component:onDestroy () end

LuxModule.registerclassfunction("remove",
	[[():(Component self) - removes the Component from its parent
	object.]])
function Component:remove ()
	if (self.parent) then
		--self.basel2d:link(l2dlist.getroot())
		local parent = self.parent
		self.parent = nil
		parent:removeComponent(self)
		if (self.skin) then self.skin:deleteVisibles() end

		if 	(self.isDisplayedFlag) then
			self.isDisplayedFlag = false
			self:onDisplayChange(false)
		end
	end
end


--------------------------------------------------------------------------------
-- visibility stuff ------------------------------------------------------------

LuxModule.registerclassfunction("deleteVisibles",
	[[():(Component self) - called when a Component is no longer displayed]])
function Component:deleteVisibles ()
end

LuxModule.registerclassfunction("createVisibles",
	[[():(Component self,l2dnode basel2d) - called when a Component is now displayable
	(doesn't have to be visible)]])--[[']]
function Component:createVisibles ()
end

LuxModule.registerclassfunction("showVisibles",
	[[():(Component self) - called when visible objects (l2ds) are now visible]])
function Component:showVisibles ()
	self.basel2d:rfNodraw(false)
end

LuxModule.registerclassfunction("hideVisibles",
	[[():(Component self) - called when visible objects (l2ds) are no longer visible]])
function Component:hideVisibles ()
	self.basel2d:rfNodraw(true)
end

LuxModule.registerclassfunction("onDisplayChange",
	[[():(Component self, boolean isDisplayed) - called whenever the
	displaystate is changed, i.e. if the component was made invisible,
	or it's parent was removed from the rootpane etc.

	A component that is displayed has not to be visible.]])
function Component:onDisplayChange (isDisplayed)
	self.basel2d:rfNodraw(not (isDisplayed and self.isVisibleFlag))
	if (isDisplayed) then
		self:createVisibles(self.basel2d)
		if self.skin then
			self.skin:createVisibles(self.basel2d)
			--self.skin:setVisibility(self:isVisible())
		end
	else
		self:deleteVisibles()
		if self.skin then
			self.skin:deleteVisibles()
			--self.skin:setVisibility(self:isVisible())

		end
	end
	self:updateSkin()
end

LuxModule.registerclassfunction("onVisibilityChange",
	[[():(Component self, boolean isVisible) - called whenever
	the component is made visible / invisible.]])
function Component:onVisibilityChange (isVisible)
	if (isVisible) then
		self:showVisibles()
		--if self.skin then self.skin:setVisibility(true) end
	else
		self:hideVisibles()
		--if self.skin then self.skin:setVisibility(false) end
	end
	self:updateSkin()
end

LuxModule.registerclassfunction("updateSkin",
	[[():(Component self) - updates skin information ]])
function Component:updateSkin ()
	self.basel2d:pos(math.floor(self:getX()+0.5),math.floor(self:getY()+0.5),0)
	if self.skin then
		local wx,wy = self:local2world(0,0)
		self.skin:setBounds(0,0,self.bounds[3],self.bounds[4])

		self.skin:selectSkin(self:getCurrentSkinName())
	end
end

LuxModule.registerclassfunction("setVisibility",
	[[(boolean):(Component self) - returns true if the component is
	displayed (is actually on the rootpane) and is visible.]])
function Component:setVisibility (visible)
	local before = self.isVisibleFlag
	self.isVisibleFlag = visible
	self.basel2d:rfNodraw(not visible)
	if (before~=self.isVisibleFlag) then
		self:onVisibilityChange(self:isVisible())
		--if (self.skin) then self.skin:setVisibility(not before)  end
	end
	if (not visible and self:hasFocus()) then
		self:transferFocus(1)
	end
end

LuxModule.registerclassfunction("setDisplayable",
	[[():(Component self, boolean displayable) -
	a displayable component should create all it needs to be displayed
	(l2dnodes & stuff).]])
function Component:setDisplayable (displayable)
	if (self.isDisplayedFlag ~= displayable) then
		self.isDisplayedFlag = displayable
		self:onDisplayChange(displayable)
	end
end

LuxModule.registerclassfunction("isVisible",
	[[(boolean):(Component self) - returns true if the component
	is displayable and visible, otherwise false]])
function Component:isVisible()
--_print(":::",self.isDisplayedFlag,self.isVisibleFlag)
	local visible = self.isDisplayedFlag and self.isVisibleFlag
	assert (self.parent~=self)
	if (self.parent and visible) then
		return visible and self.parent:isVisible()
	end
	return visible
end

LuxModule.registerclassfunction("validateFocus",
	[[():(Component self) - Called once a frame and build a list of focusable components
	]])
function Component.validateFocus()
	local stack = {Container.getRootContainer()}
	focusorder = {}
	while #stack>0 do
		local e = table.remove(stack)
		for i=e.components and #e.components or 0,1,-1 do
			table.insert(stack,e.components[i])
		end
		if e:isFocusable() and not e:isDisabled() then
			table.insert(focusorder,e)
		end
	end

	if (focuselement == nil or not focuselement:isVisible()) then
		if focusorder[1] then focusorder[1]:focus() else
			focuselement = nil
		end
	end
end

LuxModule.registerclassfunction("validate",
	[[([int z]):(Component self, [int z]) - validates the new position of the component.
	don't call this method during a mouseevent or strange things may happen.
	If the Component's bounds were changed, positionUpdate will be called.
	The z parameter is the z-sort order and the component should return an
	incremented z value, reserving the z-values it needs for its own
	(validation is down in z-sorted order). If no z is given, no reordering
	is done.]])
function Component:validate (z,clip)
	--print(debug.traceback())
	--print("validating",self)
	local bounds = type(self.dirt)=="table" and self.dirt or self.bounds
	local wx,wy = self:local2world(0,0)

	--if not focusorder[self] and self:isFocusable() then
	--	table.insert(focusorder,1,self)
	--	focusorder[self] = true
	--end


	clip = clip or self.clip
	z = z or self.zid
	clip = Rectangle.intersection(
		Rectangle:new (
			self.clipinsets[4]+wx,
			self.clipinsets[1]+wy,
			bounds[3]-self.clipinsets[2],
			bounds[4]-self.clipinsets[3]),
		clip)
	self.clip = clip
	local isvis = self:isVisible()
	if (self.dirt or z ~= self.zsortid) then
		--print("comp validate?",self)

		if (self.dirt~=true and self.dirt) then
			self.bounds = self.dirt
		end
--		assert(self.bounds)

		if (self.skin) then
			--print("Skin update",self)
			--z = self.skin:setSortID(z)
		--	self.skin:setVisibility(isvis)
		--	self.skin:setBounds(0,0,self.bounds[3],self.bounds[4])
		--	self.basel2d:scissorsize(self.bounds[3],self.bounds[4])
			--print(wx,wy,self.bounds[3],self.bounds[4])
			--if (clip) then self.skin:setClip(unpack(clip))
			--else self.skin:setVisibility(false) end
		--	print(self.skin)
		end

		self.zsortid = self:positionUpdate(z,clip)
		z = self.zsortid

	end
	self.dirt = nil
	self.zid = z
	return z
end



LuxModule.registerclassfunction("invalidate",
	[[():(Component self) - marks component to be updated on next validation]])
function Component:invalidate (flag)
    --print("invalidation running",self,self.dirt)
    if self.dirt and not flag then return end
	self.dirt = flag or self.dirt or true

	if self.parent then self.parent:invalidate() end
	--local browse = self.parent
	--while (browse) do
		--browse.dirt = browse.dirt or true
		--browse = browse.parent
	--end
end

function Component:invalidateAll()
	self:invalidate()
end


LuxModule.registerclassfunction("think",
	[[():(Component self) - called each frame if the component is visible and
	inserted in the rootpane or one of its ancestors. Overload this method
	to perform your own operations here]])
function Component:think () end

LuxModule.registerclassfunction("positionUpdate",
	[[(int newz):(Component self,int zindex,Rectangle clip) - called when the component was moved or resized.
	Overload this method for visual appearance. The zindex is the id to use for
	zordering the components. Return the newz value, reserving all the values that
	you require for your visual nodes.]])
function Component:positionUpdate(z,clip)
	return z end

LuxModule.registerclassfunction("contains",
	[[(boolean):(Component self, float x,y) - checks if the given point is
	inside the component. Overload this method if you need more specific
	hittests than a rectangletest. Coordinates are relative.]])
function Component:contains (x,y)
--	_print(string.format("%s %.2f %.2f",tostring(self.bounds),x,y))
	local con = self.bounds:contains(x+self.bounds[1],y+self.bounds[2])
	return con and (self.skin==nil and true or self.skin:contains(x,y))
end

-- end of visibility stuff -----------------------------------------------------
--------------------------------------------------------------------------------


--------------------------------------------------------------------------------
--- Focusing -------------------------------------------------------------------

LuxModule.registerclassfunction("transferFocus",
	[[():(Component self, int direction) - call this to transfer the focus in a direction
	relative to the component (forward/backward]])
function Component:transferFocus(direction)
	local focus = self:getFocusComponentAt(direction)

	if (focus and focus ~= self) then
		focus:focus()
		self:invalidate()
	end
end

LuxModule.registerclassfunction("getFocusIndex",
	"():(Component self) - returns absolute index of the focusorder")
function Component:getFocusIndex ()
	for a,b in ipairs(focusorder) do
		if ( b == self ) then return a end
	end
end

LuxModule.registerclassfunction("focus",
	[[():(Component self) - registers an element as
	the new focusowner.]])
function Component:focus()
	local prev = focuselement
	focuselement = self
	if (prev) then
		prev:onLostFocus(self)
	end

	self:onGainedFocus(prev)
end


LuxModule.registerclassfunction("getFocusElement",
	[[([Component]):() - returns the current element of focus if available]])
function Component:getFocusElement() 	return focuselement end

LuxModule.registerclassfunction("hasFocus",
	[[(boolean):(Component) - returns true if the element is focused]])
function Component:hasFocus() 	return self == focuselement end


LuxModule.registerclassfunction("isFocusable",
	[[(boolean):(Component) - returns true if the component can be focused.
	If you overload this method, please remember that you should return false
	if your element is current not focusable, ie. if not visible or not enabled.

	For example:
	 function TextField.isFocusable()
	   return true and self:isVisible()
	 end
	]])
function Component:isFocusable() return false end

LuxModule.registerclassfunction("onLostFocus",
	[[():(Component,Component tocomponent) - event if the component lost the
	focus to tocomponent.]])
function Component:onLostFocus(tocomponent)
	if (self.skin) then self.skin:selectSkin((self.mouseisinside or self:isMouseLocker())and self.skinnames.hoveredskin or self.skinnames.defaultskin) end
end

LuxModule.registerclassfunction("onGainedFocus",
	[[():(Component,Component fromcomponent) - event if the component
	gained the focus from fromcomponent.]])
function Component:onGainedFocus(fromcomponent)
	if (self.skin) then self.skin:selectSkin(self.skinnames.focusedskin) end
end

LuxModule.registerclassfunction("getFocusComponentAt",
	[[(Component):([Component], int direction) -
	returns focuselement. If Component is given, the direction is relative
	to the component, if not, the absolute focuselement is returned.
	Only elements that return true on isFocusable are counted. ]])
function Component:getFocusComponentAt (direction)
	local pos,start,n
	n = table.getn(focusorder)

	if (direction) then
		pos = self:getFocusIndex()
		if (not pos) then return nil end
		start = pos
		local sdir = direction
		if (pos<1) then return self end
		while (direction~=0) do
			pos = pos + (direction<0 and -1 or 1)
			if (pos>n) then pos = pos - n end
			if (pos<1) then pos = pos + n end
			if (focusorder[pos] and focusorder[pos]:isFocusable()) then
				direction = direction<0 and (direction + 1) or (direction - 1)
			end
			if (pos == start and sdir == direction) then return self end
		end
--		_print(pos,n,focusorder[pos])
		return focusorder[pos]
	elseif(self>0) then
		pos = self
		for i=1,n do
			if (focusorder[i]:isFocusable()) then pos = pos - 1 end
			if (pos<1) then return focusorder[i] end
		end
	end

	return nil
end

LuxModule.registerclassfunction("newFocusOrder",
	[[(table old):(table new) -
	Sets a new focus order list as given in the passed table.
	It returns the old focuslist then and sets the current focuselement to nil.
	This method can be used to create modal dialogs, though you have to
	watch out to restore the list again, once the old elements should be
	accesible again. ]])
function Component.newFocusOrder(order)
	local old = focusorder
	focusorder = order
	focuselement = nil
	return old
end


LuxModule.registerclassfunction("transferFocusOnTab",
	[[(boolean):(Component) - returns true if the component transfers the
	focus if tab is pressed]])
function Component:transferFocusOnTab()	return true end

LuxModule.registerclassfunction("transferFocusOnArrows",
	[[(boolean):(Component) - returns true if the component transfers the
	focus if an arrow is pressed is pressed]])
function Component:transferFocusOnArrows()	return true end

LuxModule.registerclassfunction("setDisabled",
	[[():(Component,boolean yes) - disabled components won't generate mouse or keyevents.
	The Component's skin is set to half transparent, storing the previous alpha value and
	keeping the current rgb value.
	]])
function Component:setDisabled(yes)
	assert(type(yes)=="boolean","disable argument must be boolean")
	if (self._isdisabled==yes) then return end -- do nothing then
	if self:getSkin() then
		local skin = self:getSkin()
		local r,g,b,a1 = skin:getSkinColor()
		skin:setSkinColor(r,g,b,yes and 0.5 or self._disabledSkinAlpha)
		local r,g,b,a2 = skin:getFontColor()
		skin:setFontColor(r,g,b,yes and 0.5 or self._disabledFontAlpha)
		self._disabledSkinAlpha = a1
		self._disabledFontAlpha = a2

		if skin:getIcon()then
			local r,g,b,a3 = skin:getIcon():getColor()
			skin:getIcon():setColor(r or 1,g or 1,b or 1,yes and .35 or self._disabledIconAlpha or 1)
			self._disabledIconAlpha = a3
		end
	end
	self._isdisabled = yes
end
function Component:isDisabled() return self._isdisabled end

-- end of focusing -------------------------------------------------------------
--------------------------------------------------------------------------------


--------------------------------------------------------------------------------
-- key and mouselisteners ------------------------------------------------------
LuxModule.registerclassfunction("addMouseListener",
	"():(Component self, MouseListener ml) - add a mouselistener to the component")
function Component:addMouseListener (ml)
	table.insert(self.mouselisteners,ml)
end

LuxModule.registerclassfunction("removeMouseListener",
	"():(Component self, MouseListener ml) - removes ml from the list of mouselisteners")
function Component:removeMouseListener (ml)
	for i,v in ipairs(self.mouselisteners) do
		if (v == ml) then
			table.remove(self.mouselisteners,i)
			return
		end
	end
end

LuxModule.registerclassfunction("addKeyListener",
	"():(Component self, KeyListener kl) - add a keylistener to the component")
function Component:addKeyListener (ml)
	table.insert(self.keylisteners,ml)
end

LuxModule.registerclassfunction("removeKeyListener",
	"():(Component self, KeyListener kl) - removes kl from the list of keylisteners")
function Component:removeKeyListener (ml)
	for i,v in ipairs(self.keylisteners) do
		if (v == ml) then
			table.remove(self.keylisteners,i)
			return
		end
	end
end

-- end of key and mouselisteners -----------------------------------------------
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- various stuff



LuxModule.registerclassfunction("setColor",
	[[():(Component self, table skincolor,table labelcololor, table iconcolor, [mix a=0], [mix b=1]) -

	]])
function Component:setColor(skincol,labelcol,iconcol,a,b)
	return Component:setColorAll(skincol,labelcol,iconcol,a,b)
end
function Component:setColorAll(skincol,labelcol,iconcol,a,b)
	--print(self,",",skincol,",",labelcol,",",iconcol,",",a,",",b)
	a,b = a or 0, b or 1

	assert(type(a)=="number" and type(b)=="number", "a and b must be of type number but are "..
		tostring(a).." and "..tostring(b))
	local function mix(rr,gg,bb,aa,to)
		return rr*a+b*to[1],gg*a+b*to[2],bb*a+b*to[3],aa*a+b*to[4]
	end
	if self.setColor~=Component.setColor then
		local rr,gg,bb,aa = self:getColor()
		self:setColor(mix(rr,gg,bb,aa,self.getColor==MultiLineLabel.getColor and labelcol or skincol))
	end
	if self:getSkin() then
		if skincol then
			local rr,gg,bb,aa = self:getSkin():getSkinColor()
			self:getSkin():setSkinColor(mix(rr,gg,bb,aa,skincol))
		end
		if labelcol then
			local rr,gg,bb,aa = self:getSkin():getFontColor()
			self:getSkin():setFontColor(mix(rr,gg,bb,aa,labelcol))
		end
		if self:getSkin():getIcon() and iconcol then
			local rr,gg,bb,aa = self:getSkin():getIcon():color()
			self:getSkin():getIcon():color(mix(rr,gg,bb,aa,iconcol))
		end
	end
	if self.components then
		for i,c in ipairs(self.components) do
			c:setColorAll(skincol,labelcol,iconcol,a,b)
		end
	end
end

LuxModule.registerclassfunction("fadeTo",
	[[():(Component self, table skincolor,table labelcololor, table iconcolor, speed,delay,onfinish,oncancel) -

	]])
function Component:fadeTo (skinto,labelto,iconto,speed,delay,onfinish,oncancel)
	local function void () end
	onfinish,oncancel = onfinish or void, oncancel or void
	delay = delay or 20
	if self.fadetoanimation then
		self.fadetoanimation.cancel(self)
		self.fadetoanimation.stop = true
	end
	local anim = {}
	anim.cancel = oncancel
	self.fadetoanimation = anim


	TimerTask.new(function ()
		local a,b = 1-speed,speed
		local t = 0
		repeat
			self:setColorAll(skinto,labelto,iconto,a,b)
			t = t*a+b
			coroutine.yield(delay)
			if anim.stop then return end
		until t>0.99
		self:setColorAll(skinto,labelto,iconto,0,1)
		onfinish(self)
		self.movetoanimation = nil
	end,delay)
end

LuxModule.registerclassfunction("moveToRect",
	[[():(Component self, x,y,w,h,speed,[delay],[onfinish],[oncancel]) -
	moves a component animated to a certain target. The given x,y, w,h are the target
	bounds. The speed must be a factor between 0 and 1, where 1 is the fastest while
	0 will never reach the target. The delay is the amount of milliseconds to be
	waited after each step (it is using a TimerTask to perform the animation).
	The onfinish function is the function to be called if the target is reached.

	If the animation is interupted by another animation call, the oncancel function
	will be called.]])
function Component:moveToRect(tx,ty,tw,th,speed,delay,onfinish,oncancel)
	local function void() end
	onfinish,oncancel = onfinish or void, oncancel or void
	local a,b = 1-speed,speed
	animfunc = animfunc or function(x,y,w,h,ox,oy,ow,oh,tx,ty,tw,th)
			return x*a+tx*b,	y*a+ty*b,	w*a+tw*b,	h*a+th*b
		end
	delay = delay or 20
	if self.movetoanimation then
		self.movetoanimation.cancel(self)
		self.movetoanimation.stop = true
		-- this code
		TimerTask.disable(self.movetoanimation.task)
		if self.movetoanimation.corot then

			local cr = self.movetoanimation.corot
			while coroutine.status(cr)=="running" do
				coroutine.resume(cr)
			end
		end
		-- till here was added to avoid flooding the
		-- timer with useless coroutines
	end
	local anim = {}
	anim.cancel = oncancel
	self.movetoanimation = anim
	local x,y,w,h = unpack(self:getBounds())
	local ox,oy,ow,oh = x,y,w,h
	anim.task = TimerTask.new(function ()
		anim.corot = coroutine.running ()
		local px,py,pw,ph
		local function rnd (val) return math.floor(val+0.5) end
		local cnt = 1
		repeat
			local dx,dy,dw,dh = x-tx,y-ty,w-tw,h-th
			x,y,w,h =
				x*a+tx*b,
				y*a+ty*b,
				w*a+tw*b,
				h*a+th*b
			local cx,cy,cw,ch = rnd(x),rnd(y),rnd(w+x),rnd(h+y)
			-- why w+x and h+y? => a bit smoother, if x=3.3 and w=3.3 => no change,
			-- but it would be, because the right side of the component should be farer right
			-- at least I think so .... -- zet
			if px~=cx or py~=cy or pw~=cw or ph~=ch then
				self:setBounds(cx,cy,rnd(w+x)-cx,rnd(h+y)-cy)
				px,py,ph,pw = cx,cy,cw,ch
			end
			coroutine.yield(delay)
			cnt = cnt + 1
			if anim.stop then return end
		until dx*dx+dy*dy+dw*dw+dh*dh<2
		self:setBounds(tx,ty,tw,th)
		onfinish(self)
		self.movetoanimation = nil
	end,delay)
end

--
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- event handling --------------------------------------------------------------


LuxModule.registerclassfunction("mouseMoved",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	called if the mouse actually moved over the Component. ]])
function Component:mouseMoved(me) end

LuxModule.registerclassfunction("mousePressed",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	called if the mouse was pressed on the component. If mouseover is true,
	the mouse is actually on THIS component and not on one of its childs,
	which will create events too.]])
function Component:mousePressed(me)
	if (self.skin) then self.skin:selectSkin(self.skinnames.pressedskin) end
end

LuxModule.registerclassfunction("mouseClicked",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	called if the mouse was clicked on the component.]])
function Component:mouseClicked(me) end

LuxModule.registerclassfunction("mouseEntered",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	called if the mouse is entered the Component]])
function Component:mouseEntered(me)
	self.mouseisinside = true
	if (self.skin) then self.skin:selectSkin(self.skinnames.hoveredskin) end
end

LuxModule.registerclassfunction("mouseExited",
	[[():(Component self, MouseEvent e) -
	called if the mouse is exited the component]])
function Component:mouseExited(me)
	self.mouseisinside = false
	if (self.skin and not self:isMouseLocker()) then self.skin:selectSkin(self:hasFocus() and self.skinnames.focusedskin or self.skinnames.defaultskin) end
end

LuxModule.registerclassfunction("mouseReleased",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	 called if the mouse was released on the component]])
function Component:mouseReleased(me)
	if (self.skin) then self.skin:selectSkin((self.mouseisinside or self:isMouseLocker()) and self.skinnames.hoveredskin or self:hasFocus() and self.skinnames.focusedskin or self.skinnames.defaultskin) end
end

LuxModule.registerclassfunction("mouseWheeled",
	[[():(Component self, MouseEvent e, boolean mouseover) -
	called if the mousewheel was moved on the component]])
function Component:mouseWheeled(me)
end


LuxModule.registerclassfunction("mouseEvent",
	[[():(Component self, MouseEvent e) -
	called if a mouseevent occurred
	that is interesting to this component. It will informa all mouselisteners
	of the component of this action. ]])
local lastmousecomp = {}
setmetatable(lastmousecomp,{__mode='v'})
function Component:mouseEvent(e)

	local wx,wy = self:local2world(e.x,e.y)
	if lastmousecomp.c and lastmousecomp.c~=self and lastmousecomp.c:isVisible() then
		local function exit (c)
			local e = e:set(c:world2local(wx,wy))
			c:mouseExited(MouseEvent.new(
				e.x,e.y,e.wheel,e.px,e.py,e.pwheel,e.button,
				e.eventtype*MouseEvent.MOUSE_EXIT,c,e.downsince))
			if c.parent and c.parent~=self then return exit(c.parent) end
		end
		exit(lastmousecomp.c)
	end
	if lastmousecomp.c~=self then
		--local known = {}
		--local browse = lastmousecomp or self
		--repeat known[browse] = true browse = browse.parent until not browse
		local function enter(c)
			local e = e:set(c:world2local(wx,wy))
			if not self:isDisabled() then
			c:mouseEntered(MouseEvent.new(
				e.x,e.y,e.wheel,e.px,e.py,e.pwheel,e.button,
				e.eventtype*MouseEvent.MOUSE_ENTER,self,e.downsince),c == self)
			end
			if c.parent then return enter(c.parent) end
		end
		enter(self)
		lastmousecomp.c = self
	end
	--if #self.mouselisteners>0 then print(e) end --print(debug.traceback(tostring(e))) end
	if not self:isDisabled() then
		for i,v in ipairs(self.mouselisteners) do
			if (v.lastshot~=system.frame() or v.lastshotevt~=tostring(e)) and v:eventcall(e) then
				-- it is possible that the eventcode is processed twice - 
				-- this is the case if the mouse got unlocked and components are 
				-- processed again
				-- to avoid that we fire the listener twice, we remember the last frame
				-- when we processed it.
				
				v.lastshot = system.frame()
				v.lastshotevt = tostring(e)
			end
		end
	end
	local function inform (who)
		local isself = who==self
		local e = e:set(who:world2local(wx,wy))
		if not self:isDisabled() then
			if e:isReleased() then who:mouseReleased(e,isself) end
			if e:isMoved() then who:mouseMoved(e,isself) end
			if e:isDragged() then who:mouseMoved(e,isself) end

			if e:isClicked() then who:mouseClicked(e,isself) end
			if e:isPressed() then who:mousePressed(e,isself) end
			if e:isWheeled() then who:mouseWheeled(e,isself) end
		end
		if who.parent then inform(who.parent) end
	end
	inform(self)
end


LuxModule.registerclassfunction("keyEvent",
	[[():([Component],KeyEvent) - called if a keyevent occurred and the component
	has the focus. If a tab was pressed and transferFocusOnTab returns true,
	the focus is transfered. If enter was pressed and a onAction function is
	present, onAction is called. Otherwise the components keylisteners are
	called. ]])
function Component:keyEvent(keyevent)
	if ( not self ) then
		if (keyevent.key=='\t' and keyevent.state == KeyEvent.KEY_TYPED) then
			self = Component.getFocusComponentAt(1)
		else return end
		if (not self) then return end
	end
	if (not self:isVisible()) then return end
	local foc = self:transferFocusOnArrows() and keyevent.state == KeyEvent.KEY_TYPED
	local prevfoc,nextfoc =
		{[Keyboard.keycode.UP]=foc,[Keyboard.keycode.LEFT]=foc},
		{[Keyboard.keycode.RIGHT]=foc,[Keyboard.keycode.DOWN]=foc}

	if (keyevent.key=='\t' and keyevent.state == KeyEvent.KEY_TYPED
		and self:transferFocusOnTab()) then
		local dir = (Keyboard.isKeyDown(Keyboard.keycode.LSHIFT) or
			Keyboard.isKeyDown(Keyboard.keycode.RSHIFT)) and -1 or 1
		self:transferFocus(dir)
	elseif (nextfoc[keyevent.keycode]) then
		self:transferFocus(1)
	elseif (prevfoc[keyevent.keycode]) then
		self:transferFocus(-1)
	elseif(((keyevent.key=='\n' and not self.noenteraction) or (keyevent.key==' ' and not self.nospaceaction))  and keyevent.state == KeyEvent.KEY_TYPED and self.onAction) then

		if not self:isDisabled() then self:onAction(self.getText and self:getText()) end
	else
		if not self:isDisabled() then
			for i,v in pairs(self.keylisteners) do
				v:eventcall(keyevent)
			end
		end
	end
end

-- end of eventhandling --------------------------------------------------------
--------------------------------------------------------------------------------

LuxModule.registerclassfunction("toString",
	[[(string):(Component self) - returns a simple string representation of this component]])
function Component:toString()
	return "[Component "..self.__id..":"..tostring(self.bounds).."]"
end

--------------------------------------------------------------------------------

Keyboard.addKeyListener(KeyListener.new(
	function (self, keyevent)
		if (console.active()) then return end
		xpcall(	function ()
					local self = focuselement
					if self then self:keyEvent(keyevent) else
						Component.keyEvent(nil,keyevent)
					end
				end,
				function (err)
					print("Component keylistener error: ")
					print(debug.traceback(err) )
				end
		)
	end,
		true,true,true)
)
