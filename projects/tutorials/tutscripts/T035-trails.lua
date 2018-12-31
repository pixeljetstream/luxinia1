-- Trails

-- In this tutorial we will make use of the trail
-- system luxinia provides. Trails are useful for drawing
-- thick lines, and have automatic trails, e.g. for 
-- rockets. They generate a triangle mesh strip,
-- so that for every trailpoint two meshpoints
-- are created. One can influence the width,
-- color and texture coordinates of the mesh.
--
-- You can either spawn the points manually
-- or automatically. This time we use
-- auomatic spawning.

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

-- Here we create an automatic trail
-- with self-spawning points and trail mesh
-- always facing camera.
actorauto = actornode.new("trailauto",0,230,0)

lsphere = l3dprimitive.newsphere("trailauto",1)
lsphere:linkinterface(actorauto)

-- The lenght parameter for creation means 
-- how many trailpoints we need as maximum.
-- With "drawlength" you can dynamically
-- modify the used size laterwards.
-- The last spawned point is always the first
-- in the mesh.
-- All points are stored in a cyclic array,
-- which means if you spawn more than the
-- amount of points the trail has, the
-- last point is overwritten.


trailauto = l3dtrail.new("trailauto",32)
trailauto:linkinterface(actorauto)
-- set to additive blending and
-- let trails always face camera
-- (good for 3d points)
trailauto:rfBlend(true)
trailauto:rsBlendmode(blendmode.amodadd())
trailauto:facecamera(true)

-- Points are in worldspace, so they directly
-- get position of the actornode.
trailauto:typeworld()
-- Set the trailtype to automatic spawning
-- spawn at 40 fps
trailauto:spawnstep(25)
-- we want manualy control over time 
trailauto:uselocaltime(true)
trailauto:localtime(system.time())


-- Let's colorize the trail to fade out at the end.
-- We could either set the colors of all 32 points,
-- in the outmesh individually (left/right vertices 
-- share color), or use some built-in linear interpotar
-- doing the work for us.
-- 
-- bright yellow at beginning, then fade out.
trailauto:color(0,1,1,0.8,1)
trailauto:color(1,1,0.5,0.3,0.8)
trailauto:color(2,1,0.3,0.3,0)

-- Same can be done for size, start small
-- the grow.
trailauto:size(0,1)
trailauto:size(1,1)
trailauto:size(2,2)

-- a few variables for our auomatic movement
actorauto.ctr = {actorauto:pos()}
actorauto.rad = 75
actorauto.dir = {0,-1,0}
actorauto.dirlast = {0,0,0}

actorauto.dirspeed = 100
actorauto.speed = 0.2
actorauto.dirangle = 60

actorauto.firetime = system.time() + actorauto.dirspeed

-- Passing time ourselves, allows "bulle-time" like
-- effects
local function updateautotrail(time)
	-- random motion within sphere
	-- for actorauto
	
	local lasttime = trailauto:localtime()
	local timediff = time-lasttime
	trailauto:localtime(time)
	
	local lerp = 1-math.min(1,(math.max(time,actorauto.firetime)-time)/actorauto.dirspeed)
	
	local pos = {actorauto:pos()}
	local dir = ExtMath.v3normalize(
					ExtMath.v3interpolate(lerp,actorauto.dirlast,actorauto.dir))
	local newpos = ExtMath.v3scaledadd(pos,dir,timediff*actorauto.speed)
	actorauto:pos(unpack(newpos))
	
	if (time > actorauto.firetime) then
		actorauto.firetime = time + actorauto.dirspeed
		actorauto.dirlast = {unpack(actorauto.dir)}
		if (ExtMath.v3dist(newpos,actorauto.ctr) > actorauto.rad) then
			-- suck to center or camera
			local ctr = (math.random() > 0.5) and actorauto.ctr or {0,0,0}
			actorauto.dir = ExtMath.v3normalize(ExtMath.v3sub(ctr,newpos))
		else
			local rotmat = matrix4x4.new()
			local angle = actorauto.dirangle
			local hangle = angle/2
			rotmat:rotdeg(hangle-math.random(angle),hangle-math.random(angle),hangle-math.random(angle))
			actorauto.dir = {rotmat:transform(unpack(actorauto.dir))}
		end
	end
end


Timer.set("tutorial",function ()
	do
		local time = system.time()
		updateautotrail(time)
	end
end)

