Compat = {}
LuxModule.registerclass("luxinialuacore","Compat",
		[[Compat is a class that provides compatibility functions for previous 
		releases of Luxinia. The changes to the last released developer version are
		automatically applied. Else load compatibility manually. Sometimes backwards compatibility cannot be achived.]],
			Compat,{
				setcompatibility = [[(boolean):(float fromversion) - sets compatibility from the given version string e.g 0.98 ]]
				})
		
		
-- this file is for compatibily changes due to changes in the luxinia API
-- if a certain compatibility interferes with the existing functions it 
-- must be activated manually.
-- Otherwise it is applied automaticly. Don't depend on older APIs

-- if luxinia is started with "nocompat" the compatibility is deactivated
-- usefull if you want to make sure that you don't use deprecated APIs
for i,v in pairs(luxarg) do 
	if (v=="nocompat") then return end
end

local deprcalled = {}

local function deprecated (a)
	local deb = debug.getinfo(3,"lS")
	local func = deb.source..":"..deb.currentline
	if (not deprcalled[func]) then
		deprcalled[func] = true
		_print(a.."\n  -- "..func)
	end
end



Compat.diff = {}

function Compat.setcompatibility (vmin)
	local execfuncs = {}
	
	for i,func in pairs(Compat.diff) do
		local inum = tonumber(i)
		if (inum and inum >= vmin) then
			table.insert(execfuncs,{inum,func})
		end
	end
	
	table.sort(execfuncs,function(a,b) return a[1] < b[1] end)
	
	for i,funcs in ipairs(execfuncs) do
		print("Compatibility:",funcs[1])
		funcs[2]()
	end
	
	return true
end

