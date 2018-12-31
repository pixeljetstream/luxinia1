-- Basic scene, physics interaction

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

mysun = actornode.new("sun",100,200,600)
mysun.light = l3dlight.new("light")
mysun.light:linkinterface(mysun)
mysun.light:makesun()

cam = actornode.new("camera",13,15,12)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

-- the object now with physic controller
actor = actornode.new("actor",0,0,0)
actor.l3d = l3dprimitive.newsphere("sphere",1,1,1)
actor.l3d:linkinterface(actor)
actor.l3d:rfLitSun(true)
actor.body = dbody.new()
actor.body:masssphere(1,1)
actor:link(actor.body)

-- collision spaces
globalspace = dspacehash.new()
staticspace = dspacehash.new(globalspace) 
staticspace:collidetest(false) 	-- don't make collision 
 								-- tests in this space
-- collision object for our sphere
actor.geom = dgeomsphere.new(globalspace)
actor.geom:body(actor.body)

----
-- other bodies
actors = {}
function addbox (x,y,z, w,h,d)
  local actor = actornode.new("box")
  table.insert(actors,actor)
  actor.body = dbody.new()
  actor.body:pos(x,y,z)
  actor.geom = dgeombox.new(w,h,d,globalspace)
  actor.l3d = l3dprimitive.newbox("box",w,h,d)
  actor.geom:body(actor.body)
  actor:link(actor.body)
  actor.l3d:linkinterface(actor)
  actor.l3d:rfLitSun(true)
  actor.l3d:color(1,0,0,1)
  actor.body:massbox(1,w,h,d)
end

addbox(4,2,0, 3,2,.5)
addbox(-4,-2,0, 1,2,.5)

---
-- visible ground
ground = scenenode.new("floor",0,0,-2)
ground.l3d = l3dprimitive.newquadcentered("ground",20,20,1)
 -- never set a scaling factor to 0 - it will result
 -- in strange behaviour because the normals of the 
 -- object might become useles after the transformation
ground.l3d:linkinterface(ground)
ground.l3d:color(0,.5,0,1)
ground.l3d:rfLitSun(true)



-- some world limitations
walls = {
  dgeomplane.new(1,0,0,-10, staticspace),
  dgeomplane.new(-1,0,0,-10, staticspace),
  dgeomplane.new(0,1,0,-10, staticspace),
  dgeomplane.new(0,-1,0,-10, staticspace),
  dgeomplane.new(0,0,1,-2, staticspace),
}

-- gravity
dworld.gravity(0,0,-.01)

function think ()
  local left = Keyboard.isKeyDown("LEFT")
  local right = Keyboard.isKeyDown("RIGHT")
  local up = Keyboard.isKeyDown("UP")
  local down = Keyboard.isKeyDown("DOWN")
  local fx = (left and 1 or 0) - (right and 1 or 0)
  local fy = (up and 1 or 0) - (down and 1 or 0)
  -- fx,fy: movement directions

  -- apply physics
  actor.body:addforce(fx*.01,fy*.01,0)
  -- check collisions
  dworld.collidetest(globalspace)
  dworld.makecontacts()
  -- run sim
  dworld.quickstep(1)
end

Timer.set("tutorial",think,20)