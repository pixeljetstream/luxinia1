-- Shader Effects, Specularmapping

-- <Resources>
-- models/t33.f3d
-- textures/t33comb.png
-- textures/t33diff.png
-- textures/t33spec.png
-- textures/t33illum.png
-- shaders/3tex_diffspecillum.shd
-- gpuprogs/simplelighting.cg

-------------------
-- basic scene
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

actor = actornode.new("t33",0,0,0)
local t33 = model.load("t33.f3d")
actor.l3d = l3dmodel.new("t33",t33)
actor.l3d:linkinterface(actor)

sun = actornode.new("sun",100,-100,500)
sun.light = l3dlight.new("sun")
sun.light:ambient(.2,.2,.2,1)
sun.light:linkinterface(sun)
sun.light:makesun()

-- because specularity is a effect that shows better in
-- motion, as it depends on eye/reflect vector, we rotate
-- the model

-- rotation of object
-- we want the object to rotate with a fixed predefined rotation
local origrottm = matrix4x4.new()
origrottm:rotdeg(40,-20,40)
local rotatortm = matrix4x4.new()

function rotit ()
	local t = system.time()
	--local rotmat
	rotatortm:rotrad(0,0,t*0.001)
	rotatortm:mul(origrottm)
	
	-- set position fix
	rotatortm:pos(0,15,0)
	
	actor:matrix(rotatortm)
end

Timer.set("tutorial",rotit,20)
-- rotit is now called every 20 milliseconds
 
 
------------------------------
-- Shader / Material setup

-- the model originally used "t33small.tga" as texture
-- we want to replace this assignment with our shader material

-- therefore we first need to create a material, which we then
-- assign to the meshes of the l3dmodel instance

-- Material creation can be done in two ways
-- 1. material.load("filename.mtl") which is a predefined materialfile,
--		and offers most functionality
-- 2. material.load("MATERIAL_AUTO:...") which creates a simple material
--		from the string

-- Because our material/shader combo isnt very complex we will use the
-- String generation method

local mtl = material.load(
	"MATERIAL_AUTO:3tex_diffspecillum.shd|t33comb.png|"..
	"t33diff.png|t33spec.png|t33illum.png")
-- Our material uses one shader "3tex_diffspecillum.shd"
-- and 4 textures. Depending on hardware capability the shader
-- will load the textures it uses. The texture slots are filled 
-- ascending 0,1,2 ... same goes with the shaders. 
-- You must always define at least one shader

-- now we want to assign the material to the l3dmodel
-- l3dmodel.matsurface requires us to pass a meshid as well
-- we simply assign the material to all meshes
for i=1,t33:meshcount() do
	-- create a meshid 
	local mid = t33:meshid(i-1)	-- meshid indices start at 0
	-- assign material to l3dmodel's mesh
	actor.l3d:matsurface(mid,mtl)
end
-- set Renderflags AFTER material assignments, as shader's
-- renderflag might overwrite things
actor.l3d:rfLitSun(true)
