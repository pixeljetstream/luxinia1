TextField = {
	TEXTFIELD_ALIGNRIGHT=1,
	TEXTFIELD_ALIGNLEFT=2,
	TEXTFIELD_ALIGNCENTERED=3,

	skinnames = {
		pressedskin = "textfield_hover",
		hoveredskin = "textfield_hover",
		defaultskin = "textfield",
		focusedskin = "textfield_focus",
		focusedhoveredskin  = "textfield_focus",
	},

	disablekeybinds = true
}
setmetatable(TextField,{__index = Component})
LuxModule.registerclass("gui","TextField",
	[[A textfield element for usertextinput. Textfields have following skinsurfaces:
	* textfield
	* textfield_hover
	* textfield_focus
	* textfield_disabled
	]],TextField,
	{
		TEXTFIELD_ALIGNRIGHT = "[int] - constant for right text alignment",
		TEXTFIELD_ALIGNLEFT = "[int] - constant for left text alignment",
		TEXTFIELD_ALIGNCENTERED = "[int] - constant for centered text alignment",
		disablekeybinds = "[boolean]=true - any component having this key will disable the keybinding (managed by the container think function)"
	},"Component")


function TextField:transferFocusOnArrows()	return false end

TextField.nospaceaction = true

LuxModule.registerclassfunction("new",
	[[(TextField):(class, int x,y,w,h, align) - creates a textfield with the
	given dimensions. The align value can be a constant from the TextField
	table (TEXTFIELD_ALIGNRIGHT, ...).]])
function TextField.new (class, x,y,w,h, align,multiline)
	local self = Component.new(class,x,y,w,h)
	self:addKeyListener(
		KeyListener.new(function (kl,keyevent) self:onKeyTyped(keyevent) end, true)
	)

	self.dataTextField = {
		text = {},
		cursorpos = 0,
		selectionstarted = 0,
		align = align or TextField.TEXTFIELD_ALIGNLEFT,
		offset = 0,
		mousepressedframe = nil,
		selectioncolor = {.75,.75,1,1},
		cursorcolor = {0,0,0,1},
		textcolor = {0,0,0,1},
		multiline = multiline,
	}
	if multiline then self.noenteraction = true end

	self:setSkin("default")

	return self
end

LuxModule.registerclassfunction("onTextChanged",
	[[():(TextField) - called if the text changed (ie. by user input).]])
function TextField:onTextChanged()
end

function TextField:textchanged ()
	if self.l2d then
		self:positionUpdate(self.l2d.text:sortid(),self.clip)
	end
	self:onTextChanged()
end

LuxModule.registerclassfunction("isFocusable",
	[[(boolean):(TextField) - returns true if visible]])
function TextField:isFocusable() return true and self:isVisible() end

function TextField:copyText()
end

