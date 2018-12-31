-- scalararray and staticarray


-- scalararray and staticarray are useful for
-- number crunching tasks. With larger counts
-- they are faster than doing arithmetic functions
-- in Lua.
--
-- scalararray is more flexible and supports most native
-- scalartypes.

l2dlist.getrcmdclear():color(true)
l2dlist.getrcmdclear():colorvalue(0.8,0.8,0.8,1)
local lbl = Container.getRootContainer():add(Label:new(20,20,300,48,
	"This sample prints to log.txt and console, will start any second."))


-- LOGPRINT is part of tutorial framework


function doit()
LOGPRINT = LOGPRINT or print
LOGPRINT("===========================")
LOGPRINT("scalararray and staticarray")
LOGPRINT("===========================")

local function printscalar(sa)
	LOGPRINT("ScalarArray: "..tostring(sa))
	
	sa:offset(0,0,0)
	local function addvec(vals,vecs)
		vals = vals.."("
		local cnt = #vecs
		for i,v in ipairs(vecs) do
			vals = vals..v..(i < cnt and "," or "")
		end
		return vals..")\t"
	end
	
	local sx,sy,sz = sa:size()
	if (sy > 1) then
		sz = sz - 1
		sy = sy - 1
		sx = sx - 1
		
		for z=0,sz do
			LOGPRINT("Z:"..z)
			for y=0,sy do
				local vals = "Y:"..y.."\t"
				for x=0,sx do
					local vecs = {sa:vectorat(x,y,z)}
					vals = addvec(vals,vecs)
				end
				LOGPRINT(vals)
			end
		end
	else 
		local cnt = sa:count()-1
		for i=0, cnt do
			local vecs ={sa:vector(i)}
			local vals = addvec("",vecs)
			LOGPRINT("I:"..i,vals)
		end
	end
end


local function runop()
	LOGPRINT("\n-----------------------")
	LOGPRINT("runop")
	LOGPRINT("-----------------------")
	coroutine.yield()
	
	sa = scalararray.new(scalartype.uint8(),16,2)
	sb = scalararray.new(scalartype.uint8(),16,2)
	sc = scalararray.new(scalartype.uint8(),4,2)
	
	sa:size(4,4,1)
	sb:size(4,4,1)
	sc:size(2,2,1)
	
	sb:vectorall(200,200)
	sc:vectorall(100,100)
	
	printscalar(sb)
	printscalar(sc)
		
	sa:op2(scalarop.add2(),sb,sc)
	printscalar(sa)
	
	sa:offset(2,2,0)
	sb:offset(2,2,0)
	
	sa:op2region(2,2,1,scalarop.add2sat(),sb,sc)
	printscalar(sa)
	
	sa:offset(0,2,0)
	sb:offset(0,2,0)
	sa:op2region(2,2,1,scalarop.add2sat(),sb,10)
	printscalar(sa)
end
runop()

local function runcurve(closed,vec3)
	LOGPRINT("\n-----------------------")
	LOGPRINT("runcurve",closed,vec3)
	LOGPRINT("-----------------------")
	coroutine.yield()

	sa = scalararray.new(scalartype.float32(),16,vec3 and 3 or 4)
	sb = scalararray.new(scalartype.float32(),4,vec3 and 3 or 4)

	sa:size(closed and 8 or 7,1,1)
	sb:size(4,1,1)
	if (vec3) then
		sb:vector(0,	0,0,0)
		sb:vector(1,	0,1,0)
		sb:vector(2,	1,1,0)
		sb:vector(3,	1,0,0)
	else
		sb:vector(0,	0,0,0,0)
		sb:vector(1,	0,1,0,0)
		sb:vector(2,	1,1,0,0)
		sb:vector(3,	1,0,0,0)
	end
	
	sa:curvelinear(sb,closed)
	printscalar(sa)
	
	sa:curvespline(sb,closed)
	printscalar(sa)
end
runcurve(false,true)

