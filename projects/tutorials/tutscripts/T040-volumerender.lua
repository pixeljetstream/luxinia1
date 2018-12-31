-- Basic GPU Volume Rendering

-- <Resource>
-- shaders/volraycast.shd
-- gpuprogs/volraycast.cg
-- shaders/volpos.shd
-- gpuprogs/volpos.cg
-- shaders/volfrontplane.shd



-- In this tutorial we create a basic volume
-- raycaster.


-- obligatory hardware check
if (not (
		technique.cg_sm3():supported() 
		and capability.tex3d()
		)) then

	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality."))

	return (function() lbl:delete() end)
end

function matobjmatrix(mobj,ctrl,matrix)
	mobj:moControl(ctrl,0,matrix:row(0))
	mobj:moControl(ctrl,1,matrix:row(1))
	mobj:moControl(ctrl,2,matrix:row(2))
	mobj:moControl(ctrl,3,matrix:row(3))
end

function matobjvector(mobj,ctrl,vec)
	mobj:moControl(ctrl,unpack(vec))
end

-- implement VolRender as main frame container, for some
-- ui integration. At the moment it can only be instanced
-- once. Otherwise we would need a bit more complex
-- rendersetup and resource management.

VolRender = {}
setmetatable(VolRender,{__index = Container})
function VolRender.new(class,x,y,w,h,camctrl,tw,th)
	local self = Container.new(class,x,y,w,h)
	-- managing this uid could allow 
	-- multiple instances
	self.uid = 1
	self.rendersize = {tw or 256,th or 256}
	
	self.camctrl = camctrl
	self.res = self:initres()
	self.vis = self:initvis()
	self.view = self:initview()
	self.drawnextframe = -1
	
	self.camlistener = function(cam,camact)
		self:updatedCamera(cam,camact)
	end
	
	camctrl:addListener(self.camlistener)

	self:addMouseListener(camctrl:createML(self))
	
	self.camthink = function()
		camctrl:runThink()
	end
	
	return self
end

function VolRender:onDestroy()
	self.camctrl:removeListener(self.camlistener)
end

function VolRender:setVolumeTex(tex)
	if (tex and tex.formattype and tex:formattype() == "3d") then
		self.vis.rcout:moTexcontrol(self.res.mcvol,tex)
		self.voltex = tex
		self.candraw = self.hull~=nil
	else
		self.candraw = false
		self.voltex  = nil
	end
end

function VolRender:setHullMesh(rmesh,wmatrix,obj2volmatrix,size)
	size = size or {1,1,1}
	obj2volmatrix = obj2volmatrix or matrix4x4.new()
	wmatrix = wmatrix or matrix4x4.new()

	self.hull = {}
	self:updatedHull(wmatrix,obj2volmatrix,size)
	
	self.vis.rcfront:rendermesh(rmesh)
	self.vis.rcback:rendermesh(rmesh)
	
	self.candraw = self.voltex~=nil
end

function VolRender:updatedHull(wmatrix,obj2volmatrix,size)
	local hull = self.hull
	hull.wmatrix = wmatrix or hull.wmatrix
	hull.obj2volmatrix = obj2volmatrix or hull.obj2volmatrix
	hull.size = size or hull.size
	
	matobjmatrix(self.vis.rcfront,self.res.mcobj2vol,hull.obj2volmatrix)
	matobjmatrix(self.vis.rcback,self.res.mcobj2vol,hull.obj2volmatrix)
	
	self.vis.rcfront:matrix(hull.wmatrix)
	self.vis.rcback:matrix(hull.wmatrix)
	self.vis.rcfront:size(unpack(hull.size))
	self.vis.rcback:size(unpack(hull.size))
	
	self:updatedMatrix()
end

