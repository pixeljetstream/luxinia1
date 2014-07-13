MultiLineLabel = {
	LABEL_ALIGNRIGHT=1,
	LABEL_ALIGNLEFT=2,
	LABEL_ALIGNCENTERED=3,
}
setmetatable(MultiLineLabel,{__index = Label})
LuxModule.registerclass("gui","MultiLineLabel",
	[[A MultiLineLabel element for displaying longer texts. A text can be marked
	with a special syntax (see function description at new) which allows
	creating texts with clickable zones, similiar to a hypertext
	document.]],MultiLineLabel,
	{},"Label")


LuxModule.registerclassfunction("new",
	[[(MultiLineLabel):(class, int x,y,w,h, text, boolean highlights, align,fontsize) -
	creates a labelwith the
	given dimensions. The align value can be a constant from the MultiLineLabel
	table (MultiLineLabel_ALIGNRIGHT, ...). If highlights is true, the displayed
	text is scanned for a special syntax that creates clickable zones on the
	component. The bounds of the zones are fitted on the marked text passages.
	The markers are stripped away and are not displayed. A marker can be set by
	inserting $$link...$$ into the text, where the ... can be just any string. This
	string is then used as zonedescription. To end a zone, a $$link$$ can be set.
	]])
function MultiLineLabel.new (class, x,y,w,h, text, highlights, autofocus, align,fontsize)
	local self = Label.new(class,x,y,w,h,autofocus,align,fontsize)

	self.dataLabel = {
		text = text,
		autofocus = autofocus or false,
		scale = fontsize or 1,
		align = align or LABEL_ALIGNLEFT,
		highlights = highlights or false,
		color = {0,0,0,1},
		zones = {},
	}


	return self
end


LuxModule.registerclassfunction("setColor",
	[[():(float r,g,b,[a]) - sets color for texts]])
function MultiLineLabel:setColor (r,g,b,a)
	--if type(g)=="table" then r,g,b,a = unpack(g) end
	assert(type(r)=="number" and type(g)=="number" and type(b)=="number","numbers expected (is "..type(r)..","..type(g)..","..type(b))
	a = a or 1
	self.dataLabel.color = {r,g,b,a}
	for i,c in pairs(self.l2d or {}) do
		c:color(r or 0,g or 0,b or 0,a or 1)
	end
end

LuxModule.registerclassfunction("setAlign",
	[[():(MultiLineLabel,align) - Sets the alignment of the text]])
function MultiLineLabel:setAlign(align)
	self.dataLabel.align = align
	self:onTextChange()	
end

LuxModule.registerclassfunction("getColor",
	[[(float r,g,b,a):(MultiLineLabel) - returns rgba for basecolor]])
function MultiLineLabel:getColor()
	return unpack(self.dataLabel.color)
end

LuxModule.registerclassfunction("onTextChange",
	[[():(MultiLineLabel) - called if the text changed (ie. by user input)]])
function MultiLineLabel:onTextChange()
	if (self.l2d) then
		for i,v in pairs(self.l2d) do
			v:delete()
		end
		self.l2d = nil
		self:createVisibles()
	end
	self:invalidate()
end

LuxModule.registerclassfunction("getPreferredSize",
	[[(w,h):(MultiLineLabel) - returns the preferred size, which is the longest line in width and the sum of all lineheights in height]])
function MultiLineLabel:getPreferredSize()
	local pw,ph = 0,0
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	
	local currentl2d = l2dtext.new("multiline line","",arialset)
	currentl2d:font(arialtex)
	currentl2d:fontset(arialset)
	--print(self.dataLabel.scale,arialset.scale)
	currentl2d:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
	local sw,sh = currentl2d:scale()
	for line in self:getText():gmatch("[^\n]+") do
		currentl2d:text(line)
		local w,h = currentl2d:dimensions()
		pw,ph = math.max(w*sw,pw),h*sh+ph
	end
	currentl2d:delete()
	
	return pw+2,ph+2
end

function MultiLineLabel:setBounds(...)
	Label.setBounds(self,...)
	self:onTextChange()
end

LuxModule.registerclassfunction("addClickZone",
	[[():(self,x,y,w,h,description,l2d,strstart,strend) - Intern function that is used for adding
	a zone. A zone is an area on the multilinelabel that can receive mouseevents (i.e.
	for creating hyperlinks). This function is called when the multilinelabel is made visible and
	the l2d nodes for displaying are created. The last two values are telling the position of the
	linked textstring.]])
function MultiLineLabel:addClickZone (x,y,w,h,description,l2d, strstart, strend)
	assert(l2d,"a l2d must be associated")
	table.insert(self.zones,{rect = Rectangle:new(x,y,w,h),description = description,l2d = l2d,strstart=strstart,strend=strend})
end

