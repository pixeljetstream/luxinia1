-- Lua-based Lightmap Generator 

-- author: Christoph Kubisch

-- Lightmap generation is typically done by raycasting
-- from our surfaces points towards the lightsources.
-- If the lightsource is visible, diffuse lighting 
-- is performed for this point, otherwise
-- it's in the shadow (ambient).
--
-- In this tutorial, we will create a sample scene
-- and assign all models their own lightmap. We will
-- throw all triangles in a collision space for raytesting.
--
-- At the core of a lightmapper lies a rasterizer, which
-- turns our mesh's texture coordinates into texel coordinates
-- of our lightmap texture. For every texel we get the 
-- surface position and normal in the world, which we can
-- then use for raycasting and illumination. Finally
-- we write the outcome at this texel of the texture.

----
-- The Rasterizer

-- So let's start with the rasterizer engine
-- The code is based on an article series by Chris Hecker
--	http://chrishecker.com/Miscellaneous_Technical_Articles
--	"Perspective Texture Mapping" by Chris Hecker
-- And on a C++ implementation by Diogo Teixeira

-- Our vertex has a 2d screenposition within [0,1]
-- And various interpolators, whose values will be
-- interpolated over the triangle's pixels.

-- spos must be a table with 2 numbers
-- interpolators host multiple tables of arbitrary length
-- 
-- We could hardcode it to only allow say position or
-- normal for interpolation, but while we are at it,
-- we can make it work for any vectors passed.

function RVertex(spos,interpolators)
	return {s=spos,i=interpolators}
end

-- Compute the gradients for the interpolators
-- gradient is "rate of change"

