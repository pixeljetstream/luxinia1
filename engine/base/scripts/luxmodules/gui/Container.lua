local rootContainer -- rootcontainer is the frame of luxinia (everything)

Container = {}
LuxModule.registerclass("gui","Container",
	[[A container is a collection of Components.]],Container,
	{
		components = "{[table]} - list of child components on the container. Sorted from top to bottom. "..
		"Be aware that the table is changed if childs are inserted / removed, which means that "..
		"it that this should be avoided while iterating the list. Additionally, the table should not "..
		"be modified, or behaviour is undefined.",
	},"Component")
setmetatable(Container,{__index = Component})


LuxModule.registerclassfunction("new",
	[[(Container):(table class, int x,y,w,h) - creates a container with the given bounds]])
function Container.new (class,x,y,w,h)
	local self = Component.new(class,x,y,w,h)

	self.components = {}

	return self
end

LuxModule.registerclassfunction("delete", "@overloaded Component")
function Container:delete ()
	if (self.destroyed) then return end
	while (table.getn(self.components)>0) do
		self.components[table.getn(self.components)]:delete()
	end
	Component.delete(self)
end

LuxModule.registerclassfunction("deleteChilds", "():(Container) - deletes all childs of the container")
function Container:deleteChilds()
	while #self.components>0 do
		local n = #self.components
		local del = self.components[#self.components]
		del:delete()
		assert(n>#self.components,"Component "..tostring(del).." did not remove itself properly")
	end

end

LuxModule.registerclassfunction("moveZ", "():(Container,Component a/ number index,toindex) - deletes all childs of the container")
function Container:moveZ(a,to)
	assert(a and to, "wrong arguments")
	local idx = a
	for i,v in ipairs(self.components) do
		if v == a then idx = i break end
	end
	if type(idx)~="number" then return end
	table.remove(self.components,idx)
	table.insert(self.components,to,a)
	for i,v in ipairs(self.components) do
		v.basel2d:sortid(#self.components-i+2)
	end
end

LuxModule.registerclassfunction("removeChildren", "():(Container) - removes all children of the container")
function Container:removeChildren()
	while #self.components>0 do
		self.components[#self.components]:remove()
	end
end
Container.removeChilds = Container.removeChildren -- compatibility fix, should throw a warning


LuxModule.registerclassfunction("isChildOf",
	[[(boolean):(Container, parent) - returns true if a parent
	of the ancestors is equal to parent]])
function Container:isChildOf (container)
	if (self.parent == nil) then return false end
	return self.parent:isChildOf(container)
end


LuxModule.registerclassfunction("add",
	[[(Component):(Container, Component,[int index]) - adds the component to the list of components.
	If Index is passed, it will be inserted at a certain z-position. The lower indexes are
	on the foreground, higher numbers are on the background. Returns the added Component]])
function Container:add (component,i)
	assert(type(component)=="table", "Component as arg 1 required")
	assert(i==nil or type(i)=="number","index must be number if given")
	assert(component.basel2d,"no basel2d, no gui element")
	if (component.parent~=nil) then
		component:remove()
	end
	if (self:isChildOf(component)) then
		error("adding this component would result in cycle")
	end
	i = i or table.getn(self.components)+1
	table.insert(self.components,i,component)

	for i,v in ipairs(self.components) do
		v.index = i
		v.basel2d:sortid(#self.components-i+2)
	end
	component.parent = self
	component:setDisplayable(self.isDisplayedFlag)
	component.basel2d:parent(self.basel2d)

	self:invalidate()
	component:invalidateAll()
	return component
end

function Container:invalidateAll()
	for i,v in ipairs(self.components) do
		v:setDisplayable(self.isDisplayedFlag)
		v:invalidateAll()
	end
end


LuxModule.registerclassfunction("removeComponent",
	[[():(Container, Component) - removes the component from the container]])
function Container:removeComponent (component)
	if (component.parent==self) then
		--error("the component is not a child of the container")
		return component:remove()
	end

	component.parent = nil
	for i,v in ipairs(self.components) do
		if (v == component) then
			table.remove(self.components,i)
		end
		v.index = i
	end
	component.basel2d:parent(l2dlist.getroot())
	component:remove()
	self:invalidate()
end


LuxModule.registerclassfunction("getRootContainer",
	[[(Container):() - returns the rootcontainer which is the main frame.]])
function Container.getRootContainer ()
	return rootContainer
end


LuxModule.registerclassfunction("getComponentAt",
	[[([Component]):(Container self, float x,y) - returns container at the
	specific position. Returns self if no child is found that matches and
	if the point is contained in the component.]])
function Container:getComponentAt (x,y)
	local wx,wy = self:local2world(x,y)
	if (not self.bounds:contains(x+self.bounds[1],y+self.bounds[2])) then
--	_print(debug.traceback())
		return nil
	end
--	_print(x,y)
--_print(self)
--	x, y = x - self.bounds[1], y - self.bounds[2]
	for i = 1,table.getn(self.components) do
		local comp = self.components[i]
		local rx,ry = x-comp.bounds[1],y-comp.bounds[2]
		--if (getmetatable(comp)==TextField) then

		--end
		if (comp:isVisible() and comp.bounds:contains(x,y)) then
			if (comp.getComponentAt) then
				local ret = comp:getComponentAt(rx,ry)
				if (ret) then return ret end
			end
			if (comp:contains(rx,ry)) then
				return comp end
		end
	end
	if (self:contains(x,y)) then return self end
end


LuxModule.registerclassfunction("invalidate","@overloaded Component")
function Container:invalidate (...)
	Component.invalidate(self,...)
	--print(self)
	for a,b in ipairs(self.components or {}) do
		b:invalidate()
	end
end

LuxModule.registerclassfunction("validate",
	[[():(Container self) - validates the container and all its childs.
	Is called once per frame for containers that are visible,
	don't call it unless you really need it.]]) --']]
function Container:validate (z,clip)
	--print("validate ",self,"?")
	--if (not self.dirt and z == self.zsortid) then return self.zsortidend end
	--print("comp validate?",clip)
	--print("bound: ",self.bounds[3],self.bounds[4])
	local bounds = type(self.dirt)=="table" and self.dirt or self.bounds
	local wx,wy = self:local2world(0,0)
	--print("validate ",self,"!")
	clip = Rectangle.intersection(
		Rectangle:new (
			self.clipinsets[4]+wx,
			self.clipinsets[1]+wy,
			bounds[3]-self.clipinsets[2],
			bounds[4]-self.clipinsets[3]),
		clip)
	self.basel2d:scissorsize(bounds[3],bounds[4])
	z = Component.validate(self,z,clip)
	local n = table.getn(self.components)
	for i = n,1,-1 do
		v = self.components[i]
		--if (v:isVisible()) then
			if (self.dirt~=true and self.dirt) then
				v.dirt = v.dirt or true
			end
			assert(v.bounds)
			z = v:validate(z,clip) or z
		--end
	end
	self.zsortidend = z
--	_print(self.zsortid,self.zsortidend)
	return z
end


LuxModule.registerclassfunction("mouseEvent",
	[[():(Container self, MouseEvent event, boolean exit) - processes the mouseevent and
	delegates it to it's children and mouselisteners. If exit is true, the mouse is now
	on another component]])
function Container:mouseEvent (me)
	local comp = self:isMouseLocker() and self or self:getComponentAt(me.x,me.y)
	if comp and comp~=self then
		local me = me:set(comp:world2local(self:local2world(me.x,me.y)))
		comp:mouseEvent(me)
	end
	if comp == self then
		Component.mouseEvent(self,me)
	end
end

LuxModule.registerclassfunction("onDisplayChange",
	[[():(Container self, boolean isDisplayed) - called whenever the
	displaystate is changed, i.e. if the component was made invisible,
	or it's parent was removed from the rootpane etc.

	A component that is displayed has not to be visible.]])
function Container:onDisplayChange (isDisplayed)
	Component.onDisplayChange(self,isDisplayed)
	for i,v in ipairs(self.components) do
		v:onDisplayChange(isDisplayed)
	end
end

LuxModule.registerclassfunction("onVisibilityChange",
	[[():(Container self, boolean isVisible) - called whenever
	the container is made visible / invisible. Calls onVisibilityChange
	of all child components. ]])
function Container:onVisibilityChange (isVisible)
	isVisible = isVisible and self:isVisible()
	for i,v in ipairs(self.components) do
		--v:setVisibility(isVisible)
		v:onVisibilityChange(isVisible)
	end
	Component.onVisibilityChange(self,isVisible)
end


LuxModule.registerclassfunction("toString",
	[[():(Container self) - returns a simple string representation of self]])
function Container:toString ()
	return "[Container "..tostring(self.bounds).."]"
end


LuxModule.registerclassfunction("isRootContainer",
	"(boolean):(Container) - returns true if the container is the rootcontainer")
function Container:isRootContainer ()
	return self == rootContainer
end


function Container:think ()
	for a,b in ipairs(self.components) do
		b:think()
	end
end

LuxModule.registerclassfunction("doLayout",
	"():(Container) - Called if the size of the container was changed. Can be overloaded (does nothing per default) to change bounds of child elements.")
function Container:doLayout()

end

LuxModule.registerclassfunction("setBounds",
	"():(Container,x,y,w,h) - Sets the bounds of the container and calls 'doLayout'.")
function Container:setBounds(x,y,w,h)
	local layout = self:getWidth()~=w or self:getHeight()~=h or self:getX()~=x or self:getY()~=y
	Component.setBounds(self,x,y,w,h)
	if layout then
		self:doLayout()
	end
end

function Container:countElements()
	local i = 1
	for a,b in ipairs(self.components) do
		if (b.countElements) then
			i = i + b:countElements()
		else
			i = i + 1
		end
	end
	return i
end
--------------------------------------------------------------------------------


local lastmove = 0
local tooltipfired,tooltip
local keybinddisabled

local function think ()
--_print("frame---------------------")
	if (console.active()) then return end

	rootContainer:setBounds(0,0,window.refsize())

	--rootContainer.bounds[3],rootContainer.bounds[4] = window.refsize()
	--foregroundContainer.bounds[3],foregroundContainer.bounds[4] = window.refsize()
	--foregroundContainer:think()
	--rootContainer:think() -- maybe too consuming and makes no sense at all

--	Component.validateAll(Component.zorderoffset,rootContainer:getClipRect())
	Component.validateFocus()

	local foc = Component.getFocusElement()
	if foc and foc.disablekeybinds then
		Keyboard.enableKeyBinds(false)
		keybinddisabled = true
	elseif keybinddisabled then
		Keyboard.enableKeyBinds(true)
		keybinddisabled = false
	end

	if os.clock()-lastmove>.8 and not tooltipfired then
		tooltipfired = true
		local cmp = rootContainer:getComponentAt(MouseCursor.pos())
		if cmp and cmp~=rootContainer then
			cmp:showTooltipInformation()
		end
	end
end

local lastmouseevent

local function mouselisten (self, me)
	lastmouseevent = me
	if (console.active()) then return end

	local lock = Component.getMouseLock()
	if (lock) then
		local x,y = lock:local2world(0,0)
		me = me:move(-x,-y)
--print(me)
		lock:mouseEvent(me,false)
	else
		if tooltip then tooltip:delete() tooltip = nil end
		if not me:isPressed() then
			lastmove = os.clock()
			tooltipfired = false
		end

		local consumed
		if not consumed then
			local con = rootContainer:getComponentAt(me.x,me.y)

			if con then

				con:mouseEvent(me:set(con:world2local(me.x,me.y)),false)
			end
		end
	end
end

Timer.set("_gui_component", think)
MouseCursor.addMouseListener(MouseListener.new(mouselisten))

--------------------------------------------------------------------------------

rootContainer = Container:new(0,0,0,0)
rootContainer:setDisplayable(true)
rootContainer:setVisibility(true)
-- somewhat of a hack
--_print("lua: ",l2dlist.getroot(),rootContainer.basel2d)
rootContainer.basel2d:parent(l2dlist.getroot())

function rootContainer:getLastMouseEvent()
	return lastmouseevent
end

rootContainer:addTooltipListener(
	function (event)
		if event:isConsumed() then return end
		local info = event:getSource():getTooltip()
		if not info then return end
		event:consume()

		local label = type(info)=="string" and Label:new(10,8,0,0,info) or info


		if type(info)=="string" then
			label:setSize(label:getMinSize())
		end

		local w,h  = label:getSize()
		label:setLocation(10,8)
		w,h = w + 20, h + 16
		local x,y = event:getX()+8,event:getY()
		local sw,sh = window.refsize()
		x,y = math.min(sw,x+w)-w,math.min(sh,y+h)-h

		tooltip = rootContainer:add(GroupFrame:new(x,y,w,h),1)
		tooltip:add(label)
		--tooltip:moveToRect(event:getX()+8,event:getY(),w+20,h+16,.25,20)
		tooltip:setColorAll({1,1,0,0},{0,0,0,0})
		tooltip:fadeTo({1,1,0,1},{0,0,0,1},nil,.2,20)


	end
)


function rootContainer:validate(...)
    rootContainer.bounds[3],rootContainer.bounds[4] = window.refsize()
  --Component.newFocusOrder({})
	local w,h = window.refsize()
	local rect = Rectangle:new (0,0,w,h)
	Container.validate(self,Component.zorderoffset,Rectangle:new (0,0,w,h))
	--Component.validateFocus()
	self.dirt = nil
end

function rootContainer:getClipRect()
	rootContainer.bounds[3],rootContainer.bounds[4] = window.refsize()
	local w,h = window.refsize()
	local rect = Rectangle:new (0,0,w,h)
	return rect
end

function rootContainer:invalidate(...)
    --print("invalidation ...")
	Timer.finally("gui validation", function () rootContainer:validate() end)
end

function rootContainer:toString()
    return "[Rootcontainer]"
end

