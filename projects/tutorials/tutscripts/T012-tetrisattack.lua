-- Tetris Attack clone

-- <Resources>
-- models/selector.f3d
-- models/box.f3d

view = UtilFunctions.simplerenderqueue () -- default start
view.rClear:colorvalue(0.0,0.0,0.0,0)  -- black background

game = { -- our gameplay structure
  field = { -- the field
    w = 6, -- width of the field
    h = 14 -- height of the field
  },
  colors = { -- our colors that the blocks may have
    {1,0,0,1}, -- red
    {0,1,0,1}, -- green
    {0,0,1,1}, -- blue
    {1,1,0,1}, -- yellow
    {.75,0,1,1}, -- violet
    {1,.5,0,1}, -- orange
    --{0,1,.75,1}, -- ultra difficult mode :)
  },
  cursor = actornode.new("cursor"), -- our cursor
  mincolors = 3, -- how many colors have to be in a row
  cam = actornode.new("camera"), -- our camera actor
  sun = actornode.new("sun"), -- our sun actor
  blockspeed = 0.15, -- how fast the blocks will exchange
  yspeed = 0.004, -- the vertical moving speed
  keyrepeattime = 175 -- how long to pause after the 
   -- key was pressed before "pressing" it again
}

l3dcamera.default():linkinterface(game.cam) -- link the cam
l3dcamera.default():fov(-15) -- negative values for the fov
 -- result in a orthogonal projection

game.cam:pos(-1,1,5) -- position the camera
game.cam:rotdeg(-100,-10,0) -- set the right angle
game.cursor.mdl = -- our cursor model and ...
  model.load("selector.f3d",false,true,false,true)
    -- ... don't let it use display lists, as we might 
    -- change the colors afterwards
game.cursor.l3d = l3dmodel.new("cursor",game.cursor.mdl)
  -- create the model
game.cursor.l3d:linkinterface(game.cursor) -- link it
game.cursor.l3d:rfLitSun(true) -- let it be lit
game.cursor:pos(.5,0,0) -- set the initial position

game.sun.light = l3dlight.new("sun") -- our light source
game.sun.light:linkinterface(game.sun) -- link it
game.sun.light:makesun() -- make it our sun light
game.sun:pos(1000,1000,2000) -- position it somewhere
game.sun.light:ambient(.3,.3,.3,1) -- a bit of ambient 
 -- lightning, so not everything becomes pitch black


local boxmdl = model.load ("box.f3d",false,true,false,true)
  -- the boxmodel that we want to use
  -- if the boxmodel is loaded as a display list, we cannot
  -- change the color of the block, so we need to prevent 
  -- this

  -- iterating now over all elements (w*h)
