--arg = arg or {}
-- LuxModules are luascripts that are integrated in luxinia and are
-- part of the official distribution.
-- LuxModules are extending luxinia with additional scripts for simplified use.

LuxModule = {modules = {}, doc = {},descriptions={}, lastmod, lastclass}
function LuxModule.register(name,description)
	if (LuxModule.modules[modulename]) then
		return error("LuxModule already registered") end
	LuxModule.modules[name] = {}
	LuxModule.descriptions[name] = description
end

function LuxModule.registerclass(modulename,name,descr,class,help,parent,interfaces, isthreadsafe, safefunctions)
	if (not LuxModule.modules[modulename]) then return error("Unknown LuxModule: "..modulename) end

	LuxModule.modules[modulename][name] = {class = class, classdescription=descr,
			help = help, parent = parent, modulename = modulename, classname = name,
			interfaces = interfaces, safe=isthreadsafe, safefunctions = safefunctions}
	LuxModule.lastclass = name
	LuxModule.lastmod = modulename
end

function LuxModule.getClassInfo(class)
	local function retinf (class)
		return class.classname,class.modulename, class.classdescription, class.help, class.parent, class.interfaces
	end
	if (type(class)=="string") then
		for a,b in pairs(LuxModule.modules) do
			if (b[class]) then
				return retinf(b[class])
			end
		end
		return nil
	end
	for modname,v in pairs(LuxModule.modules) do
		for name,inf in pairs(v) do
			if (inf.class == class) then
				return retinf(inf.class)
			end
		end
	end
	local mt = getmetatable(class)
	if (not mt or not mt.__index) then return nil end
	return LuxModule.getClassInfo(mt)
end

