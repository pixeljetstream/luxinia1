Skin2D = {
	REASON = {
		RESIZED="resized",
		SKINCHANGE="skinchange",
		VISCHANGE="vischange",
		CREATEDVIS = "createdvis",
		DELETEDVIS = "deletedvis",
		NEWTEXT = "newtext",
		ICONCHANGED = "iconchanged",
		COLORCHANGED = "colorchanged",
	},
	ALIGN = {
		TOP = "top",
		BOTTOM = "bottom",
		CENTER = "center",
		LEFT = "left",
		RIGHT = "right",
		ICONTORIGHT = "icontoright",
		ICONTOLEFT = "icontoleft",
		ICONTOTOP = "icontotop",
		ICONTOBOTTOM = "icontobottom",
		FONTCHANGED = "fontchanged",
	},

	defaultfont = "arial11-16x16",
}

LuxModule.registerclass("gui","Skin2D",
	[[The Skin2D class is a two dimensional graphical representation for
	abstract information. It defines the look and feel of components.

	I.e. this information could be a button. A skin does only have to provide some
	mechanisms for hiding, creating and deleting. Any class that requires a
	twodimensional representation (x,y,width,height) can use a skin to display
	itself. The skin takes only care about its graphical representation, its
	position and dimensions.

	Skins can provide multiple "surfaces". Surfaces are variations of a skin.
	Components may request certain surfacetypes for a skin (i.e. mouseover
	event) to change the visual appearance of the skin.

	A Skin can also provide mechanisms to show text on the label.
	]],Skin2D,{
	bounds = "[table] - contains bounds of image",
	["bound.x"] = "[float] - position x",
	["bound.y"] = "[float] - position y",
	["bound.w"] = "[float] - width",
	["bound.h"] = "[float] - height",
	["REASON.RESIZED"] = "{[string]}=resized - resized skin",
	["REASON.SKINCHANGE"] = "{[string]}=skinchange - skinchange reason",
	["REASON.VISCHANGE"] = "{[string]}=vischange - vischange reason",
	["REASON.CREATEDVIS"] = "{[string]}=createdvis - vischange reason",
	["REASON.DELETEDVIS"] = "{[string]}=deletedvis - vischange reason",
	["REASON.NEWTEXT"] = "{[string]}=newtext - new label text",
	["REASON.ICONCHANGED"] = "{[string]}=iconchanged - changed icon reason",
	["REASON.COLORCHANGED"] = "{[string]}=colorchanged - changed color (Skin/font) reason",
	["ALIGN.TOP"] = "{[string]} - position information for label",
	["ALIGN.BOTTOM"] = "{[string]} - position information for label",
	["ALIGN.CENTER"] = "{[string]} - position information for label",
	["ALIGN.LEFT"] = "{[string]} - position information for label",
	["ALIGN.RIGHT"] = "{[string]} - position information for label",
	["ALIGN.ICONTOLEFT"] = "{[string]} - position information for icon",
	["ALIGN.ICONTORIGHT"] = "{[string]} - position information for icon",
	["ALIGN.ICONTOBOTTOM"] = "{[string]} - position information for icon",
	["ALIGN.ICONTOTOP"] = "{[string]} - position information for icon",
	defaultfont = "[string]=arial11-16x16 - fontname to be used as default",
	z = "[float]=0 - z position in space",
	id = "[int]=0 - sortid for l2dlistset",
	icon = "[Icon]=nil - the icon for this skin",
	iconalignhorizontal = "[float/string]=ALIGN.ICONTOLEFT - alignment for icon",
	iconalignvertical = "[float/string]=ALIGN.CENTER - alignment for icon",
	textvertical = "[float/string]=ALIGN.CENTER - alignment for text",
	texthorizontal = "[float/string]=ALIGN.CENTER - alignment for text",
	ptop = "[float] - current padding to top",
	pbottom = "[float] - current padding to bottom",
	pleft = "[float] - current padding to left",
	pright = "[float] - current padding to right",
	paddings = "[table] - table containing paddings for different skins",
	currentskin = "[string]=default - current key of used skin",
	})

local skincnt = 0

LuxModule.registerclassfunction("new",
	[[(Skin2D):(class) - Sets
	metatable index to class. Per default, a skin does not create any visual
	objects during start. These must be created by calling createVisibles. ]])
