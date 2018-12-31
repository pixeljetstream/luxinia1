-- Spriteanimation with matobject

-- <Resources>
-- textures/explosprite.png


-- initialization and light / camera setup
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.1,0.1,0.1,0)

----------------------------------
-- lets create a orthographic view
camact = actornode.new("cam")
cam = l3dcamera.default()
-- negative fov means orthographic and the amount is the 
-- "width" in units
cam:fov(-64)
cam:linkinterface(camact)
-- lets rotate the camera in such a way that
-- we look at the X,Y plane, -Z becomes depth
camact:rotdeg(-90,0,0)


----------------------------------
-- create the sprite object
-- load the texture
tex = texture.load("explosprite.png")

-- create actor
act = actornode.new("spr")
-- move it a bit to the left
act:pos(-5,0,-50)

spr = l3dprimitive.newquadcentered("spr",32,32)
spr:linkinterface(act)
spr:matsurface(tex)
-- lets blend the quad as decal (ie Alpha channel
-- becomes transparency)
spr:rsBlendmode(blendmode.decal())
spr:rfBlend(true)


----------------------
-- Sprite animation
function choosesprite(mo,n,tex,imgcntW,imgcntH)
	-- With the matobject interface we can access
	-- the texturematrix. We change that matrix in
	-- such a way that the texture coordinates from [0,1]
	-- are mapped to a rectangle within the [0,1] dimensions
	-- that reflect a rectangle of the texture.
	
	local w,h = tex:dimension()
	n = (n or 0)%(imgcntW*imgcntH)
	local x,y = n%imgcntW, math.floor(n/imgcntW)
	
	-- We must divide the coordinates by the image dimension
	-- to get into the [0,1] space.
	mo:moRotaxis(
	  1/imgcntW,0,0,
	  0,1/imgcntH,0,
	  0,0,1)
	  
	-- for the position offset we also have to 
	-- flip Y axis, because OpenGL defines 0,0 as bottom left
	-- And we assume our sprites to start at top left.
	mo:moPos(x/imgcntW,1-y/imgcntH-1/imgcntH,0)
end

local n = 0
function anim ()
	-- hand over 
	-- 1. our object (spr)
	-- 2. the subrectangle we want (starting top left)
	-- 3. the texture we use
	-- 4/5. the grid size of subrectangles stored
	
	choosesprite(spr,n,tex,4,4)
	print(n%16)
	n = n + 1
end

-- execute anim once, and then with every few milliseconds
anim()

-- lower the last value to speed up the playback
Timer.set("tutorial",anim,100)
