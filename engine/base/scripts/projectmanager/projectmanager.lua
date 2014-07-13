-- projectmanager
local root = Container.getRootContainer()
local refw,refh = window.width(),window.height()
window.refsize(refw,refh)
local rcmd = l2dlist.getrcmdclear()
rcmd:color(true)
rcmd:colorvalue(0.9,0.9,0.9,1)
MouseCursor.showMouse(true)
--root:setSize(refw,refh)
do
	local texmem = {n = 0}
	function texmem.get(path)
		local id = texmem.n + 1

		for i=1,texmem.n do
			if texmem[i].isunused then
				texmem[i].isunused = false
				id = i
				break
			end
		end

		texmem.n = math.max(texmem.n,id)

		if not texmem[id] then
			texmem[id] = texture.create2d("projectman-tex-"..id,64,64,textype.rgb(),false)
		end
		texmem[id]:reload(path,true)
		return texmem[id]
	end
	function texmem.release(tex)
		tex.isunused = true
	end

	local function deinit(path)
		root:deleteChilds()
		window.refsize(640,480)
		rcmd:color(false)
		MouseCursor.showMouse(false)
		if not path  then return end
		reschunk.getdefault():clearload(-1)
		local err = loadproject(path)
		print ("Loading:",path)
		if (err) then
			window.refsize(refw,refh)
			root:setLocation((w-refw)/2,(h-refh)/2)
			console.active(true)
			print("The projectpath ",path," could not be loaded. Sorry. Error was:")
			print(err)
			dofile("projectmanager.lua")
		else

		end
	end

	local w = 300
	local h = 400
	local menu = {} --root:add(TitleFrame:new(refw/2,refh,0,h,nil,system.version()))

	local opaque,trans,black = {1,1,1,1},{1,1,1,0},{0,0,0,1}

	function menu:setPage(con,title)
		if self.active == title then return end
		self.active = title

		for i,v in ipairs(root.components) do
			v:fadeTo(trans,trans,trans,0.1,nil,function () v:delete() end)
			v:moveToRect(refw,refh*0.5,0,0,0.2)
		end

		local w,h = con:getSize()
		w,h = w + con:getX()*2,h + 38
		local x,y,w,h = math.floor((refw-w)*.5),math.floor((refh-h)*.5),w,h
		local frame = root:add(TitleFrame:new(0,y+h*.5,0,0,nil,title),1)

		frame:add(con)
		frame:setColor(trans,trans,trans)
		frame:moveToRect(x,y,w,h,0.2)
		frame:fadeTo(opaque,black,opaque,0.2)
	end

	function menu:showProjCompMenu()
		local con = Container:new(10,28,600,360)
		con:add(Label:new(0,0,160,20,"Projectlist"))
		local list = con:add(ListBox:new(0,20,160,con:getHeight()-45))

		local lab = con:add(Label:new(list:getWidth()+10,20,con:getWidth()-list:getWidth()-10,50,""))
		lab:setText(lab:wrapLine(
			"Chose a project and compile it. Every file that has a .lua "..
			"extension will be compiled into a file with the same name. The project will be "..
			"copied to a new directory with the the extension '_bin'. .svn and .cvs directories will be ignored."))

		local back = con:add(Button:new(0,con:getHeight()-20,120,20,"Back to mainmenu"))
		function back:onClicked()
			menu:showProjects()
		end

		local info = Label:new(list:getRight()+10,lab:getBottom()+10,con:getWidth()-list:getRight()-10,60)
		con:add(info)
		local compile = con:add(Button:new(info:getRight()-100,info:getBottom(),100,20,"Compile"))

		menu:setPage(con,"Project compiler")

		local projects = {}

		local log = con:add(Label:new(list:getRight()+10,compile:getBottom(),info:getWidth()-10,con:getHeight()-compile:getBottom()-20))
		local scroll = con:add(Slider:new(log:getRight(),log:getY(),10,log:getHeight(),true))

		function scroll:onValueChanged(value)
			if scroll.disabledevent then return end
			local nmax = math.floor(log:maxLineCount()-1.5)
			local to = math.ceil((#log.text-nmax)*value+.5+nmax)
			log:toLine(to)
		end
		log:setAlignment(Label.LABEL_ALIGNLEFT,Label.LABEL_ALIGNTOP)
		log.text = {}
		function log:append(str)
			str = log:wrapLine(str)
			for line in (str.."\n"):gmatch("(.*)\n") do
				table.insert(log.text,line)
			end
			log:toLine(#self.text)
			coroutine.yield()
		end
		function log:toLine(n)
			local out = ""
			local maxn = math.floor(self:maxLineCount()-1.5)
			for i=n-maxn,n do
				out = out .. "\n"..(self.text[i] or "")
			end
			self:setText(out)
			scroll.disabledevent = true
			scroll:setSliderPos((n-maxn)/(#self.text-maxn))
			scroll.disabledevent = false
		end
		function log:clear()
			log.text = {}
			log:setText("")
		end



		function list:onSelected(idx,cap,cmd)
			local p = projects[cmd]
			local outp = p.path.."_bin"
			info:setText(
				"Chosen project: \vx90;"..p.name..
				"\nLocation: \vx90;"..p.path..
				"\nOutputdirectory:\vx90;"..(outp))
			local function compilenow ()
				local errorlist = {}

				log:clear()
				log:append("Start to compiling project \v950"..p.name)

				local function recscan (dir,clone)
					log:append("\v000Creating directory:")
					log:append("  \v000"..clone)
					local suc,error = FileSystem.mkdir(clone)
					if not suc and error~="File exists" then
						log:append("\v500Error: "..error)
						return
					else
						log:append("\v050Done"..(not suc and " (already exists)" or ""))
					end

					log:append("\v000Scanning: "..dir)
					local dirs = {}
					for name in FileSystem.dir(dir) do
						if name ~="." and name~=".." then
							if FileSystem.attributes(dir.."/"..name,"mode") == "directory" then
								if name~=".svn" and name~=".cvs" then
									dirs[#dirs+1] = name
								end
							else
								if name:match("%.[lL][uU][aA]$") then
									log:append("\v009compiling "..name.."...")
									local fn,err = loadfile(dir.."/"..name)
									if not fn then
										log:append("\v500  Error compiling: "..err)
										table.insert(errorlist,"Error compiling "..dir.."/"..name..": "..err)
									else
										log:append("\v000  Creating outputfile: "..name)
										local outfp,err = io.open(clone.."/"..name,"wb")
										if not outfp then
											log:append("\v500  Error opening for writing: "..err)
											table.insert(errorlist,"Error opening "..clone.."/"..name.." for writing: "..err)
										else
											outfp:write(string.dump(fn))
											outfp:close()
											log:append("\v050  Done")
										end
									end
								else
									log:append("\v000copy "..name)
									local infp,err = io.open(dir.."/"..name,"rb")
									if not infp then
										log:append("\v500  Error opening for reading: "..err)
										table.insert(errorlist,"Error opening "..dir.."/"..name.." for reading: "..err)
									else
										local outfp,err = io.open(clone.."/"..name,"wb")
										if not outfp then
											log:append("\v500  Error opening for writing: "..err)
											table.insert(errorlist,"Error opening "..clone.."/"..name.." for writing: "..err)
										else
											local size = infp:seek("end",0)
											infp:seek("set",0)
											local read,perc = 0,0
											while true do
												local i = infp:read(1024)
												if not i then break end
												outfp:write(i)
												read = read + #i
												if read<size then
													local p = math.floor(.5+read/size*100)
													if p~=perc then
														perc = p
														log:append("\v050  "..p.."%")
														coroutine.yield()
													end
												end
											end
											outfp:close()
										end
										infp:close()
									end
								end
							end
						end
					end
					table.sort(dirs)
					for i,v in ipairs(dirs) do
						recscan(dir.."/"..v,clone.."/"..v)
					end
				end

				recscan(p.path,outp)
				log:append("\v009Finished, errors: "..#errorlist)
				for i,v in ipairs(errorlist) do
					log:append(v)
				end
			end
			function compile:onClicked()
				TimerTask.new(compilenow)
			end
		end

		for i,v in pairs(projecthistory) do
			if (type(i)=="string") then
				local name = v.name or i:gsub("^.*[\\/]([^\\/]+)$","%1")
				if FileSystem.attributes(i) then
					local fn,err = loadfile(i.."/.projectinfo.lua")
					local info
					if fn then
						local suc,err = pcall(function () info = fn() end)
						if not suc then _print("error in projectinfo of "..i,err) end
					end
					table.insert(projects,{name = name,i,v.callcount,info,path = i})
				end
			end
		end
		table.sort(projects,function(a,b) return a.name<b.name end)

		for i,v in ipairs(projects) do
			list:addItem(v.name,i)
		end
	end

	function menu:showAddMenu()
		local con = Container:new(10,28,320,400)
		local lbl = con:add(Label:new(0,0,con:getWidth(),20))
		local list = con:add(ListBox:new(40,20,con:getWidth()-40,con:getHeight()-80))
		local ok = con:add(Button:new(con:getWidth()-80,con:getHeight()-20,80,20,"OK"))
		ok:setVisibility(false)
		function ok:onClicked()
			local e = addproject(lbl:getText())
			menu:showProjects()
		end

		local function upd(path)
			path = path:gsub("\\\\","\\"):gsub("\\[^\\]+\\%.%.","\\")
			lbl:setText(path)
			lbl:setTooltip(path)
			list:clearItems()
			list.onSelected = nil
			ok:setVisibility(false)
			for name in FileSystem.dir(path) do
				if name~="." and
					FileSystem.attributes(path.."/"..name,"mode")=="directory" then
					list:addItem(name)
				end
				if name == "main.lua" then
					ok:setVisibility(true)
				end
			end
			function list:onSelected(idx,tx)
				print(tx)
				upd(path.."\\"..tx)
			end
		end
		local drives = {system.drivelist()}
		local grp = ButtonGroup.new()
		for i,v in ipairs(drives) do
			local b = con:add(Button:new(0,20+(i-1)*22,40,22,v))
			function b:onClicked() upd(v) end
		end


		local cancel = con:add(Button:new(0,con:getHeight()-20,80,20,"Cancel"))
		function cancel:onClicked()
			menu:showProjects()
		end



		--[=[local path = con:add(TextField:new(0,20,con:getWidth(),22))
		local err = con:add(Label:new(0,44,con:getWidth(),20))

		local cancel = con:add(Button:new(0,con:getHeight()-20,80,20,"Cancel"))
		function cancel:onClicked()
			menu:showProjects()
		end

		local ok = con:add(Button:new(con:getWidth()-80,con:getHeight()-20,80,20,"Add Project"))

		function ok:onClicked()
			local e = addproject(path:getText())
			if e then
				err:setText("\v900"..e)
			else
				menu:showProjects()
			end
		end

		con:add(Label:new(0,0,con:getWidth(),20,"Specify path (sorry, yet another path selection component)"))

		]=]
		menu:setPage(con,"Add a new project")
	end

	function menu:showProjects()
		Component.newFocusOrder({})
		local dprojects = Container:new(10,28,460,h-35)

		local submenu = dprojects:add(Container:new(0,dprojects:getHeight()-30,330,30))
		submenu.skinnames = Label.skinnames
		submenu:setSkin "default"

		local boptions = submenu:add(Button:new(
			5,5,60,20, "Options"))
		boptions:setTooltip("Configure the Luxinia engine")
		function boptions:clicked()
			menu:showOptions()
		end
		local viewbtn = submenu:add(Button:new(65,5,80,20,"Start Viewer"))
		viewbtn:setTooltip("Start the viewer application")
		function viewbtn:onClicked()
			deinit()
			_print("Starting viewer")
			Viewer.initbase()
		end

		local addproj = submenu:add(Button:new(145,5,80,20,"Add project"))
		addproj:setTooltip("Add a new project to the projectlist")
		function addproj:onClicked()
			menu:showAddMenu()
		end

		local projcomp = submenu:add(Button:new(225,5,100,20,"Compile project"))
		projcomp:setTooltip("Create a project that contains only\nbinary lua files, ready for distribution")
		function projcomp:onClicked()
			menu:showProjCompMenu()
		end

		local projects = {}
		for i,v in pairs(projecthistory) do
			if (type(i)=="string") then
				local name = v.name or i:gsub("^.*[\\/]([^\\/]+)$","%1")
				if FileSystem.attributes(i) then
					local fn,err = loadfile(i.."/.projectinfo.lua")
					local info
					if fn then
						local suc,err = pcall(function () info = fn() end)
						if not suc then _print("error in projectinfo of "..i,err) end
					end
					table.insert(projects,{name,i,v.callcount,info})
				end
			end
		end


		local function showprojects(n,delay)
			TimerTask.new(function ()
				for i,c in ipairs(dprojects.components) do
					if c.leave then c:leave() end
				end

				for i=n,math.min(3+n,#projects) do
					local y = (i-n)*83
					local con = dprojects:add(Container:new(0,y*0.3,dprojects:getWidth(),0))
					con.skinnames = Label.skinnames
					con:setSkin "default"
					local tx = con:add(MultiLineLabel:new(78,5,con:getWidth()-100,70,"test"))
					local name = projects[i][1]
					local path = projects[i][2]
					local tex,istex = matsurface.vertexcolor()
					if projects[i][4] then
						local info = projects[i][4]
						tx:setText(info.title.."\n"..info.description)
						if info.thumbnail then
							tex = texmem.get(path.."/"..info.thumbnail)
							istex = true
						end
					else
						local short = path:sub(1,math.min(64,#path))
						tx:setText(name.."\n"..short)
					end
					local bg = {0.97,0.97,0.97,1}
					con:fadeTo(bg,black,opaque,0.1)
					function con:mouseEntered(me,on)
						con:fadeTo({1,1,1,1},{1,0,0,1},{1,1,1,1},0.1)
					end
					function con:mouseExited(me,on)
						con:fadeTo(bg,{0,0,0,1},{1,1,1,1},0.1)
					end
					function con:mouseClicked(me,on)
						deinit(path)
					end
					con:add(ImageComponent:new(7,7,64,64,tex))
					con:moveToRect(0,y,con:getWidth(),80,0.2)

					function con:leave()
						if con.leaving then return end
						con.leaving = true
						con:moveToRect(0,dprojects:getHeight()-40,con:getWidth(),0,0.2,nil,
							function()
								con:delete()
								if istex then texmem.release(tex)  end
							end)
					end

					--coroutine.yield(100)
					--local btn = con:add(Button:new(con:getWidth()-80,con:getHeight()-25,75,20,"Start"))
				end
			end,delay or 400)
		end
		local projectn = 1
		showprojects(1)

		if #projects>4 then
			local con = dprojects:add(Container:new(dprojects:getWidth()-120,dprojects:getHeight()-30,120,30))
			con.skinnames = Label.skinnames
			con:setSkin("default")
			local next = con:add(Button:new(con:getWidth()-45,5,40,20,">"))
			local prev = con:add(Button:new(5,5,40,20,"<"))
			local tx = con:add(Label:new(50,5,20,20))
			function tx:updateTX()
				local max = math.ceil(#projects/4)
				tx:setText(math.ceil(projectn/4).." / "..max)
			end
			tx:updateTX()
			prev:setVisibility(false)

			if projectn+4>#projects then
				con:setVisibility(false)
			end

			function next:clicked()
				projectn = projectn + 4
				showprojects(projectn,1)
				if projectn+4>#projects then
					next:setVisibility(false)
				end
				prev:setVisibility(true)
				tx:updateTX()
			end
			function prev:clicked()
				projectn = projectn - 4
				showprojects(projectn,1)
				if projectn==1 then
					prev:setVisibility(false)
				end
				next:setVisibility(true)
				tx:updateTX()
			end
		end

		menu:setPage(dprojects,"Select a project to start")
	end


	function menu:showOptions()
		local cfg = dofile "options.lua"

		if self.active == "options" then return end
		Component.newFocusOrder({})
		self.active = "options"
		local doptions = Container:new(10,28,400,250)


		local tw = 80
		local bw = 80-10
		local bcon = doptions:add(Container:new(doptions:getWidth()-20,doptions:getHeight()-30,0,30))
		bcon.skinnames = Label.skinnames

		local bclose = bcon:add(Button:new(5,5,0,20, "Cancel"))
		function bclose:clicked()
			loadprojecthistory()
			menu:showProjects()
		end
		local bapply = bcon:add(Button:new(5,5,0,20,"Apply"))
		local bok = bcon:add(Button:new(5,5,0,20,"OK"))
		local pages

		function bapply.clicked()
			cfg.apply()
		end

		function bok.clicked()
			cfg.apply()
			menu:showProjects()
		end


		local rs = 0.3
		TimerTask.new(function ()

			bcon:moveToRect(doptions:getWidth()-tw*3,bcon:getY(),10+bw*3+4,30,rs)
			bclose:moveToRect(5,5,bw,20,rs)
			bapply:moveToRect(5+2+bw,5,bw,20,rs)
			bok:moveToRect(5+2+2+bw*2,5,bw,20,rs)

			coroutine.yield(40)

			pages = doptions:add(Container:new(bcon:getX(),doptions:getHeight()-25,doptions:getWidth()-bcon:getX(),0))
			pages.skinnames = Label.skinnames
			pages:setSkin("default")
			pages:moveToRect(0,0,doptions:getWidth(),doptions:getHeight()-25,0.2)

			coroutine.yield(150)

			local dconf
			local function newconf (title)
				if dconf then
					local m = dconf
					dconf:moveToRect(doptions:getWidth()+20,m:getY(),
						m:getWidth(),m:getHeight(),0.1,20,function () m:delete() end)
				end
				dconf = pages:add(Container:new(
					doptions:getWidth(),5,doptions:getWidth()-110,doptions:getHeight()-40),1)

				dconf.skinnames=Label.skinnames
				dconf:setSkin("default")
				dconf:setColor({0.05,0.05,0.05,1})
				dconf:moveToRect(100,5,dconf:getWidth(),dconf:getHeight(),0.1)

				function dconf:addbool(x,y,w,h,title,cmd)
					assert(cfg[cmd],"No such command: "..cmd)
					local cbox = self:add(CheckBox:new(x,y,w,h,title))
					cbox:setPushState(cfg[cmd]())
					function cbox:onClicked(state)
						cfg.select(cmd,state)
					end
					return function (title,cmd)
						return self:addbool(x,y+17,w,h,title,cmd)
					end
				end


				local lbl = Label:new(5,0,dconf:getWidth()-10,35,title)
				lbl:setAlignment(Label.LABEL_ALIGNLEFT,Label.LABEL_ALIGNTOP)
				lbl:setFont("arial20-16x16")
				dconf:add(lbl)
				return dconf
			end

			local cnt = 0

			local ops = {}
			function ops.About ()
				local con = newconf("About Luxinia")
				con:add(MultiLineLabel:new(5,30,con:getWidth()-10,con:getHeight(),
					([[Copyright Eike Decker & Christoph Kubisch 2004-2007<br>
					<br>
					Redistributing of artwork included in the sample projects is not allowed
					(Any form).
					This includes textures, models, animations and sound.
					<br>
					<br><red>
					The Free Version License prohibits any form of commercial exploitation!
					<br><br>
					See license.txt for more information!
					]]):gsub("\n*",""):gsub("\t*",""):gsub("<br>","\n"):gsub("\r*",""):gsub("<red>","\v900")))
			end
			function ops.System ()
				local con = newconf("System")
			end
			function ops.Projects ()
				local con = newconf("Project manager")
				local list = con:add(ListBox:new(5,30,100,con:getHeight()-35))

				local tx = con:add(MultiLineLabel:new(110,30,con:getWidth()-115,120))
				local rem = con:add(Button:new(con:getWidth()-80,con:getHeight()-25,75,20,"Remove"))

				function list:onSelected(idx,name,cmd)
					tx:setText("Project: "..name.."\n"..cmd[2].."\nCallcount: "..cmd[3])
					function rem:clicked()
						list:removeItem(idx)
						projecthistory[cmd[2]]= nil
					end
				end


				local projects = {}
				for i,v in pairs(projecthistory) do
					if (type(i)=="string") then
						local name = v.name or i:gsub("^.*[\\/]([^\\/]+)$","%1")
						if FileSystem.attributes(i) then
							table.insert(projects,{name,i,v.callcount})
						end
					end
				end
				for i,v in ipairs(projects) do
					list:addItem(v[1],v)
				end

			end
			function ops.System ()
				local con = newconf("System")
			end
			function ops.Sound()
				local con = newconf "Sound"

				--local box = con:add(ComboBox:new(95+x,y-1,90,20))
				con:add(Label:new(5,30,con:getWidth()-10,40,
					("Currently used sounddevice: %s\n"..
					 "Mono / Stereo sources limits: %d / %d"):format(
						sound.getdevicename(),sound.monocount(),sound.stereocount()
					)))
				con:addbool(5,180,160,20,"No Sound (requires restart)","nosound")
				con:add(Label:new(5,80,60,20,"Device:",Label.LABEL_ALIGNRIGHT))
				local devlist = con:add(ComboBox:new(65,80,160,20))
				for i,name in ipairs{sound.devices()} do
					devlist:addItem(name)
					if name==sound.getdevicename() then
						devlist:select(i)
					end
				end
				function devlist:onSelected(idx,tx)
					cfg.select("sounddevice",tx)
				end

			end
			function ops.Debug()
				local con = newconf "Debug helpers"

				con:addbool(5,30,130,20,"Draw all objects*","drawall"
					)("Draw axis","drawall"
					)("Draw bonelimits","drawbonelimits"
					)("Draw bonenames", "drawbonenames"
					)("Draw lights", "drawlights"
					)("Draw camera axis", "drawcamaxis"
					)("Draw FPS rate","drawfps"
					)("Don't draw GUI*", "drawnogui"
					)
				con:addbool(140,30,130,20,"Draw stats", "drawstats"
					)("Draw normals", "drawnormals"
					)("Draw projectors", "drawprojectors"
					)("Draw shadowvolumes*", "drawshadowvolumes"
					)("Draw stencilbuffer*", "drawstencilbuffer"
					)("Draw wireframes*", "drawwire"
					)("Draw perf.graph*", "drawpgraph")
				con:add(Label:new(5,166,100,20,"* not saved on quit"))
			end
			function ops.Video ()
				local con = newconf("Video options")

				local function addbox(x,y,title,cmd,...)
					assert(cfg[cmd],"no such cmd:"..cmd)
					local items = {...}
					con:add(Label:new(x,y,90,20,title))
					local box = con:add(ComboBox:new(95+x,y-1,90,20))
					for i,v in ipairs(items) do
						box:addItem(v)
					end
					box:select(cfg[cmd]())
					function box:onSelected(idx)
						cfg.select(cmd,idx)
					end
					return function (...)
						return box:addItem(...)
					end
				end
				addbox(5,30,"Resolution:","winres",
					 "640x480","800x600","1024x768","1200x1024","1600x1200")
				addbox(5,50,"Color Depth:","bpp",
					"16","24","32")
				addbox(5,70,"Details:","detail",
					"Low","Medium","High")
				addbox(5,90,"Max. Framerate:","maxfps",
					"No limit","15","30","45","60","75","90","105","120")
				addbox(5,110,"FS AntiAlias:","multisamples",
					"0","2","4","6","8","16")

				con:addbool(195,31,100,20,"Fullscreen","fullscreen")
				con:addbool(195,91,100,20,"V-Synch","vsynch")

				con:addbool(2,135,160,20,"Anisotropic texture filtering","anisotropic")
				con:addbool(158,135,150,20,"Texture compression","texcompression")

				con:addbool(2,135+20,150,20,"Vertex Buffer Objects","usevbos")
				con:addbool(158,135+20,150,20,"No default GPU progs","nodefaultgpuprogs")

				con:addbool(2,135+40,300,20,"No OpenGL extensions (unstable, not recommended)","noglextensions")

			end

			local function addpb(name)
				local btn = pages:add(Button:new(5,cnt*25+5,90,20,name))
				cnt = cnt + 1
				btn:setColor(trans,trans,trans)
				btn:fadeTo(opaque,black,opaque,0.09)
				function btn.clicked ()
					if ops.active == name then return end
					ops.active = name
					ops[name]()
				end
				coroutine.yield(80)
			end

			addpb("Video")
			addpb("Sound")
			addpb("Projects")
			--addpb("System")
			addpb("Debug")
			addpb("About")
		end,300)

		bcon:setSkin("default")
		menu:setPage(doptions,"Options")
	end

	menu:showProjects()

end



