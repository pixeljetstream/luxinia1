-- Basic scene, keyboard controls the sphere

-- scene setup like in tutorial 002
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

actor = actornode.new("actor",0,0,0)
actor.l3d = l3dprimitive.newsphere("box",1,1,1)
actor.l3d:linkinterface(actor)
actor.l3d:rfLitSun(true)

mysun = actornode.new("sun",100,200,600)
mysun.light = l3dlight.new("light")
mysun.light:linkinterface(mysun)
mysun.light:makesun()

cam = actornode.new("camera",3,5,2)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

-- this function polls the input state
function think ()
  local left = Keyboard.isKeyDown("LEFT")
  local right = Keyboard.isKeyDown("RIGHT")
  local up = Keyboard.isKeyDown("UP")
  local down = Keyboard.isKeyDown("DOWN")
  local fx = (left and 1 or 0) - (right and 1 or 0)
  local fy = (up and 1 or 0) - (down and 1 or 0)
  -- fx,fy: movement directions
  local x,y,z = actor:pos()
  actor:pos(x+fx*.1,y+fy*.1,z)
end

Timer.set("tutorial",think,20)