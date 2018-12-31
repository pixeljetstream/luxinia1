-- User Mesh with VBO

-- In this tutorial we use the vidbuffer class
-- to create hardware buffers for vertex and index
-- storage. We assign them as usermesh to a
-- l3dprimitive


-- at first a little precheck
if (not capability.vbo()) then

	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality."))

	return (function() lbl:delete() end)
end


-- lets start with a little helper function, that creates
-- one vbo,ibo pair for a given vertextype

local function createStaticVBOs(vtype,vertcount,indexcount,doallocs)

	local vsize = vtype:memsize()*vertcount
	local vbo = vidbuffer.new(vidtype.vertex(),vidhint.draw(1),
								vsize)

	
	-- indices are either 16bit or 32bit depending on vertexcount
	local is16bit = vertcount <= 65536
	local isize = (is16bit and 2 or 4)*indexcount
	local ibo = vidbuffer.new(vidtype.index(),vidhint.draw(1),
								isize)
	
		
	if (doallocs) then
	
		-- We fill our vbos manually, that means their
		-- internal content offsets are managed by us.
	
		-- Another option would be creating many usermeshes
		-- and they would be packed sequentially into the same
		-- vbo. Which would call localalloc for each usermesh.
		
		-- Ideally you would want to pack a good deal of
		-- geometry into buffers, as changing a buffer is a costly
		-- operation during rendering.
		
	
		ibo:localalloc(isize)
		vbo:localalloc(vsize)
	end
	
	return vbo,ibo,is16bit
end


-- We want to draw a torus in this sample, and make use of the
-- oo class system.
Torus = {}
function Torus : new (oradius,iradius,sides,rings)
	local this = {}
	setmetatable(this,{__index = self})
	local self = this
	
	self.oradius = oradius or 0.5
	self.iradius = iradius or 0.25
	
	self.sides = sides or 16
	self.rings = rings or 16
	-- we want a small vertex with normals
	self.vtype = vertextype.vertex32normals()

	local vbo,ibo,is16bit = createStaticVBOs(self.vtype,
		self:getVertexCount(),
		self:getIndexCount(),
		true)

	self.vbo = vbo
	self.ibo = ibo
	self.is16bit = is16bit
	
	self:fillContent()
	
	return self
end

function Torus : fillContent()
	-- lets fill our torus with content!
	-- hardware buffers are typically not accessible
	-- directly, we must "map" them and get pointers.
	
	-- we want to overwrite the whole thing
	local pfirst,plast = self.vbo:map(vidmapping.discard())
	
	-- To work with those pointers	
	-- we create a vertexarray handle,
	-- as that makes it more conveniant to fill
	-- the vbo.
	
	-- Be aware that you are responsible to keep the pointers
	-- valid. Ie dont access them once unmapped.
	-- And make sure the vbo is kept alive!
	
	local va = vertexarray.newfromptrs(self.vtype,pfirst,plast)
	
	
	------
	-- create the vertex data
	-- this is based on the glut implementation
	
	local nRings = self.rings
	local nSides = self.sides
	local oradius = self.oradius
	local iradius = self.iradius
	
	local dpsi =  2.0 * math.pi / (nRings - 1) ;
	local dphi = -2.0 * math.pi / (nSides - 1) ;
	local psi  = 0.0;
	
	for  j=0,nRings-1 do

		local cpsi = math.cos ( psi ) ;
		local spsi = math.sin ( psi ) ;
		local phi = 0.0;

		for i=0,nSides-1 do

			local offset = ( j * nSides + i ) ;
			local cphi = math.cos ( phi ) ;
			local sphi = math.sin ( phi ) ;
			
			va:vertexPos(offset,
				cpsi * ( oradius + cphi * iradius ),
				spsi * ( oradius + cphi * iradius ),
				sphi * iradius)
			
			va:vertexNormal(offset,
				cpsi * cphi,
				spsi * cphi,
				sphi)
			
			phi = phi + dphi;
		end

		psi = psi + dpsi;
	end
	

	-- we are done with the vertices
	-- just to make sure we dont access them accidently
	va = nil
	self.vbo:unmap()
	
	-- map the indexbuffer
	local pfirst,plast = self.ibo:map(vidmapping.discard())
	
	-- this time we use the scalararray
	local sa = scalararray.newfrompointer(pfirst,plast,
		self.is16bit and scalartype.uint16() or scalartype.uint32(),4)
	
	------
	-- create the quad indices
	
	local idx = 0
	for  j=0,nRings-2 do
		for i=0,nSides-2 do
			local offset = ( j * nSides + i ) ;
			sa:vector(idx,
						offset,
						offset+1,
						offset+nSides+1,
						offset+nSides)
			
			idx = idx + 1
		end
	end
	
	-- and finish
	sa = nil
	self.ibo:unmap()
	
