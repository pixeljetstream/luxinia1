-- Cuda Plugin

-- In this tutorial we load a custom lua extension (.dll)
-- which runs a cuda kernel on a vidbuffer. The vidbuffer is
-- used as vertices for a l3dprimitive usermesh.


-- obligatory hardware check
if (not (
		rendersystem.vendor() == "nvidia" and
		technique.cg_sm4():supported() and
		os.getenv("CG_BIN_PATH") and
		)) then

	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality. Or CUDA not installed"))

	return (function() lbl:delete() end)
end


if (cudatest) then
	
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"This sample can only run once."))

	return (function() lbl:delete() end)
end

-- As our plugin resides in the tutorial projectpath
-- we must add the path as search directory for lua's
-- dll loading mechanism

do	
	-- first try to convert the path to absolute path
	local abspath = system.projectpath()
	if (not abspath:find(":/")) then
		local curdir = FileSystem.currentdir()
		curdir = curdir:gsub("\\","/")
		abspath = abspath:sub(1,1) == "/" and abspath or "/"..abspath
		abspath = curdir..abspath
	end
	
	-- add the dll search dir
	package.cpath = package.cpath..abspath.."/plugins/?.dll;"..abspath.."/plugins/?.so;"

end

-- load our plugin (without .dll extension)
require "cudatest"

-- Similar setup as in previous tutorial (T037)
-- import some functions and create the line
-- object
dofile "Shared/advrendering.lua"

Grid = {}
function Grid : new(size,dim)
	local this = {}
	setmetatable(this,{__index = self})
	local self = this
	
	self.size = size
	self.dim  = dim
	local vtype = vertextype.vertex16color()
	local vcount = dim*dim
	local icount = dim*dim
	
	local vbo,ibo,is16bit = ARCreateStaticVBOs(vtype,
			vcount,icount,true)
			
	self.vbo = vbo
	self.ibo = ibo
	self.is16bit = is16bit
	
	local act = actornode.new("")
	act:rotdeg(90,0,90)
	local obj = l3dprimitive.newbox("obj",size,size,size*0.5)
	obj:linkinterface(act)
	
	obj:usermesh(	vtype,
					vcount,
					icount,
					vbo,0,ibo,0)

	-- map the indexbuffer
	local pfirst,plast = ibo:map(vidmapping.discard())

	local sa = scalararray.newfrompointer(pfirst,plast,
		is16bit and scalartype.uint16() or scalartype.uint32(),1)
	
	for i=0,icount-1 do
		sa:vector(i,i)
	end
	
	-- and finish
	sa = nil
	self.ibo:unmap()

	obj:vertexCount(vcount)
	obj:indexCount(icount)
	
	obj:indexPrimitivetype(primitivetype.linestrip())
	obj:indexMinmax(0,vcount-1)
	obj:indexTrianglecount()
	--obj:color(0.85,0.7,0.3,0)
	obj:rfNovertexcolor(false)
	obj:rsLinewidth(2)
	obj:renderscale(size,size,size)
	obj:rfBlend(true)
	obj:rsBlendmode(blendmode.add())
	obj:rfNodepthtest(true)
	
	--obj:renderscale(size/dim,size/dim,size/dim)
	
	self.act = act
	self.l3d = obj
	
	return self
end

function Grid : getVBO()
	return self.vbo
end

function Grid : getDim()
	return self.dim
end

gridobj = Grid:new(2.5,128)

-- Now it gets interesting again,
-- we must tell cuda that we want to work
-- with the vidbuffer.
-- Therefore we need to pass the native glid,
-- to our plugin which does the cuda calls.

cudatest.registerbuffer(gridobj:getVBO():glid(true))
cudatest.runtestkernel(gridobj:getVBO():glid(true),
				gridobj:getDim(),gridobj:getDim(),system.time())

-- in our little frame func we call the cuda
-- kernel with a different animation value every frame
-- You can set this to false if you want just a static
-- image
if (true) then
	local t = system.time()
	Timer.set("tutorial",
		function()

			local nt = system.time()
			nt = (nt - t)/2000
			
			local dim = gridobj:getDim()
			cudatest.runtestkernel(gridobj:getVBO():glid(true),dim,dim,nt)
			
		end)

else
	cudatest.unregisterbuffer(gridobj:getVBO():glid(true))
end

-- rest setup as usual
-- initialization and light / camera setup
-- although we dont use the light here
window.refsize(window.width(),window.height())

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.15,0.2,0.25,0) 

cam = actornode.new("cam",0,8,0)
cam.camera = l3dcamera.default()
cam.camera:linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,500)
sun.light = l3dlight.new("sun")
sun.light:linkinterface(sun)
sun.light:makesun()
