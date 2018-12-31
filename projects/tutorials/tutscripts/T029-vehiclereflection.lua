-- Carmodel with Reflection

-- <Resources>
-- models/car.f3d
-- models/car_wheel_l.f3d
-- models/car_wheel_r.f3d
-- models/car_shadow.f3d
-- materials/chassis.mtls
-- shaders/1color_cubemapsky.shd
-- models/enviro/...

-- In this tutorial we create a little scene with dynamic
-- reflections. Rasterisation pipelines as common in todays
-- 3d games/applications cannot do accurate reflections like
-- raytracers easily. We have to use a workaround and store
-- reflections for each object's center into a cubemap.
-- There is no self-reflection nor is it very accurate
-- when shared among multiple objects, however in many
-- games you will notice that it doesn't have to be that
-- precise.

-- First we create a car and link wheels
-- to the car's wheel bones. Then we create the cameras
-- with FXUtils.getCubeMapCameras, and setup render-to-cubemap
-- needed for the relfections.
-- At the end of the tutorial we add a little scenery
-- and a little animation to the world.



-- Let's create a simple skybox so our background is a bit
-- more interesting. We use a "matrix"-style texture.
local str = "tech_02.jpg"
bgskybox = skybox.new(str,str,str,str,str,str)

-- Create view as usual
view = UtilFunctions.simplerenderqueue()
view:bgskybox(bgskybox)
-- turn color clearing off, as skybox will do the job
view.rClear:color(false)

cam = l3dcamera.default()


-- With this function we set the visibility to each camera
-- for a l3dnode. Every camera has a unique bit-id, the
-- default is bit 0 = true, which is also the bit-id for
-- the default camera. For the six cubemap sides we use
-- ids 1 to 6.

-- We apply this visibility flag to all l3d objects that are
-- visible to the cubemap cameras, so that it is visible
-- in the reflections.
local function cubevisflags(l3d)
	for i=1,6 do 
		l3d:visflag(i,true)
	end
end
	

--------
-- Start with the visual car
-- load the model with normals supported, but no animations needed.
--
-- We chante the default l3dlayerid, because we want to add
-- a very simple "blob" shadow to the scene. Therefore
-- we use 3 layers: 0 for ground, 1 for shadows, 2 for objects

l3dset.layer(2)

-- we scale the car visually down a bit, as the enviroment
-- models came from a different project with other scale.
-- "loadprescale" allows us to change the mesh's dimensions
-- it's a bit slower as all vertices/bones need to be processed,
-- but it's on runtime faster than using renderscale/rfNormalize.
-- The scaling factor remains active for all following model.load
-- calls.
model.loaderprescale(0.033,0.033,0.033)

local carmodel = model.load("car.f3d",false,true)
-- load wheels. We have one for each side, so that wheel spinning
-- is a bit easier. You could however do the same with a single
-- wheel model and inverting spinning direction for one side.
local carwheelmodel_r = model.load("car_wheel_r.f3d",false,true)
local carwheelmodel_l = model.load("car_wheel_l.f3d",false,true)

-- our main actor for the car
caractor = actornode.new("car")

-- create & link l3d equivalents
-- chassis
carl3d = l3dmodel.new("car",carmodel,false,false)
carl3d:linkinterface(caractor)
carl3d:rfLitSun(true)



-- add wheels to the car. The car model has bone positions
-- for each wheel.
wheell3ds = {}
local function addwheel(bonename,wheelmodel,rimname)
	-- retrieve boneid and get its local matrix
	local bone = carmodel:boneid(bonename)
	local matrix = bone:matrix()
	
	-- create l3d and link it to chassis
	local l3d = l3dmodel.new(bonename,wheelmodel,false,false)
	l3d:rfLitSun(true)
	l3d:parent(carl3d)
	l3d:localmatrix(matrix)
	
	l3d.rimmid = wheelmodel:meshid(rimname)
	table.insert(wheell3ds,l3d)
end

addwheel("wheel_joint00",carwheelmodel_l,"rim")
addwheel("wheel_joint01",carwheelmodel_l,"rim")
addwheel("wheel_joint02",carwheelmodel_r,"rim02")
addwheel("wheel_joint03",carwheelmodel_r,"rim02")