LuxModule.registerclassfunction("testForZone",
	[[([table zone]):(self,x,y) - tests if the coordinates (local) are inside a zone - the first hit
	is returned. The zone contains following value:
	* rect: Rectangle info of size and position of zone
	* description: linkstring info
	* l2d: the l2dtext node where the line is located
	* strstart: position where the linked string starts (including the link)
	* strend: position where the linked string ends (excluding the link)]])
function MultiLineLabel:testForZone (x,y)
	for i,v in ipairs(self.zones) do
		if v.rect:contains(x,y) then return v end
	end
end

LuxModule.registerclassfunction("zoneAction",
	[[():(self,string what,mouseevent,[zone]) - called on mouseevent action. The 'what' value is either
	"exit", "moved" or "clicked" and describe the type of action. The zone is the retrieved zone for that
	mouseposition. If no zone was matched it is nil.]])
function MultiLineLabel:zoneAction(what,mouseevent,zone)

end

function MultiLineLabel:mouseExited(e)
	self:zoneAction("exit",e,nil)
end

function MultiLineLabel:mouseMoved (e)
	local action = self:testForZone(e.x,e.y)
	self:zoneAction("moved",e,action)
end

function MultiLineLabel:mouseClicked (e)
	local action = self:testForZone(e.x,e.y)
	self:zoneAction("clicked",e,action)
end

LuxModule.registerclassfunction("countLines", 
	"(lines):(MultiLineLabel, text) - returns number of lines for this text for the current dimension of the textfield")