function Skin2D.new (class)
	skincnt = skincnt+1
	local self = {
		bounds = {
			x = 0, y = 0, w = 0, h = 0
		},
		skincntid = skincnt,
		visibility = true,
		z = 0,
		id = 0,
		constructor = constructor,
		iconalignhorizontal = Skin2D.ALIGN.ICONTOLEFT,
		iconalignvertical = Skin2D.ALIGN.CENTER,
		paddings = {default = {0,0,0,0}},
		currentskin = "default",
		fontsize = 1,
		fontspacing = 8,
		fonttabwidth = 0,
		skincolor = {1,1,1,1},
		fontcolor = {0,0,0,1},
	}
	setmetatable(self,{
		__index = class,
		__tostring = function (self) return self:toString() end,
		__gc = function (self) self:onDestroy() end
	})

	return self
end

LuxModule.registerclassfunction("setSkinColor",
	[[():(Skin2D, float r,g,b,a) - sets Primary RGBA Color for the skin]])
function Skin2D:setSkinColor (r,g,b,a)
	local c = self.skincolor
	c[1],c[2],c[3],c[4] = r or c[1], g or c[2], b or c[3], a or c[4]
	self:update(Skin2D.REASON.COLORCHANGED)
end

LuxModule.registerclassfunction("getSkinColor",
	[[(float r,g,b,a):(Skin2D) - Primary RGBA Color for the skin]])
function Skin2D:getSkinColor ()
	return unpack(self.skincolor)
end

LuxModule.registerclassfunction("setFontColor",
	[[():(Skin2D, float r,g,b,a) - sets RGBA Color for the label]])
function Skin2D:setFontColor (r,g,b,a)
	local c = self.fontcolor
	c[1],c[2],c[3],c[4] = r or c[1], g or c[2], b or c[3], a or c[4]
	self:update(Skin2D.REASON.COLORCHANGED)
end

LuxModule.registerclassfunction("getFontColor",
	[[(float r,g,b,a):(Skin2D) - RGBA Color for the label]])
function Skin2D:getFontColor ()
	return unpack(self.fontcolor)
end


LuxModule.registerclassfunction("setFont",
	[[():(Skin2D,[string fontname]) - Name of font to be used, pass nil (or nothing) if the defaultfont should be used]])
function Skin2D:setFont(font)
	self.font = font
	self:update(Skin2D.REASON.FONTCHANGED)
end

LuxModule.registerclassfunction("getFontset",
	[[(fontset):(Skin2D) - returns currently fontset]])
function Skin2D:getFontset()
	return UtilFunctions.loadFontFile (self.font or self.defaultfont)
end

LuxModule.registerclassfunction("setIcon",
	[[():(Skin2D,Icon,[iconskinname, [alignh,[alignv] ] ]) -
	sets the Icon to be used for this skin.
	The second parameter denotes the name of the skin that should be used for
	the icon when displayed. If it is nil, the currently set skinname for the
	icon (that can be nil) is not overwritten. If it is false, it will be
	set to nil, otherwise it is set to the given value. If the
	3rd and 4th parameter is not nil, the icon alignment is set to these values.]])
function Skin2D:setIcon(Icon,skinname,alignh,alignv)
	if self.icon == Icon then return end
	if self.icon then
		Icon:setClip(self:getClip())
	end
	if (self.icon) then self.icon:deleteVisibles() end
	self.icon = Icon
	if skinname~=nil then
		self.iconselection = skinname or nil
	end
	if alignh then
		self.iconalignhorizontal = alignh
	end
	if alignv then
		self.iconalignvertical = alignv
	end
	self:update(Skin2D.REASON.ICONCHANGED)
end

LuxModule.registerclassfunction("getIcon",
	[[([Icon]):(Skin2D) - gets the Icon to be used for this skin.]])
function Skin2D:getIcon()
	return self.icon
end

LuxModule.registerclassfunction("clone",
	[[(Skin2D):(Skin2D) - returns a copy of the skin. This function MUST be
	overloaded, otherwise an error is thrown.]])
function Skin2D:clone ()
	error("Skin2D cloning function must be overloaded")
end

LuxModule.registerclassfunction("setLocation",
	[[():(Skin2D,x,y) - sets 2D location of skin (calls directly setBounds)]])
function Skin2D:setLocation (x,y)
	self:setBounds(x,y,self.bounds.w,self.bounds.h)
end

LuxModule.registerclassfunction("setDimensions",
	[[():(Skin2D,w,h) - sets width and height of skin (calls directly setBounds)]])
function Skin2D:setDimension (w,h)
	self:setBounds(self.bounds.x,self.bounds.y,w,h)
