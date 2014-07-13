ComboBox = {
	boxskinnames = {
		defaultskin = "combobox",
		hoveredskin = "combobox_hover",
		pressedskin = "combobox_pressed",
		pressedhoveredskin = "combobox_hover_pressed",
		focusedskin = "combobox_focus",
		focusedhoveredskin = "combobox_focus_hover",
		focusedpressedskin = "combobox_focus_pressed",
		focusedhoveredpressedskin = "combobox_focus_hover_pressed",
	},
	buttonskinnames = {
		defaultskin = 				"comboboxbtn",
		hoveredskin = 				"comboboxbtn_hover",
		pressedskin = 				"comboboxbtn_pressed",
		pressedhoveredskin =	 	"comboboxbtn_hover_pressed",
		focusedskin = 				"comboboxbtn_focus",
		focusedhoveredskin = 		"comboboxbtn_focus_hover",
		focusedpressedskin = 		"comboboxbtn_focus_pressed",
		focusedhoveredpressedskin = "comboboxbtn_focus_hover_pressed",
	},

	listskinnames = {
		defaultskin = 				"comboboxlst",
		hoveredskin = 				"comboboxlst_hover",
		pressedskin = 				"comboboxlst_pressed",
		pressedhoveredskin =	 	"comboboxlst_hover_pressed",
		focusedskin = 				"comboboxlst_focus",
		focusedhoveredskin = 		"comboboxlst_focus_hover",
		focusedpressedskin = 		"comboboxstt_focus_pressed",
		focusedhoveredpressedskin = "comboboxlst_focus_hover_pressed",
	},


	labelskinnames = {
		defaultskin = 				"comboboxlbl",
		hoveredskin = 				"comboboxlbl_hover",
		pressedskin = 				"comboboxlbl_pressed",
		pressedhoveredskin =	 	"comboboxlbl_hover_pressed",
		focusedskin = 				"comboboxlbl_focus",
		focusedhoveredskin = 		"comboboxlbl_focus_hover",
		focusedpressedskin = 		"comboboxsbl_focus_pressed",
		focusedhoveredpressedskin = "comboboxlbl_focus_hover_pressed",
	}
}
setmetatable(ComboBox,{__index = Container})

LuxModule.registerclass("gui","ComboBox",
	[[A ComboBox combines multiple strings that can be selected in a
	small rectangle. The current state is very simple but is capable of mimic the
	basics of a normal ComboBox.]],ComboBox,
	{
	},"Container")

LuxModule.registerclassfunction("new",
	[[(ComboBox):(table class, int x,y,w,h,[Skin2D skin]) - creates a combobox
	at the given coordinates with the given skin.]])

function ComboBox.new (class, x,y,w,h, skin)
	local self = Container.new(class,x,y,w,h)

	self.itemlist = {}
	self.selecteditem = {}
	self.button = 	self:add(Button:new(w-h,0,h,h))
	self.box = 		self:add(Button:new(0,0,w-h,h))

	function self.box:isFocusable() return false end

	self.button.skinnames = self.buttonskinnames
	self.box.skinnames = self.boxskinnames
	self.box:getSkin():selectSkin("combobox")
	self.box:getSkin():setLabelPosition(Skin2D.ALIGN.LEFT,Skin2D.ALIGN.CENTER)
	self.button:getSkin():setIcon(defaulticon16x16set:clone())
	self.button:getSkin():setIconSkinSelection("arrowdownsmall")
	self.button:getSkin():setIconAlignment(Skin2D.ALIGN.CENTER,Skin2D.ALIGN.CENTER)

	local function click ()
		self:onBoxClick()
	end
	self.button.onClicked = click
	self.box.onClicked = click

	return self
end

function ComboBox:delete()
	Container.delete(self)
	self.button:delete()
	self.box:delete()
end

function ComboBox:setBounds(x,y,w,h)
	Container.setBounds(self,x,y,w,h)
	self.button:setBounds(w-h,0,h,h)
	self.box:setBounds(0,0,w-h,h)
end

LuxModule.registerclassfunction("getItems",
	[[(table):(ComboBox) - returns a table with all items (copy).
	Each tableelement is a table containing following keys:
	* caption
	* command
	* icon
	* iconname
	The values are all optional except for the caption.]])
function ComboBox:getItems ()
	local tab = {}
	for i,v in ipairs(self.itemlist) do 
		tab[i] = {caption=v.caption,command=v.command,icon=v.icon,iconname=v.iconname}
	end
	return tab
end

LuxModule.registerclassfunction("setItems",
	[[():(ComboBox,table) - sets (and clears previous) all items. Table must be built like the one
	retrieved by getItems.]])
function ComboBox:setItems (tab)
	local idx = self:getSelected()
	self:clearItems()
	for i,v in ipairs(tab) do
		self:addItem(v.caption,v.command,v.icon,v.iconname)
	end
	self:select(idx or 1)
end

LuxModule.registerclassfunction("addItem",
	[[(function add):(ComboBox, string caption, [string cmd,[icon, iconskinselection] ]) - 
	adds an item to the combobox. It returns a function that can be called again:
	 mycombobox:addItem("Hello")("World") -- adds hello and world 
	 ]])