-- a simple drop shadow
-- set default layer higher, so that its drawn after the ground
l3dset.layer(1)
carshadowmodel = model.load("car_shadow.f3d",false,true)
carshadowl3d = l3dmodel.new("car_shadow",carshadowmodel,false,false)
carshadowl3d:linkinterface(caractor)
-- Draw without depthtest and blend set to modulate, so that it
-- darkens the ground model. Modulate blend is 
-- output = source * destination. Which means if source pixel
-- is < 1 (ie less than white) the destination pixel becomes 
-- darker a bit. The carshadowmodel uses a pre-baked shadow-map
-- of the car, and therefore will look like a realistic shadow.
-- We disable depthtest so that there is no z-fighting with the
-- ground model. Be aware that this only works now as we know
-- the ground is flat and shadow is always above it.
-- Later tutorials will feature more complex texture-based 
-- shadowing methods.

local function makeshadowblend(l3d)
	l3d:rfNodepthtest(true)
	l3d:rfBlend(true)
	l3d:rsBlendmode(blendmode.modulate())
	cubevisflags(l3d)
end
makeshadowblend(carshadowl3d)



--------
-- setup the dynamic cubemap reflection

-- create the cubemap for reflections. This time we make the 
-- demo a bit backwards compatible, ie it should run without
-- reflections as well.

if (capability.texcube()) then
	-- create the cubemap. For more sharpness, raise this value
	-- but make sure it stays a power-of-2 number.
	local cubesize = 128
	local reflectcube = texture.createcube("carreflect",
					cubesize,textype.rgba(),false)
	
	render.drawtexture(reflectcube)
	-------
	-- Material
	do
		-- load the reflective material, which needs the
		-- "carreflect" texture to be created before.
		local reflectmtl = material.load("chassis.mtl")
		
		-- apply the material to certain meshes
		carl3d:matsurface(carmodel:meshid("chassis"),reflectmtl)
		carl3d:matsurface(carmodel:meshid("glass"),reflectmtl)
		
		-- and the wheels rims, although this is not physically
		-- correct.
		for i,l3d in ipairs(wheell3ds) do
			l3d:matsurface(l3d.rimmid,reflectmtl)
		end
	end
	
	--------
	-- Cube Map cameras
	do
		cubeviews = {}
		cubecameras = {}
		
		-- In this function the main work is done. For each 
		-- cubeside a l3dview is created and its output is
		-- stored into a side of the cubemap texture.
		-- We could use renderfbo to write into texture, but for
		-- small texture sizes (here 128x128) simply copying from
		-- the backbuffer is sufficiently fast. 
		
		local function addcubeviews(cubeside,cam)
			-- creat the l3dview and setup its size
			-- and renderqueue
			
			local view = l3dview.new()
			view = UtilFunctions.simplerenderqueue(view)
			view:windowsized(false)
			view:viewsize(cubesize,cubesize)
			
			-- activate it, by default is added "first", ie
			-- before our default view which will be our 
			-- final output.
			
			view:activate(l3dset.default())
			
			-- assign the view a specific camera
			-- and make sure it clears background
			view:camera(cam)
			view.rClear:depth(true)
			view.rClear:stencil(true)
			view.rClear:color(false)
			
			view:bgskybox(bgskybox)
			
			-- at the end of the renderqueue we add a rcmdcopytex,
			-- which will copy current framebuffer to texture.
			-- By default it copies the size of the viewport.
			local rc = rcmdcopytex.new()
			rc:tex(reflectcube)
			rc:side(cubeside)
			view:rcmdadd(rc)
			
			-- prevent gc
			table.insert(cubecameras,cam)
			table.insert(cubeviews,view)
		end
		
		
		-- FXUtils.getCubeMapCameras is used to create the cameras, 
		-- and links them to the main actor.
		
		local cams = FXUtils.getCubeMapCameras(1,caractor,
								cam:frontplane(),cam:backplane())
		
		for i=1,6 do
			addcubeviews(i-1,cams[i])
		end
		
	end
end

-- The main effect work is done now, the rest is simply adding
-- models to the scene so we have something that gets reflected.
-- At the end a simple automatic camera movement as added as 
-- 'think' function.

--------
-- Add a little scenery with trees so we have things that are
-- reflected. 

-- These scenery models came from different project and should 
-- not use any prescaling.
model.loaderprescale(1,1,1)

local carshadowbbox = {carshadowmodel:bbox()}

groundscale = 10
groundlevel = carshadowbbox[3] -- minz as groundlevel

-- revert default layer back to 0
l3dset.layer(0)

