GH = {}
GH.sldLabelwidth = 64
GH.sldLabelval = 16

GuiHelpers = GH

LuxModule.registerclass("gui","GuiHelpers",
	[[GuiHelpers contain a few functions to aid application development. Can be accessed via GH as well.
	]],GH,
	{
		sldLabelwidth = "[int] - width of description label for addslider",
		sldLabelval = "[int] - default width for value label for addlsider",
	})

local function checkextension(name,extensions)
	for i,str in ipairs(extensions) do 
		if name:match("%."..str.."$") then
	   		return str
		end 
	end
	return nil
end
LuxModule.registerclassfunction("modaldialog",
	[[(TitleFrame):([titlename], [int x,y,w,h], [int fw,fh], [boolean nomodal]) -
	creates a modal titleframe on top of everything, and normally darkens the background (unless nomodal is true). All arguments are optional. x,y,w,h default to 0,0 and window size.
	fw/fh are the dimensions of the TitleFrame created. Returns the empty titleframe, mostly used for pop-up menus.]])
function GH.modaldialog(title,x,y,w,h,fw,fh,nomodal)
	local ww,wh = window.refsize()
	local modal 
	
	if (not nomodal) then
		modal = Container.getRootContainer():add(Container:new( 0,0,ww,wh),1)
		ww,wh = modal:getWidth(),modal:getHeight()
		imgc = modal:add(ImageComponent:new(0,0,ww,wh,matsurface.vertexcolor(),nil,blendmode.modulate()))
		imgc:color(0.5,0.5,0.5,1)
		function modal:onParentLayout(x,y,w,h)
			modal:setSize(w,h)
			imgc:setSize(w,h)
		end
		function modal:doLayout()
			local x,y,w,h = unpack(self:getBounds())
			for i,c in ipairs(self.components) do
				if (c.onParentLayout) then
					c:onParentLayout(x,y,w,h)
				end
			end
		end
	end
	
	local parentframe = modal or Container.getRootContainer()
	
	fh = fh or 300
	fw = fw or 440
	local frame = parentframe:add(TitleFrame:new(x or ((ww-fw)/2),y or ((wh-fh)/2),w or fw,h or fh,nil,title or nil),1)
	frame.modal = modal
	
	local order = Component.newFocusOrder({})
	
	function frame:delete()
		GroupFrame.delete(self)
		if (frame.modal) then frame.modal:delete() end
		Component.newFocusOrder(order)
	end

	return frame
end

LuxModule.registerclassfunction("popupYesNo",
	[[(GroupFrame):(int w,h, string title, question, function fnyes, [fnno], [string yes], [no], [int buttonwidth]) -
	creates a modal popup with description label string, and two buttons for "yes" and "no".]])
function GH.popupYesNo(w,h,title,question,fnyes,fnno,yesstr,nostr,btnw)
	-- popup for query on 
	w = w or 250
	h = h or 140
	local btnoff = 4
	local btnheight = 32
	
	local lblh = h-20-btnoff-btnheight-20
	local modframe = GH.modaldialog(title or "",nil,nil,nil,nil,w,h)
	modframe:add(Label:new(20,20,w-40,lblh,question or "Really?",Label.LABEL_ALIGNCENTERED))
	
	local bw = (w-20)/2
	local mw = btnw or 70
	local mwh = (bw-mw)/2
	
	local btnyes 	= modframe:add(Button:new(10+mwh,20+lblh+btnoff,mw,32,yesstr or "Yes"))
	local btnno		= modframe:add(Button:new(10+bw+mwh,20+lblh+btnoff,mw,32,nostr or "No"))
	
	function modframe:onParentLayout(x,y,w,h)
		local mw,mh = self:getSize()
		self:setLocation(math.floor((w-mw)/2),math.floor((h-mh)/2))
	end
	
	function btnyes.onClicked(bt)
		fnyes()
		modframe:delete()
	end
	
	function btnno.onClicked(bt)
		if (fnno) then fnno() end
		modframe:delete()
	end
end

LuxModule.registerclassfunction("popupInfo",
	[[(GroupFrame):(int w,h, string title, text, [function fnok], [string ok], [int buttonwidth]) -
	creates a modal popup with a description label string, and a "okay" button.]])
