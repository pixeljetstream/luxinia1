window.title "Developer Tutorials"
window.refsize(window.size())
window.resizemode(2)

local w,h = window.refsize()
local listwidth = 240

if not (ProjectpathFull) then
	ProjectpathFull = UtilFunctions.projectpathabs()
end

function LOGPRINT(...)
	print(...)
	
	logfile = logfile or io.open(ProjectpathFull.."/log.txt","w")
	assert(logfile,"LOGFILE")
	
	local args = {...}
	local last = #args

	for i,v in ipairs(args) do
		logfile:write(tostring(v)..(i==last and "" or "\t"))
	end
	logfile:write("\n")
end

function NOTREADY()
	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"This sample is not ready yet."))

	return (function() lbl:delete() end)
end

-----
-- prepare root container

-- local oroot = Container.getRootContainer()
-- prep root for demos
local newroot = oroot and Container:new(0,0,w,h) or Container.getRootContainer()

if (oroot) then
function oroot.doLayout(self)
	local w,h = window.refsize()
	newroot:setSize(w,h)
	newroot:doLayout()
end
end

function Container.getRootContainer()
	return newroot
end

----------------
--- GUI setup

------
-- main gui
local frame = newroot:add(GroupFrame:new(-5,-5,w+10,h+10))
frame.sub = frame:add(TitleFrame:new(10,10,listwidth,h-40,nil,"Tutoriallist"))
frame.list = frame.sub:add(ListBox:new(10,30,listwidth-20,h-100))
frame.start = frame.sub:add(Button:new(10,h-70,listwidth-20,20,"Start"))
frame.sourceframe = frame:add(TitleFrame:new(listwidth+10,10,w-listwidth-5,h-20))

function frame.doLayout(self)
	local ww,wh = window.refsize()
	self:setSize(ww+10,wh+10)
	self.sub:setBounds(10,10,listwidth,wh-20)
	self.list:setBounds(10,30,listwidth-20,wh-80)
	self.start:setBounds(10,wh-50,listwidth-20,20)
	self.sourceframe:setBounds(listwidth+10,10,ww-listwidth-5,wh-20)
	self.sourceframe:doLayout()
end

------
-- tut list

