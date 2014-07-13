ListBox = {
	skinnames = {
		defaultskin = "listbox",
		hoveredskin = "listbox_hover",
		pressedskin = "listbox_pressed",
		pressedhoveredskin = "listbox_hover_pressed",
		focusedskin = "listbox_focus",
		focusedhoveredskin = "listbox_focus_hover",
		focusedpressedskin = "listbox_focus_pressed",
		focusedhoveredpressedskin = "listbox_focus_hover_pressed",
	},

	labelskinnames = {
		defaultskin = 				"listboxlbl",
		hoveredskin = 				"listboxlbl_hover",
		pressedskin = 				"listboxlbl_pressed",
		pressedhoveredskin =	 	"listboxlbl_hover_pressed",
		focusedskin = 				"listboxlbl_focus",
		focusedhoveredskin = 		"listboxlbl_focus_hover",
		focusedpressedskin = 		"listboxsbl_focus_pressed",
		focusedhoveredpressedskin = "listboxlbl_focus_hover_pressed",
		selectedskin = 				"listboxlbl_focus_hover_pressed",
	}
}
setmetatable(ListBox,{__index = Container})

LuxModule.registerclass("gui","ListBox",
	[[A scrollable listbox.]],ListBox,
	{
	},"Container")

LuxModule.registerclassfunction("new",
	[[(ListBox):(table class, int x,y,w,h,[Skin2D skin]) - creates a listbox
	at the given coordinates with the given skin.]])

function ListBox.new (class, x,y,w,h, skin)
	local self = Container.new(class,x,y,w,h)

	self:setSkin(skin or "default")
	self.itemlist = {}
	self.selecteditem = 1

	local t,r,b,l = self:getSkin():getPadding()
	self.scrollbar = self:add(Slider:new(w-18,0,18,h,true))
	self.clipper = self:add(Container:new(t,l,w-l-20,h-t-b))
	self.viewarea = self.clipper:add(Container:new(0,0,self.clipper:getSize()))
	
	function self.scrollbar.onValueChanged(bar,value)
		local t,r,b,l = self:getSkin():getPadding()
		local s = self.clipper:getHeight()
		local h = self.viewarea:getHeight()
		self.viewarea:setLocation(0,math.floor(-value*math.max(0,(h-s))))
	end

	function self.scrollbar.isFocusable() return false end
	
	self:setBounds(x,y,w,h)

	return self
end

function ListBox:delete()
	Container.delete(self)
	self.viewarea:delete()
	self.scrollbar:delete()
end

function ListBox:setBounds(x,y,w,h)
	Container.setBounds(self,x,y,w,h)
	local t,r,b,l = self:getSkin():getPadding()
	self.viewarea:setBounds(0,0,w-l-r,h-t-b)
	self.clipper:setBounds(l,t,w-l-r,h-t-b)
	self.scrollbar:setBounds(w-14,0,14,h)
	self:renewList()
end

LuxModule.registerclassfunction("itemCount",
	[[(n):(table class) - returns the number of items in the list.]])

function ListBox:itemCount()
	return #self.itemlist
end

function ListBox:renewList ()
	local same = true
	local y = 0
	for i,v in ipairs(self.itemlist)do
		local btn = self.viewarea.components[i]
		if not btn or btn.itemobj~=v then
			same = false
			break
		end
		y = y + (btn:getHeight())
		btn:setSize(self.viewarea:getWidth(),btn:getHeight())
	end
	if same then 
		local h = y + 1
		self.viewarea:setSize(self.viewarea:getWidth(),y+1)
		self.scrollbar:setVisibility(h>self:getHeight())
			
		self.scrollbar:setSliderPos(0)
		
		
		return
	end
	self.viewarea:deleteChilds()
	local y = 0
	local buttons = {}
	self.itemscrollinfo = {}
	for i,v in ipairs(self.itemlist) do
		local btn = Button:new(0,y,self.viewarea:getWidth(),22,v.caption)
		btn.itemobj = v
		buttons[i] = btn
		btn.skinnames = self.labelskinnames
		self.viewarea:add(btn)
		btn:setSkin("default")
		btn:getSkin():setLabelPosition(Skin2D.ALIGN.LEFT)
		if v.icon then
			btn:getSkin():setIcon(v.icon:clone())
			if v.iconname then
				btn:getSkin():setIconSkinSelection(v.iconname)
			end
		end
		function btn.getCurrentSkinName()
			if self.selecteditem == i then
				return self.labelskinnames.selectedskin end
			return Button.getCurrentSkinName(btn)
		end

		function btn.onClicked()
			self:select(i)
			self:focus()
		end

		self.itemscrollinfo[i] = function ()
			local x,y = self.viewarea:getLocation()
			local h = self.clipper:getHeight()
			local X,Y = btn:getLocation()
			local H = btn:getHeight()
			if Y+y<0 then
	--			self.viewarea:setLocation(0,-Y)
				local div = (self.viewarea:getHeight()-h)
				self.scrollbar:setSliderPos(Y/div)
			end
			if Y+H+y>h then
				local div = (self.viewarea:getHeight()-h)
				self.scrollbar:setSliderPos(-(-Y+h-H)/div)
