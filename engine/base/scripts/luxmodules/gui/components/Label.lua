Label = {
	LABEL_ALIGNRIGHT=1,
	LABEL_ALIGNLEFT=2,
	LABEL_ALIGNCENTERED=3,
	LABEL_ALIGNTOP=4,
	LABEL_ALIGNBOTTOM=5,
	skinnames = {
		defaultskin = "label",
		hoveredskin = "label_hover",
		pressedskin = "label_pressed",
		pressedhoveredskin = "label_hover_pressed",
		focusedskin = "label_focus",
		focusedhoveredskin = "label_focus_hover",
		focusedpressedskin = "label_focus_pressed",
		focusedhoveredpressedskin = "label_focus_hover_pressed",
	}
}
setmetatable(Label,{__index = Component})
LuxModule.registerclass("gui","Label",
	[[A Label element for displaying only.]],Label,
	{
		LABEL_ALIGNRIGHT = "[int] - constant for right text alignment",
		LABEL_ALIGNLEFT = "[int] - constant for left text alignment",
		LABEL_ALIGNCENTERED = "[int] - constant for centered text alignment",
		LABEL_ALIGNTOP = "[int] - constant for vertical text alignment at top",
		LABEL_ALIGNBOTTOM = "[int] - constant for vertical text alignment at bottom",
	},"Component")


LuxModule.registerclassfunction("new",
	[[(Label):(class, int x,y,w,h, text, [align, [autofocus] ]) - creates a labelwith the
	given dimensions. The align value can be a constant from the Label
	table (Label_ALIGNRIGHT, ...).]])
function Label.new (class, x,y,w,h, text, align,autofocus)
	local self = Component.new(class,x,y,w,h)

	self.dataLabel = {
		text = text,
		autofocus = autofocus or false,
		scale = fontsize or 1,
		align = align or Label.LABEL_ALIGNLEFT,
		valign = Label.LABEL_ALIGNCENTERED,
		tabwidth = 0,
	}

	return self
end


function Label:setSkin(skin)
	Component.setSkin(self,skin)
	if self:getSkin() then
--		if self.l2d then self.l2d.text:delete() end
--		self.l2d = nil
		if self.l2d then self.l2d.text:text("") end
		self:getSkin():setLabelTabWidth(self.dataLabel.tabwidth)
		self:getSkin():setLabelText(self.dataLabel.text)
		
		self:setAlignment(self.dataLabel.align)
	else
		if self.isDisplayedFlag then
			self:createVisibles()
		end
	end
end

LuxModule.registerclassfunction("setAlignment",
	[[():(Label,[alignment],[vertical_align]) - sets textalignment, or resets to default (left aligned)]])
function Label:setAlignment (align,valign)
	self.dataLabel.align = align or Label.LABEL_ALIGNLEFT
	self.dataLabel.valign = valign or Label.LABEL_ALIGNCENTER
	if self:getSkin() then
		local align = self.dataLabel.align
		self:getSkin():setLabelPosition(
			align == Label.LABEL_ALIGNLEFT and Skin2D.ALIGN.LEFT or
			align == Label.LABEL_ALIGNRIGHT and Skin2D.ALIGN.RIGHT or
			Skin2D.ALIGN.CENTER,
			align == Label.LABEL_ALIGNTOP and Skin2D.ALIGN.TOP or
			align == Label.LABEL_ALIGNBOTTOM and Skin2D.ALIGN.BOTTOM or
			Skin2D.ALIGN.CENTER)
	end
	self:updateText()
end

