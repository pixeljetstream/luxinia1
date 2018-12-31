-- Loading and showing a .f3d model

-- <Resources>
-- models/t33.f3d

view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

actor = actornode.new("t33",0,10,0)
actor:rotdeg(40,-20,40)
t33 = model.load("t33.f3d")
actor.l3d = l3dmodel.new("t33",t33)
actor.l3d:linkinterface(actor)