--				self.viewarea:setLocation(0,-Y+h-H)
			end
		end

		function btn.isFocusable() return false end


		local t,r,b,l = btn:getSkin():getPadding()
		local h = btn:getSkin():getFontset():linespacing()
		btn:setSize(btn:getWidth(),h + t + b + (self.elementheight or 0))
		y = y + (btn:getHeight())
	end
	local h = y + 1
	self.viewarea:setSize(self.viewarea:getWidth(),y+1)
	self.scrollbar:setVisibility(h>self:getHeight())
		
	self.scrollbar:setSliderPos(0)
end

function ListBox:setElementHeight(h)
	self.elementheight = h
	self:renewList()
end

function ListBox:mouseWheeled(e,owner)
	self.scrollbar:mouseWheeled(e,owner)
	Container.mouseWheeled(self,e,owner)
end
function ListBox:mouseClicked(e,owner)
	Container.mouseClicked(self,e,owner)
	if self:isFocusable() then self:focus() end
end
function ListBox:isFocusable() return self:isVisible() end

function ListBox:keyEvent(key)
	Container.keyEvent(self,key)
	if key.state~=key.KEY_TYPED or #self.itemlist==0 then return end
	if key.keycode == Keyboard.keycode.UP then
		self:select(self.selecteditem - 1)
	elseif key.keycode == Keyboard.keycode.DOWN then
		self:select(self.selecteditem + 1)
	end
end

function ListBox:transferFocusOnArrows() return false end

LuxModule.registerclassfunction("addItem",
	[[():(ListBox, string caption, [string cmd,[icon, iconskinselection] ]) - ]])
function ListBox:addItem (caption,cmd,icon,iconname)
	self.itemlist[#self.itemlist + 1] = {
			caption = caption, command = cmd or caption,
			icon = icon and icon:clone(), iconname = iconname}
	Timer.finally(self, function () self:renewList() end)
	if #self.itemlist == 1 then
		self:select(1)
	end
end

LuxModule.registerclassfunction("removeItem",
	[[():(ListBox, int index, [string what]) - removes item at given index. If what is 'command' then the 
	index is searched of an item whichs command is equal to id, if what is 'caption' the id is compared 
	with the caption.]])
function ListBox:removeItem(id,what)
	if what == "command" then
		local found = false
		for i,v in ipairs(self.itemlist) do
			if v.command == id then id = i found = true break end
		end
		if not found then  return false end
	elseif what == "caption" then
		local found = false
		for i,v in ipairs(self.itemlist) do
			if v.caption == id then id = i found = true break end
		end
		if not found then return false end
	end
	table.remove(self.itemlist,id)
	self:select(self.selecteditem)
	Timer.finally(self, function () self:renewList() end)
end

LuxModule.registerclassfunction("find",
	[[([index,caption,command]):(ListBox, id, string what) - returns index of element in list that matches the 
	search. If what is 'command', id is compared with the commands, if what is 'caption', the
	id is compared with the captions.]])
function ListBox:find(id,what)
	if what == "command" then
		local found = false
		for i,v in ipairs(self.itemlist) do
			if v.command == id then id = i found = true break end
		end
		if not found then return false end
	elseif what == "caption" then
		local found = false
		for i,v in ipairs(self.itemlist) do
			if v.caption == id then id = i found = true break end
		end
		if not found then return false end
	end
	return id,self.itemlist[id].caption,self.itemlist[id].command
end

LuxModule.registerclassfunction("renameItem",
	[[():(ListBox, int index, [caption],[command],[icon],[iconname]) - renames an item at the given index]])
function ListBox:renameItem(id,caption,cmd,icon,iconname)
	local item = assert(self.itemlist[id],"item index does not exist")
	self.itemlist[id] = {
		caption = caption or item.caption, command = cmd or item.command,
		icon = icon and icon:clone() or item.icon, iconname = iconname or item.iconname}
	Timer.finally(self, function () self:renewList() end)
end

LuxModule.registerclassfunction("clearItems",
	[[():(ListBox) - deletes all items in the list ]])
function ListBox:clearItems ()
	self.itemlist = {}
	self:select(1)
	Timer.finally(self, function () self:renewList() end)
end

LuxModule.registerclassfunction("getSelected",
	[[([int indx, string caption, command]):(ListBox) - returns the currently selected item]])
function ListBox:getSelected ()
	local sel = self.itemlist[self.selecteditem] or {}
	return self.selecteditem, sel.caption, sel.command
end

LuxModule.registerclassfunction("select",
	[[(int selected):(ListBox, string/int something, [boolean iscommand]) -
	selects the specified item by its name, number or commandstring]])
function ListBox:select (something,cmd)
	if type (something) == "string" then
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

	self:invalidate()

	if #self.itemlist>0 then
		local item = self.itemlist[self.selecteditem]
		self:onSelected(self.selecteditem, item.caption,item.command)

		if self.itemscrollinfo and self.itemscrollinfo[self.selecteditem]
			and self:isVisible() then
			self.itemscrollinfo[self.selecteditem]()
		end
	end


	return self.selecteditem
end


LuxModule.registerclassfunction("onSelected",
	[[():(ListBox,int index, string caption, string command) - this function
	can be overloaded without calling this function. It is called each time the
	user selects an item - which does not have to be different from the previously
	selected item.]])
function ListBox:onSelected (idx,cap,cmd)
end