LuxModule.registerclassfunction("getAlignment",
	[[(string alignment):(Label) - returns current alignment of the label's text]])
function Label:getAlignment ()
	return self.dataLabel.align or Label.LABEL_ALIGNLEFT
end

LuxModule.registerclassfunction("onTextChange",
	[[():(Label) - called if the text changed (ie. by user input)]])
function Label:onTextChange()
	if self:getSkin() then
		self:getSkin():setLabelText(self.dataLabel.text)
		if self.l2d then self.l2d.text:text("") end
	elseif (self.l2d) then
		self.l2d.text:text(self.dataLabel.text)
		self:updateText()
	end
end

LuxModule.registerclassfunction("isFocusable",
	[[(boolean):(Label) - returns true if visible]])
function Label:isFocusable() return self.dataLabel.autofocus and true and self:isVisible() end

LuxModule.registerclassfunction("getText",
	[[(string):(Label) - returns the string label]])
function Label:getText() return self.dataLabel and self.dataLabel.text or "" end

LuxModule.registerclassfunction("setText",
	[[():(Label,string) - sets the label's text]]) -- ']]
function Label:setText(text)
	self.dataLabel.text = text
	self:onTextChange()
end

--function Label:setFontSize (sz)
--	self.dataLabel.scale = sz
--	self:onTextChange()
--end

LuxModule.registerclassfunction("getTabWidth",
	[[(float):(Label) - gets the label's tabwidth.]]) -- ']]
function Label:getTabWidth ()
	return self.dataLabel.tabwidth
end

LuxModule.registerclassfunction("setTabWidth",
	[[():(Label,float) - sets the label's tabwidth. 0 means fontspacing*4 is used, which is default.]]) -- ']]
function Label:setTabWidth (sz)
	self.dataLabel.tabwidth = sz
	
	if self:getSkin() then
		self:getSkin():setLabelTabWidth(sz)
	elseif (self.l2d) then
		self.l2d.text:tabwidth(sz)
		self:updateText()
	end
end

LuxModule.registerclassfunction("toString",
	[[(string):(Label) - returns simple component representation]])
function Label:toString ()
	return string.format("[Label text=%s]",self:getText())
end

--------------------------------------------------------------------------------
LuxModule.registerclassfunction("deleteVisibles", "@overloaded Component")
function Label:deleteVisibles ()
	Component.deleteVisibles(self)
	if (not self.l2d or self:getSkin()) then return end
	for a,b in pairs(self.l2d) do
		b:delete()
	end
	self.l2d = nil
end

function Label:getMinSize()
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local l2d = l2dtext.new("",self:getText(),arialset)
	
	l2d:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
	l2d:font(arialtex)
	l2d:fontset(arialset)
	local w,h = l2d:dimensions()
	local tw,th = l2d:font():dimension()
	w,h = w*tw/256,h*th/256
	l2d:delete()
	return w,h
end

function Label:setFont(font)
	Component.setFont(self,font)
	if not self.l2d then return end
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	self.l2d.text:font(arialtex)
	self.l2d.text:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
	self.l2d.text:fontset(arialset)
end

LuxModule.registerclassfunction("createVisibles", "@overloaded Component")
function Label:createVisibles (parent)
	Component.createVisibles(self)
	if (self.l2d) then return end

	self.l2d = {}

	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	self.l2d.text = l2dtext.new("textbutton caption","",arialset)
	self.l2d.text.collectforgc = true
	self.l2d.text:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
	self.l2d.text:font(arialtex)
	self.l2d.text:fontset(arialset)
	self.l2d.text:tabwidth(self.dataLabel.tabwidth)
	self.l2d.text:parent(self.basel2d)
	self.l2d.text:scissorparent(true)

	--self.l2d.text:scale(self.dataLabel.scale,self.dataLabel.scale,self.dataLabel.scale)
	--self.l2d.text:spacing(8)
end

LuxModule.registerclassfunction("getMaxLines","(lines):(Label,[height=getHeight]) - "..
	"gets the maximum number of currently (or for height) visible lines")
function Label:getMaxLines (height)
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local h = height or self:getHeight()
	return math.floor(h/arialset:linespacing())
end

LuxModule.registerclassfunction("maxLineCount",
	[[(float lines):([height]) - uses the current label's font and size to
	calculate how many lines can be displayed at the moment. The return
	value is floating point, as lines might not fit in completly.
	]])
function Label:maxLineCount (height)
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local l2d = l2dtext.new("t"," ",arialset)
	local w,lh = l2d:dimensions()
	lh = lh * self.dataLabel.scale * arialset.scale
	
	height = height or self:getHeight()
	--print("HHHH",height,lh,math.floor(height/lh))
	return height / lh
end
LuxModule.registerclassfunction("textDimensions",
	[[(float,float):(Label,text) - returns the text's bounding size (can exceed component's size)]])
function Label:textDimensions (tx)
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local l2d = l2dtext.new("t",tx,arialset)
	local w,h = l2d:dimensions()
	w,h = w*self.dataLabel.scale*arialset.scale,h*self.dataLabel.scale*arialset.scale
	return w,h
end

LuxModule.registerclassfunction("wrapLine",
	[[(string):(Label,string,[width]) - uses the current label's font and size to wrap the line by inserting \n
	characters. If a width is provided, the wrapping is done using this width
	]])
function Label:wrapLine (tx,width)
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local l2d = l2dtext.new("textbutton caption","",arialset)

	l2d:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
	l2d:font(arialtex)

	local function size(tx)
		l2d:text(tx)
		local tw,th = l2d:dimensions()
		local sw,sh = l2d:scale()
		tw = tw*sw
		th = th*sh
		return tw,th
	end

	local w = width or self:getWidth()

	local lines = {}
	for word,space in tx:gmatch "(%S*)(%s*)" do
		local line = lines[#lines]
		if not line then
			line = ""
			table.insert(lines,line)
		end
		local nline = line..word
		local nw = size(nline)
		--print("wordwrap for ",w,nw,nline)
		if nw>w then
			table.insert(lines,word..space)
		else
			lines[#lines] = nline..space
		end
	end

	l2d:delete()

	return table.concat(lines,"\n")
end

function Label:setBounds(...)
	Component.setBounds(self,...)
	self:updateText()
end

function Label:updateText ()
  if not self.l2d  or not self.l2d.text then return end
  if self:getSkin() then return end
	local ox,oy = 0,0--self:local2world(0,0)

	local text = self:getText()
	local x,y,w,h = ox,oy,self.bounds[3],self.bounds[4]
	self.l2d.text:text(text)

	local tw,th = self.l2d.text:dimensions()
	local sw,sh = self.l2d.text:scale()
	tw = tw*sw
	th = th*sh


	local twx = x
	local twy = math.floor(y+h/2-th/2)

	if (self.dataLabel.align==Label.LABEL_ALIGNRIGHT) then
		twx = math.floor(x+w-tw)
	elseif (self.dataLabel.align==Label.LABEL_ALIGNCENTERED) then
		twx= math.floor(x+w/2-tw/2)
	end
	if self.dataLabel.valign==Label.LABEL_ALIGNTOP then
		twy = 0
	elseif self.dataLabel.valign ==Label.LABEL_ALIGNBOTTOM then
		twy = h-th
	end
	self.l2d.text:pos(twx,twy,0)
	self:updateColors()
end

LuxModule.registerclassfunction("positionUpdate","@overloaded Component")
function Label:positionUpdate(z,clip)
	if true then return Component.positionUpdate(self,z,clip) end

	if (not self.l2d) then return end

	self:updateText()
		-- above: Why math.floor? Because otherwise the font is drawn blury

	if (clip == nil) then
		--_print(debug.traceback())
		for a,l2d in pairs(self.l2d) do
			l2d:rfNodraw(true)
		end
		return z+1
	end
	local vis = self:isVisible()
	for a,l2d in pairs(self.l2d) do
		l2d:scissor(Component.isClipped())
		l2d:scissorsize(clip[3],clip[4])
		l2d:scissorstart(clip[1],clip[2])
		l2d:rfNodraw(not vis)
		l2d:sortid(z)
	end

	self:updateColors()
	return z + 1
end

LuxModule.registerclassfunction("setFontColor",
	[[():(r,g,b,a) - sets RGBA font color of the label
	]])
function Label:setFontColor(r,g,b,a)
	if self.colors == Label.colors then
		self.colors = {}
		setmetatable(self.colors,{__index = Label.colors})
	end
	if self.focuscolors == Label.focuscolors then
		self.focuscolors = {}
		setmetatable(self.focuscolors,{__index = Label.colors})
	end
	self.colors.text = {r,g,b,a}
	self.focuscolors.text = {r,g,b,a}
	if self.l2d then self.l2d.text:color(r,g,b,a) end
	if self:getSkin() then self:getSkin():setFontColor(r,g,b,a) end
end

function Label:updateSkin()
    Component.updateSkin(self)
    self:updateColors()
    self:updateText()
end

function Label:updateColors()
	local colortab = self:hasFocus() and self.focuscolors or self.colors
	for a,b in pairs(self.l2d or {}) do
		local c = colortab[a]
		if (type(c)=="table") then
			b:color(unpack(c))
		end
		if (type(c)=="function") then
			b:color(c(self))
		end
	end
end


LuxModule.registerclassfunction("mouseClicked","@overloaded Component")
function Label:mouseClicked(event)
	Component.mouseClicked(self,event)
	self:focus()
end

LuxModule.registerclassfunction("onGainedFocus",
	"():(from) - simply forwards the focus to the next element.")
function Label:onGainedFocus (from)
	local dir = (Keyboard.isKeyDown(Keyboard.keycode.LSHIFT) or
		Keyboard.isKeyDown(Keyboard.keycode.RSHIFT)) and -1 or 1
	local focus = self:getFocusComponentAt(dir)
	if focus~=self then
		return self:transferFocus(dir)
	end
end
