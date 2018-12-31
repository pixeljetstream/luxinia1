-- Material Instances, matobjects

-- <Resources>
-- + T022
-- materials/t33.mtl

-------------------
-- basic scene
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

-- load the resources we need
local t33 = model.load("t33.f3d")
local mtl = material.load("t33.mtl")

-- the material will contain controllers to allow each instance of a surface,
-- that uses this material having its own unique parameters.
-- all these unique values can be accessed with the matobject interface.
-- now we first retrieve the matcontrolid we need later.
local specctrlid = mtl:getcontrol("specularctrl")

-- as we want more than one ship lets use a function
function makespaceship(pos,specctrl)
	local actor = actornode.new("t33",0,0,0)
	
	actor.l3d = l3dmodel.new("t33",t33)
	actor.l3d:linkinterface(actor)
	actor.spos = pos
	
	-- for all meshids 
	for i=1,t33:meshcount() do
		-- create a meshid 
		local mid = t33:meshid(i-1)	-- meshid indices start at 0
		-- assign material to l3dmodel's mesh
		actor.l3d:matsurface(mid,mtl)
		
		-- give each spaceship a unique specctrl value using the
		-- matobject interface command
		-- we also need to hand over the meshid as well
		actor.l3d:moControl(mid,specctrlid,unpack(specctrl))
	end
	-- always set Renderflags AFTER material assignments
	actor.l3d:rfLitSun(true)
	
	return actor
end

-- lets create some spaceships
spaceships = {}
table.insert(spaceships,makespaceship({-3,15,0},{1,1,1,8}))

-- more intense reddish specular and sharper 
table.insert(spaceships,makespaceship({3,15,0},{1.4,0.7,0.7,32}))


sun = actornode.new("sun",100,-100,500)
sun.light = l3dlight.new("sun")
sun.light:ambient(.2,.2,.2,1)
sun.light:linkinterface(sun)
sun.light:makesun()


-- rotation of object
-- A simple user input system for rotation
local cx,cy = 0,0
local pdx,pdy,pdz = 0,0,0
function rotit ()
	local x,y = input.mousepos() -- mouse
	x,y = -y,-x
	
	local l,r = input.ismousedown(0),input.ismousedown(1)
	if not l and not r then -- if no mousebutton is pressed...
		cx,cy = x,y -- set reference coordinate to the mouse position
	end
	local dx,dy,dz = l and (cx-x) or 0, cy-y,r and (cx-x) or 0
	  -- dx,dy,dz is now a direction vector
	dx,dy,dz = dx*.01, dy*.01, dz*.01 -- smaller numbers now
	pdx,pdy,pdz = pdx*.9+dx*.1, pdy*.9+dy*.1, pdz*.9+dz*.1
	dx,dy,dz = pdx,pdy,pdz -- dampen the vector with previous one
	
	for i,actor in ipairs(spaceships) do
		actor.rx,actor.ry,actor.rz = -- set rotation info for the actor
			(actor.rx or 0) + dx, (actor.ry or 0)+dy, (actor.rz or 0)+dz
		actor:rotdeg(actor.rx,actor.ry,actor.rz)
		actor:pos(unpack(actor.spos)) -- position/rotation update
	end
end

Timer.set("tutorial",rotit,20)
-- rotit is now called every 20 milliseconds