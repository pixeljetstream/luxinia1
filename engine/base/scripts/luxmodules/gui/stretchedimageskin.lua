StretchedImageSkin = {}
setmetatable(StretchedImageSkin,{__index = Skin2D})

LuxModule.registerclass("gui","StretchedSkinnedImage",
	[[A skinned image is an image that uses a special texture to create a
	2D rectangle visual surface.

	The skin is created from 9 areas on the texture:

	 1 2 3
	 4 5 6
	 7 8 9

	The areas 1,3,7 and 9 (the corners) are not stretched, while
	2 and 8 are stretched horizontaly while 4 and 6 are stretched
	vertically. Area 5 is stretched in both dimensions, depending
	on the chosen dimension of the image.

	The SkinnedImage class can handle multiple verticaly aligned
	skins on one texture that can be displayed one per time.]],
	StretchedImageSkin,{},"Skin2D")


LuxModule.registerclassfunction("new",
	[[():(class,string/texture tex,float top,right,bottom,left, [table uvs]) -
	creates new object of given class and initializes the stretchedimageskin
	specific variables of this object.

	If the texture is a string, the texture is being loaded without compression.
	This has no effect if the image was loaded before with compression. This
	affects only configurations where compression is enabled.

	The 4 integer arguments top, right, bottom and left are the widths of
	the borders which are used in the way as the class description.

	The uvs are optional and can be used to specify regions on the texture to
	be used for a certain surface. The table must contain named arrays of
	coordinates, i.e. @@{default = {x,y,width,height}}@@ where the coordinates
	specify the regions to be used. Creating a single texture with multiple
	GUI components to be used is more efficient and easier to be created.
	The uvs must be named to provide different surfaces for instance, if assigned
	to a button, the button will ask for different surfaces if the button is
	clicked, hovered and so on. Look up the documentation on the components
	that are using skins for a detailed overview on the surfaces for skins.

	If no uvs are passed, the skin will stretch out for the whole image.
	If a requested surface in a skin is not specified in the uvs table, the
	whole image will be used also. Additionally the uvpositions can overload
	the margin widths. The format for the uvpositions is
	 {x, y, width, height, [top, left, right, bottom]}

	All coordinates are in absolute pixel coordinates.

	Per default, the filtering of the skin texture is deactivated if the
	size of the window is the same as the refsize for the drawn objects.
	Otherwise the filtering value is not touched. Be aware that the
	skins will look best if they are drawn in the same size as they really are.
	]])
function StretchedImageSkin.new (class, tex, top,right,bottom,left, uvpositions)
	local self = Skin2D.new(class)

	self.top,self.right,self.bottom,self.left = top,right,bottom,left
	self.uvpositions = uvpositions

	local com =rendersystem.texcompression()
	rendersystem.texcompression(false)
	self.texture = type(tex)=="string" and texture.load(tex,true,false)
		or tex
		
	assert(self.texture,"couldn't load texture or no texture was passed ("..tostring(tex)..")")

	local rw,rh = window.refsize()
	local ww,wh = window.width(), window.height()
	--if (rw==ww and rh==wh) then -- don't filter, the size of window is ok
	--	self.texture:filter(false,false)
	--else
		self.texture:filter(false,true)
	--end
	rendersystem.texcompression(com)


	self.imagew,self.imageh = self.texture:dimension()
	if (rendersystem.detail()<2) then
		self.imagew, self.imageh = self.imagew*2, self.imageh*2
	end

	return self
end

function StretchedImageSkin:clone ()
	assert(getmetatable(self).__index == StretchedImageSkin, "clone function must be overloaded")
	local copy = StretchedImageSkin:new(self.texture,self.top,self.right,self.bottom,self.left,table.copy(self.uvpositions))
	copy.id,copy.z = self.id,self.z
	copy:copystylefrom(self)
	return copy
end

