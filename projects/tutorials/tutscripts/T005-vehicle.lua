-- Cartest: Simple driving physics

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

dworld.surfacecombination(0,1, 1) -- 0/1 combination, use surfacetype 1

dworld.surfacebitfdir(1,true) -- enable the friction direction
dworld.surfacemu(1,50)
dworld.surfacebitmu2(5,true) -- enable mu2
dworld.surfacemu2(1,0.5) 	-- low friction in the other direction
dworld.gravity(0,0,-.01)	-- gravity

globalspace = dspacehash.new()
staticspace = dspacehash.new(globalspace) 
  -- add it to globalspace
staticspace:collidetest(false)

-- create the frame
local cw,cl,ch = 2,3,1 -- width, length, height
car = dbody.new()
car.space = dspacehash.new(globalspace)
car.space:collidetest(false)
-- the actor serves as bridge between physics and visuals
car.actor = actornode.new("frame")
car.actor:link(car)
-- the collision object
car.geom = dgeombox.new(cw,cl,ch,car.space)
car.geom:body(car)
-- the visual object
car.l3d = l3dprimitive.newbox("box",cw,cl,ch)
car.l3d:linkinterface(car.actor)
car.l3d:rfLitSun(true)


function car:addwheel(x,y,z,rad,h)
	local wheel = dbody.new()
	wheel:pos(x,y,z)
	wheel.actor = actornode.new("wheel")
	wheel.actor:link(wheel)
	
	-- the collision object
	wheel.geom = dgeomsphere.new(rad,car.space)
	wheel.geom:fdirnormal(1,0,0)
	wheel.geom:surfaceid(1)
	wheel.geom:body(wheel)
	
	-- the wheel physics characteristics
	wheel:masscylinder(1,1, rad,h)
	wheel.hinge = djointhinge2.new()
	wheel.hinge:attach(self,wheel)
	wheel.hinge:anchor(x,y,z)
	wheel.hinge:axis1(0,0,1)
	wheel.hinge:axis2(1,0,0)
	wheel.hinge:suspensionerp(.5)
	wheel.hinge:suspensioncfm(15)
	
	-- the visual object
	wheel.l3d = l3dprimitive.newcylinder("cyl",rad,rad,h)
	wheel.l3d:linkinterface(wheel.actor)
	wheel.l3d:uselocal(true)
	wheel.l3d:localrotdeg(0,90,0)
	wheel.l3d:rfLitSun(true)
	wheel.l3d:color(1,1,0,0)
	return wheel
end

car.fl = car:addwheel(-1, 1,-1,.5,.25)
car.fr = car:addwheel( 1, 1,-1,.5,.25)
car.bl = car:addwheel(-1,-1,-1,.5,.25)
car.br = car:addwheel( 1,-1,-1,.5,.25)


-- Add some walls so we can collide with stuff and not fall into
-- infinite emptyness.
walls = {
  dgeomplane.new(1,0,0,-10, staticspace),
  dgeomplane.new(-1,0,0,-10, staticspace),
  dgeomplane.new(0,1,0,-10, staticspace),
  dgeomplane.new(0,-1,0,-10, staticspace),
  dgeomplane.new(0,0,1,-2, staticspace),--floorplane
}

-- sun to make scene more interesting
mysun = actornode.new("sun",100,200,600)
mysun.light = l3dlight.new("light")
mysun.light:linkinterface(mysun)
mysun.light:makesun()
mysun.light:ambient(.4,.4,.4,1)


-- a visual floor
floor = scenenode.new("floor",0,0,-2)
floor.l3d = l3dprimitive.newquadcentered("floor",20,20,1)
floor.l3d:linkinterface(floor)
floor.l3d:color(0,.5,0,1)
floor.l3d:rfLitSun(true)

-- a fixed camera looking to origin
cam = actornode.new("camera",6,-25,22)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,0,0,0,0,1)


function think ()
  local left = Keyboard.isKeyDown("LEFT")
  local right = Keyboard.isKeyDown("RIGHT")
  local up = Keyboard.isKeyDown("UP")
  local down = Keyboard.isKeyDown("DOWN")
  local fx = (left and 1 or 0) - (right and 1 or 0)
  local fy = (up and 1 or 0) - (down and 1 or 0)
  -- fx,fy: movement directions

-- this is new  
  -- let the joint steer to a given angle
  local function steerto (hinge,to)
    local a = hinge:angle1()
    local dif = to - a
    hinge:fmax(0.5) -- maximum applied steering force 
    local maxvel = math.max(-.05,math.min(.05,dif))
    -- limit the applied turnrate
    hinge:velocity(maxvel)
  end
  steerto(car.fl.hinge,-fx*.4) -- use left/right
  steerto(car.fr.hinge,-fx*.4) -- as steering
  steerto(car.bl.hinge,0) -- stabilize backwheels
  steerto(car.br.hinge,0) -- otherwise they will turn

  -- backwheel rotation
  car.bl.hinge:fmax2(.003)
  car.br.hinge:fmax2(.003)
  car.bl.hinge:velocity2(fy)
  car.br.hinge:velocity2(fy)
-- till here

  dworld.collidetest(globalspace)
  dworld.makecontacts()
  dworld.quickstep(1)
end

Timer.set("tutorial",think,20)