function TextField:getSelectionRange ()
	local from,to = self.dataTextField.selectionstarted, self.dataTextField.cursorpos
	return  math.max(0,math.min(to,from)),math.min(#self.dataTextField.text,math.max(to,from))
end

LuxModule.registerclassfunction("cutText",[[():(TextField) -
	Cut selected text in textfield into clipboard, deleting the selected text.]])
function TextField:cutText ()
	local from,to = self:getSelectionRange()
	local tx = self:getText(from,to)
	self.dataTextField.cursorpos = from
	self.dataTextField.selectionstarted = self.dataTextField.cursorpos
	self:replaceText(from+1,to)
	Clipboard.setText(tx)
end

LuxModule.registerclassfunction("copyText",[[():(TextField) -
	Copy selected text in textfield into clipboard.]])
function TextField:copyText ()
	local from,to = self:getSelectionRange()
	local tx = self:getText(from+1,to)
	Clipboard.setText(tx)
end

LuxModule.registerclassfunction("pasteText",[[():(TextField) -
	paste text from clipboard in textfield and overwrite the selection]])
function TextField:pasteText ()
	local from,to = self:getSelectionRange()
	local tx = Clipboard.getText()
	if not tx then return end
	self:replaceText(from,to,tx)
	self.dataTextField.cursorpos = math.min(from,to) + #tx
	self.dataTextField.selectionstarted = self.dataTextField.cursorpos
end

LuxModule.registerclassfunction("replaceText",
	[[(boolean):(TextField) - returns true if visible]])
function TextField:replaceText(from,to,tx)
	tx = tx or ""
	from,to = math.min(to,from),math.max(to,from)

	local ntext = {}
	do
		local cpos,txpos,opos = 1,1,1
		local text = self.dataTextField.text
		local tpos = 1
		while opos-to+from<=#text+#tx do
			if opos>from and tpos<=#tx then
				ntext[#ntext+1] = tx:sub(tpos,tpos)
				tpos = tpos + 1
			elseif opos<=from then
				ntext[#ntext+1] = text[opos]
				opos = opos+1
			else
				ntext[#ntext+1] = text[opos+to-from]
				opos = opos+1
			end
		end
	end
	--for i=from,to-1 do
--		table.remove(self.dataTextField.text,from+1)
--	end
--	for i=1,#tx do
--		table.insert(self.dataTextField.text,from+i,tx:sub(i,i))
--	end
	self.dataTextField.text = ntext
	self:textchanged()
	--self:invalidate()
end

LuxModule.registerclassfunction("onAction",
	[[():(TextField, String text) - called if the user pressed enter. Overload this to react on this]])
function TextField:onAction(text)
end

function TextField:getLineStartPos(line)
	local pos = 1
	local tx = self.dataTextField.text

	while line>0 and pos<=#tx do
		if tx[pos]=='\n' then line = line - 1 end
		pos = pos + 1
	end

	return pos-(line>0 and 0 or 1)
end

function TextField:getLineEndPos(line)
	return self:getLineStartPos(line+1)
end

function TextField:getCursorLine ()
	local pos = self.dataTextField.cursorpos
	local tx = self.dataTextField.text
	local line = 0
	for i=1,math.min(#tx,pos) do
		if tx[i] == '\n' then
			line = line + 1
		end
	end
	return line
end

LuxModule.registerclassfunction("setPasswordChar",
	[[():(TextField, [char]) - if a character is passed, this character is used as character
	for each typed character, like a passwordfield. to deactivate this, pass nothing]])
function TextField:setPasswordChar(char)
	self.passwordchar = char
end

LuxModule.registerclassfunction("onKeyTyped",
	[[():(TextField, KeyEvent) - called if a key was typed on the textfield.]])
function TextField:onKeyTyped(keyevent)
	local code = keyevent.keycode
	local ctrl = Keyboard.isKeyDown("LCTRL") or Keyboard.isKeyDown("RCTRL")
	local shift = (Keyboard.isKeyDown("RSHIFT") or Keyboard.isKeyDown("LSHIFT"))
--	_print(keyevent)

	local numpad = {
		[Keyboard.keycode.NUM0]	= "0",
		[Keyboard.keycode.NUM1]	= "1",
		[Keyboard.keycode.NUM2]	= "2",
		[Keyboard.keycode.NUM3]	= "3",
		[Keyboard.keycode.NUM4]	= "4",
		[Keyboard.keycode.NUM5]	= "5",
		[Keyboard.keycode.NUM6]	= "6",
		[Keyboard.keycode.NUM7]	= "7",
		[Keyboard.keycode.NUM8]	= "8",
		[Keyboard.keycode.NUM9]	= "9",
		[Keyboard.keycode.NUMPLUS] = "+",
		[Keyboard.keycode.NUMMINUS] = "-",
		[Keyboard.keycode.NUMMUL] = "*",
		[Keyboard.keycode.NUMDIV] = "/",
		[Keyboard.keycode.NUMCOMMA] = ",",
	}

	if code == Keyboard.keycode.ENTER and not self.dataTextField.multiline then
		self:onAction(self:getText())
	elseif ((code>0 and code<256 or numpad[code]) and not ctrl or (code == Keyboard.keycode.ENTER and self.dataTextField.multiline)) then
		local from,to = self:getSelectionRange()
		local key = numpad[code] or keyevent.key
		self:replaceText(from,to,keyevent.key)
		self.dataTextField.cursorpos = self.dataTextField.cursorpos+1
			--self:insertText(keyevent.key,self.dataTextField.cursorpos)
		self.dataTextField.selectionstarted = self.dataTextField.cursorpos
--		_print(self)
	elseif code==Keyboard.keycode.BACKSPACE or code==Keyboard.keycode.DEL then
		local from,to = self:getSelectionRange()
		if (from == to and code == Keyboard.keycode.DEL) then
			to = math.min(to + 1,#self.dataTextField.text)
		elseif (from == to) then
			from = math.max(0,from -1)
		end
		self:replaceText(from,to)
		self.dataTextField.cursorpos = from
		self.dataTextField.selectionstarted = self.dataTextField.cursorpos
	elseif ctrl then
		local pos = self.dataTextField.cursorpos
		local prev = self.dataTextField.cursorpos
		local n = table.getn(self.dataTextField.text)

		if code == Keyboard.keycode.C then self:copyText()
		elseif code == Keyboard.keycode.V then self:pasteText()
		elseif code == Keyboard.keycode.X then self:cutText()
		elseif code == Keyboard.keycode.LEFT then
			while pos>0 and self.dataTextField.text[pos]:byte()<=(" "):byte() do
				pos = pos - 1
			end
			while pos>0 and self.dataTextField.text[pos]:byte()>(" "):byte() do
				pos = pos - 1
			end
			self.dataTextField.cursorpos = math.max(0,pos)
		elseif code == Keyboard.keycode.RIGHT then
			pos = pos + 1
			while pos<n and self.dataTextField.text[pos]:byte()<=(" "):byte() do
				pos = pos + 1
			end
			while pos<n and self.dataTextField.text[pos]:byte()>(" "):byte() do
				pos = pos + 1
			end
			self.dataTextField.cursorpos = math.min(n,pos-1)
		end

		if not shift and prev~= self.dataTextField.cursorpos then
			self.dataTextField.selectionstarted = self.dataTextField.cursorpos
		end
	else
		local n = table.getn(self.dataTextField.text)
		local prev = self.dataTextField.cursorpos

		if (code == Keyboard.keycode.LEFT) then
			self.dataTextField.cursorpos =
				math.max(0,self.dataTextField.cursorpos - 1)
		elseif code == Keyboard.keycode.UP and self.dataTextField.multiline then
			local line = self:getCursorLine()
			local pos = self.dataTextField.cursorpos

			if line>0 then
				local linestart = self:getLineStartPos(line)
				local plinestart = self:getLineStartPos(line-1)
				local off = math.min(linestart-plinestart-1,pos-linestart)
				pos = plinestart + off

				self.dataTextField.cursorpos = math.max(0,pos)
			end
		elseif code == Keyboard.keycode.DOWN and self.dataTextField.multiline then
			local line = self:getCursorLine()
			local pos = self.dataTextField.cursorpos

			local linestart = self:getLineStartPos(line)
			local nlinestart = self:getLineStartPos(line+1)
			local nlineend = self:getLineStartPos(line+2)
			local off = math.min(nlineend-nlinestart-1,pos-linestart)
			pos = nlinestart + off

			self.dataTextField.cursorpos = math.max(0,pos)

		elseif (code == Keyboard.keycode.RIGHT) then
			self.dataTextField.cursorpos =
				math.min(n,self.dataTextField.cursorpos + 1)
		elseif (code == Keyboard.keycode.HOME) then
			self.dataTextField.cursorpos = self:getLineStartPos(self:getCursorLine())
		elseif (code == Keyboard.keycode.END) then
			self.dataTextField.cursorpos = self:getLineEndPos(self:getCursorLine())-1
		end

		if not shift and
			prev~= self.dataTextField.cursorpos then
			self.dataTextField.selectionstarted = self.dataTextField.cursorpos
		end
	end
	self:positionUpdate(self.l2d.text:sortid(),self.clip)
end

LuxModule.registerclassfunction("insertText",
	[[(int):(TextField, string tx, int at) - inserts the text at the specified
	index. Returns the index at which the insertion stoped.]])
function TextField:insertText(tx, at)
	local text = self.dataTextField.text
	local n = table.getn(text)
	at = at or 1
	at = at<1 and 0 or at>n and n or at
	for i=1,string.len(tx) do
		local chr = string.sub(tx,i,i)
		if (chr=='\b') then
			at = at - 1
			if (i+at>0) then
				table.remove(text,at+i)
				at = at - 1
			end
		elseif(string.byte(chr)==0 or string.byte(chr)>255) then
			at = at - 1
		else
			table.insert(text,at+i,chr)
		end
--			_print(at,i)
	end
	self:textchanged()
	return math.max(0,math.min(table.getn(text),string.len(tx) + at))
end

LuxModule.registerclassfunction("getText",
	[[(string):(TextField,[from,[to] ]) - returns the text of the textfield, if
	bounds are given it returns the text within that range]])
function TextField:getText(from,to)
	if (from and to and from>to or not self.dataTextField) then return "" end
	return table.concat(self.dataTextField.text):sub(from or 1,to or #self.dataTextField.text)
end

LuxModule.registerclassfunction("setText",
	[[():(TextField,string,[boolean donnotfireevent]) - sets text of the textfield,
	causes onTextChanged call. If donnotfireevent is true, the onTextChanged
	function is not called.]])
function TextField:setText(str, donotfireevent)
	self.dataTextField.text = {}
	for i=1,str:len() do
		table.insert(self.dataTextField.text,str:sub(i,i))
	end
	self.dataTextField.cursorpos = math.min(self.dataTextField.cursorpos,str:len())
	self.dataTextField.selectionstarted =
		math.min(self.dataTextField.selectionstarted,str:len())
	if donotfireevent then
		if self.l2d then self:positionUpdate(self.l2d.text:sortid(),self.clip) end
	else
		self:textchanged()
	end
	--else self:invalidate() end
end

LuxModule.registerclassfunction("toString",
	[[(string):(TextField) - returns simple component representation]])
function TextField:toString ()
	return string.format("[TextField cursor=%i text=%s]",self.dataTextField and self.dataTextField.cursorpos or 0,self:getText())
end

--------------------------------------------------------------------------------
LuxModule.registerclassfunction("deleteVisibles", "@overloaded Component")
function TextField:deleteVisibles ()
	if (not self.l2d) then return end
	for a,b in pairs(self.l2d) do
		b:delete()
	end
	self.l2d = nil
end

LuxModule.registerclassfunction("createVisibles", "@overloaded Component")
function TextField:createVisibles ()
	if (self.l2d) then return end

	self.l2d = {}

	--self.l2d.border1 = l2dimage.new("textbutton line",self.white)
	--self.l2d.border2 = l2dimage.new("textbutton line",self.white)
	--self.l2d.textbackground = l2dimage.new("textbutton background",self.white)

	self.l2d.textbase = l2dnode.new("textnode")

	self.l2d.cursor = l2dimage.new("textbutton cursor",self.white)
	self.l2d.cursor:rfBlend(true)
	self.l2d.cursor:rsBlendmode(blendmode.decal())
	self.l2d.cursor:rfNodraw(true)
	self.l2d.cursor.collectforgc = true

	--self.l2d.border1:rfWireframe(true)
	--self.l2d.border2:rfWireframe(true)
	local set,tex = UtilFunctions.loadFontFile(self.font or self.defaultfont)

	self.l2d.text = l2dtext.new("textbutton caption",self:getText(),set)
	self.l2d.text.collectforgc = true
	self.l2d.text:font(tex)
	local tw,th = tex:dimension()

	self.l2d.text:scale(tw/256,th/256,1)

	self.l2d.selection = l2dimage.new("textbutton selection",self.white)

	self.l2d.selection:parent(self.l2d.textbase)
	self.l2d.cursor:parent(self.l2d.textbase)
	self.l2d.text:parent(self.l2d.textbase)
	self.l2d.textbase:parent(self.basel2d)

	local vis = not self:isVisible()
	for a,b in pairs(self.l2d) do
		b:rfNodraw(vis)
		--b:parent(self.basel2d)
		b:scissorparent(true)
	end
end

LuxModule.registerclassfunction("showVisibles", "@overloaded Component")
function TextField:showVisibles ()
	for a,b in pairs(self.l2d) do b:rfNodraw(false) end
end

LuxModule.registerclassfunction("hideVisibles", "@overloaded Component")
function TextField:hideVisibles ()
	for a,b in pairs(self.l2d) do b:rfNodraw(true) end
end

LuxModule.registerclassfunction("positionUpdate","@overloaded Component")
function TextField:positionUpdate(z,clip)
	if (not self.l2d) then return z end
	local ptop,pright,pbottom,pleft = 0,0,0,0
	if (self.skin) then ptop,pright,pbottom,pleft = self.skin:getPadding() end

	local x,y,w,h = 0,0,self.bounds[3],self.bounds[4]
	local ptop,pright,pbottom,pleft = 0,0,0,0
	if (self.skin) then ptop,pright,pbottom,pleft = self.skin:getPadding() end

	z = z or self.zsortid

	h = h - ptop - pbottom
	local text = self:getText()
	local cursorpos = self.dataTextField.cursorpos
	local selectionstart = self.dataTextField.selectionstarted

	if self.passwordchar then
		self.l2d.text:text(self.passwordchar:rep(#text))
	else
		self.l2d.text:text(text)
	end
	self.l2d.text:pos(pleft,ptop,0)
	local sx,sy = self:textpos2xy(math.min(selectionstart,cursorpos))
	local tx,ty = self:textpos2xy(math.max(selectionstart,cursorpos))
	local cx,cy
	if selectionstart >= cursorpos then cx,cy = sx,sy else cx,cy = tx,ty end

	local pox = self.dataTextField.offset or 0
	local ox = -math.max(0,cx-w+pright+2)
	--print(cx,pox,w)
	ox = math.min(pox + math.max(-pox - cx + pleft + 2,0),ox)
	self.l2d.text:pos(ox,0,0)
	if self.dataTextField.multiline then
		local fh = self.l2d.text:fontset():linespacing()
		self.l2d.cursor:pos(cx-pleft+ox-1,ptop+cy-fh+2,0)
		self.l2d.cursor:scale(1,fh,1)

		self.l2d.selection:pos(sx-pleft+ox-1,0,0)
		self.l2d.selection:scale(math.max(1,tx-sx),h-pbottom,1)
	else
		self.l2d.cursor:pos(cx-pleft+ox-1,0,0)
		self.l2d.cursor:scale(1,h-pbottom,1)

		self.l2d.selection:pos(sx-pleft+ox-1,0,0)
		self.l2d.selection:scale(math.max(1,tx-sx),h-pbottom,1)
	end


	self.l2d.textbase:pos(pleft,ptop,0)
	self.l2d.textbase:scissorstart(-1,0)
	self.l2d.textbase:scissorsize(w-pleft-pright,h-pbottom)
	self.l2d.textbase:scissorlocal(true)
	self.l2d.textbase:scissor(true)

	--local colortab = self:hasFocus() and self.focuscolors or self.colors

	self.l2d.cursor:color(unpack(self.dataTextField.cursorcolor))
	self.l2d.selection:color(unpack(self.dataTextField.selectioncolor))
	self.l2d.text:color(unpack(self.dataTextField.textcolor))

	self.dataTextField.offset = ox

	local vis = not self:isVisible()

	for a,b in pairs(self.l2d) do
	--	local c = colortab[a]
	--
		b:rfNodraw(vis)
	--
	--	if (type(c)=="table") then
	--		b:color(unpack(c))
	--	end
	--	if (type(c)=="function") then
	--		b:color(c(self))
	--	end
	end
	
	if vis then self.l2d.selection:rfNodraw(selectionstart==cursorpos) end

	for a,b in pairs(self.l2d or {}) do
		b:sortid(z)
	end

	return z + 1
end

LuxModule.registerclassfunction("getTextLength","(int length):(TextField) - returns number of chars")
function TextField:getTextLength()
	return #self.dataTextField.text
end

function TextField:textpos2xy (x)
	local pleft,ptop = self.l2d.text:pos()
	local font = self.l2d.text:fontset()
	local x,y = self.l2d.text:posatchar(x-1)
	local sx,sy = self.l2d.text:scale()
	x,y = sx*x,sy*y
	x = x + pleft
	y = y + ptop - font:linespacing()
	return x,y
end

LuxModule.registerclassfunction("getCursorCaretPosAtXY",[[(x):(TextField,x) -
	converts relative pixel coordinate in caret position ]])
function TextField:getCursorCaretPosAtXY(x,y)
	local pleft,ptop = self.l2d.text:pos()
	local font = self.l2d.text:fontset()
	x,y = x-1-pleft,y
	local sx,sy = self.l2d.text:scale()
	x,y = x/sx,y/sy
	local suc,x = self.l2d.text:charatpos(x-1-pleft,y)
--	print(x)
	return x
end

LuxModule.registerclassfunction("setCursorCaretPosAtXY",[[():(TextField,x,boolean ignoreselectionstartset) -
	sets caret position to local pixel coordinate. If a fourth argument is passed and
	is true, the selectionstart is not repositioned.]])
function TextField:setCursorCaretAtXY(x,y,ignorestart)
	self.dataTextField.cursorpos = self:getCursorCaretPosAtXY(x,y)
	if (not ignorestart) then
		self.dataTextField.selectionstarted = self.dataTextField.cursorpos
	end

	--self:invalidate()
end

LuxModule.registerclassfunction("mousePressed","@overloaded Component")
function TextField:mousePressed(me)
	Component.mousePressed(self,me)
	self:lockMouse()
	self.dataTextField.pressed = true
	self.dataTextField.mouseover = true
	self.dataTextField.pressedSince = me.downsince
	self.dataTextField.pressedMouseButton = me.button
	self:setCursorCaretAtXY(me.x,me.y,Keyboard.isKeyDown("LSHIFT") or Keyboard.isKeyDown("RSHIFT"))
	self:positionUpdate()
end

function TextField:mouseMoved(me)
	Component.mouseMoved(self,me)
	if me:isDragged() then
		self.dataTextField.selectionstarted = self:getCursorCaretPosAtXY(me.x,me.y)
		self:invalidate()
	end
end

LuxModule.registerclassfunction("mouseReleased","@overloaded Component")
function TextField:mouseReleased(me)
	Component.mouseReleased(self,me)
	if (self.dataTextField.pressedSince == me.downsince) then
		self:focus()
		self.dataTextField.pressed = false
		self.dataTextField.pressedSince = nil
		self.dataTextField.pressedMouseButton = nil
	end
	self:positionUpdate()
	self:unlockMouse()
end

function TextField:onGainedFocus()
	Component.onGainedFocus(self)

	TimerTask.new(function()
		if not self.l2d or not self.l2d.cursor then return end
		while self:hasFocus() and self.l2d do
			local r,g,b,a = unpack(self.dataTextField.cursorcolor)
			local t = math.sin(os.clock()*5)^2
			self.l2d.cursor:color(r,g,b,a*t)
			coroutine.yield(100)
		end
		if not self.l2d or not self.l2d.cursor then return end
		local r,g,b,a = unpack(self.dataTextField.cursorcolor)
		
		self.l2d.cursor:color(r,g,b,0)
	end,100)
end

function TextField:onLostFocus()

end

function TextField:mouseEnter(me)
	Component.mouseEnter(self,me)
	self.mouseover = true
	self:positionUpdate()
end

function TextField:mouseExit(me)
	Component.mouseExit(self,me)
	self.mouseover = false
	self:positionUpdate()
end
