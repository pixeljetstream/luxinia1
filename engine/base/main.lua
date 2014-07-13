---------------------------
-- Init
if (luxinia) then 
	dofile("base/scripts/projectmanager/projectmanager.lua")
	return
end

luxinia = luxinia or {time = system.time}
luxarg = {...}

-- no buffering for output channels
io.stdout:setvbuf "no"
io.stderr:setvbuf "no" 

---------------------------
-- Plugin 

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
package.cpath = [[?.dll;.\base\lib\?.dll;.\base\lib\?\?.dll;./?.so;base/lib/?.so;]]
package.path = [[?.lua;.\base\lib\?.lua;.\base\lib\?\?.lua;]]

package.cpath = buildpath("?.dll","?\?.dll","?.so","?/?.dll") ([[.\base\lib\]],[[.\]],system.projectpath())


---------------------------
-- wx startup

if system.queryfrontend("frontendtype") == "luxinia_lua" then
	dofile "base/wxstartup.lua"
end

---------------------------
-- file system start lxi or lxz

dofile "base/scripts/luxmodules/luxinialuacore/vfs.lua"


for i,v in ipairs(luxarg) do
	if v:lower():match("%.lxi$") or v:lower():match("%.lxz$") then
		luxarg[i] = v:match("^(.*)[\\/][^\\/]+$")
		table.insert(luxarg,i,"-i")
		if v:lower():match("%.lxi$") then
			dofile(v)
		end
	end
end

-----------------------------
-- Debug

if false then -- heavy debug

	jit.debug(2)
	--jit.off()
	local fp = io.open("dbgout.txt","w")
	local function dbg(what,line)
		local function concat (...)
			local out = {}
			for i=1,select('#',...) do
				table.insert(out,tostring(select(i,...)))
			end
			return table.concat(out)
		end
		--print(what,line)
		--if (what=="call") then
			--for i=-1,-100,-1 do
				local info = debug.getinfo(2)
				--if not info then break end
				--if info.source~="=(tail call)" then

				--fp:write(concat(what,info.source,info.name,line))
				print(concat(what,info.source,info.name,line))
			--end
			--end
		--end

	end
	debug.sethook(dbg,"cr")

	local cocr = coroutine.create
	function coroutine.create (...)
		local function co (co,...)
			debug.sethook(dbg,"cr")
			return co,...
		end
		return co(cocr(...))
	end
end

--[[
if ___LUXINIA___ then
	dofile("scripts/projectmanager/projectmanager.lua")
	return end -- don't load twice
___LUXINIA___ = true
]]


--------------------------
-- LuaJIT

--require("jit.opt").start()
jit.off()

do -- fixes the coroutine exit bug
	-- optimized version by Mike Pall
	local main = coroutine.running()
  local exit
  local orig_resume, orig_exit = coroutine.resume, system.exit
  local function check_resume(...)
    if exit then system.exit() end
    return ...
  end
  function coroutine.resume(...)
    return check_resume(orig_resume(...))
  end
  function system.exit()
    exit = true
    if coroutine.running() then coroutine.yield() else orig_exit() end
  end
end

for i,v in ipairs(luxarg) do
	if v:match "%-%-PREEXECSTR" and luxarg[i+1] then
		local content = luxarg[i+1]
		xpcall(function ()
			loadstring(content)()
			end,function(err)
				print(debug.traceback(err))
			end)
	end
end


-----------------------------------------
-- Config

_G.luxargs = arg

local confignosave = false

