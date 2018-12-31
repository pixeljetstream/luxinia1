-- Dynamic mesh generation

-- In this tutorial we create a mesh as part of a custom
-- model. We could also use a rendermesh (introduced in
-- later versions of luxinia).

-- initialization and light / camera setup
window.refsize(window.width(),window.height())

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1,1,1,0) 

cam = actornode.new("cam",6,3,2)
cam.camera = l3dcamera.default()
cam.camera:linkinterface(cam)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,500)
sun.light = l3dlight.new("sun")
sun.light:linkinterface(sun)
sun.light:makesun()


-- all resource create functions behave similar as to load:
-- they first look for the name identifier being loaded already,
-- and return it when found, or they execute the creation process.

-- modelname and number of quads

name,n = "test",32 -- the modelname should be unique!
-- we want to have colors and normals
mdl = model.create(name,vertextype.vertex32normals(),1,0)
mesh = mdl:meshid(0) -- retrieve the mesh we work with

-- initialize the mesh with the number of required vertices
mesh:init("",n*2,n*2)
mesh:indexPrimitivetype(primitivetype.quadstrip()) -- make it a quadstrip
mesh:vertexCount(n*2) -- set the vertexcount
mesh:indexCount(n*2) -- set the indexcount

for i=0,n-1 do -- initial tube mesh
	local angle = math.pi/(n-1)*i*2 -- current angle
	local y,z = math.sin(angle),math.cos(angle) 
	mesh:vertexPos(i*2, -1,y,z) -- setting the position
	mesh:vertexPos(i*2+1,1,y,z) -- ... for the front point
	
	mesh:vertexColor(i*2,1,0,0,1) -- set the colors
	mesh:vertexColor(i*2+1,1,1,0,1) -- ...
	
	mesh:vertexNormal(i*2,0,y,z) -- and now the normal very
	mesh:vertexNormal(i*2+1,0,y,z) -- easy for a unitsized tube
end
for i=0,n*2-1 do
	mesh:indexValue(i,i) -- tell how the quadstrip runs
	 -- over our vertices (really easy for such a quadstrip
end
 
mesh:indexMinmax() -- let the application determine the 
 -- used vertices

mdl:createfinish(false,true,true) -- let's finalize the model
 -- the additional parameters allow us to modify it dynamically
mdl:updatebbox () -- updating the axisaligned boundingbox

actor = actornode.new("",0,0,0) -- create an actor so we 
l3d = l3dmodel.new("",mdl) -- ... can attach an model to it
l3d:linkinterface(actor)
l3d:rfLitSun(true) -- let it be lit 

text = Container.getRootContainer():add(
	Label:new(5,5,120,80,"")) -- a GUI element to show the 
	 -- display states


local t = 0 -- current time

function text:updateinfo()
	-- update the information text
	self:setText(("Dynamic Model animation Demo\n"..
		"[1] Wireframe: %s\n"..
		"[2] Draw normals: %s\n"..
		"[3] No culling: %s\n"..
		"[4] Lit: %s\nTime: %.2f"):format(
		l3d:rfWireframe() and "yes" or "no", 
		render.drawnormals() and "yes" or "no",
		l3d:rfNocull() and "yes" or "no",
		l3d:rfLitSun() and "yes" or "no",t))
end

-- a few keybindings so we can interactivly change the 
-- way our model is displayed
Keyboard.setKeyBinding("1",
	function ()l3d:rfWireframe(not l3d:rfWireframe()) end)
Keyboard.setKeyBinding("2",
	function ()render.drawnormals(not render.drawnormals()) end)
Keyboard.setKeyBinding("3",
	function ()l3d:rfNocull(not l3d:rfNocull()) end)
Keyboard.setKeyBinding("4",
	function ()l3d:rfLitSun(not l3d:rfLitSun()) end)

	-- a timer that modifies the mesh
Timer.set("tutorial",
	function()
		text:updateinfo()
		t = t + .01
		for i=0,n-1 do
			local angle = math.pi/(n-1)*i*2+t
			local y,z = math.sin(angle),math.cos(angle)
			mesh:vertexPos(i*2, -1,y,z)
			mesh:vertexNormal(i*2, -1,y,z)
		end
	end,20)


  