end

LuxModule.registerclassfunction("setBounds",
	[[():(Skin2D,x,y,w,h) - sets location and dimension of skin.
	Calls sizechanged ("resize") - only if the size and position was truly changed!]])
function Skin2D:setBounds (x,y,w,h)
	if (self.bounds.w == w and self.bounds.h == h and self.bounds.x == x and self.bounds.y == y) then
		return
	end
	self.bounds = {x=x,y=y,w=w,h=h}
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("getBounds",
	[[(x,y,w,h):(Skin2D) - returns location and dimension of skin]])
function Skin2D:getBounds ()
	return self.bounds.x,self.bounds.y,self.bounds.w,self.bounds.h
end

LuxModule.registerclassfunction("setLabelText",
	[[():(Skin2D, [text]) - sets the text for the skin. If no argument is nil
	or is missing the text is removed.]])
function Skin2D:setLabelText (tx)
	self.labeltext = tx
	self:update(Skin2D.REASON.NEWTEXT)
end

LuxModule.registerclassfunction("getLabelSize",
	[[(w,h):(Skin2D) - Returns the current size of the label.]])


LuxModule.registerclassfunction("getLabelText",
	[[([text]):(Skin2D) - gets the text for the skin. ]])
function Skin2D:getLabelText()
	return self.labeltext
end

LuxModule.registerclassfunction("setLabelFontSize",
	[[():(Skin2D, size) - size of the font for the label. 1 = 100%, 2 = 200% etc. Default value: 1]])
function Skin2D:setLabelFontSize (size)
	if (self.fontsize==size) then return end
	self.fontsize = size
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setLabelFontSpacing",
	[[():(Skin2D, size) - spacing of the font for the label]])
function Skin2D:setLabelFontSpacing (size)
	if (self.fontspacing==size) then return end
	self.fontspacing = size
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setLabelTabWidth",
	[[():(Skin2D, spacing) - tab spacing of font for the label. 0 means spacing * 4 is used.]])
function Skin2D:setLabelTabWidth (tabwidth)
	if (self.fonttabwidth==tabwidth) then return end
	self.fonttabwidth = tabwidth
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setLabelPosition",
	[[():(Skin2D,horizontal,vertical) - sets position of label. The position
	can be a string as described in Skin2D.ALIGN or a number. ]])
function Skin2D:setLabelPosition (horizontal,vertical)
	if (self.texthorizontal==horizontal and self.textvertical == vertical) then return end
	self.texthorizontal = horizontal
	self.textvertical = vertical
	self:update(Skin2D.REASON.RESIZED)
end

local validations = {}

LuxModule.registerclassfunction("update",
	[[():(Skin2D, string reason) - called if the visual appearance changed.
	The reason contains the source which caused the modification. Depending
	on your implementation, it might be useful to collect modifications until
	the frame is finaly really drawn.]])
function Skin2D:update (reason)
	--validations[self] = reason
	Timer.finally(self,function () self:validate(reason) end)
	--self:validate(reason)
end

function Skin2D:validate (reasons)
end

LuxModule.registerclassfunction("getPreferredSize",
	[[(w,h):(Skin2D,w,h) - Returns preferred size of the skin. A skin may have a
	minimum or maximum size - or wants to be sized in multiplies of a certain
	size.]]);
function Skin2D:getPreferredSize(w,h) return math.floor(w),math.floor(h) end

LuxModule.registerclassfunction("getPreferredLocation",
	[[(x,y):(Skin2D,x,y) - Returns position which is preferred by the skin.
	Since most textures are drawn filtered, a component might like to be
	positionend at an integer position.]]);
function Skin2D:getPreferredLocation(x,y) return math.floor(x),math.floor(y) end

LuxModule.registerclassfunction("getPreferredBounds",
	[[(x,y,w,h):(Skin2D,x,y,w,h) - returns preferred bounds for this skin.
	Calls getPreferredSize and getPreferredLocation per default.]])
function Skin2D:getPreferredBounds (x,y,w,h)
	x,y = self:getPreferredLocation(x,y)
	return x,y, self:getPreferredSize(w,h)
end

LuxModule.registerclassfunction("setClip",
	[[():(Skin2D,[x,y,w,h]) - Sets clipping for skin. If no value is passed,
	the clipping is disabled. ]])
function Skin2D:setClip(x,y,w,h) end

