-- Frustum check spatialnodes

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1.0,1.0,1,0) 

function buildscene (adder) -- constructs a scene, 
	-- independent from the used scenegraphmodel
	adder(l3dprimitive.newsphere("sp",1,1,1),0,0,0,0,0,0)
	adder(l3dprimitive.newbox("sp",5,1.5,.5),6,0,0,0,0,0)
	adder(l3dprimitive.newcylinder("sp",1,1.,5),-6,0,0,20,40,0)
end


nodes = {} -- a list of all nodes, prevents garbage collection

function scenenodeadder (l3d,x,y,z,rx,ry,rz)
	-- provides functionality to use the scenenode system
	local node = scenenode.new("node")
	
	node:localpos(x,y,z)
	node:localrotdeg(rx,ry,rz)
	node.l3d = l3d
	l3d:linkinterface(node)
	l3d:rfLitSun(true)
	
	nodes[#nodes+1] = node
end

function actornodeadder(l3d,x,y,z,rx,ry,rz)
	-- provides functionality to use the actornode system
	local node = actornode.new("node")
	
	node:pos(x,y,z)
	node:rotdeg(rx,ry,rz)
	node.l3d = l3d
	l3d:linkinterface(node)
	l3d:rfLitSun(true)
	
	nodes[#nodes+1] = node
end 

-- use either one or another system to create the same scene:
buildscene(actornodeadder)
--buildscene(scenenodeadder) 

--- some scene setup for our camera / lighting
cam = actornode.new("cam")
l3dcamera.default():linkinterface(cam)
cam:pos(10,10,35)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,200)
sun.light = l3dlight.new("sun")
sun.light:makesun()
sun.light:linkinterface(sun)

render.drawvisspace(1)

-- only for cleanup stuff used in the tutorial framework
return function() render.drawvisspace(0) end