function VolRender:updatedCamera(cam,camact)
	self.view:camera(cam)

	local fov = cam:fov()
	local aspect = cam:aspect()
	local frontplane = cam:frontplane()
	
	local vec
	if (fov < 0) then
		-- update frontplane
		local fov = -fov * 0.5
		vec = {fov,fov/aspect,frontplane,1}
	else
		local fovx = fov * 0.5
		local fovy = (fov/aspect) * 0.5
		
		fovx = math.tan(fovx)*frontplane
		fovy = math.tan(fovy)*frontplane
		
		vec = {fovx,fovy,frontplane,1}
	end
	matobjvector(self.vis.rcnear,self.res.mccamattribs,vec)
	
	self.cammatrix = camact:matrix()
	
	self:updatedMatrix()
	self:draw()
end

function VolRender:updatedMatrix()
	local hull = self.hull
	if (hull and self.cammatrix) then
		local cammat = self.cammatrix
		local screen2vol = matrix4x4.new()
		local invscale = matrix4x4.new()
		local invworld = matrix4x4.new()
		
		local size = hull.size
		invscale:scale(1/size[1],1/size[2],1/size[3])
		invworld:affineinvert(hull.wmatrix)
		
		invscale:mul(invworld)
		local campos = {invscale:transform(cammat:pos())}
		
		invscale:mul(cammat)
		screen2vol:mul(hull.obj2volmatrix,invscale)
		
		matobjmatrix(self.vis.rcnear,self.res.mcscreen2vol,screen2vol)
		matobjvector(self.vis.rcout,self.res.mcvrcam,campos)
		matobjvector(self.vis.rcout,self.res.mcvrscale,hull.size)
		
	end
end

function VolRender:draw()
	self.drawnextframe = system.frame()+1
	self.view:rcmdflag(0,self.candraw)
end

function VolRender:getView()
	return self.view
end

function VolRender:getTexOut()
	return self.res.texout
end

function VolRender:setRenderSize(w,h)
	
	self.vis.fbo:size(w,h)
	self.vis.rbo:setup(textype.depth(),w,h)
	self.vis.fboout:size(w,h)
	
	self.view:viewsize(w,h)
	
	self.res.texstart:resize(w,h)
	self.res.texend:resize(w,h)
	self.res.texout:resize(w,h)
	
end

function VolRender:setRayControl(x,y,z,w)
	local a,b,c,d = self:getRayControl()
	self.vis.rcout:moControl(self.res.mcvrctrl,x or a,y or b,z or c,w or d)
	self:draw()
end

function VolRender:getRayControl()
	return self.vis.rcout:moControl(self.res.mcvrctrl)
end

function VolRender:think()
	if (self.drawnextframe) then
		if (system.frame() > self.drawnextframe+30) then
			self.view:rcmdflag(0,false)
			self.drawnextframe = nil
		end
	end
end

------------
--- internals


function VolRender:initres()
	-- GPU Raycasting is mostly done by creating two textures in imagespace,
	-- that encode our ray start- and endpositions. Typically float16 
	-- precision is enough for this.
	
	-- if we wanted multi-instances, we would use uid
	-- to create or reuse old resources.
	
	local uid = self.uid
	local w,h = unpack(self.rendersize)
	local raytype = (capability.texfloat() and technique.cg_sm4():supported()) and texdatatype.float16() or texdatatype.fixed8()
	local res = {
		texstart = texture.create2d("raystart"..uid,w,h,textype.rgba(),false,true,
							false,raytype),
		texend = texture.create2d("rayend"..uid,w,h,textype.rgba(),false,true,
							false,raytype),
		texout = texture.create2d("rayout"..uid,w,h,textype.rgba(),false,false,
							false,texdatatype.fixed8()),

		-- The materials and shaders needed.
		-- The first two are there to generate our entry/exit position images.
		-- They simply write out the object positions in [0,1] space.
		-- However special care must be taken when our frontplane clips stuff.

		mtlpos = material.load("fxs/volraycast/volpos.mtl"),
		mtlnear = material.load("fxs/volraycast/volfrontplane.mtl"),

		-- This one performs the actual raycasting
		mtlout = material.load("fxs/volraycast/volraycast.mtl"),
	}
	
	-- get some controls
	res.mcobj2vol = res.mtlpos:getcontrol("obj2vol")
	res.mcscreen2vol = res.mtlnear:getcontrol("screen2vol")
	res.mccamattribs = res.mtlnear:getcontrol("camattribs")
	
	res.mcvol = res.mtlout:gettexcontrol("volume")
	res.mcvrstart = res.mtlout:gettexcontrol("raystart")
	res.mcvrend = res.mtlout:gettexcontrol("rayend")
	
	res.mcvrctrl = res.mtlout:getcontrol("control")
	res.mcvrcam = res.mtlout:getcontrol("campos")
	res.mcvrscale = res.mtlout:getcontrol("volscale")
	
	return res
