-- Convolution PostFX
--
-- http://www.codeproject.com/KB/GDI-plus/csharpfilters.aspx
-- A tutorial about C# filterin which describes some of the 
-- effects used here.
--
-- Effects are done via convolution filtering, which means
-- the output pixel is a weighted sum of surrounding pixels.
--
-- Smooth
-- Mean Removal
-- Sharpen
-- Edge Detect/Emboss
-- Custom

-- check if hardware is sufficient
if (not ( 	capability.fbo() and  
			capability.texrectangle() and
			 technique.arb_vf():supported_tex4())
	) then
	-- if not print some error message
	l2dlist.getrcmdclear():color(true)
	l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
	local lbl = Container.getRootContainer():add(
		Label:new(20,20,300,48,
		"Your hardware or driver is not capable of needed functionality."))

	return (function() lbl:delete() end)
end


--------
-- scene setup

-- a ready to use scene with sky rendering
-- and some models
skydata = dofile("T025/skyscene.lua")

-- prepare our view for rendering to texture

-- windowsized texture
local frametex = texture.createfullwindow("frame")
local framedepth = renderbuffer.new(textype.depth(),2,2,true)
-- windowsized fbo
local framefbo = renderfbo.new(2,2,true)

-- The commands will be added to the beginning of the
-- l3dview. As it's more easy to add "as first" or "last",
-- commands are stored in a array first and then
-- added "as first" in opposite order of the array.
-- That way they keep their order once added in the l3dview.

local cmds = {}

-- start with fbo bind
local rc = rcmdfbobind.new()
rc:setup(framefbo)
table.insert(cmds,rc)

-- assign texture
rc = rcmdfbotex.new()
rc:color(0,frametex)
table.insert(cmds,rc)

-- assign renderbuffer
rc = rcmdfborb.new()
rc:depth(framedepth)
table.insert(cmds,rc)

-- drawto
rc = rcmdfbodrawto.new()
table.insert(cmds,rc)

-- add the cmds to l3dview, in reversed order, because
-- when added as first their order will be back to normal
for i=#cmds,1,-1 do
	-- add as first
	skydata.bgview:rcmdadd(cmds[i],true)
end

-- finally unbind fbo
rc = rcmdfbooff.new()
skydata.view:rcmdadd(rc)

-- Collect all fbo related commands, so it's easier to
-- disable them.
table.insert(cmds,rc)


-- At the moment rendering is done just into the texture,
-- but we still want to make things visible to the screen,
-- so we draw a rectangle at the end.
-- This rectangle is also the one we change with the postfx-
-- filters. 

local postquad = rcmddrawmesh.new()
skydata.view:rcmdadd(postquad)

table.insert(cmds,postquad)

-- a final error check
fboerrtype,fboerrmsg = l3dlist.fbotest()

if (fboerrtype) then
	-- we will always get a warning message,
	-- about leaving the fbobind active
	-- for the first l3dview
	print(fboerrmsg)
	if (fboerrtype > 0) then
		-- revert
		skydata.view = 
			UtilFunctions.simplerenderqueue(skydata.view)
	end
end

-- let's make a function to disable fbo rendering easily
local function fbostate(state)
	for i,r in ipairs(cmds) do
		r:flag(0,state)
	end
end

---------
-- the filters
--
--
-- Convolution filtering is a weighted sum of pixel values
-- surrounding the original pixel. We will need two arrays:
-- one with the relative coordinates to the center pixel
-- and a second with weights for that pixels contribution
-- to the final output.
-- We use the FXUtils.applyMaterialTexFilter function,
-- to make use of image filtering shaders provided by
-- luxinia.

-- we create a function that creates pixel coordinates
local function getCoords(scale)
	-- We could directly write "scale" everywhere instead of 
	-- doing 1*scale later. But that way it's easier to edit
	scale = scale or 1
	
	local coords = {
	-- upper row
	{-1,1},{0,1},{1,1},
	-- middle row
	{-1,0},{0,0},{1,0},
	-- lower row
	{-1,-1},{0,-1},{1,-1},
	}
	
	-- scale them
	for i,p in ipairs(coords) do
		for n,c in ipairs(p) do
			p[n] = c*scale
		end
	end
	
	return coords
end

-- Another function that will compute the final output weights.
-- As FXUtils.applyMaterialTexFilter requires weights to be
-- vector4s and when describing the filters we dont want to write
-- so much, this function will expand one weight to all channels.
-- Furthermore it will guarantee weights sum to 1 or 0. When
-- weights sum to 0, we will need a color offset bias, which
-- is returned as well.

local function getColorWeightsBias(wtab) 
	local outtab = {}
	local sum = 0
	local wmax = 0
	for i,w in ipairs(wtab) do
		sum = sum + w
		wmax = math.max(wmax,w)
	end
	
	local div = (sum ~= 0) and 1/sum or 1 --1/wmax
	
	for i,w in ipairs(wtab) do
		w = w*div
		table.insert(outtab,{w,w,w,w})
	end
	
	return outtab, (sum == 0) and {0.5,0.5,0.5,0.5}
end



-- Now to the effects, we store their base weights and optional
-- bias. 
local effects = {}

-- Smooth with Gauss-coeff
effects[1] = {
	name = "Smooth-Guass",
	weights = {
	1,2,1,
	2,4,2,
	1,2,1
	}
}

