-- User textures
-- shows how to create user created textures

-- basic scene setup
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1.0,1.0,1.0,1) 

cam = actornode.new("camera",8,5,6)
l3dcamera.default():linkinterface(cam)
cam:lookat(0,0,0, 0,0,1)

cube = actornode.new("actor")
cube.l3d = l3dprimitive.newbox("cube",2,2,2)
cube.l3d:linkinterface(cube)

-- create the texture now
tw,th = 32,32 -- width and height values
 -- should be power of 2, otherwise it's becoming tricky

usertex = texture.create2d("usertex",tw,th,textype.rgb(),
  true,false,false) -- let's create an rgb texture with,
   -- we are keeping the data, we don't want mipmaping nor 
   -- texture filtering
usertex:clamp(true) -- switching on clamping since texture
 -- is not repeating on any axis. Because of the symmetry
 -- of our chosen pattern, we wouldn't see much difference
 -- here if we used no clamping, however if other patterns
 -- are used, borders can become visible

cube.l3d:matsurface(usertex) -- assign our texture to our cube

Timer.set("tutorial",
  function ()
    -- we create growing rings, thus we need a phase
    local phase = os.clock()*2
    -- loop over all pixels (yes, that is very expensive)
    for x=0,tw-1 do
      for y=0,th-1 do
        -- distance of the pixel to the center
        local dx,dy = x-tw*.5-.5,y-th*.5-.5
        -- distance length (using again a^2+b^2 = c^2)
        local d = math.sqrt(dx*dx+dy*dy)
        -- calculate an r/g/b value, the blue value
        -- is now a sinus signal - we square the value 
        -- so there won't be negative values
        local r,g,b = 0,0,math.sin(phase-d*.1)^2
        -- assign the rgb value to the pixel
        usertex:pixel(x,y,r,g,b,1)
      end
    end    
    -- upload the data
    usertex:uploaddata()
  end
,20)
