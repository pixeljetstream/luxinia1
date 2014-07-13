CheckBox = {
	skinnameschecked = {
		pressedskin = "checkedcheckbox_pressed",
		defaultskin = "checkedcheckbox",
		hoveredskin = "checkedcheckbox_hovered",
		pressedhoveredskin = "checkedcheckbox_hovered_pressed",
		focusedskin = "checkedcheckbox_focused",
		focusedhoveredskin = "checkedcheckbox_focused_hovered",
		focusedpressedskin = "checkedcheckbox_focused_pressed",
		focusedhoveredpressedskin = "checkedcheckbox_focused_hovered_pressed",
	},
	skinnamesunchecked = {
		pressedskin = "checkbox_pressed",
		defaultskin = "checkbox",
		hoveredskin = "checkbox_hovered",
		pressedhoveredskin = "checkbox_hovered_pressed",
		focusedskin = "checkbox_focused",
		focusedhoveredskin = "checkbox_focused_hovered",
		focusedpressedskin = "checkbox_focused_pressed",
		focusedhoveredpressedskin = "checkbox_focused_hovered_pressed",
	}
}

setmetatable(CheckBox,{__index = Button})

LuxModule.registerclass("gui","CheckBox",
	[[A CheckBox component. Basicly it is a slightly modified Button and
	works in the same ways like the Button. The checkbox class is using the icon
	to display the checkbox next to the text.

	You can create and use a checkbox in this way:

	 ck = CheckBox:new(10,90,200,24,"Check me out!")
	 function ck:onClicked(state)
	   if (state) then print("I am checked")
	   else print("I am unchecked") end
	 end
	 Container.getRootContainer():add(ck)

	The following skinssurface names are used by the checkbox class:

	* checkedcheckbox_pressed
	* checkedcheckbox
	* checkedcheckbox_hovered
	* checkedcheckbox_hovered_pressed
	* checkedcheckbox_focused
	* checkedcheckbox_focused_hovered
	* checkedcheckbox_focused_pressed
	* checkedcheckbox_focused_hovered_pressed

	* checkbox_pressed
	* checkbox
	* checkbox_hovered
	* checkbox_hovered_pressed
	* checkbox_focused
	* checkbox_focused_hovered
	* checkbox_focused_pressed
	* checkbox_focused_hovered_pressed
	]],
	CheckBox,
	{
		skinnameschecked = "{[table]} - skinnames in case the element is checked",
		skinnamesunchecked = "{[table]} - skinnames in case the element is not checked",
	},"Button")

LuxModule.registerclassfunction("new",
	[[(CheckBox):(class, float x,y,w,h, string caption, [Skin2D skin,Icon] ]) -
	Creates a checkbox at given location and with given size. The caption
	is automaticly set to be aligned left on the skin and vertically centered.
	It also activates the autowidth parameter of the skin and makes the
	button it was derived from pushable.
	]])
function CheckBox.new (class, x,y,w,h, caption, skin, icon)
	local self = Button.new(class,x,y,w,h, caption)

	self:setPushable(true)
	self:setSkin(skin or "default")
	skin = self:getSkin()
	local checkIcon = ImageIcon:new(skin.texture,16,16,blendmode.decal(),
		{
			default = {160,96,16,16},
			checkbox_pressed = {160,96,16,16},
			checkbox = {192,112,16,16},
			checkbox_hovered = {224,112,16,16},
			checkbox_hovered_pressed = {160,96,16,16},
			checkbox_focused = {208,112,16,16},
			checkbox_focused_hovered = {208,112,16,16},
			checkbox_focused_pressed = {176,112,16,16},
			checkbox_focused_hovered_pressed = {176,112,16,16},

			checkedcheckbox_pressed = {224,112,16,16},
			checkedcheckbox = {176,96,16,16},
			checkedcheckbox_hovered = {160,96,16,16},
			checkedcheckbox_hovered_pressed = {224,112,16,16},
			checkedcheckbox_focused = {176,112,16,16},
			checkedcheckbox_focused_hovered = {176,112,16,16},
			checkedcheckbox_focused_pressed = {208,112,16,16},
			checkedcheckbox_focused_hovered_pressed = {208,112,16,16},
		}
	)
	skin:setIcon(checkIcon)
	skin:setLabelPosition(Skin2D.ALIGN.LEFT,Skin2D.ALIGN.CENTER)
	skin:setAutowidth(true)
	self.skinnames = self.skinnamesunchecked

	return self
end

function CheckBox:clicked ()
	if self:isPushed() then
		self.skinnames = self.skinnameschecked
	else
		self.skinnames = self.skinnamesunchecked
	end
	Button.clicked(self)
end


function CheckBox:setPushState(state)
	Button.setPushState(self,state)
	if self:isPushed() then
		self.skinnames = self.skinnameschecked
	else
		self.skinnames = self.skinnamesunchecked
	end
end

function CheckBox:updateSkin()
	if not self.skin then return end
	local p,m,f = (self.dataButton.pressed) and 1 or 0,
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


	local wx,wy = self:local2world(0,0)
	self.skin:setBounds(0,0,self.bounds[3],self.bounds[4])
	self.skin:selectSkin(skins[sel] or "checkbox")

	self.skin:setLabelText(self.caption)
end