function LuxModule.registerclassfunction(...)
	local arg ={...}
	local function c1 (modulename,classname,functionname,description)
		if (description==nil) then return error("Functiondescription must not be null",3) end
		if (functionname==nil) then return error("Functionname must not be null",3) end
		if (not LuxModule.modules[modulename]) then return error("Unknown LuxModule: "..modulename) end
		if (type(description)=="string" and string.find(description,"@overloaded %w*")) then
			local class = string.gfind(description,"@overloaded (%w*)")()
			description = string.gsub(LuxModule.modules[modulename][class].help[functionname],
				"(.-)-%s*(.+)","%1 - description from overloaded method of "..class..":\n\n%2")
		end
		LuxModule.modules[modulename][classname].help[functionname]=description
	end
	
	if (#arg==2) then
		table.insert(arg,1,LuxModule.lastmod)
		table.insert(arg,2,LuxModule.lastclass)
	end
	

	return c1(unpack(arg))
end

function LuxModule.load (modulename)
	if (not LuxModule.modules[modulename]) then return error("Unknown LuxModule") end
	for a,b in pairs(LuxModule.modules[modulename]) do
		_G[a] = b.class
	end
end

function LuxModule.getclasses (modulename)
	local classes = {}
	for a,b in pairs(LuxModule.modules[modulename]) do
		classes[a] = b.class
	end
	return classes
end

function LuxModule.getclassinfo (modulename,classname)
	if (not LuxModule.modules[modulename]) then return error("Unknown LuxModule") end
	if (not LuxModule.modules[modulename][classname]) then return error("Unknown class") end
	local c = LuxModule.modules[modulename][classname]
	local desc = c.classdescription
	if (type(desc)=="function") then desc = desc(modulename,classname) end
	if (type(desc)=="table") then desc = table.concat(desc) end
	return c.class, c.classdescription, desc, c.parent, c.interfaces, c.safe,c.safefunctions
end

function LuxModule.getclasshierarchy (modulename)
	local tab = {}
	for cname,info in pairs(LuxModule.modules[modulename]) do
		local class = info.class
		if (class) then
			local fns = {}
			local cnt = 0
			local safes = {}
			for name,descript in pairs(info.help) do
				if (type(descript)=="function") then descript = descript(modulename,cname,name) end
				if (type(descript)=="table") then descript = table.concat(descript) end
				fns[name] = descript
				safes[name] = info.safefunctions and info.safefunctions[name] and true
				cnt = cnt + 1
			end
			tab[cname] = {
				description = info.classdescription or "",
				functions = fns,
				functioncount = cnt,
				classid = i,
				parent = info.parent,
				modulename = modulename,
				interfaces = info.interfaces or {},
				safefunctions = safes or {},
			}
		end
	end

	local hierarchy = {}
	local childs = {}
	local classcount,functioncount = 0,0

	for i,j in pairs(tab) do
		local parent = j.parent
		local interfaces = j.interfaces
		tab[i] = {name = i, description = j.description, childs = nil, parent = nil,
			functions = j.functions, interfaces = interfaces, safefunctions = j.safefunctions}
		if (parent and parent~=i) then
			childs[parent] = childs[parent] or {}
			childs[parent][i]=tab[i]
		end
		classcount = classcount + 1
		functioncount = functioncount + j.functioncount
	end

	for i,j in pairs(childs) do
		if (tab[i]) then
			tab[i].childs = j
			for a,b in pairs(j) do
				b.parent = tab[i]
			end
		end
	end

	for i,j in pairs(tab) do
		if (not j.parent) then hierarchy[i] = j end
	end
	return hierarchy,tab
end

function LuxModule.modulenames ()
	local names = {}
	for a,b in pairs(LuxModule.modules) do table.insert(names,a) end
	return names
end

function LuxModule.getLuxModuleDescription (modulename)
	local x = LuxModule.descriptions[modulename]
	if (type(x) == "function") then return x(modulename) end
	return LuxModule.descriptions[modulename]
end

function LuxModule.setLuxModuleDescription (modulename,description)
	LuxModule.descriptions[modulename] = description
end

LuxModule.register("luxinialuacore",
	[[The luxinia lua core contains the most useful utility functions. These
	handle the console, the timercallback and the keyboard / mouse event
	events, the module core and the autodoc system.
	
	The 'luxinia' table can have following fields:
	* think: a function that is called every frame after rendering. The default function takes care of the the Timers.
	* windowresized: The default function calls functions registered with UtilFunctions.addWindowResizeListener.
	
	These functions also reside in the table for advanced access.
	* framebegin:
	* framepostvistest:
	* framepostl3d:
	* frameend:
	
	The 'luxiniaptrs' table has following fields:
	* fmcache: lightuserdata pointer to FastMathCache_t (luxmath library)
	* refsys: lightuserdata pointer to LuxiniaRefSys_t (luxinia library)
	* fpub: lightuserdata pointer to LuxiniaFPub_t (luxinia library)
	
	Please do not alter those! They are typically read on "mymodule_luaopen" in lua dll extenders.
	]])
LuxModule.registerclass("luxinialuacore","LuxModule",
			[[LuxModules are luascripts that are
	part of the official Luxinia distribution. LuxModules are not related to
	the module and package system of lua. LuxModules are creating lot's of
	global functionnames, so watch out for problems for namespaces.

	LuxModules are extending the luxinia API with addtitional scripts for
	simplified use. In difference to the native API of Luxinia, modules are implemented
	within the scripting of Luxinia. LuxModules extend Luxinia with functions and classes
	that are simpler to use than the native API functions. This means that you can modify
	the modules, but you should always keep in mind that Luxinia can and should be used
	for different projects and maybe not only for your own projects. Changing the official
	scripts may cause problems with other projects that are not written by you into. Even
	worse, changes on the official scripts can will need you to update your modified scripts, too.<br>
	However, you may write your own modules that fits into the modulestructure of Luxinia.
	By doing so, you can extend Luxinia in a way that other can easily use, too. ]],LuxModule,
	{
		getLuxModuleDescription = "([string]):(string modulename) - return description of LuxModule",
		setLuxModuleDescription = [[():(string modulename, string/function description) -
			sets description of LuxModule]],
		modulenames = "(table modulenames, int count):() - returns names and number of "..
			"known modules.",
		getclasshierarchy = "(table hierarchy, table allclasses) : "..
			"(string modulename) - returns a hierarchy in a datastructure that is "..
			"compatible to the used format in the autodoc.lua file. ",
		register = "Takes care about modules. LuxModules consist of a number of "..
			"classes. Each class provides its own functionality. The modules "..
			"class itself takes care about all the registered modules i.e. "..
			"loading and documentation.",
		register = "():(string modulename, string description) - "..
			"Registers a modulename as LuxModule. The description should explain "..
			"what the whole LuxModule is good for.",
		getclassinfo = "():(string modulename, string classname)"..
			" - "..
			"returns all known information on a class of "..
			"a LuxModule",
		load = "():(string modulename) - "..
			"Puts all the classes that belong to the LuxModule into the global "..
			"variable space. I.e. if a LuxModule has a certain class that's name "..
			"is 'foo', the class can be used from now on by simply using the "..
			"name 'foo'. This can of course override your own tables or even "..
			"classes of other modules. If this is a problem to you, you can use "..
			"getclasses instead, which returns a table of classes.",
		registerclass = [[():(string modulename,string name,
			string/function/table description,
			table class,
			table help, [string parent],[table interfaces], [boolean isthreadsafe],
			[table threadsafefunctions]) -
			registers a class that is part of a certain LuxModule with a certain
			name. The help table should describe each function of the class with
			the functionname as key and a descriptionstring that describes the
			arguments and functiondetails.
			You can optionally define the name
			of the parentclass and the names of the interfaces. This has only a
			hierachical purpose for the documentation.

			If the description is a function, the function is called with the
			modulename and classname as argument. It may return a string or a
			table.

			If the description is a table, it is concatenated to a string.
			]],
		getclasses = "(table classes):(string modulename) - "..
			"Returns the classes as a table. This prevents overriding previously "..
			"defined global variables as described in 'load'. I.e. if a LuxModule "..
			"named 'foo' is loaded and it contains the classes 'a' and 'b', the "..
			"returned table contains the two classes 'a' and 'b'.",
		registerclassfunction = "():(string modulename,string classname,string "..
			"functionname,string description) - sets a functiondescription for "..
			"a function of a class.",
		getClassInfo = "([string name, modulename,description, table functionhelp, "..
			"string parent, table interfaces]):"..
			"(table obj/string name) - returns "..
			"name and modulename of the given class or object if possible. If a string "..
			"was passed, the name and the modulename is returned if it is a registered "..
			"class. Additionally, the description of the class and the description "..
			"of the functions is returned as a table where the keys are the functionnames "..
			"and the values are the functiondescriptions.",
	}
)

local function loadLuxModule (name)
	local err,msg = xpcall(
		function () dofile("base/scripts/luxmodules/"..name.."/mod_"..name..".lua") end,
		debug.traceback
	)
	if (not err) then print(msg) end
end

require "lfs"

--local files = luxinia.dir("base/scripts/luxmodules/*")
loadLuxModule("luxinialuacore")
for b in lfs.dir("base/scripts/luxmodules") do
	if (string.sub(b,1,1) ~= "." and string.sub(b,-4)~=".lua" and b~="CVS" and
		b~="luxinialuacore") then
		loadLuxModule(b)
	end
end


