Slider = {
	skinnamesvertical = {
		pressedskin = "sliderv_pressed",
		hoveredskin = "sliderv_hovered",
		defaultskin = "sliderv",
		focusedskin = "sliderv_focused",
	},
	skinnameshorizontal = {
		pressedskin = "sliderh_pressed",
		hoveredskin = "sliderh_hovered",
		defaultskin = "sliderh",
		focusedskin = "sliderh_focused",
	},

	sliderincrement = 0.1,
}

setmetatable(Slider,{__index = Component})

LuxModule.registerclass("gui","Slider",
	[[The slider is a handle than can be moved within some limits.

	You can create and use a slider in this way:

	 slider = Slider:new(100,20,100,20)
	 function slider:onValueChanged (newvalue,oldvalue)
	   print(newvalue,oldvalue)
	 end
	 Container.getRootContainer():add(slider)

	The following skinssurface names are used by the slider class:
	* Vertical slider skin surfaces
	** sliderv
	** sliderv_pressed
	** sliderv_hovered
	** sliderv_focused
	* Horizontal slider skin surfaces
	** sliderh
	** sliderh_pressed
	** sliderh_hovered
	** sliderh_focused

	]],
	Slider,
	{
		sliderincrement = [[{[float]}=0.1 - increment of sliderposition. Can be set per
			slider. The increment value is used if the mousewheel or the arrow keys have been used.]]
	},"Component")

LuxModule.registerclassfunction("new",
	[[(Slider):(class, int x,y,w,h, [boolean vertical, [Skin2D, [Icon] ] ]) -
	Creates Slider with given bounds. If vertical is true the slider
	is vertical aligned.]])
function Slider.new(class, x,y,w,h,vert,skin,icon)
	local self = Component.new(class,x,y,w,h)

	self:addKeyListener(
		KeyListener.new(function (kl,keyevent) self:onKeyTyped(keyevent) end, true)
	)

	self.skinnames = vert and self.skinnamesvertical or self.skinnameshorizontal
	self.vertical = vert

	self:setSkin(skin or "default")
	local skin = self:getSkin()

	icon = icon or ImageIcon:new(skin.texture,16,16,blendmode.decal(),
		{
			default = {64,80,16,16},

			sliderh = {64,80,16,16},
			sliderh_hovered = {80,80,16,16},
			sliderh_focused = {112,80,16,16},
			sliderh_pressed = {112,80,16,16},

			sliderv = {64+64,80,16,16},
			sliderv_hovered = {80+64,80,16,16},
			sliderv_focused = {112+64,80,16,16},
			sliderv_pressed = {112+64,80,16,16},
		}
	)

	skin:setIcon(icon)
	skin:setIconAlignment(10,Skin2D.ALIGN.CENTER)

	self:setSliderPos(0.5)
	return self
end

LuxModule.registerclassfunction("onValueChanged",
	[[():(Slider, float newvalue,prevvalue) - called if the slider position was changed.
	The values are always between 0 and 1. You can overload this function without
	calling the original overloaded function. This function is only called if
	the previous value is distinct from the new value.]])
function Slider:onValueChanged(newvalue,prevvalue) end


function Slider:setBounds(...)
	Component.setBounds(self,...)
	if (not self.vertical) then
		local iw = self:getSkin().icon:getWidth()
		local w = self:getWidth()

		local x = self.sliderpos*(w-iw)
		self:getSkin():setIconAlignment(x,Skin2D.ALIGN.CENTER)
	else
		local ih = self:getSkin().icon:getHeight()
		local h = self:getHeight()

		local y = self.sliderpos*(h-ih)
		self:getSkin():setIconAlignment(Skin2D.ALIGN.CENTER,y)
	end
end

LuxModule.registerclassfunction("setSliderPos",
	[[():(Slider, pos, noevent,force) - set the position of the slider, calls automaticly the
	onValueChanged method. The value is automaticly clamped to a value
	between 0 and 1. If noevent is true, no valuechanged event is sent]])
