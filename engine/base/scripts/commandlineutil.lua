do
	LuxiniaParam.addTriggerDescription ("b",
		[[This parameter must be followed by a string that defines code to launch the debugger]])
	local arg = LuxiniaParam.getArg("b")
	if arg then
		local fn,err = loadstring(arg[1])
		if (err) then
			print("dbg string error:",err)
		else
			fn()
		end
	end
end


do
	LuxiniaParam.addTriggerDescription ("p",
[[
requires a path argument (i.e. -p c:\project\bla). Sets the
projectpath to the given argument and trys to execute a "main.lua"
file in the given path. The path may be relative or absolute.
"main.lua" is not loaded if -v for the viewer is also set.
]])

	LuxiniaParam.addTriggerDescription ("i",
[[
Like -p but for .lxi files
]])

	LuxiniaParam.addTriggerDescription ("s",
[[
Like -p but allows filepaths inside project- or subdirectory of the project to be passed.
The actual projectpath is searched by recursively stepping down the directory,
searching for main.lua. Only absolute paths are allowed.
]])


	local ppath = system.projectpath
	function system.projectpath (path)
		if (path) then
			projectpath = path or projectpath

			local function buildpath(...)
				local searches = {...}
				return function (...)
					local str = {}
					for i,p in ipairs({...}) do
						for j,s in ipairs(searches) do
							table.insert(str,p..s..";")
						end
					end
					return table.concat(str)
				end
			end
			package.cpath = buildpath("?.dll","?\?.dll","?.so","?/?.dll") (".\\base\\lib\\",".\\",path.."\\")

			return ppath(path)
		end
		return ppath()
	end
	local path = LuxiniaParam.getArg("p") or LuxiniaParam.getArg("i") or LuxiniaParam.getArg("s")
	if (LuxiniaParam.getArg("v")) then
		path = nil
	end

	function validateprojects ()
	end

	function loadprojecthistory()
		local cfgfile = system.queryfrontend("pathconfig").."/projecthistory.lua"
		local fn,err = loadfile(cfgfile)
		local lfs = require "lfs"
		if (fn) then
			projecthistory = fn() or {}
			for i,v in pairs(projecthistory) do
				if type(v)=="string" then
					local path = v.."/main.lua"
					if not lfs.touch(path) then
						projecthistory[i] = nil -- invalid projectpath
					end
				end
			end
		else print(err) projecthistory = {} end
	end
	loadprojecthistory()

	function saveprojecthistory()
		local slashconv = {}
		for i,v in pairs(projecthistory) do
			if (type(i)=="number") then slashconv[i] = v else
				slashconv[i:gsub("\\","/"):gsub("/+","/"):gsub("^(%w:)",string.lower):gsub("/$","")] = v
			end
		end
		projecthistory = slashconv
		local cfgfile = system.queryfrontend("pathconfig").."/projecthistory.lua"
		file,err = io.open(cfgfile,"w")
		if (file) then
			file:write("return {\n");
			for i,tab in UtilFunctions.pairsByKeys(projecthistory) do
				if (type(i)=="number") then
					file:write(string.format(" [%d] = {\n",i))
				else
					file:write(string.format(" [%q] = {\n",i))
				end

				for j,q in pairs(tab or {}) do
					file:write(string.format("    [%q] = %q,\n",j,q))
				end
				file:write("  },\n")
			end
			file:write("}")
			file:close()
		else
			print("Error: Couldn't write projecthistory.")
			print(err)
		end
	end

	function addproject(name)
		local fn,err = loadfile(""..name.."/main.lua")
		if (fn) then
			local path = name
			projecthistory[path] = projecthistory[path] or {}
			projecthistory[path].callcount = (projecthistory[path].callcount or 0)
			projecthistory[path].lastcall = projecthistory[path].lastcall or os.time()
			saveprojecthistory()
			return
		end
		return err
	end

	local function findlast(str,catch,plain)
		local start = 0
		local pos = str:find(catch,0,plain)
		local lpos = pos
		
		while (pos) do
			lpos = pos
			pos = str:find(catch,pos+1,plain)
		end

		return lpos
	end


	function loadproject(name,allowsearch)
		print("LOADPROJECT",name)
		name = name:gsub("\\","/"):gsub("/[^/]*%.lua$","")
		
		-- make sure ends without /
		local last = name:find("/",-1,true)
		name = last and name:sub(0,last-1) or name
		
		--print("projectpath=======================",name)
		
		local fn,err = loadfile(""..name.."/main.lua")
		while ((not fn) and allowsearch) do
			local last = findlast(name,"/",true)
			if (last) then
				name = name:sub(0,last-1)
				fn,err = loadfile(""..name.."/main.lua")
			else
				allowsearch = false
			end
		end
		
		if (fn) then
			-- in case we searched down to "C:"
			local any = name:find("/",0,true)
			if (not any) then name = name.."/" end
			
			system.projectpath(name)
			--local coro = coroutine.create(fn)
			--local suc,err = coroutine.resume(coro)
			--if not suc then
