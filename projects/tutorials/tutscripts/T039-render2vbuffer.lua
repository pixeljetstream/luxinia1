-- Render To VertexBuffer

-- <Resources>
-- textures/arrow.tga
-- fxs/particlesys/*

-- In this tutorial we create particle mesh vertices
-- with the help of render2vertexbuffer functionality.
-- At the point of writing this only works with Nvidia
-- hardware.
-- Another workaround would be using "rcmdreadpixels"
-- But the setup would be less conveniant.

-- obligatory hardware check
if (not (
		technique.cg_sm4():supported() and
		capability.r2vb()
		)) then

	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality."))

	return (function() lbl:delete() end)
end


-- For this effect we will have two meshes:
-- * the particles
--		This one stores particle data, such as position
-- 		speed and color
--
-- * the particle billboard mesh
--		We generate the billboard vertices from the
--		particle data. 
--
-- To make further use of the generator, we will draw
-- the particles with different rotation multiple times.
-- Just rotating the mesh, would break billboards facing to camera,
-- but as they are generated on the fly accordingly we can,
-- create instances.
--
-- Similar could be done with geometry shaders, however
-- geometry shader's perfomance isn't as good for older hardware.
-- There is one disadvantage however, we cannot discard vertices,
-- we will always output something.


---------
-- The particle System

-- our lua driven particles, gives us more flexibility
-- at the cost of speed. One could port the
-- particle update code to a vertexshader as well, or
-- use lualanes for threading ( see tutorial T027).

function NewObject(class)
	local self = {}
	setmetatable(self ,{__index = class})
	return self 
end



PSys = {}
function PSys:new(maxparticles, inputtype, outputtype, quadshdname)
	local self = NewObject(self)
	-- regular luxinia particles are capped at 8192 drawn per system/cloud.
	-- we can go beyond the limit with custom mesh drawing.
	self.maxparticles = maxparticles or 16384
	self.inputtype = inputtype or vertextype.vertex32tex2()
	self.outputtype = outputtype or vertextype.vertex32tex2()
	self.quadshdname = quadshdname or "fxs/particlesys/particle2quads.shd"
	
	-- call the init functions
	self:initmesh()
	self:initdraw()
	

	self.updtime = 1
	self.maxage = 6000
	self.speed = 40/1000
	self.sprate = 50/1000
	self.gravity = {0,0,-0.02/1000}
	self.cnt = 0

	return self
end

-- we split the init functions away for better overview

function PSys:initmesh()
	----------
	-- Let's setup the buffers
	local maxparticles = self.maxparticles
	local vtype = self.inputtype
	local prtvertsize = vtype:memsize()*maxparticles
	-- we change the data frequently, hence 2, and draw
	-- because we specify data from lua side
	local prtbuffer = vidbuffer.new(vidtype.vertex(),vidhint.draw(2),
									prtvertsize)


	-- a bit different, this time its a copy operation (written and read by GL)
	local vtype = self.outputtype
	local prtvertsize = vtype:memsize()*maxparticles*4
	local quadbuffer = vidbuffer.new(vidtype.vertex(),vidhint.copy(2),
									prtvertsize)

	
	self.prtbuffer = prtbuffer
	self.quadbuffer = quadbuffer

	----------
	-- Now the meshes

	-- we don't use indices, but draw the stuff straight
	local prtmesh  = rendermesh.new(vtype,maxparticles,0,prtbuffer)
	local quadmesh = rendermesh.new(vtype,maxparticles*4,0,quadbuffer)

	prtmesh:indexPrimitivetype(primitivetype.points())
	quadmesh:indexPrimitivetype(primitivetype.quads())
	
	self.prtmesh = prtmesh
	self.quadmesh = quadmesh
	

end

function PSys:initdraw()
	----------
	-- material and shader

	self.mtl = material.load("MATERIAL_AUTO:"..self.quadshdname)
	
	----------
	-- And finally the instances

	-- the rendering setup is as follows
	-- for every instance we need to render into the quadbuffer
	-- and then draw the quadbuffer.
	-- Drawing will be the same for all,
	-- but the fill prior to draw is different,
	-- because the matrices change

	local rcdraw = rcmddrawmesh.new()
	rcdraw:matsurface(texture.load("arrow.tga"))
	rcdraw:orthographic(false)
	-- both orthographic and matsurface alter the renderflag
	-- so always make custom changs afterwards
	
	rcdraw:rfBlend(true)
	rcdraw:rfNodepthtest(true)
	rcdraw:rfNocull(true)
	--rcdraw:rfAlphatest(true)
	--rcdraw:rsAlphacompare(comparemode.greater())
	--rcdraw:rsAlphavalue(0.3)
	rcdraw:rsBlendmode(blendmode.amodadd())
	
	rcdraw:rendermesh(self.quadmesh)
	rcdraw:rfNovertexcolor(false)

	self.rcdraw = rcdraw
	
end

function PSys:getShader()
	return self.mtl:getshader()
end

-- this function creates rcmds to draw an instance
-- of the particles at a given world matrix
function PSys:createinstance(wmat,dodebug)
	local rcfilldraw = rcmddrawmesh.new()
	rcfilldraw:orthographic(false)
	rcfilldraw:rendermesh(self.prtmesh)
	rcfilldraw:matsurface(dodebug and matsurface.vertexcolor() or self.mtl)
	rcfilldraw:matrix(wmat)
	
	if (dodebug) then
		print "DODEBUG"
		return {rcfilldraw}
	end
	
	local rcfill = rcmdr2vb.new()
	
	-- our vertices are always multiples of 16
	-- and each texcoord is 16 byte wide (4 x float 32-bit)
	local needed = (self.outputtype:memsize()/16)*4
	for i=0,needed-1 do 
		rcfill:attrib(i,"TEXCOORD"..i,4)
	end
	rcfill:numattrib(needed)
	rcfill:buffer(self.quadbuffer)
	rcfill:noraster(true)
	rcfill:drawmesh(rcfilldraw)
	
	return {rcfill,self.rcdraw}
end


-- our partcile update function in lua,
-- for speed one could do this either in C/C++,
-- or as vertexprogram (however handling the dead particles,
-- is harder to achieve then)
function PSys:update(curtime)
	local lasttime = self.lasttime or curtime
	local lastspawn = self.lastspawn or curtime
	self.lasttime = lasttime
	self.lastspawn = lastspawn
	
	local prts = self.prts or {}
	local cnt = self.cnt or 0
	
	local updtime = self.updtime
	local maxage = self.maxage
	local speed  = self.speed 
	local sprate = self.sprate
	local g = self.gravity
	
	local d = curtime-lasttime
	
	if (d < updtime) then
		return
	end
	
		
	-- spawn new
	local function addprt(i)
		local new = {
			n = prts.n,
			p = {0,0,0},
			a = 0,
			v ={	(math.random()-0.5)*speed,
					(math.random()-0.5)*speed,
					(math.random()+0.5)*speed},
		}
		prts.n = new 
	end
	
	local newcnt = math.floor((curtime-lastspawn)*sprate)
	newcnt = math.min(newcnt+cnt,self.maxparticles)-cnt
	

	for i=1,newcnt do
		addprt()
		self.lastspawn = curtime
	end
	
	-- we want to overwrite the whole thing
	local pfirst,plast = self.prtbuffer:map(vidmapping.discard())
	local va = vertexarray.newfromptrs(self.inputtype,pfirst,plast)
	
	-- update
	local prt = prts.n
	local prev = prts
	local i = 0
	while (prt) do
		
		if(prt.a > maxage) then 
			-- remove
			prev.n = prt.n
		else
			-- update
			local p = prt.p
			local v = prt.v
			local nv = {
				v[1] + g[1]*d,
				v[2] + g[2]*d,
				v[3] + g[3]*d,
			}
			
			p[1] = p[1] + ((v[1] + nv[1])*0.5*d)
			p[2] = p[2] + ((v[2] + nv[2])*0.5*d)
			p[3] = p[3] + ((v[3] + nv[3])*0.5*d)
			
			prt.p = p
			prt.v = nv
			prt.a = prt.a + d
			
			va:vertexTexcoord(i,nv[1],nv[2],0)
			va:vertexTexcoord(i,nv[3],prt.a/maxage,1)
			va:vertexPos(i,p[1],p[2],p[3])
			va:vertexColor(i,1,1,1,1)
						
			i = i + 1
			prev = prt
		end
		
		prt = prt.n
	end
	
	
	self.prtbuffer:unmap()
	self.prtmesh:vertexCount(i)
	self.quadmesh:vertexCount(i*4)

	self.cnt = i
	
	self.prts = prts
	
	self.lasttime = curtime
end




-------
-- View & Scene setup

-- use default values
mypsys = PSys:new()
local shd = mypsys:getShader()
local pid = shd:getparamid("params")

function shdparam(x,y,z,w)
	local a,b,c,d = shd:param(pid)
	shd:param(pid,x or a,y or b ,z or c,w or d)
	return shd:param(pid)
end


view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.15,0.2,0.25,0) 

-- create a couple instances
local bounds = 128
local instances = 4
for i=1,instances do
	local wmat = matrix4x4.new()
	wmat:pos(math.random()*bounds,math.random()*bounds,0)
	wmat:rotdeg(0,0,math.random()*360)
	
	local rcs = mypsys:createinstance(wmat,dodebug)
	-- add directly to view, be aware that
	-- on view:rcmdempty, the instances
	-- will be lost
	
	for i,r in ipairs(rcs) do
		view:rcmdadd(r)
	end
end

camact = actornode.new("cam",-bounds*0.5,-bounds*1,bounds*1.5)
camact.camera = l3dcamera.default()
camact.camera:linkinterface(camact)
camact:lookat(bounds*0.25,bounds*0.1,bounds*0.5,0,0,1)

l3dnode.updateall()

local root = Container.getRootContainer()

local ctrl = CameraEgoMKCtrl:new(camact.camera,camact,
				{shiftmulti = 5,movemulti = 10.0, fix = true, mousesense=4})

root:addMouseListener(ctrl:createML(root))


---------
-- GUI
-- add ui and cam ctrl
local gf = root:add(GroupFrame:new(0,0,32,32))
local y = 5
local gfw = 300
GH.sldLabelwidth = 64
GH.sldLabelval = 64

y = GH.addslider(gf,5,y,gfw-10,16,"shdsize",
	{
		callback = function(val)
			shdparam(val)
			return val
		end,
		initvalue = shdparam(val)/128,
		scale = 128,
		formatstr = "%.1f",
	})
	
y = GH.addslider(gf,5,y,gfw-10,16,"shdstretch",
	{
		callback = function(val)
			shdparam(nil,nil,val)
			return val
		end,
		initvalue =  select(3,shdparam(val))/128,
		scale = 128,
		formatstr = "%.1f",
	})
	
y = GH.addslider(gf,5,y,gfw-10,16,"shdvelmax",
	{
		callback = function(val)
			shdparam(nil,1/val)
			return val
		end,
		initvalue = (1/select(2,shdparam(val)))/1,
		scale = 1,
		formatstr = "%.1f",
	})
	
--[[
	self.maxage = 6000
	self.speed = 40/1000
	self.sprate = 50/1000
	self.gravity = {0,0,-0.02/1000}
]]

	
y = GH.addslider(gf,5,y,gfw-10,16,"maxage",
	{
		callback = function(val)
			mypsys.maxage = val
			return val
		end,
		initvalue = mypsys.maxage/10000,
		scale = 10000,
		formatstr = "%d",
	})

y = GH.addslider(gf,5,y,gfw-10,16,"speed",
	{
		callback = function(val)
			mypsys.speed = val
			return val*1000
		end,
		initvalue = mypsys.speed/0.5,
		scale = 0.5,
		formatstr = "%.1f",
	})
	
y = GH.addslider(gf,5,y,gfw-10,16,"gravity",
	{
		callback = function(val)
			mypsys.gravity[3] = -val/1000
			return val*1000
		end,
		initvalue = -mypsys.gravity[3]*1000,
		scale = 1,
		formatstr = "%.1f",
	})
	
y = GH.addslider(gf,5,y,gfw-10,16,"rate",
	{
		callback = function(val)
			mypsys.sprate = val
			return val*1000
		end,
		initvalue = mypsys.sprate/2,
		scale = 2,
		formatstr = "%.1f",
	})
	
y = GH.addslider(gf,5,y,gfw-10,16,"timescale",
	{
		callback = function(val)
			timescale = val
			return val
		end,
		initvalue = 1,
		formatstr = "%.2f",
	})
	
local lbl= gf:add(Label:new(5,y,128,16,""))
local lbl2= gf:add(Label:new(140,y,128,16,""))
y = y + 20
gf:add(Label:new(5,y,gfw-10,16,"Use WASD + C/SPACE for movement, mouse for look"))
y = y + 20

gf:setBounds(0,0,300,y+5)

timescale = 1.0
local t = 0
Timer.set("tutorial",
	function()
		local f,fdiff = system.time()
		t = t + fdiff*timescale
		
		mypsys:update(t)
		lbl:setText("Simulated: "..(mypsys.cnt))
		lbl2:setText("Drawn: "..(mypsys.cnt*instances))
		
		ctrl:runThink()
		
	end)

