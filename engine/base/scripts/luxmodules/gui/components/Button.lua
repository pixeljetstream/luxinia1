-- preload default texture into core

Button = {
	skinnames = {
		defaultskin = "button",
		hoveredskin = "button_hover",
		pressedskin = "button_pressed",
		pressedhoveredskin = "button_hover_pressed",
		focusedskin = "button_focus",
		focusedhoveredskin = "button_focus_hover",
		focusedpressedskin = "button_focus_pressed",
		focusedhoveredpressedskin = "button_focus_hover_pressed",
	}

}
setmetatable(Button,{__index = Component})
LuxModule.registerclass("gui","Button",
	[[A Button is a Component that reacts on mouseclicks and other mouseactions.

	Creating a simple button that reacts on mouseclicks is as easy as this:
	 btn = Button:new(10,10,80,30,"Hello") -- create button
	 function btn:onClicked()  -- function that is called on click events
	   print("Hello") -- do whatever you like
	 end
	   -- put the button now on the root container
	 Container.getRootContainer():add(btn)
	   -- otherwise it won't be visible]],
	Button,
	{
		["dataButton"] = "{[table]} - the current state info of the button.",
		["dataButton.pressed"] = "{[boolean]} - true if the button is hold down",
		["dataButton.mouseover"] = "{[boolean]} - true if the mouse is over the button",
		["dataButton.pressedSince"] =
			[[{[int]} - if button is pressed, this value stores the frame when the
			mousebutton was pressed]],
		["dataButton.pressedMouseButton"] =
			"{[int]} - the mousebuttonid that holds the button down",
		["dataButton.isFocusable"] =
			"{[boolean=true]} - can gain focus if true",
		["actionListeners"] = "{[table]} - a list of all actionlisteners",
	},"Component")

LuxModule.registerclassfunction("new",
	[[(Button):(table class, float x,y,w,h, string caption, [Skin2D skin]) -
	creates a new Button object.

	The Skin should have 8 states:
	* button
	* button_hover
	* button_pressed
	* button_hover_pressed
	* button_focus
	* button_focus_hover
	* button_focus_pressed
	* button_focus_hover_pressed

	If no skin is passed the default skin is cloned.
	]])
function Button.new (class, x,y,w,h, caption, skin)
	local self = Component.new(class, x,y,w,h)

	self.dataButton =
	{
		pressed = false,
		mouseover = false,
		pressedSince = nil,
		pressedMouseButton = nil,
		isFocusable = true,
		ispushbutton = false,
		pushstate = false,
	}
	self.actionListeners = {}
	self.caption = caption or ""

	self:setSkin(skin or "default",true)

	self.skin:setLabelText(self.caption)

	return self
end

LuxModule.registerclassfunction("isPushButton",
	[[(boolean):(Button) - returns true if the Button is a pushbutton]])
function Button:isPushButton()
	return self.dataButton.ispushbutton and true
end

LuxModule.registerclassfunction("isPushed",
	[[(boolean):(Button) - returns true if the Button is pushed]])
function Button:isPushed()
	return self.dataButton.pushstate and true
end

function Button:getPushState(state)
	return self.dataButton.pushstate
end

LuxModule.registerclassfunction("setPushState",
	[[ ():(Button,boolean) - pass true if the button should be pushed. Setting 
	The pushstate won't affect actionlisteners and click functions.]]) --[[']]
function Button:setPushState(state)
	if (self.dataButton.pushstate~=state) then
		self.dataButton.pushstate = state
		
		self:updateSkin()
		--print("inv?")
	end
end

LuxModule.registerclassfunction("setText",
	[[():() - sets text of button]])
function Button:setText(tx)
	self.caption = tx or ""
	self:getSkin():setLabelText(self.caption)
	self:invalidate()
	self:updateSkin()
end

LuxModule.registerclassfunction("setPushable",
	[[():(Button, pushable, [state]) - Makes the button pushable. A pushbutton switches
	between the normal and pressed state. You can use a third optional parameter
	to set the state to an initial value.]])
function Button:setPushable (val,value)
	self.dataButton.ispushbutton = val and true
	if (value~=nil) then self:setPushState(value) end
	self:invalidate()
end

LuxModule.registerclassfunction("addActionListener",
	[[():(function action,[string/any description]) -
	Adds a function that is called when the button is clicked. The
	description is optionial. The action function
	signature:
	 function actionfunction (buttonobject,datatable)
	the datatable contains at index 1 the function and at index 2
	the description. You can change the datatable in any way you want -
	however, the function at index 1 will be the function that is being
	called if an action occurs.]])
function Button:addActionListener (action,description)
	table.insert(self.actionListeners,{action,description})
end

function Button:getMinSize()
	return self:getPreferredSize()
end

function Button:getPreferredSize()
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)

	local currentl2d = l2dtext.new("button line",self.caption or "",arialset)
	local pw,ph = currentl2d:dimensions()
	currentl2d:delete()

	if self:getSkin() then
		pw,ph = self:getSkin():getLabelSize()
		
		if self:getSkin():getIcon() then
			local icon = self:getSkin():getIcon()
			local skin = self:getSkin()
			local m = self:getSkin():getIconMargin()
			local iw,ih = self:getSkin():getIcon():getSize()
			pw,ph = math.max(iw,pw),math.max(ih,ph)
			if skin.iconalignhorizontal == Skin2D.ALIGN.ICONTOLEFT or skin.iconalignhorizontal == Skin2D.ALIGN.ICONTORIGHT then
				pw = pw + m + iw
			end
			if skin.iconalignvertical == Skin2D.ALIGN.ICONTOTOP or skin.iconalignvertical == Skin2D.ALIGN.ICONTOBOTTOM then
				ph = ph + m + ih
			end

		end
		local a,b,c,d = self:getSkin():getPadding()
		pw,ph = pw + b + d, ph + (a + c)/2
	end

	return pw,ph
end

LuxModule.registerclassfunction("removeActionListener",
	[[([function,description]):(function/any key) - removes the actionlistener from the
	object where the key can be the function or the description itself.]])
function Button:removeActionListener (action)
	if (action == nil) then return end
	for i,v in pairs(self.actionListeners) do
		if (v[1] == action or v[2]==action) then
			return unpack(table.remove(self.actionListeners,i) or {})
		end
	end
end

LuxModule.registerclassfunction("clicked",
	[[():(Button self,wasmouse,wasdoubleclicked) - called if the button was actually clicked. Is called
	if the mouse is clicked, but can be called by other sources, too. Calls
	all actionlistener functions. This method calls the actionlisteners.
	Overload onClicked instead for simple use.]])
function Button:clicked(wasmouse,wasdoubleclicked)
	if (self:isDisabled()) then return end
	if (self:isPushButton()) then
		self:setPushState(not self:getPushState())
	end
	for i,v in pairs(self.actionListeners) do
		v[1](self,v)
	end
	local co = coroutine.create(self.onClicked)
	while coroutine.status(co)~="dead" do
		local noerr,msg = coroutine.resume(co,self,self:isPushed(),wasmouse,wasdoubleclicked)
		if not noerr then
			print "Error"
			print (msg)
			print (("-"):rep(70))
			print (debug.traceback(co))
		elseif coroutine.status(co)~="dead" then
			coroutine.yield()
		end
	end
--[=[	xpcall (
		function ()
			self:onClicked(self:isPushed())
		end,
		function (err)
			print("Error calling button function 'onClicked':")
			print(err)
			print("------------")
			print(debug.traceback())
		end
	)]=]
	self:updateSkin()
end

LuxModule.registerclassfunction("onClicked",
	[[():(Button self, boolean state, wasmouse,wasdoubleclick) - simple callback function that can be overloaded without
	calling the superclass etc. Is called after all ActionListeners has been
	processed. The state boolean flag is true if the button is currently pushed,
	which matters only for pushable buttons - otherwise it is always false.]])
function Button:onClicked(state,wasmouse,wasdoubleclick) end

LuxModule.registerclassfunction("onAction",
	[[():(Button self) - calls directly clicked (invoked by actioncommands such
	as ENTER on focused buttons]])
function Button:onAction()
	self:clicked(false,false)
end

LuxModule.registerclassfunction("mouseMoved",
	[[():(Button self, MouseEvent e) - called if the mouse actually moved over the button]])
function Button:mouseMoved(me)
	Component.mouseMoved(self,me)
end

LuxModule.registerclassfunction("mousePressed",
	[[():(Button self, MouseEvent e) - called if the mouse was pressed on the button]])
function Button:mousePressed(me)
	Component.mousePressed(self,me)
	self:lockMouse()
	self.dataButton.pressed = true
	self.dataButton.mouseover = true
	self.dataButton.pressedSince = me.downsince
	self.dataButton.pressedMouseButton = me.button
	self:updateSkin()
end

LuxModule.registerclassfunction("setDoubleClickTriggerOnly",
	[[():(Button self, boolean on) - If set to true, the button reacts only on double clicks]])
function Button:setDoubleClickTriggerOnly(on)
	self.doubleclicktrigger = on
end

LuxModule.registerclassfunction("getDoubleClickTriggerOnly",
	[[(boolean):(Button self) - returns true if the button is reacting only on double clicks]])
function Button:getDoubleClickTriggerOnly()
	return self.doubleclicktrigger
end

LuxModule.registerclassfunction("mouseReleased",
	[[():(Button self, MouseEvent e) - called if the mouse was released on the button]])
function Button:mouseReleased(me)
	Component.mouseReleased(self,me)
	if Component.getMouseLock()~=self then return end
	self:unlockMouse()
	self.dataButton.pressed = false
	self.dataButton.pressedSince = nil
	self.dataButton.pressedMouseButton = nil
	local x,y = self:local2world(0,0)
	--if (self:contains(me.x-x,me.y-y)) then
	--print(Container.getRootContainer():getComponentAt(me.x+x,me.y+y),me.x+x,me.y+y)
	if (Container.getRootContainer():getComponentAt(me.x+x,me.y+y)==self) then
		--if (self:isPushButton()) then
		--	self.dataButton.pushstate = not self.dataButton.pushstate
		--end
		self:focus()
		if not self.doubleclicktrigger or me:isDoubleClick() then
			self:clicked(true,me:isDoubleClick())
		end
	else
		self:updateSkin()
	end
end

LuxModule.registerclassfunction("mouseEntered",
	"():(Button self) - sets mouseinside to true")
function Button:mouseEntered()
	Component.mouseEntered(self,me)
	self.dataButton.mouseover = true
	self:updateSkin()
end

LuxModule.registerclassfunction("mouseExited",
	"():(Button self) - sets mouseinside and mousedown to false")
function Button:mouseExited()
	Component.mouseExited(self,me)
	self.dataButton.mouseover = false
	self:updateSkin()
end

LuxModule.registerclassfunction("setVisibility","@overloaded Component")
function Button:setVisibility(visible)
	Component.setVisibility(self, visible)
	self.dataButton.pressed = false
	self.dataButton.mouseover = false
	self.dataButton.pressedSince = nil
	self.dataButton.pressedMouseButton = nil
end

LuxModule.registerclassfunction("setDisplayable","@overloaded Component")
function Button:setDisplayable(disp)
	Component.setDisplayable(self, disp)
	self.dataButton.pressed = false
	self.dataButton.mouseover = false
	self.dataButton.pressedSince = nil
	self.dataButton.pressedMouseButton = nil
end

LuxModule.registerclassfunction("isFocusable","@overloaded Component")
function Button:isFocusable()
	return self.dataButton.isFocusable and self:isVisible()
end

function Button:onLostFocus()
	self:updateSkin()
end

function Button:onGainedFocus()
	self:updateSkin()
end


LuxModule.registerclassfunction("toString",
	"():(Button self) - returns simple string representation of the button")
function Button:toString()
	return string.format("[Button caption=%s]",self.caption or "")
end



LuxModule.registerclassfunction("deleteVisibles",
	[[():(Button self) - called when a Component is no longer displayed]])
function Button:deleteVisibles ()
	Component.deleteVisibles(self)
	for a,b in pairs(self.l2d or {}) do b:delete() end
end

--LuxModule.registerclassfunction("showVisibles",
--	[[():(Component self) - called when visible objects (l2ds) are now visible]])
--[=[function Button:showVisibles ()
	self.l2d.text:rfNodraw(false)
	if (self.l2d.custom) then self.l2d.custom:rfNodraw(false) end
	if (self.l2d.icon) then self.l2d.icon:rfNodraw(false) end
end]=]

LuxModule.registerclassfunction("setCustomL2d",
	[[([l2dnode]):(Component self,[l2dnode, float x, y,z]) -
	set a custom l2dnode for own purpose. The previously set custom l2dnode is
	deleted. The new customl2dnode ist also deleted if the button is deleted.
	The new customl2dnode is returned by this function. If no l2dnode is given,
	the old customl2dnode is deleted.]])
function Button:setCustomL2d(l2d,x,y,z)
	if (not self.l2d) then self.l2d = {} end
	local old = self.l2d.custom
	self.l2d.custom = l2d
	if (l2d) then
		self.customx,self.customy,self.customz = x or 0,y or 0,z or 0
		self:invalidate()
	end
	if (old and old~=l2d) then old:delete() end
	return l2d
end

function Button:createIconImage ()
	if (not self.l2d) then self.l2d = {} end
	local icon = assert(self.icon,"no icon set for button")
	if (self.l2d.icon) then self.l2d.icon:delete() end
	local l2d = l2dimage.new("skinned image icon",self.icon.icon)
	l2d.collectforgc = true
	self.l2d.icon = l2d
	if (icon.uvx==0 and icon.uvy==0 and icon.uvw==1 and icon.uvw==1) then
	else
		local x,y,w,h = icon.uvx,icon.uvy,icon.uvw,icon.uvh
		if (self.icon.reluv) then
			x = x / self.icon.reluv[1]
			y = y / self.icon.reluv[2] + 1/self.icon.reluv[2]
			w = w / self.icon.reluv[1]
			h = h / self.icon.reluv[2]
		end
		self.l2d.icon:moPos(x,1-y-h,0)
		self.l2d.icon:moRotaxis(w,0,0, 0,h,0, 0,0,1)
	end

	if (icon.blendmode) then
		self.l2d.icon:rfBlend(true)
		self.l2d.icon:rsBlendmode(icon.blendmode)
	end

	self:invalidate()
end

LuxModule.registerclassfunction("setIcon",
	[[():(texture/string icon, width,height,uvx,uvy,uvw,uvh, [blendmode]) -
	sets icon for the button. This is only a forwarder on the Skin2D.setIcon
	function and will throw an error if no skin is set for this button.
	The default blendmode is decal. If false is passed as blendmode, no
	blend is used.]])
function Button:setIcon(icon,w,h,uvx,uvy,uvw,uvh,bmode)
	assert(self.skin,"skin required for icons")
	local icon = ImageIcon:new(icon,w, h, bmode==nil and blendmode.decal() or nil, {default = {uvx,uvy,uvw,uvh}})
	self.skin:setIconAlignment(Skin2D.ALIGN.CENTER,Skin2D.ALIGN.CENTER)
	self.skin:setIcon(icon)
	--[=[
	if (type(icon) == "string") then
		local comp = rendersystem.texcompression()
		rendersystem.texcompression(false)
		icon = assert(texture.load(icon,false),"Icon Image file not found")
		if (nofilter) then icon:filter(false) end
		rendersystem.texcompression(true)
	end
	local reluv = {icon:dimension()}
	uvx,uvy,uvw,uvh,h = uvx or 0, uvy or 0, uvw or reluv[1], uvh or reluv[2],h or w
	assert(w,"no width given")
	self.icon = {icon=icon, uvx=uvx,uvy=uvy,uvw=uvw,uvh=uvh,w=w,h=h,
		blendmode = blendmode, reluv = reluv}
	self:createIconImage()
	]=]
end

function Button:deleteIcon ()
	if (self.icon) then
		self.icon = nil
		if (self.l2d.icon) then self.l2d.icon:delete() end
	end
end

LuxModule.registerclassfunction("hideVisibles",
	[[():(Button self) - called when visible objects (l2ds) are no longer visible]])
function Button:hideVisibles ()
	if (not self.l2d) then return end
	if (self.l2d.text) then self.l2d.text:rfNodraw(true) end
	if (self.l2d.custom) then self.l2d.custom:rfNodraw(true) end
	if (self.l2d.icon) then self.l2d.icon:rfNodraw(true) end
end

LuxModule.registerclassfunction("positionUpdate",
	"():(Button self) - updates all the position info")
function Button:positionUpdate(z,clip)
	self:updateSkin()
	if (self.skin) then
	end

	if (not self.l2d) then return z end

	z = z or self.zsortid
	assert(self.bounds)

	local ox,oy = self:local2world(0,0)
	local x,y,w,h = 0,0,self.bounds[3],self.bounds[4]
	x,y = x+ox,y+oy

	if (self.l2d.text) then
		local sx,sy = self.l2d.text:scale()
		local tw = (string.len(self.caption)+1)*self.l2d.text:spacing()*sx
		local th = 16*sy
		self.l2d.text:pos(math.floor(x+w/2-tw/2),math.floor(y+h/2-th/2),0)
	end
		-- above: Why math.floor? Because otherwise the font is drawn blury
	if (self.l2d.custom) then
		self.l2d.custom:pos(self.customx + x, self.customy + y, self.customz)
	end
	--self.skin:setSize(w,h)
	--self.skin:setLocation(x,y)

	if (self.l2d.icon) then
		self.l2d.icon:pos(math.floor(x + (w - self.icon.w)/2+0.5), math.floor(y + (h - self.icon.h)/2+0.5),0)
		self.l2d.icon:scale(self.icon.w,self.icon.h,1)
	end

	if (clip == nil) then

		for a,l2d in pairs(self.l2d) do
			l2d:rfNodraw(true)
		end
		return z + 1
	end
	local vis = self:isVisible()
	for a,l2d in pairs(self.l2d) do
		l2d:scissor(true)
		l2d:scissorsize(clip[3],clip[4])
		l2d:scissorstart(clip[1],clip[2])
		l2d:rfNodraw(not vis)
		l2d:sortid(z)
	end

	self:updateSkin()

	return z + 1
end

function Button:getCurrentSkinName ()
	if not self.skin then return end
	local p,m,f = (self.dataButton.pressed or self:isPushed()) and 1 or 0,
					self.dataButton.mouseover and 1 or 0,
					self:hasFocus() and 1 or 0
	local sel = m + p*2 + f*4 + 1

	local skins = {	self.skinnames.defaultskin,
				self.skinnames.hoveredskin,
				self.skinnames.pressedskin,
				self.skinnames.pressedskin,
				self.skinnames.focusedskin,
				self.skinnames.focusedhoveredskin,
				self.skinnames.focusedpressedskin,
				self.skinnames.focusedhoveredpressedskin }
	return skins[sel]
end
