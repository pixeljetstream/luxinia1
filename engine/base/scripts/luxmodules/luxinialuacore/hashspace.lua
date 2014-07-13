-- hashlayer: Objects are sorted into tables to accelerate collision tests
-- improves performance ALOT

local hashlayer = {}
function hashlayer.new (width)
	local self = {}
	setmetatable(self,{__index = hashlayer})
	self.data = {}
	self.width = width or hashlayergridsize or 250
	self.n = 0
	self.indexlist = {}
	self.celllist = {}

	return self
end


local function intersects(a,b,c,d, e,f,g,h)
	a = a>e and a-e or e-a
	b = b>f and b-f or f-b
	return 
		(a)<c+g and
		(b)<d+h
	--not 
	--	(a+c<e or e+g<a or 
	--	 b+d<f or f+h<b)
end

function hashlayer:add(item,x,y,w,h)
	local ins = false
	w,h = w/2,h/2
	item = {item,x,y,w,h} --{w=w,h=h,item=item, x=x, y=y}
	local idxlst = {}
	assert(not self.indexlist[item[1]])
	self.indexlist[item[1]] = idxlst
	local function ins (x,y)
		local gridx = math.floor((x+0.5)/self.width)
		local gridy = math.floor((y+0.5)/self.width)
		--print("insert at ",gridx,gridy)
		local rows = self.data[gridx] or {n=0}
		self.data[gridx] = rows
		local cell = rows[gridy]
		if not cell then
			cell = {x=gridx,y = gridy}
			rows.n = rows.n + 1
			table.insert(self.celllist,cell)
			cell.id = #self.celllist
			rows[gridy] = cell
		end
		self.n = self.n + 1
		table.insert(cell,item)
		table.insert(idxlst,cell)
	end
	
	--local w,h = math.max(w,self.width),math.max(h,self.width)
	--local w,h = math.max(w,self.width*2),math.max(h,self.width*2)
	local stepsw = math.ceil(w*2/self.width)*self.width	
	local stepsh = math.ceil(h*2/self.width)*self.width
	for a = x-w,x-w+stepsw,self.width do
		for b = y-h, y-h+stepsh, self.width do
			ins(a,b)
		end
	end
	
	--print(table.count(self.indexlist),#idxlst)
end

function hashlayer:remove(item)
	local celllist = self.indexlist[item] or {}
	for i=1,#(celllist) do
		local cell = celllist[i]
		local rem = false
		for j=1,#cell do 
			if cell[j][1] == item then 
				table.remove(cell,j)
				if #cell==0 then 
					local xch = table.remove(self.celllist)
					if xch~=cell then
						xch.id = cell.id
						self.celllist[xch.id] = xch
					end
					self.data[cell.x][cell.y] = nil
					self.data[cell.x].n = self.data[cell.x].n - 1
					if self.data[cell.x].n == 0 then
						self.data[cell.x] = nil
					end
				end
				self.n = self.n - 1
				rem = true
				break
			end
		end
	end
	self.indexlist[item] = nil
end

local weak = {__mode="k"}
-- get query 
-- queries a list of objects for this region
function hashlayer:get(x,y,w,h,append)
	local collection = append or {}
	
	w,h = w/2,h/2
	local tested = {}
	local flag = true
	--print "-----------------------"
	local function collect(a,b)
		local gridx = math.floor((a+0.5)/self.width)
		local gridy = math.floor((b+0.5)/self.width)
		
		local rows = self.data[gridx]
		if not rows then return end
		local cell = rows[gridy]
		if not cell then return end
		--print("check at ",gridx,gridy)
		for i=1,#cell do
			item = cell[i]
			if tested[item]~=flag then 
				if intersects (x,y,w,h, item[2],item[3],item[4],item[5]) then
					table.insert(collection,item[1])
				end
				tested[item] = flag
			end
		end
	end
	
	--local w,h = math.max(w,self.width),math.max(h,self.width)
	local stepsw = math.ceil(w*2/self.width)*self.width	
	local stepsh = math.ceil(h*2/self.width)*self.width
	for a = x-w,x-w+stepsw,self.width do
		for b = y-h, y-h+stepsh, self.width do
			collect(a,b)
		end
	end
	
	return collection
end

-- hashlayer / hashlayer testing
-- only more efficient than the linear searching if you build both spaces 
-- anyway, otherwise it is about 20% slower than the naive searching for tests
-- by using the get query
function hashlayer:test(otherspace,fncall)
	if self.n>otherspace.n then self,otherspace = otherspace,self end
	--assert(self.width == otherspace.width, "must have same cellsizes")
	if self.width~=otherspace.width and self.width>otherspace.width then
		self,otherspace = otherspace,self
	end
	
	local pairlist = self.pairlist or {}
	local flag = true
	local function cellcheck (a,b)
		for i=1,#a do
			e = a[i]
			for j=1,#b do
				f = b[j]
				if not pairlist[e] or pairlist[e][f]~=flag then
					pairlist[e] = pairlist[e] or {}
					pairlist[e][f] = flag
					if intersects(
						e[2],e[3],e[4],e[5],
						f[2],f[3],f[4],f[5]
					) then
						fncall(e[1],f[1])
					end
				end
			end
		end
	end
	
	local r = self.width/otherspace.width
	
	for i=1,#self.celllist do
		local cell = self.celllist[i]
		local otherrow = otherspace.data[math.floor(cell.x*r+0.5)]
		if otherrow then	
			othercell = otherrow[math.floor(cell.y*r+0.5)]
			if othercell then
				cellcheck(cell,othercell)
			end
		end
	end
end

Hashspace = {}
LuxModule.registerclass("luxinialuacore","Hashspace",
		[[A hashspace stores data paired with positional information in order
		to accelerate requests for data within a certain area. 
		
		This hashspace is implemented in lua and is restricted to 2D spaces 
		and rectangular areas,
		which means that only two coordinates can be used to store the data.
		
		The structure is - even though it is implemented in lua only - quite
		fast. If you have complex environments but don't want to use ODE for 
		collision detection, you can use this class. It is very easy to use and
		can manage thousands of objects cheaply. You can even remove items 
		from the space and reinsert them somewhere else while the costs for 
		doing that are cheap.
		
		!Example
		
		 space = Hashspace.new() -- create hashspace
		 space:add("pawn1",5,4,10,10) -- insert pawn at 5,4 with size 10,10
		 space:add("pawn2",15,6,10,10) -- insert pawn at 15,6 with size 10,10
		 environment = Hashspace.new() -- create a hashspace for the environment
		 for i=1,100 do
		   -- insert now lots of small boxes scattered around randomly
		   environment:add("obstacle "..i,math.random()*100,math.random()*100,4,4)
		 end
		 space:test(environment, -- test the playerspace against the environment
		   function (a,b) -- function is called if two items from each space intersects
		     print(a,b) -- print out intersecting items
		   end)
		
		!Output
		The result from the test above (may differ from other results due to different
		random number generation):
		 obstacle 4   pawn2
		 obstacle 6   pawn2
		 obstacle 23  pawn1
		 obstacle 29  pawn1
		
		 ]],Hashspace,{})

LuxModule.registerclassfunction("new",
	[[(Hashspace):([gridsize = 2]) - Creates a hashspace structure. The gridsize
	determines the minimum size of the cells in the space - which cannot be 
	smaller than that. If your objects and coordinates are much smaller (i.e. 0.01),
	you should pass another size which fits the average sizes in your case better.
	The hashspace works well if the gridsize is somewhere between the minimum and
	average size of all your objects in your environment.]])
function Hashspace.new(gridsize)
	local self = {}
	setmetatable(self,{__index = Hashspace})
	self.gridsize = gridsize or 2
	self.layers = {n=0}
	return self
end

function Hashspace.new()
	local self = {dspacehash.new(),n=0,list={},lut = {}}
	setmetatable(self,{__index = Hashspace})
	return self
end

function Hashspace:getlayerid(w,h)
	w = (w>h and w or h)
	local tw = self.gridsize
	local n = 1
	while tw<w do tw,n = tw*2,n+1 end
	return n,tw
end

LuxModule.registerclassfunction("get",
	[[(table) : (Hashspace,centerx,centery,width,height,[appendtable = {}]) - 
	rectangular request 
	to the hashspace. It will return a list of all items that are intersecting
	this area. If the appentable is passed, the items will be appended to that
	list. This is useful if you have multiple spaces and test them each for 
	an area, creating a single list.
	
	 list = space:get(2,4,10,6) -- request area is at 2,4 with width/height=10,6
	 for i,item in ipairs(list) do -- iterate all items
	   print(item) -- print out intersecting items
	 end
	]])
function Hashspace:get(x,y,w,h,append)
	append = append or {}
	for i=1,self.layers.n do
		local space = self.layers[i]
		if space then
			space:get(x,y,w,h,append)
		end
	end
	return append
end

function Hashspace:get(x,y,w,h,append)
	append = append or {}
	local box = dgeombox.new(w,h,1)
	box:pos(x,y,0)
	local cnt,tab = box:testnear(self[1],self.n)
	for i=1,#tab do 
		--print(i,tab[i])
		if box~=tab[i] then
			local c,d = tab[i]:size()
			local a,b = tab[i]:pos()
			if intersects (x,y,w,h, a,b,c,d) then
				table.insert(append,self.list[tab[i]])
			end
		end
	end
	box:delete()
	return append
end

LuxModule.registerclassfunction("add",
	[[():(Hashspace, item, centerx,centery,width,height) - 
	inserts item (which can be any type) into the hashspace at the given coordinates.
	The given rectangle will be used if an intersection case is tested.
	
	Reinserting an element if it is already present in the space will throw an 
	error. Make sure it does not exist in that space before adding it!]])
function Hashspace:add(item,x,y,w,h)
	local layer,grid = self:getlayerid(w,h)
	self.layers.n = self.layers.n>layer and self.layers.n or layer
	self.layers[layer] = self.layers[layer] or hashlayer.new(grid) 
	self.layers[layer]:add(item,x,y,w,h)
	--print(self.layers[layer].n)
	--print(layer,grid)
end

function Hashspace:add(item,x,y,w,h)
	local box = dgeombox.new(w,h,1,self[1])
	assert(not self.lut[item],"element already inserted")
	self.list[box] = item
	self.lut[item] = box
	box:pos(x,y,1)
	self.n = self.n + 1
end

LuxModule.registerclassfunction("remove",
	[[():(Hashspace, item) - removes a certain item from the hashspace. 
	This can be used if an object has moved over the time and the 
	rectangle has to be updated in the hashspace. If you have lots of objects
	in a hashspace and only few are changing their position, you shouldn't rebuild
	the space from scratch (which might be more efficient if lot's of data has changed)
	but remove the item and reinsert it again. As this is still somewhat 
	expensive, you could also prefer to insert your items with a slightly larger
	rectangle and make an exact collisiontest for the hitlists yourself and update
	the rectangle only if the object has moved a larger distance. It improves 
	speed alot if you chose rectangles that are 10% larger if it helps to reduce
	the number of removes and add calls per frame for about 1:10. ]])
function Hashspace:remove(item)
	for i=1,self.layers.n do
		local space = self.layers[i]
		if space then
			space:remove(item)
		end
	end
end

function Hashspace:remove(item)
	local box = self.lut[item]
	if not box then return end
	self.lut[item],self.list[box] = nil,nil
	box:delete()
	self.n = self.n - 1
end

LuxModule.registerclassfunction("test",
	[[():(Hashspace self, Hashspace otherspace, fncallback) - 
	Tests two hashspaces against each other and call the callback function in
	case of intersection with both intersecting items (function (a,b)). 
	Testing two hashspaces against each other is much more efficient than 
	doing all the requests by calling the get function. If you have more requests
	to do, you should batch all requests in another hashspace and test it then
	with the other space. However, if both structures must be built from scratch,
	the get function will be a little bit faster. It only pays out if at least 
	one of both structures has been built before anyway.
	
	If two spaces are tested, all items will be only be tested once against 
	each other. However, if both spaces are the same space (for testing 
	intercollisions), you have to ignore selfintersection of an item which 
	will occur then.
	]])
function Hashspace:test(s,fn)
--print (debug.getinfo(2).short_src,debug.getinfo(2).currentline)
	--print(self.layers.n,s.layers.n)
	for i=1,self.layers.n do
		local space = self.layers[i]
		if space then
			for j=1,s.layers.n do 
				local osp = s.layers[j]
				if osp then 
					--print(osp.width,space.width)
					space:test(osp,fn) 
				end
			end
		end
	end
end

function Hashspace:test(s,fn)
	local cnt,lst = self[1]:testnear(s[1],self.n)
	for i=1,cnt,2 do 
		local i1,i2 = self.list[lst[i]],s.list[lst[i+1]]
		if not i1 then i1,i2 = s.list[lst[i]],self.list[lst[i+1]] end
		
		local w,h = lst[i+1]:size()
		local x,y = lst[i+1]:pos()
		local c,d = lst[i]:size()
		local a,b = lst[i]:pos()
		if intersects (x,y,w,h, a,b,c,d) then
			fn(i1,i2)
		end
		
		
	end
end


--Hashspace = hashlayer