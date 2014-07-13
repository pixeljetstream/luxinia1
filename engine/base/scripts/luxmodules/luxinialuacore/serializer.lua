
UtilFunctions = UtilFunctions or {}

LuxModule.registerclassfunction("serialize",
[[
	(string serialized):(table input) - Serializes a given table into a string that allows
	the reconstruction of the given table. It returns a string that describes the table's content
	with lua code. To deserialize the string, it must only be executed:
	
	 mytable = {1,2,3}
	 serial = UtilFunctions.serialize(mytable)
	 
	 mytable = nil
	 reconstructed = loadstring(serial)()
	 
	 for i,v in ipairs(reconstructed) do print(v) end
	  -- prints out 1,2,3 - converted first to string and then 
	  -- to a table again successfully
	
	The table may contain self cycles:
	
	 mytable = {1,2,3}
	 mytable.self = mytable
	 mytable[mytable] = "self index"
	 serial = UtilFunctions.serialize(mytable)
	 
	 mytable = nil
	 reconstructed = loadstring(serial)()
	 
	 print(reconstructed[reconstructed])
	 for i,v in ipairs(reconstructed.self) do print(v) end
	   -- prints out "self index" \n 1,2,3
	
	If the table contains function, the serilizing function will try to compile the 
	function in its binary representation using string.dump - which may fail if the 
	function is using local values from outside. 
	
	If the table has a metatable function entry named __serialize, this function 
	is being called and the return value is further serialized if it is a table value.
	However, if the function returns a string, the string is directly written 
	in the place where the serialisation takes place! You can inject lua code this way:
	
	 mytable = {}
	 setmetatable(mytable, {__serialize = 
	   function (self, idx)
	     return "function () return true end"
	   end
	 end
	 })
	 
	 print (UtilFunctions.serialize({ mytable }))
	 
	Output result:
	
	 local refs = {}
	 local fns = {}
	 for i=1,1 do refs[i] = {} end
	 		
	 refs[1][1] = function () return true end
	 
	 return refs[1]
	
	Important: don't serialize tables with __serialize metatables directly!
	Include them in a table instead, like in the example above.
	
	The serialisation function receives two arguments: the own table as 
	argument 1 and an index value as argument 2. The indexvalue represents 
	the id where the table data is stored (the refs table includes all 
	required values and other values must reference them as well, building 
	the cyclic tables and so on). The serialize function can return a 
	second value which tells the serializer how many reference table you 
	require - i.e. if you return 1 as second argument, an additional table
	is stored in refs. 
	
	You could also return a table, which is then serialized too, or a 
	string of a serialisation included in a function that is executed:
	
	 (function () UtilFunctions.serialize({1,2,3}) end)() 
	  -- creates a table {1,2,3}
	
	Since you can modify the output result, you can write inject any code you 
	want to save, meaning loops, functions and so on.
	
	The way the serialisation is stored is given in the output result above:
	A list of tables stored in refs is used to fill the data of the tables 
	one after another without breaking the consistence due to cyclic references.
	The fns table contains functions that are created with the string.dump functions.
	The loop creates all required tables so we can easily use them later.
	After this initialisation, the tables in refs are filled with all values. 
	At last, the first table value (which was passed to the serialize function) 
	is returned. You can either write this string in a file or make a lua 
	function of it by prepending "function serializedstuff () " and adding 
	"end" to the string returned by the serializing function.
	
]])

function UtilFunctions.serialize (obj)
	local output = {}
	local function write (...) 
		for i=1,select('#',...) do 
			table.insert(output,select(i,...)) end 
		table.insert(output,"\n") 
	end
	
	write ("local refs = {}")
	write ("local fns = {}")
	
	local serialized = {}
	local sc,fncnt = 0,0
	local fnserials = {}
	local mtserialtabs = {}
	
	local function makerefs (obj)
		local t = type(obj)
		if t == "table" and not serialized[obj] and not mtserialtabs[obj] then
			local mt = getmetatable(obj)
			if mt and mt.__serialize then 
				local mtab,inc = mt.__serialize(obj,sc+1)
				mtserialtabs[obj] = mtab
				sc = sc + ( inc or 0 )
				obj = mtab
				if type(mtab~="table") then return end
			end
			
			sc = sc + 1
			serialized[obj] = sc
			for i,v in pairs(obj) do
				makerefs(i)
				makerefs(v)
			end
		end
		if t == "function" then
			fncnt = fncnt + 1
			fnserials[obj] = fncnt
			write("fns["..fncnt.."] = loadstring "..("%q"):format(string.dump(obj)))
		end
	end
	
	makerefs(obj)
	
	write("for i=1,"..sc.." do refs[i] = {} end\n")
	
	
	for tab,id in pairs(serialized) do
		local prefix = "refs["..id.."]"
		
		for i,v in pairs(tab) do
			
			local i,imt = mtserialtabs[i] or i, mtserialtabs[i]
			local v,vmt = mtserialtabs[v] or v, mtserialtabs[v]
			local ti, tv = type(i),type(v)	
			
			if serialized[i] then i = "refs["..serialized[i].."]" 
			elseif ti == "string" and not imt then i = ("%q"):format(i)
			elseif ti == "function" then i = "fns["..fnserials[i].."]"
			else i = tostring(i) end
			
			if serialized[v] then v = "refs["..serialized[v].."]"
			elseif tv == "string" and not vmt then v = ("%q"):format(v)
			elseif tv == "function" then v = "fns["..fnserials[v].."]"
			else v = tostring(v) end
			
			write(prefix .."["..i.."] = "..v)
		end
		write("")
	end
	
	write("return refs[1]")

	return table.concat(output)
end

if serialisationtests then
	local complex = {}
	setmetatable(complex,{__serialize = function (self,id) return "refs["..id.."]\nsetmetatable(refs["..id.."],{})",1  end})
	
	local testtable = {}
	testtable.value = {" a value ", "another", testtable}
	testtable.self = testtable
	testtable.complex = complex
	testtable[{1,2,3,testtable,testtable.value}] = true
	testtable[function () return true end] = false
	
	local str = UtilFunctions.serialize(testtable)
	print (str)
	local out = assert(loadstring(str))()
	print (UtilFunctions.serialize(out))
	
	mytable = {1,2,3}
	serial = UtilFunctions.serialize(mytable)
	reconstructed = loadstring(serial)()
	
	for i,v in ipairs(reconstructed) do print(v) end
	-- prints out 1,2,3
	
	 mytable = {1,2,3}
	 mytable.self = mytable
	 mytable[mytable] = "self index"
	 serial = UtilFunctions.serialize(mytable)
	 
	 mytable = nil
	 reconstructed = loadstring(serial)()
	 
	 print(reconstructed[reconstructed])
	 for i,v in ipairs(reconstructed.self) do print(v) end
	   -- prints out 1,2,3
	   
	   
	 mytable = {}
	 setmetatable(mytable, {__serialize = 
	   function (self, idx)
	     return "function () return true end"
	   end
	 })
	 
	 print (UtilFunctions.serialize({mytable}))
end
