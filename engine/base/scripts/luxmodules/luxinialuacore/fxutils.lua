FXUtils = {}
LuxModule.registerclass("luxinialuacore","FXUtils",
		"The FXUtils class is a collection of lua functions that aid setting up effects.",FXUtils,{})
		
LuxModule.registerclassfunction("applyMaterialTexFilter",
	[[(matcontrolid texoffsets, weights, clrbias):(matobject, texture, table texoffsets, table weights, [table colorbias], [Rectangle visible]) - adds a texture filter material to an object that supports
	matobject and matsurface assignments. Texoffsets and weights should match in length and may not exceed 9. Both contain tables of sizes
	up to 4. Texoffsets should be passed in "pixelsize" ie {-1,-1} would be lower left neighboring pixel. Weights reflect
	weighting for each color channel. The resulting pixel will be the weighted sum of all contributing pixels. Rectangle or powerof2 textures are
	correctly dealt with and approrpriate conversions of texture coordinates are done.<br>
	Colorbias is added at the end and be default a table of {0,0,0,0}. 
	
	Returns matcontrolids for changing control values later, if needed. 
	]])
function FXUtils.applyMaterialTexFilter(mo, tex, offsets, weights, clrbias, viewrect)
	-- 
	assert(technique.arb_vf():supported_tex4(),"Technique not supported")
	
	local isrect = tex:formattype() == "rect"
	local texw,texh = tex:dimension()
	
	local cnt = math.min(9,math.min(#offsets,#weights))
	
	local mtl = material.load("texfilter"..cnt..".mtl", isrect and "-DUSETEXRECT;")
	
	local mt  = mtl:gettexcontrol("source")
	local mcw = mtl:getcontrol("weights")
	local mcoff = mtl:getcontrol("coords")
	local mcb = mtl:getcontrol("clradd")
	local mcs = (isrect or viewrect) and mtl:getcontrol("texscale")
	local mcm = viewrect and mtl:getcontrol("texmove")
	
	local sx,sy = unpack(isrect and {1,1} or {1/texw,1/texh})
	
	mo:matsurface(mtl)
	mo:moTexcontrol(mt,tex)
	if (cnt > 1) then
		for i=1,cnt do
			local x,y,z,w = unpack(offsets[i])
			mo:moControl(mcoff,i-1,x*sx,y*sy,z,w)
			mo:moControl(mcw,i-1,unpack(weights[i]))
		end
	else
		local x,y,z,w = unpack(offsets[1])
		mo:moControl(mcoff,x*sx,y*sy,z,w)
		mo:moControl(mcw,unpack(weights[1]))
	end
	
	if (viewrect) then
		if (isrect) then
			mo:moControl(mcm,viewrect[1],viewrect[2],0,0)
			mo:moControl(mcs,viewrect[3],viewrect[4],0,0)
		else
			mo:moControl(mcm,viewrect[1]*sx,viewrect[2]*sy,0,0)
			mo:moControl(mcs,viewrect[3]*sx,viewrect[4]*sy,0,0)
		end
	elseif (isrect) then
		mo:moControl(mcs,texw,texh,0,0)
	end
	
	mo:moControl(mcb,unpack(clrbias or {0,0,0,0}))
	
	return mcoff,mcw,mcb
end

LuxModule.registerclassfunction("getCubeMapCameras",
	[[(table cameras):(int visflagstart, [spatialnode], [float frontplane], [float backplane]) - returns table with 6 cameras. 
	Each setup for dynamic cubemap rendering. Order is CubeMap sides 0-5. Local matrices are set and rotation inherition is
	disabled.
	]])
function FXUtils.getCubeMapCameras(visflagstart,spnode,frontplane,backplane)
	local cams = {}
	local outmat = matrix4x4.new()
	local rotmat = matrix4x4.new()
		
	local function addcamera(name,id,x,y,z)
		local cam = l3dcamera.new(name,id)
		if (spnode) then cam:linkinterface(spnode) end
		
		cam:aspect(1)
		cam:fov(90)
		cam:uselocal(true)
		cam:rotationlock(3,true)
		
		outmat:identity()
		rotmat:identity()
		
		rotmat:rotdeg(x,y,z)
		-- -90 as skyrotmatrix does +90
		outmat:rotdeg(-90,0,0)
		outmat:mul(rotmat)
		
		cam:localmatrix(outmat)
		
		cam:backplane(backplane)
		cam:frontplane(frontplane)
		
		table.insert(cams,cam)
	end

	addcamera("FXCam+x",visflagstart,	0,90,-90)
	addcamera("FXCam-x",visflagstart+1,	0,-90,90)
	addcamera("FXCam+y",visflagstart+2,	0,0,0)
	addcamera("FXCam-y",visflagstart+3,	0,180,180)
	addcamera("FXCam+z",visflagstart+4,	90,0,0)
	addcamera("FXCam-z",visflagstart+5,	-90,0,-180)
	
	return cams
end