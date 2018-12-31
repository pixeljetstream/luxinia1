-- generate skyscene
-- data = {..
--	view
--	sun,sunact,cam,camact,l3dsky
--	scene,scenes3d
--	frame

dofile("../Shared/htmllabels.lua")

local data = {}
do

	-- setup l3dset
	l3dset.get(0):layer(0)
	
	-------
	-- view
	local view = UtilFunctions.simplerenderqueue()
	-- background does it for us
	view:filldrawlayers(false)
	view:fogstate(true)
	view:fogstart(10)
	view:fogend(100)
	-- disable, as background sky fills stuff
	view.rClear:flag(0,false)
	data.view = view
	
	-- reorder background drawing
	local bgview = l3dview.new()
	bgview:activate(l3dset.get(0))
	bgview:viewdepth(1,1)
	data.bgview = bgview

	------
	-- sun
	local sun = l3dlight.new("sun")
	local sunact = actornode.new("blah",true,-100,100,100)
	
	sun:linkinterface(sunact)
	
	sun:makesun()
	sun:diffuse(1,1,0.7)
	sun:ambient(0.1,0.1,0.2)
	
	sun:attenuateconst(1)
	sun:attenuatelinear(0.0003)
	sun:attenuatequadratic(0.00001)
	
	data.sun = sun
	data.sunact = sunact
	
	------
	-- camera
	local camact = actornode.new("camera")
	camact:rotdeg(30,0,-10)
	camact:pos(6,20,15)
	camact:rotdeg(2,0,-360*0.55)

	
	local cam = l3dcamera.default()
	cam:fov(90)
	cam:backplane(4096)
	cam:linkinterface(camact)

	data.cam = cam
	data.camact = camact
	
	------
	-- scene model
	local scenes3d = scenenode.new("scene",true)
	local mdl = model.load("scene.f3d",false,true)
	local scene = l3dmodel.new("scene",mdl)
	
	scene:linkinterface(scenes3d)
	scene:rfLitSun(true)
	scenes3d:localrotdeg(0,0,60)
	scene:rfFog(true)
	
	data.scene = scene
	data.scenes3d = scenes3d

	
	------
	-- sky material
		
	local nocloudcolor = false

	-- combiner setup
	if (technique.arb_vf():supported_tex4()) then
		resource.condition("TECH_TEXCOMB",true)
	elseif (technique.arb_texcomb():supported_tex4()) then
		if (capability.texcomb_combine4()) then
			resource.condition("TECH_TEXCOMB_TEX4_COMBINE4",true)
		else
			resource.condition("TECH_TEXCOMB",true)
		end
	elseif (technique.arb_texcomb():supported()) then
		resource.condition("TECH_TEXCOMB",true)
	else
		nocloudcolor = true
	end
	
	local mtl = material.load("sky.mtl")
	
	------
	-- sky model
	
	local mdl = model.load("skydome.f3d",false,false)
	
	-- we want the mesh to have vertex alphas depending on height 
	-- from ground and scale uvs based on height as well.
	-- The resusuerstring is used to mark the model being
	-- processed before, so we only do this once in a session.
	if (mdl:resuserstring() ~= "p") then
		print("PROC SKY start")
		local mid = mdl:meshid(0)
		local minx,miny,minz,maxx,maxy,maxz = mdl:bbox()
		local thresh = 0.99
		local divthresh = 1/(1-thresh)
		local divthresh2 = 1/thresh
		for i=0,mid:vertexCount()-1 do
			local x,y,z = mid:vertexPos(i)
			local w
			z = z/maxz
			if (z < thresh) then
				w = 0
			else
				w= (z-thresh)*(divthresh) * 3
			end
			mid:vertexColor(i,1,1,1,w)
		
			 x,y = mid:vertexTexcoord(i,1)
			 
			 w = 1 + (1-z)
			 
			 x = (x-0.5) * w + 0.5
			 y = (y-0.5) * w + 0.5
			 
			 mid:vertexTexcoord(i,x,y,1)
			
		end
		-- rebuild display list/vbo...
		mdl:setmeshtype(meshtype.auto())
		mdl:resuserstring("p")
		print("PROC SKY end")
	end
	
	
	local l3dsky = l3dmodel.new("sky",mdl)
	l3dsky:uselocal(true)
	l3dsky:localpos(0,0,-18)
	
	local drawl3d = rcmddrawl3d.new()
	drawl3d:node(l3dsky)
	drawl3d:forcedraw(true)
	bgview:rcmdadd(drawl3d)
	
	data.l3dsky = l3dsky
	
	-------------------------------------------------
	-- Parameter Handles

	
	local cloudctrl = mtl:getcontrol("cloudbrightness")
	local cloudbrightness
	if (cloudctrl) then
		cloudbrightness = function (val)
			mtl:moControl(cloudctrl,val)
		end
	end

	local ambient,suncolors = {1,1,1,1},{1,1,1,1}
	
	
	-- tracking of sun
	local sunautomat = matautocontrol.newmatposproj(sunact)
	sunautomat:vector(0.5)
	mtl:moAutotexstage(2,false,sunautomat)
	mtl:moAutotexstage(3,false,sunautomat)
	

	local sunctrl = mtl:getcontrol("sun")
	
	local function fogcolor(r,g,b,a)
		view:fogcolor(r or 0,g or 0,b or 0,a or 0)
	end
	local function fogstart(dist)
		view:fogstart(dist)
	end
	local function fogend(dist)
		view:fogend(dist)
	end
	
	local function updatelightning ()
		local rd,gd,bd = unpack(suncolors)
		mtl:moControl(sunctrl,rd,gd,bd)

		local r,g,b,a = unpack(ambient)
		fogcolor(r,g,b)
		sun:ambient(r*a*2,g*a*2,b*a*2,a)
		local sr,sg,sb,sa = sun:diffuse()
		local suninfluence = 
			math.max(0,math.min(10,select(3,sunact:pos())/5000))
		local amb = 1.5-suninfluence/3
		local dif = suninfluence*6
		scene:color(r*a*amb + sr*dif,
					g*a*amb + sg*dif,
					b*a*amb + sb*dif,
					1)
	end
	
	local cloudctrl = mtl:getcontrol("cloudclr")
	local function cloudcolor(r,g,b)
		mtl:moControl(cloudctrl,r,g,b)
	end
	
	local function suncolor(r,g,b,a)
		suncolors = {r,g,b}
		updatelightning()
	end

	
	local daytimec = mtl:getcontrol("daytime")
	local function daytime(val)
		mtl:moControl(daytimec,val)
	end
	
	local function sunsize(val)
		sunautomat:vector(val)
	end

	local function ambientcolor(r,g,b,a)
		ambient = {r,g,b,a}
		updatelightning()
	end
	
	local sunpolar= {1000,math.pi,0}
	local function sunpos (value)
		sunpolar[3] = (math.pi*0.6)-value
		sunact:pos(ExtMath.v3polar(unpack(sunpolar)))
		updatelightning()
	end
	local function sundir (value)
		sunpolar[2] = - value
		sunact:pos(ExtMath.v3polar(unpack(sunpolar)))
		updatelightning()
	end
	
	-------------------------------------------------
	-- Gui for Sky
	do
		
		local fw = 260
		frame = Container.getRootContainer():add(
				TitleFrame:new(0,0,260,32,nil,"Sky Settings"))
		frame:getSkin():setSkinColor(1,1,1,0.8)
		
		GH.sldLabelval = 32
		GH.sldLabelwidth = 100
		
		local y = 25
		local function addslider (name,fn,ini,scale,off)  
			y = GH.addslider (frame,10,y,fw-20,16,name,
				{callback = function(value) 
								fn(value) 
								return value 
							end,
				initvalue=ini,scale=scale,offset=off})
		end
		
		addslider("Daytime:",daytime,0.35)
		if (cloudbrightness) then
			addslider("Cloudbrightness:",cloudbrightness,0.6)
		end
		addslider("Sunsize:",sunsize,0.2,4,0.01)
		addslider("Sunheight:",sunpos,0.3,math.pi*0.6)
		addslider("Sundirection:",sundir,0.25,math.pi*2)
		addslider("Fog start:",fogstart,0.1,400)
		addslider("Fog end:",fogend,0.8,3000)
		
		y  = GH.addcolorbutton(frame,60,y,fw-100,16,"Suncolor",{
			initvalue={0,0,1,0.5},
			callback=function (obj,r,g,b,a) 
				suncolor(r,g,b,a)
				sun:diffuse(r*a*2,g*a*2,b*a*2)
			end,
			nomodal=true,noalpha=true,alphaname="Brightness",
			menuboundsfunc=function()
				local winw,winh = window.refsize()
				return {winw-fw-15-256,winh-256,256,256}
			end})
			
				
		if (not nocloudcolor) then
			y  = GH.addcolorbutton(frame,60,y,fw-100,16,
				"Cloudcolor",{
				initvalue={0,0,1,1},
				callback=function (obj,r,g,b,a) 
					cloudcolor(r,g,b)
				end,
				nomodal=true,noalpha=true,--alphaname="Brightness",
				menuboundsfunc=function()
					local winw,winh = window.refsize()
					return {winw-fw-15-256-128,winh-256,256,256}
				end})
		end
	
		y  = GH.addcolorbutton(frame,60,y,fw-100,16,
			"Ambientcolor / Fogcolor",{
			initvalue={0,0,1,0.2},
			callback=function (obj,r,g,b,a) 
				ambientcolor(r,g,b,a)
			end,
			nomodal=true,noalpha=true,alphaname="Brightness",
			menuboundsfunc=function()
				local winw,winh = window.refsize()
				return {winw-fw-15-256-256,winh-256,256,256}
			end})

		local ww,wh = window.refsize()
		local fh = y+5
		local fx,fy = ww-fw,wh-fh
		
		frame:setSize(fw,fh)
		frame:setLocation(fx,fy)
		
		-- some rudimentary layout management
		
		local layoutermax = GH.makeLayoutFunc(
			Container.getRootContainer():getBounds(),
			frame:getBounds(),
			{bottom=true, right=true,	
			relpos={false,false},	relscale={false,false}})
		local layoutermin = GH.makeLayoutFunc(
			Container.getRootContainer():getBounds(),
			{fx,wh-24,fw,24},
			{bottom=true, right=true,	
			relpos={false,false},	relscale={false,false}})
			
			
		local minbtn = GH.frameminmax(frame,layoutermin,layoutermax)
		
		function frame:onParentLayout(nx,ny,nw,nh)
			local layouter = 
				minbtn.oldcomponents and layoutermin or layoutermax
			self:setBounds(layouter(nx,ny,nw,nh))
		end
		
		data.frame = frame
	end
end

return data