LuxModule.registerclassfunction("getClip",
	[[([x,y,w,h]):(Skin2D) - Gets clipping for skin. If no value is returned,
	the clipping is disabled. ]])
function Skin2D:getClip() end

LuxModule.registerclassfunction("setClipping",
	[[():(Skin2D,on) - Enables or disables clipping. ]])
function Skin2D:setClipping(on) end

LuxModule.registerclassfunction("getClipping",
	[[(boolean):(Skin2D) - returns clipping value]])
function Skin2D:getClipping() end

LuxModule.registerclassfunction("delete",
	[[():(Skin2D) - deletes the skin. This function might get called multiple
	times so flag your skin once it got deleted. Per default it calls
	the deleteVisibles method. ]])
function Skin2D:delete () end

LuxModule.registerclassfunction("deleteVisibles",
	"():(Skin2D) - deletes resources for visual representation")
function Skin2D:deleteVisibles ()
	self:update(Skin2D.REASON.DELETEDVIS)
	if (self.icon) then self.icon:deleteVisibles() end
end

LuxModule.registerclassfunction("createVisibles",
	"():(Skin2D,l2dnode parent) - restores resources for visual representation")
function Skin2D:createVisibles (l2dparent)
	self.l2dparent = l2dparent
	self:update(Skin2D.REASON.CREATEDVIS)
	if (self.icon) then self.icon:createVisibles(l2dparent) end
end

LuxModule.registerclassfunction("setVisibility",
	[[():(Skin2D,boolean visible) - sets visibility of the skin. Won't create
	visibles, call createvisibles. Calls update function]])
function Skin2D:setVisibility (visible)
	self.visibility = visible
	if (self.icon) then self.icon:setVisibility(visible) end
	self:update(Skin2D.REASON.VISCHANGE)
end

LuxModule.registerclassfunction("selectSkin",
	[[():(Skin2D, skin) - A skin may have different visual appearances (surfaces).
	This method selects (activates) a skin. The argument should be a string,
	but it is up to the implementation if other types are being passed.

	GUI elements will pass specific strings. If a skin is created certain
	mappings should be provided. The setSkin method of the components contain
	more information on the mappings.]])
function Skin2D:selectSkin(skin)
	if (self.icon) then self.icon:selectIcon(self.iconselection or skin) end
	self.ptop,self.pright,self.pbottom ,self.pleft =
		unpack(self.paddings[skin or "default"] or {self.ptop,self.pright,self.pbottom ,self.pleft})
	self.currentskin = skin
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("selectSkin",
	[[(skin):(Skin2D) - returns currently selected skin]])
function Skin2D:getSelectedSKin ()
	return self.currentskin
end

LuxModule.registerclassfunction("setIconSkinSelection",
	[[():(Skin2D, [id]) - if a skiniconselection is present, it is preferred if
	the skin of the Skin2D object is changed, otherwise, the icon's selection is
	using the same name as the current skinname.
	]])
function Skin2D:setIconSkinSelection(skin)
	self.iconselection = skin
	self:selectSkin(self.currentskin)
end

LuxModule.registerclassfunction("getIconSkinSelection",
	[[([id]):(Skin2D) - returns skinselection ID]])
function Skin2D:getIconSkinSelection()
	return self.iconselection
end

LuxModule.registerclassfunction("getIconSkinSelection",
	[[([id]):(Skin2D) - returns current iconskinselection preference]])
function Skin2D:getIconSkinSelection()
	return self.iconselection
end

LuxModule.registerclassfunction("getSelectedSkin",
	[[(string):(Skin2D) - Returns the currently selected skin.]])
function Skin2D:getSelectedSkin(skin)
	return self.currentskin
end

LuxModule.registerclassfunction("toString",
	[[(string):(Skin2D) - returns simple text representation of the skin. It is
	called whenever the tostring function is called.]])
function Skin2D:toString ()
	return ("[Skin2D id=%s (%d,%d,%d,%d)]"):format(self.skincntid,self:getBounds())
end

LuxModule.registerclassfunction("contains",
	[[(boolean):(Skin2D, x,y) - returns true (default) if the skin contains a
	certain pixel in local coordinates. This method can be overloaded to
	let the mouse pointing reject transparent areas on the skin.]])
function Skin2D:contains (x,y) return true end

LuxModule.registerclassfunction("onDestroy",
	[[():(Skin2D) - called by garbagecollecter. Calls delete.]])
function Skin2D:onDestroy ()
	if (not self.destroyed) then
		self:delete()
		self.destroyed = true
	end
