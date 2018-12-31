-- Render2Texture FBO Glow, Multiple Render Targets

-- <Resources>
-- + T023
-- fxs/postfx/pfx_add2.mtl
-- fxs/postfx/pfx_add2.shd

-- In this tutorial we create glow and simple postfx. The glow will be visible 
-- around specular highlights and emissive texture details. We will make use
-- of the rendercommand (rcmd) mechanism for l3dviews

-- The tutorial needs FrameBufferObject (rcmdfbo) extension support as it allows us
-- to directly render to textures. Normally we render into the window's
-- backbuffer, but with fbo we can store results of rendering elsewhere.

-- Another way to get rendered images into a texture, is copying the backbuffer
-- into a texture using "rcmdcopytex". That method isnt as fast as fbo, hence
-- fbo is preferred. However fbo at the moment wont allow us to do anti-aliasing.
-- So in case you need anti-alias you should copy the backbuffer into a texture.
-- Also when no fbo support is given, you might use the copy mechanism. The
-- "normalmapping" demo contains code for similar glow effect like shown here,
-- which also supports non-fbo hardware.

-- check if hardware is sufficient
if (not ( 	capability.fbo() and  
			capability.texrectangle() and
			 technique.arb_vf():supported_tex4())
	) then
	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality."))

	return (function() lbl:delete() end)
end

-- depending on hardware capability we need several codepaths
-- check if there is support for multiple render targets, which means 
-- we can write to 2 or more colorbuffers at the same time
local hasmrt = capability.drawbuffers() > 1


--d-- only for debug purposes
--d-- hasmrt = false 		-- enforce mrt off 
--d-- blurtest2d = true 	-- use 2D textures instead of RECT for glow
--d-- rendersystem.cglowprofileglsl(true)
--d-- rendersystem.cgdumpbroken(true)
--d-- rendersystem.cgdumpcompiled(true)
--d-- rendersystem.cgprefercompiled(true)

-----------------
-- Texture Setup
-- We will need several textures to store our render outputs.
-- At the end we will use them to draw the final image into the backbuffer.

-- create fullscreen textures for color
texfmain = texture.createfullwindow("mainbuffer",textype.rgba())

-- if we have MRT support we also render glowsources at full res
texfglow= hasmrt and texture.createfullwindow("glowbuffer",textype.rgba())
if (texfglow) then
	texfglow:filter(true)
end


-- Create two slightly resized version for blurring, or when no MRT support 
-- exists. Because we blur anyway, its not needed to blur on full resolution.
-- That way we can speed up the process as less pixels need to be computed. 
local ww,wh = window.size()
local wsmallw,wsmallh = math.floor(ww/3),math.floor(wh/3)

--d-- debug case
if (blurtest2d) then
	wsmallw,wsmallh = 256,256
	texglow0 = texture.create2d("glow0",wsmallw,wsmallh,textype.rgba())
	texglow1 = texture.create2d("glow1",wsmallw,wsmallh,textype.rgba())
else
--	normally we use rectangle textures
	texglow0 = texture.create2drect("glow0",wsmallw,wsmallh,textype.rgba())
	texglow1 = texture.create2drect("glow1",wsmallw,wsmallh,textype.rgba())
end


texglow0:filter(true)
texglow1:filter(true)
texglow0:clamp(true)
texglow1:clamp(true)


-- create a renderbuffer for depth
-- renderbuffer wont allow us to be used as texture, but when we dont need
-- that functionality they are the preferred storage
rbdepth = renderbuffer.new()
rbdepth:setup(textype.depth(),ww,wh)

if (not hasmrt) then
	rbdepthsmall = renderbuffer.new()
	rbdepthsmall:setup(textype.depth(),wsmallw,wsmallh)
end