end

function Torus : getVertexType()
	return self.vtype
end

function Torus : getVertexCount()
	return self.sides*self.rings
end

function Torus : getIndexCount()
	return (self.sides-1)*(self.rings-1)*4
end

function Torus : getVBOData()
	-- We have already manually filled 
	-- our vbo allocations, therefore we 
	-- know that data starts at beginning.
	-- Every usemesh will use the same instance
	-- of the data.
	
	return self.vbo,0,self.ibo,0
end

function Torus : fillMesh(mesh)
	mesh:vertexCount(self:getVertexCount())
	mesh:indexCount(self:getIndexCount())
	
	mesh:indexPrimitivetype(primitivetype.quads())
	mesh:indexMinmax(0,self:getVertexCount()-1)
	mesh:indexTrianglecount()
end

function Torus : getRendermesh()
	-- lazy evaluation, ie dont create
	-- if noone asks for it
	if ( not self.rmesh) then
		self.rmesh = rendermesh.new(self.vtype,
					self:getVertexCount(),
					self:getIndexCount(),
					self:getVBOData())
	
		self:fillMesh(self.rmesh)
	end
	
	return self.rmesh
end

function Torus : makeUsermesh(mesh)
	mesh:usermesh(self.vtype,
					self:getVertexCount(),
					self:getIndexCount(),
					self:getVBOData())
					
	self:fillMesh(mesh)
end

-- create our torus intance, with lots of triangles
local tsize = 1.75
local tsep = 64
local tor128 = Torus:new(tsize*0.5,tsize*0.25,tsep,tsep)

-- a little function to create l3d objects
local function maketorusl3d(tor,link)
	local obj = l3dprimitive.newbox("obj",tsize,tsize,tsize*0.5)
	obj:linkinterface(act)

	-- two options either we give each torus its own
	-- usermesh, or we let them all share
	-- a common rendermesh, created by torus
	
	if (false) then
		tor:makeUsermesh(obj)
	else
		obj:rendermesh(tor:getRendermesh())
	end
	
	obj:renderscale(1,1,1)
	
	return obj
end

act = actornode.new("")

-- lets do a little effect work
-- render two tori, one with wireframe
l3dset.layer(0)
objsolid = maketorusl3d(tor128,act)
objsolid:color(0.8,0.8,1,0)
objsolid:rfLitSun(true)

l3dset.layer(1)
objwire = maketorusl3d(tor128,act)
objwire:color(1,1,0,0)
objwire:rfNodepthtest(true)
objwire:rfNocull(true)
objwire:rfWireframe(true)
objwire:rfBlend(true)
objwire:rsBlendmode(blendmode.add())
objwire:rsLinewidth(2)
objwire:rfLitSun(true)


-- rest setup as usual
-- initialization and light / camera setup
window.refsize(window.width(),window.height())

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.3,0.4,0.5,0) 

cam = actornode.new("cam",6,3,2)
cam.camera = l3dcamera.default()
cam.camera:linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,500)
sun.light = l3dlight.new("sun")
sun.light:linkinterface(sun)
sun.light:makesun()


local t = 0
Timer.set("tutorial",
	function()

		t = t + .01
		act:rotdeg(t*90,t*70,0)
		
	end,20)