end

LuxModule.registerclassfunction("setSortID",
	[[(nextid):(Skin2D,int id) - sets sortid for l2d sorting. Returns the
	next id that can be used without interfering the l2delements of the
	skin. Per default it reserves 4 elements.]])
function Skin2D:setSortID(id)
	self.id = id
	if (self.icon) then self.icon:setSortID(id) end
	self:update(Skin2D.REASON.RESIZED)
	return id + 4
end

LuxModule.registerclassfunction("setZ",
	[[():(Skin2D,float z) - sets z offset in l2d system - 0 per default]])
function Skin2D:setZ (z)
	self.z = z
	if (self.icon) then self.icon:setZ(z) end
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setPaddingAll",
	"():(Skin2D,[float top],[right],[bottom],[left],boolean incremental=false) - "..
	"Sets padding of all padding keys. If nil is passed, the given value (top,right,bottom,left) is"..
	"not modified. If incremental is true, the values are added to the current values.")

function Skin2D:setPaddingAll(top,right,bottom,left,incremental)
	for i,v in pairs(self.paddings) do
		local t,r,b,l = unpack(v)
		if incremental then 
			t = t + (top or 0)
			l = l + (left or 0)
			r = r + (right or 0)
			b = b + (bottom or 0)
		else
			t = top or t
			l = left or l
			r = right or r
			b = bottom or b
		end
		self.paddings[i] = {t,r,b,l}
		if (i) == self.currentskin then
			self.ptop,self.pright,self.pbottom ,self.pleft = t,r,b,l
		end
	end
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setPadding",
	[[():(Skin2D,float top,right,bottom,left,[string/table key=default]) -
	padding of text and icon to the sides, sets default valueif the key is not
	present. The key is used when a different skin is selected. The key
	can be a table of keys that should be assigned with the given padding values. ]])
function Skin2D:setPadding(top,right,bottom,left, key)
	--self.ptop,self.pright,self.pbottom,self.pleft = top,right,bottom,left
	if type(key)~="table" then key = {key or "default"} end
	for i,key in ipairs(key) do
		self.paddings[key] = {top,right,bottom,left}
		if (key) == self.currentskin then
			self.ptop,self.pright,self.pbottom ,self.pleft = top,right,bottom,left
		end
	end
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("getPadding",
	[[(float top,right,bottom,left):(Skin2D) - returns current padding values]])
function Skin2D:getPadding()
	return self.ptop or 0,self.pright or 0,self.pbottom or 0,self.pleft or 0
end

LuxModule.registerclassfunction("setIconMargin",
	[[():(Skin2D,float margin) - margin arount the icon]])
function Skin2D:setIconMargin(margin)
	self.imargin = margin
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("getIconMargin",
	[[(float margin):(Skin2D) - returns margin arount the icon]])
function Skin2D:getIconMargin()
	return self.imargin or 0
end

LuxModule.registerclassfunction("copystylefrom",
	[[():(Skin2D to,Skin2D from) - copies style of another skin (padding and iconmargin)]])
function Skin2D:copystylefrom(other)
	self.ptop,self.pright,self.pbottom,self.pleft = other:getPadding()
	self.imargin = other:getIconMargin()
	self.iconalignhorizontal,self.iconalignvertical =
		other.iconalignhorizontal, other.iconalignvertical
	self.texthorizontal = other.texthorizontal
	self.textvertical = other.textvertical
	self.paddings = table.copy(other.paddings)
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setTextAlignment",
	[[():(Skin2D,string/number horizontal, vertical) - sets alignment for text. If number is passed
	it is used as coordinate from the top left corner.]])
	
function Skin2D:setTextAlignment(horizontal, vertical)
	self.textvertical = horizontal
	self.texthorizontal = vertical
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setIconAlignment",
	[[():(Skin2D,string/number horizontal, vertical) - sets alignment for icon. If number is passed
	it is used as coordinate from the top left corner.]])
function Skin2D:setIconAlignment(horizontal,vertical)
	self.iconalignhorizontal = horizontal
	self.iconalignvertical = vertical
	self:update(Skin2D.REASON.RESIZED)
end

LuxModule.registerclassfunction("setAutowidth",
	[[():(Skin2D, on) - Autowidth will change the *visual* appearance of the
	component to a width that is chosen depending on the current size of
	the caption and/or icon. ]])
function Skin2D:setAutowidth(on)
	self.autowidth = on
	self:update(Skin2D.REASON.RESIZED)
end