--------------------
-- FBO Setup Main
--
-- The general order of rcmds when using fbo is:
--  1. rcmdfbo: make sure it has proper size
--  2. rcmdfbotex: assigns textures for render storage
--  3. rcmdfborb: assigns renderbuffers for render storage
--  4. rcmdfbodrawto: which color assigns are activated for render output
--  5. do your drawing commands
--  6. rcmdfbooff: deactivates fbo


-- first we need a l3dview in which our rendering takes place
view = l3dset.get(0):getdefaultview()

-- if we have MRT support, we can render both color and overbrightness at 
-- the same time, otherwise we need a dedicated pass

-- We will create several rcmds for conveniance we just use the same variable.
-- The rcmds are automatically protected from garbage collection, as long
-- as they are used in a l3dview.

local rc
-- create fbo with proper size
local fbo = renderfbo.new(ww,wh)
rc = rcmdfbobind.new()
rc:setup(fbo)
-- and insert the command before clearing
view:rcmdadd(rc)

-- now the fbo is active, but we need to attach the
-- textures/renderbuffers which serve as drawing
-- destination
rc = rcmdfbotex.new()
rc:color(0,texfmain)

-- bind both textures for MRT
if (hasmrt) then
	rc:color(1,texfglow)
end
view:rcmdadd(rc)

-- bind renderbuffer for depth
rc = rcmdfborb.new()
rc:depth(rbdepth)
view:rcmdadd(rc)

-- we setup assignments, now we activate the color drawbuffer
rc = rcmdfbodrawto.new()
if (hasmrt) then
	rc:setup(0,1)	
	-- render into colorassign 0 & 1 
	-- the fragment shader, takes care of what is rendered in each buffer
else
	rc:setup(0)
	-- just render into colorassign 0
end
view:rcmdadd(rc)

-- by default the simplerenderqueue would remove all rcmds
-- but the second arg tells it to keep cmds, and just
-- add the new ones afterwards
view = UtilFunctions.simplerenderqueue(view,true)
view.rClear:colorvalue(0.0,0.0,0.0,0)


--------------------
-- L3DView Glow

-- setup a dedicated (smaller) l3dview for the glow processing
viewglow = l3dview.new()
viewglow:windowsized(false)
viewglow:viewsize(wsmallw,wsmallh)

-- activate it after the mainview
viewglow:activate(l3dset.get(0),view,true)

-- setup fbo
local fbo = renderfbo.new(wsmallw,wsmallh)
rc = rcmdfbobind.new()
rc:setup(fbo)
viewglow:rcmdadd(rc)

rc = rcmdfbotex.new()
rc:color(0,texglow0)
viewglow:rcmdadd(rc)

if (hasmrt) then
	-- when we have mrt support we also bind texglow1, because changing
	-- between rendertargets is faster with "rcmdfbodrawto" than with
	-- "rcmdfbotex"
	rc:color(1,texglow1)
	
	
	-- we just render the fullsized image into this framebuffer, and therefore
	-- perform a downscaling
	rc = rcmddrawmesh.new()
	rc:matsurface(texfglow)
	-- dont forget that rectangle textures need special texturematrix
	rc:moTexmatrix(window.texmatrix())
	viewglow:rcmdadd(rc)
else
	-- we could not write "glow" sources with the main pass, so we render it
	-- into the resized glow with a dedicated pass. We need a depthbuffer as well
	
	rc = rcmdfborb.new()
	rc:depth(rbdepthsmall)
	viewglow:rcmdadd(rc)
	
	-- our material contains 2 shader stages, one for the "main pass" and one
	-- for "glowsources". To use the second stage we will need "rcmdshaders"
	
	rc = rcmdshaders.new()
	-- With this command we can override "regular" non-material rendering
	-- to make use of shaders, or like mentioned to set active shaderstage
	-- of a material.
	-- we need to pass 4 shaders which are used for
	-- "color only", "color and lightmap", "1 texture", "1 texture and lightmap"
	-- because in our scene every mesh has a material assignment, we can just
	-- use any shader here, as it won't be used.
	local someshader = shader.load("3tex_diffspecillum.shd")
	rc:shaders(someshader,someshader,someshader,someshader)
	-- but what we actually wanted is raising the shaderstage to 1
	rc:shaderstage(1)
	viewglow:rcmdadd(rc)
	
	-- now render stuff as usual
	viewglow = UtilFunctions.simplerenderqueue(viewglow,true)
	viewglow.rClear:colorvalue(0.0,0.0,0.0,0)
	
	-- Every "l3dview" fills the draw layers, and checks if stuff is visible to
	-- its camera. As the camera and what we render is identical to the main
	-- scene, we can skip this costly operation.
	viewglow:filldrawlayers(false)
