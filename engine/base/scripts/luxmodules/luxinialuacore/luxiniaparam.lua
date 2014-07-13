local luxiniaparamdescription = [[
The LuxiniaParam class stores the original list of arguments with which 
luxinia was called. The string of the commandline execution, i.e.:

 luxinia.exe -abc test 1 -f "hello world" --word 13

will be converted in a array that is stored in a global variable named "arg", 
which will look like this:

 arg[1] = "luxinia.exe"
 arg[2] = "-abc"
 arg[3] = "test"
 arg[4] = "1"
 arg[5] = "-f"
 arg[6] = "hello world"
 arg[7] = "--word"
 arg[8] = "13"

The LuxiniaParam class also provides a set of utility functions 
to simplify the processing of commandline options. The arguments with 
which luxinia was called are put into a table that can be accessed and 
modified. I.e. the example above will make create a table structure that 
looks like this:

 env["a"] = {"test","1"}
 env["b"] = {"test","1"}
 env["c"] = {"test","1"}
 env["f"] = {"hello world"}
 env["word"] = {"13"}

You can change values in this table with the given getters and setters.

Additionally, the LuxiniaParam class can help to describe the commandline 
options. ]]

LuxiniaParam = {}
LuxModule.registerclass("luxinialuacore","LuxiniaParam",
			luxiniaparamdescription,LuxiniaParam,{})


do -- getorigargs
	LuxModule.registerclassfunction("getOrigArgs",
	[[
	(table):() - returns a copy of the original arguments which is stored in 
	a global named "arg". Since the LuxiniaParam module makes a copy of this 
	table during startup, you must not worry about overwriting the original
	argument table suplied by the luxinia core.
	]])

	local _arg = {} -- original arguments 
	for a,b in pairs(luxarg) do 
		_arg[a] = b 
	end -- make a copy of the original args
	
	function LuxiniaParam.getOrigArgs()
		local ret = {}
		for a,b in pairs(_arg) do ret[a] = b end
		return ret
	end

	local function istriggerstr (v)
		return (string.byte(v) == string.byte("-")) 
	end
	
	local function istriggername (v)
		return 	(string.byte(v,1) == string.byte("-")) and 
				(string.byte(v,2) == string.byte("-"))
	end
	
	local function getargs(arg, index)
		local args = {}
		while (index<=table.getn(arg)) do
			if (istriggerstr(arg[index])) then break end
			table.insert(args,arg[index])
			index = index + 1
		end
		return args
	end
	
	local argenv = {}
	for i,v in pairs(_arg) do 
		if (istriggername(v)) then
			argenv[string.sub(v,2)] = getargs(_arg,i+1)
		elseif (istriggerstr(v)) then
			for j = 2,string.len(v) do
				argenv[string.sub(v,j,j)] = getargs(_arg,i+1)
			end
		end
	end
	
	LuxModule.registerclassfunction("getArg",
	[[
	([table]):(string name) - returns an argument table for the given 
	triggername.
	]])

	function LuxiniaParam.getArg (name) 
		return argenv[name]
	end

	LuxModule.registerclassfunction("setArg",
	[[
	([table]):(string name, table newvalue) - sets a new table value for the 
	given triggername. Pass nil to delete the triggername value.
	]])

	function LuxiniaParam.setArg(name, newvalue) 
		local value = argenv[name]
		argenv[name] = newvalue
		return value
	end	
	
	local knowntriggers = {[[
	():(string trigger, string description) - Adds a trigger description to the
	known descriptions. The known descriptions will be listed here. If your 
	trigger description is not listed here, please make sure that the 
	autodocc creation is done right after you have registered the trigger.
	!!!!Known triggers:	
	
	]]}
	
	LuxModule.registerclassfunction("addTriggerDescription",knowntriggers)
	local triggers = {}
	function LuxiniaParam.addTriggerDescription (trigger, description)
		triggers[trigger] = description
		table.insert(knowntriggers,"* "..trigger.."<br>"..description.."\n")
	end
	
	LuxModule.registerclassfunction("triggerDescriptionIterator",
	[[
	(function iterator):() - returns a iterator function that can be used 
	to list all known triggers to this moment. Use it like this:
	
	 for index, trigger, description in LuxiniaParam.iterator() do
	   print(index,trigger,description)
	 end
	]])
	function LuxiniaParam.iterator (trigger, description)
		local tab = {}
		for i,v in pairs(triggers) do table.insert(tab,i)  end
		local index,n = 1, table.getn(tab)
		return function ()
			if (index>n) then return end
			local i = index
			index = index + 1
			return i, tab[i], triggers[tab[i]]
		end
	end
	
	
end