--				print(debug.traceback(coro,err))
	--			return err
		--	end
			TimerTask.new(fn)
			local path = name
			table.insert(projecthistory,
				{time = os.time(), path = path})
			if (table.getn(projecthistory)>30) then
				table.remove(projecthistory,1)
			end
			projecthistory[path] = projecthistory[path] or {}
			projecthistory[path].callcount = (projecthistory[path].callcount or 0) + 1
			projecthistory[path].lastcall = os.time()
			saveprojecthistory()

			return err
		else
			return err
		end
	end

	if (path) then
		local allowsearch = LuxiniaParam.getArg("s") ~= nil
		
		local function outpath(pth)
			local nstr = ""
			local len = #pth
			for n,t in ipairs(pth) do
				nstr = nstr..t..(n<len and " " or "")
			end
			
			return nstr
		end

		path = outpath(path)
		
		local err = loadproject(path,allowsearch)
		if (err) then
			console.active(true)
			print("The projectpath ",path," could not be loaded. Sorry. Error was:")
			print(err)
		end
	elseif (not LuxiniaParam.getArg("v")) then
		dofile("projectmanager/projectmanager.lua")
	end
end

do
	LuxiniaParam.addTriggerDescription ("v",
"launches the viewer requires 1 filename (.mtl, .prt, .f3d, .mm) optionally a second (.ma). You can combine it with setting the projectpath and resources will be loaded from there.")

	local args = LuxiniaParam.getArg("v")
	if (args) then
		_print("Starting viewer")
		dofile("base/scripts/luxmodules/luxinialuacore/viewer.lua")
		Viewer.initbase()
		Viewer.loadres(args[1],args[2])
	end

end

if window.pos then
	LuxiniaParam.addTriggerDescription("w",
		"x y - sets window position")
	local arg = LuxiniaParam.getArg("w")
	--print("-------------------------",arg)
	if arg and arg[1] and arg[2] then
		window.pos(tonumber(arg[1]),tonumber(arg[2]))
	end
end

do
	LuxiniaParam.addTriggerDescription ("h",
		"prints out all known module trigger descriptions")
	if (LuxiniaParam.getArg("h")) then
		for index,trigger,description in LuxiniaParam.iterator() do
			_print("  "..trigger..": ",description)
		end
	end
end

LuxiniaParam.addTriggerDescription ("d",[[Writes the documentation files.]])

if (LuxiniaParam.getArg("d")) then
	_print("writing documentation")
	--local htmldoc = AutoDoc.createDoc(AutoDoc.templatePages.apiSingleHtmlDoc)
	--local f = io.open("doc/api2.html","w")
	--f:write(htmldoc)
	--f:close()

	--local _,moduledoc =
	--	AutoDoc.createDoc(AutoDoc.templatePages.moduleSingleHtmlDoc)
	--local f = io.open("doc/mod2.html","w")
	--f:write(table.concat(moduledoc.main))
	--f:close()
	AutoDoc.writeDoc("doc/",AutoDoc.templatePages.wxideapi)
	AutoDoc.writeDoc("doc/frames/",AutoDoc.templatePages.apiHTMLFrameDoc)
	AutoDoc.writeDoc("doc/",AutoDoc.templatePages.apiDocIndex)
	
end