local list = {}
for name in FileSystem.dir(system.projectpath().."/tutscripts") do
	if name:match("%.lua$") then
		list[#list+1] = name
	end
end
table.sort(list)
for i,name in ipairs(list) do 
	local info = io.open(system.projectpath().."/tutscripts/"..name):read():sub(4)
	frame.list:addItem(info,name)
end

------
-- code display
do
	local w = w-listwidth-25
	frame.sourceclip = frame.sourceframe:add(Container:new(10,30,w,h-65))
	frame.info = frame.sourceclip:add(Label:new(0,0,0,0))
	frame.info:setAlignment(Label.LABEL_ALIGNLEFT,Label.LABEL_ALIGNTOP)
	frame.info:setFont("lucidasanstypewriter9-16x16")
	frame.info:setTabWidth(12)
	
	frame.numberbg = frame.sourceclip:add(ImageComponent:new(0,0,25,h,matsurface.vertexcolor()))
	frame.numberbg:color(.3,.3,.3,1)
	frame.hscroll = frame.sourceframe:add(Slider:new(w+10,25,10,h-60,true))
end
function frame.hscroll:onValueChanged(v)
	local cw,ch = frame.sourceclip:getSize()
	local w,h = frame.info:getSize()
	local scrollh = math.max(0,h-ch)
	local x,y = frame.info:getLocation()
	frame.info:setLocation(x,-scrollh*v)
end

function frame.list:onSelected(idx,info,name) 
	local content = io.open(system.projectpath().."/tutscripts/"..name):read("*a")
	local strs = {}
	content = content:gsub("(%-%-[^\n]+)",function (line)
		strs[#strs+1] = line
		return "\v050\vz"..#strs..";\vc"
	end)
	content = content:gsub("(([\"'])(.-[^\\])(%2))",function (str)
		strs[#strs+1] = str
		return "\v505\vz"..#strs..";\vc"
	end)
	content = content:gsub("%S+",function(word)
		local key1 = {
			["function"]=true,["if"]=true,["then"]=true,
			["do"] = true,["while"]=true,["until"]=true,
			["repeat"] = true, ["local"]=true,["end"]=true,
		}
		if key1[word] then 
			word = "\v009"..word.."\vc"
		end
		return word
	end)
	content = content:gsub("\vz(%d+);",function (num)
		num = tonumber(num)
		return strs[num]
	end)
	frame.sourceframe:setTitle("Sourcefile: "..name)
	local str = content
	local idx = 0
	str = str:gsub("\r*\n\r*"," \n")
	str = str:gsub("([^\n]+)",function(line)
		idx = idx + 1
		return "\vx3;\v888"..idx.."\vc\vx24; "..line
	end)
	frame.info:setText(str)
	local w,h = frame.info:getMinSize()
	frame.info:setSize(w,h)
	frame.hscroll:setSliderPos(0)
end


function frame.sourceframe:doLayout()
	local ww,wh = window.refsize()
	local w = ww-listwidth-25
	frame.sourceclip:setBounds(10,30,w,wh-65)
	frame.hscroll:setBounds(w+10,25,10,wh-60)
	frame.numberbg:setBounds(0,0,25,wh-60)
	
	local w,h = frame.info:getMinSize()
	frame.info:setSize(w,h)
	frame.hscroll:setSliderPos(0)
end
-------
--

function newroot_setlayout()
	frame:doLayout()
	function newroot:doLayout()
		frame:doLayout()
	end
end

-------
-- actions start 
frame.list:select(1) 

function frame.list:onAction()
	frame.start:onClicked()
end

local function defaultstate()
	local cam = l3dcamera.default()
	local mat = matrix4x4.new()
	cam:unlink()
	cam:worldmatrix(mat)
	
	cam:backplane(1024)
	cam:frontplane(1)
	cam:fov(45)
	cam:aspect(-1)
	
	cam:nearpercentage(0.05)
	cam:useclipping(false)
	cam:useinfinitebackplane(false)
	cam:usemanualprojection(false)
	cam:usereflection(false)
	
	for i=0,l3dlist.l3dsetcount()-1 do
		l3dset.get(i):getdefaultview():rcmdempty()
		l3dset.get(i):disabled(false)
	end
	l3dset.get(0):layer(0)

	window.refsize(window.size())
	UtilFunctions.freelook(false)
	render.drawtexture()
	
	model.loaderprescale(1,1,1)
	animation.loaderprescale(1,1,1)
	
	local rc = l2dlist.getrcmdclear()
	rc:color(false)
end

function frame.start:onClicked()
	local idx,tit,name = frame.list:getSelected()
	--frame = nil
	local fn,err = loadfile("tutscripts/"..name)
	if not fn then
		print("error:",err)
		return
	end
	defaultstate()
	
	frame:remove()
	
	input.escquit(false)
	env = {}
	setmetatable(env,{__index = _G})
	setfenv(fn,env)
	Keyboard.setKeyBinding("ESC",
		function()
			if (_cleanup) then 
				_cleanup(env) 
			end
			_cleanup = nil
			
			Keyboard.removeKeyBinding("ESC")
			Timer.remove "tutorial"
			TimerTask.deleteAll()

			env = {}
			TimerTask.new(
				function()
					Keyboard.removeAllKeyBindings ()
					
					input.escquit(true)
					defaultstate()
					collectgarbage "collect"
					collectgarbage "collect"
					
					newroot:removeChilds()
					
					if (oroot) then
						oroot:removeChilds()
						oroot:add(newroot)
					end
					
					newroot_setlayout()
					newroot:add(frame)
					
					MouseCursor.showMouse(true)
					
					
				end,100
			)
		end)
	_cleanup = fn()
end

MouseCursor.showMouse(true)

if (oroot) then
	oroot:add(newroot)
end

newroot_setlayout()

if LuxiniaParam.getArg "t" then
	local file = LuxiniaParam.getArg "t" [1] or ""
	for i,v in ipairs(list) do
		if v == file then
			frame.list:select(i)
			frame.start:onClicked()
		end
	end
else
	-- info popup
	GuiHelpers.popupInfo(250,180,"Info",
[[The demo framework is rather simple,
therefore the order in which demos are run
may corrupt other demos' state. In that
case close and relaunch the application
to start with the demo that had errors.
]])
end
