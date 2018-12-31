-- l3dlevelmodel for large geometry

-- <Resources>
-- models/chiropteradm.f3d

-- this demo
-- The level is converted with the luxf3d tool and loaded as 
-- l3dlevelmodel, which generates an octree from it and applies
-- the lightmaps. 
--
-- Authors of the level (textures were removed):
-- Alcatraz (http://www.planetquake.com/bighouse), 
-- Nunuk ( http://www.planetquake.com/nunuk), 
-- Sock (http://www.planetquake.com/simland)]]

-- change the background color to our needs
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.7,0.7,0.7,0)

data = {}
data.nodes = {}

local function addlevel(name,lmscale,x)
	-- This function will load a model and create a scenenode
	-- and l3dlevelmodel for it. It will also apply lightmap
	-- color scaling.
	
	-- First we load the model without animation, and normals, and 
	-- without "bigvertex", as lighting is done via lightmap.
	
	-- We set last parameter to true (nodisplaylist) to turn GL 
	-- generation for displaylists off. Because displaylists
	-- cannot be used with levelmodels.
	-- Normally a mesh is rendered via VBO (if hardware allows),
	-- as that is optimal for todays cards. Older (especially
	-- nvidia cards) still have good performance for displaylists.
	-- Displaylists aren't as flexible as regular vertex arrays
	-- (VA), but faster, whilst VBO give benefits of both worlds.
	
	local mdl = model.load(name,false,false,false,true)
	
	-- I experimented with some values and 512 units size of 
	-- a octree cube seems good for performance. 
	-- Levelmodels are split into chunks for each octree box.
	-- Later when rendering only the content of the visible
	-- boxxes is drawn. This is not as polygon saving
	-- as classic bsp, which quake used, but less CPU expensive,
	-- and fast enough for GeForce2+ generation cards.
	local cubesize = 512
	
	-- we get dimensions of level
	local minx,miny,minz,maxx,maxy,maxz = mdl:bbox()
	sizex,sizey,sizez = maxx-minx,maxy-miny,maxz-minz
	-- and now find out how many octree cells we want to make for 
	-- each dimension (at least always 1)
	sizex,sizey,sizez = math.max(1,math.floor(sizex/cubesize)),
						math.max(1,math.floor(sizey/cubesize)),
						math.max(1,math.floor(sizez/cubesize))
	
	-- create the l3dlevelmodel and link it with a scenenode
	local l3d = l3dlevelmodel.new(name,mdl,sizex,sizey,sizez)
	local scn = scenenode.new(name,true,-minx +  (x or 0),0,0)
	l3d:linkinterface(scn)
	
	-- l3dlevelmodel will search the materialstring of each mesh
	-- for "lightmap_<custom name>;", and then load lightmap  
	-- textures accordingly. 
	
	-- Excurse:
	-- In luxinia it is legal to have additional
	-- keywords in the materialstring, which are ignored.
	-- E.g texture.load("lightmap_mylevel0.tga;ground/gravel.jpg")
	-- will load "ground/gravel.jpg" and ignore the ; preceding
	-- text. This text can contain custom keywords, however
	-- the engine interprets a few automatically, like
	-- "normalize" and "nocompress". If the texture name
	-- contains the word "lightmap", it will be automatically set
	-- to "nocompress".
	--
	-- l3dmodel will check if this string starts with !
	-- and then search for renderflag keywords, and will
	-- set them accordingly. E.g for invisble collision meshes
	-- you can give the mesh a materialstring of "!rfNodraw;".
	
	-- Lightmaps can be scaled (1x 2x or 4x) to create a more 
	-- color rich environment. We go thru all lightmaps of the 
	-- l3dlevelmodel and set the texture's lightmapscale.
	-- This scale will be used auotmatically whenever a texture 
	-- is used as lightmap.
	
	for i=0,l3d:lightmapcount()-1 do
		local tex = l3d:lightmap(i)
		tex:lightmapscale(lmscale or 4)
	end
	
	-- prevent garbage collection and store them
	table.insert(data.nodes,{l3d,scn})
	
	return (maxx-minx)
end

-- call the function and load the level
addlevel("chiropteradm.f3d",4,0)

-- Be aware because the default texture of luxinia is blueish
-- The level will look blueish as well, as the base textures
-- the level normally would have, are not found.


-- setup parameters for camera
cam = l3dcamera.default()
cam:fov(90)
cam:frontplane(16)
cam:backplane(2048)

camact = actornode.new("cam")
cam:linkinterface(camact)


local ctrl = CameraEgoMKCtrl:new(cam,camact,
				{shiftmulti = 5,movemulti = 10.0})

--local root = Container.getRootContainer()
--root:addMouseListener(ctrl:createML(root))

MouseCursor.showMouse(false)

Timer.set("tutorial",function()
	ctrl:runThink()
end)

------------------
-- finally create a little gui with information about the map

dofile("Shared/htmllabels.lua")
local ww,wh = window.refsize()

helpframe = Container.getRootContainer():add(
				TitleFrame:new(0,wh-230,280,230,nil,"Information"))
helpframe:getSkin():setSkinColor(1,1,1,0.8)

doc = helpframe:add(MultiLineLabel:new(
		10,26,helpframe:getWidth()-20,helpframe:getHeight()-30,
		HLgetdocpage("T028/q3test_index.html",title),true))
--doc.zoneAction = HLaction(title,actions)

-- layout management
local layoutermax = GH.makeLayoutFunc(Container.getRootContainer():getBounds(),helpframe:getBounds(),
	{bottom=true,	relpos={false,false},	relscale={false,false}})
local layoutermin = GH.makeLayoutFunc(Container.getRootContainer():getBounds(),{0,wh-24,280,24},
	{bottom=true,	relpos={false,false},	relscale={false,false}})
	
	
local minbtn = GH.frameminmax(helpframe,layoutermin,layoutermax,nil,true)

function helpframe:onParentLayout(nx,ny,nw,nh)
	local layouter = minbtn.oldcomponents and layoutermin or layoutermax
	self:setBounds(layouter(nx,ny,nw,nh))
end


local root = Container.getRootContainer()
function root:doLayout()
	local nw,nh = self:getSize()
	for i,c in ipairs(self.components) do
		if (c.onParentLayout) then
			c:onParentLayout(0,0,nw,nh)
		end
	end
end


