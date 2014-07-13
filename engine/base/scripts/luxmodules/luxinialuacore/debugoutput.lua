DebugOutput = {}
LuxModule.registerclass("luxinialuacore","DebugOutput",
			[[A few helpers to print text on screen quickly.
			The output has 10 slots that you can use to print text out. 
			
			Example:
			 DebugOutput.enable(true)
			 DebugOutput.set(1,string.format("a = %.2f",math.pi))
			
			This will print pi as text on the screen. This is useful i.e. if
			you try to figure out good values for angles, rotations, velocities, 
			etc. during runtime. ]],DebugOutput,{})

local l2ds = {}
local x1,y1 = 10,10


function DebugOutput.set (i, tx)
	assert(type(i)=="number" and i>0 and i<=10,"slotnumber must be >=1 and <=10")
	tx = tostring(tx)
	if (l2ds[i]) then
		l2ds[i]:text(tx)
	end
end

function DebugOutput.enable (on) 
	if (not on and l2ds[1]) then
		l2ds = {}
	end
	if (on and not l2ds[1]) then
		for i=1,10 do 
			l2ds[i] = l2dtext.new("debugout","",0)
			l2ds[i]:parent(l2dlist.getroot())
			l2ds[i]:sortid(100000)
			l2ds[i]:spacing(8)
		end
		DebugOutput.setTopLeft(x1,y1)
	end
end

function DebugOutput.setTopLeft(x,y)
	x1,y1 = x,y
	for i,v in ipairs(l2ds) do
		v:pos(x,y+16*i,0)
	end
end

function DebugOutput.color(r,g,b)
	for i,v in ipairs(l2ds) do
		v:color(r,g,b,1)
	end
end