end

function VolRender:initvis()
	-- these values would always be per instance
	local w,h = unpack(self.rendersize)
	local vis = {
		fbo = renderfbo.new(w,h),
		fboout = renderfbo.new(w,h),
		rbo = renderbuffer.new(textype.depth(),w,h),
		rcfront = rcmddrawmesh.new(),
		rcback = rcmddrawmesh.new(),
		rcout = rcmddrawmesh.new(),
		rcnear = rcmddrawmesh.new(),
	}
	vis.rcfront:orthographic(false)
	vis.rcback:orthographic(false)

	vis.rcnear:autosize(-1)
	vis.rcout:autosize(-1)

	vis.rcfront:matsurface(self.res.mtlpos)
	vis.rcback:matsurface(self.res.mtlpos)
	vis.rcnear:matsurface(self.res.mtlnear)
	
	vis.rcback:rfFrontcull(true)
	
	vis.rcout:matsurface(self.res.mtlout)
	print(self.res.mtlout,self.res.mcvrstart)
	vis.rcout:moTexcontrol(self.res.mcvrstart,self.res.texstart)
	vis.rcout:moTexcontrol(self.res.mcvrend,self.res.texend)
	
	
	-- assign fbodata
	local rctex = rcmdfbotex.new() 
	rctex:color(0,self.res.texend)
	rctex:color(1,self.res.texstart)
	
	
	local rcrb = rcmdfborb.new()
	rcrb:depth(vis.rbo)
	vis.rctexin = rctex
	vis.rcrbin = rcrb
	
	local rctex = rcmdfbotex.new() 
	rctex:color(0,self.res.texout)
	vis.rctexout = rctex
		
	self:attachFBO(vis)
	
	return vis
end

function VolRender:attachFBO(vis)
	local vis = vis or self.vis
	vis.fbo:applyattach(vis.rctexin,vis.rcrbin)
	print("FBO CHECK",vis.fbo:check())
	vis.rctexin = nil
	vis.rcrbin = nil
	
	vis.fboout:applyattach(vis.rctexout)
	print("FBOOUT CHECK",vis.fboout:check())
	vis.rctexout = nil
end

-- This version here allows complex hull geometry,
-- ie more than a simple box.
-- However the hull must be a watertight closed surface.
-- view in which rendercmds should
-- be inserted.
function VolRender:initview()
	local view = l3dview.new()
	local vis = self.vis
	view:windowsized(false)
	view:viewsize(unpack(self.rendersize))
	
	view:filldrawlayers(false)
	
	-- render start/end
	local rc = rcmdfbobind.new()
	rc:setup(self.vis.fbo)
	view:rcmdadd(rc)
	
	---
	-- render farthest backfaces
	local rc = rcmdfbodrawto.new()
	rc:setup(0)
	view:rcmdadd(rc)
	
	local rc = rcmdclear.new()
	rc:depthvalue(0)
	view:rcmdadd(rc)
	
	local rc = rcmddepth.new()
	rc:compare(comparemode.greater())
	view:rcmdadd(rc)
	
	view:rcmdadd(self.vis.rcback)

	----
	-- render closest frontfaces/nearplane
	local rc = rcmdfbodrawto.new()
	rc:setup(1)
	view:rcmdadd(rc)
	
	local rc = rcmdclear.new()
	rc:depthvalue(1)
	view:rcmdadd(rc)
	
	-- drawnearplane (dont write depth)
	view:rcmdadd(self.vis.rcnear)
	
	local rc = rcmddepth.new()
	rc:compare(comparemode.less())
	view:rcmdadd(rc)
	
	
	-- render backfaces (closest), dont write color (protect nearplane)
	local rc = rcmdforceflag.new()
	rc:rfNocolormask(true)
	view:rcmdadd(rc)

	view:rcmdadd(self.vis.rcback)
	
	local rc = rcmdforceflag.new()
	view:rcmdadd(rc)
	
	
	-- render frontfaces (might be clipped by nearplane)
	-- nearplane protected by previous backface z
	view:rcmdadd(self.vis.rcfront)

	----
	-- do raycast into out fbo
	local rc = rcmdfbobind.new()
	rc:setup(self.vis.fboout)
	view:rcmdadd(rc)
	
	local rc = rcmdfbodrawto.new()
	rc:setup(0)
	view:rcmdadd(rc)
	
	view:rcmdadd(self.vis.rcout)
	
	view:rcmdadd(rcmdfbooff.new())
	
	print("fbotest",l3dlist.fbotest(view))

	-- disable all rcmds first and 
	-- enable once a valid texture was bound
	view:rcmdflag(0,false)	
	
	return view