function MultiLineLabel:countLines(tx)
	local l2dlist = {}
	local openlink

	local maxwidth = self:getWidth()
	local currentl2d = nil
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local function createline (init)
		if openlink and currentl2d then
			local w,h = currentl2d:dimensions()
			local sw,sh = currentl2d:scale()
			w,h = w*sw,h*sh
			--print("`",w,h)
			self:addClickZone(openlink.x,openlink.y,w-openlink.x,h,openlink.link,openlink.l2d,openlink.start,#currentl2d:text())
			openlink.x,openlink.y = 0,openlink.y + h
			print "overflow"
		end
		if currentl2d then
			local prevtext = currentl2d:text()
			local lastcolor = prevtext:match("^.*(\v%d%d%d).-$")
			if lastcolor then init = lastcolor..init end
		end
		currentl2d = l2dtext.new("multiline line",init,arialset)
		currentl2d.collectforgc = true
		currentl2d:parent(self.basel2d)
		currentl2d:scissorparent(true)
		currentl2d:font(arialtex)
		currentl2d:fontset(arialset)
		currentl2d:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
		currentl2d:color(unpack(self.dataLabel.color))
		table.insert(l2dlist,currentl2d)
		return true
	end

	local function addword(word)
		if not currentl2d then return createline(word) end
		local pureword,cnt = word


		if self.dataLabel.highlights then
			pureword,cnt = word:gsub("%$%$link.-%$%$","")
		end

		local prev = currentl2d:text()
		currentl2d:text(prev..pureword)
		local w,h = currentl2d:dimensions()
		local sw,sh = currentl2d:scale()
		w,h = w*sw,h*sh
		if #prev>0 and w>maxwidth then
			currentl2d:text(prev)
			return false
		end

		if cnt and cnt>0 then
			local line = prev
			for space,link,rest in word:gmatch("(.-)%$%$link(.-)%$%$(.-)") do
				line = line .. space
				currentl2d:text(line)
				if openlink and link ~= openlink.description then
					self:addClickZone(openlink.x,openlink.y, w-openlink.x,h,openlink.link,openlink.l2d,openlink.start,#line)
					openlink = nil
				end
				if not openlink and link~="" then
					local w,h = currentl2d:dimensions()
					openlink = {link = link,x = w, y = (#self.l2d-1)*h, l2d = currentl2d,start = #line}
				end
				line = line .. rest
				currentl2d:text(line)
			end
		end

		return true
	end

	for space,w in string.gmatch(tx, "(%s*)(%S*)") do
		local nspace = space
		for i=1,#space do
			if space:sub(i,i)=="\n" then
				createline("")
				nspace = space:sub(i+1)
			end
		end

		if not addword(nspace..w) then createline(nspace:gsub("^[^\v]+","")..w) end
	end

	local linecnt = #l2dlist
	for i,v in ipairs(l2dlist) do
		v:delete()
	end
	return linecnt
end

LuxModule.registerclassfunction("createVisibles", "@overloaded Component")
function MultiLineLabel:createVisibles ()
	if (self.l2d) then return end

	self.l2d = {}

	self.zones = {}

	local tx = self:getText()
	local openlink

	local maxwidth = self:getWidth()
	local currentl2d = nil
	local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
	local function createline (init)
		if openlink and currentl2d then
			local w,h = currentl2d:dimensions()
			self:addClickZone(openlink.x,openlink.y,w-openlink.x,h,openlink.link,openlink.l2d,openlink.start,#currentl2d:text())
			openlink.x,openlink.y = 0,openlink.y + h
			print "overflow"
		end
		if currentl2d then
			local prevtext = currentl2d:text()
			local lastcolor = prevtext:match("^.*(\v%d%d%d).-$")
			if lastcolor then init = lastcolor..init end
		end
		currentl2d = l2dtext.new("multiline line",init,arialset)
		currentl2d.collectforgc = true

		currentl2d:parent(self.basel2d)
		currentl2d:scissorparent(true)
		currentl2d:font(arialtex)
		currentl2d:fontset(arialset)
		--print(self.dataLabel.scale,arialset.scale)
		currentl2d:scale(self.dataLabel.scale * arialset.scale,self.dataLabel.scale * arialset.scale,1)
		currentl2d:color(unpack(self.dataLabel.color))
		table.insert(self.l2d,currentl2d)
		return true
	end

	local function addword(word)
		if not currentl2d then return createline(word) end
		local pureword,cnt = word


		if self.dataLabel.highlights then
			pureword,cnt = word:gsub("%$%$link.-%$%$","")
		end

		local prev = currentl2d:text()
		currentl2d:text(prev..pureword)
		local w,h = currentl2d:dimensions()
		local sw,sh = currentl2d:scale()
		w,h = w*sw,h*sh
		--print(w,h,self.font)
		if #prev>0 and w>maxwidth then
			currentl2d:text(prev)
			return false
		end

		if cnt and cnt>0 then
			local line = prev
			for space,link,rest in word:gmatch("(.-)%$%$link(.-)%$%$(.-)") do
				line = line .. space
				currentl2d:text(line)
				if openlink and link ~= openlink.description then
					self:addClickZone(openlink.x,openlink.y, w-openlink.x,h,openlink.link,openlink.l2d,openlink.start,#line)
					openlink = nil
				end
				if not openlink and link~="" then
					local w,h = currentl2d:dimensions()
					openlink = {link = link,x = w, y = (#self.l2d-1)*h, l2d = currentl2d,start = #line}
				end
				line = line .. rest
				currentl2d:text(line)
			end
		end

		return true
	end

	for space,w in string.gmatch(tx, "(%s*)(%S*)") do
		local nspace = space
		for i=1,#space do
			if space:sub(i,i)=="\n" then
				createline("")
				nspace = space:sub(i+1)
			end
		end

		if not addword(nspace..w) then createline(nspace:gsub("^[^\v]+","")..w) end
	end

	--local vis = not self:isVisible()
--	for a,b in pairs(self.l2d) do
--		b:rfNodraw(true)
--	end

	self:positionLines()
end

function MultiLineLabel:positionLines(z,clip)
	--local ox,oy = 0,0 --self:local2world(0,0)
	local x,y,w,h = 0,0,self.bounds[3],self.bounds[4]
	
	--x,y = x+ox,y+oy
	local toth = table.getn(self.l2d)*self.dataLabel.scale
	local z,clip = self.__z,self.__clip
	local mw,mh = 0,0
	for i,l2d in ipairs(self.l2d) do
		local sx,sy = l2d:scale()
		local gw = l2d:spacing()*sx -- glyph width
		local text = l2d:text()
		local tw,th = l2d:dimensions()
		local sw,sh = l2d:scale()
		local tw,th = tw*sw,th*sh
		local twx = math.floor(x + 1)
		local y = th*(i-1) + y
		mw = math.max(mw,tw)
		mh = mh + th
		if (self.dataLabel.align==MultiLineLabel.LABEL_ALIGNRIGHT) then
			twx = math.floor(x+w-tw-1)
		elseif (self.dataLabel.align==MultiLineLabel.LABEL_ALIGNCENTERED) then
			twx= math.floor(x+w/2-tw/2)
		end
		l2d:pos(twx,math.floor(y),0)
		l2d:text(text)
			-- above: Why math.floor? Because otherwise the font is drawn blury
		--if (clip) then
		--	l2d:scissor(true)
		--	l2d:scissorsize(clip[3],clip[4])
		--	l2d:scissorstart(clip[1],clip[2])
		--	l2d:rfNodraw(not vis)
		--	l2d:sortid(z)
		--end
	end
	self.minh,self.minw = mw,mh
end


function MultiLineLabel:getMinSize()
	return self:getPreferredSize()
	--return self:getSize()
end

LuxModule.registerclassfunction("positionUpdate","@overloaded Component")
function MultiLineLabel:positionUpdate(z,clip)
	if true then return Component.positionUpdate(self,z,clip) end
	if (not self.l2d) then return end
	self.__z, self.__clip = z,clip

	self:positionLines()

	if (clip == nil) then
		--_print(debug.traceback())
		for a,l2d in pairs(self.l2d) do
			l2d:rfNodraw(true)
		end
		return z+1
	end
	local vis = self:isVisible()
	for a,l2d in pairs(self.l2d) do
		l2d:scissor(true)
		l2d:scissorsize(clip[3],clip[4])
		l2d:scissorstart(clip[1],clip[2])
		l2d:rfNodraw(not vis)
		l2d:sortid(z)
	end

	self:updateColors()
	return z + 1
end