function GH.popupInfo(w,h,title,text,fnyes,yesstr,btnw)
	-- popup for query on 
	w = w or 250
	h = h or 140
	local btnoff = 4
	local btnheight = 32
	
	local lblh = h-20-btnoff-btnheight-20
	local modframe = GH.modaldialog(title or "",nil,nil,nil,nil,w,h)
	modframe:add(Label:new(20,20,w-40,lblh,text or "",Label.LABEL_ALIGNCENTERED))
	
	local bw = (w)/2
	local mw = btnw or 70
	local mwh = (bw-(mw/2))
	
	local btnyes 	= modframe:add(Button:new(mwh,20+lblh+btnoff,mw,32,yesstr or "Okay"))

	function modframe:onParentLayout(x,y,w,h)
		local mw,mh = self:getSize()
		self:setLocation(math.floor((w-mw)/2),math.floor((h-mh)/2))
	end
	
	function btnyes.onClicked(bt)
		if (fnyes) then fnyes() end
		modframe:delete()
	end
	

end

LuxModule.registerclassfunction("scrollfield",
	[[(GroupFrame):(int x,y,w,h) -
	creates and returns groupframe, which gets a vertically scroll slider if content is out of bounds. ".area" of returned frame should be used to add components to. ".area" also contains the function "scrollto(itemY,itemHeight)" with which you can set the scroll pos (if slider exists, otherwise ignored).]])
function GH.scrollfield (x,y,w,h)
	local frame = GroupFrame:new(x,y,w,h)
	frame.skinnames = TextField.skinnames
	frame:getSkin():selectSkin("textfield")

	local scrollbar = frame:add(Slider:new(
		frame:getWidth()-16,0,16,frame:getHeight(),true))
	local view = Container:new(5,4,frame:getWidth()-22,frame:getHeight()-7)
	frame:add(view)
	frame.area = view:add(Container:new(0,0,0,0))
	
	local function updatesize ()
		local self = frame.area
		local w,h = frame.area:getSize()

		local h2 = view:getHeight()
		local n = math.max(h-h2,0)/8
		if n==0 then
			scrollbar:setSliderPos(0)
			scrollbar:setVisibility(false)
		else
			scrollbar:setVisibility(true)
			scrollbar:setSliderPos(0)
			if 1/n>0 and 1/n<1 then
				scrollbar:setIncrement(1/n)
			end
		end

	end
	
	function frame:doLayout()
		view:setSize(frame:getWidth()-22,frame:getHeight()-7)
		scrollbar:setBounds(frame:getWidth()-16,0,16,frame:getHeight())
		updatesize()
	end
	
	function frame.area:doLayout()
		if (not frame.area.nolayout) then
			updatesize()
		end
	end
	function frame:mouseWheeled(me)
		scrollbar:mouseWheeled(me)
	end
	function scrollbar:onValueChanged(value)
		local h = view:getHeight()
		local H = frame.area:getHeight()
		
		frame.area.nolayout = true
		value = value*math.max(0,H-h)
		frame.area:setLocation(0,-value)
		frame.area.nolayout = nil
	end
	
	function frame.area:scrollto(start,size)
		local H = frame.area:getHeight()
		size = size or 0
		start = (start+size) > H/2 and (start+size) or start
		scrollbar:setSliderPos(math.max(math.min(start/H,1),0))
	end
	
	return frame
end

LuxModule.registerclassfunction("filedialog",
	[[(GroupFrame):(string title,oktext,canceltext,[initpath],[fn onsave], [oncancel], [table strings extensions], [string startname], [fn addedframe], [table warnonoverwrite]) -
	creates and file dialog as popup menu.<br>
	Functions: onsave(fileframe,filepath,filetext), oncancel(fileframe). Both must return true if dialog should be destroyed.<br>
	addedframe(fileframe) allows to add custom frame to the modal dialog which must be returned by the function.<br><br>
	warnonoverwrite will cause yes/no popup and can contain <br>
	* width: int <br>
	* height: int <br>
	* title: string <br>
	* descr: string <br>
	* yes: string <br>
	* no: string <br>]])
