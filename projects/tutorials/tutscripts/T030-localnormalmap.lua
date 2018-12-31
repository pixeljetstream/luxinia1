-- TODO Localspace Normal Mapping

if (true) then
	return NOTREADY()
end

-- <Resources>
-- models/mirroredlocalmap.f3d
-- shaders/normalmap_local.shd
-- gpuprogs/localnormalmap.cg


-------------------
-- basic scene
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

model.loaderprescale(0.05,0.05,0.051)
local mdlname = "mirroredlocalmap.f3d"

actor = actornode.new(mdlname,0,0,0)
local mdl = model.load(mdlname,false,true,true)
actor.l3d = l3dmodel.new(mdlname,mdl)
actor.l3d:linkinterface(actor)

sun = actornode.new("sun",-3,10,10)
sun.light = l3dlight.new("sun")
sun.light:ambient(.2,.2,.2,1)
sun.light:linkinterface(sun)
sun.light:makesun()
sun.sphere = l3dprimitive.newsphere("sun",0.03)
sun.sphere:linkinterface(sun)

-- because specularity is a effect that shows better in
-- motion, as it depends on eye/reflect vector, we rotate
-- the model

-- rotation of object
-- we want the object to rotate with a fixed predefined rotation
local origrottm = matrix4x4.new()
--origrottm:rotdeg(40,-20,40)
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

local mtl = material.load(
	"MATERIAL_AUTO:normalmap_local.shd|nocompress;Box01NormalsMap.tga","-DUSEMIRROR;")

for i=1,mdl:meshcount() do
	local mid = mdl:meshid(i-1)	
	actor.l3d:matsurface(mid,mtl)
end
actor.l3d:rfLitSun(true)
