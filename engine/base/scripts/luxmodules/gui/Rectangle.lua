Rectangle = {}
LuxModule.registerclass("gui","Rectangle",
	[[A 2D rectangle for hittests. 2D Rectangles are used as bounding
	boxes for the Component class.]],Rectangle,{
	["1"] = "{[int]} - x component",
	["2"] = "{[int]} - y component",
	["3"] = "{[int]} - width",
	["4"] = "{[int]} - height"})

LuxModule.registerclassfunction("new", 
	[[():([table class, [float x,y,width,height]/[Rectangle rect] ]) - creates a rectangle object with the 
	given values as attributes. If a parameter is not specified, 0 is used as
	default value.]])
function Rectangle.new (class, x,y,w,h)
	local self = {x or 0,y or 0,w or 0,h or 0}
	if (type(self[1])=="table") then
		self[1],self[2],self[3],self[4] = unpack(x)
	end
	setmetatable(self,{
		__index = class, 
		__tostring = function(self) return self:toString() end
	})
	return self
end

LuxModule.registerclassfunction("contains", 
	[[(boolean):(Rectangle r, float x,y) - returns true if the given point is 
	inside the rectangle. The borders of the rectangle are included.]])
function Rectangle:contains(x,y)
	return x>=self[1] and x<=self[1]+self[3] and y>=self[2] and y<=self[2]+self[4]
end

LuxModule.registerclassfunction("merged", 
	[[(Rectangle):(Rectangle a, Rectangle b) - returns new Rectangle containing both.]])
function Rectangle:merged(a,b)
	local nx,ny,nw,nh
	nx = math.min(a[1],b[1])
	ny = math.min(a[2],b[2])
	nw = math.max(a[3]+a[1],b[3]+b[1])-nx
	nh = math.max(a[4]+a[2],b[4]+b[2])-ny
	
	return Rectangle:new(nx,ny,nw,nh)
end

LuxModule.registerclassfunction("add", 
	[[():(Rectangle self, Rectangle r / float x,float y) - adds a point or 
	rectangle to the rectangle.]])
function Rectangle:add (x,y)
	if (type(x)=="table") then
		self:add(x[1],x[2])
		self:add(x[1]+x[3],x[2])
		self:add(x[1]+x[3],x[2]+x[4])
		self:add(x[1],x[2]+x[4])
		return
	end
	local x1,y1,x2,y2 = 
		math.min(x,self[1]),
		math.min(y,self[2]),
		math.max(x,self[1]+self[3]),
		math.max(y,self[2]+self[4])
	self[1],self[2],self[3],self[4] = x1,y1,x2-x1,y2-y1
end

LuxModule.registerclassfunction("intersects", 
	[[(boolean):(Rectangle self, Rectangle other) - returns true if both
	rectangles do intersect]])
function Rectangle.intersects(a,b)
	return not (a[1]+a[3]<b[1] or b[1]+b[3]<a[1] or 
		a[2]+a[4]<b[2] or b[2]+b[4]<a[2])
end

LuxModule.registerclassfunction("area", 
	[[(area):(Rectangle self) - area of rectangle (width*height)]])
function Rectangle:area()
	return self[3]*self[4]
end

LuxModule.registerclassfunction("intersection", 
	[[([Rectangle]):(Rectangle a,b) - returns intersection of both rectangles
	or nil if a and b do not intersect. If a or b is nil, nil is returned.]])
function Rectangle.intersection (a,b)
	if (not a or not b) then return nil end
	local a = {a:getCorners()}
	local b = {b:getCorners()}
	local min,max = math.min,math.max
	local x1,y1,x2,y2 = max(a[1],b[1]),max(a[2],b[2]),min(a[3],b[3]),min(a[4],b[4])
	if (x1>x2 or y1>y2) then return nil end
	return Rectangle:new(x1,y1,x2-x1,y2-y1)
end

LuxModule.registerclassfunction("translate", 
	"():(Rectangle,x,y) - translates the rectangle")
function Rectangle:translate(x,y)
	self[1], self[2] = self[1] + x, self[2] + y
end

LuxModule.registerclassfunction("getCorners", 
	"(x1,y1,x2,y2):(Rectangle) - returns top left and bottom right coordinates")
function Rectangle:getCorners()
	return self[1],self[2],self[1]+self[3],self[2]+self[4]
end

LuxModule.registerclassfunction("setBounds", 
	"():(Rectangle self, float x,y,w,h) - sets bounds of the Rectangle")
function Rectangle:setBounds(x,y,w,h)
	self[1], self[2],self[3], self[4] = x,y,w,h
end

LuxModule.registerclassfunction("toString", 
	[[(string):(Rectangle self) - returns a simple string representing this rectangle.]])
function Rectangle:toString()
	return string.format("[Rectangle x=%.2f y=%.2f width=%.2f height=%.2f]",
		self[1],self[2],self[3],self[4])
end

LuxModule.registerclassfunction("getClosestSide", 
	"(side):(Rectangle,x,y) - returns closest side to a given point, 1 top edge, 2... clockwise")
function Rectangle:getClosestSide(x,y)
	--
	--		--------- -ty
	--				 |
	--		m		 |
	--				 |
	--		--------- ty
	--
	--  our target can be within 4 possible sectors
	--	each belonging to one edge
	-- 	we compare dotproducts to find out which sector it is
	--	we always dot with (0,1)
	
	local mx,my = self[3]/2,self[4]/2 
	local ty = my/math.sqrt(mx*mx+my*my)
	-- compute middle to outer point
	mx,my = x-self[1]-mx,y-self[2]-my

	my = my/math.sqrt(mx*mx+my*my)
	
	-- top or bottom are easy
	-- side could be either left or right
	if (my > ty) then 		return 1
	elseif (my < -ty) then	return	3
	elseif (mx > 0) then	return 2
	else return 4
	end
	
end