function GH.filedialog(title,oktext,canceltext,initpath,onsave,oncancel,extensions,startname,addedframefunc,warn)
	GH.guipath = ""
	local frame = GH.modaldialog(title)
	--frame:add(Label:new(0,2,frame:getWidth(),20,"\v999\vb"..title,Label.LABEL_ALIGNCENTERED),
	--	Label.LABEL_ALIGNCENTERED)

	local cancel = frame:add(Button:new(7,frame:getHeight()-32,100,26,canceltext))
	function cancel:onClicked()
		if not oncancel or oncancel(frame) then frame:delete() end
	end

	--local path = frame:add(Label:new(
	--8,40,frame:getWidth()-10,22))

	local chose = frame:add(ComboBox:new(7,30,frame:getWidth()-12,24))
	

	local dirs = frame:add(GH.scrollfield(7,60,180,frame:getHeight()-150))

	local files = frame:add(GH.scrollfield(187,60,frame:getWidth()-187-6,frame:getHeight()-150))

	local lblpath = frame:add(Label:new(10,frame:getHeight()-60,frame:getWidth()-16,20,initpath or ""))
	
	local filename = frame:add(TextField:new(
		6,frame:getHeight()-80,frame:getWidth()-10,22,""))
		
	
	local ok = frame:add(Button:new(frame:getWidth()-100-5,frame:getHeight()-32,100,26,oktext))
	function ok:onClicked()
		local fname = filename:getText()
		if (warn and FileSystem.attributes(GH.guipath..fname)) then
			-- popup for query on 
			GH.popupYesNo(warn.width or 250,warn.height or 140,warn.title or "File exists", warn.descr or "Do you really want\n to overwrite the file?",
				function()
					if(onsave(frame,GH.guipath,fname)) then
						frame:delete()
					end
				end,
				nil,
				warn.yes, warn.no)
		elseif onsave(frame,GH.guipath,fname) then
			frame:delete()
		end
	end
	
	if (addedframefunc) then
		local addframe = addedframefunc(frame)
		frame.modal:add(addframe,1)
		-- recenter
		local ww,wh = window.refsize()
		local fbounds = frame:getBounds()
		local abounds = addframe:getBounds()
		
		-- get merged rect
		local mrect = Rectangle:merged(fbounds,abounds)
		-- repos to center
		local mhx,mhy = mrect[1]+mrect[3]/2, mrect[2]+mrect[4]/2
		-- deltas to screen center
		mhx,mhy = ww/2 - mhx, wh/2 - mhy
		
		frame:setLocation(fbounds[1]+mhx,fbounds[2]+mhy)
		addframe:setLocation(abounds[1]+mhx,abounds[2]+mhy)
	end
	
	
	--local btnpath = frame:add(Button:new((frame:getWidth()-100)/2,frame:getHeight()-32,100,26,"set projectpath"))
	--function btnpath:onClicked()
	--	system.projectpath(GH.guipath)
	--	projectpath:setText("projectpath: "..GH.guipath)
	--end

	function frame.updatepath(p)
		frame.lastclicked = nil
		
		p = p:gsub("/$","")
		p = (p:gsub("\\","/").."/"):gsub("//","/"):gsub("/%./","/")
		p = p:gsub("/[^/]-/%.%.","")
		local curdrive
		
		GH.guipath = p
		chose:clearItems()
		function add (p)
			if p and p:match("/") then
				curdrive = p
				chose:addItem(p,nil,defaulticon16x16set,path == p and "openfolder" or "folder")
				return add(p:match("^(.*/).*/[^/]*$"))
			end
		end
		add(p)
		
		
		GH.guidrives = {system.drivelist()}
		for i,v in pairs (GH.guidrives) do		
			v = v:gsub("\\+","/") 
			GH.guidrives[i]= v
			
			if (v ~= curdrive) then
			chose:addItem(v,nil,defaulticon16x16set,
				"folder")
			end
		end
		--path:setText(p)

		local cnt = 0
		local dirlist,filelist = {},{}
		for name in FileSystem.dir(p) do
			if FileSystem.attributes(p..name,"mode")=="directory" then
				table.insert(dirlist,name)
			else
				local supported = not (extensions~=nil)
				if (extensions) then
					supported = checkextension(name,extensions)
				end
				if (supported) then
					table.insert(filelist,name)
				end
			end
		end

		local function addbtn (i,on,title,icon)
			local btn = Button:new(0,i*20,on:getWidth(),20,title)
			btn.skinnames = Label.skinnames
			btn:getSkin():setLabelPosition(Skin2D.ALIGN.LEFT)
			btn:getSkin():setSkinColor(1,1,1,0)
			btn:getSkin():setIcon(defaulticon16x16set:clone())
			btn:getSkin():setIconSkinSelection(icon or "folder")
			function btn:mouseEntered (me)
				Button.mouseEntered(self,me)
				if (btn ~= frame.lastclicked) then
					btn:getSkin():setSkinColor(0.5,0.5,1,0.5)
				end
			end
			function btn:mouseExited (me)
				Button.mouseExited(self,me)
				if (btn ~= frame.lastclicked) then
					btn:getSkin():setSkinColor(1,1,1,0)
				end
			end
			return on:add(btn)
		end

		dirs.area:deleteChilds()
		dirs.area:setSize(dirs.area.parent:getWidth(),#dirlist*20)

		-- DIRECTORY
		table.sort(dirlist,function (a,b) return a:lower()<b:lower() end)
		for i,name in ipairs(dirlist) do
			local bt = addbtn(i-1,dirs.area,name)
			bt.onClicked = function (b,state,wasmouse,wasdouble)
				if(wasdouble) then
					frame.updatepath(p..name)
				end
			end
		end

		-- FILES
		files.area:deleteChilds()
		files.area:setSize(files.area.parent:getWidth(),#filelist*20)
		table.sort(filelist,function (a,b) return a:lower()<b:lower() end)
		for i,name in ipairs(filelist) do
			local icon = ({
					lua = "filelua",
					f3d = "filef3d",
					shd = "fileshd",
					jpg = "fileimg",
					tga = "fileimg",
					png = "fileimg",
					dds = "fileimg",
					mtl = "filemtl",
					prt = "fileprt",
					mma = "filemma",
					mm  = "filef3d",
					txt = "filetxt",
					exe = "fileexe",
					dll = "filedll",
					lnk = "filelnk",
					wav = "filewav"})
				[name:match("%.(%P+)$")] or "file"
			addbtn(i-1,files.area,name,icon).onClicked = function (b,state,wasmouse,wasdouble)
				GH.guifile = name
				filename:setText(name)
				lblpath:setText(p)
				
				-- colorize self
				b:getSkin():setSkinColor(1,1,0.5,1)
				if (frame.lastclicked and frame.lastclicked ~= b) then
					-- revert color old
					frame.lastclicked:getSkin():setSkinColor(1,1,1,0)
				elseif (frame.lastclicked and wasdouble) then
					
					-- double click
					return ok:onClicked()
				end
				
				frame.lastclicked = b
			end
		end
	end
	frame.updatepath(initpath)
	
	filename:setText(startname or "")
	
	function chose:onSelected(idx,cap,fn) 
		if cap and cap~=GH.guipath then 
			current = cap
			frame.updatepath(cap) 
		end 
	end
end

local huetex = texture.load("colorhue.tga")
local guitexgrey = texture.load("colorgradientgrey.tga")

LuxModule.registerclassfunction("createcolormenu",
	[[(Container):(int x,y,w,h, string title, table colorHSVA, fn onchange, [upvalue], [string alphaname]) -
	creates a container with a graphical color menu. Initilized with the table value.<br>
	Function: onchange(upvalue,r,g,b,alpha)]])
function GH.createcolormenu(x,y,w,h,title, colortab, fn,obj,alphaname)
	local maxint = 8
	local intsteps = 32
	local frame= TitleFrame:new(x,y,w,h,nil,title)
	frame:add(ContainerMover:new(0,0,w,26))
	local a = colortab[4] or 1
	w,h = w-20,h-25
	local self = frame:add(Container:new(10,25,w,h))
	local hue = self:add(ImageComponent:new(0,0,w,10,huetex))
	local colorhue = hue
	
	--frame:add(Label:new(10,2,w-10,20,title))

	local colorfield			
	-- saturation and brightness
	local x,y,w,h = hue:getX(),hue:getY() + hue:getHeight() + 2,
		hue:getWidth(), h - hue:getY() - hue:getHeight() - (20)
	local texmatrix = matrix4x4.new()
	texmatrix:rotdeg(0,0,90)
	local colorbrtb = self:add(ImageComponent:new(x,y,w,h,guitexgrey,nil,blendmode.modulate(),texmatrix))
	local colorsatb = self:add(ImageComponent:new(x,y,w,h,guitexgrey,nil,blendmode.add()))
	local colorsbc = self:add(ImageComponent:new(x,y,w,h,matsurface.vertexcolor()))

	local colorhbar	= self:add(ImageComponent:new(100,10,2,hue:getHeight(),matsurface.vertexcolor()),1)
	local colorsbs = self:add(ImageComponent:new(100,60,2,h,matsurface.vertexcolor()),1)
	local colorsbb = self:add(ImageComponent:new(100,60,w,2,matsurface.vertexcolor()),1)

	function colorsbs:contains(x,y) return false end
	function colorsbb:contains(x,y)	return false end
	function colorhbar:contains(x,y) return false end

	
	
	local alpha = self:add(Label:new(0,self:getHeight()-18,75,16,alphaname or "Alpha: ",Label.LABEL_ALIGNRIGHT))
	local colormul = self:add(Slider:new(75,self:getHeight()-18,w-95,16))

	colorfield = self:add(Component:new(w-20,self:getHeight()-18,20,16))
	colorfield.skinnames = TextField.skinnames
	colorfield:setSkin("default")
	function colorfield:color (r,g,b,a)
		self:getSkin():setSkinColor(r,g,b,colortab[4])
	end
	
	if not alphaname then 
		colormul:setVisibility(false) 
		alpha:setVisibility(false) 
		colorfield:setVisibility(false) 
	end

	--local h,s,v,i
	local function setclr(sliderset,h,s,v,i)
		colortab[1] = h
		colortab[2] = s
		colortab[3] = v
		colortab[4] = i

		local r,g,b = UtilFunctions.color3hsv(h,s,v)
		colorfield:color(r,g,b,1)
		colorsbc:color(UtilFunctions.color3hsv(h,1,1))
		colorhbar:color(1-r,1-g,1-b,1)
		colorsbb:color(1-r,1-g,1-b,1)
		colorsbs:color(1-r,1-g,1-b,1)

		colorhbar:setLocation(colorhue:getX()+math.floor((h*(colorhue:getWidth()-2))),colorhue:getY())
		colorsbs:setLocation(
			colorsbc:getX()+math.floor(((s)*(colorsbc:getWidth()-2))),
			colorsbc:getY()
			)
		colorsbb:setLocation(
			colorsbc:getX(),
			colorsbc:getY()+math.floor(((1-v)*(colorsbc:getHeight()-2)))
			)
		
		if (sliderset) then
			colormul:setSliderPos(1-((i/-maxint)+0.5))
		end

		--colortargetfn(r,g,b,h,s,v,i or 1)
		
		fn(obj,r,g,b,i)

	end
	local function getclr()
		local h,s,v,i
		local f = function (x,p,v) return math.floor(x*p+v)/p end

		h = (colorhbar:getX()-colorhue:getX())/(colorhue:getWidth()-2)
		s = (colorsbs:getX()-colorsbc:getX())/(colorsbc:getWidth()-2)
		
		v = 1-((colorsbb:getY()-colorsbc:getY())/(colorsbc:getHeight()-2))
		i = colormul:getSliderPos()
		
		-- corrupts values
		--s,v = f(s,100,1.0),f(v,200,-2.5)
		


		return h,s,v,i
	end

	--setclr(true,r or 0.3,g or 0.4,b or 0.3,i or 1)
	setclr(true,unpack(colortab or {1,1,1,1}))

	function colorhue:mouseMoved(e)
		if e:isPressed() and not e:isDragged() then self:lockMouse() end
		if not e:isDragged() and not e:isPressed() then return self:unlockMouse() end
		if Component.getMouseLock()~=self then return end
		local x,y = 
			math.min(self:getWidth()-2,math.max(0,e.x)),
			math.min(self:getHeight()-2,math.min(0,e.y))
		
		colorhbar:setLocation(colorhue:getX()+x,colorhue:getY())

		setclr(false,getclr())
	end

	colorhue.mousePressed = colorhue.mouseMoved

	function colorbrtb:mouseMoved(e)
		if e:isPressed() and not e:isDragged() then self:lockMouse() end
		if not e:isDragged() and not e:isPressed() then return self:unlockMouse() end
		if Component.getMouseLock()~=self then return end
		local x,y = 
			math.min(self:getWidth()-2,math.max(0,e.x)),
			math.min(self:getHeight()-2,math.max(0,e.y))
	
		colorsbb:setLocation(
			colorsbc:getX(),
			colorsbc:getY()+y)

		colorsbs:setLocation(
			colorsbc:getX()+x,
			colorsbc:getY())

		setclr(false,getclr())
	end

	colorbrtb.mousePressed = colorbrtb.mouseMoved

	function colormul:onValueChanged(val)
		setclr(false,getclr())
		alpha:setText((alphaname or "Alpha (%.2f):"):format(val))
	end

	colormul:setSliderPos(1)
	colormul:setSliderPos(a)
	--colormul:setSliderPos(colortab[4])
	--fn(obj,unpack(colortab))
	return frame
end

LuxModule.registerclassfunction("addcolorbutton",
	[[(int newy, fn setcolor, Button):(Container frame,int x,y,w,h, string title, table parameters) -
	creates a color button which on execution pops up a colormenu dialog. returns a function, which
	can be used to change preview button color, and new y value which is y + h.<br>
	Function: setcolor(table colorRGBA, [boolean noevent]. Returned button contains .lbl<br>
	Parameters table contains:<br>
	* initvalue: table, color HSVA<br>
	* initrgba: table, color RGBA<br>
	* callback: function, callback(obj,r,g,b,a)<br>
	* [obj]: upvalue for function callback<br>
	* [alphaname]: string, Alphaname if alpha slider should be available<br>
	* [nomodal]: boolean, no darkening modal dialog<br>
	* [noalpha]: boolean, no alpha in the preview button color<br>
	* [titleicon]: Icon, instead of button label.<br>
	* [ok]: string, okay text<br>
	* [cancel]: string, cancel text<br>
	]])
function GH.addcolorbutton(frame,x,y,w,h,title,param)
	local init,fnchange,obj,alphaname,menuboundsfunc,nomodal,noalpha =
		param.initvalue,param.callback,param.obj,param.alphaname,param.menuboundsfunc,param.nomodal,param.noalpha
		
	local lbl = param.titleicon and frame:add(Button:new(x,y,w-24,h,"")) or frame:add(Label:new(x,y,w-24,h,title))

	if param.titleicon then
		local skin = lbl:getSkin()
		skin:setIconAlignment(Skin2D.ALIGN.CENTER,Skin2D.ALIGN.CENTER)
		skin:setSkinColor(1,1,1,0)
		skin:setIcon(param.titleicon)
	end
	
	local initrgb = param.initrgba or {UtilFunctions.color3hsv(unpack(init))}
	initrgb[4] = initrgb[4] or init[4]
	local btn = frame:add(Button:new(x+(w-24),y,h,h,nil))
	btn.color = {unpack(initrgb)}
	
	local function newchange(objn,r,g,b,a)
		btn.color = {r,g,b,a}
		btn:getSkin():setSkinColor(r,g,b,noalpha and 1 or math.max(0.1,a))
		fnchange(objn,r,g,b,a)
	end
	
	function btn:onClicked()
		local frame = GH.modaldialog(nil,nil,nil,nil,nil,nil,nil,nomodal)
		local winw,winh = window.refsize()
		local menubounds = menuboundsfunc and menuboundsfunc() or {(winw-220)/2,(winh-160)/2,220,160}
		
		frame:setBounds(unpack(menubounds))
		
		
		local curcolor = btn.color
		local curhsv = {UtilFunctions.color3rgb2hsv(unpack(curcolor))}
		curhsv[4] = curcolor[4]
		frame:add(GH.createcolormenu(0,0,frame:getWidth(),frame:getHeight()-20-16,title,
			curhsv,newchange,obj,alphaname))
		
		local btnok = frame:add(Button:new(10,frame:getHeight()-16-10,64,24,param.ok or "OK"))
		function btnok:onClicked()
			frame:delete()
		end
		
		local btnc = frame:add(Button:new(frame:getWidth()-10-64,frame:getHeight()-16-10,64,24,param.cancel or "CANCEL"))
		function btnc:onClicked()
			newchange(obj,unpack(curcolor))
			frame:delete()
		end
	end
	lbl.onClicked = btn.onClicked
	btn.lbl = lbl
	
	newchange(obj,unpack(initrgb))
	
	return y + h,
		function(clr,noevent) 
			if(clr) then 
				local r,g,b,a = unpack(clr)
				btn.color = {r,g,b,a}
				btn:getSkin():setSkinColor(r,g,b,noalpha and 1 or math.max(0.1,a))
				if (not noevent) then fnchange(objn,r,g,b,a) end
			else 
				return {unpack(btn.color)} 
			end 
		end,
		btn
end

-- if allowinput is set then callback is like
-- 		outvalue, sliderpos = callback(slidervalue,numericinput,sliderpos)
--			on slidervalue change first/third is set
--			on numericinput change only second is set, and function must return sliderpos 0-1
LuxModule.registerclassfunction("addslider",
	[[(int newy, Slider):(Container frame,int x,y,w,h, string title, table parameters) -
	creates a slider with leading description label (slider.lbl) and following value label (slider.lblval). New y value which is y + h.<br>
	slidervalue = sliderpos * scale + offset<br>
	Parameters table contains:<br>
	* initvalue: float, 0-1 position along slider
	* callback: function, (outvalue, sliderpos) : callback (slidervalue,numericinput,sliderpos). When allowinput is true and callback is called with "nil,numericinput,nil" it must return sliderpos as well. Otherwise only outvalue is taken and (slidervalue, nil, sliderpos) are passed.
	* [scale]: float
	* [offset]: float
	* [increment]: float, Slider increment
	* [intmode]: boolean, Slider IntegerMode
	
	* [labelw]: int, name label widht, defaults to GH.sldLabelwidth
	* [valw]: int, value label width, defaults to GH.sldLabelval
	* [formatstr]: string, argument to string.format for value label
	* [allowinput]: boolean, allows manual popup dialog input, when clicking on value label
	]])
function GH.addslider (frame,x,y,w,h,name,param) 
	local 	fn,ini,scale,off,incr,
			intmode,labelw,formatstr,valw,
			allowinput =
		param.callback,param.initvalue,param.scale,param.offset,param.increment,
		param.intmode,param.labelw or GH.sldLabelwidth,param.formatstr,param.valw or GH.sldLabelval,
		param.allowinput

	local lbl=frame:add(Label:new(x,y,labelw,h,name,Label.LABEL_ALIGNRIGHT))
	if param.icon then
		lbl:setSkin("default")
		local s = lbl:getSkin()
		s:setSkinColor(1,1,1,0)
		s:setIcon(param.icon)
	end
	
	local lblval = frame:add(Label:new(x+w-valw,y,valw,h,""))
	local slider = 	frame:add(Slider:new(x+labelw,y+(h-14)*.5,w-(labelw+valw+4),14))
	slider:getSkin():setSkinColor(0.9,0.9,0.9,0.6)
	slider.onValueChanged  =
		function (sld,value)
			local v = fn(value*(scale or 1) + (off or 0),nil,value)
			lblval:setText((formatstr or "%.2f"):format(v))
			lblval.value = v
		end
	slider:setIncrement(incr or 0.005)
	slider:setIntegerMode(intmode or false)
	
	slider:setSliderPos(0)
	slider:setSliderPos(ini or 0,nil,true)
	slider.lbl = lbl
	slider.lblval = lblval
	
	function slider.setFromValue(sld,value)
		if (value) then
			local val,slpos = fn(nil,value)
			if (slpos) then
				sld:setSliderPos(slpos,nil,true)
			end
		end
	end
	
	function slider.setFromValueSimple(sld,value)
		if (value) then
			value = (value - (off or 0))/(scale or 1)
			sld:setSliderPos(value,nil,true)
		end
	end
	
	if (allowinput) then
		function lblval:mouseEntered()
			lblval:setFontColor(0.5,0.25,0,1)
		end
		
		function lblval:mouseExited()
			lblval:setFontColor(0,0,0,1)
		end
		
		function lblval:mouseClicked()
			-- evoke modal dialog around the area of lbl
			local ww,wh = window.refsize()
			local x,y = lblval:local2world(0,-25)
			x = x + 120 > ww and ww-120 or x
			y = math.max(0,y)
			
			local frame = GH.modaldialog("Input:",x,y,120,50)
			local txtval = frame:add(TextField:new(5,25,110,20))
			txtval:setText(lblval:getText(),true)
			function txtval:onAction(newtext)
				local nmbr = tonumber(newtext)
				if (nmbr) then
					local outval,sldpos = fn(nil,nmbr)
					lblval:setText((formatstr or "%.2f"):format(outval))
					lblval.value = outval
					slider:setSliderPos(sldpos,true)
					
					frame:delete()
				end
			end
		end
	
	end
	
	return y + h,slider
end

--(frame,y,name,fn,ini,x,width or (GH.guiw-10))
LuxModule.registerclassfunction("addcheckbox",
	[[(int newy, CheckBox):(Container frame,int x,y,w,h, string title, table parameters) -
	adds CheckBox to frame.<br>
	Parameters table contains:<br>
	* initvalue: boolean
	* callback: function, callback(state).
	]])
function GH.addcheckbox (frame,x,y,w,h,name,param)
	local fn,ini = param.callback,param.initvalue
	local chk = frame:add(CheckBox:new(x,y,w,h or 16,name))
	function chk:onClicked(state)
		fn(state)
	end
	chk:setPushState(ini or false)
	fn(ini or false)
	return y + (h or 16),chk
end


LuxModule.registerclassfunction("makeLayoutFunc",
	[[(fnlayouter):(Rectangle hostref, Rectangle ref, table params) -
	returns a function that computes new rectangles according to 
	rescaling / repositioning behavior. x,y,w,h returned by fnlayouter (nx,ny,nw,nh).
	On call of makeLayoutFunc the current Rectangles and their relative 
	positions to refbounds are stored.<br>
	The params table contains:<br>
	* bottom: boolean for anchor.y
	* right: boolean for anchor.x
	* relscale: {boolean w,h}, required
	* relpos: {boolean x,y}, required
	<br>
	When relscale is true for a dimension the size is changed according to the ratio of 
	original refsize and newsize. When relpos is true, the position of top left corner
	of the component changes relatively to scaling, when false it will keep its original
	distance to the anchor point. Anchor point is made from reference dimension of
	hostframes and is created from "right" and "bottom".
	]])
local getAnchor = {
	function(x,y,w,h)
		return x,y
	end,
	function(x,y,w,h)
		return x+w,y
	end,
	function(x,y,w,h)
		return x,y+h
	end,
	function(x,y,w,h)
		return x+w,y+h
	end,
}
function GH.makeLayoutFunc (hostref,ref,c)
	local hx,hy,hw,hh = unpack(hostref)
	local x,y,w,h = unpack(ref)
	
	local anchor = getAnchor[(c.bottom and 2 or 0) + (c.right and 1 or 0) + 1]
	local ax,ay = anchor(hx,hy,hw,hh)
	ax,ay = x-ax,y-ay
	
	local hostref = {hx,hy,hw,hh}
	local ref = {x,y,w,h,ax,ay}
	
	
	local function layouter(nx,ny,nw,nh)
		local sx,sy = nw/hostref[3],nh/hostref[4]

		local ox,oy,ow,oh,oax,oay = unpack(ref)
		local ax,ay = anchor(nx,ny,nw,nh)
		local w,h = c.relscale[1] and ow*sx or ow, c.relscale[2] and oh*sy or oh
		local x,y = c.relpos[1] and ny+(ox*sx) or ax+oax,c.relpos[2] and nx+(oy*sy) or ay+oay
		
		return math.floor(x),math.floor(y),math.floor(w),math.floor(h)
	end
	
	return layouter
end

LuxModule.registerclassfunction("frameminmax",
	[[(Button btn):(Container frame, fnminbounds, fnmaxbounds, [fninput], [boolean spaceable]) -
	creates a button "Hide"/"Show" which sets bounds based on the returns of
	fnmaxbounds and fnminbounds, which use fninput as input. All functions return x,y,w,h. If
	fninput is not specified current window dimensions are taken.
	When minimized all children are removed. Once minimized btn.oldcomponents will be set and
	hold all temporarily removed Components.
	When spaceable is set, focus and space bar will not cause the button to act.
	]])
function GH.frameminmax (frame,fnminbounds,fnmaxbounds,fninput,spaceable)
	local maximize,minimize
	local minbtn = frame:add(Button:new(frame:getWidth()-60,4,58,18,"Hide"))
	function minimize (self)
		if spaceable and Keyboard.isKeyDown("space") then return end
		
		local ww,wh = window.refsize()
		
		local dim = fninput and {fninput()} or {0,0,window.refsize()}

		frame:setBounds(fnminbounds(unpack(dim)))
		minbtn:setBounds(frame:getWidth()-60,4,58,18)
		
		minbtn.oldcomponents = {}
		-- copy children and remove all
		for i,c in ipairs(frame.components) do
			if (c ~= minbtn) then
				table.insert(minbtn.oldcomponents,c)
			end
		end
		frame:removeChilds()
		
		frame:add(minbtn)
		
		self:setText("Show")
		self.onClicked = maximize
	end
	function maximize(self)
		if spaceable and Keyboard.isKeyDown("space") then return end
		
		local dim = fninput and {fninput()} or {0,0,window.refsize()}
		
		frame:setBounds(fnmaxbounds(unpack(dim)))
		minbtn:setBounds(frame:getWidth()-60,4,58,18)
		
		-- add back children
		for i,c in ipairs(minbtn.oldcomponents) do
			frame:add(c)
		end
		minbtn.oldcomponents = nil
		
		self:setText("Hide")
		self.onClicked = minimize
	end
	minbtn.onClicked = minimize
	
	return minbtn
end

LuxModule.registerclassfunction("getIconUVs",
	[[(float u,v,uscale,vscale):(texture,x,y,[iconwidth],[iconheight],[gridwidth],[gridheight]) -
	returns u,v position and scalings for an icon inside a texture (top =0,0) for setting texture
	matrices. Takes rectangle textures into account. Grid and icon dimensions default to 24.
	]])
function GH.getIconUVs(tex,x,y,iw,ih,gw,gh)
	iw,ih = iw or 24, ih or 24
	gw,gh = gw or 24, gh or 24
	local tw,th = tex:dimension()
	if (tex:formattype() == "rect") then
		tw,th = 1,1
	end
	x,y = x*gw,y*gh
	
	y = th-y-ih-1
	local u,v = x/tw,y/th
	local us,vs = iw/tw, ih/th
	
	return u,v,us,vs
end