function RGradients(v1,v2,v3)
	-- interpolators must match
	local ic = #v1.i
	assert((ic == #v2.i) and (ic == #v3.i))
	
	local y13 = (v1.s[2]-v3.s[2])
	local y23 = (v2.s[2]-v3.s[2])
	local x13 = (v1.s[1]-v3.s[1])
	local x23 = (v2.s[1]-v3.s[1])
	local ox = (x23*y13)-(x13*y23)
	
	--DBL_EPSILON	2.2204460492503131e-016
	--FLT_EPSILON	1.192092896e-07F
	-- let's use float eps, although lua uses double
	
	ox = (math.abs(ox) > 1.192092896e-07) and (1.0 / ox) or 0.0;
	local oy = -ox;
	
	grads = {}
	
	-- in a shader this would be equivalent of the result
	-- by ddx/ddy
	
	for i=1,ic do
		local x = {}
		local y = {}
		
		local i1 = v1.i[i]
		local i2 = v2.i[i]
		local i3 = v3.i[i]
		assert((#i1 == #i2) and (#i2 == #i3))
		
		local cc = #i1
		for c=1,cc do
			table.insert(x,ox* ((i2[c]-i3[c])*y13 - (i1[c]-i3[c]) * y23))
			table.insert(y,oy* ((i2[c]-i3[c])*x13 - (i1[c]-i3[c]) * x23))
		end
		
		table.insert(grads,{x,y})
	end
	
	return grads
end

function NewObj(class)
	local self = {}
	setmetatable(self,{__index = class})
	return self
end

REdge = {}
function REdge:new(top, bot, grads)
	local self = NewObj(self)
	
	local topy = top.s[2]- 0.5
	local boty = bot.s[2]- 0.5
	local fw = bot.s[1]-top.s[1]
	local fh = boty - topy
	fh = (math.abs(fh) > 1.192092896e-07) and (1.0 / fh) or 0.0
	
	-- get the values at the lower-left centroids, once
	-- edge was rastered
	
	local yend = math.ceil(boty)
	local y = math.ceil(topy)
	local ypre = y - topy
	self.height = yend - y
	local x = ((fw* ypre)* fh)+ top.s[1]
	local xpre = x- top.s[1]
	local xstep = fw* fh
	self.xstep = xstep
	self.y = y
	self.x = x
	

	-- precompute starting positions of gradients
	-- on the centroid grid
	-- and their increments when walking along the line
	-- in y direction
	
	local ctr = {}
	local ic = #top.i
	for i=1,ic do
		local ti = top.i[i]
		local gi = grads[i]
		
		-- start values and incrementors
		local s = {}
		local a = {}
		
		local cc = #ti
		for c=1,cc do
			table.insert(s,ti[c] + (xpre* gi[1][c]) + (ypre* gi[2][c]))
			table.insert(a,(xstep* gi[1][c]) + gi[2][c])
		end
		
		table.insert(ctr,{s,a})
	end
	self.ctr = ctr
	return self
end

function REdge:step()
	self.y = self.y+1
	self.x = self.x+self.xstep
	
	-- update ctr positions
	local ctr = self.ctr
	
	local ic = #ctr
	for i=1,ic do
		local ii = ctr[i]
		
		local cc = #ii[1]
		for c=1,cc do
			ii[1][c] = ii[1][c] + ii[2][c]
		end
	end
end

Rasterizer = {}

-- callback(x,y,interpolated,grads)
function Rasterizer:scanline(left,right,grads,callback)

	local x1 = math.ceil(left.x- 0.5)
	local x2 = math.ceil(right.x- 0.5)
	local y = left.y
	if (x2-x1 > 0) then

		local xpre = x1- left.x;
		
		-- interpolate start
		local int = {}
		local ic = #grads
		
		for i=1,ic do
			local ii = left.ctr[i][1]
			local gi = grads[i][1]
			
			local vi = {}
			
			local cc = #ii
			for c=1,cc do
				vi[c] = ii[c] + (xpre*gi[c])
			end
			table.insert(int,vi)
		end


		x2 = x2-1
		for x = x1,x2 do
			callback(x, y, int, grads)

			-- interpolate step x
			for i=1,ic do
				local vi = int[i]
				local gi = grads[i][1]
				
				local cc = #vi
				for c=1,cc do
					vi[c] = vi[c] + gi[c]
				end
			end

		end
	end
end

function Rasterizer:triangle(v1,v2,v3, callback)
	-- sort along y
	if (v1.s[2] > v3.s[2]) then
		v1,v3 = v3,v1
	end
	if (v1.s[2] > v2.s[2]) then
		v1,v2 = v2,v1
	end
	if (v2.s[2] > v3.s[2]) then
		v2,v3 = v3,v2
	end
	
	-- find out if middle vertex is left or right of the top2bot edge
	local midleft = ((v3.s[1]- v1.s[1])* (v2.s[2]- v1.s[2])-
					(v2.s[1]- v1.s[1])* (v3.s[2]- v1.s[2]) > 0)
	
	
	
	-- compute gradients
	local grads = RGradients(v1, v2, v3)
	
	-- and the centroid-grid aligned edges that define this triangle
	local top2bot = REdge:new(v1,v3,grads)
	local top2mid = REdge:new(v1,v2,grads)
	local mid2bot = REdge:new(v2,v3,grads)
	
	-- now raster the hell out of the it!
	-- first compute the upper "half" of the triangle
	
	local left,right = unpack((midleft) and {top2mid,top2bot} or {top2bot,top2mid})
	local height = top2mid.height
	while (height > 0) do
	
		self:scanline(left, right, grads, callback);
		left:step()
		right:step()
		
		height = height-1
	end
	
	-- then the lower
	local left,right = unpack((midleft) and {mid2bot,top2bot} or {top2bot,mid2bot})
	local height = mid2bot.height
	while (height > 0) do
	
		self:scanline(left, right, grads, callback);
		left:step()
		right:step()
		
		height = height-1
	end
end

---
-- The Lightmapper

LightMapper = {}
LightMapper.ray = dgeomray.new()
LightMapper.coll = dcollresult.new(128)
LightMapper.nodge = 0.01
LightMapper.padruns = 4
LightMapper.texsize = 256

local t_edge = 0
local t_pixel = 0
local t_raster = 0
local p_raster = 0
local t_total = 0

function LightMapper:prep(l3dlist)
		
	for k,l3d in pairs(l3dlist) do
		local lstring = tostring(l3d)
		local s,e,unique = string.find(lstring,"%@(.+)")
		local mdl = l3d:getmodel()
		local mcnt = mdl:meshcount()-1
		
		l3d.lmtex = texture.create2d("lightmap"..unique,self.texsize,self.texsize,textype.rgba())
		l3d:lightmap(l3d.lmtex)
	end
end

function LightMapper:run(l3dlist,collspace,lights)
	for k,l3d in pairs(scenel3ds) do
		t_total = t_total - system.getmillis()
		
		local mdl = l3d:getmodel()
		local mcnt = mdl:meshcount()-1
		
		-- TODO 
		-- derive texture size from geometric area
		--local nsize = 256
		local tex = l3d.lmtex
		--tex:resize(nsize,nsize)
		
		tex:downloaddata()
		local sarr = tex:mountscalararray()
		sarr:op0(scalarop.clear0())
		
		for i=0,mcnt do
			self:runmesh(mdl:meshid(i),l3d:worldmatrix(),tex,collspace,lights)
		end
		
		t_edge = t_edge - system.getmillis()
		for i=1,self.padruns do
			self:edgepad(tex)
		end
		t_edge = t_edge + system.getmillis()
		
		tex:uploaddata()
		tex:deletedata()
		
		t_total = t_total + system.getmillis()
		
		coroutine.yield()
	end
	
	LightMapper.coll:clear()
end

function LightMapper:edgepad(tex)
	local tw,th = tex:dimension()
	tw = tw-1
	th = th-1
	
	local function nbr(x,y)
		local x = math.min(tw,math.max(x,0))
		local y = math.min(th,math.max(y,0))
		
		return tex:pixelbyte(x,y)
	end
	
	local ad4 = ExtMath.v4add
	print("edgepadding...",tex)
	for y=0,th do
		for x=0,tw do
			local r,g,b,a = tex:pixelbyte(x,y)
			if (a < 1) then
				local clr = {0,0,0,0}
				clr = ad4(clr,{nbr(x-1,y-1)})
				clr = ad4(clr,{nbr(x,  y-1)})
				clr = ad4(clr,{nbr(x+1,y-1)})
				
				clr = ad4(clr,{nbr(x-1,y)})
				clr = ad4(clr,{nbr(x+1,y)})
				
				clr = ad4(clr,{nbr(x-1,y+1)})
				clr = ad4(clr,{nbr(x,  y+1)})
				clr = ad4(clr,{nbr(x+1,y+1)})
				
				if (clr[4] > 0) then
					tex:pixelbyte(x,y,unpack(ExtMath.v4scale(clr,1.0/(clr[4]/255))))
				end
			end
		end
	end
end


function LightMapper:makeTexelCallback(tex,collspace,lights)
	-- cache vars for speed
	local ray = self.ray
	local col = LightMapper.coll
	
	local sub = ExtMath.v3sub
	local len = ExtMath.v3len
	local scl = ExtMath.v3scale
	local sca = ExtMath.v3scaledadd
	local ad4 = ExtMath.v4add
	local dot = ExtMath.v3dot
	local nrm = ExtMath.v3normalize
	local ndg = self.nodge
	local tw,th = tex:dimension()
	
	return function(x,y,int,grad)
		
		t_pixel = t_pixel - system.getmillis()
		
		assert(x >= 0 and x < tw and y >= 0 and y < th)
		
		local normal = nrm(int[2])
		local pos = sca(int[1],normal,ndg)
		
		
		local clr = {0.2,0.2,0.2,1}
		
		for i,l in ipairs(lights) do
			-- direction to light
			
			local d = sub(l.pos,pos)
			local ln = len(d)
			local dn = scl(d,1.0/ln)
			
			ray:length(ln)
			ray:set(pos[1],pos[2],pos[3],dn[1],dn[2],dn[3])
			
			local n = col:test(ray,collspace)
			
			if (n==0) then
				n = math.max(0,dot(dn,normal))
				clr = ad4(clr,{n,n,n,1})
			end
		end
		
		tex:pixel(x,y,clr[1],clr[2],clr[3],clr[4])
		
		--tex:pixel(x,y,1,1,1,1)
		
		p_raster = p_raster + 1
		t_pixel = t_pixel + system.getmillis()
		
	end
end

function LightMapper:makeVertexBuilder(mesh,wmatrix,tw,th)
	return function(v)
		local pos = {mesh:vertexPos(v,wmatrix)}
		local normal = {mesh:vertexNormal(v,wmatrix)}
		
		local tx,ty = mesh:vertexTexcoord(v,1)
		
		return RVertex({tx*tw,ty*th},{pos,normal})
	end
end

function LightMapper:runmesh(mesh,wmatrix,tex,collspace,lights)
	-- rasterize all triangles in uvspace
	local tw,th = tex:dimension()

	assert(mesh:indexPrimitivetype() == primitivetype.triangles())
	
	local callback = self:makeTexelCallback(tex,collspace,lights)
	local vbuild = self:makeVertexBuilder(mesh,wmatrix,tw,th)
	
	local tricnt = mesh:indexPrimitivecount()-1
	print("rasterizing...",mesh,mesh:name())
	for i=0,tricnt do
		local a,b,c = mesh:indexPrimitive(i)
		
		-- skip degenerated
		if (not (a==b or b==c or c==a)) then
			
			-- run the computation
			t_raster = t_raster - system.getmillis()
			Rasterizer:triangle(vbuild(a),vbuild(b),vbuild(c), callback)
			t_raster = t_raster + system.getmillis()
		end
		
		
		--print(string.format("%.2f",i*100/tricnt))
	end
end

---------------------------------
--- Setup scene
--
-- consist of a pseudo tree and some
-- random boxes.

-- scene data will be put in here
scenelights = {}
scenel3ds = {}


local view = UtilFunctions.simplerenderqueue()
local mdltree = model.load("lmtree.f3d",false,true,true)
local mdlbox = model.load("lmbox.f3d",false,true,true)

do
	-- create the l3dmodel nodes
	-- and setup camera

	local function addmodel(mdl,x,y,z,tex)
		local l3d = l3dmodel.new("scn",mdl)
		l3d.scn = scenenode.new("tree",true,x,y,z)
		local mcnt = mdl:meshcount()
		for i=0,mcnt-1 do
			l3d:matsurface(mdl:meshid(i),tex)
			l3d:moRotaxis(mdl:meshid(i),0.3,0,0, 0,0.3,0, 0,0,0.3)
		end
		
		l3d:linkinterface(l3d.scn)
		table.insert(scenel3ds,l3d)
	end
	
	local function getmodeldim(mdl)
		local minx,miny,minz,maxx,maxy,maxz = mdl:bbox()
		local sx,sy,sz = maxx-minx,maxy-miny,maxz-minz
		
		return {min={minx,miny,minz},
				max={maxx,maxy,maxz},
				size={sx,sy,sz}}
	end

	addmodel(mdltree,0,0,0,texture.load("tech_03.jpg"))
	
	local dimbox = getmodeldim(mdlbox)
	local dimtree = getmodeldim(mdltree)
	
	local f = 1
	for y=-f,f do
		for x=-f,f do
			local nz = (x==0 and y==0) and 0 or math.random()
			local sx = dimbox.size[1]
			local sy = dimbox.size[2]
			local sz = dimbox.size[3]
			
			local minx = dimbox.min[1]
			local miny = dimbox.min[2]
			local minz = dimbox.min[3]
			
			addmodel(mdlbox,(x*sx)-minx-(sx/2),(y*sy)-miny-(sy/2),-minz-sz+(-nz*sz),
					texture.load("tech_02.jpg"))
		end
	end
	
	cam = l3dcamera.default()
	camact = actornode.new("cam",true,0,-f*dimbox.size[2]*7,dimbox.size[3]*3)
	camact:lookat(0,0,0, 0,0,1)
	cam:linkinterface(camact)
	cam:backplane(dimbox.size[2]*f*2*6)
	
	-- create a light
	table.insert(scenelights,{pos=ExtMath.v3mul({f,-f,2},dimbox.size)})
end


do
	-- build collision mesh
	-- it's most efficient to throw all triangles in one big
	-- soup for a small scene like this.
	
	l3dnode.updateall()
	
	local verts = {}
	local inds = {}
	
	local function addcolldata(mdl,iarr,varr,wmatrix)
		local mcnt = mdl:meshcount()-1
		for n=0,mcnt do
			local m = mdl:meshid(n)
			assert(m:indexPrimitivetype() == primitivetype.triangles())
			
			local voff = (#verts)/3
			local tricnt = m:indexPrimitivecount()-1
			local vertcnt = m:vertexCount()-1
			
			for i=0,tricnt do
				local a,b,c = m:indexPrimitive(i)
				
				-- skip degenerated
				if (not (a==b or b==c or c==a)) then
					table.insert(inds,a+voff)
					table.insert(inds,b+voff)
					table.insert(inds,c+voff)
				end
			end
			
			for i=0,vertcnt do
				local p = {m:vertexPos(i,wmatrix)}
				table.insert(verts,p[1])
				table.insert(verts,p[2])
				table.insert(verts,p[3])
			end
		end
	end
	
	for k,l3d in pairs(scenel3ds) do
		local mdl = l3d:getmodel()
		addcolldata(mdl,inds,verts,l3d:worldmatrix())
	end
	
	scenecollgeom = dgeomtrimesh.new(dgeomtrimeshdata.new(inds,verts))
	scenetris = (#inds/3)
end

-- a little UI
local w,h = 250,200
local gf = Container.getRootContainer():add(GroupFrame:new(0,0,w,h))
local lbl = gf:add(Label:new(5,5,w-10,h-10,"Running..."))

------
-- DO IT
function doit()
	LightMapper:prep(scenel3ds)
	--for i=1,10 do
	LightMapper:run(scenel3ds,scenecollgeom,scenelights)
	GarbageCollector.collect()
	--end
	
	local str = ""
	local function mprint(...)
		print(...)
		for i,v in ipairs({...}) do
			str = str.." "..(tostring(v))
		end
		str = str.."\n"
	end
	
	mprint ("scenenodes",#scenel3ds,"scenetris:",scenetris)
	mprint ("lightmapper time:",(t_total)/1000)
	mprint ("lightmapper edgepad:",string.format("%.2f %s",(t_edge/t_total)*100,"%"))
	mprint ("lightmapper texel:",string.format("%.2f %s",(t_pixel/t_total)*100,"%"))
	mprint ("lightmapper raster:",string.format("%.2f %s",((t_raster-t_pixel)/t_total)*100,"%"))
	mprint ("lightmapper rays:",string.format("%d",p_raster))

	lbl:setText(str)
	
	local ctrl = CameraEgoMKCtrl:new(cam,camact,
					{shiftmulti = 5,movemulti = 10.0})
	
	MouseCursor.showMouse(false)
	
	Timer.set("tutorial",function()
		ctrl:runThink()
	end)
end


TimerTask.new(doit,1000)