local function bench(size)
	local dim = size or 32
	local runs = 1024
	
	LOGPRINT("\n-----------------------")
	LOGPRINT("bench",tostring(dim).."x"..tostring(dim))
	LOGPRINT("-----------------------")

	
	local time = -system.getmillis()
	for i=1,runs do
	end
	time = time + system.getmillis()
	LOGPRINT(":runs",runs)
	LOGPRINT(":runstime",time)
	
	local cnt = dim*dim
	
	sa4 = scalararray.new(scalartype.float32(),cnt,4)
	so4 = scalararray.new(scalartype.float32(),cnt,4)
	
	sa4:size(dim,dim,1)
	so4:size(dim,dim,1)
	
	sa3 = scalararray.new(scalartype.float32(),cnt,3)
	so3 = scalararray.new(scalartype.float32(),cnt,3)
	
	sa3:size(dim,dim,1)
	so3:size(dim,dim,1)
	
	sc3 = scalararray.new(scalartype.float32(),4,3)
	sc4 = scalararray.new(scalartype.float32(),4,4)
	
	sa4:vectorall(10,10,10,10)
	sa3:vectorall(10,10,10)
	
	
	sc3:vector(0,	0,0,0)
	sc3:vector(1,	0,1,0)
	sc3:vector(2,	1,1,0)
	sc3:vector(3,	1,0,0)
	
	sc4:vector(0,	0,0,0,0)
	sc4:vector(1,	0,1,0,0)
	sc4:vector(2,	1,1,0,0)
	sc4:vector(3,	1,0,0,0)
	
	st3 = floatarray.new(cnt*3)
	stc3 = floatarray.new(4*3)
	stc3:v3(0,	0,0,0)
	stc3:v3(1,	1,0,0)
	stc3:v3(2,	1,1,0)
	stc3:v3(3,	1,0,0)
	
	-- SPLINE

	LOGPRINT("FLOAT SPLINE")
	local time = -system.getmillis()
	for i=1,runs do
		so3:curvespline(sc3,false)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar3",time)
	
	local time = -system.getmillis()
	for i=1,runs do
		so4:curvespline(sc4,false)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar4",time)
	
	local time = -system.getmillis()
	for i=1,runs do
		floatarray.v3spline(st3,stc3)
	end
	time = time + system.getmillis()
	LOGPRINT("static3",time)
	

	-- ARITHMETIC

	LOGPRINT("FLOAT ARITHMETIC")
	local time = -system.getmillis()
	for i=1,runs do
		sa3:op2(scalarop.add2(),sa3,1.5)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar3",time)
	
	local time = -system.getmillis()
	for i=1,runs do
		sa4:op2(scalarop.add2(),sa4,1.5)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar4",time)
	
	st3:v3all(10,10,10)
	local time = -system.getmillis()
	for i=1,runs do
		floatarray.add(st3,1.5)
	end
	time = time + system.getmillis()
	LOGPRINT("static3",time)
	
	local vals = {}
	local vcnt = cnt*3
	for i=1,vcnt do 
		table.insert(vals,10)
	end
	local time = -system.getmillis()
	for i=1,runs do
		for n=1,vcnt do
			vals[n] = vals[n] + 1.5
		end
	end
	time = time + system.getmillis()
	LOGPRINT("lua3",time)
	
	-- SAMPLING

	LOGPRINT("FLOAT SAMPLING")
	local samplecnt = dim*4
	local samplepos = {}
	LOGPRINT(":samples",samplecnt*runs)
	local time = -system.getmillis()
	for i=1,runs do
		for i=1,samplecnt do
		end
	end
	time = time + system.getmillis()
	LOGPRINT(":samplestime",time)
		
	for i=1,samplecnt do
		table.insert(samplepos,{math.random(),math.random(),0})
	end
	
	local time = -system.getmillis()
	for i=1,runs do
		for i=1,samplecnt do
			sa3:sample(unpack(samplepos[i]))
		end
	end
	time = time + system.getmillis()
	LOGPRINT("scalar3",time)
	
	local time = -system.getmillis()
	for i=1,runs do
		for i=1,samplecnt do
			sa4:sample(unpack(samplepos[i]))
		end
	end
	time = time + system.getmillis()
	LOGPRINT("scalar4",time)
	
	-- TRANSFORM

	LOGPRINT("FLOAT TRANSFORM")
	local mat = matrix4x4.new()
	mat:rotdeg(45,60,70)
	
	local time = -system.getmillis()
	for i=1,runs do
		sa3:ftransform(sa3,mat)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar3",time)
	
	local time = -system.getmillis()
	for i=1,runs do
		sa4:ftransform(sa4,mat)
	end
	time = time + system.getmillis()
	LOGPRINT("scalar4",time)
	
	

end
	print("...")
	coroutine.yield()
bench(4)
	print("...")
	coroutine.yield()
bench(8)
	print("...")
	coroutine.yield()
bench(16)
	print("...")
	coroutine.yield()
bench(32)
	print("...")
	coroutine.yield()
bench(256)
end



TimerTask.new(function()
	console.active(true)
	doit()
	print("")
	print("DONE - Close console with F1, then press ESC to return")
	print("Pressing ESC when console is open, will always quit luxinia")
end,3000)


return (function() lbl:delete() end)