configdata = {
	system_maxfps = {system.maxfps, true},
	rendersystem_detail = {rendersystem.detail, true},
	rendersystem_texcompression = {rendersystem.texcompression, true},
	rendersystem_texanisotropic = {rendersystem.texanisotropic, true},
	rendersystem_texanisolevel = {rendersystem.texanisolevel, true},
	rendersystem_swapinterval = {rendersystem.swapinterval, true},
--	rendersystem_hwbonesparams = {rendersystem.hwbonesparams, true},
	rendersystem_nodefaultgpuprogs = {rendersystem.nodefaultgpuprogs, true}, -- require save after init
	rendersystem_noglextensions = {rendersystem.noglextensions, true}, -- require save after init
	rendersystem_force2tex = {rendersystem.force2tex, true}, -- require save after init
	rendersystem_forcefinish = {rendersystem.forcefinish, true},
	rendersystem_batchmaxindices = {rendersystem.batchmaxindices, true},
	rendersystem_batchmeshmaxtris = {rendersystem.batchmeshmaxtris, true},
	rendersystem_cgallowglslsm3up = {rendersystem.cgallowglslsm3up, true},
	--render_drawpgraph = {function () return 0 end, true},
	render_drawvisto = {render.visto, true},
	render_drawvisfrom = {render.visfrom, true},
	render_drawvisspace = {render.drawvisspace, true},
	render_drawfps = {render.drawfps, true},
	render_drawprojectors = {render.drawprojectors, true},
	render_drawlights = {render.drawlights, true},
	render_drawbones = {render.drawbones, true},
	render_drawaxis = {render.drawaxis, true},
	render_statscolor = {
		function (val)
			if (val) then
				if (type(val)=="table") then
					render.statscolor(unpack(val))
				else
					render.statscolor(1,1,1,0.8)
				end
			else
				return ("{%f,%f,%f,%f}"):format(render.statscolor()),true
			end end, true},
	window_multisamples = {function (val) if (val) then window.multisamples(val) else return window.multisamples() end end,true},
	render_drawstats = {render.drawstats, true},
	render_normallength = {render.normallength, true},
	window_width = {function (val) if (val) then window.width(val) else return window.width() end end,true},
	window_height = {function (val) if (val) then window.height(val) else return window.height() end end,true},
	window_bpp = {window.bpp,true},
	window_pos =  {
	function (val)
		if (val) then
			if (type(val)=="table") then
				window.pos(unpack(val))
			else
				window.pos(100,100)
			end
		else
			return ("{%f,%f}"):format(window.pos()),true
		end end, true},
	--render_bonelength = {render.bonelength, true},
	--render_axislength = {render.axislength, true},
	rendersystem_usevbos = {rendersystem.usevbos, true},
	system_nosound = {system.nosound, true},
	sound_device = {
		function (val)
			if val then
				sound.setdevice(val)
			else
				return sound.getdevicename()
			end
		end,true},
}

local initvalues = {}

local function loadconfig ()
	local cfgfile = system.queryfrontend("pathconfig").."/config.lua"
	print("Loading Config:",cfgfile)
	local fn,err = loadfile(cfgfile)
	if (err) then
		print("Couldn't load config.lua file from base directory. Reason:")
		print(err)
		confignosave = not err:match("No such file")
		if confignosave then print("Saving of config.lua file disabled") end
		return
	end
	setfenv(fn,{})
	xpcall(
		function ()
			local data = fn()
			for i,v in pairs(data or {}) do
				--print(i)
				if configdata[i] then
					--print("loading ",i)
					initvalues[i] = v
					configdata[i][1](v)
				end
			end
		end,
		function (err)
			print("Error loading config.lua file:")
			print(err)
			print(debug.traceback())
			confignosave = true
			if confignosave then print("Saving of config.lua file disabled") end
		end
	)

end

function _G.confignosave(value)
	configdata[value][2] = false
end

local function saveconfig ()
	local out = {}
	local function write (str,...) table.insert(out,str:format(...)) end
	write("return {")
	for i,v in pairs(configdata) do
		local fn,save = unpack(v)
		if (save) then
			--print("get value ",i)
			local val,noquotes = fn()
			--print(val)
			local fmt = (type(val)=="string" and not noquotes) and "	%s = %q," or "	%s = %s,"
			write(fmt,i,tostring(val))
		else
			local fmt = type(initvalues[i])=="string" and "	%s = %q," or "	%s = %s,"
			write(fmt,i,tostring(initvalues[i]))
		end
	end
	write("}")
	local cfgfile = system.queryfrontend("pathconfig").."/config.lua"
	print("Saving Config:",cfgfile)
	fp = io.open(cfgfile,"w")
	fp:write(table.concat(out,"\n"))
	fp:close()
end

function luxinia.atexit ()
	if (confignosave) then return end
	saveconfig()

	-- HACK dirty, FIXME do proper mechanism
	if (LuxiniaExitCallbacks) then
		for i,v in pairs(LuxiniaExitCallbacks) do
			v()
		end
	end
end

loadconfig()

for i,v in ipairs(luxarg) do
	if v == "-w" and tonumber(luxarg[i+1]) and tonumber(luxarg[i+2]) then
		window.pos(tonumber(luxarg[i+1]),tonumber(luxarg[i+2]))
		if (tonumber(luxarg[i+3]) and tonumber(luxarg[i+4])) then
			window.width(tonumber(luxarg[i+3]))
			window.height(tonumber(luxarg[i+4]))
		end
	end
end


------------------------------------------
-- Intro Think and load rest of modules