end

function VolRender:mousePressed()
	if (self ~= Component.getFocusElement()) then
		self:focus()
	end
end

function VolRender:onGainedFocus()
	Timer.set("VolRender"..tostring(self),self.camthink)
end

function VolRender:onLostFocus()
	Timer.remove(self.camthink)
end

function VolRender:isFocusable() return true and self:isVisible() end

-- we are always fullscreen
function VolRender:onParentLayout()
	self:setBounds(0,0,window.refsize())
	self.camctrl:runListeners()
end


---------------------
-- Build our mini application
local root = Container.getRootContainer()
view = UtilFunctions.simplerenderqueue()
local volsize = 64
do	-- camctrl
	local camact = actornode.new("cam")
	local cam = l3dcamera.default()
	cam:linkinterface(camact)
	
	camact:pos(volsize*2,volsize*2,volsize*2)
	camact:lookat(volsize/2,volsize/2,volsize/2,0,0,1)

	camctrl = CameraEgoMKCtrl:new(cam,camact,
					{shiftmulti = 5,movemulti = 10.0, fix = true,})
	
	view:camera(cam)
	view:drawcamaxis(true)
end
do
	-- myvol
	local ww,wh = window.refsize()
	myvol = VolRender:new(0,0,ww,wh,camctrl,128,128)
	root:add(myvol)
end

-- create volume texture and assign it
do
	local size = 32
	local datatype = texdatatype.fixed8()
	local voltex = texture.create3d("vol",size,size,size,textype.lum(),true)
	-- this may take a while and so we do it only once
	-- for the time the tutorial framework is loaded (because
	-- resources are kept alive between tutorial toggles)
	
	-- we use the userstring to mark our texture as processed already
	-- this is done only to speed up time when toggling between tutorials,
	-- there are no restrictions to updating a texture's content
	
	if (voltex:resuserstring() ~= "s") then
	
		local function sphere(coord)
			local dist = ExtMath.v3len(ExtMath.v3sub(coord,{0.5,0.5,0.5}))
			dist = 1-math.min(1,math.max(0,dist/0.5))
			return dist
		end
	
		local function raster(coord)
			local x,y,z = unpack(ExtMath.v3scale(coord,32))
			local dist = math.sin((x*math.pi*0.1)+1)+math.cos((y*math.pi*0.4))+math.sin((z*math.pi*0.07)+3)
			dist = math.cos(dist*0.5*math.pi)
			dist = math.max(math.min(1.0,dist),0.0)
			
			return dist
		end

		local function marschnarlobb(coord)
			local f = 6
			local a = 0.25
			
			--local x,z,y = unpack(ExtMath.v3scaledadd({-1,-1,-1},coord,2))
			local x,z,y = unpack(coord)
			
			local function pr(x)
				return math.cos(2*f*math.pi*math.cos(math.pi*x/2))
			end
			
			return ((1-math.sin(math.pi*z/2))+(a*(1+pr(math.sqrt(x*x+y*y)))))/
					(2*(1+a))
		end

		local invtex = 1/(size-1)
		for z=1,size do
			for y=1,size do
				for x=1,size do
					local coord = {(x-1)*invtex,(y-1)*invtex,(z-1)*invtex}
					local a = raster(coord)
					--local a = marschnarlobb(coord)
					voltex:pixel(x-1,y-1,z-1,a,a,a,a)
				end
			end
		end
		voltex:uploaddata()
		
		voltex:resuserstring("s")
	end
	
	l3dlist.fbotest()
	
	myvol:setVolumeTex(voltex)