function ComboBox:addItem (caption,cmd,icon,iconname)
	self.itemlist[#self.itemlist + 1] = {
			caption = caption, command = cmd,	-- was cmd or caption which imo is no good
			icon = icon and icon:clone(), iconname = iconname}
	if #self.itemlist == 1 then
		self:select(1)
	end
	return function (caption,cmd,icon,iconname)
		return self:addItem(caption,cmd,icon,iconname)
	end
end

LuxModule.registerclassfunction("removeItem",
	[[():(ComboBox, int id) - Removes an item from the list. The id refers to the 
	index of the item in the list.]])
function ComboBox:removeItem(id)
	table.remove(self.itemlist,id)
	self:select(self.selecteditem)
end

LuxModule.registerclassfunction("clearItems",
	[[():(ComboBox) - removes all items from the list]])
function ComboBox:clearItems ()
	self.itemlist = {}
	self:select(1)
end

LuxModule.registerclassfunction("getSelected",
	[[([int indx, string caption, command]):(ComboBox) - returns the currently selected item]])
function ComboBox:getSelected ()
	local sel = self.itemlist[self.selecteditem] or {}
	return self.selecteditem, sel.caption, sel.command
end

LuxModule.registerclassfunction("select",
	[[(int selected):(ComboBox, string/int something, [boolean iscommand]) -
	selects the specified item by its name, number or commandstring]])
function ComboBox:select (something,cmd)
	if type (something) == "string" or cmd then
		for i,v in ipairs(self.itemlist) do
			if cmd and v.command == something or
				not cmd and v.caption == something then
				self.selecteditem = i
				break
			end
		end
	else
		assert(type(something)=="number","Number or string expected as arg1")
		self.selecteditem = math.max(math.min(#self.itemlist,something),1)
	end
	--self:getSkin():setLabelText(self.itemlist[self.selecteditem].caption)
	local tx = self.itemlist[self.selecteditem] or {}
	self.box:setText(tx.caption or "")
	if tx.icon then
		self.box:getSkin():setIconAlignment(Skin2D.ALIGN.ICONTOLEFT,Skin2D.ALIGN.CENTER)
		self.box:getSkin():setIcon(tx.icon)
		self.box:getSkin():setIconSkinSelection(tx.iconname)
	end
	self:invalidate()
	self:onSelected(self:getSelected())
	return self.selecteditem
end

LuxModule.registerclassfunction("onSelected",
	[[():(ComboBox,int index, string caption, string command) - this function
	can be overloaded without calling this function. It is called each time the
	user selects an item - which does not have to be different from the previously
	selected item.]])
function ComboBox:onSelected (idx,cap,cmd)
end

LuxModule.registerclassfunction("getSelected",
	[[([int index, string caption, string command]):(ComboBox) -
	returns information on the currently selected item.]])
function ComboBox:getSelected()
	if #self.itemlist>0 then
		local i = self.itemlist[self.selecteditem]
		return self.selecteditem,i.caption,i.command
	end
end

function ComboBox:setFont (font)
	if self.skin then self.skin:setFont(font) end
	self.button:setFont(font)
	self.box:setFont(font)
	self.font = font
	self:invalidate()
end

function ComboBox:onBoxClick (e)
	local root = Container.getRootContainer()
	local order = Component.newFocusOrder({})
	local fullscreen = root:add(
		Container:new(0,0,root:getWidth(),root:getHeight()),1)

	function fullscreen:mouseClicked(e,owner)
		if not owner then return end
		fullscreen:delete()
		Component.newFocusOrder(order)
	end


	local x,y = self:local2world(0,0)
	local choose = Container:new(x,y + self:getHeight()-4,self:getWidth(),self:getHeight())
	fullscreen:add(choose)
	choose.skinnames = self.listskinnames
	choose:setSkin("default")
	local t,r,b,l = choose:getSkin():getPadding()
	local y = t
	for i,v in ipairs(self.itemlist) do
		local btn = Button:new(l,y,choose:getWidth()-r-l,0,v.caption)
		btn:setFont(self.font)
		if v.icon then
			btn:getSkin():setIcon(v.icon:clone())
			btn:getSkin():setIconSkinSelection(v.iconname)
		end
		btn.skinnames = self.labelskinnames
		btn:getSkin():selectSkin(self.labelskinnames.defaultskin)
		local t,r,b,l = btn:getSkin():getPadding()
		local mw,mh = btn:getMinSize()
		local h = mh
		
		btn:setSize(btn:getWidth(),h)

		function btn.onClicked()
			self:select(i)
			fullscreen:delete()
			Component.newFocusOrder(order)
		end
		btn:getSkin():setLabelPosition(Skin2D.ALIGN.LEFT,Skin2D.ALIGN.CENTER)

		choose:add(btn)
		if i == self.selecteditem then btn:focus() end
		y = y + h+1
	end
	choose:setSize(choose:getWidth(),y+b)
	local dist = choose:getY()+y+b
	if (dist > root:getHeight()) then
		dist = dist - root:getHeight()
		choose:setLocation(choose:getX(),choose:getY()-dist)
	end
end