local initsuccess = true
function luxinia.think()
	if not initsuccess then return end
	initsuccess = false

	dofile("base/scripts/luxmodules/modulekernel.lua")
	if not UtilFunctions then return end
	initsuccess = true
	
	
	
	local intro = loadstring("return "..system.queryfrontend("logotime"))() + system.time()
	local introskip = false
	local nosmall = false
	local pressed = nil

	for i,v in ipairs(luxarg) do
		if v == ("--nologo") then
			introskip = true
		end
		if v ==("--nosmalllogo") then
			nosmall = true
		end
	end

	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(unpack(loadstring("return "..system.queryfrontend("logoclearcolor"))() ))

	local function makelogo()
		local prev = rendersystem.texcompression()
		rendersystem.texcompression(false)
		local logoimg = texture.load(system.queryfrontend("logobig"))
		logoimg:clamp(true)
		local logo = l2dimage.new("logo",logoimg)
		logo:rfBlend(true)
		logo:rsBlendmode(blendmode.decal())
		logo:fullscreen()
		logo:parent(l2dlist.getroot())
		logo.texdata = {logoimg:dimension()}

		local arialset,arialtex = UtilFunctions.loadFontFile ("arial11-16x16")
		local versionstr = l2dtext.new("version",("%s (%s)"):format(system.version()),arialset)
		versionstr:color(0,0,0,1)
		local w,h = versionstr:dimensions()
		versionstr:font(arialtex)

		versionstr:pos(math.floor(320-w/2),456,0)

		rendersystem.texcompression(prev)
		
		return logo,versionstr
	end
	
	local function makesmall()
		logoimg = texture.load(system.queryfrontend("logosmall"))
		logoimg:clamp(true)
		local logosm = l2dimage.new("logosm",logoimg)
		logosm:sortid(100000000)
		logosm:color(1,1,1,1)
		logosm:rfNodraw(true)
		logosm:rfNodepthtest(true)
		logosm:rsBlendmode(blendmode.decal())
		logosm:rfBlend(true)
		logosm:parent(l2dlist.getroot())
		
		return logosm,3
	end
	
	local logo,versionstr = unpack((not introksip) and {makelogo()} or {})
	local logosm,logosmpos =  unpack((not nosmall) and {makesmall()} or {})

	if (logo or logosm) then
		local logosmpos = 3
		local oldrefsize = window.refsize
		local function logoupdate ()
			local s,w = 32,4
			local sw,sh = window.size()
			local rw,rh = window.refsize()
			local pxsx,pxsy= rw/sw,rh/sh
			local posi = ({
					{rw-(w+s)*pxsx,w*pxsy},
					{rw-(w+s)*pxsx,rh-pxsy*(w+s)},
					{w*pxsx,rh-pxsy*(w+s)},
					{w*pxsx,w*pxsy},
				})[logosmpos]
			if (logosm) then
				logosm:pos(posi[1],posi[2],-127)
				logosm:scale(s*pxsx,s*pxsy,1)
			end

			if logo then
				local vaspect = 640/480
				local asp = sw/sh
				local aw,ah = 1,1

				if (vaspect > asp) then
					-- width is important
					aw = sw
					ah = aw/vaspect
				else
					-- height is important
					ah = sh
					aw = ah*vaspect
				end
				
				aw,ah = math.min(aw,800),math.min(ah,600)
				
				local x,y,w,h = ((sw-aw)/2),((sh-ah)/2),aw,ah
				-- convert to refsize system,
				x,w = x*pxsx,w*pxsx
				y,h = y*pxsy,h*pxsy

				logo:pos(x,y,-127)
				logo:scale(w,h,1)
			end
		end

		UtilFunctions.addWindowResizeListener(logoupdate)

		function window.refsize(w,h)
			if (not w) then return oldrefsize() end
			oldrefsize(w,h)
			logoupdate()
		end

		function smallLogoPos (n)
			logosmpos = math.max(1,math.min(n,4))
			logoupdate()
		end

		logoupdate()

		local winupd = window.update
		function window.update(...)
			local ret = {winupd(...)} -- db: level 1

			logoupdate()

			return ...
		end
	end

	local oldthink = luxinia.think

	function luxinia.think ()

		if (input.popkey() or input.ismousedown(1) or
			input.ismousedown(0)) then pressed = true end

		if (not introskip) then
			local t = system.time()
			if (t<intro) then
				if (not pressed or introskip) then return end
				introskip = true
				intro = t+400
				return
			end
		end

		if logo then logo:delete() end
		l2dlist.getrcmdclear():color(false)
		logo = nil
		if (logosm) then
			logosm:rfNodraw(false)
		end


		versionstr:delete()
		intro = nil

		--- initialisation here
		xpcall (
            function()
				luxinia.think = oldthink
				UtilFunctions.smallLogoPos = smallLogoPos
				Compat.setcompatibility(0.98)

				dofile("base/scripts/commandlineutil.lua")
            end,
            function (err)
                print(err)
                print(debug.traceback())
                luxinia.think = function () end
            end
        )
	end

end

