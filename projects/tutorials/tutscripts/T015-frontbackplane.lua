-- Camera clipping planes

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(1.0,1.0,1,0) 


box = actornode.new("box",0,0,0)
box.l3d = l3dprimitive.newbox("box",1.5,1.5,1.5)
box.l3d:linkinterface(box)
box.l3d:rfLitSun(true)
box.l3d:color(1,0,0,1)

box2 = actornode.new("box",0,0,0)
box2.l3d = l3dprimitive.newbox("box",1.0,2.5,1.5)
box2.l3d:linkinterface(box2)
box2.l3d:rfLitSun(true)
box2.l3d:color(1,1,0,1)

--sphere = actornode.new("sphere",0,-.5,0)
--sphere.l3d = l3dprimitive.newsphere("sphere",1,1,1)
--sphere.l3d:linkinterface(sphere)
--sphere.l3d:rfLitSun(true)



--- some scene setup for our camera / lighting
cam = actornode.new("cam")
l3dcamera.default():linkinterface(cam)
l3dcamera.default():backplane(15.5)
l3dcamera.default():frontplane(14.5)
cam:pos(10,10,5)
cam:lookat(0,0,0,0,0,1)

sun = actornode.new("sun",100,40,200)
sun.light = l3dlight.new("sun")
sun.light:makesun()
sun.light:linkinterface(sun)