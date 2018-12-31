-- Physical surfaces

-- first setup
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1.0,1.0,1,0) 

	-- a sun
sun = actornode.new("sun",100,40,500)
sun.light = l3dlight.new("sun")
sun.light:linkinterface(sun)
sun.light:makesun()

	-- a camera
cam = actornode.new("cam",10,6,5)
cam.camera = l3dcamera.default()
cam.camera:linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

-- physic setup
globalspace = dspacehash.new() -- our space for everything

objects = {} -- list of objects


function makebody (p)
	-- our helper function takes a table as argument with
	-- all the information we need. Using a table makes it
	-- simpler to extend the arguments to pass and makes
	-- the calling code more readable.
	
	local actor = actornode.new("box") -- actor as base
	
		-- visible object
	if p.w then -- it's a box
		actor.l3d = l3dprimitive.newbox("box",p.w,p.h,p.d) 
	end
	if p.rad then -- it's a sphere
		actor.l3d = l3dprimitive.newsphere("box",p.rad) 
	end
	actor.l3d:linkinterface(actor)
	actor.l3d:rfLitSun(true)
	local r,g,b = UtilFunctions.color3hsv(p.hue,1,1)
	actor.l3d:color(r,g,b,1) -- hue is easier to set
	
		-- physical geometry
	if p.w then -- box geom
		actor.geom = dgeombox.new(p.w,p.h,p.d,globalspace)
	end 
	if p.rad then -- it's a sphere geom
		actor.geom = dgeomsphere.new(p.rad,globalspace)
	end
	actor.geom:surfaceid(p.surfaceid or 0)
	if p.fdir then -- it has a friction direction vector
		actor.geom:fdirnormal(unpack(p.fdir))
	end
	
		-- our body now
	actor.body = dbody.new()
	if p.w then -- mass for box body
		actor.body:massbox(p.mass or 1, p.w,p.h,p.d,true)
	end
	if p.rad then -- mass for sphere body
		actor.body:masssphere(p.mass or 1, p.rad,true)
	end
	actor:link(actor.body) -- link it with the actor
	actor.geom:body(actor.body) -- link it with the geom
	
		-- position and force setup
	actor.body:pos(p.x,p.y,p.z)
	actor.body:addforce(p.vx or 0,p.vy or 0,p.vz or 0)
	
		-- prevent Gargabe Collection
	objects[#objects+1] = actor
end

-- create 6 boxes with different surfaceids
makebody{x=-2,y=0,z=0, w=.5,h=.5,d=.5, hue=.0, surfaceid=1}
makebody{x=-1,y=0,z=0, w=.5,h=.5,d=.5, hue=.2, surfaceid=2}
makebody{x= 0,y=0,z=0, w=.5,h=.5,d=.5, hue=.4, surfaceid=3}
makebody{x= 1,y=0,z=0, w=.5,h=.5,d=.5, hue=.6, surfaceid=4}
makebody{x= 2,y=0,z=0, w=.5,h=.5,d=.5, hue=.8, surfaceid=5, 
	fdir={0,1,0}} -- the violet box shall have an fdirnormal

-- create 6 spheres that push the objects
makebody{x=-2.3,y=4,z=0, rad=.25, hue=.5, surfaceid = 6, vy=-.05}
makebody{x=-1.3,y=4,z=0, rad=.25, hue=.5, surfaceid = 6, vy=-.05}
makebody{x=-0.3,y=4,z=0, rad=.25, hue=.5, surfaceid = 6, vy=-.05}
makebody{x= 0.7,y=4,z=0, rad=.25, hue=.5, surfaceid = 6, vy=-.05}
makebody{x= 1.7,y=4,z=0, rad=.25, hue=.5, surfaceid = 6, vy=-.05}

-- a simplified way how to set the surfaceproperties
function setsurfaceparam(p)
	-- p.combos: with which surfaceids does it map to this material?
	for i,with in ipairs(p.combos) do
		-- so if p.surfaceid collides with our surfaceid, 
		-- let's map it to p.matid
		dworld.surfacecombination(p.surfaceid,with,p.matid)
	end
	 
	-- we don't want to use these features now, but 
	-- they are enabled per default:
	dworld.surfacebitsoftcfm(p.matid,false)
	dworld.surfacebitsofterp(p.matid,false)
	
	-- le't set the friction values (mu)
	dworld.surfacemu(p.matid,p.mu or 1000)
	-- mu2 makes only sense in combination with the fdir
	dworld.surfacemu2(p.matid,p.mu2 or 1000)
	-- mu2 must be enabled
	dworld.surfacebitmu2(p.matid,p.mu2 and true or false)
	
	
	-- this defines how to use the fdirnormal, we set it
	-- that way that it uses the fdirnormal of our id
	dworld.surfacebitfdir(p.matid,p.fdir and true or false)
	dworld.surfacefdirmixmode(p.matid,4)
	dworld.surfacefdirid(p.matid,p.surfaceid)
end

-- let's set the material properties
setsurfaceparam{surfaceid=1,matid=1, combos={6,0}, mu=0}
setsurfaceparam{surfaceid=2,matid=2, combos={6,0}, mu=.05}
setsurfaceparam{surfaceid=3,matid=3, combos={6,0}, mu=.1}
setsurfaceparam{surfaceid=5,matid=5, combos={6,0}, mu=0.001, 
	mu2=10.5, fdir=true}

-- a floorplane so our object's don't fall without resistance
floorplane = dgeomplane.new(0,0,1,-2,globalspace)

-- set a gravity
dworld.gravity(0,0,-.01)


-- timer setup
function think ()
	while not Keyboard.isKeyDown("space") do coroutine.yield() end
	while true do
		dworld.collidetest(globalspace)
		dworld.makecontacts()
		dworld.quickstep(1)
		coroutine.yield()
	end
end

Timer.set("tutorial",think,20)

