-- Frustum check l3dnode parent

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1.0,1.0,1,0) 

snode = scenenode.new("sn") -- just create a new one

function l3d(l3d,x,y,z)
	l3d:rfLitSun(true)
	l3d:localpos(x or 0, y or 0, z or 0)
	return l3d
end

snode.l3d = l3d(l3dprimitive.newsphere("s1",1))
snode.l3d:linkinterface(snode)

snode.l3d.box = l3d(l3dprimitive.newbox("b1",.5,3,1),0,1.5,0)
snode.l3d.box:parent(snode.l3d)

snode.l3d.box.cyl = l3d(l3dprimitive.newcylinder("c1",.5,.5,5),0,1.5,0)
snode.l3d.box.cyl:parent(snode.l3d.box)

snode.l3d.box.box = l3d(l3dprimitive.newbox("b2",3,.5,.5),1.5,0,0)
snode.l3d.box.box:parent(snode.l3d.box)

snode:vistestbbox(-1,-1,-2.5,3,3.5,2.5)

--- some scene setup for our camera / lighting
cam = actornode.new("cam")
l3dcamera.default():linkinterface(cam)
cam:pos(10,10,35)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,200)
sun.light = l3dlight.new("sun")
sun.light:makesun()
sun.light:linkinterface(sun)

render.drawvisspace(2)

-- only for cleanup stuff used in the tutorial framework
return function() render.drawvisspace(0) end