end



-- create boxmesh and assign it
do
	local function makeboxmesh(min,max)
		local mesh = rendermesh.new(vertextype.vertex16color(),8,24)
		mesh:vertexCount(8)
		mesh:vertexPos(0,min[1],min[2],min[3])
		mesh:vertexPos(1,max[1],min[2],min[3])
		mesh:vertexPos(2,max[1],min[2],max[3])
		mesh:vertexPos(3,min[1],min[2],max[3])
		mesh:vertexPos(4,max[1],max[2],min[3])
		mesh:vertexPos(5,max[1],max[2],max[3])
		mesh:vertexPos(6,min[1],max[2],min[3])
		mesh:vertexPos(7,min[1],max[2],max[3])
		
		mesh:indexPrimitivetype(primitivetype.quads())
		mesh:indexCount(24)
		mesh:indexPrimitive(0,0,1,2,3)
		mesh:indexPrimitive(1,1,4,5,2)
		mesh:indexPrimitive(2,4,6,7,5)
		mesh:indexPrimitive(3,6,0,3,7)
		mesh:indexPrimitive(4,3,2,5,7)
		mesh:indexPrimitive(5,6,4,1,0)
		mesh:indexMinmax(0,7)
		mesh:indexTrianglecount()
		
		return mesh
	end

	local defmesh = makeboxmesh({0,0,0},{1,1,1})
	myvol:setHullMesh(defmesh,nil,nil,{volsize,volsize,volsize})
end


do	-- draw setup
	-- we could draw other stuff in the main view
	local volview = myvol:getView()
	-- by default is inserted before "view" is drawn,
	-- therefore all data will be ready for the final
	-- blit
	volview:activate(l3dset.default())
	
	-- our volume rendering is finally blended on top
	local rcdraw = rcmddrawmesh.new()
	rcdraw:autosize(-1)
	rcdraw:rfBlend(true)
	rcdraw:rsBlendmode(blendmode.decal())
	rcdraw:matsurface(myvol:getTexOut())
	view:rcmdadd(rcdraw,view.rDrawdebug,0,true)
	
	
end


-- add little controller gui
do
	local gf = myvol:add(GroupFrame:new(0,0,32,32))
	local y = 5
	local gfw = 300
	GH.sldLabelwidth = 64
	GH.sldLabelval = 64

	y = GH.addslider(gf,5,y,gfw-10,16,"samples",
		{
			callback = function(val)
				myvol:setRayControl(val/volsize)
				return val
			end,
			initvalue = 1/4,
			scale = 256,
			formatstr = "%.1f",
		})
		
	y = GH.addslider(gf,5,y,gfw-10,16,"density",
		{
			callback = function(val)
				myvol:setRayControl(nil,nil,val)
				return val
			end,
			initvalue = 0.1,
			scale = 1,
			formatstr = "%.2f",
		})
		
	y = GH.addslider(gf,5,y,gfw-10,16,"falloff range",
		{
			callback = function(val)
				myvol:setRayControl(nil,val)
				return val
			end,
			initvalue = 1,
			scale = 1024,
			formatstr = "%.1f",
		})
		
		
		
	gf:setBounds(0,0,300,y+5)
end


-- kickoff
camctrl:runListeners()
-- think (handles rendering of l3dview)
Timer.set("tutorial", function()
	myvol:think()
	end)

-- overload root layouter, it is called if windowsize
-- changes
root.doLayout = function(rt)
	for a,b in ipairs(rt.components) do
		if (b.onParentLayout) then
			b:onParentLayout()
		end
	end
end

