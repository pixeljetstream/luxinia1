Viewer = {}
Viewer.paint = {}

LuxModule.registerclass("luxinialuacore","Viewer",
	[[The Viewer is a builtin model/animation/particle and material viewer
	of Luxinia. ]],Viewer,
	{
	initbase = 		"():() - sets up all data needed for the viewer, call this first",
	loadres = 		"():(string resname, [string animname]) - loads a given resource (.mtl,.f3d/.mm,.prt) and for models an animation (.ma) can be specified.",
	reloadMain = 	"():() - reloads maindata and all dependent data",
	reloadAnim = 	"():() - reloads animations",
	reloadMat = 	"():() - reloads materials and all dependent data",
	})


-- Luxinia Model/Particle Viewer
local ProjectpathFull

local function SetProjectPath()
	ProjectpathFull = system.projectpath()
	if (not (ProjectpathFull:find(":/") or ProjectpathFull:find(":\\"))) then
		local curdir = FileSystem.currentdir()
		curdir = curdir:gsub("\\","/")
		ProjectpathFull = ProjectpathFull:sub(1,1) == "/" and ProjectpathFull or "/"..ProjectpathFull
		ProjectpathFull = curdir..ProjectpathFull
	end
end


function Viewer.reloadMain()
	LuaConsole.print("reloadmain")
	if (Viewer.vnode) then
		Viewer.vnode:delete()
		Viewer.vnode = nil
	end
	Viewer.paint.detex()
	Viewer.matchunk:clearload(-1)
	Viewer.animchunk:clearload(-1)
	Viewer.geochunk:clearload(-1)
	Viewer.geochunk:activate()
	Viewer.loadres(Viewer.name,nil)
	--Viewer.paintlist.onActivate()
end

function Viewer.reloadAnim(name)	
	LuaConsole.print("reloadanim")
	if (name and name ~= Viewer.animname) then
		Viewer.animchunk:clearload(-1)
		Viewer.anim = animation.load(name)
		Viewer.animname = name
	else
		Viewer.animchunk:clearload(0)
	end
	Viewer.geochunk:activate()
	Viewer.update_anim()
end

function Viewer.reloadMat()
	LuaConsole.print("reloadmat")
	Viewer.paint.detex()
	-- keep the textures from model->tex
	Viewer.matchunk:clearload(1)
	Viewer.geochunk:activate()
	--Viewer.paintlist.onActivate()
end


function Viewer.toggleBones()
	render.drawbones(not render.drawbones())
end

function Viewer.toggleBonenames()
	render.drawbonenames(not render.drawbonenames())
end

function Viewer.toggleBonelimits()
	render.drawbonelimits(not render.drawbonelimits())
end

function Viewer.toggleLight()
	Viewer.vnode:rfLitSun(not Viewer.vnode:rfLitSun())
end

function Viewer.resetTransform()
	if (Viewer.prtres) then
		Viewer.vhandle:rotdeg(90,0,0)
	else
		Viewer.vhandle:rotdeg(0,0,0)
	end
	Viewer.vhandle:pos(0,0,0)
end

function Viewer.toggleAnim()
	if (Viewer.anim) then
		local state = Viewer.vnode:seqState(0)
		if (state == 1) then
			state = 2
		else 
			state = 1
		end	
		Viewer.vnode:seqState(0,state)
	end
	if (Viewer.prtres) then
		if (Viewer.prton) then
			Viewer.vnode:stop(0,Viewer.norestarts)
		else
			Viewer.vnode:start(0)
		end
		Viewer.prton = not Viewer.prton
	end
end	

-- START GUI HELPERS -------------

local function checkextension(name,extensions)
	for i,str in ipairs(extensions) do 
		if name:match("%."..str.."$") then
	   		return str
		end 
	end
	return nil
end

local function modeldialog(title)
	local modal = Container.getRootContainer():add(Container:new(0,0,Viewer.w,Viewer.h),1)
	local order = Component.newFocusOrder({})
	
	local frame = modal:add(TitleFrame:new(100,100,440,300,title or nil))
	frame.modal = modal
	
	function frame:delete()
		GroupFrame.delete(self)
		modal:delete()
		Component.newFocusOrder(order)
	end

	return frame
end

local function scrollfield (x,y,w,h)
	local frame = GroupFrame:new(x,y,w,h)
	frame.skinnames = TextField.skinnames
	frame:getSkin():selectSkin("textfield")

	local scrollbar = frame:add(Slider:new(
		frame:getWidth()-16,0,16,frame:getHeight(),true))
	local view = Container:new(5,4,frame:getWidth()-22,frame:getHeight()-7)
	frame:add(view)
	frame.area = view:add(Container:new(0,0,0,0))
	local W,H = 0,0
	function frame.area:setBounds(x,y,w,h)
		Container.setBounds(self,x,y,w,h)
		if H~=h then
			W,H = w,h
			local h2 = view:getHeight()
			local n = math.max(H-h2,0)/8
			if n==0 then
				scrollbar:setSliderPos(0)
				scrollbar:setVisibility(false)
			else
				scrollbar:setVisibility(true)
				scrollbar:setSliderPos(0)
				scrollbar:setIncrement(1/n)
			end
		end
	end
	function frame:mouseWheeled(me)
		scrollbar:mouseWheeled(me)
	end
	function scrollbar:onValueChanged(value)
		local h = view:getHeight()
		value = value*math.max(0,H-h)
		frame.area:setLocation(0,-value)
	end
	return frame
end


