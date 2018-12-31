-- Static particles

-- In this tutorial we will use "static" particles. Compared
-- to "dynamic" particles, you can control each particle
-- directly. Despite their name you can move them freely
-- as well. Just "dynamic" particles use the ".prt" particle
-- scripts, which allow automatic spawning and movement effects
-- like gravity.
-- Static particles are ideal for detail objects, or when you
-- want custom particle system simulation. They use the 
-- "particlecloud" resource class, while dynamic use the
-- "particlesystem".


-- basic scene setup
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(.8,.8,.8,1) 

cam = actornode.new("camera",8,5,6)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,0,0, 0,0,1)


-- Now lets create a particle cloud, that can have up to 512
-- particles.
cloud = particlecloud.create("bushes",512)

-- This function allows us to change the particletype
-- Luxinia allows quite many different particle types,
-- here we show then all. With the function its easier
-- to change between types.

function setcloudtype(typename)
	-- if set already and same type, do nothing
	if (cloud.typename == typename) then return end
	cloud.typename = typename
	
	-- revert to defaults if previoues type was set
	cloud:matsurface(matsurface.vertexcolor())
	cloud:instancemesh("_")
	
	if typename == "point" then
		-- Points or pointsprites are the most basic particle.
		-- Their size is typpically the amount of pixels
		-- at 640x480 (luxinia scales their size according
		-- to current screen resolution) and they are rendered
		-- as small quads. You can make them round and
		-- change size based on distance with additionally
		-- commands.
		-- Be aware that they have a maximum size limit that
		-- depends on the hardware (often 64 pixels). They only
		-- appear when their center is visible on screen.
		
		cloud:typepoint() 
		cloud:pointsize(5)
		
	elseif typename == "triangle" then
		-- A single triangle, similar to quad below.
		cloud:typetriangle()
		
	elseif typename == "quad" then
		-- The classic billboard quad is the most common used
		-- particletype. You can rotate, scale, texture them
		-- individually.
		
		cloud:typequad()
	elseif typename == "hemisphere" then
		-- A 5 triangles arranged as circle, with the center point
		-- being pushed forward. It is rendered just like triangle,
		-- and quad before, but its curved surface allows for
		-- better vertex lighting.
		
		cloud:typehemisphere()
	elseif typename == "model" then
		-- Luxinia can also render models, as particles. Each
		-- particle will be a random "mesh" of the model, and
		-- use the meshes' individual material. This type
		-- is the slowest of all.
	
		cloud:typemodel ()
		local mdl =assert(model.load "t33.f3d","model not found")
		cloud:model(mdl)

	elseif typename == "instancemesh" then
		-- A variant of the model-type are instanced meshes.
		-- One mesh of the model is used for all particles, and
		-- can be rendered fairly efficient. Each particle can
		-- be individually textured. So you could have a "grass"
		-- mesh, using different textures for variety.
		
		cloud:typemodel ()
		local mdl =assert(model.load "t33.f3d","model not found")
		cloud:model(mdl)
		-- one mesh of the model is used for all particles
		cloud:instancemesh(mdl:meshid(0):name())
		-- assign particles the original texture from the model
		cloud:matsurface(mdl:meshid(0):texture())
	elseif typename == "sphere" then
		-- The sphere is rendered similar to the model. Its
		-- geometry is a fairly low-poly sphere.
		
		cloud:typesphere()
	end
end

-- initial type
setcloudtype("point")

-- we want the particles to have individual colors
cloud:particlecolor(true)


-- To make use of the particecloud we need to create the 
-- equivalent l3dnode, the l3dpgroup. Each group can have
-- their own set of particles, which can be linked to other
-- spatialnodes or bones... or you set positions indiviually.
-- Positions can be in local coordinates (ie final is tranformed
-- by l3dnode's final matrix) or world coordinates (positions
-- are used unaltered).

cube = actornode.new("actor")
plink = l3dpgroup.new("particle owner", cloud)
plink:linkinterface(cube)
plink:typeworld()	-- let's use unaltered positions

-- Use a loop to create a random set of points in the 
-- l3dproup and give them random colors.

local function r(n) return (math.random()-.5)*n end
local s = math.random
for i=1,512 do
	-- spanw the particle at a random position
	local p = plink:spawn(r(3),r(3),r(3),1)
	-- change particle's color
	plink:prtColor(p,s(),s(),s(),1)
	-- and size
	plink:prtSize(p,.1)
end

local types = {"point","quad","triangle","hemisphere","sphere","model","instancemesh"}
local curtypeindex = 1

-- add a little gui for toggling the particle type
do
	frame = Container.getRootContainer():add(GroupFrame:new(10,10,128,52))
	btn = frame:add(Button:new(10,8,128-20,20,"Change Type"))
	lbl = frame:add(Label:new(10,28,128-20,16,"point",Label.LABEL_ALIGNCENTERED))
	function btn:onClicked()
		-- raise by one with each press, but make sure we are in
		-- range of 1 to #types
		curtypeindex = math.mod(curtypeindex,#types) + 1
		-- set type and change label text
		setcloudtype(types[curtypeindex])
		lbl:setText(types[curtypeindex])
	end
end

-- Lets make a little camera animation, so that our
-- camera moves on a circle around particles.
Timer.set("tutorial",function ()
	local curtime = os.clock()
	cam:pos(math.sin(curtime)*10,math.cos(curtime)*10,5)
	cam:lookat(0,0,0, 0,0,1)
end)

-- You will notice that particles overlap each other and are drawn
-- in the same order you created them. You can prevent this by
-- activating zbuffer writing:
-- cloud:rfNoDepthmask(false)
-- By default particles are drawn without writing into zbuffer.
-- Or you can sort particles based on distance to camera.
-- cloud:sort(true)
-- This is particular interesting when using the blendmode.decal.