-- sun to make scene more interesting
mysun = actornode.new("sun",100,200,600)
mysun.light = l3dlight.new("light")
mysun.light:linkinterface(mysun)
mysun.light:makesun()
local diff = 0.7
mysun.light:diffuse(4*diff,1.8*diff,1.6*diff)
local ambi = 0.3
mysun.light:ambient(0.4*ambi,0.3*ambi,0.2*ambi)


-- a visual floor
ground = scenenode.new("ground",0,0,groundlevel)
ground.l3d = l3dprimitive.newquadcentered("ground",groundscale*12,groundscale*12,1)
ground.l3d:linkinterface(ground)
ground.l3d:color(0,.5,0,1)
ground.l3d:rfLitSun(true)
ground.l3d:matsurface(texture.load("grasstile.jpg"))
ground.l3d:moRotaxis(4,0,0,	0,4,0,	0,0,1)
cubevisflags(ground.l3d)


scene = {}
local function addscene(mdlname,x,y,noshadow)
	local treemdl = model.load(mdlname,false,true)
	local treescn = scenenode.new("tree",x,y,groundlevel)
	treescn:localrotrad(0,0,math.random()*math.pi*2)
	
	-- trees we draw later again
	l3dset.layer(3)
	local treel3d = l3dmodel.new("tree",treemdl,false,false)
	treel3d:linkinterface(treescn)
	treel3d:rfLitSun(true)
	cubevisflags(treel3d)
	
	-- go thru all meshes and disable them if they have no
	-- texture applied
	for i=1,treemdl:meshcount() do
		local mid = treemdl:meshid(i-1)
		if (mid:name():find("invisible")) then
			treel3d:rfNodraw(mid,true)
		end
	end
	
	if (not noshadow) then
		l3dset.layer(1)
		treel3dshadow = l3dprimitive.newquadcentered("shadow",20,20,1)
		treel3dshadow:linkinterface(treescn)
		treel3dshadow:matsurface(texture.load("treeshadow.jpg"))
		makeshadowblend(treel3dshadow)
	end
		
	table.insert(scene,{treescn,treel3d,treel3dshadow})
end

-- Trees
local treenames = {"baum.f3d","baum2.f3d","baum3.f3d",
					"baum4.f3d","baum5.f3d","baum6.f3d"}
local treesquare = 4.5*groundscale


for i=1,12 do
	local tree = treenames[math.mod(1,#treenames)+1]

	local radius = 35
	local angle = math.random()*math.pi*2
	local x,y = math.cos(angle)*radius,math.sin(angle)*radius
	
	addscene("enviro/"..tree,x,y)
end
-- Stones
for i=1,10 do
	local radius = ExtMath.interpolate(math.random(),4,10)
	local angle = math.random()*math.pi*2
	local x,y = math.cos(angle)*radius,math.sin(angle)*radius
	addscene("enviro/stein"..(math.mod(i,5)+1)..".f3d",x,y,false)
end

-- camera
camact = actornode.new("camera",0,-treesquare*1.5,40)
camact:lookat(0,0,0,0,0,1)
cam:linkinterface(camact)

---------
-- Motion
-- The idea is that we have the car moving on a circle with wheels
-- spinning. Slightly behind the car will move the camera and 
-- getting closer while moving up and down a bit in a continous
-- smooth motion

local radius = 27
local angle = 0
addangle = true
caractor:pos(radius,0,0)

function think ()
	angle = angle + (addangle and 0.008 or 0)
	
	local carx,cary = math.cos(angle)*radius,math.sin(angle)*radius
	caractor:pos(carx,cary,0)
	caractor:rotrad(0,0,angle-(math.pi/2))
	
	for i,l3d in pairs(wheell3ds) do
		l3d:localrotrad(0,-angle*20,0)
	end
	
	local camspeed = 0.7
	local camradius = 22
	local camangle = angle - math.pi*0.3 + (math.cos(angle*camspeed)*0.7) - 0.2
	
	camact:pos(	math.cos(camangle)*camradius,
				math.sin(camangle)*camradius,
				ExtMath.interpolate(math.sin(angle*camspeed)*0.5+0.5,
									3,13))
	camact:lookat(carx,cary,0,0,0,1)
	
end


Timer.set("tutorial",think,20)

-- keybinding to toggle motion
local function toggleangle()
	addangle = not addangle
end
Keyboard.setKeyBinding(Keyboard.keycode.P,toggleangle,200)

-- and a little text about it
infotext = l2dtext.new("info","\vsUse P to toggle animation")
infotext:parent(l2dlist.getroot())