local function filedialog(title,oktext,canceltext,initpath,onsave,oncancel,extensions)
	Viewer.guipath = ""
	local frame = modeldialog()
	frame:add(Label:new(0,2,frame:getWidth(),20,"\v999\vb"..title,Label.LABEL_ALIGNCENTERED),
		Label.LABEL_ALIGNCENTERED)

	local cancel = frame:add(Button:new(7,frame:getHeight()-32,100,26,canceltext))
	function cancel:onClicked()
		if not oncancel or oncancel(frame) then frame:delete() end
	end
	


	--local path = frame:add(Label:new(
	--8,40,frame:getWidth()-10,22))

	local chose = frame:add(ComboBox:new(7,30,frame:getWidth()-12,24))
	

	local dirs = frame:add(scrollfield(7,60,180,frame:getHeight()-150))

	local files = frame:add(scrollfield(187,60,frame:getWidth()-187-6,frame:getHeight()-150))

	local projectpath = frame:add(Label:new(7,frame:getHeight()-60,frame:getWidth()-70,20,"projectpath: "..system.projectpath()))
	
	local filename = frame:add(TextField:new(
		6,frame:getHeight()-80,frame:getWidth()-10,22,""))
		
	
	local ok = frame:add(Button:new(frame:getWidth()-100-5,frame:getHeight()-32,
		100,26,oktext))
	function ok:onClicked()
		if onsave(frame,Viewer.guipath,filename:getText()) then
			frame:delete()
		end
	end
	
	local btnpath = frame:add(Button:new((frame:getWidth()-100)/2,frame:getHeight()-32,100,26,"set projectpath"))
	function btnpath:onClicked()
		system.projectpath(Viewer.guipath)
		projectpath:setText("projectpath: "..Viewer.guipath)
	end

	function frame.updatepath(p)
		p = p:gsub("/$","")
		p = (p:gsub("\\","/").."/"):gsub("//","/"):gsub("/%./","/")
		p = p:gsub("/[^/]-/%.%.","")
		local curdrive
		
		Viewer.guipath = p
		chose:clearItems()
		function add (p)
			if p and p:match("/") then
				curdrive = p
				chose:addItem(p,nil,defaulticon16x16set,path == p and "openfolder" or "folder")
				return add(p:match("^(.*/).*/[^/]*$"))
			end
		end
		print("updatepath",p)
		add(p)
		
		if (not Viewer.guidrives) then
			Viewer.guidrives = {}
			local startdrive = string.byte("C")
			local enddrive = string.byte("Z")
			for i=startdrive,enddrive do
				local curpath = string.char(i)..":/"
				local fn,err = FileSystem.dir(curpath)
				if (fn()) then
					table.insert(Viewer.guidrives,curpath)
				end
			end
		end
		for i,v in pairs (Viewer.guidrives) do
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
				btn:getSkin():setSkinColor(0.5,0.5,1,0.5)
			end
			function btn:mouseExited (me)
				Button.mouseExited(self,me)
				btn:getSkin():setSkinColor(1,1,1,0)
			end
			return on:add(btn)
		end

		print("deletechilds")
		dirs.area:deleteChilds()
		dirs.area:setSize(dirs.area.parent:getWidth(),#dirlist*20)

		table.sort(dirlist,function (a,b) return a:lower()<b:lower() end)
		for i,name in ipairs(dirlist) do
			addbtn(i-1,dirs.area,name).onClicked = function ()
				frame.updatepath(p..name)
			end
		end

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
			addbtn(i-1,files.area,name,icon).onClicked = function ()
				Viewer.guifile = name
				filename:setText(name)
			end
		end
	end
	frame.updatepath(initpath)
	
	function chose:onSelected(idx,cap,fn) 
		if cap and cap~=Viewer.guipath then 
			current = cap
			frame.updatepath(cap) 
		end 
	end
	
	return frame
end

local function createcolormenu(title,x,y,w,h, colortab, fn,obj,alphaname)
	local maxint = 8
	local intsteps = 32
	local frame= TitleFrame:new(x,y,w,h)
	local huetex = Viewer.guitexhue
	local a = colortab[4] or 1
	w,h = w-20,h-25
	local self = frame:add(Container:new(10,25,w,h))
	local hue = self:add(ImageComponent:new(0,0,w,10,huetex))
	local colorhue = hue
	
	frame:add(Label:new(10,2,w-10,20,title))

	local colorfield			
	-- saturation and brightness
	local x,y,w,h = hue:getX(),hue:getY() + hue:getHeight() + 2,
		hue:getWidth(), h - hue:getY() - hue:getHeight() - (alphaname and 20 or 10)
	local texmatrix = matrix4x4.new()
	texmatrix:rotdeg(0,0,90)
	local colorbrtb = self:add(ImageComponent:new(x,y,w,h,Viewer.guitexgrey,nil,blendmode.modulate(),texmatrix))
	local colorsatb = self:add(ImageComponent:new(x,y,w,h,Viewer.guitexgrey,nil,blendmode.add()))
	local colorsbc = self:add(ImageComponent:new(x,y,w,h,matsurface.vertexcolor()))

	local colorhbar	= self:add(ImageComponent:new(100,10,2,hue:getHeight(),matsurface.vertexcolor()),1)
	local colorsbs = self:add(ImageComponent:new(100,60,2,h,matsurface.vertexcolor()),1)
	local colorsbb = self:add(ImageComponent:new(100,60,w,2,matsurface.vertexcolor()),1)

	function colorsbs:contains(x,y) return false end
	function colorsbb:contains(x,y)	return false end
	function colorhbar:contains(x,y) return false end

	
	local alpha = self:add(Label:new(0,self:getHeight()-18,75,16,alphaname or "Alpha: ",Label.LABEL_ALIGNRIGHT))
	local colormul = self:add(Slider:new(75,self:getHeight()-18,self:getWidth()-95,16))
	
	
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

-- some general info on the sizes
local guiw = 200
local guislw = 56
local guicw = 200

local function addslider (frame,y,name,fn,ini,scale,off,incr,intmode,x) 
	frame:add(Label:new(x or 10,y,guislw,16,name,Label.LABEL_ALIGNRIGHT))
	local val = frame:add(Label:new(guiw-30,y,28,16,""))
	local slider = 	frame:add(Slider:new((x or 10)+guislw,y+1,guiw-35-(x or 10)-guislw,14))
	slider:getSkin():setSkinColor(0.9,0.9,0.9,0.6)
	slider.onValueChanged  =
		function (self,value)
			local v = fn(value*(scale or 1) + (off or 0))
			val:setText(("%.2f"):format(v))
		end
	slider:setIncrement(incr or 0.005)
	slider:setIntegerMode(intmode or false)
	
	slider:setSliderPos(0)
	slider:setSliderPos(ini or 0)
	return y + 16,slider
end

local function addcheckbox (frame,y,name,fn,ini,x,width,help)
	local chk = frame:add(Checkbox:new(x or 10,y,width or (guiw-10) ,16,name))
	function chk:onClicked(state)
		fn(state)
	end
	chk:setPushState(ini or false)
	chk.statusinfo = help
	return y + 16,chk
end

local function addbutton (frame,x,y,width,height,name,fn,descr)
	local chk = frame:add(Button:new(x,y,width or 50,height,name))
	chk.statusinfo = descr or " "
	function chk:onClicked()
		fn(chk)
	end
	return y + height,chk
end

-- END GUI HELPERS --------------

-- GUI init
function 	Viewer.initgui()
	local root = Container.getRootContainer()
	local w,h = window.refsize()
	Viewer.toollist = 		root:add(GroupFrame:new(0,0,w,31))
	Viewer.envirolist = 	root:add(GroupFrame:new(w-guiw,30,guiw,h-30))
	Viewer.proplist = 		root:add(GroupFrame:new(w-guiw,30,guiw,h-30))
	Viewer.optionslist = 	root:add(GroupFrame:new(w-guiw,30,guiw,h-30))
	--Viewer.paintlist = 		root:add(GroupFrame:new(w-guiw,30,guiw,h-30))
	Viewer.controlslist = 	root:add(GroupFrame:new((w-guicw)/2,h-31,guicw,31))
	
	local dynamicmenus = {
		Enviro = {Viewer.envirolist},
		Prop = {Viewer.proplist},
		Options = {Viewer.optionslist},
		--Paint = {Viewer.paintlist},
	}
	
	function Viewer.resizegui()
		w,h = window.refsize()
		Viewer.w,Viewer.h = w,h
		
		for i,m in pairs(dynamicmenus) do
			m[1]:setBounds(w-guiw,30,guiw,h-30)
		end
		Viewer.toollist:setBounds(0,0,w,31)
		Viewer.controlslist:setBounds((w-guicw)/2,h-31,guicw,31)
		
		Viewer.lview:viewrefbounds(0,30,w-guiw+5,h-30)
		if (Viewer.visiblegui) then
			Viewer.cam:aspect((w-guiw+5)/(h-30))
		end
			
	end
	UtilFunctions.addWindowResizeListener(Viewer.resizegui)
	
	window.minsize(640,480)
	window.resizemode(2)
	
	
	Viewer.guitexhue = texture.load("base/textures/colorhue.tga")
	Viewer.guitexgrey = texture.load("base/textures/colorgradientgrey.tga")
	
	
	function Viewer.activateDynamicMenu (name)
		Container.getRootContainer().statusinfo = nil
		for i,v in pairs(dynamicmenus) do
			v[1]:setVisibility(i==name)
			if (i == name) then
				Viewer.activeViewerMenu = v[1]
				v[1]:invalidate()
				if (v[1].onActivate) then v[1]:onActivate() end
			end
		end
	end
	
	Viewer.visiblegui = true
	function Viewer.hidegui()
		Viewer.visiblegui = not Viewer.visiblegui
		Viewer.toollist:setVisibility(Viewer.visiblegui)
		Viewer.activeViewerMenu:setVisibility(Viewer.visiblegui)
	end
	
	--MouseCursor.enable(true)
	MouseCursor.showMouse(true)
	
	do -- TOOL MENU ------------------------------------------
		local width = 60
		local x = 5
		local menu = Viewer.toollist
		local function btnadd(name)
			btn = menu:add(Button:new(x,4,width,24,name))
			x = x+width
			return btn
		end
		
		local btnload= btnadd("load")
		
		local function luxf3dgui(dlg,path,name)
		
			local frame =  dlg.modal:add(TitleFrame:new(dlg:getX(),dlg:getY()+dlg:getHeight()-4,dlg:getWidth(),80))
			frame:add(Label:new(0,2,frame:getWidth(),20,"\v999\vb".."LuxF3D Conversion",Label.LABEL_ALIGNCENTERED),
				Label.LABEL_ALIGNCENTERED)
				
			local frameswizzle = dlg.modal:add(TitleFrame:new(dlg:getX()+dlg:getWidth()-4,dlg:getY(),100,dlg:getHeight()-4+frame:getHeight()))
	
				
			local y = 20
			
			Viewer.luxf3d.outdir = Viewer.luxf3d.outdir or path
			Viewer.luxf3d.outtexdir = Viewer.luxf3d.outtexdir or path
			
			local lblout = frame:add(Label:new(10,y,frame:getWidth()-10,20,"OutDir: "..Viewer.luxf3d.outdir))
			y = y + 16
			local lblouttex = frame:add(Label:new(10,y,frame:getWidth()-10,20,"OutTexDir: "..Viewer.luxf3d.outtexdir))
			

			
			y = 80-20
			addbutton(frame,10,y,90,16,"Cancel",
				function() 
					Viewer.luxf3d.guiframe = nil
					frame:delete() 
					frameswizzle:delete()
				end,"")
			
			addbutton(frame,(frame:getWidth()-200)/2,y,90,16,"Set OutDir",
				function() 
					Viewer.luxf3d.outdir = Viewer.guipath 
					lblout:setText("OutDir: "..Viewer.guipath)
				end,"where f3d/ma files are written to")
			addbutton(frame,(frame:getWidth()-200)/2 + 100,y,90,16,"Set OutTexDir",function() 
					Viewer.luxf3d.outtexdir = Viewer.guipath 
					lblouttex:setText("OutTexDir: "..Viewer.guipath)
				end,"where textures are written to (q3 bsp)")
				
				
				
	
			addbutton(frame,frame:getWidth()-100,y,90,16,"Convert",
			function() 
				Viewer.luxf3d.guiframe = nil 
				local commandstring = Viewer.luxf3d.exefile.." "..path..name
				commandstring = commandstring.." -odir"..Viewer.luxf3d.outdir.." -lmdir"..Viewer.luxf3d.outtexdir
			
				commandstring = commandstring..(Viewer.luxf3d.nof3d and " -nof3d" or "")
				commandstring = commandstring..(Viewer.luxf3d.noma and " -noma" or "")
				commandstring = commandstring..(Viewer.luxf3d.tangents and " -tangents" or "")
				commandstring = commandstring..(Viewer.luxf3d.optimize and " -optimize" or "")
				commandstring = commandstring..(Viewer.luxf3d.stripify and " -strip" or "")
				commandstring = commandstring..(Viewer.luxf3d.cache16 and " -cache16" or "")
				commandstring = commandstring..(Viewer.luxf3d.fixcyl and " -fixcyl" or "")
				commandstring = commandstring..(" -tancos"..Viewer.luxf3d.tancos)
				commandstring = commandstring..(" -bincos"..Viewer.luxf3d.bincos)
				commandstring = commandstring..(" -frametime"..Viewer.luxf3d.frametime)
				commandstring = commandstring..(" -nthframe"..Viewer.luxf3d.nthframe)
				if (Viewer.luxf3d.swrevert>0 or Viewer.luxf3d.swflip>0 or Viewer.luxf3d.swaxis > 1 or Viewer.luxf3d.swsign > 1) then
					local swizzlestr = " -swizzle("
					for i=1,3 do 
						swizzlestr = swizzlestr..(Viewer.luxf3d.swaxistab[i]*Viewer.luxf3d.swsigntab[i])..","
					end
					
					swizzlestr = swizzlestr..Viewer.luxf3d.swrevert..","..Viewer.luxf3d.swflip..","..Viewer.luxf3d.swsigntab[4]..")"
					commandstring = commandstring..swizzlestr
				end
				
				-- CALL command
				print(commandstring)
				os.execute(commandstring)
				
				frame:delete()
				frameswizzle:delete()
				dlg.updatepath (Viewer.luxf3d.outdir)
			end,"runs conversion")
			
			
			frameswizzle:add(Label:new(0,2,frameswizzle:getWidth(),20,"\v999\vbSwizzle",Label.LABEL_ALIGNCENTERED),
				Label.LABEL_ALIGNCENTERED)
			y = 24
			
			local ddswizzle = frameswizzle:add(ComboBox:new(5,y,90,16))
			ddswizzle:addItem(" X Y Z QW",{1,2,3})
			ddswizzle:addItem(" X Z Y QW",{1,3,2})
			ddswizzle:addItem(" Z X Y QW",{3,1,2})
			ddswizzle:addItem(" Z Y X QW",{3,2,1})
			ddswizzle:addItem(" Y X Z QW",{2,1,3})
			ddswizzle:addItem(" Y Z X QW",{2,3,1})
					
			
			function ddswizzle:onSelected(idx,cap,fn) 
				Viewer.luxf3d.swaxis = idx 
				Viewer.luxf3d.swaxistab = fn
			end
			ddswizzle:select(Viewer.luxf3d.swaxis)
			
			y = y + 20
			local ddsign = frameswizzle:add(ComboBox:new(5,y,90,16))
			ddsign:addItem(" + + + +",{1,1,1,1})
			ddsign:addItem(" + + - +",{1,1,-1,1})
			ddsign:addItem(" + - + +",{1,-1,1,1})
			ddsign:addItem(" + - - +",{1,-1,-1,1})
			ddsign:addItem(" - + + +",{-1,1,1,1})
			ddsign:addItem(" - + - +",{-1,1,-1,1})
			ddsign:addItem(" - - + +",{-1,-1,1,1})
			ddsign:addItem(" - - - +",{-1,-1,-1,1})
			ddsign:addItem(" + + + -",{1,1,1,-1})
			ddsign:addItem(" + + - -",{1,1,-1,-1})
			ddsign:addItem(" + - + -",{1,-1,1,-1})
			ddsign:addItem(" + - - -",{1,-1,-1,-1})
			ddsign:addItem(" - + + -",{-1,1,1,-1})
			ddsign:addItem(" - + - -",{-1,1,-1,-1})
			ddsign:addItem(" - - + -",{-1,-1,1,-1})
			ddsign:addItem(" - - - -",{-1,-1,-1,-1})
			
			function ddsign:onSelected(idx,cap,fn) 
				Viewer.luxf3d.swsign = idx 
				Viewer.luxf3d.swsigntab = fn
			end
			
			ddsign:select(Viewer.luxf3d.swsign)
			
			y = y + 20
			y = addcheckbox (frameswizzle,y,"reverse tris",
				function(state) Viewer.luxf3d.swrevert = state and 1 or 0 end,Viewer.luxf3d.swrevert>0,nil,90) 
			y = addcheckbox (frameswizzle,y,"flip normals",
				function(state) Viewer.luxf3d.swflip = state and 1 or 0 end,Viewer.luxf3d.swflip>0,nil,90)
				
			return {frame,frameswizzle}
		end
		
		local function onsave(dlg,path,filename)
			if (Viewer.luxf3d.guiframe) then return false end
			if (checkextension(filename,Viewer.luxf3d.convextensions)) then
				Viewer.luxf3d.guiframe = luxf3dgui(dlg,path,filename)
				return false
			end
			Viewer.lastpath = path
			Viewer.name = path..filename
			Viewer.animname = nil
			Viewer.reloadMain()
			return true
		end
		
		local function oncancel(dlg)
			if (Viewer.luxf3d.guiframe) then
				Viewer.luxf3d.guiframe[1]:delete()
				Viewer.luxf3d.guiframe[2]:delete()
			end
			return true
		end
		
		function btnload:onClicked()
			local extensions = {"mm","mtl","f3d","prt"}
			if (Viewer.luxf3d.exefile ~= nil) then
				for i,v in pairs(Viewer.luxf3d.convextensions) do
					table.insert(extensions,v)
				end
			end
			Viewer.luxf3d.guiframe = nil
			local frame = filedialog("choose resource","load","cancel",Viewer.lastpath,onsave,oncancel,extensions)
		end
		
		
		x = x+5
		local btnenviro = btnadd("enviro")
		local btnprop = btnadd("properties")
		local btnopt = btnadd("options")
		--local btnpaint = btnadd("paint")
		
		local menugroup = ButtonGroup.new()
		menugroup:addButton(btnenviro)
		menugroup:addButton(btnprop)
		menugroup:addButton(btnopt)
		--menugroup:addButton(btnpaint)
		
		menugroup:click(btnenviro)
		
			
		function btnenviro:onClicked() Viewer.activateDynamicMenu("Enviro") end
		function btnprop:onClicked() Viewer.activateDynamicMenu("Prop") end
		function btnopt:onClicked() Viewer.activateDynamicMenu("Options") end
		--function btnpaint:onClicked() Viewer.activateDynamicMenu("Paint") end
		btnload.statusinfo = "Load new Resources"
		btnenviro.statusinfo = "Environment settings, sun, ambient fog..."
		btnprop.statusinfo = "Resource properties"
		btnopt.statusinfo = "General options, quick reload, LuxF3D options"
		--btnpaint.statusinfo = "Paint into Textures"
		
		local statusinfo =
			menu:add(Label:new(x,6,w-10-(x),20,"Loaded",Label.LABEL_ALIGNLEFT))
			

		if statusmouselistener then
			MouseCursor.removeMouseListener(statusmouselistener)
		end
		statusmouselistener = MouseListener.new(
			function ()
				local x,y = MouseCursor.pos()
				local comp = Container.getRootContainer():getComponentAt(x,y)

				if comp and comp.statusinfo then
					if type(comp.statusinfo) == "function" then
						staatusinfo:setText(comp.statusinfo())
					else
						statusinfo:setText(tostring(comp.statusinfo))
					end
				end
			end)
		statusinfo.statusinfo = "Status information bar"
		MouseCursor.addMouseListener(statusmouselistener)

		statusinfo:setSkin("default")
		statusinfo:getSkin():setSkinColor(0.8,0.8,0.9,1)
		
		
		function menu:positionUpdate(z)
			local w = self:getWidth()
			local x = statusinfo:getX()
			statusinfo:setBounds(x,6,w-10-(x),20)
			return z
		end
		
	end	-- TOOL MENU ------------------------------------------
	
	do	-- PROP MENU ------------------------------------------
		local menu = Viewer.proplist
		menu:add(Label:new(10,5,100,30,"No resource loaded"))
	end	-- PROP MENU ------------------------------------------
	
	do	-- ENVIRO MENU ------------------------------------------
		local menu = Viewer.envirolist
		
		local y = 5
		menu:add(Label:new(0,y,guiw,16,"FOG",Label.LABEL_ALIGNCENTERED))
		y = y+16
		y = addslider(menu,y,"start:",
			function(val)Viewer.lview:fogstart(val) return val end,
			0,4096,0) 
		y = addslider(menu,y,"end:",
			function(val)Viewer.lview:fogend(val) return val end,
			0.25,4096,0) 
		y = addcheckbox(menu,y,"on/off", function(val) Viewer.lview:fogstate(val) end,false,64)
		local con = createcolormenu("color",5,y,menu:getWidth()-10,100,
			{0,0,1,1},
			function (obj,r,g,b,a) 
				Viewer.lview:fogcolor(r*a,g*a,b*a,1)
			end,nil, "Brightness:")
		menu:add(con)
		y = y+100
		menu:add(Label:new(0,y,guiw,16,"CAMERA",Label.LABEL_ALIGNCENTERED))
		y = y+16
		y = addslider(menu,y,"frontplane:",
			function(val)
				val = math.floor(val+0.3)
				val = 2^val
				Viewer.cam:frontplane(val) return val
			end,
			0.0,14,0,1/13,true)
		y = addslider(menu,y,"backplane:",
			function(val)
				val = math.floor(val+0.3)
				val = 2^val
				Viewer.cam:backplane(val) return val
			end,
			0.75,14,0,1/13,true)
		y = addslider(menu,y,"fov:",
			function(val)
				Viewer.cam:fov(val) return val
			end,
			1/3,180,0)
		menu:add(Label:new(0,y,guiw,16,"SUN",Label.LABEL_ALIGNCENTERED))
		y = y+16
		local con = createcolormenu("diffuse",5,y,menu:getWidth()-10,100,
			{0,0,1,1},
			function (obj,r,g,b,a) 
				Viewer.sunsphere:color(r*a,g*a,b*a,1)
				Viewer.sunl3d:diffuse(r*a,g*a,b*a,1)
			end,nil, "Brightness:")
		menu:add(con)
		y = y+100
		local con = createcolormenu("ambient",5,y,menu:getWidth()-10,100,
			{0,0,1,0.2},
			function (obj,r,g,b,a) 
				Viewer.sunl3d:ambient(r*a,g*a,b*a,1)
			end,nil, "Brightness:")
		menu:add(con)
		y = y+100
		
	end	-- ENVIRO MENU ------------------------------------------
	
	if (false) then -- PAINT MENU --------------------------------------
		local menu = Viewer.paintlist
		local y = 5
		
		y = addcheckbox(menu,y,"texcompress (must be off)",		function(val) rendersystem.texcompression(val) end,		rendersystem.texcompression(),5,nil,"must be disabled before loading textures")
		
		menu:add(Label:new(0,y,guiw,16,"TEXTURE",Label.LABEL_ALIGNCENTERED))
		y = y +20
		
		-- image
		local brd = 35
		local ic = menu:add(ImageComponent:new(brd,y,guiw-(brd*2),guiw-(brd*2),matsurface.vertexcolor(),false,blendmode.replace()))
		y = y + guiw-(brd*2) + 5
		
		-- dropdown
		local ddtex = menu:add(ComboBox:new(5,y,guiw-10,16))
		y = y + 20
				
		
		function Viewer.paint.settex(tex)
			Viewer.paint.detex()
			Viewer.paint.tex = tex
			Viewer.paint.tex:mipmapping(false)
			Viewer.paint.tex:getdata()
			
			ic:matsurface(tex)
			
			local w,h = tex:dimension()
			Viewer.paint.texw,Viewer.paint.texh = w-1,h-1
			Viewer.paint.texwf,Viewer.paint.texhf = 1/(w*2),1/(h*2)
		end
		
		function Viewer.paint.detex()
			if (not Viewer.paint.tex) then return end
			Viewer.paint.tex:mipmapping(true)
			Viewer.paint.tex:reloaddata()
			Viewer.paint.tex:deletedata()
			Viewer.paint.tex = nil
			ic:matsurface(matsurface.vertexcolor())
			if (Viewer.paint.active and Viewer.paint.btnp) then
				Viewer.paint.btnp:onAction()
			end
		end
		
		function Viewer.paint.savetex ()
			if (Viewer.paint.tex) then
				Viewer.paint.tex:savetofile(Viewer.paint.tex:getresname(),100)
			end
		end	
		
		-- button to save texture
		y = 5 + addbutton(menu,(guiw-100)/2,y,100,16,"Save File",Viewer.paint.savetex,
			"saves texture / overwrites !")
			
		menu:add(Label:new(10,y,guiw,16,"Disable Channel",Label.LABEL_ALIGNLEFT))
		y = y + 16
		Viewer.paint.mask = {false,false,false,false}
		-- checkbox channel
		local bw = 40
		addcheckbox(menu,y,"R",		function(val) Viewer.paint.mask[1] = val end,		Viewer.paint.mask[1],5,bw)
		addcheckbox(menu,y,"G",		function(val) Viewer.paint.mask[2] = val end,		Viewer.paint.mask[2],5+bw,bw)
		addcheckbox(menu,y,"B",		function(val) Viewer.paint.mask[3] = val end,		Viewer.paint.mask[3],5+(bw*2),bw)
		y = addcheckbox(menu,y,"A",		function(val) Viewer.paint.mask[4] = val end,		Viewer.paint.mask[4],5+(bw*3),bw)
		
	
		function ddtex:onSelected(idx,cap,fn) 
			if (not idx) then return end
			if (idx < 2) then
				Viewer.paint.detex()
			else
				Viewer.paint.settex(fn)
			end
		end
		
		function Viewer.paintlist.onActivate()
			-- update dropdown with all textures
			-- make sure to select current paint tex
			local newidx
			
			ddtex:clearItems()
			ddtex:addItem(" NONE ",nil)
			local n = Viewer.matchunk:getloaded(texture.getrestype())
			
			if(n > 0) then
				for i=1,n do
					local tex = Viewer.matchunk:getloaded(texture.getrestype(),i-1)
					ddtex:addItem(tex:getresshortname(),tex)
					if (tex == Viewer.paint.tex) then
						newidx = i
					end
				end
			end
			
			if (not Viewer.paint.tex or not newidx) then
				ddtex:select(1)
			else
				ddtex:select(1+newidx)
			end
		end
		

		
		y = y + 5
		menu:add(Label:new(0,y,guiw,16,"BRUSH",Label.LABEL_ALIGNCENTERED))
		y = y + 20
		-- brush is stored in a floataray, max size is 129*129
		Viewer.paint.brush = floatarray.new(129*129*4)
		Viewer.paint.brush:set(1)
		Viewer.paint.color = {1,1,1}
		Viewer.paint.smooth = true
		
		function Viewer.paint.gauss(kernelsize)
			if (kernelsize < 2) then return {1} end
		
			local coeffs = {1,1}
			local i = 2
			local n
			local prev
			local self
			local cf
			local sum = 2
			local divsum = 0
			-- in place variant
			while (i < kernelsize) do
				coeffs[i+1] = 1
	
				prev = coeffs[1]
				cf = 2
				for n=1,i-1 do
					self = coeffs[cf]
					coeffs[cf] = prev + self
					divsum = math.max(coeffs[cf],divsum)
					prev = self
					cf = cf + 1
				end
	
				i = i+1
			end
			
			
			divsum = 1.0/divsum
			for i=1,kernelsize do
				coeffs[i] = coeffs[i]*divsum
				print(coeffs[i])
			end
			
			return coeffs
		end
		
		function Viewer.paint.smoothtab (kernelsize)
			local coeffs = {}
			local half = math.floor(kernelsize/2)
			coeffs[half+1] = 1.0
			
			local num
			local div = 1/(half)
				for i=1,half do
				num = math.cos(((i-1)*div)*(math.pi) - (math.pi))*0.5 + 0.5
				coeffs[i],coeffs[kernelsize-i+1] = num,num
			end
					
			return coeffs
		end
		
		function Viewer.paint.brushweights()
			-- must recompute brush alphas
			if (Viewer.paint.smooth) then
				local gauss = Viewer.paint.smoothtab(Viewer.paint.size)
				for y=0,Viewer.paint.size-1 do
					for x=0,Viewer.paint.size-1 do
						Viewer.paint.brush:index((y*Viewer.paint.size + x)*4 +3,gauss[x+1]*gauss[y+1])
					end
				end
			else
				local cnt = Viewer.paint.size*Viewer.paint.size
				for i=0,cnt-1 do
					Viewer.paint.brush:index(i*4 +3,1.0)
				end
			end
		end
		-- size
		--- must be smaller <= 129
		y = addslider(menu,y,"size:",
			function(val)
				val = math.floor(val+0.1)
				val = 2^(val)
				val = val > 1 and val+1 or val 
				
				
				if (val ~= Viewer.paint.size) then
					Viewer.paint.size = val	
					
					Viewer.paint.brushweights()
					
					
					-- also must reset colors
					Viewer.paint.brush:v3all(Viewer.paint.color[1],Viewer.paint.color[2],Viewer.paint.color[3],0,Viewer.paint.size*Viewer.paint.size,1)	
				end
				
				
				return Viewer.paint.size
			end,
			0.0,7,0,1/7,true)
		-- tex coord
		-- dropdown texcoord
		y = addslider(menu,y,"texcoord:",
			function(val)
				Viewer.paint.coord = math.floor(val+0.1)
				return Viewer.paint.coord
			end,
			0.0,3,0,1/3,true)
		
		y = y + 5		
		
		-- colormenu
		local con = createcolormenu("Color",5,y,menu:getWidth()-10,100,
			{0,0,1,1},
			function (obj,r,g,b,a) 
				Viewer.paint.color = {r,g,b,a}
				Viewer.paint.brush:v3all(r,g,b,0,Viewer.paint.size*Viewer.paint.size,1)
			end,nil, "Opacity:")
		menu:add(con)
		y = y+110
		
		
		-- toggle button "paint"
		Viewer.paint.btnp = menu:add(Button:new(10,y,60,16,"PAINT"))
		Viewer.paint.btnp:setPushable(true)
		function Viewer.paint.btnp:onClicked()
			Viewer.paint.active = self:isPushed()
			Viewer.paintl2d:rfNodraw(not Viewer.paint.active)
			if (not Viewer.collgeom) then
				Viewer.paintl2d:text("\vsprop.collision needed")
				Viewer.paintl2d:pos(180,Viewer.h-45,0)
			end
			
		end	
		Viewer.paint.btnp.statusinfo = "When active you can paint, Toggle via B, collision needed"
		
		addcheckbox(menu,y,"smooth",		
			function(val) 
				Viewer.paint.smooth = val 
				Viewer.paint.brushweights()
			end,
			Viewer.paint.smooth,guiw/2,100)
		
	
		Keyboard.setKeyBinding(Keyboard.keycode.B, 	function() Viewer.paint.btnp:onAction() end, 200)	
	end -- PAINT MENU --------------------------------------
	
	do	-- OPTIONS MENU ---------------------------------------
		local menu = Viewer.optionslist
		local y = 5
		local btnw = 100
		menu:add(Label:new(0,y,guiw,16,"GENERAL",Label.LABEL_ALIGNCENTERED))
		y = y+16
		y = 5 + addbutton(menu,(guiw-btnw)/2,y,btnw,20,"Save Settings",Viewer.storesettings,
			"saves some data (luxf3d filepath...)")
		y = 5 + addbutton(menu,(guiw-btnw)/2,y,btnw,20,"Reload All",Viewer.reloadMain,
			"reloads all resources (F5)")
		y = 5 + addbutton(menu,(guiw-btnw)/2,y,btnw,20,"Reload Materials",Viewer.reloadMat,
			"reloads materials,shaders,textures... (F6)")
		y = 5 + addbutton(menu,(guiw-btnw)/2,y,btnw,20,"Reload Animation",Viewer.reloadAnim,
			"reloads animations (F7)")
		local con = createcolormenu("background",5,y,menu:getWidth()-10,100,
			{0.1,0.1,0.1,1},
			function (obj,r,g,b,a) 
				Viewer.lview.rClear:colorvalue(r*a,g*a,b*a,0)
			end,nil, "Brightness:")
		menu:add(con)
		y = y+100
		y = addslider(menu,y,"mouse:",
			function(val)
				Viewer.controlspeed = (val) 
				return val 
			end,
			Viewer.controlspeed/4,4,0) 
			
		y = y + 5
		
		menu:add(Label:new(0,y,guiw,16,"LUXF3D",Label.LABEL_ALIGNCENTERED))
		y = y+16
		y = 5 + addbutton(menu,(guiw-btnw)/2,y,btnw,20,"Set LuxF3D .exe",
			function(chk)
				local function oncancel(dlg)	return true end
				local function onsave(dlg,path,name)
					Viewer.luxf3d.exefile = path..name
					chk.statusinfo = Viewer.luxf3d.exefile
					return true 
				end
				filedialog("select LuxF3D.exe","set","cancel",ProjectpathFull,onsave,oncancel,{"exe"})	
			end,
			Viewer.luxf3d.exefile or "no .exe set")

		addcheckbox(menu,y,"no f3d",		function(val) Viewer.luxf3d.nof3d = val end,		Viewer.luxf3d.nof3d,10,90)
		y = addcheckbox(menu,y,"no ma",		function(val) Viewer.luxf3d.noma = val end,			Viewer.luxf3d.noma,guiw-100,90)
		y = addcheckbox(menu,y,"tangents",	function(val) Viewer.luxf3d.tangents = val end,		Viewer.luxf3d.tangents)
		y = addcheckbox(menu,y,"fixcylindrical",function(val) Viewer.luxf3d.fixcyl = val end,		Viewer.luxf3d.fixcyl,20)
		y = addslider (menu,y,"tancos",		function(val)Viewer.luxf3d.tancos = val return val end,(Viewer.luxf3d.tancos+1)/2,2,-1,nil,nil,12)
		y = addslider (menu,y,"bincos",		function(val)Viewer.luxf3d.bincos = val return val end,(Viewer.luxf3d.bincos+1)/2,2,-1,nil,nil,12)
		y = addcheckbox(menu,y,"optimize",	function(val) Viewer.luxf3d.optimize = val end,		Viewer.luxf3d.optimize )
		addcheckbox(menu,y,"tristrips",		function(val) Viewer.luxf3d.stripify = val end,		Viewer.luxf3d.stripify,20,90)
		y = addcheckbox(menu,y,"cache16",	function(val) Viewer.luxf3d.cache16 = val end, 		Viewer.luxf3d.cache16,guiw-100,90)
		y = addcheckbox(menu,y,"q3 keepsky",function(val) Viewer.luxf3d.keepsky = val end,		Viewer.luxf3d.keepsky)
		y = addslider(menu,y,"3ds frame:",
			function(val)
				Viewer.luxf3d.frametime = (val) 
				return val 
			end,
			0.3333,100,0)
		y = addslider(menu,y,"3ds nth:",
			function(val)
				val = math.floor(val+0.3)
				Viewer.luxf3d.nthframe = val
				return val 
			end,
			0,24,1,1/23)
	end	-- OPTIONS MENU ---------------------------------------
	
	do	-- CONTROLS MENU --------------------------------------
		local menu = Viewer.controlslist
		local width = 46
		local x = 5
		local function btnadd(name)
			btn = menu:add(Button:new(x,4,width,24,name))
			x = x+width
			return btn
		end
		
		Viewer.controlslist.ctrlobj = false
		
		local btnobj= btnadd("object")
		local btncam= btnadd("camera")
		local btnlight = btnadd("light")
		x = x + 5
		local btnhide = btnadd("hide")
		
		local controlgroup = ButtonGroup.new()
		controlgroup:addButton(btnobj)
		controlgroup:addButton(btncam)
		controlgroup:addButton(btnlight)
	
			
		function btnobj:onClicked() 
			Viewer.controlslist.ctrlobj = true
			Viewer.controlact,Viewer.controll3d,Viewer.controlcam = Viewer.vhandle,Viewer.vnode,false 
			end
		function btncam:onClicked() 
			Viewer.controlslist.ctrlobj = false
			Viewer.controlact,Viewer.controll3d,Viewer.controlcam = Viewer.camact,Viewer.cam,true 
			end
		function btnlight:onClicked() 
			Viewer.controlslist.ctrlobj = false
			Viewer.controlact,Viewer.controll3d,Viewer.controlcam = Viewer.sunact,Viewer.sunl3d,false end
			
		function Viewer.controlobj()
			if (Viewer.controlslist.ctrlobj) then
				controlgroup:click(btnobj)
				btnobj:onClicked()
			end
		end
		
		function btnhide:onClicked() 
			Viewer.hidegui()
			local w,h = window.size()
			if (Viewer.visiblegui) then
				Viewer.lview:windowsized(false)
				Viewer.lview:viewrefbounds(0,30,w-guiw+5,h-30)
				Viewer.cam:aspect((w-guiw+5)/(h-30))
			else
				Viewer.lview:windowsized(true)
				Viewer.cam:aspect(-1)
			end
		end
		
		window._vupdate = window.update
		function window.update()
			window._vupdate()
			local w,h = window.size()
			Viewer.lview:viewrefbounds(0,30,w-guiw+5,h-30)
			if (Viewer.visiblegui) then
				Viewer.cam:aspect((w-guiw+5)/(h-30))
			end
		end
		
		
		btnobj.statusinfo = "Move/Rotate Object (MOUSE 0,1 = move, 2 or 0+1 = rotate, R = Reset)"
		btnlight.statusinfo = "Move/Rotate Light (MOUSE 0,1 = move)"
		btncam.statusinfo = "Move Camera (MOUSE 0 = rotate, 1,2 (0+1) = move)"
		btnhide.statusinfo = "hide GUI"
		Viewer.controlobj()
	end	-- CONTROLS MENU --------------------------------------
	


	-- OBJECT FRAME
	-- model movement
	local mx,my = 0,0
	listener = MouseListener.new(
		function (self,me)
			if (Viewer.collgeom) then
				-- do collision
				-- transform mouse position 
				local wx,wy,wz = Viewer.cam:toworld(me.x,me.y,0.5,Viewer.lview)
				local px,py,pz = Viewer.camact:pos()
				local dx,dy,dz = wx-px, wy-py, wz-pz
				-- transform ray parameters with inverse of model transform
				local mat = Viewer.vhandle:matrix()
				mat:affineinvert()
				px,py,pz = mat:transform(px,py,pz)
				dx,dy,dz = mat:transformrotate(dx,dy,dz)
				-- prep ray
				Viewer.collray:set(px,py,pz, dx,dy,dz)
				
				local function getnearest (count,dist)
					local nearest = count
					for i=1,count-1 do
						nearest = (dist[nearest] > dist[i]) and i or nearest
					end
					return nearest
				end
				
				-- shoot ray
				local count,dist,pos,tris = Viewer.collray:test(Viewer.collgeom,"dpt")

				if (count < 1) then
					Viewer.hitl3d:rfNodraw(true)
					Viewer.hitdata = nil
					Viewer.hitl2d:rfNodraw(true)
				else
					Viewer.hitl3d:rfNodraw(false)
					
					local n = getnearest(count,dist) - 1
					Viewer.hitdata = {}
					Viewer.hitdata.tris = {tris[(n)*3+1],tris[(n)*3+2],tris[(n)*3+3]}
					Viewer.hitdata.pos = {pos[(n)*3+1],pos[(n)*3+2],pos[(n)*3+3]}
					-- transform back to world
					mat:affineinvert()
					Viewer.hitdata.pos = {mat:transform(unpack(Viewer.hitdata.pos))}
					Viewer.hitactor:pos(unpack(Viewer.hitdata.pos))
					Viewer.hitdata.tr = Viewer.colltris[Viewer.hitdata.tris[1]+1]
				
					Viewer.hitl2d:text(string.format("\vs%s\n\t%.3f\n\t%.3f\n\t%.3f",Viewer.hitdata.tr.mesh:name(),unpack(Viewer.hitdata.pos)))
					Viewer.hitl2d:rfNodraw(false)
				end
			end
		
			if (Viewer.paint.active and Viewer.collgeom and Viewer.paint.tex and input.ismousedown(0) and true or false) then 
				if (Component.getMouseLock() == mcon) then
					mcon:unlockMouse()
				end
				if (Viewer.hitdata and Viewer.hitdata.tr.mesh:matsurface():contains(Viewer.paint.tex)) then
					-- find uv coord
					local tr = Viewer.hitdata.tr
					local a,b,c = tr.mesh:indexPrimitive(tr.index)
					local u,v = Viewer.hitdata.tris[2],Viewer.hitdata.tris[3]

					u,v = tr.mesh:vertexTexcoordTris(a,b,c,u,v,Viewer.paint.coord)
					-- round to integer
					u,v = u+Viewer.paint.texwf, v+Viewer.paint.texhf
					u,v = math.floor(u * Viewer.paint.texw),math.floor( v * Viewer.paint.texh)
					
					a = math.floor(Viewer.paint.size/2)
					
					-- update rect around texture and upload
					Viewer.paint.tex:arraydecal(Viewer.paint.brush,u-a,v-a,Viewer.paint.size,Viewer.paint.size,Viewer.paint.color[4],unpack(Viewer.paint.mask))
					Viewer.paint.tex:uploaddata(u-a,v-a,0,Viewer.paint.size,Viewer.paint.size,0)
				
					
					
				end
			
				return
			end
			
			if ((not Viewer.controll3d and  not Viewer.controlcam) or 
				not Viewer.controlact) then return end
		
			local root = Container.getRootContainer()
			if me:isPressed() then
				mx,my = me.x,me.y
				MouseCursor.pos(mx,my)
				mcon:lockMouse()
				return
			end
			if me:isDragged() and not me:isPressed() and Component.getMouseLock() == mcon then
				local dx,dy = me.x-mx,me.y-my
				dx,dy = dx*Viewer.controlspeed,dy*Viewer.controlspeed
				if dx==0 and dy==0 then return end
				MouseCursor.pos(mx,my)
				local xx,xy,xz,yx,yy,yz,zx,zy,zz = Viewer.camact:rotaxis()
				
				xx,xy,xz,yx,yy,yz,zx,zy,zz = xx*Viewer.controlmve,xy*Viewer.controlmve,xz*Viewer.controlmve,yx*Viewer.controlmve,yy*Viewer.controlmve,yz*Viewer.controlmve,zx*Viewer.controlmve,zy*Viewer.controlmve,zz*Viewer.controlmve
				if (not Viewer.controlcam) then
					-- object controls
					if (input.ismousedown(0) and not input.ismousedown(1)) then
						local a,b,c = Viewer.controlact:pos()
						a = a + dx*0.5*xx - dy*0.5*zx
						b = b + dx*0.5*xy - dy*0.5*zy
						c = c + dx*0.5*xz - dy*0.5*zz
						Viewer.controlact:pos(a,b,c)
					end
					if (input.ismousedown(1) and not input.ismousedown(0)) then
						local a,b,c = Viewer.controlact:pos()
						a = a + dx*0.5*xx - dy*0.5*yx
						b = b + dx*0.5*xy - dy*0.5*yy
						c = c + dx*0.5*xz - dy*0.5*yz
						Viewer.controlact:pos(a,b,c)
						dx = 0
					end
			
					
					if (input.ismousedown(2) or (input.ismousedown(1) and input.ismousedown(0))) then
						--local a,b,c = Viewer.controlact:rotdeg()
						--local A,B,C = Viewer.controll3d:localrotdeg()
						--local x,y = math.max(-90,math.min(90,a+dy*0.5)),C+dx*0.5
						--Viewer.controlact:rotdeg(x,b,c)
						--Viewer.controll3d:localrotdeg(A,B,y)
						-- rotation in screenspace
						local actmat = Viewer.controlact:matrix()
						local cammat = Viewer.camact:matrix()
						local rotmat = matrix4x4.new()
						rotmat:rotdeg(dy*0.5,0,dx*0.5)
						cammat:pos(0,0,0)
						cammat:affineinvert()
						rotmat:mul(cammat)
						cammat:affineinvert()
						rotmat:mul(cammat,rotmat)
						local a,b,c = actmat:pos()
						actmat:pos(0,0,0)
						actmat:mul(rotmat,actmat)
						actmat:pos(a,b,c)
						Viewer.controlact:matrix(actmat)
					end
				else
					if (input.ismousedown(0) and not input.ismousedown(1)) then
						local a,b,c = Viewer.controlact:rotdeg()
						local x,y = math.max(-90,math.min(90,a-dy*0.5)),c-dx*0.5
						Viewer.controlact:rotdeg(x,b,y)
					end
					if (input.ismousedown(1) and not input.ismousedown(0)) then
						local a,b,c = Viewer.controlact:pos()
					
						a = a + dx*0.5*xx - dy*0.5*yx
						b = b + dx*0.5*xy - dy*0.5*yy
						c = c + dx*0.5*xz - dy*0.5*yz
						dx = 0
						Viewer.controlact:pos(a,b,c)
					end
					if (input.ismousedown(2) or (input.ismousedown(1) and input.ismousedown(0))) then
						local a,b,c = Viewer.controlact:pos()
						a = a + dx*0.5*xx - dy*0.5*zx
						b = b + dx*0.5*xy - dy*0.5*zy
						c = c + dx*0.5*xz - dy*0.5*zz
						Viewer.controlact:pos(a,b,c)
						dx = 0
					end
				end
			elseif not me:isPressed() then
				mcon:unlockMouse()
			end
		end
	)
	mcon = Container:new(0,0,window.refsize())
	Container.getRootContainer():add(mcon)
	mcon:addMouseListener(listener)
	
	Viewer.activateDynamicMenu("Enviro")
	-- keybindings
	Keyboard.setKeyBinding(Keyboard.keycode.F5, Viewer.reloadMain, 500)
	Keyboard.setKeyBinding(Keyboard.keycode.F7,	Viewer.reloadAnim, 500)
	Keyboard.setKeyBinding(Keyboard.keycode.F6,	Viewer.reloadMat,  500)
	
	Keyboard.setKeyBinding(Keyboard.keycode.R, 	Viewer.resetTransform, 200)
	Keyboard.setKeyBinding(Keyboard.keycode.P, 	Viewer.toggleAnim, 200)
	
	-- viewport
	Viewer.lview:windowsized(false)
	Viewer.lview:viewrefbounds(0,30,w-guiw+5,h-30)
	Viewer.cam:aspect((w-guiw+5)/(h-30))
	Viewer.lview:drawcamaxis(true)
end

Viewer.guifuncs = {}


local function meshdialog()
	Viewer.materialauto = {}
	local frame = modeldialog()
	frame:setBounds(Viewer.w-320,30,320,Viewer.h-60)
	frame:add(Label:new(0,2,frame:getWidth()-50,20,"\v999\vbMeshes",Label.LABEL_ALIGNCENTERED),
		Label.LABEL_ALIGNCENTERED)

	local cancel = frame:add(Button:new(frame:getWidth()-50,3,46,20,"close"))
	function cancel:onClicked()
		 frame:delete()
	end
	
	local selectedmids = {}
	local selectedcnt = 0
	
	local mdl = Viewer.res

	local btnassign = frame:add(Button:new(frame:getWidth()-170,30,160,20,"assign material to selected"))

	local function assignmaterial(matsurf)
		local islit = Viewer.vnode:rfLitSun()
		for i,v in pairs(selectedmids) do
			Viewer.vnode:matsurface(v,matsurf)
		end
		Viewer.vnode:rfLitSun(islit)
	end
	
	local function materialautogui(dlg,path,name)
		
		local frame =  dlg.modal:add(TitleFrame:new(dlg:getX(),dlg:getY()+dlg:getHeight()-4,dlg:getWidth(),80))
		frame:add(Label:new(0,2,frame:getWidth(),20,"\v999\vbMaterial Auto Setup: "..name,Label.LABEL_ALIGNCENTERED),
			Label.LABEL_ALIGNCENTERED)
		y = 22
		Viewer.materialauto.shadername = path..name
		Viewer.materialauto.textures = {}
		Viewer.materialauto.textypes = {"TEX","TEXCUBE","TEXDOTZ","TEXALPHA"}
		for i=1,8 do
			table.insert(Viewer.materialauto.textures,{"unset",1})
		end
		Viewer.materialauto.activetex = 1
		
		local lblout = frame:add(Label:new(10,y,60,20,"Stage:"))
		local lblout = frame:add(Label:new(180,y,60,20,"TexType:"))

		y = y + 16
		local lblouttex = frame:add(Label:new(10,y,frame:getWidth()-10,20,"Texture: unset"))
		y = y - 16
		
		-- comboboxes
		-- first type
		local ddtype = frame:add(ComboBox:new(270,y,70,16))
		for i=1,4 do
			ddtype:addItem(Viewer.materialauto.textypes[i],i)
		end
		function ddtype:onSelected(idx,cap,fn) 
			Viewer.materialauto.textures[Viewer.materialauto.activetex][2] = idx 
		end
		ddtype:select(1)
		
		-- then stage
		local ddstage = frame:add(ComboBox:new(70,y,100,16))
		for i=1,8 do
			ddstage:addItem("Texture:"..(i-1),i)
		end
		function ddstage:onSelected(idx,cap,fn) 
			Viewer.materialauto.activetex = fn
			local tex = Viewer.materialauto.textures[fn]
			lblouttex:setText("Texture: "..tex[1])
			ddtype:select(tex[2])
		end
		ddstage:select(1)
		
		
		y = 80-20
		addbutton(frame,10,y,90,16,"Cancel",
			function() 
				Viewer.materialauto.guiframe = nil
				frame:delete() 
			end,"")
		
		addbutton(frame,(frame:getWidth()-200)/2,y,90,16,"Set Tex",
			function() 
				local fname = Viewer.guipath..Viewer.guifile
				Viewer.materialauto.textures[Viewer.materialauto.activetex][1] = fname
				lblouttex:setText("Texture: "..fname)
			end,"Sets current file as texture for given stage")
			
			

		addbutton(frame,frame:getWidth()-120,y,110,16,"Material_Auto",
		function() 
			-- cleanup
			Viewer.materialauto.guiframe = nil 
			-- build string
			local fname = "MATERIAL_AUTO:"..Viewer.materialauto.shadername.."|"
			for i=1,8 do 
				local tex = Viewer.materialauto.textures[i]
				if (tex[1] == "unset") then
					break
				else
					fname = fname..Viewer.materialauto.textypes[ tex[2] ].." "..tex[1].."|"
				end
			end
			-- apply
			local mats = material.load(fname)
			if (mats) then
				assignmaterial(mats)
			else
				print("Resource not found")
			end
			
			-- delete dialogs
			frame:delete()
			dlg:delete()
			-- TODO delete dialog
		end,"creates MATERIAL_AUTO")
	
		return frame			
	end
	
	local function onsave(dlg,path,filename)
		if (Viewer.materialauto.guiframe) then return false end
		if (checkextension(filename,{"shd"})) then
			Viewer.materialauto.guiframe = materialautogui(dlg,path,filename)
			return false
		end

		local mats = nil
		if (checkextension(filename,{"mtl"})) then
			mats = material.load(filename)
		else 
			mats = texture.load(filename)
		end
		
		if (mats == nil) then
			print("Resource not found")
		else
			assignmaterial(mats)
		end
		
		return true
	end
	
	local function oncancel(dlg)
		if (Viewer.materialauto.guiframe) then
			Viewer.materialauto.guiframe:delete()
		end
		return true
	end
	
	function btnassign:onClicked()
		if (selectedcnt > 0) then
			Viewer.materialauto.guiframe = nil
			filedialog("choose material/texture/shader","load","cancel",ProjectpathFull,onsave,oncancel,{"dds","jpg","png","tga","mtl","shd"})
		end
	end

	
	frame:add(Label:new(8,42,84,20,"Visible | Selected"))
	
	if (not Viewer.islevelmodel) then
		local meshes = frame:add(scrollfield(10,60,frame:getWidth()-20,frame:getHeight()-70))
		meshes.area:deleteChilds()
		meshes.area:setSize(meshes.area.parent:getWidth(),Viewer.res:meshcount()*20)
		
		
		for i=0,Viewer.res:meshcount()-1 do
			local mid = Viewer.res:meshid(i)
			
			
			local chk = meshes.area:add(Checkbox:new(-6,i*20,32,20,""))
			function chk:onClicked(state)
				Viewer.vnode:rfNodraw(mid,not state)
			end
			chk:setPushState(not Viewer.vnode:rfNodraw(mid))
			
			local chk = meshes.area:add(Checkbox:new(18,i*20,meshes.area.parent:getWidth()-20,20,i..": "..mid:name()))
			function chk:onClicked(state)
				selectedmids[mid] = state and mid or nil
				selectedcnt = selectedcnt + (state and 1 or -1)
			end
		end
	else
		frame:add(Label:new(0,50,frame:getWidth(),20,"Not available for l3dlevelmodel",Label.LABEL_ALIGNCENTERED),
			Label.LABEL_ALIGNCENTERED)
	end

end

Viewer.guifuncs["mdl"] = function()
	Viewer.proplist:deleteChilds()
	local menu = Viewer.proplist
	local y = 5
	local lbl = menu:add(Label:new(0,y,guiw,16,"INFO",Label.LABEL_ALIGNCENTERED))
	y = y+16
	
	local btncoll = menu:add(Button:new(90,y,100,16,"gen.collision"))
	btncoll.statusinfo = "generates collision data, allows painting"
	function btncoll:onClicked()
		if (Viewer.collgeom) then
			Viewer.collgeom:delete()
		end
		local gdata,hitmesh,rawdata = UtilFunctions.getModelColMesh(Viewer.res ,true,nil,true,true)
		Viewer.colltris = rawdata.tris
		Viewer.collgeomtri = dgeomtrimeshdata.new(rawdata.inds,rawdata.verts)
		Viewer.collgeom = dgeomtrimesh.new(Viewer.collgeomtri)
		Viewer.collgeom:raytrisresult(true)
		
		Viewer.paintl2d:text("\vsPAINTMODE")
		Viewer.paintl2d:pos(268,Viewer.h-45,0)
	end
	
	local minx,miny,minz,maxx,maxy,maxz = Viewer.res:bbox()
	menu:add(Label:new(10,y,guiw-10,60,string.format("ModelBBox:\n  min(%.2f,%.2f,%.2f)\n  max(%.2f,%.2f,%.2f)\n  size(%.2f,%.2f,%.2f)",
					minx,miny,minz,maxx,maxy,maxz,maxx-minx,maxy-miny,maxz-minz)))
	y = y+60
	
	local btnhide = menu:add(Button:new(90,y+5,100,16,"meshes options"))
	btnhide.statusinfo = "show/hide meshes or change materials"
	function btnhide:onClicked()
		meshdialog()
	end
	
	
	
	-- count vertices and tris
	local tris,verts = 0,0
	
	for i=0,Viewer.res:meshcount()-1 do
		local mid = Viewer.res:meshid(i)
		tris = tris + mid:indexTrianglecount()
		verts = verts + mid:vertexCount()
	end
	
	menu:add(Label:new(10,y,guiw-10,70,string.format("Stats:\n  meshcount: %d bonecount: %d\n  skincount: %d\n  vertices: %d tris: %d",
				Viewer.res:meshcount(),Viewer.res:bonecount(),Viewer.res:skincount(),verts,tris)))

					
	y = y+70
	y = y + 5
	local litchk
	menu:add(Label:new(0,y,guiw,16,"DEBUG",Label.LABEL_ALIGNCENTERED))
	y = y+16
	y = addcheckbox(menu,y,"wireframe",
		function(val) Viewer.vnode:rfWireframe(val) end)
	y = addcheckbox(menu,y,"bones",
		function(val) render.drawbones(val) end,render.drawbones())
	y = addcheckbox(menu,y,"bonesnames",
		function(val) render.drawbonenames(val) end,render.drawbonenames(),20)
	y = addcheckbox(menu,y,"normals (and tangents)",
		function(val) render.drawnormals(val) end, render.drawnormals())
	y,litchk = addcheckbox(menu,y,"lit",
		function(val) Viewer.vnode:rfLitSun(val) end,true)
	Viewer.modeldata.matsoff = false
	y = addcheckbox(menu,y,"vertexcolors",
		function(val) 
			 if (Viewer.modeldata.matsoff == val or Viewer.islevelmodel) then return end
			 if (val) then
			 	for i=0,Viewer.res:meshcount()-1 do
					local mid = Viewer.res:meshid(i)
					Viewer.vnode:matsurface(mid,matsurface.vertexcolor())
				end
			 else
			 	local litsun = Viewer.vnode:rfLitSun()
			 	local wire = Viewer.vnode:rfWireframe()
				 for i=0,Viewer.res:meshcount()-1 do
					local mid = Viewer.res:meshid(i)
					Viewer.vnode:matsurface(mid,mid:matsurface())
				end
				Viewer.vnode:rfLitSun(litsun)
				Viewer.vnode:rfWireframe(wire)
			 end
			 Viewer.modeldata.matsoff = val
		end)
	render.normallength(Viewer.noderadius*0.01)

	y = y + 4
	local lbl = menu:add(Label:new(0,y,guiw,16,"METHOD",Label.LABEL_ALIGNCENTERED))
	lbl.statusinfo = "l3dmodel supports animation, l3dlevelmodel lightmaps (but less debug info)"

	y = y+18
	local function prim(createfn,lit)
		local x,y,z = Viewer.vnode:localpos()
		local ctrl = Viewer.controll3d == Viewer.vnode
		Viewer.vnode:delete()
		Viewer.vnode = createfn()
		Viewer.vnode:linkinterface(Viewer.vhandle)
		Viewer.vnode:rfLitSun(lit)
		Viewer.vnode:rfFog(true)
		Viewer.vnode:uselocal(true)
		Viewer.vnode:localpos(x,y,z)
		Viewer.controll3d = ctrl and Viewer.vnode or  Viewer.controll3d
	end
	
	local ddmesh = menu:add(ComboBox:new(40,y,110,16))
	ddmesh:addItem("l3dmodel",
		function () 
			prim(function()	
				litchk:setPushState(true)
				Viewer.islevelmodel = false
				return l3dmodel.new("l3dviewer",Viewer.layerid,Viewer.res,true,false)
				end,true)
		end)
	ddmesh:addItem("l3dlevelmodel",
		function () 
			prim(function()	
				local cubesize = Viewer.modeldata.cubesize
				print("blah")
				local minx,miny,minz,maxx,maxy,maxz = Viewer.res:bbox()
				local sizex,sizey,sizez = maxx-minx,maxy-miny,maxz-minz
				sizex,sizey,sizez = math.max(1,math.floor(sizex/cubesize)),math.max(1,math.floor(sizey/cubesize)),math.max(1,math.floor(sizez/cubesize))
				Viewer.islevelmodel = true
				litchk:setPushState(false)
				return l3dlevelmodel.new("l3dviewer",Viewer.res,sizex,sizey,sizez)
				end,false)
		end)
	function ddmesh:onSelected(idx,cap,fn) fn() end
		
	local slider
	y = y +16
	y,slider = addslider(menu,y,"cubesize:",
		function(val)
			val = math.floor(val+0.3)
			val = 2^val
			Viewer.modeldata.cubesize = val
			return val
		end,
		0.65,14,0,1/13,true)
	slider.statusinfo = "separator cube size for levelmodel"
	
	y,slider = addslider(menu,y,"brightness:",
		function(val)
			if (Viewer.islevelmodel) then
				val = math.floor(val+0.3)
				val = 2^val
				for i=0,Viewer.vnode:lightmapcount()-1 do
					local tex = Viewer.vnode:lightmap(i)
					tex:lightmapscale(val)
				end
			else
				val = 1
			end
			return val
		end,
		0.0,2,0,1/2,true)
	slider.statusinfo = "lightmap brightness multiplier"
	
	y = y + 4
	menu:add(Label:new(0,y,guiw,16,"ANIMATION",Label.LABEL_ALIGNCENTERED))
	y = y+18
	local btnload= menu:add(Button:new(10,y,80,16,"load & apply"))
	
	local btnplay = menu:add(Button:new(100,y,80,16,"play/stop"))
	btnplay.statusinfo = "play/stop animation (P)"
	function btnplay:onClicked()
		Viewer.toggleAnim()
	end
	y = y + 18
	
	local sldstart,sldend
	

	y = addslider(menu,y,"speed:",
			function(val)
				Viewer.vnode:seqSpeed(0,val)
				return val
			end,
			0.25,4,0)
	y,sldstart = addslider(menu,y,"start:",
			function(val)
				if (not Viewer.anim) then return 0 end
				local atime = Viewer.anim:length()*val
				local oldtime = Viewer.vnode:seqAnimstart(0,0)
				Viewer.vnode:seqAnimstart(0,0,atime)
				local leng = Viewer.vnode:seqAnimlength(0,0)
				leng = math.max(1,oldtime-atime+leng)
				Viewer.vnode:seqAnimlength(0,0,leng)
				Viewer.vnode:seqOffset(0,0) -- will just take anim0's length as duration
				return atime
			end,
			0.0,1,0)
			
	y,sldend = addslider(menu,y,"end:",
			function(val)
				if (not Viewer.anim) then return 0 end
				local atime = Viewer.anim:length()*val
				local starttime = Viewer.vnode:seqAnimstart(0,0)
				local leng = math.max(1,atime-starttime)
				Viewer.vnode:seqAnimlength(0,0,leng)
				Viewer.vnode:seqOffset(0,0) -- will just take anim0's length as duration
				return starttime+leng
			end,
			1.0,1,0)
	menu:add(Label:new(10,y,70,20,"current time:"))
	Viewer.animtime = menu:add(Label:new(90,y,100,20,"",Label.LABEL_ALIGNRIGHT))
			
	local function onsave(dlg,path,filename)
		Viewer.reloadAnim(path..filename)
		sldstart:setSliderPos(0)
		sldend:setSliderPos(1)
		return true
	end
	
	local function oncancel(dlg)
		return true
	end
	
	function btnload:onClicked()
		if (not Viewer.islevelmodel) then
			filedialog("choose animation","load","cancel",ProjectpathFull,onsave,oncancel,{"ma"})
		end
	end
end

Viewer.guifuncs["prt"] = function()
	Viewer.proplist:deleteChilds()
	local menu = Viewer.proplist
	local y = 5
	menu:add(Label:new(0,y,guiw,16,"EMITTER",Label.LABEL_ALIGNCENTERED))
	y = y+16
	local ddmesh = menu:add(ComboBox:new(40,y,110,16))
	ddmesh.hasdefault = true
	
	ddmesh:addItem("default",
		function () 
		end)
	ddmesh:addItem("point",
		function () 
			if (ddmesh.hasdefault) then
				ddmesh.hasdefault = nil
				ddmesh:removeItem(1)
			end
			Viewer.vnode:typepoint()
		end)
	ddmesh:addItem("circle",
		function () 
			if (ddmesh.hasdefault) then
				ddmesh.hasdefault = nil
				ddmesh:removeItem(1)
			end
			Viewer.vnode:typecircle(Viewer.vnode:size())
		end)
	ddmesh:addItem("rectangle ",
		function () 
			if (ddmesh.hasdefault) then
				ddmesh.hasdefault = nil
				ddmesh:removeItem(1)
			end
			Viewer.vnode:typerectangle (Viewer.vnode:size(),Viewer.vnode:height())
		end)
	ddmesh:addItem("rectanglelocal",
		function () 
			if (ddmesh.hasdefault) then
				ddmesh.hasdefault = nil
				ddmesh:removeItem(1)
			end
			Viewer.vnode:typerectanglelocal (Viewer.vnode:size(),Viewer.vnode:height())
		end)
	function ddmesh:onSelected(idx,cap,fn) fn() end
	y = y + 20
	y = addslider(menu,y,"sizeA:",
			function(val)
				Viewer.vnode:size(val) 
				return val
			end,
			math.min(1,Viewer.vnode:size()/1000),1000,0)
			
	y = addslider(menu,y,"sizeB:",
			function(val)
				Viewer.vnode:height(val)
				return val
			end,
			math.min(1,Viewer.vnode:height()/1000),1000,0)
	y = addslider(menu,y,"spreadin:",
			function(val)
				Viewer.vnode:spreadin (val)
				return val
			end,
			Viewer.vnode:spreadin()/(math.pi*2),(math.pi*2),0)
	y = addslider(menu,y,"spreadout:",
			function(val)
				Viewer.vnode:spreadout (val)
				return val
			end,
			Viewer.vnode:spreadout()/(math.pi*2),(math.pi*2),0)
	y = addslider(menu,y,"rate:",
			function(val)
				Viewer.vnode:rate(val)
				return val
			end,
			math.min(1,Viewer.vnode:rate()/4000),4000,0)
	y = addslider(menu,y,"startage:",
			function(val)
				Viewer.vnode:startage(val)
				return val
			end,
			math.min(1,Viewer.vnode:startage()/4000),4000,0)
	y = addslider(menu,y,"velocity:",
			function(val)
				Viewer.vnode:velocity(val)
				return val
			end,
			math.min(1,Viewer.vnode:velocity()/1000),1000,0)
	y = addslider(menu,y,"velocityvar:",
			function(val)
				Viewer.vnode:velocityvar(val)
				return val
			end,
			math.min(1,Viewer.vnode:velocityvar()/1000),1000,0)
	y = addslider(menu,y,"offset:",
			function(val)
				Viewer.vnode:velocityvar(val)
				return val
			end,
			math.min(1,Viewer.vnode:maxoffsetdist()/1000),1000,0)
	y = addcheckbox(menu,y,"showaxis",
		function(val) 
			for i,v in pairs(Viewer.vnode.childs) do
				v:rfNodraw(not val)
			end
		end,true)
		
	y = y + 5
	menu:add(Label:new(0,y,guiw,16,"SYSTEM",Label.LABEL_ALIGNCENTERED))
	y = y+16
	y = addslider(menu,y,"timescale:",
			function(val)
				Viewer.res:timescale(val)
				return val
			end,
			0.25,4,0)
	y = addcheckbox(menu,y,"globalaxis",
		function(val) Viewer.res:useglobalaxis(val) end,false)
	y = addcheckbox(menu,y,"forceinstance",
		function(val) Viewer.res:forceinstance(val) end,false)
	y = addcheckbox(menu,y,"nogpu",
		function(val) Viewer.res:nogpu(val) end,false)
	y = addcheckbox(menu,y,"probability",
		function(val) Viewer.res:useprobability(val) end,false)
	y = addslider(menu,y,"prob.value:",
			function(val)
				Viewer.res:probability(val)
				return val
			end,
			1,1,0)
	y = addslider(menu,y,"prob.fade:",
			function(val)
				Viewer.res:probabilityfadeout(val)
				return val
			end,
			0,1,0)
	y = addcheckbox(menu,y,"sizemultiplier",
		function(val) Viewer.res:usesizemul(val) end,false)
	y = addslider(menu,y,"sizemul:",
		function(val)
			Viewer.res:sizemul(val)
			return val
		end,
		0.25,4,0)
	y = addcheckbox(menu,y,"volume",
		function(val) Viewer.res:usevolume(val) end,false)
	y = addslider(menu,y,"vol.caminf:",
		function(val)
			Viewer.res:volumecaminfluence(val,val,val)
			return val
		end,
		1,1,0)
	y = addslider(menu,y,"vol.camdist:",
		function(val)
			Viewer.res:volumedistance(val)
			return val
		end,
		0.125,2048,0)
	y = addslider(menu,y,"vol.size:",
		function(val)
			Viewer.res:volumesize(val,val,val)
			return val
		end,
		0.125,4096,0)
end

Viewer.guifuncs["mtl"] = function()
	Viewer.proplist:deleteChilds()
	
	local menu = Viewer.proplist
	local y = 5
	local lbl = menu:add(Label:new(0,y,guiw,16,"MATERIAL",Label.LABEL_ALIGNCENTERED))
	y = y + 16
	menu:add(Label:new(10,y,40,16,"mesh:"))
	
	local function prim(createfn,size)
		local lit = Viewer.vnode:rfLitSun()
		local ctrl = Viewer.controll3d == Viewer.vnode
		Viewer.vnode:delete()
		Viewer.vnode = createfn("mtl",size)
		Viewer.vnode:matsurface(Viewer.res)
		Viewer.vnode:linkinterface(Viewer.vhandle)
		Viewer.vnode:rfLitSun(lit)
		Viewer.vnode:rfFog(true)
		Viewer.vnode:uselocal(true)
		Viewer.controll3d = ctrl and Viewer.vnode or  Viewer.controll3d
	end
	
	local ddmesh = menu:add(ComboBox:new(70,y,80,16))
	ddmesh:addItem("cube",
		function () 
			prim(l3dprimitive.newbox,Viewer.materialdata.size)
		end)
	ddmesh:addItem("sphere",
		function () 
			prim(l3dprimitive.newsphere,Viewer.materialdata.size/2)
		end)
	ddmesh:addItem("cylinder",
		function () 
			prim(l3dprimitive.newcylinder,Viewer.materialdata.size/2)
		end)
	ddmesh:addItem("quad",
		function () 
			prim(l3dprimitive.newquadcentered,Viewer.materialdata.size)
		end)
	function ddmesh:onSelected(idx,cap,fn) fn() end
	y = y + 16 + 5
	
	local lbl = menu:add(Label:new(0,y,guiw,16,"DEBUG",Label.LABEL_ALIGNCENTERED))
	y = y + 16 + 5
	y = addcheckbox(menu,y,"lit",
		function(val) Viewer.vnode:rfLitSun(val) end,true)
end


function Viewer.update_gui()
	Viewer.proplist:deleteChilds()
	Viewer.proplist:add(Label:new(10,5,100,30,"No resource loaded"))
end

function Viewer.initbase()
	SetProjectPath()

	confignosave("window_width")
	confignosave("window_height")
	-- memory stuff
	reschunk.destroydefault()
	
	window.refsize(window.width(),window.height())
	Viewer.w,Viewer.h = window.refsize()
	
	local settings = dofile("viewer_settings.lua")
	
	-- we try to keep them in their natrual dependency order
												--	m,	a,	t,	mt,	s,	g,	pc,	ps,	tb, tr,	sd
	Viewer.geochunk = reschunk.new("geometry",32,	-1,	0,	0,	0,	0,	0,	-1,	-1,	-1, -1,	-1)
	Viewer.animchunk = reschunk.new("anims",8,		0,-1,	0,	0,	0,	0,	-1,	-1,	-1, -1,	-1)
	Viewer.matchunk = reschunk.new("materials",4,	0,	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1, -1,	-1)
		
	-- our primary chunk, all other resources get "bubbled" up
	resdata.filltonext()
	Viewer.geochunk:activate()
	
	-- set up scene environment
	
	-- setup viewport
	Viewer.lview = l3dset.get(0):getdefaultview()
	Viewer.layerid = l3dset.get(0):layer(0)
	
	UtilFunctions.simplerenderqueue(Viewer.lview)
	
	Viewer.lview.rClear:colorvalue(unpack( settings and settings.bgcolor or {0.1,0.1,0.1,0}))
	
	-- sun
	Viewer.sunl3d = l3dlight.new("sun",Viewer.layerid)
	Viewer.sunl3d:makesun()
	Viewer.sunl3d:ambient(0.2,0.2,0.2,1)
	Viewer.sunl3d:diffuse(1.1,1.1,1.1,1)
	Viewer.sunact = actornode.new("sun")
	Viewer.sunl3d:linkinterface(Viewer.sunact)
	Viewer.sunact:pos(0,0,0)
	
	Viewer.sunsphere = l3dprimitive.newsphere("sunsphere",l3dset.get(0):layer(1),4)
	Viewer.sunsphere:rfBlend(true)
	Viewer.sunsphere:rsBlendmode(blendmode.add())
	Viewer.sunsphere:linkinterface(Viewer.sunact)
	
	
	-- hitpoint
	Viewer.hitl3d = l3dprimitive.newsphere("hitpoint",l3dset.get(0):layer(1),2)
	Viewer.hitactor = actornode.new("hit")
	Viewer.hitl3d:linkinterface(Viewer.hitactor)
	Viewer.hitl3d:color(1,1,0,0)
	Viewer.hitactor:pos(0,0,0)

	Viewer.hitl2d = l2dtext.new("hit","test")
	Viewer.hitl2d:parent(l2dlist.getroot())
	Viewer.hitl2d:pos(40,Viewer.h-40,0)
	Viewer.hitl2d:size(8)
	Viewer.hitl2d:rfNodraw(true)
	
	Viewer.paintl2d = l2dtext.new("pt","\vsPAINTMODE")
	Viewer.paintl2d:parent(l2dlist.getroot())
	Viewer.paintl2d:color(1,0.5,0,1)
	Viewer.paintl2d:size(12)
	Viewer.paintl2d:pos(268,Viewer.h-45,0)
	Viewer.paintl2d:rfNodraw(true)
	
	-- default handle node
	Viewer.vhandle = actornode.new("viewer",true,0,0,0)
	
	-- camera
	Viewer.cam = l3dcamera.default()
	Viewer.camact = actornode.new("viewercam",true,-300,-500,300)
	Viewer.camact:lookat(0,0,0,0,0,1)
	Viewer.cam:linkinterface(Viewer.camact)
	Viewer.cam:backplane(4096)	
	Viewer.cam:uselocal(true)
		
	


	
	-- misc data
	Viewer.lastpath = ProjectpathFull
	
	Viewer.restype = "none"
	Viewer.move = false
	Viewer.movedata = {}
	Viewer.movedata.radius = 10
	Viewer.movedata.speed = 1
	
	Viewer.controlspeed = settings and settings.controlspeed or 0.5
	Viewer.controlmve = 1
	
	Viewer.modeldata = {}
	Viewer.modeldata.animspeed = 1
	Viewer.modeldata.cubesize = 512
	
	Viewer.particledata = {}
	Viewer.particledata.norestarts = true
	Viewer.shaderdata = {}
	Viewer.materialdata = {}
	Viewer.materialdata.size = 500
	
	-- collision
	Viewer.collray = dgeomray.new(65536)
	
	Viewer.luxf3d = {}
	-- defaults
	Viewer.luxf3d.outdir = nil
	Viewer.luxf3d.outtexdir = nil
	Viewer.luxf3d.nof3d = false
	Viewer.luxf3d.noma = false
	Viewer.luxf3d.tangents = false
	Viewer.luxf3d.fixcyl = false
	Viewer.luxf3d.tancos = 0.1
	Viewer.luxf3d.bincos = 0.1
	Viewer.luxf3d.optimize = false
	Viewer.luxf3d.stripify = false
	Viewer.luxf3d.cache16 = false
	Viewer.luxf3d.keepsky = false
	Viewer.luxf3d = settings and settings.luxf3d or Viewer.luxf3d
	if (Viewer.luxf3d.exefile and not (FileSystem.attributes(Viewer.luxf3d.exefile))) then
		Viewer.luxf3d.exefile = nil
	end
	Viewer.luxf3d.convextensions = {"bsp","ms3d","3ds","md3","obj","b3d","fbx","dae"}

	Viewer.luxf3d.swsign = 1
	Viewer.luxf3d.swaxis = 1
	Viewer.luxf3d.swrevert = 0
	Viewer.luxf3d.swflip = 0
	Viewer.initgui()
	
	
	Timer.set("viewer",Viewer.viewerthink)
end

function Viewer.storesettings()
	local settings = {}
	settings.luxf3d = Viewer.luxf3d
	settings.controlspeed = Viewer.controlspeed
	settings.bgcolor = {Viewer.lview.rClear:colorvalue()}
	local fp = assert(io.open("base/scripts/luxmodules/luxinialuacore/viewer_settings.lua","w"))
	fp:write(UtilFunctions.serialize(settings))
	fp:close()
end

function Viewer.update_camera()
	if (not Viewer.vnode) then return end
	
	local minx,miny,minz,maxx,maxy,maxz = Viewer.vhandle:vistestbbox()
	local dimx,dimy,dimz = maxx-minx,maxy-miny,maxz-minz
	
	--Viewer.vnode:localpos(-dimx/2-minx,-dimy/2-miny,-dimz/2-minz)
	Viewer.noderadius = math.max(dimz,math.max(dimy,math.max(0,dimx)))
	if (Viewer.restype == "prt") then
		Viewer.noderadius = Viewer.noderadius * 0.01
		local sz = Viewer.noderadius*0.1
		for i,v in pairs (Viewer.vnode.childs) do
			v:size(sz/8,sz/8,sz)
		end
		Viewer.vnode.childs[1]:localpos(sz/2,0,0)
		Viewer.vnode.childs[2]:localpos(0,sz/2,0)
		Viewer.vnode.childs[3]:localpos(0,0,sz/2)
	end
	
	-- first find current camera position
	local cx,cy,cz = Viewer.camact:pos()
	local dist = ExtMath.v3dist({cx,cy,cz},{Viewer.vhandle:pos()})
	local ctab,l = ExtMath.v3normalize({cx,cy,cz})
	local dx,dy,dz = unpack(ctab)
	dx,dy,dz = -dx,-dy,-dz
	local offset = dist-(math.min(Viewer.cam:backplane()*0.6,2*Viewer.noderadius))

	Viewer.camact:pos(unpack(ExtMath.v3add({cx,cy,cz},{dx*offset,dy*offset,dz*offset})))
	Viewer.sunact:pos(1.1*maxx,1.1*miny,1.1*maxz)
	Viewer.sunsphere:size(Viewer.noderadius*0.01,Viewer.noderadius*0.01,Viewer.noderadius*0.01)
	Viewer.hitl3d:size(Viewer.noderadius*0.005,Viewer.noderadius*0.005,Viewer.noderadius*0.005)

	--Viewer.vhandle:vistestbbox(-dimx/2,-dimy/2,-dimz/2,dimx/2,dimy/2,dimz/2)
	
	Viewer.controlmve = Viewer.noderadius*0.01
end

function Viewer.update_anim()
	if (not Viewer.anim or Viewer.islevelmodel) then return end
	local atime = Viewer.anim:length()
	Viewer.vnode:seqTime(0,0)
	Viewer.vnode:seqAnimsource(0,0,Viewer.anim)
	Viewer.vnode:seqAnimstart(0,0,0)
	Viewer.vnode:seqAnimlength(0,0,atime)
	Viewer.vnode:seqOffset(0,0) -- will just take anim0's length as duration
	Viewer.vnode:seqLoop(0,true)
	Viewer.vnode:seqState(0,1)
end

function Viewer.loadres(name,animname)
	Viewer.update_gui()
	if (Viewer.collgeom) then 
		Viewer.collgeom:delete() 
		Viewer.collgeom = nil 
	end
	--Viewer.activateDynamicMenu ("Prop")
	Viewer.name = name
	Viewer.animname = animname
	Viewer.islevelmodel = false
	Viewer.anim = nil
	Viewer.res = nil
	Viewer.prtres = nil
	Viewer.anim = nil
	Viewer.prton = nil
	Viewer.vhandle:vistestbbox(-1,-1,-1,1,1,1)
	local ismodel = false
	if (Viewer.vnode) then
		return Viewer.reloadMain()
	end
	if (name) then
		Viewer.vhandle:pos(0,0,0)
		if (string.find(name,".prt")) then
			Viewer.res = particlesys.load(name)
			if (not Viewer.res) then
				LuaConsole.print("resource not found")
				return 
			end
			Viewer.prtres = Viewer.res
			Viewer.vnode = l3dpemitter.new("l3dviewer",Viewer.res,Viewer.layerid)
			Viewer.vnode:linkinterface(Viewer.vhandle)
			Viewer.vhandle:rotdeg(90,0,0)
			Viewer.vnode:start(200)
			Viewer.vnode:uselocal(true)
			
			local r,g,b = l3dprimitive.newcylinder("l3daxis",1,1,4),l3dprimitive.newcylinder("l3daxis",1,1,4),l3dprimitive.newcylinder("l3daxis",1,1,4)
			r:parent(Viewer.vnode)
			r:color(1,0,0,1)
			r:localrotdeg(0,90,0)
			g:parent(Viewer.vnode)
			g:color(0,1,0,1)
			g:localrotdeg(90,0,0)
			b:parent(Viewer.vnode)
			b:color(0,0,1,1)
			Viewer.vnode.childs = {r,g,b}
			
			Viewer.restype = "prt"
			Viewer.prton = true
		elseif (string.find(name,".mtl")) then
			Viewer.res = material.load(name)
			if (not Viewer.res) then
				LuaConsole.print("resource not found")
				return 
			end
			Viewer.vnode = l3dprimitive.newbox("pic",Viewer.materialdata.size)
			Viewer.vnode:matsurface(Viewer.res)
			Viewer.vnode:linkinterface(Viewer.vhandle)
			Viewer.vnode:rfLitSun(true)
			Viewer.vnode:rfFog(true)
			Viewer.vnode:uselocal(true)
			Viewer.vhandle:rotdeg(0,0,0)
			Viewer.restype = "mtl"
		else
			ismodel = true
			
			Viewer.res = model.load(name,true,true,true,true) 
			if (not Viewer.res) then
				LuaConsole.print("resource not found")
				return 
			end
			Viewer.vnode = l3dmodel.new("l3dviewer",Viewer.layerid,Viewer.res,true,false)
			Viewer.vnode:rfLitSun(true)
			Viewer.vnode:rfFog(true)
			Viewer.vnode:linkinterface(Viewer.vhandle)
			Viewer.vhandle:rotdeg(0,0,0)
			Viewer.vnode:uselocal(true)
			Viewer.restype = "mdl"
			
		end
	else
		return
	end
	
	if (ismodel and animname and string.find(animname,".ma")) then
		
		Viewer.anim = animation.load(animname,true,false)
		if (not Viewer.anim) then
			Viewer.animname = nil
			LuaConsole.print("resource not found")
		else
			Viewer.update_anim()
		end
	end
	Viewer.controlobj()
	Viewer.update_camera()

	Viewer.guifuncs[Viewer.restype]()
end

function Viewer.viewerthink()
	if (Viewer.move) then
		local fracc = Viewer.speed * luxinia.time() * 0.001
		Viewer.vhandle:pos(math.cos(fracc)*Viewer.radius,(math.sin(fracc)*Viewer.radius),0)
	end
	if (Viewer.anim) then
		Viewer.animtime:setText(""..Viewer.vnode:seqTime(0))
	end
end

