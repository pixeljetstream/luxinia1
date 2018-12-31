-- Stencilshadows


-- we generate a l3dshadowmodel from a given l3dmodel or l3dprimtive
-- l3dmodels must always have closed geometry
function addshadow(sun,hostl3d,alwaysvisible)
	-- create the models in the second to last layer
	-- it is important that shadowmodels are drawn
	-- after regular geometry.
	local shadow = l3dshadowmodel.new("shadow",
		l3dset.get(0):layer(l3dlist.l3dlayercount()-2),sun,hostl3d)
	-- shadow might be nil, in case the hostl3d was not a closed
	-- mesh, or too many triangle/vertices
	-- Unclosed meshes can also happen when you used 
	-- "multi-materials" in 3dsmax export, or when
	-- different element's vertices overlap each other closely.
	
	if (not shadow) then return end
	
	-- and we link the model to the host
	shadow:parent(hostl3d)
	-- Be aware now that the shadowmodel is now only rendered when
	-- the host is visible. Which might not be correct in cases 
	-- where the shadow volume is very long.
	-- Therefore we can make sure the shadowmesh is alwas rendered
	-- with enabling "novistest".
	shadow:novistest(alwaysvisible)
	
	hostl3d.l3dshadow = l3dshadow
end

-- do scene setup here
-- do not use layer(15), which is last one, for your stuff however!
-- as we want to use that one for drawing the shadows
-- you can also use it draw geometry "on top" of shadow
function buildscene ()
	globalspace = dspacehash.new()
	quad = scenenode.new("floor")
	quad.l3d = l3dprimitive.newquadcentered("quad",15,15)
	quad.l3d:linkinterface(quad)
	quad.l3d:color(.2,.8,0,1)
	quad.l3d:rfLitSun(true)
	quad.geom = dgeomplane.new(0,0,1,0,globalspace)
	
	cam = actornode.new("cam",8*.6,8*.6,9*.6)
	cam.l3d = l3dcamera.default()
	cam.l3d:linkinterface(cam)
	cam:lookat(0,0,0, 0,0,1)
	
	sun = actornode.new("sun",-20,-10,25)
	sun.l3d = l3dlight.new("sun")
	sun.l3d:linkinterface(sun)
	sun.l3d:makesun()
	sun.l3d:ambient(.3,.3,.3,1)
	sun.l3d:diffuse(.7,.7,.7,1)
	
	objects = {}
	local function makebody(p)
		local ac = actornode.new("obj")
		ac.body = dbody.new()
		
		ac.body:autodisable(true)
		ac:link(ac.body)
		if p.w then
			ac.l3d = l3dprimitive.newbox("box",p.w,p.h,p.d)
			ac.geom = dgeombox.new(p.w,p.h,p.d,globalspace)
			ac.body:massbox(p.mass or 1, p.w, p.h, p.d,true)
		else
			ac.l3d = l3dprimitive.newsphere("sp",p.rad)
			ac.geom = dgeomsphere.new(p.rad,globalspace)
			ac.body:masssphere(p.mass or 1, p.rad,true)
		end
		ac.geom:maxcontacts(8)
		ac.geom:body(ac.body)
		ac.l3d:linkinterface(ac)
		ac.l3d:rfLitSun(true)
		local r,g,b = UtilFunctions.color3hsv(p.hue or 0,1,1)
		ac.l3d:color(r,g,b,1)
		ac.body:pos(p.x,p.y,p.z)
		ac.body:addforce(p.fx or 0, p.fy or 0, p.fz or 0)
		
		-- now lets add shadowmodel to the l3dprimitive 
		-- in our sample it's mostly okay to not set the
		-- always visible flag
		addshadow(sun.l3d,ac.l3d)
		
		objects[#objects+1] = ac
		if p.disabled then
			ac.body:state(false)
		end
	end
	
	makebody{rad=.4, x=0, y=4, z=.4, hue=0, fy=-6, mass=40}
	local s = .4
	for z=0.0,s*4,s do
		for x=-2+z*.5,2-z*.5,s+.1 do
			makebody{w=s,h=s,d=s, x=x,y=-1,z=z+s*.5, 
				hue= x/4+1, disabled=true}
		end
	end
	
	dworld.autodisablesteps(1)
	dworld.surfacebitsofterp(0,false)
	dworld.surfacebitsoftcfm(0,false)
	dworld.surfacemu(0,1.5)
	dworld.cfm(.001)
	dworld.erp(.6)
	dworld.iterations(16)
	dworld.contactsurfacelayer(.01)
	
	dworld.gravity(0,0,-.01)
	
	collectgarbage "collect"
end

buildscene()


Timer.set("tutorial",function ()
	if not globalspace then return end
	dworld.jointempty()
	dworld.collidetest(globalspace)
	dworld.makecontacts()
	dworld.quickstep(1)	
end,33)

Keyboard.setKeyBinding("r",buildscene)
Keyboard.setKeyBinding("s",
	function ()
		render.drawshadowvolumes(
			not render.drawshadowvolumes()
		)
	end
)

-- set all stencil values to 127
-- we use 127 because stencil shadows do + and - and
-- it could be that some systems do not support wrapping
-- (ie 0-1 becomes 255)
-- so we start with 127 and are fairly safe
-- note that most hardware will support wrapped operations

view = UtilFunctions.simplerenderqueue()
view.rClear:stencilvalue(127)

-- now we want to do regular drawing but apply the stencil
-- shadows at the end before last layers are drawn
--
-- the default renderqueue looks like this
-- rClear  (rcmdclear)
--     clears color,stencil and depth values for the viewport
-- rDrawbg (rcmddrawbg)
--     draws background stuff (skybox, images whatever)
-- rLayers[1 .. l3dlist.l3dlayercount()] for each l3dlayer
--  .stencil     (rcmdstencil)
--     sets up stencil functions, only l3dnodes with 
--     rfStenciltest(true) will be affected
--  .drawlayer   (rcmddrawlayer)
--     renders the corresponding l3dlayerid with material sorting 
--     enabled. [1] will render layer(0), 
--     as internal values start at 0
--  .drawprt     (rcmddrawprt)
--     renders the corresponding particle effects that are set to 
--     be drawn in this layer
--     by default all particles are rendered in the last layer.
--
-- we use the stencilsetup for the layer that already exists in the
-- renderqueue
local ffxstencil = view.rLayers[l3dlist.l3dlayercount()].stencil
ffxstencil:scEnabled(true)
-- The l3dshadowmodel works in a fashion that any region in the 
-- shadow will have a value that is not equal to the original 
-- value (127). So we setup stenciltest to draw only in the 
-- shadowed areas
ffxstencil:scFunction(comparemode.notequal(),comparemode.notequal(),
						127,255)
-- we dont actually want to change stencil values, while drawing 
-- so set everythin to keep
ffxstencil:scOperation(0,operationmode.keep(),
						operationmode.keep(),operationmode.keep())

-- now draw a fullscreen quad into the view, that will darken
-- all areas in the shadow

local fullscrn = rcmddrawmesh.new()
-- enable stenciltest, so we only draw in shadow pixels
fullscrn:rfStenciltest(true)

-- we want to darken the areas in the shadow
-- so we set blendmode to modulate
fullscrn:rfBlend(true)
fullscrn:rsBlendmode(blendmode.modulate())
-- we set the color to slightly less than white
-- blendmode.modulate will do viewpixel * quad's colorpixel
-- so 0.8 will mean 80% of original brightness inside the shadow
fullscrn:color(0.4,0.4,0.4,1)


-- and finally we add it after the stencil command
-- into the view's renderqueue
-- if we would not have passed ffxstencil, the quad would be drawn
-- at the end of the queue, however want to keep the possibility to
-- render stuff unshadowed
view:rcmdadd(fullscrn,ffxstencil)

-- we can use "local" for fullscrn here, because
-- rcmdadd prevents garbage collection of any active rendercommands