function StretchedImageSkin:initLabel()
	if (self.labeltext and not self.label) then
		local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
		self.label = l2dtext.new("stretchedimagetext",self.labeltext,arialset)
		self.label.collectforgc = true
		
		self.label:font(arialtex)
		
		--self.label:sortid(self.id+1)
		if self.cx then
			self.label:scissor(true)
			self.label:scissorparent(true)
			self.label:scissorsize(self.cw,self.ch)
			self.label:scissorstart(self.cx,self.cy)
		end
	end
end

function StretchedImageSkin:validate (reason)
	if not self.l2d then
		return
	end

	if not self.visibility then
		self.l2d:rfNodraw(true)
		if (self.label) then self.label:rfNodraw(true) end
		return
	end

	local textwidth = 0
	self:initLabel()
	if (self.label) then
		local arialset,arialtex = UtilFunctions.loadFontFile (self.font or self.defaultfont)
		self.label:font(arialtex)
		self.label:fontset(arialset)
		
		self.label:parent(self.l2d)
		textwidth = self.label:dimensions()
		self.label:color(unpack(self.fontcolor))
		local w,h = self.label:font():dimension()
		local sw,sh = w/256,h/256
		textwidth = textwidth * sw * self.fontsize
		self.label:scale(sw*self.fontsize,sh*self.fontsize,1)
		
		
		--self.label = l2dtext.new("stretchedimagetext",self.labeltext,arialset)
		--self.label.collectforgc = true
		
		
	end

	if (self.icon) then self.icon:createVisibles(self.l2d) end

	--print(system.time(),self,self.label)
	self.l2d:rfNodraw(false)
	local x,y,w,h = self.bounds.x,self.bounds.y,self.bounds.w,self.bounds.h
	self.l2d:scissorsize(w,h)

	local autox = 0
	--self.l2d:pos(x,y,0)

	local ptop,pright,pbottom,pleft = 	self:getPadding()

	if (self.autowidth) then
		local autowidth = pleft + pright

		if (self.labeltext) then
			--local textwidth = #self.labeltext*self.fontsize*self.fontspacing
			autowidth = autowidth + textwidth
		end
		if (self.icon) then
			local ip,iw = self:getIconMargin(),self.icon:getWidth()
			autowidth = autowidth + ip*2 + iw
		end

		local fx =	self.texthorizontal or self.ALIGN.CENTER

		if fx == Skin2D.ALIGN.LEFT then
		elseif fx == Skin2D.ALIGN.RIGHT then autox = w-autowidth
		elseif fx == Skin2D.ALIGN.CENTER then autox = (w-autowidth)/2
		end

		w = autowidth
	end

	local ux,uy,uw,uh,t,r,b,l = unpack(self.uvpositions[self.currentskin] or self.uvpositions.default
		or self.uvpositions[1] or {0,0,self.imagew,self.imageh})


	local top,right,bottom,left	= t or self.top,r or self.right,b or self.bottom,l or self.left
	--local w,h = self.imagew,self.imageh
	--top,right,bottom,left = top / h, 1 - right / w, 1 - bottom / h, left / w
	do
		local right,bottom = w-right,h-bottom
		local top,left = top,left
		if left>right then
			left,right = (left+right)*.5,(left+right)*.5
			left = math.min(w,left)
			right = math.max(0,right)
		end
		if top>bottom then
			top,bottom = (top+bottom)*.5,(top+bottom)*.5

			top = math.min(h,top)
			bottom=math.max(0,bottom)
		end
		local pos = {
			{0,0},		{left,0},		{right,0},		{w,0},
			{0,top},	{left,top},		{right,top},	{w,top},
			{0,bottom},	{left,bottom},	{right,bottom},	{w,bottom},
			{0,h},		{left,h},		{right,h},		{w,h}
		}

		for i,v in ipairs(pos) do
			self.l2d:vertexPos(i-1,math.floor(v[1]+0.5)+autox,math.floor(v[2]+0.5),0)
		end
	end

	--self.l2d:sortid(self.id)
	self.l2d:color(unpack(self.skincolor))
	self.l2d:pos(math.floor(x),math.floor(y),self.z)
	local textleft,textright,texttop,textbottom = w/2,w/2,h/2,h/2
	local tw,th = 0,0



	if (self.label) then
		--self.label:scale(self.fontsize,self.fontsize,1)
		--self.label:spacing(self.fontspacing)
		
		-- not sure
		self.label:tabwidth(self.fonttabwidth)
		
		local x,y = 0,0
		local top,right,bottom,left	= top,right,bottom,left
		self.label:rfNodepthtest(true)

		local fx,fy = 	self.texthorizontal or self.ALIGN.CENTER,
						self.textvertical or self.ALIGN.CENTER
		local fw,fh = (self.fontspacing or 16)*(self.fontsize or 1),(self.fontsize or 1)*16
		local X,Y = x,y
		--tw,th = self.labeltext:l2dlen()*fw,self.labeltext:l2dheight()*fh
		tw,th = self.label:dimensions()
		local ftw,fth = self.label:font():dimension()
		tw = tw*ftw/256*self.fontsize
		th = th*fth/256*self.fontsize
		

		local ifx,ify = self.icon and (self.iconalignhorizontal or self.ALIGN.CENTER),
						self.icon and (self.iconalignvertical or self.ALIGN.CENTER)
		local ip = self:getIconMargin()
		local iw,ih = self.icon and self.icon:getWidth(),self.icon and self.icon:getHeight()

		if (type(fx) == "string") then
			local includeicon = ifx==self.ALIGN.ICONTOLEFT
			if 		fx == Skin2D.ALIGN.LEFT 	then X = x + pleft + (ifx==self.ALIGN.ICONTOLEFT and (ip*2+iw) or 0)
			elseif 	fx == Skin2D.ALIGN.CENTER 	then 
				X = x+pleft+((w-pleft-pright)-tw + (includeicon and (ip*2+iw) or 0))/2 --+ (includeicon and (ip*2+iw) or 0)
			elseif 	fx == Skin2D.ALIGN.RIGHT 	then
				X = autox + x + w - pright - tw - (ifx==self.ALIGN.ICONTORIGHT and (ip*2+iw) or 0)
			end
		else
			X = x + fx
		end

		if (type(fy) == "string") then
			if 		fy == Skin2D.ALIGN.TOP 		then Y = y+ptop
			elseif	fy == Skin2D.ALIGN.CENTER	then Y = y+(h-ptop-pbottom)/2-th/2+ptop
			elseif	fy == Skin2D.ALIGN.BOTTOM	then Y = y + h - th - pbottom end
		else
			Y = y + fy
		end
		--self.label:spacing(fw)
		--self.label:scale(fh,fh,1)
		self.label:text(self.labeltext or "")
		self.label:rfNodraw(false)
		self.label:sortid(self.id)

		self.label:scissor(Component.isClipped())
		self.label:scissorparent(true)
		self.label:scissorlocal(true)
		--self.l2d:scissorsize(
		--print(X,Y)
		self.label:pos(math.floor(X),math.floor(Y),self.z)
		textleft,textright,texttop,textbottom = X-x,X-x+tw,Y-y,Y-y+th
	end

	if (self.icon) then
		local iw,ih = self.icon:getSize()
		local ip = self:getIconMargin()
		local fx,fy = 	self.iconalignhorizontal or self.ALIGN.CENTER,
						self.iconalignvertical or self.ALIGN.CENTER
		local x,y = 0,0

		if 		fx == Skin2D.ALIGN.LEFT			then x = x + ip
		elseif	fx == Skin2D.ALIGN.RIGHT		then x = x + w - ip - iw
		elseif	fx == Skin2D.ALIGN.CENTER		then x = x + (textleft+(tw-iw)/2)
		elseif	fx == Skin2D.ALIGN.ICONTOLEFT	then x = x+ textleft-iw-ip
		elseif	fx == Skin2D.ALIGN.ICONTORIGHT	then x = x+ textright+ip
		else	x = x + fx end


		if 		fy == Skin2D.ALIGN.TOP			then y = y + ip
		elseif	fy == Skin2D.ALIGN.CENTER		then y = y + texttop+(th-ih)/2 + 1
		elseif	fy == Skin2D.ALIGN.BOTTOM		then y = y + h - ip - ih
		elseif	fy == Skin2D.ALIGN.ICONTOTOP	then y = y + texttop - ih - ip
		elseif	fy == Skin2D.ALIGN.ICONTOBOTTOM	then y = y + textbottom + ip
		else	y = y + fy end

		self.icon:setLocation(math.floor(x),math.floor(y))
		self.icon:setSortID(self.id + 2)
	end

	--self:setLabelFontSize(1)
	--self:setLabelFontSpacing(16)

	self:setClip(self.cx,self.cy,self.cw,self.ch)
	--self.currentskin = "default"
end

function StretchedImageSkin:getLabelSize()
	self:initLabel()
	if not self.label then 
		return 0,0 
	else
		if self.cache and self.cache.text == self.label:text() then
			return self.cache.tw,self.cache.th
		end
		self.cache = self.cache or {}
		self.cache.text = self.label:text()
		local tw,th = self.label:dimensions()
		local ftw,fth = self.label:font():dimension()
		tw = tw*ftw/256
		th = th*fth/256
		self.cache.tw,self.cache.th = tw,th
		return tw,th
	end
end

function StretchedImageSkin:delete ()
	if (self.l2d) then self.l2d:delete() self.l2d = nil end
	if (self.label) then self.label:delete() self.label = nil end
end

function StretchedImageSkin:selectSkin(skin)
	--if self.currentskin == skin then return end
	Skin2D.selectSkin(self,skin)
	self.currentskin = skin	or self.currentskin
	skin = self.currentskin

	if not self.uvpositions or not self.l2d then return end

	local ux,uy,uw,uh,t,r,b,l = unpack(self.uvpositions[skin] or self.uvpositions.default
		or self.uvpositions[1] or {0,0,self.imagew,self.imageh})
	ux,uy,uw,uh = ux/self.imagew, uy/self.imageh, uw/self.imagew, uh/self.imageh



	local top,right,bottom,left	= t or self.top,r or self.right,b or self.bottom,l or self.left
	local w,h = self.imagew,self.imageh

	top,right,bottom,left = top / h, uw - right / w, uh - bottom / h, left / w

	local pos = {
		{0,0},		{left,0},		{right,0},		{uw,0},
		{0,top},	{left,top},		{right,top},	{uw,top},
		{0,bottom},	{left,bottom},	{right,bottom},	{uw,bottom},
		{0,uh},		{left,uh},		{right,uh},		{uw,uh}
	}
	for i,v in ipairs(pos) do
		local x,y = ux+v[1],uy+v[2]
		self.l2d:vertexTexcoord(i-1,x,1-y,0)
	end
	self:update(Skin2D.REASON.RESIZED)
end

function StretchedImageSkin:createVisibles (l2dparent)
	Skin2D.createVisibles(self,l2dparent)
	if (not self.l2d) then
		self.l2d = l2dimage.new("skinned image",self.texture)
		self.l2d.collectforgc = true

		self.l2d:rfNodepthtest(true)
		self.l2d:rfNodepthmask(true)
		self.l2d:scissor(true)
		self.l2d:scissorparent(true)
		self.l2d:scissorlocal(true)
		local l2d = self.l2d
		l2d:parent(l2dparent)
		local indices = {
			0, 1, 5, 4,  1, 2, 6, 5,  2, 3, 7, 6,
			4, 5, 9, 8,  5, 6,10, 9,  6, 7,11,10,
			8, 9,13,12,  9,10,14,13, 10,11,15,14
		}

		l2d:usermesh(vertextype.vertex32normals(),16,table.getn(indices))
		l2d:indexCount(table.getn(indices))
		l2d:vertexCount(16)
		l2d:scale(1,1,1)
		l2d:indexPrimitivetype(primitivetype.quads())
		for i,v in ipairs(indices) do l2d:indexValue(i-1,v) end

		local top,right,bottom,left	= self.top,self.right,self.bottom,self.left
		local w,h = self.imagew,self.imageh
		top,right,bottom,left = top / h, 1 - right / w, 1 - bottom / h, left / w

		local pos = {
			{0,0},		{left,0},		{right,0},		{1,0},
			{0,top},	{left,top},		{right,top},	{1,top},
			{0,bottom},	{left,bottom},	{right,bottom},	{1,bottom},
			{0,1},		{left,1},		{right,1},		{1,1}
		}

		for i=0,15 do
			l2d:vertexPos(i,math.mod(i,4)/3,math.floor(i/4)/3,0)
			l2d:vertexTexcoord (i,pos[i+1][1],1-pos[i+1][2],0)
			l2d:vertexColor(i,1,1,1,1)
			l2d:vertexNormal(i,0,1,0)
		end
		l2d:indexMinmax()
		l2d:rfBlend(true)
		l2d:rsBlendmode(self.blendmode or blendmode.decal())
		l2d:rfNodraw(not self.visibility)
		l2d:color(unpack(self.skincolor))
		self:selectSkin(self.currentskin)
	end
	--Skin2D.createVisibles(self)
end

function StretchedImageSkin:setBlendmode(mode)
	self.blendmode = mode
	if self.l2d then self.l2d:rsBlendmode(self.blendmode or blendmode.decal()) end
end

function StretchedImageSkin:deleteVisibles ()
	Skin2D.deleteVisibles(self)
	if (self.l2d) then self.l2d:delete() self.l2d = nil end
	if (self.label) then self.label:delete() self.label = nil end
end

function StretchedImageSkin:setClip(x,y,w,h)
	self.cx,self.cy,self.cw,self.ch = x,y,w,h

	if (not x) then
		self.l2d:scissor(false)
		if (self.icon) then self.icon:setClip() end
		if (self.label) then self.label:scissor(false) end
	else
		if (self.icon) then self.icon:setClip(x,y,w,h) end
		self.l2d:scissor(true)
		self.l2d:scissorparent(true)
		self.l2d:scissorsize(w,h)
		self.l2d:scissorstart(x,y)
		if (self.label) then
			self.label:scissor(true)
			self.label:scissorsize(w,h)
			self.label:scissorstart(x,y)
		end
	end
end

function StretchedImageSkin:getClip()
	return self.cx,self.cy,self.cw,self.ch
end

function StretchedImageSkin:setLabelText (tx)
	Skin2D.setLabelText(self,tx)
	if tx == self.labeltext then return end
	if not tx or tx:len() == 0 then
		if (self.label) then
			self.label:delete()
			self.label = nil
			self.labeltext = nil
		end
	else
		self.labeltext = tx
		--if (not self.label and self.l2d) then
		--	self.label = l2dtext.new("stretchedimagetext",self.labeltext)
		if (self.label) then
			self.label:rfNodraw(not self.visibility)
			self.label:text(tx)
			self.label:scissor(true)
			self.label:scissorparent(true)
			self.label:scissorsize(self.bounds.w,self.bounds.h)
			if self.cx then
				self.label:scissor(true)
				self.label:scissorparent(true)
				self.label:scissorsize(self.cw,self.ch)
				self.label:scissorstart(self.cx,self.cy)
			end
		end
	end
	self:update(Skin2D.REASON.NEWTEXT)
end