for i=1,game.field.w*game.field.h do 
  local box = actornode.new("block") -- our box
  function box:randomcolor(encolor) -- a function that 
    -- will set a new random color for this block
    box.colorid = math.random(#game.colors) -- a random color
    box.origcolorid = box.colorid -- a backup reference - 
     -- we will modify the colorid variable later
    if encolor then -- if the argument was true 
      box.l3d:color(unpack(game.colors[box.colorid])) 
        -- set the colors of the block
    else
      box.l3d:color(0,0,0,1) -- otherwise, intialize it with
        -- a black color
    end
    box.l3d:rfNodraw(false) -- force it to be drawn 
      --(important for later)
  end
  
  box.l3d = l3dmodel.new("block",boxmdl) -- our box model
  box.l3d:rfNovertexcolor(true) -- don't use the vertex 
    -- colors of the model, instead use our own colors
  box.l3d:linkinterface(box) -- link it
  box.l3d:rfLitSun(true) -- let it be lit
  box:randomcolor(true) -- and colorize it now 
  local x,y = -- calculate the coordinates now
    (i-1)%game.field.w-game.field.w/2, -- we center the field
    math.floor((i-1)/game.field.w)-game.field.h/2 -- around
      -- 0,0, that's why we shift it 
  box:pos(x,y,0) -- setting the initial position now
  game.field[i] = box -- assign the box to our field, using
    -- the index
end



-- the gravity handling
function game.field.gravity ()
  for x=1,game.field.w do -- iterate ove each ROW ...
    for y=1,game.field.h-1 do -- ... upwards
      local i = x+(y-1)*game.field.w 
      local i2 = x+y*game.field.w
      -- i: index of element 
      -- i: index of element above
        
      if game.field[i].colorid<0 then -- block it is 'empty'
        game.field[i2].onground = false -- above cannot be 
          -- on its ground
        game.field[i],game.field[i2] =  -- swap above 
          game.field[i2],game.field[i] -- and below
          -- (this is causing a constant swapping of 
          -- free elements, swapping is just as expensive
          -- as an additional check if swapping is required
          -- so this is kinda senseless to check
      elseif game.field[i].onground or y==1 -- our element
      then  -- is on ground, or its the bottom line
        if game.field[i2].inposition then -- if element 
        -- above is in position ...
          game.field[i2].onground = true -- it is also on the
           -- ground
        end
      end
    end
  end
end

-- solving blocks
function game.field.solve ()
  -- a few local functions, which keep our scanned results
  local colcount,colid = 0,nil -- colorcound and colorid
    -- colorid: current color in the row
  local remlist,rowlist = {},{} -- removelist and rowlist
  local invalidatelist = {} -- list of elments that 
  --need to be invalidated
  -- the rowlist is the list of scanned elements that 
  -- are of the same color
  
  local function startrow()
    -- starting a new row will need us to reset some vars
    colcount,colid = 0,nil -- no colors counted so far
    remlist,rowlist = {},{} --empty remove and rowlist
  end
  
    -- once we have finished a scanline or another color 
    -- starts, we check if we can make the blocks vanish
  local function finishappend()
    if rowlist and colcount >= game.mincolors 
    then -- we have a rowlist and there are more then 3 
     -- in a row
      for i,v in ipairs(rowlist) do  -- insert the elements
        table.insert(remlist,v)-- of the row in the removelist
      end
    end
  end
  
  -- test if we can append a block to our row
  -- which can be done if has the same color
  local function appendtest (x,y)
    local i = x+(y-1)*game.field.w -- calculate the index
    local color = game.field[i].colorid -- this is the color
    if rowlist and colid==color and color>0 and -- we have
      game.field[i].inposition and -- a row and color, it is
      game.field[i].onground -- inposition and on the ground
    then 
      colcount = colcount + 1 -- append it to the row
      table.insert(rowlist,i)
    else -- otherwise, we need to check if a row has been
      finishappend() -- finished
      if game.field[i].onground and 
        game.field[i].inposition 
      then -- it is a valid block to start a new list
        rowlist = {i}
      else
        rowlist = nil -- or it is not
      end
      colid = color -- anyway, set the color id
      colcount = 1 -- and set the elementcount to 1
    end
  end
  
  -- check now the elements to be removed
  local function check()
    for i,v in ipairs(remlist) do
      game.field[v].vanishanim = 0 -- init vanishanim
       -- (which starts the pushback/fadeout animation)
      table.insert(invalidatelist,game.field[v]) 
        -- we cannot change the colorid now, we need to 
        -- wait till the end
    end
  end
  
  -- up to down check
  for x=1,game.field.w do
    startrow() -- start a new row
    for y=2,game.field.h do
      appendtest(x,y) -- append if possible
    end
    finishappend()-- finish the row
    check() -- and check out the remlist list 
  end
  -- left to right check
  for y=2,game.field.h do
    startrow() -- similiar to the loop before
    for x=1,game.field.w do
      appendtest(x,y)
    end
    finishappend()
    check()
  end
  
  for i,v in ipairs(invalidatelist) do
    v.colorid = -1 -- invalidate all removed blocks
  end
end


local fieldy = 0 -- our vertical movement variable
function game.think () -- our think method
  local dontmove = false -- we won't move if top is reached
  for x=1,game.field.w do -- let's see if this is the case
    local i = x + (game.field.h-1) * game.field.w 
      -- ^^ top row index
    if game.field[i].colorid>0 then 
      dontmove = true -- we set colorid to a negative value
       -- if the block is vanished, if this is not negative,
       -- we know that the top is reached by a block
      break -- it won't become more true than that
    end
  end
  
    -- if we are moving it up now, let's add the 
    -- speed variable or don't, if we don't move
  fieldy = fieldy + (dontmove and 0 or game.yspeed)
  
  -- We want the first row to fade in - fieldy is a 
  -- value between 0 and 1 and if it is 1, the next 
  -- row will start. So we just modulate the color with 
  -- fieldy
  for x=1,game.field.w do
    local box = game.field[x] -- our box in first row
    local r,g,b,a = unpack(game.colors[box.colorid])
      -- ^^ desired color of the block
    r,g,b = r*fieldy,g*fieldy,b*fieldy  -- modulated (darker)
    box.l3d:color(r,g,b,a) -- set it
  end
  
  
  game.field.solve() -- a function that seeks rows that 
    -- can vanish (solved)
  game.field.gravity() -- let blocks drop down if they 
    -- float in the air
  
  -- insert top row in bottom row
  if fieldy >= 1 then
    fieldy = fieldy - 1
    for i=1,game.field.w do 
      local o = table.remove(game.field)
      table.insert(game.field,1,o)
      local px,py,pz = o:pos()  
      o:randomcolor(false) -- set a new random color and 
        -- make it black
      o:pos(-i+game.field.w/2,-game.field.h/2,0) 
        -- set the new position
      o.vanishanim = nil
       -- our vanishanimation variable needs to be reseted
    end
  end
  
  local t = system.time() -- current time in milliseconds
  local kd = Keyboard.isKeyDown -- short access to keypress
  local mx =  -- our x movement for the cursor
    -((kd "left" and 1 or 0) + (kd "right" and -1 or 0))
  local my =  -- our y movement for the cursor
    ((kd "up"   and 1 or 0) + (kd "down"  and -1 or 0))
  local cx,cy = mx,my -- we will correct the movement ...
  local lmxt,lmyt = t,t -- based on the last time we pressed
   -- the keys
  if game.cursor.lastmx == mx and -- same movement as before
      t-game.cursor.lastmxt<game.keyrepeattime 
  then -- we have pressed the keys just a few millis ago
    cx = 0 -- set our x movemnt to 0
    lmxt = game.cursor.lastmxt -- and don't change time 
     -- when we made the press
  end
  if game.cursor.lastmy == my and -- same movement as before
      t-game.cursor.lastmyt<game.keyrepeattime  
  then -- the same as for y
    cy = 0
    lmyt = game.cursor.lastmyt
  end
  
   -- remember now what the user pressed in this frame
  game.cursor.lastmx, game.cursor.lastmy = mx,my
  game.cursor.lastmxt,game.cursor.lastmyt = lmxt,lmyt
  
  if kd "SPACE" and (not game.cursor.lastswitch or 
    t-game.cursor.lastswitch>game.keyrepeattime) 
  then -- after checking the time again ...
    local x,y = game.cursor:pos() -- current position
    x,y =  -- we need a x/y coordinate that is integer
      x + game.field.w/2+.5,  
      math.ceil(y + game.field.h/2 - fieldy -.5)
    local i = x + y*game.field.w -- lets calculate our 
      -- index in the field
    local a,b = i,i+1 -- we swap a and b now
    game.field[a],game.field[b] = -- swapping in lua is 
      game.field[b],game.field[a] -- really easy :D
    game.cursor.lastswitch = t -- remember not 
     -- to swap again for some time
  end
  
  -- a small helper function to let the value be between
  -- a given minimum and maximum value  
  local function mima (min,max,val)
    return math.min(max,math.max(min,val))
  end
  local x,y = game.cursor:pos()
  local x,y = -- limit the position of the cursor horizontal
    mima(-game.field.w/2+.5,game.field.w/2-1.5,x+cx),
    mima(-game.field.h/2+fieldy,game.field.h/2-1 + 
      fieldy,y+cy+game.yspeed) --and vertical
    
    -- set the cursor position and make sure y to be inline
  game.cursor:pos(x,math.ceil(y-.5-fieldy)+fieldy,0)

  -- for each block ...  
  for i,box in ipairs(game.field) do
    -- calculate the (desired) position, based on the index
    local x,y = (i-1)%game.field.w-game.field.w/2,
      math.floor((i-1)/game.field.w)-game.field.h/2 + fieldy
    local px,py,pz = box:pos() -- current position
    local dx,dy = x-px,y-py -- difference to the desired pos
    
      -- if our block is near its desired location, it is
      -- inposition
    game.field[i].inposition = dx*dx+dy*dy<.01
    
     -- move the block, but only with constant velocity
    local mx,my = mima(-game.blockspeed,game.blockspeed,dx),
      mima(-game.blockspeed,game.blockspeed,dy)
    dz = dx -- if the block moves to the right, make it 
     -- move to the front, if it moves to the left, it 
     -- moves behind - this simulates the "rotation"
    
    if box.vanishanim then -- if the box is vanishing
      if box.vanishanim>1 then -- we don't need to draw it
        box.l3d:rfNodraw(true) -- if it is finished
      else -- otherwise, simulate the movement to the back
        -- and modulate the color to black
        box.vanishanim = box.vanishanim +.02 -- step forward
         -- in animation
        box:pos(px,py,-box.vanishanim*2) -- move it behind
        local r,g,b,a = unpack(game.colors[box.origcolorid])
        local sub = box.vanishanim -- modulate colors
        r,g,b = math.max(0,r-sub),math.max(0,g-sub),
          math.max(0,b-sub)
        box.l3d:color(r,g,b,a) -- and set the color
      end
    else -- otherwise, we just move towards our 
      game.field[i]:pos(px+mx, -- desired location
        py+(math.abs(dx)<.1 and my or 0),dz) 
    end
  end
end

Timer.set("tutorial",game.think,20)