end


--------------------
-- FBO Setup Blur
--
-- Now texglow0 contains the unblurred glow sources
-- Lets do a bit blurring now, blur is basically averaging colors
-- around the processed pixel. 
-- We blur using two passes one horizontal and one vertical pass.
-- Reason is that our blur kernel would be too big for a single
-- pass, but the blur process is "separable", ie we can split
-- the operation in the mentioned "two directions". 

-- We setup the pixeloffsets and weights
-- make sure the weights sum to 1 else we would change brightness
-- lets write a function for that as it may be useful in other
-- projects. And for colors we want a vector4 tab for each weight.

function getnormalizedVEC4(tab)
	local sum = 0
	for i,v in ipairs(tab) do
		sum = sum + v
	end
	local outtab = {}
	for i,v in ipairs(tab) do
		outtab[i] = {v/sum,v/sum,v/sum,v/sum}
	end
	
	return outtab
end


-- also a function for getting 2D pixeloffsets from 1D
function getoffsets2D(tab,vertical)
	local newcoords = {}
	for i,v in ipairs(tab) do
		-- create a 2d tab for each pixel offset
		-- set x or y to 0 depending whether we are in vertical 
		-- or horizontal mode
		newcoords[i] = {vertical and 0 or v,vertical and v or 0}
	end
	
	return newcoords
end


-- Setup the offsets & weights for later use.
-- Experiment with different values and see the result.
-- The more values (maximum 9) the slower but wider the blur
-- kernel. You can also widen it by rasing lookup steps,
-- but you will get stepping artifacts.

-- local offsets = {-6,-4,-2,0,2,4,6} --wider but with artifacts
local offsets = {-3,-2,-1,0,1,2,3}



local weights = {1,1,1,1,1,1,1}

weights = getnormalizedVEC4(weights)

local offsetshoriz = getoffsets2D(offsets)
local offsetsvert =  getoffsets2D(offsets,true)


-- As we can never read and write to the same texture at the same
-- time. So if you want to change a texture you either "ping-pong"
-- render into a temp, then back to original texture. Or you
-- directly output to a new texture that you later use.  

for i=1,1 do
	-- horizontal pass
	-- render first pass into texglow1, reading from texglow0
	if (hasmrt) then
		rc = rcmdfbodrawto.new()
		rc:setup(1)
		viewglow:rcmdadd(rc)
	else
		rc = rcmdfbotex.new()
		rc:color(0,texglow1)
		viewglow:rcmdadd(rc)
	end
	
	rc = rcmddrawmesh.new()
	viewglow:rcmdadd(rc)
	
	-- set matobject & matsurface controllers
	-- we use a new helper function introduced in 0.98
	FXUtils.applyMaterialTexFilter(rc, texglow0, offsetshoriz, weights)
	
	-- vertical pass
	-- render into texglow0 reading from texglow1
	if (hasmrt) then
		rc = rcmdfbodrawto.new()
		rc:setup(0)
		viewglow:rcmdadd(rc)
	else
		rc = rcmdfbotex.new()
		rc:color(0,texglow0)
		viewglow:rcmdadd(rc)
	end

	rc = rcmddrawmesh.new()
	viewglow:rcmdadd(rc)
	
	-- set matobject...
	FXUtils.applyMaterialTexFilter(rc, texglow1, offsetsvert, weights)

