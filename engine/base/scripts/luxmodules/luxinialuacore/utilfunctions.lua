UtilFunctions = {}
LuxModule.registerclass("luxinialuacore","UtilFunctions",
		"The UtilFunctions class is a collection of miscellanous useful lua functions.",UtilFunctions,{})

LuxModule.registerclassfunction("pairsByKeys",
[[
	(function):(table,[comperator]) - returns an iterator function to traverse the table in
	sorted order. The comperator function can be used to use another ordering for traversion.
	The returned function returns three values if it is called: the key, the value, a
	number that tells the current index and another number that tells how many indeces
	are left to be be iterated. The comperator should take care about both
	values it gets - these could either be numbers, functions, strings, tables, etc.

	!!!!Example:

	 for key,value in UtilFunctions.pairsByKeys({cde=1, abc=2})
	 	do print(key,value) end

	!!!!Output:

	 abc  2
	 cde  3
]])
function UtilFunctions.pairsByKeys (t, f)
	local a = {}
	for n in pairs(t) do table.insert(a, n) end
	table.sort(a, f or
		function (a,b)
			if (type(a)=="number" and type(b)=="number") then return a<b
			elseif (type(a)=="number") then return true
			elseif (type(b)=="number") then return false
			else return tostring(a)<tostring(b) end
		end
	)
	local i,n = 0,table.getn(a)      -- iterator variable
	local iter = function ()   -- iterator function
		i = i + 1
		if a[i] == nil then return nil
		else return a[i], t[a[i]], i, n-i end
	end
	return iter
end

LuxModule.registerclassfunction("printf",
[[
	():(string format, ...) - prints out a formated  string as the rules of string.format describe. This
	function is also stored in the global variable named printf
]])

function UtilFunctions.printf (fmt, ...)
	print(fmt:format(...))
end

printf = UtilFunctions.printf

LuxModule.registerclassfunction("smallLogoPos",
[[
	():(int n) - Sets the position of the small Luxinia Logo at one of the four
	corners (whatever you like more). Topright = 1, rightbottom = 2, bottomleft = 3,
	topleft = 4.
]])
function UtilFunctions.smallLogoPos (n) end


LuxModule.registerclassfunction("projectpathabs",
[[
	(string):() - returns the current projectpath as absolute path. Useful for opening
	files.
]])
function UtilFunctions.projectpathabs () 
	local path = system.projectpath()
	if (not path:find(":/")) then
		local curdir = FileSystem.currentdir()
		curdir = curdir:gsub("\\","/")
		path  = path:sub(1,1) == "/" and path  or "/"..path 
		path  = curdir..path 
	end
	
	return path
end

