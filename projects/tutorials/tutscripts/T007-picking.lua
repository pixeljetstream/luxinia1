-- Object picking

-- <Resources>
-- models/t33.f3d

-- basic scene
view = UtilFunctions.simplerenderqueue()
view.rClear:colorvalue(0.0,0.0,0.0,0) 

actor = actornode.new("t33",0,10,0)
actor:rotdeg(40,-20,40)
local t33 = model.load("t33.f3d")
actor.l3d = l3dmodel.new("t33",t33)
actor.l3d:linkinterface(actor)

-- picking collision
pickspace = dspacehash.new() -- our space containing 
 -- all pickable geometry
local coldata = UtilFunctions.getModelColMesh(t33).data
 -- the t33 is the model and we store the coldata
 -- temporarily. The utilfunction extracts
 -- everything we need.
 
 -- Create a dgeomtrimesh from the data, so we
 -- can make hit tests with it. dgeomtrimesh prevents
 -- garbage collection of coldata.
 
actor.geom = dgeomtrimesh.new(coldata,pickspace)
actor.geom:pos(actor:pos()) -- we need to transform
  -- the geom in the same way as the actor does!
actor.geom:rotaxis(actor:rotaxis())
actor.geom.identifier = "Hello World" -- an identificator

input.showmouse(true)

pickresult = actornode.new("pickres")
pickresult.l3d = l3dprimitive.newcylinder("pick",.03,.03,1)
pickresult.l3d:linkinterface(pickresult)
pickresult.l3d:color(1,0,0,1)

function picking ()
  local geom, pos,norm = UtilFunctions.picktest(pickspace)
   -- if not passed, it will automaticly use the 
   -- default camera and the mouse coordinates
  if geom then  -- we've had a hit
    pickresult:pos(unpack(pos))
    pickresult:lookat(pos[1]+norm[1],pos[2]+norm[2],pos[3]+norm[3],
      0,0,1,2) -- let the cylinder look into the direction
     -- of the normal. The last argument is telling that
     -- the Z axis should be used for orientation
    print(geom.identifier) -- print the identifier of 
     -- the picked object
  end
end

Timer.set("tutorial",picking,20)