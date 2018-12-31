-- Basic scene, a simple lit sphere

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

-- a sphere
actor = actornode.new("actor",0,10,0)
actor.l3d = l3dprimitive.newsphere("sphere",1,1,1)
actor.l3d:linkinterface(actor)
actor.l3d:rfLitSun(true)

-- sunlight
mysun = actornode.new("sun",100,200,600)
mysun.light = l3dlight.new("light")
mysun.light:linkinterface(mysun)
mysun.light:makesun()

mysun.light:diffuse(.8,.5,0,1)
mysun.light:ambient(.1,.2,.5,1)

-- camera
cam = actornode.new("camera",0,5,5)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,10,0, 0,0,1)

-- movement for camera
function moveit ()
  local t = os.clock()
  local x,y = math.sin(t) * 10,math.cos(t)*10
  cam:pos(x,y,5)
  cam:lookat(0,10,0,0,0,1)
end

Timer.set("tutorial",moveit,20)
 -- movit is now called every 20 milliseconds