end

-- we are done with fbo rendering
viewglow:rcmdadd(rcmdfbooff.new())

--------------------
-- Final output into backbuffer

viewout = l3dview.new()
viewout:windowsized(true)

-- activate it after the glow/blur stuff
viewout:activate(l3dset.get(0),viewglow,true)

-- we draw the result as fullscreen quad 
rcfinal = rcmddrawmesh.new()
viewout:rcmdadd(rcfinal)

-- a function to handle the final composition effect
function pfx_add(rcmesh,mscale,gscale)
	local mtlout = material.load("fxs/postfx/pfx_add2.mtl")
	local tex0id = mtlout:gettexcontrol("source0")
	local tex1id = mtlout:gettexcontrol("source1")
	local wtsid  = mtlout:getcontrol("weights")
	
	-- assign the material and setup controller values
	rcmesh:matsurface(mtlout)
	
	rcmesh:moTexcontrol(tex0id,texfmain)
	rcmesh:moTexcontrol(tex1id,texglow0)
	
	rcmesh:moControl(wtsid,0,mscale,mscale,mscale,mscale)	-- mainpass scale
	rcmesh:moControl(wtsid,1,gscale,gscale,gscale,gscale)  -- glow scale
end

function pfx_debug(rcmesh,tex)
	rcmesh:color(1,1,1,1)
	rcmesh:matsurface(tex)
	
	local w,h = unpack(tex:formattype() == "rect" and 
									{tex:dimension()}
							or {1,1})
	rcmesh:moRotaxis(w,0,0,	0, h,0,	0,0,1)
end

--d-- debug tex2d output
if (blurtest2d) then
	pfx_debug(rcfinal,texglow0)
else
	-- assign the effect
	pfx_add(rcfinal,1,1)
end


-- a final error check
fboerrtype,fboerrmsg = l3dlist.fbotest()

if (fboerrtype) then
	-- we will always get a warning message,
	-- about leaving the fbobind active
	-- for the first l3dview
	print(fboerrmsg)
	if (fboerrtype > 0) then
		-- revert
		view = UtilFunctions.simplerenderqueue(view)
		viewglow:delete()
		viewout:delete()
	end
end

-------------------
-- basic scene, like in the material tutorial

-- a slight change now however, this time
-- it is important for the material to know
-- if there is split or MRT rendering

local mtl
if (hasmrt) then
	-- tell Cg compilers to use GLOWSRCMRT codepath
	mtl = material.load("t33.mtl","-DGLOWSRCMRT;")
else
	-- create a resource condition
	resource.condition("USE_SPLITGLOWSRC",true)
	-- this condition will activate the codepath inside the material
	-- which will create 2 shaderstages. One for main pass
	-- and another for the glowsrc path
	mtl = material.load("t33.mtl")
end


local specctrlid = mtl:getcontrol("specularctrl")
-- when split rendering is used, we need two controls
local specctrlsplitid = mtl:getcontrol("specularctrlSplit")

local t33 = model.load("t33.f3d")

function makespaceship(pos,specctrl)
	local actor = actornode.new("t33",0,0,0)
	
	actor.l3d = l3dmodel.new("t33",t33)
	actor.l3d:linkinterface(actor)
	actor.spos = pos
	
	for i=1,t33:meshcount() do
		local mid = t33:meshid(i-1)	-- meshid indices start at 0
		actor.l3d:matsurface(mid,mtl)
		actor.l3d:moControl(mid,specctrlid,unpack(specctrl))
		-- again this time we might have 2 ctrls
		if (specctrlsplitid) then
			actor.l3d:moControl(mid,specctrlsplitid,unpack(specctrl))
		end
	end
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
sun.light:ambient(.18,.18,.18,1)
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