-- Smooth Box-car
effects[2] = {
	name = "Smooth-Mean",
	weights = {
	1,1,1,
	1,1,1,
	1,1,1
	}
}

-- Mean Removal
effects[3] = {
	name = "Mean Removal",
	weights = {
	-1,-1,-1,
	-1,9,-1,
	-1,-1,-1
	}
}

-- Sharpen
effects[4] = {
	name = "Sharpen",
	weights = {
	0,-2,0,
	-2,11,-2,
	0,-2,0
	}
}

-- Edge Detect
effects[5] = {
	name = "Emboss",
	weights = {
	-1,-1,-1,
	-1, 8,-1,
	-1,-1,-1,
	},
}

-- Emboss
effects[6] = {
	name = "Emboss Laplacian",
	weights = {
	-1,0,-1,
	 0,4, 0,
	-1,0,-1,
	},
}

-- Custom
effects[7] = {
	name = "Custom",
	weights = {
	1,2,1,
	2,8,2,
	1,2,1
	}
}


-- The filter is applied to "postquad", which is a simple
-- screen filling rectangle. We use FXUtils helper class
-- and the functions we made before.
function applyeffect(eff,cscale)
	if (not eff) then return end
	FXUtils.applyMaterialTexFilter(postquad,frametex,
		getCoords(cscale), 
		getColorWeightsBias(eff.weights))
end


---------
-- gui for changing the filters
-- 
local ww,wh = window.refsize()
local w,h = 240,100
-- titleframe
local tframe = Container.getRootContainer():add(
		TitleFrame:new(0,wh-h,w,h,nil,"PostFX Filters"))
-- init some values we store in tframe
tframe.sldw = 1 -- colorweight slider value
tframe.sldc = 1 -- coordinate scale slider
tframe.eff = nil -- the current active effect

-- combobox
local cbox = tframe:add(ComboBox:new(10,30,w-20,20))

-- add "None" dummy, which disables fbos
cbox:addItem("None")
cbox:addItem("Pass Thru")

-- add all other filters

for n,eff in ipairs(effects) do
	cbox:addItem(eff.name,eff)
end

-- overload the function when a item is selected
function cbox:onSelected(idx,cap,eff)
	if (cap == "None") then
		fbostate(false)
	elseif(cap == "Pass Thru") then
		fbostate(true)
		postquad:matsurface(frametex)
		postquad:moTexmatrix(window.texmatrix())
	else
		fbostate(true)
		applyeffect(eff,tframe.sldc)
	end
	tframe.eff = eff
end

-- init gui to no filter
cbox:select("None")

-- slider for weight intensity

local y = 55
GH.sldLabelwidth = 60

-- slider for coord scale
y = GH.addslider (tframe,10,y,w-20,16,"Coords:",
	{callback = function(value) 
					tframe.sldc = value
					applyeffect(tframe.eff,tframe.sldc)
					return value
				end,
	initvalue=0,scale=3,offset=1,intmode=true,increment = 1/3})

-- Add a pop-up menu for entering weights manually
local function makecustom()
	local btnw = 40
	local btnh = 20
	local modal = GH.modaldialog("Custom Weights",nil,nil,nil,nil,btnw*3+20,btnh*3+50,true)
	-- make moveable
	modal:add(ContainerMover:new(0,0,btnw*3+20,24))
	
	local wts = effects[7].weights
	
	-- add weight textfields
	for i=1,9 do
		local x = (math.floor(math.mod(i-1,3)) * btnw) + 10
		local y = (math.floor((i-1)/3) * btnh) + 28
		
		local txt = modal:add(TextField:new(x,y,btnw,btnh))
		txt:setText(tostring(wts[i]),true)
		
		function txt.onTextChanged(self)
			local text = txt:getText()
			wts[i] = tonumber(text) or wts[i]
		end
		
		function txt.onAction()
			cbox:select("Custom")
		end
	end
	
	
	-- btns for apply and close
	local btnapply = modal:add(Button:new(10,btnh*3+30,50,16,"Apply"))
	local btnclose = modal:add(Button:new(btnw*3+20-10-50,btnh*3+30,50,16,"Close"))
	
	function btnapply:onClicked()
		cbox:select("Custom")
	end
	function btnclose:onClicked()
		modal:delete()
		tframe.custom = nil
	end
	
	return modal
end

-- button for setting custom weights
local btn = tframe:add(Button:new(10,y,100,20,"Set Custom"))
function btn.onClicked(self)
	if (not tframe.custom) then
		tframe.custom = makecustom()
	end
end

------
-- at the end add container with mouselistener for camera
-- controls

dofile("Shared/camcontrols.lua")
local ccframe = CCaddRot(skydata.camact)

-----
-- a little gui management
local root = Container.getRootContainer()

local layouter = GH.makeLayoutFunc(
	root:getBounds(),tframe:getBounds(),
	{bottom=true, relpos={false,false},	relscale={false,false}})
		
function tframe:onParentLayout(x,y,w,h)
	tframe:setBounds(layouter(x,y,w,h))
end


function root:doLayout()
	local nw,nh = self:getSize()
	for i,c in ipairs(self.components) do
		if (c.onParentLayout) then
			c:onParentLayout(0,0,nw,nh)
		end
	end
end