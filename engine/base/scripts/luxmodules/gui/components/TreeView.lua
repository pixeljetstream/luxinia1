-- yet another undocumented feature! YAY!

TreeView = { -- copying simply the listbox here.
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
setmetatable(TreeView,{__index = Container})

function TreeView . new (class,x,y,w,h)
	local self = Container.new(class, x,y,w,h)
	self:setSkin("default",true)
	self.itemids = {}
	self.itemlist = {}
	
	self.hscroll = self:add(Slider:new(0,0,0,0))
	self.vscroll = self:add(Slider:new(0,0,0,0,true))
	self.view = self:add(Container:new(0,0,0,0))
	self.content = self.view:add(Container:new(0,0,0,0))
	
	self.sliderwidth = 12
	self.inset = 4
	self.indentionwidth = 12
	self.animated = false
	self.animationspeed = .20
	self.animationdelay = 20
	
	local skin = self:getSkin()
	assert(skin.texture)
	
	self.foldedicon = ImageIcon:new(skin.texture,16,16,blendmode.decal(),
		{
			default = {64,160,16,16},
		}
	)
	self.expandedicon = ImageIcon:new(skin.texture,16,16,blendmode.decal(),
		{
			default = {80,160,16,16},
		}
	)
	self.normalicon = ImageIcon:new(skin.texture,16,16,blendmode.decal(),
		{
			default = {96,160,16,16},
		}
	)
	self : doLayout()
	
	return self
end

function TreeView : addItem (id,parentid, caption, icon, onclicked,folded)
	local tv = self
	assert(id, "Expected an id as first argument")
	assert(not self.itemids[id], "The ID is not unique or the element has already been added")
	
	local parent = self.itemids[parentid]
	local item = {
		id = id, caption = caption or "", icon = icon, onclicked = onclicked, parent = parent,
		folded = folded,
		children = {}
	}
	item.gui = self.content:add(Button:new(0,0,0,0,caption))
	item.gui.skinnames = ComboBox.labelskinnames
	item.expander = self.content:add(Button:new(0,0,0,0))
	item.gui:setFont(self.font)
	item.gui:setSkin "default"
	local exicon = tv.normalicon --item.folded and tv.foldedicon or tv.expandedicon
	item.expander:getSkin():setIcon(exicon :clone())
	--item.expander:getSkin():setIconAlignment (0,0)
	item.expander:getSkin():setSkinColor(1,1,1,0)
	function item.gui:onClicked() 
		local foldstate
		if onclicked then foldstate = onclicked() end
		if not item.children or #item.children==0 then return end
		print ("------->",foldstate)
		if foldstate == nil then
			item.folded = not item.folded
		elseif foldstate == "nochange" then
			return
		else
			item.folded = foldstate == "fold"
		end
		local icon = item.folded and tv.foldedicon or tv.expandedicon
		item.expander:getSkin():setIcon(icon:clone())
		tv.layoutcache = nil
		tv:doLayout()
	end
	function item.expander:onClicked() 
		if not item.children or #item.children==0 then return end
		item.folded = not item.folded
		local icon = item.folded and tv.foldedicon or tv.expandedicon
		item.expander:getSkin():setIcon(icon:clone())
		tv.layoutcache = nil
		tv:doLayout()
	end
	if icon then item.gui:getSkin():setIcon(icon) end
	
	self.itemids[id] = item
	
	if parent then
		parent.children[#parent.children+1] = item
	else
		table.insert(self.itemlist,item)
	end
	self.layoutcache = nil
end

function TreeView : removeAllItems ()
	self.itemlist = {}
	self.itemids = {}
	self.content:removeChildren()
	self.layoutcache = nil
	self:doLayout()
end

function TreeView : removeItem (id)
	local item = self.itemids[id]
	if not item then return end
	local list = self.parent and self.parent.children or self.itemlist
	for i=1,#list do
		if list[i] == item then
			table.remove(list,i)
			break
		end
	end
	item.gui:delete()
	self.layoutcache = nil
	self:doLayout()
end


function TreeView : doLayout()
	-- MUST prevent relayouting if parameters are equal
	-- thus testing the whole tree against a copy and sizes etc.
	
	local w,h = self:getSize()
	
	
	local ins = self.inset
	local sw = self.sliderwidth
	self.hscroll:setBounds(ins,h-sw-ins,w-sw-ins,sw)
	self.vscroll:setBounds(w-sw-ins,ins,self.sliderwidth,h-sw-ins)
	local vw,vh = w-sw-ins*2,h-sw-ins*2
	self.view:setBounds(ins,ins,vw+sw-ins,vh+sw-ins)
	
	--self.content:removeChildren()
	local y,maxw
	if self.layoutcache then
		y = self.layoutcache.maxh
		maxw = self.layoutcache.maxw
	else
		y = 0
		maxw = 0
		local iw = self.indentionwidth
		local function traverse (tab,x,visible)
			for i,item in ipairs(tab) do
				self.content:add(item.gui)
				local mw,mh = 0,0
				if visible then mw,mh = item.gui:getMinSize() end
				if not self.animated then
					item.gui:setBounds(x,y,mw,visible and mh or 0)
					item.expander:setBounds(x-iw,y,iw,visible and mh or 0)
					item.gui:setVisibility(visible)
					item.expander:setVisibility(visible)
					local icon = #item.children>0 and (item.folded and self.foldedicon or self.expandedicon) or self.normalicon
					item.expander:getSkin():setIcon(icon:clone())
				else
					item.gui:setVisibility(item.gui:getHeight()>1 or visible)
					item.gui:moveToRect(x,y,mw,visible and mh or 0,self.animationspeed,self.animationdelay,function()
						item.gui:setVisibility(visible)
					end)
					local icon = #item.children>0 and (item.folded and self.foldedicon or self.expandedicon) or self.normalicon
					item.expander:getSkin():setIcon(icon:clone())
					item.expander:setVisibility((item.gui:getHeight()>1 or visible))
					item.expander:moveToRect(x-iw,y,iw,visible and mh or 0,self.animationspeed,self.animationdelay,function()
						item.expander:setVisibility(visible)
					end)
				end
				
				if visible then
					maxw = math.max(x+mw,maxw)
					y = y + mh
					function item.gui:isFocusable() 
						return self:isVisible()
					end
				else
					function item.gui:isFocusable() 
						return false
					end
				end
				
				if item.children then 
					traverse(item.children,x+self.indentionwidth,visible and not item.folded)
				end
			end
		end
		traverse(self.itemlist,self.indentionwidth,true)
		self.content:setSize(2^20,2^20)
	end
	self.layoutcache = {maxw = maxw, maxh = y}
	
	local ow = math.max(0,maxw - vw)
	local oh = math.max(0,y - vh)
	local x,y = self.content:getLocation()
	x,y = math.max(-ow,x),math.max(-oh,y)
	self.content:setLocation(x,y)
	self.vscroll:setVisibility(oh>0)
	self.hscroll:setVisibility(ow>0)
	
	local px,py = -x / ow, -y / oh
	self.vscroll:setSliderPos(py,true)
	self.hscroll:setSliderPos(px,true)
	local tv = self
	function self.vscroll:onValueChanged(val)
		tv.content:setLocation(tv.content:getX(),-oh*val)
	end
	function self.hscroll:onValueChanged(val)
		tv.content:setLocation(-ow*val,tv.content:getY())
	end
	local px,py 
	function self.view:mousePressed(me,over)
		Container.mousePressed(self,me,over)
		local over = self:getComponentAt(me.x,me.y)
		if (over~=self and over~=tv.content) or not input.ismousedown(0) then 
			px,py = nil
			return 
		end
		self:lockMouse()
		print "LOCK"
		px,py = me.x,me.y
	end
	function self.view:mouseReleased(me,over)
		self:unlockMouse()
		Container.mouseReleased(self,me,over)
		px,py = nil
	end
	function self.view:mouseMoved(me,over)
		Container.mouseMoved(self,me,over)
		if Component.getMouseLock()~=self then return end
		local dx,dy = me.x-px,me.y-py
		px,py = me.x,me.y
		local x,y = tv.content:getLocation()
		x,y = math.min(0,math.max(-ow,x+dx)),math.min(0,math.max(-oh,y+dy))
		tv.content:setLocation(x,y)
		local px,py = -x / ow, -y / oh
		print(px)
		tv.vscroll:setSliderPos(py,true)
		tv.hscroll:setSliderPos(px,true)
	end
end

function TreeView : removeItem (id)
	if not id or not self.itemids[id] then return end
	local item = self.itemids[id]
	self.itemids[id] = nil
	for idx, value in ipairs(item.parent.children or {}) do
		if value == item then table.remove(item.parent.children,idx) break end
	end
end

--[=[
------
-- TreeView

TreeView:new(x,y,w,h)
-- scroll field 2 slider horizontal / vertical


TreeView:getRootItem()	-- always exists
TreeView:addItem(item,parentitem)
TreeView:removeItem(item)	-- invalid for root

TreeView:getItemParent(item)
TreeView:getItemPrev(item)	-- can jump in hierarchy
TreeView:getItemNext(item)
TreeView:getItemPrevSibling(item)	-- cannot change depth
TreeView:getItemNextSibling(item)

TreeView:getItemByData(data)
TreeView:getItemByText(text)
TreeView:getItemByDataConcat(datatab) -- table of data hierharchy
TreeView:getItemByTextConcat(texttab)
		-- e.g {"C:","test","subdir"}


TreeView:iterateItemChildren(item,funciterator)	-- only primary children list
TreeView:removeChildren(item)

TreeView:iterateItemChildrenAll(item,funciterator)	--children + sub hierarchy
TreeView:removeChildrenAll(item)

TreeView:expandItem(item)
TreeView:collapseItem(item)
TreeView:selectItem(item)

TreeView:refresh()		-- should be called when finished with add/remove
						-- rebuilds visuals, adds/removes sliders

-- callbacks to dynamically, add/change Tree
-- event on clicking icon (focus + > <)
TreeView:onCollapseItem(item)
TreeView:onExpandItem(item)

TreeView:onSelectItem(item)

TreeView:setTextures(texicons,icontab) -- horizontally icons 
TreeView:setItemsRenameable(state)


--------
-- TreeItem

TreeItem:setIcon

-- evtl nur Label ?, Component wäre sehr flexibel aber evtl overkill ?
-- auch schwer zu handhaben irgendwie mit so sachen wie expand on click
-- oder "focus" traversal usw.

TreeItem:setComponent


TreeItem:setText
TreeItem:getText

TreeItem:setData
TreeItem:getData









]=]