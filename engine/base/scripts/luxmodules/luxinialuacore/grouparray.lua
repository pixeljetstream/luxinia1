GroupArray = {
}
LuxModule.registerclass("luxinialuacore","GroupArray",
	[[
	A grouparray is an array that groups a couple of elements
	and behaves like one of them. This will only be done for 
	pure indexed array of this group, not for named arrays 
	(arrays that elements are indexed by numbers).
	
	This class is experimental.
	
	!!!Example:
	
	 test = GroupArray.new({
	   l2dtext.new("test","test",0),
	   l2dtext.new("test","test",0),
	 })
	
	 test:color(0,0,0,1)

	!!!Result:
	
	both l2dtext elements will be colored black. The call will return 
	a list of all results grouped in tables.
	
	]],GroupArray,{})

LuxModule.registerclassfunction("new",
	[[(GroupArray):(table atable,[boolean docopy]) -
	returns a GroupArray of the given table. It does nothing more than 
	setting the table's metatable to the GroupArray table. If docopy 
	is true, the given array is copied.]],nil)
function GroupArray.new (tab,docopy)
	local copy
	if (docopy) then
		copy = {}
		for a,b in tab do copy[a] = b end
	else
		copy = tab
	end
	setmetatable(copy,GroupArray)
	return copy
end

LuxModule.registerclassfunction("get",
	[[([value]/[...]):(GroupArray,[index]) - calls rawget if index is supplied,
	otherwise it returns all numbered elements of the array.]])
function GroupArray:get (index)
	if (index) then return rawget(self,index) end
	return unpack(self)
end

function GroupArray.__index (self,i)
	local exists = rawget(self,i)
	if (exists) then return exists end
	local first = rawget(self,1)
	if (not first) then return nil end
	local val = first[i]
	local n = table.getn(self)
	if (type(val)=="function") then
		return function (...)
			local results = {}
			local replace = arg[1] == self
			if (replace) then
				table.remove(arg,1)
			end
			for x=1,n do
				local element = rawget(self,x)
				if (replace) then
					table.insert(results,{element[i](element,unpack(arg))})
				else
					table.insert(results,{element[i](unpack(arg))})
				end
			end
			return results
		end
	end
end