function Slider:setSliderPos(pos,noevent,force)
	local prev = self.sliderpos
	self.sliderpos = math.min(1,math.max(0,pos))
	if (self.integermode) then
		self.sliderpos = math.floor(self.sliderpos/self.sliderincrement+0.5)*self.sliderincrement
	end
	if (not self.vertical) then
		local iw = self:getSkin().icon:getWidth()
		local w = self:getWidth()

		local x = self.sliderpos*(w-iw)
		self:getSkin():setIconAlignment(x,Skin2D.ALIGN.CENTER)
	else
		local ih = self:getSkin().icon:getHeight()
		local h = self:getHeight()

		local y = self.sliderpos*(h-ih)
		self:getSkin():setIconAlignment(Skin2D.ALIGN.CENTER,y)
	end
	if (not noevent and (prev~=self.sliderpos or force)) then
		self:onValueChanged(self.sliderpos,prev)
	end
end

LuxModule.registerclassfunction("getSliderPos",
	[[(float pos):(Slider) - get the position of the slider 0-1.]])
function Slider:getSliderPos()
	return self.sliderpos
end

function Slider:isFocusable() return self:isVisible() end

LuxModule.registerclassfunction("setSliderTo",
	[[():(Slider,x,y) - sets the slider position nearest to a pixelposition
	given by x and y that are in local coordinates of the component.]])
function Slider:setSliderTo(x,y)
	if (not self.vertical) then
		local iw = self:getSkin().icon:getWidth()
		local w = self:getWidth()
		local x = x-iw/2
		x = math.min(math.max(0,x/(w-iw)),1)
		self:setSliderPos(x)
	else
		local ih = self:getSkin().icon:getHeight()
		local h = self:getHeight()
		local y = y-ih/2
		y = math.min(math.max(0,y/(h-ih)),1)
		self:setSliderPos(y)
	end
end

LuxModule.registerclassfunction("mouseWheeled",
	[[():(Slider,MouseEvent) - This function sets the sliderposition if the
	mousewheel was turned.]])
function Slider:mouseWheeled(me)
	self:setSliderPos(self.sliderpos-me.wheelmove*self.sliderincrement)
end

function Slider:transferFocusOnArrows() return false end

LuxModule.registerclassfunction("onKeyTyped",
	[[():(Slider,KeyEvent) - Attached to Slider with a keylistener.
	Will react on arrow up/down/left/right keypresses and moves the slider.]])
function Slider:onKeyTyped(keyevent)
	local c = keyevent.keycode
	if (c == Keyboard.keycode.LEFT or c== Keyboard.keycode.UP) then
		self:setSliderPos(self.sliderpos - self.sliderincrement)
	end
	if (c == Keyboard.keycode.RIGHT or c == Keyboard.keycode.DOWN) then
		self:setSliderPos(self.sliderpos + self.sliderincrement)
	end
end

LuxModule.registerclassfunction("setIntegerMode",
	[[(),(Slider, boolean on) - switches IntegerMode on and of. In integermode,
	the values that are set are being rounded to even multiplies of the
	sliderincrement value.]])
function Slider:setIntegerMode ( on )
	self.integermode = on
	self:setSliderPos(self.sliderpos)
end

LuxModule.registerclassfunction("setIncrement",
	[[(),(Slider, float inc) - Sets sliderincrement of slider. You could
	set the value directly by assigning a new value to the sliderincrement
	value, but you shouldn't chose values <=0 or >1 and you should update
	the sliderposition to its new value in case that the integermode is on
	and the value is hereby changed.
	]])
function Slider:setIncrement (inc)
	assert(inc>0 and inc<=1, "increment must be >0 and <=1")
	self.sliderincrement = inc
	self:setSliderPos(self.sliderpos)
end

function Slider:mousePressed(me)
	self:lockMouse()
	Component.mousePressed(self,me)
	self:setSliderTo(me.x,me.y)
	if self:isFocusable() then self:focus() end
end

function Slider:mouseReleased(me)
	self:unlockMouse()
	Component.mouseReleased(self,me)
end

function Slider:mouseMoved(me)
	Component.mouseMoved(self,me)
	if (me:isDragged() and self:getMouseLock() == self) then
		self:setSliderTo(me.x,me.y)
	end
end

function Slider:toString()
	return ("[Slider %.2f %s]"):format(self.sliderpos or 0,tostring(self.bounds))
end