LuxModule.registerclassfunction("tabletotalcount",
[[
	(n):(table) - returns the number of elements in that table and all of its childs
	it also traverses into the metatables, but ignores tables that are weak typed. It also counts metatables as element.

	This function should help to find memory leaks, which can occure if table references
	are not removed.
]])
function UtilFunctions.tabletotalcount (tab)
	local visited = {}
	local stack = {tab}
	local n = 0

	local function add(t)
		n = n + 1
		if type(t)=="table" and not visited[t] then
			visited[t] = true
			stack[#stack+1] = t
		end
		if type(t)=="userdata" then
			add(debug.getmetatable(t))
		end
	end

	while #stack>0 do
		local t = table.remove(stack)
		local mt = getmetatable(t)
		if mt then add(mt) end
		if not mt or not mt.__mode then
			for k,v in pairs(t) do
				add(k)
				add(v)
			end
		end
	end
	return n
end

LuxModule.registerclassfunction("setTableValue",
[[
	([index,oldvalue]):(table,value,[newvalue]) - searches a val in the given table.
	The first hit is replaced by newvalue, which removes the tableentry if
	it is nil. You can use it therefore to remove a value from a table.
	The index of the removed element and the value is then returned.

	If no match was found, nothing is returned.
]])
function UtilFunctions.setTableValue(tab,val,newvalue)
	for i,v in pairs(tab) do
		if (v == val) then
			tab[i] = newvalue
			return i,val
		end
	end
end


LuxModule.registerclassfunction("fileexists",
	[[
	(boolean exists):(string path) - tries out if the given file exists.
	This is not very clean since it only trys to open and close the file and
	if it doesn't throw an error it is assumed that the file exists.
	]])

function UtilFunctions.fileexists(file)
	return VFSmapper.fileexists(file)
end

LuxModule.registerclassfunction("luafilename", [[
	(string filename):(int level) - retrieves the filename of a luafunction,
	if possible.
	The level is the functioncallerstack and should be >=1
	returns filename and level of the found path.
	]])
function UtilFunctions.luafilename(level)
	level = level and level + 1 or 2
	local src
	while (true) do
		src = debug.getinfo(level)
		if (src == nil) then return nil,level end
		if (string.byte(src.source) == string.byte("@")) then
			return string.sub(src.source,2),level
		end
		level = level + 1
	end
end

LuxModule.registerclassfunction("luafilepath",
	[[(string filepath):(int level) - like luafilename,
	but removes the filename itself]])
function UtilFunctions.luafilepath(level)
	local src,level = UtilFunctions.luafilename(level)
	if (src == nil) then return src,level end
	src = string.gsub(src,"[\\/][^\\//]*$","")
	return src,level
end



LuxModule.registerclassfunction("luaLoadFile", "vfsmapper lua loadfile function")
UtilFunctions.luaLoadFile = loadfile
LuxModule.registerclassfunction("loadfile",
	[[():(string path) - similiar to original loadfile except that this
	function trys to find the file in different locations relative to all
	loaded luafiles. This function replaces the original loadfile function.]])
local _loadfile = loadfile
function UtilFunctions.loadfile (file,fromdo)
	assert(type(file)=='string',"String as filename expected")
	local name = file
	local level = fromdo and 4 or 3
	while (name) do
		if (UtilFunctions.fileexists(name)) then return _loadfile(name) end
		name,level = UtilFunctions.luafilepath(level)
		if (name == nil) then break end
		name = name .. "/" .. file
	end
	return _loadfile(file)
end
loadfile = UtilFunctions.loadfile
local uloadfile = UtilFunctions.loadfile

-- wrapper for the dofile function that loads and executes the luachunk
LuxModule.registerclassfunction("luaDoFile", "vfsmapper dofile function")
UtilFunctions.luaDoFile = dofile

LuxModule.registerclassfunction("dofile",
	[[():(string path,...) - similiar to original dofile except that this
	function trys to find the file in different locations relative to all
	loaded luafiles. This function replaces the original dofile function.]])
local _dofile = dofile
function UtilFunctions.dofile (file,...)
	assert(type(file) == 'string',"String as filename expected")
	return assert(uloadfile(file,true))(...)
	--[=[local name = file
	local level = 3
	while (name) do
		if (UtilFunctions.fileexists(name)) then
			return _dofile(name,unpack(arg))
		end
		name,level = UtilFunctions.luafilepath(level)
		if (name == nil) then break end
		name = name .. "/" .. file
		--_print(name)
	end
	return _dofile(file)]=]
end
dofile = UtilFunctions.dofile

LuxModule.registerclassfunction("loadFontFile",
	[[(fontset,texture):(string fontname) - loads a font generated by the Font Bitmap Generator. The fontname must not contain
	a suffix (.lua). The function trys to load a fontfile that must reside in a 'fonts' directory. You must not delete
	the fontset yourself. All characters in the fontset that are not set by the font itself are mapped to the glyphid 0 (lefttopmost glyph)]])
local fonttables = {}
function UtilFunctions.loadFontFile (fontname)
	if fonttables[fontname] then
		return  fonttables[fontname][1], texture.load("fonts/"..fontname..".tga")
	end

	--reschunk.usecore(true)
	local tex = assert(texture.load("fonts/"..fontname..".tga"), "font texture not found - make sure that "..fontname.."exists!")
	--reschunk.usecore(false)
	local param
	local flua = "base/fonts/"..fontname..".lua"
	if not FileSystem.attributes(flua) then flua = system.projectpath().."/fonts/"..fontname..".lua" end
	param = dofile(flua)

	local fset = fontset.new(param.tablesize)
	for i=0,255 do fset:lookup(i,0) end
	local div = param.resolution/param.tablesize
	for i=1,param.tablesize*param.tablesize do
		local chr = param.chartable[i]
		local w = param.widths[i]
		fset:width(chr,w/div)
		fset:lookup(chr,i-1)
	end

	fset:offsetx(param.xoffset)
	fset:offsety(param.yoffset)
	fset.scale = param.resolution/256
	fset:linespacing(param.lineheight/fset.scale)
	fonttables[fontname] = {fset,tex}
	return fset,tex
end

LuxModule.registerclassfunction("getModelColMesh",
	[[(tableBase,tableHit,[tableRaw]):(model,[boolean needrawtable],[matrix4x4],[boolean visibleonly],[boolean neednotrimeshdata]) - Creates a geom trimeshdata from the given model. Optionally transforms vertices.<br>
	tableBase : {data = dgeomtrimeshdata, aabox = {min{x,y,z},max{x,y,z}, center = {x,y,z}} <br>
	tableHit : {table similar to base, but for every "hitmesh" = meshes that contain "hitzone" in the material name <br>
	tableRaw : { inds = indices{}, verts = vertices{}, tris = tridata{{mesh=meshid,orig=original index}...} }, table with all indices/vertices and a table with the tridata for every face in the model
	]])
function UtilFunctions.getModelColMesh (mdl,needrawtable,matrix,visibleonly,neednotrimeshdata)
	local indices,vertices= {},{}
	local hitmeshes
	local globalprop = {}
	local tridata = {}

	local function addmesh (m,mcur)
		local useindex,usevertices = indices,vertices
		local hitmesh = false
		local props = globalprop
		if (m:texname() and string.find(m:texname(),"hitzone")) then
			useindex = {}
			usevertices = {}
			hitmesh = true
			props = {}
		end
		if not visibleonly and ((m:texname() and string.find(m:texname(),".*light.*")) or (m:name() and m:name():find("nocol"))) then
			return
		end
		if visibleonly and string.find(m:name(),"invisible") then return end

		-- handle bone transforms
		local mymatrix = nil
		local bid = m:boneid()
		if (bid) then
			local bmatrix = bid:matrix()
			if (matrix) then
				mymatrix = matrix4x4.new()
				mymatrix:mul(matrix,bmatrix)
			else
				mymatrix = bmatrix
			end
		else
			mymatrix = matrix
		end

		local offset = table.getn(usevertices)/3
		local tricnt = m:indexPrimitivecount()-1

		do
			local idxoff = #useindex
			local maxref = 0
			for i=0,tricnt do
			   local tri = {m:indexPrimitive(i)}
			   local a,b,c = unpack(tri)
			   --_print(a,b,c)
			   if (not (a==b or b==c or c==a)) then
				   table.insert(tridata,{mesh = m, index = i})

				   table.insert(useindex,offset+a)
				   table.insert(useindex,offset+b)
				   table.insert(useindex,offset+c)
				   maxref = math.max(a,b,c,maxref)
			   end
			end

			for i=0,maxref do
			   local p = {m:vertexPos(i,mymatrix)}
			   props.min = props.min or {p[1],p[2],p[3]}
			   props.max = props.max or {p[1],p[2],p[3]}
			   for k=1,3 do
					   props.min[k] = math.min(props.min[k], p[k])
					   props.max[k] = math.max(props.max[k], p[k])
			   end
			   table.insert(usevertices,p[1])
			   table.insert(usevertices,p[2])
			   table.insert(usevertices,p[3])
			end
		end
		
		if (hitmesh) then
			hitmeshes = hitmeshes or {}
			cntr = {(props.min[1]+props.max[1])*0.5,(props.min[2]+props.max[2])*0.5,
				(props.min[3]+props.max[3])*0.5 }
			table.insert(hitmeshes,{data = dgeomtrimeshdata.new(useindex,usevertices),
				aabox = props, center = cntr})
		end
	end

	for i=0,mdl:meshcount()-1 do
		addmesh(mdl:meshid(i),i)
	end
	local props = globalprop
	props.min = props.min or {0,0,0}
	props.max = props.max or {0,0,0}
	local cntr = {(props.min[1]+props.max[1])*0.5,(props.min[2]+props.max[2])*0.5,
		(props.min[3]+props.max[3])*0.5 }

	local rawtable = nil
	if (needrawtable) then
		rawtable = {inds = indices, verts = vertices, tris = tridata}
	end
	--print("trimeshcount: ",#indices/3)
	return 	{data = neednotrimeshdata or dgeomtrimeshdata.new(indices,vertices), aabox = globalprop, center = cntr}, hitmeshes, rawtable
end



LuxModule.registerclassfunction("loadCollisionDataFile",
	[[
	(dgeom table,[extratable]):([string path,table],[boolean needoriginaltable],[boolean createl3ds]) - loads dgeoms from the collision data lua file. extratable can contain the tables: "original" and/or "visual".Currently exported by 3dsmax.
	]])

function UtilFunctions.loadCollisionDataFile(file,needoriginaltable,createl3ds)
	local header,geomdata
	if (type(file) ~= "table") then
		header,geomdata = dofile(file)
	else
		header,geomdata = unpack(file)
	end


	local geoms = {}
	local visual = {}
	local trimeshdatas = {}
	local loader = {}

	if (not createl3ds) then
	 createl3ds = nil
	end

	if (header ~= "luxinia_collision_data version 001") then return end

	--print("importing..")

	loader.dgeombox = function (tab)
		return 	dgeombox.new(unpack(tab.parameters.sizes)),
			createl3ds and l3dprimitive.newcube(tab.name,unpack(tab.parameters.sizes))
	end

	loader.dgeomsphere = function (tab)
		return 	dgeomsphere.new(tab.parameters.radius),
			createl3ds and l3dprimitive.newsphere(tab.name,tab.parameters.radius)
	end

	loader.dgeomcylinder = function (tab)
		return 	dgeomcylinder.new(tab.parameters.radius,tab.parameters.height),
			createl3ds and l3dprimitive.newcylinder(tab.name,tab.parameters.radius,tab.parameters.radius,tab.parameters.height)

	end

	loader.dgeomccylinder = function (tab)
		local vis = createl3ds and l3dprimitive.newcylinder(tab.name,tab.parameters.radius,tab.parameters.radius,tab.parameters.height)
		if (vis) then
			local l3d = l3dprimitive.newsphere(tab.name,tab.parameters.radius)
			l3d:link(vis)
			l3d:localpos(0,0,tab.parameters.height/2)
			l3d:rfLitSun(true)
			l3d = l3dprimitive.newsphere(tab.name,tab.parameters.radius)
			l3d:link(vis)
			l3d:localpos(0,0,-tab.parameters.height/2)
			l3d:rfLitSun(true)
		end
		return dgeomccylinder.new(tab.parameters.radius,tab.parameters.height),vis
	end

	loader.dgeomtrimesh = function (tab)
		local trimesh
		local tabname
		local l3d = createl3ds and l3dprimitive.newcube(tab.name,100) --todo better size approximation

		if (tab.parameters.trimesh) then
			trimesh = trimeshdatas[tab.parameters.trimesh].mesh
			tabname = tab.parameters.trimesh
		else
			trimesh =  dgeomtrimeshdata.new(tab.parameters.indicesdata,tab.parameters.verticesdata)
			trimeshdatas[tab.name] = {mesh = trimesh, verts = tab.parameters.verticesdata, inds = tab.parameters.indicesdata}
			tabname = tab.name
		end
		if (not trimesh) then
			return nil
		end
		if (l3d) then
			l3d:renderscale(1,1,1)
			local tabverts = trimeshdatas[tabname].verts
			local tabinds = trimeshdatas[tabname].inds
			l3d:usermesh(vertextype.vertex32normals(),#tabverts,#tabinds)
			l3d:vertexCount(#tabverts)
			for i,v in ipairs(tabverts) do
				l3d:vertexPos(i-1,unpack(v))
				local normal = {unpack(v)}
				-- poor man's normals
				normal = ExtMath.v3normalize(normal)
				l3d:vertexNormal(i-1,unpack(normal))
			end

			l3d:indexPrimitivetype(primitivetype.triangles())
			l3d:indexCount(#tabinds)
			for i,v in ipairs(tabinds) do
				l3d:indexValue(i-1,v)
			end
			l3d:indexMinmax()
			l3d:indexTrianglecount()
			l3d:rfLitSun(true)
		end
		return  dgeomtrimesh.new(trimesh),l3d
	end

	for i,obj in pairs(geomdata) do
		local geom,l3dvis = loader[obj.type](obj)
		if (geom ~= nil) then
			geom:pos(unpack(obj.pos))
			geom:rotdeg(unpack(obj.rot))
			table.insert(geoms,geom)
		else
			print("luxinia_collision_data loader error: no geom created "+obj.name)
		end
		if (l3dvis) then
			vis = {s3d = scenenode.new(obj.name) , l3d = l3dvis}
			vis.s3d:localpos(unpack(obj.pos))
			vis.s3d:localrotdeg(unpack(obj.rot))
			vis.l3d:linkinterface(vis.s3d)
			vis.l3d:rfLitSun(true)
			table.insert(visual,vis)
		end
	end


	if (needoriginaltable or createl3ds) then
		local extradata = {}
		if (needoriginaltable) then extradata.original = geomdata end
		if (createl3ds) then extradata.visual = visual end

		return geoms,extradata
	else
		return geoms
	end
end

UtilFunctions.div255 = 1/255
LuxModule.registerclassfunction("color3byte", [[(float r,g,b):(float r,g,b) - converts the incoming color values 0-255, to 0-1 floats]])
function UtilFunctions.color3byte(r,g,b)
	return r*UtilFunctions.div255,g*UtilFunctions.div255,b*UtilFunctions.div255
end

LuxModule.registerclassfunction("color4byte", [[(float r,g,b,a):(float r,g,b,a) - converts the incoming color values 0-255, to 0-1 floats]])
function UtilFunctions.color4byte(r,g,b,a)
	return r*UtilFunctions.div255,g*UtilFunctions.div255,b*UtilFunctions.div255,a*UtilFunctions.div255
end


LuxModule.registerclassfunction("color3rgb2hsv", "(float h,s,v) : (float r,g,b) - converts the rgb color to a hsv color value")
function UtilFunctions.color3rgb2hsv (r,g,b)
	local min = (r<g and r<b and r) or (g<b and g) or b
	local max = (r>g and r>b and r) or (g>b and g) or b
	local v,s,h = max

	local delta = max - min;

	if  max ~= 0 then
		s = delta / max
	else
		return 0,0,0
	end

	if delta == 0 then
		return 0,0,v
	end

	if r == max then
		h = ( g - b ) / delta
	elseif g == max then
		h = 2 + ( b - r ) / delta
	else
		h = 4 + ( r - g ) / delta
	end

	h = h * 60
	if h < 0 then
		h = h + 360
	end

	return h/360,s,v
end

LuxModule.registerclassfunction("color3hsv", [[(float r,g,b):(float h,s,v) - converts the incoming hsv to color to 0-1 rgb floats]])
function UtilFunctions.color3hsv(H,S,V)
	if (S == 0) then
		return V*1.0,V*1.0,V*1.0
	else
		local var_h = (H*6)%6

		local var_i = math.floor(var_h)
		local var_1 = V * ( 1 - S )
		local var_2 = V * ( 1 - S * ( var_h - var_i ) )
		local var_3 = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) )

		if     ( var_i == 0 ) then 	return V,		var_3,	var_1
		elseif ( var_i == 1 ) then 	return var_2, 	V,		var_1
		elseif ( var_i == 2 ) then 	return var_1, 	V,		var_3
		elseif ( var_i == 3 ) then 	return var_1, 	var_2,	V
		elseif ( var_i == 4 ) then 	return var_3,  	var_1,	V
		else                  		return V,     	var_1,	var_2
		end
	end

end

FreeLook = {}
FreeLook.state = false
FreeLook.shiftmulti = 5.0
FreeLook.movemulti = 2.5
FreeLook.invert = false

function FreeLook.think()
	if not FreeLook.camctrl then return end -- got disabled
	FreeLook.camctrl:runThink()
end

LuxModule.registerclassfunction("freelook",
	[[(boolean):(boolean,Component guicomp) - enables/disables freelook of current 
	default l3dcamera. Useful for debugging, 
	FreeLook.invert .movemulti .shiftmulti let you modify behavior. 
	Controls are WASD,C,SPACE + Mouse. 
	Optionally installed as drag listener into guicomp.]])
	
function UtilFunctions.freelook(state,guicomp)
	if state == nil then return FreeLook.state end
	if (FreeLook.state == state) then return end
	FreeLook.state = state
	if (state) then
		FreeLook.act = actornode.new("_fl",true)
		FreeLook.cam = l3dcamera.default()
		FreeLook.oldlink = FreeLook.cam:linkinterface()

		if (FreeLook.oldlink) then
			local mat = FreeLook.oldlink.worldmatrix and FreeLook.oldlink:worldmatrix() or FreeLook.oldlink:matrix()
			FreeLook.act:matrix(mat)
		else
			FreeLook.oldwm = FreeLook.cam:worldmatrix()
		end
		FreeLook.cam:linkinterface(FreeLook.act)
		render.drawcamaxis(true)
		Timer.set("_util_freelook",FreeLook.think)
		FreeLook.camctrl = CameraEgoMKCtrl:new(FreeLook.cam,FreeLook.act,FreeLook)
		FreeLook.camctrl:reset()
		FreeLook.mouselistener = FreeLook.camctrl:createML(guicomp)
			
		if (guicomp) then
			guicomp:addMouseListener(FreeLook.mouselistener)
			FreeLook.guicomp = guicomp
		else
			local w,h = window.refsize()
			MouseCursor.pos(w/2,h/2)
			MouseCursor.addMouseListener(FreeLook.mouselistener)
		end
	else
		Timer.remove("_util_freelook")
		if (FreeLook.guicomp) then
			FreeLook.guicomp:removeMouseListener(FreeLook.mouselistener)
			FreeLook.guicomp = nil
		else
			MouseCursor.removeMouseListener(FreeLook.mouselistener)
		end

		render.drawcamaxis(false)
		if (FreeLook.oldlink) then
			FreeLook.cam:linkinterface(FreeLook.oldlink)
		else
			FreeLook.cam:unlink()
			FreeLook.cam:usemanualworld(true)
			FreeLook.cam:worldmatrix(FreeLook.oldwm)
		end
		FreeLook.cam = nil
		FreeLook.oldwm = nil
		FreeLook.camctrl = nil

	end

end

LuxModule.registerclassfunction("simplerenderqueue",
	[[(l3dview):([l3dview],[boolean noempty],[tab ignorelayers]) - sets up a standard renderqueue for the view (which is also returned).
	The l3dview has equivalent rcmds in its table. The order is: 
	* rClear
	* rDepth
	* rDrawbg
	* rLayers[1..16]
	** stencil
	** drawlayer
	** drawprt
	* rDrawdebug
	
	Each rLayer contains: stencil,drawlayer,drawprt for the equivalent l3dlayerids-1. Sorting is enabled and material-based.<br>
	ignorelayers should contain 'true' at layer+1 field. Ie {true,true} will disable layer 0 & 1.
	]])
function UtilFunctions.simplerenderqueue(view,noempty,ignorelayers)
	view = view or l3dset.get(0):getdefaultview()

	--if (not l3dview._globalrefs) then
	--	l3dview._globalrefs = {}
	--end

	if (not noempty) then
		view:rcmdempty()
	end

	view.rClear = rcmdclear.new()
	view.rClear:colorvalue(0.8,0.8,0.8,0)
	view:rcmdadd(view.rClear)
	
	view.rDepth = rcmddepth.new()
	view.rDepth:compare(comparemode.lequal())
	view:rcmdadd(view.rDepth)
	
	view.rDrawbg = rcmddrawbg.new()
	view:rcmdadd(view.rDrawbg)

	view.rLayers = {}
	for i=1,l3dlist.l3dlayercount() do
		if ((not ignorelayers) or (not ignorelayers[i])) then
			local st,d,prt = rcmdstencil.new(),rcmddrawlayer.new(),rcmddrawprt.new()
			view.rLayers[i] = {drawlayer=d,drawprt=prt,stencil=st}
			d:layer(i-1)
			d:sort(1)
			prt:layer(i-1)

			view:rcmdadd(st)
			view:rcmdadd(d)
			view:rcmdadd(prt)
		else
			view.rLayers[i] = false
		end
	end
	view.rDrawdebug = rcmddrawdbg.new()
	view:rcmdadd(view.rDrawdebug)
	--l3dview._globalrefs[view] = view

	return view
end

LuxModule.registerclassfunction("picktest",
	[[([geom,{x,y,z},{nx,ny,nz}]):(dspace space,[x=mousex],[y=mousey],[maxlength=1000000],[l3dcamera camera = l3dcamera.default]) -
	This function will test the given space with a ray and returns, if a hit was generated, the geom, the coordinate of the
	nearest hit and the hitnormal. The ray is in line with the x,y pair (or the mouse coordinates if not given)
	on the camera projection plane and through the last known camera worldposition (this worldposition can be one frame behind, so
	you might have to call camera:forceupdate() to update to the currently set position if this is important, which is not in most cases).
	The maxlength can be optionally set and is per default set to 1000000. If no camera is specified, the default camera is used.

	This function handles also orthogonal camera fovs, which requires a different
	raysetting code.

	!!Example:

	[img utilfunctions.picktest.png]

	 UtilFunctions.simplerenderqueue()

	 input.showmouse(true) -- so we can see the mouse
	 space = dspacehash.new() -- our space with our objects
	 box = actornode.new("box")  -- our box location
	 box.geom = dgeombox.new(1,1,1,space) -- the geom, placed in 0,0,0
	 box.geom.l3d = l3dprimitive.newbox("box",1,1,1) -- the l3d for our box
	 box.geom.l3d:linkinterface(box) -- link it
	 cam = actornode.new("cam",5,4,3) -- our cameranode so we can view
	 l3dcamera.default():linkinterface(cam) -- link it with defaultcamera
	 cam:lookat(0,0,0,0,0,1) -- and look at 0,0,0 (the box location)

	 highlight = actornode.new("highlight") -- a simple highlighter
	 highlight.l3d = l3dprimitive.newsphere("hightlight",0.05)
	 highlight.l3d:color(1,0,0,1) -- make sphere red
	 highlight.l3d:linkinterface(highlight) -- link it

	 local function test () -- our testing function
	 	 local g,pos = UtilFunctions.picktest(space)  -- test it
	 	 highlight.l3d:rfNodraw(not pos) -- don't highlight if no hit

	 	 if pos then  -- if we have a hit
	     highlight:pos(pos[1],pos[2],pos[3]) -- move the highlighter
	     g.l3d:color(1,math.sin(os.clock()*5)/2+.5,1,1)
	     	-- ^^ make the hit object do something
	 	 end
	 end

	 Timer.set("picker", test,20) -- run the test every 20ms
	]])
local pickray = dgeomray.new(1)
local pickcoll = dcollresult.new()

function UtilFunctions.picktest(space,mx,my,maxlength,camera)
	------------------------
	-- setup the test ray:--
	------------------------
	-- compute origin as point
	camera = camera or l3dcamera.default()
	local mousex,mousey = input.mousepos()

	maxlength = maxlength or 1000000 
	if camera:fov()<0 then
		local w,h = window.refsize()
		local s = -camera:fov()
		local a = camera:aspect()
		if a<0 then a = w/h end
		local mx,my = (mx or mousex)/w - .5,((my or mousey)-h*.5)/w
		local x,y,z = camera:worldpos()
		local rx,ry,rz, vx,vy,vz, ux,uy,uz = camera:worldrotaxis()
		local function sc (a,b,c,s) return a*s,b*s,c*s end

		rx,ry,rz = sc(rx,ry,rz, mx*s)
		ux,uy,uz = sc(ux,uy,uz, -my*s)
		x,y,z = x + rx + ux,y + ry + uy,z + rz + uz
		--print(mx,my)
		pickray:set(x,y,z,vx,vy,vz)
	else
		local x,y,z = camera.default():worldpos()
		local tx,ty,tz = camera:toworld(mx or mousex or 0,my or mousey or 0,1)
		-- compute direction as unit vector
		local dx,dy,dz = tx-x,ty-y,tz-z
		local d = (dx*dx+dy*dy+dz*dz)^0.5
		dx,dy,dz = dx/d,dy/d,dz/d
		-- set ray values
		pickray:set(x,y,z,dx,dy,dz)
	end
	pickray:length(maxlength)
	---------------------
	-- make coll. test --
	---------------------
	local x,y,z = camera:worldpos()
	local cnt = pickcoll:test(pickray,space)

	-- prefix for
	if (cnt>0) then -- a hit is produced
		local nearest,d
		for i = 1,cnt do
			local px,py,pz = pickcoll:pos(i-1)
			local dx,dy,dz = x-px,y-py,z-pz
			local dist = dx*dx+dy*dy+dz*dz
			if not d or d>dist then
			nearest = i
			d = dist
			end
		end
		local i = nearest-1
		local g1,g2 = pickcoll:geoms(i)
		local p = {pickcoll:pos(i)}
		local n = {pickcoll:normal(i)}

		return (g1 == pickray and g2 or g1),p,n
	end
end


dofile("serializer.lua")


LuxEventCallbacks = {}
LuxEventCallbacks.windowresize = {}

function luxinia.windowresized()
	for i,f in pairs(LuxEventCallbacks.windowresize) do
		f()
	end
end

LuxModule.registerclassfunction("addWindowResizeListener",
	[[():(function) - will call function after window was resized (directly after window.update or with a few milliseconds delay if thru window.resizemode > 0.]])
function UtilFunctions.addWindowResizeListener(func)
	LuxEventCallbacks.windowresize[func] = func
end

LuxModule.registerclassfunction("removeWindowResizeListener",
	[[():(function) - removes the given function from being called on window resize events]])
function UtilFunctions.removeWindowResizeListener(func)
	LuxEventCallbacks.windowresize[func] = nil
end


LuxModule.registerclassfunction("checkargtable",
[[(table):(table) - This function provides a system to check table keys
for values. This is useful if a functions is taking a table containing 
various "named" function arguments. The return value of this function is a
table which provides sophisticated functionalities. If the table is
called with a string as argument, it expects that a key in the table
is to be checked and returns a table with possible value checking functions:

* type: checks if the value of this key is of a certain type
* optional: if the value is nil, the passed optional value is returned (may be nil)
* translate: expects an table to be passed. That table contains values that substitue the value of the key. A 2nd optional value is used if no value was found 
* oneofthis: like translate, but doesn't substitute the original value

If the field "finish" is called, the collected arguments will be returned in the order
they've been checked

  local x,y,vx,vy,type = UtilFunctions.checkargtable(info) 
    "x":type "number" 
    "y":type "number"
    "vx":optional(0)
    "vy":optional(0)
    "type":oneofthis{["player"]=true, ["AI"]=true}
    :finish()

]])
function UtilFunctions.checkargtable (info)
  local ret = {}
  local handler = {}
  local mt = {}
  local nret = 0
  setmetatable(handler,mt)
  function mt:__call(name)
    local checking = {}
    
    -- using colon for cosmetical reason only, though could be useful later...
    function checking:type (whichtype)
      if type(info[name])~=whichtype then
        error("Invalid type for argument "..name..": "..type(info[name]),3)
      end
      nret = nret + 1
      ret[nret] = info[name]
      return handler
    end
    
    function checking:oneofthis (tab, opt)
      if (info[name]==nil and not opt) or not tab[info[name] or opt] then
      	error("Invalid value for argument "..name..": none of the expected possible values")
      end
      nret = nret + 1
      ret[nret] = info[name] or opt
      return handler
    end
    
    function checking:translate (tab,opt)
      if (info[name]==nil and not opt) or tab[info[name] or opt]==nil then
      	error("Invalid value for argument "..name..": Could not be translated")
      end
      
      nret = nret + 1
      ret[nret] = tab[info[name] or opt]
      return handler
    end
    
    function checking:optional(value)
   	  nret = nret + 1
      if info[name]==nil then
      	ret[nret] = value
      else
      	ret[nret] = info[name]
      end
      return handler
    end
    
    return checking
  end
  
  function handler:finish()
  	return unpack(ret,1,nret) -- I am done
  end
    
  return handler
end