Compat.diff[0.92] = function ()
	do	-- the naming of the ODE object destructors was incompatible to the other 
		-- apis. This changed in V0.92
		local odestuff = {
			dgeom,dbody,dgeombox,dgeomccylinder,dgeomcone,dgeomcylinder,
			dgeomplane,dgeomray,dgeomsphere,dgeomtransform,dgeomtrimesh,
			dgeomtrimeshdata,dspace,dspacehash,dspacequad,dspacesimple,dterrain,
			djoint,djointamotor,djointball,djointcontact,djointfixed,djointhinge,
			djointhinge2,djointslider,djointuniversal,djointgroup,dworld 
		}
		local names = {
			"dgeom","dbody","dgeombox","dgeomccylinder","dgeomcone","dgeomcylinder",
			"dgeomplane","dgeomray","dgeomsphere","dgeomtransform","dgeomtrimesh",
			"dgeomtrimeshdata","dspace","dspacehash","dspacequad","dspacesimple","dterrain",
			"djoint","djointamotor","djointball","djointcontact","djointfixed","djointhinge",
			"djointhinge2","djointslider","djointuniversal","djointgroup","dworld"
		}
		for i,v in pairs(odestuff) do
			local name = names[i]
			v.destroy = function (self) 
				deprecated("warning: "..name..
						".destroy is deprecated; use delete instead.")
				self:delete()
			end
		end	
		
		local odestuff = {
			dgeom,dbody,dgeombox,dgeomccylinder,dgeomcone,dgeomcylinder,
			dgeomplane,dgeomray,dgeomsphere,dgeomtransform,dgeomtrimesh,
			dgeomtrimeshdata,dspace,dspacehash,dspacequad,dspacesimple,dterrain,
		}
		for i,v in pairs(odestuff) do -- the bodies are already using 
			-- the name "local2global" and since the geoms do have now a 
			-- "global2local" function, I prefer to rename the old functionname
			-- "transform", which could be misinterpreted. 
			local name = names[i]
			v.transform = function (self,...) 
				deprecated("warning: "..name..
						".transform is deprecated; use local2global instead.")
				return self:local2world(...)
			end
		end	
		
		for i,v in pairs(odestuff) do -- rot is now rotrad
			local name = names[i]
			v.rot = function (self,...) 
				deprecated("warning: "..name..
						".destroy is deprecated; use rotrad instead.")
				return self:rotrad(...)
			end
		end			
		
	end
	
	do 	-- luxinia.dir returned a table with files within a directory
		-- this is replaced by the lua file system module from the 
		-- kepler project (thanks alot :))
		function luxinia.dir (path) 
			local find = string.gsub(path,".*/([^/]*)","%1"):gsub("%*.%*",".*"):gsub("%*.",".*%.")
			local path = string.gsub(path,"(.*)/[^/]*","%1")
			
			--print("luxinia.dir: ",path,find)
			--find = find~="" and find or ".*"
			deprecated("warning: luxinia.dir is deprecated. Use lfs.dir "..
				"function from the lfs project instead (see "..
				"http://www.keplerproject.org/luafilesystem/) - this function may not work as the previous one!")
			local tab = {}
			for name in lfs.dir(path) do 
				if (string.match(name,find)) then 
					table.insert(tab,name)
				end
			end
			return tab
		end
	end
	
	do 	-- previously, the mouseposition was stored in table named mouse
		-- mouse.x,mouse.y, mouse[1],mouse[2],mouse[3]
		mouse = {}
		local idx = {
			function (self,index) return MouseCursor.wasButtonPressed (0) end,
			function (self,index) return MouseCursor.wasButtonPressed (1) end,
			function (self,index) return MouseCursor.wasButtonPressed (2) end,
			x = function (self,index) local x,y = MouseCursor.pos() return x end,
			y = function (self,index) local x,y = MouseCursor.pos() return y end,
		}
		setmetatable(mouse, {__index = 
			function (self, index) 
				if (idx[index]) then return idx[index](self,index) end
				return rawget(mouse,index)
			end})
				
	end
	
	do 	-- the name was not correct - "add" should be used in conjunction with
		-- lists, and each "add" should have a "remove" function. 
		Keyboard.addKeyBinding = function (...)
			deprecated("Keyboard.addKeybinding")
			Keyboard.setKeyBinding(...)	
		end
	end
			
	do	-- certain l3dset functions are not directly accessible anymore
		l3dset.fogstate = function (a,b)
			deprecated(" warning l3dset.fogstate is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):fogstate(b)
		end
		l3dset.fogstart = function (a,b)
			deprecated(" warning l3dset.fogstart is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):fogstart(b)
		end
		l3dset.fogend = function (a,b)
			deprecated(" warning l3dset.fogend is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):fogend(b)
		end
		l3dset.fogcolor = function (a,b,c,d,e)
			deprecated(" warning l3dset.fogcolor is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):fogcolor(b,c,d,e)
		end
		l3dset.backgroundcolor = function (a,b,c,d,e)
			deprecated(" warning l3dset.backgroundcolor is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):backgroundcolor(b,c,d,e or 1)
		end
		l3dset.clearcolor = function (a,b)
			deprecated(" warning l3dset.clearcolor is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):clearcolor(b)
		end
		l3dset.cleardepth = function (a,b)
			deprecated(" warning l3dset.cleardepth is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):cleardepth(b)
		end
		l3dset.skybox = function (a,b)
			deprecated(" warning l3dset.skybox is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):skybox(b)
		end
		l3dset.backgroundmaterial = function (a,b)
			deprecated(" warning l3dset.backgroundmaterial is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):backgroundmatsurface(b)
		end
		l3dset.cam = function (a,b)
			deprecated(" warning l3dset.cam is deprecated; use l3dset.getdefaultview to change environment.")
			return l3dset.getdefaultview(a):cam(b)
		end
		
		for j,v in pairs (matobject)
		do
			local i = j
			if (string.sub(i,1,1) ~= "_") then
				l3dset[i] = function (...)
					deprecated("l3dset."..i.." is deprecated; use l3dset.gedefaultview()."..i)
					local l3dv = l3dset.gedefaultview(table.remove(...,1))
					return l3dv[i](l3dv,...)
				end
			end
		end
		
		-- rotation behavior change
		local function rotwrap (tab,name,targetname,altname)
			local str = " warning "..name.." is deprecated; use ."..targetname.." or ."..altname
			tab[name] = function (a,b,c,d)
				deprecated(str)
				if (a and b and c and d) then
					return tab[targetname](a,b,c,d)
				else
					return tab[targetname](a)
				end
			end
			
		end
	
		for i,v in ipairs({l3dnode,l3dmodel,l3dprojector,l3dpemitter,l3dpgroup,
				l3dtrail,l3dprimitive,l3dlight,scenenode})
		do	
			rotwrap(v,"localrot","localrotdeg","localrotrad")
			rotwrap(v,"worldrot","worldrotdeg","worldrotrad")
		end
		
		for i,v in ipairs({l2dnode,l2dtext,l2dimage,l2dnode3d}) do
			rotwrap(v,"rot","rotdeg","rotrad")
		end
	
		rotwrap(actornode,"rot","rotrad","rotdeg")
		
		--material gone matsurface won
		l3dtrail.material = function (a,b)
			deprecated(" warning l3dtrail.material is deprecated; use l3dtrail.matsurface instead.")
			return l3dtrail.matsurface(a,b)
		end
		particlecloud.material = function (a,b)
			deprecated(" warning particlecloud.material is deprecated; use particlecloud.matsurface instead.")
			return particlecloud.matsurface(a,b)
		end
		l2dimage.material = function (a,b)
			deprecated(" warning l2dimage.material is deprecated; use l2dimage.matsurface instead.")
			return l2dimage.matsurface(a,b)
		end
		l3dmodel.material = function (a,b,c)
			deprecated(" warning l3dmodel.material is deprecated; use l3dmodel.matsurface instead.")
			return l3dmodel.matsurface(a,b,c)
		end
		
		
		-- mesh interfaces
		model.getbone = function (a,b)
			deprecated(" warning model.getbone is deprecated; use model.boneid or .bonecount instead.")
			
			if (b) then
				return model.boneid(a,b)
			else
				return model.bonecount(a)
			end
		end
		
		model.getmesh = function (a,b)
			deprecated(" warning model.getmesh is deprecated; use model.meshid or .meshcount instead.")
			
			if (b) then
				return model.meshid(a,b)
			else
				return model.meshcount(a)
			end
		end
		
		meshid.vertex = function (a,b)
			deprecated(" warning meshid.vertex is deprecated; use meshid.vertexPos or .vertexCount instead.")
			if (b) then
				return meshid.vertexPos(a,b)
			else
				return meshid.vertexCount(a)
			end
		end
		
		meshid.triangle = function (a,b)
			deprecated(" warning meshid.triangle is deprecated; use meshid.indexPrimitive or .indexPrimitivecount instead.")
			if (b) then
				return meshid.indexPrimitive(a,b)
			else
				return meshid.indexPrimitivecount(a)
			end
		end
		
		scenenode.getdefault = function ()
			deprecated(" warning scenenode.getdefault is deprecated; use scenenode.getroot instead.")
			return scenenode.getroot()
		end
		
		-- rfLit behavior change
		local function rflitwrap (tab,name,targetname)
			local str = " warning "..name.." is deprecated; use ."..targetname
			tab[name] = function (a,b,c,d)
				deprecated(str)
				if (a and b and c) then
					return tab[targetname](a,b,c)
				elseif (a and b) then
					return tab[targetname](a,b)
				else
					return tab[targetname](a)
				end
			end
		end
	
		for i,v in ipairs({l3dtrail,l3dmodel,l3dprimitive,l2dnode,l2dtext,l2dimage,particlecloud})
		do	
			rflitwrap(v,"rfLit","rfLitSun")
		end
		
		-- resdata
		resdata.destroydefault = function ()
			deprecated("resdata.destroydefault is deprecated use reschunk.destroydefault instead")
			return reschunk.destroydefault()
		end
		
		--light spelling
		l3dlight.attenautequadratic = function (...)
			deprecated("l3dlight.attenautequadratic is deprecated use l3dlight.attenuatequadratic instead (typo)")
			return l3dlight.attenuatequadratic(...)
		end
		
		-- pgraph
		render.drawpgraph = function (...)
			deprecated("render.drawpgraph is deprecated use pgraph.draw instead")
			return pgraph.draw(...)
		end
		
	end
end

Compat.diff[0.93] = function ()
	SkinableButton = {}
	setmetatable(SkinableButton,{__index = function (self,idx) 
		if (Button[idx]) then 
			deprecated(" warning SkinableButton is deprecated; use Button instead.")
			return Button[idx]
		end
		return rawget(self,idx)
	end})
	function SkinableButton.new (class, x,y,w,h,skinparam,caption)
		deprecated(" warning SkinableButon is deprecated; use Button instead.")
		return Button.new(Button,x,y,w,h,caption,skinparam)
	end
	
	
	texture.nofilter = function (a)
		deprecated("texture.nofilter is deprecated use texture.filter(false) instead")
		texture.filter(a,false,false)
	end
	
	render.drawcube = function(a)
		deprecated("render.drawcube is deprecated use render.drawtexture instead")
		if (a) then
			render.drawtexture(a)
		else
			render.drawtexture()
		end
	end

	SkinableGroupFrame = {}
	setmetatable(SkinableGroupFrame,{ __index = function (self,idx)
		if (Container[idx]) then
			deprecated(" warning SkinablgeGroupFrame is deprecated; use Container instead.")
			return Container[idx]
		end
		return rawget(self,idx)
	end})
	function SkinableGroupFrame.new(class,x,y,w,h)	
		local self = Container:new(x,y,w,h)
		self:setSkin("default")
		return self
	end

	function Component:releaseMouse()
		dprecated(" warning: Component.releaseMouse is deprecated; use unlockMouse instead (due to clearer name")
		self:unlockMouse()
	end
	
	particlesys.deletall = function(a)
		deprecated("particlesys.deletall is deprecated, use particlesys.clearparticles instead")
		particlesys.clearparticles(a)
	end
	
	particlecloud.deletall = function(a)
		deprecated("particlecloud.deletall is deprecated, use particlecloud.clearparticles instead")
		particlecloud.clearparticles(a)
	end
	
	registry = {}
	setmetatable(registry,{__index = 
		function()
			deprecated("registry class does not exist anymore")
			return function () end
		end})
	render.usevbos = function (a)
		deprecated("render.usevbos is deprecated, use system.usevbos instead")
		return system.usevbos(a)
	end
	
	render.capability = function (a)
		deprecated("render.capability is deprecated, use system.capability instead")
		return system.capability(a)
	end
	-- matrix4x4
	matrix4x4.seteuler = function(a,x,y,z)
		deprecated("matrix4x4.seteuler is depreceted, use matrix4x4.rotrad instead")
		matrix4x4.rotrad(a,x,y,z)
	end
	matrix4x4.setrotationrad = function(a,x,y,z)
		deprecated("matrix4x4.setrotationrad is depreceted, use matrix4x4.rotrad instead")
		matrix4x4.rotrad(a,x,y,z)
	end
	-- renderflag
	renderflag.new = function()
		deprecated("renderflag.new is deprecated, use renderflag.newrf instead")
		return renderflag.newrf()
	end
	
	-- l3dview backgroundcolor
	l3dview.backgroundcolor = function(...)
		deprecated("l3dview.backgroundcolor is deprecated use l3dview.clearcolorvalue instead")
		return	l3dview.clearcolorvalue(...)
	end
end

Compat.diff[0.94] = function ()

	l3dnode.deleteall = function()
		deprecated("l3dnode.deleteall is deprecaded use l3dlist.reinit instead")
		l3dlist.reinit()
	end

end

Compat.diff[0.96] = function ()

	-- global consistently replaced by world
	skybox.globalrotation = function(...)
		deprecated("skybox.globalrotation is deprecaded use skybox.worldrotation instead")
		return skybox.worldrotation(...)
	end
	
	particlecloud.useglobalaxis = function(...)
		deprecated("particlecloud.useglobalaxis is deprecaded use particlecloud.useworldaxis instead")
		return particlecloud.useworldaxis(...)
	end
	particlecloud.globalaxis = function(...)
		deprecated("particlecloud.globalaxis is deprecaded use particlecloud.worldaxis instead")
		return particlecloud.worldaxis(...)
	end
	
	particlesys.useglobalaxis = function(...)
		deprecated("particlesys.useglobalaxis is deprecaded use particlesys.useworldaxis instead")
		return particlesys.useworldaxis(...)
	end
	particlesys.globalaxis = function(...)
		deprecated("particlesys.globalaxis is deprecaded use particlesys.worldaxis instead")
		return particlesys.worldaxis(...)
	end
	
	dbody.global2local = function(...)
		deprecated("dbody.global2local is deprecaded use dbody.world2local instead")
		return dbody.world2local(...)
	end
	dbody.globalpointvel = function(...)
		deprecated("dbody.globalpointvel is deprecaded use dbody.worldpointvel instead")
		return dbody.worldpointvel(...)
	end
	dbody.local2global = function(...)
		deprecated("dbody.local2global is deprecaded use dbody.local2world instead")
		return dbody.local2world(...)
	end
	
	dgeom.global2local = function(...)
		deprecated("dgeom.global2local is deprecaded use dgeom.world2local instead")
		return dgeom.world2local(...)
	end
	dgeom.local2global = function(...)
		deprecated("dgeom.local2global is deprecaded use dgeom.local2world instead")
		return dgeom.local2world(...)
	end
	
	-- link renamed to parent
	do
		local classes = {
			l2dnode,l2dimage,l2dtext,l2dflag,l2dnode3d,
		}
		local classnames = {
			"l2dnode","l2dimage","l2dtext","l2dflag","l2dnode3d",
		}
		for i,v in pairs(classes) do
			local name = classnames[i]
			v.link = function (...) 
				deprecated("warning: "..name..".link is deprecated; use parent instead.")
				return v.parent(...)
			end
		end	
	end
	do
		local classes = {
			l3dnode,l3dlevelmodel,l3dlight,l3dmodel,l3dpemitter,l3dpgroup,l3dprimitive,l3dprojector,
			l3dshadowmodel,l3dtext,l3dtrail,
		}
		local classnames = {
			"l3dnode","l3dlevelmodel","l3dlight","l3dmodel","l3dpemitter","l3dpgroup",
			"l3dprimitive","l3dprojector","l3dshadowmodel","l3dtext","l3dtrail",
		}
		for i,v in pairs(classes) do
			local name = classnames[i]
			v.link = function (...) 
				deprecated("warning: "..name..".link is deprecated; use parent instead.")
				return v.parent(...)
			end
			
			v.linkbone = function (...) 
				deprecated("warning: "..name..".linkbone is deprecated; use parentbone instead.")
				return v.parentbone(...)
			end
		end	
	end
end


Compat.diff[0.97] = function ()

	-- texture loading changed
	-- why ? because files actually store
	-- the dimension of a texture
	texture.load2d = function(...)
		deprecated("texture.load2d is deprecaded use texture.load instead")
		return texture.load(...)
	end
	texture.load2dlum = function(...)
		deprecated("texture.load2dlum is deprecaded use texture.loadlum instead")
		return texture.loadlum(...)
	end
	texture.load2dalpha = function(...)
		deprecated("texture.load2dalpha is deprecaded use texture.loadalpha instead")
		return texture.loadalpha(...)
	end
	texture.load2ddata = function(...)
		deprecated("texture.load2ddata is deprecaded use texture.load instead")
		return texture.loaddata(...)
	end
	
	-- texture sample/pixel unified
	
	texture.sample2d = function(...)
		deprecated("texture.sample2d is deprecaded use texture.sample instead")
		return texture.sample(...)
	end
	
	texture.pixel2d = function(...)
		deprecated("texture.pixel2d is deprecaded use texture.pixel instead")
		return texture.pixel(...)
	end
	
	

	meshid.new = function(...)
		deprecated("meshid.new is deprecaded use meshid.init instead")
		return meshid.init(...)
	end
	boneid.new = function(...)
		deprecated("boneid.new is deprecaded use boneid.init instead")
		return boneid.init(...)
	end
	skinid.new = function(...)
		deprecated("skinid.new is deprecaded use skinid.init instead")
		return skinid.init(...)
	end
	
	
	-- ExtMath cleanups
	
	ExtMath.v3dif = function(...)
		deprecated("ExtMath.v3dif is deprecaded use ExtMath.v3sub instead")
		return ExtMath.v3sub(...)
	end
	ExtMath.v4dif = function(...)
		deprecated("ExtMath.v4dif is deprecaded use ExtMath.v4sub instead")
		return ExtMath.v4sub(...)
	end
	ExtMath.v3sum = function(...)
		deprecated("ExtMath.v3sum is deprecaded use ExtMath.v3add instead")
		return ExtMath.v3add(...)
	end
	ExtMath.v4sum = function(...)
		deprecated("ExtMath.v4sum is deprecaded use ExtMath.v4add instead")
		return ExtMath.v4add(...)
	end
	
	-- renamed cube to box
	l3dprimitive.newcube = function(...)
		deprecated("l3dprimitive.newcube is deprecaded use l3dprimitive.newbox instead")
		return l3dprimitive.newbox(...)
	end
	
	-- resouce.new to resource.create
	texture.new2d = function(...)
		deprecated("texture.new2d is deprecaded use texture.create2d instead")
		return texture.create2d(...)
	end
	texture.new3d = function(...)
		deprecated("texture.new3d is deprecaded use texture.create3d instead")
		return texture.create3d(...)
	end
	texture.newcube = function(...)
		deprecated("texture.new3d is deprecaded use texture.createcube instead")
		return texture.createcube(...)
	end
	model.new = function(...)
		deprecated("model.new is deprecaded use model.create instead")
		return model.create(...)
	end
	model.newfinish = function(...)
		deprecated("model.newfinish is deprecaded use model.createfinish instead")
		return model.createfinish(...)
	end
	
	
	-- l3dview background
	
	l3dview.skybox = function(...)
		deprecated("l3dview.skybox is deprecaded use l3dview.bgskybox instead")
		return meshid.bgskybox(...)
	end
	
	l3dview.skyl3dnode = function(...)
		deprecated("l3dview.skyl3dnode is deprecaded use l3dview.bgskyl3dnode instead")
		return meshid.bgskyl3dnode(...)
	end
	
	l3dview.backgroundmatsurface = function(...)
		deprecated("l3dview.backgroundmatsurface is deprecaded use l3dview.bgmatsurface instead")
		return meshid.bgmatsurface(...)
	end
	
	-- scenenode parent
	
	scenenode.link = function(...)
		deprecated("scenenode.link is deprecaded use scenenode.parent instead")
		local args = {...}
		return scenenode.parent(args[1],args[3])
	end
	
	-- rendersystem class
	system.detail = function(...)
		deprecated("system.detail is deprecated use rendersystem.detail instead")
		return rendersystem.detail(...)
	end
	
	system.texcompression = function(...)
		deprecated("system.texcompression is deprecated use rendersystem.texcompression instead")
		return rendersystem.texcompression(...)
	end
	
	system.texanisotropic = function(...)
		deprecated("system.texanisotropic is deprecated use rendersystem.texanisotropic instead")
		return rendersystem.texanisotropic(...)
	end
	
	-- floatarray calls
	floatarray.vector3 = function(...)
		deprecated("floatarray.vector3 is deprecated use floatarray.v3 instead")
		return floatarray.v3(...)
	end
	floatarray.vector3all = function(...)
		deprecated("floatarray.vector3all is deprecated use floatarray.v3all instead")
		return floatarray.v3all(...)
	end
	floatarray.vector4 = function(...)
		deprecated("floatarray.vector4 is deprecated use floatarray.v4 instead")
		return floatarray.v4(...)
	end
	floatarray.lit = function(...)
		deprecated("floatarray.lit is deprecated use floatarray.v4lit instead")
		return floatarray.v4lit(...)
	end
	floatarray.spline3 = function(...)
		deprecated("floatarray.spline3 is deprecated use floatarray.v3spline instead")
		return floatarray.v3spline(...)
	end
	floatarray.tonext3 = function(...)
		deprecated("floatarray.tonext3 is deprecated use floatarray.v3tonext instead")
		return floatarray.v3tonext(...)
	end
	floatarray.lerp3 = function(...)
		deprecated("floatarray.lerp3 is deprecated use floatarray.v3lerp instead")
		return floatarray.v3lerp(...)
	end
	floatarray.lerp4 = function(...)
		deprecated("floatarray.lerp4 is deprecated use floatarray.v4lerp instead")
		return floatarray.v4lerp(...)
	end
end

Compat.diff[0.98] = function ()
	-- consistent "create" functions for resources
	particlecloud.load = function(...)
		deprecated("particlecloud.load is deprecated use particlecloud.create instead")
		return particlecloud.create(...)
	end
	
	--[[
	tbterrain.newblank = function(...)
		deprecated("tbterrain.newblank is deprecated use tbterrain.createblank instead")
		return tbterrain.createblank(...)
	end
	
	tbterrain.newfinish = function(...)
		deprecated("tbterrain.newfinish is deprecated use tbterrain.createfinish instead")
		return tbterrain.createfinish(...)
	end
	
	tbterrain.newfromtex = function(...)
		deprecated("tbterrain.newfromtex is deprecated use tbterrain.createfromtex instead")
		return tbterrain.createfromtex(...)
	end
	
	tbterrain.newspecs = function(...)
		deprecated("tbterrain.newspecs is deprecated use tbterrain.createspecs instead")
		return tbterrain.createspecs(...)
	end
	]]
	
	particleforceid.newgravity = function(...)
		deprecated("particleforceid.newgravity is deprecated use particleforceid.creategravity instead")
		return particleforceid.creategravity(...)
	end
	
	particleforceid.newagedvelocity = function(...)
		deprecated("particleforceid.newagedvelocity is deprecated use particleforceid.createagedvelocity instead")
		return particleforceid.createagedvelocity(...)
	end
	
	particleforceid.newwind = function(...)
		deprecated("particleforceid.newwind is deprecated use particleforceid.createwind instead")
		return particleforceid.createwind(...)
	end
	
	particleforceid.newtarget = function(...)
		deprecated("particleforceid.newtarget is deprecated use particleforceid.createtarget instead")
		return particleforceid.createtarget(...)
	end
	
	particleforceid.newmagnet = function(...)
		deprecated("particleforceid.newmagnet is deprecated use particleforceid.createmagnet instead")
		return particleforceid.createmagnet(...)
	end
	
	reschunk.new = function(...)
		deprecated("reschunk.new is deprecated use reschunk.create instead")
		return reschunk.create(...)
	end

	-- matrix16
	matrix16 = {}
	local matfuncs = {"new","tostring","component","identity","mul","mulfull","lookat","setaxisangle","transform",
				"transformrotate","pos","rotdeg","rotrad","rotaxis","rotquat","copy","column","affineinvert","invert","transpose"}
	for i,v in ipairs(matfuncs) do
		matrix16[v] = function(...)
			deprecated("matrix16."..v.." is deprecated use matrix4x4."..v.." instead")
			return matrix4x4[v](...)
		end
	end
	
	-- input
	input.isKeyDown = function(...)
		deprecated("input now has lower-case commands")
		return input.iskeydown(...)
	end
	input.isMouseDown = function(...)
		deprecated("input now has lower-case commands")
		return input.ismousedown(...)
	end
	input.mousePos = function(...)
		deprecated("input now has lower-case commands")
		return input.mousepos(...)
	end
	input.mouseWheel = function(...)
		deprecated("input now has lower-case commands")
		return input.mousewheel(...)
	end
	input.popKey = function(...)
		deprecated("input now has lower-case commands")
		return input.popkey(...)
	end
	input.popMouse = function(...)
		deprecated("input now has lower-case commands")
		return input.popmouse(...)
	end
	
end

Compat.diff[1.282] = function ()
	rcmdcopytex.side = function(...)
		deprecated("rcmdcopytex.side renamed to rcmdcopytex.sideordepth")
		return rcmdcopytex.sideordepth(...)
	end
	
	capability.cg_sm3_tex8 = function(...)
		deprecated("capability.cg_sm3_tex8 renamed to capability.cg_sm3")
		return capability.cg_sm3(...)
	end
end

Compat.diff[1.323] = function()
	texture.getdata = function(...)
		deprecated("texture.getdata renamed to texture.downloaddata")
		return texture.downloaddata(...)
	end
end

Compat.diff[1.337] = function()
	
	scalartype.float = function(...)
		deprecated("scalartype.float renamed to scalartype.float32")
		return scalartype.float32(...)
	end
	
end

Compat.diff[1.341] = function()
	
	Checkbox = {}
	setmetatable(Checkbox,{__index = Button})
	for n,f in pairs(CheckBox) do
		Checkbox[n] = f
		if (type(f) == "function") then
			Checkbox[n] = function(...)
				deprecated("Checkbox renamed to CheckBox")
				return f(...)
			end
		end
	end
	
end

Compat.diff[1.346] = function()
	

	texdatatype.ubyte = function(...)
		deprecated("texdatatype.ubyte renamed to texdatatype.fixed8")
		return texdatatype.fixed8(...)
	end
	
	texdatatype.ushort = function(...)
		deprecated("texdatatype.ushort renamed to texdatatype.fixed16")
		return texdatatype.fixed16(...)
	end
end

Compat.diff[1.349] = function()
	
	vertextype.vertex32texcoords = function(...)
		deprecated("vertextype.vertex32texcoords renamed to vertextype.vertex32tex2")
		return vertextype.vertex32tex2(...)
	end
	
	vertextype.vertex64 = function(...)
		deprecated("vertextype.vertex64 renamed to vertextype.vertex64tex4")
		return vertextype.vertex64tex4(...)
	end
	
	local collresult = dcollresult.new(128)
	dcollider.test = function(...)
		deprecated("dcollider.test deprecated use dcollresult mechanism")
		local a,b,what
		
		local count = collresult:test(a,b)
		local pos = {}
		local normal = {}
		local depth = {}
		local geoms = {}
		
		for i = 0,count-1 do 
			local x,y,z = collresult:pos(i)
			local nx,ny,nz,d =collresult:normal(i)
			local g1,g2 = collresult:geoms(i)
			
			
			table.insert(pos,x)
			table.insert(pos,y)
			table.insert(pos,z)
			
			table.insert(normal,nx)
			table.insert(normal,ny)
			table.insert(normal,nz)
			
			table.insert(depth,d)
			
			table.insert(geoms,g1)
			table.insert(geoms,g2)
		end
		
		if (what == nil or what == "pndg" or what == "") then
			return count,pos,normal,depth,geoms
		else
			local lookup = {
				["p"] = pos,
				["n"] = normal,
				["d"] = depth,
				["g"] = geoms,
			}
			local out = {count}
			local len = #what
			for i=1,len do
				local add = lookup[what:sub(i,i)]
				if (not add) then
					error("unsupported query")
					return count
				end
				table.insert(out,add)
			end
			
			return unpack(out)
		end
	end
end


Compat.diff[1.374] = function()

	rcmddraw2dmesh = {}
	for n,f in pairs(rcmddrawmesh) do
		rcmddraw2dmesh[n] = f
		if (type(f) == "function") then
			rcmddraw2dmesh[n] = function(...)
				deprecated("rcmddraw2dmesh renamed to rcmddrawmesh")
				return f(...)
			end
		end
	end
	
	rcmddraw2dmesh.usermeshvbo = function(...)
		deprecated("rcmddraw2dmesh.usermeshvbo deprecated, arguments work with rcmddrawmesh.usermesh")
		return rcmddraw2dmesh.usermesh(...)
	end
	
	l3dprimitive.usermeshvbo = function(...)
		deprecated("l3dprimitive.usermeshvbo deprecated, arguments work with l3dprimitive.usermesh")
		return l3dprimitive.usermesh(...)
	end
	
	l2dimage.usermeshvbo = function(...)
		deprecated("l2dimage.usermeshvbo deprecated, arguments work with l2dimage.usermesh")
		return l2dimage.usermesh(...)
	end
	
end

Compat.diff[1.381] = function()
	
	local strtypes = {
		{"char",scalartype.int8()},
		{"short",scalartype.int16()},
		{"int",scalartype.int32()},
		{"float",scalartype.float32()},
		{"double",scalartype.float64()},
	}
	
	for i,typ in ipairs(strtypes) do
		local bin2type = "bin2"..typ[1]
		
		string[bin2type] = function(...)
			deprecated("string."..bin2type.." deprecated, use scalartype.bin2value instead")
			return typ[2]:bin2value(...)
		end
		
		local type2bin = typ[1].."2bin"
		string[type2bin] = function(...)
			deprecated("string."..type2bin.." deprecated, use scalartype.value2bin instead")
			return typ[2]:value2bin(...)
		end
	end

end


