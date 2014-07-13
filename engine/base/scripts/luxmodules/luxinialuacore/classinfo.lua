ClassInfo = {}
LuxModule.registerclass("luxinialuacore","ClassInfo",
[[There are two systems in luxinia that describe the classes and their
information: The fpubclass class of the the c core and the LuxModule class
that is written in lua. While the fpubclass takes care about all C classes,
the LuxModule class handles all classes that are written in Lua or are
imported via DLLs. The ClassInfo class trys to provide a minimalistic
access on both systems for returning classnames or their functions and
descriptions without distinguishing between the origin of the class.
]],ClassInfo,{})


LuxModule.registerclassfunction("getClassName",
		[[(string name):(table/userdata object) - returns the classname of the
		given datavalue if possible or nil.]])
function ClassInfo.getClassName (obj)
	if (type(obj)~="table" and type(obj)~="userdata") then return nil end
	local cls = fpubclass.class(obj)
	if (cls) then
		local name = fpubclass.name(cls)
		return name
	else
		local name = LuxModule.getClassInfo(obj)
		return name
	end
end

LuxModule.registerclassfunction("getClassDescription",
		[[(string description):(string classname) - Returns the dedscription for
		a class of the given name]])
function ClassInfo.getClassDescription (name)
	assert(type(name)=="string", "expected string as name for class")
	local cls = fpubclass.class(name)
	if (cls) then
		local name = fpubclass.name(cls)
		if (not name) then return nil end
		return fpubclass.description(cls)
	else
		local name,modname,descr,fnhelp = LuxModule.getClassInfo(name)
		if (not name) then return nil end
		return descr
	end
end

LuxModule.registerclassfunction("getFunctionDescription",
		[[(string name):(string classname, functionname) - Returns the description
		for the function of the given name of the given classname.]])
function ClassInfo.getFunctionDescription (classname, fnname)
	assert(type(classname)=="string" and type(fnname)=="string", "expected string as name for class")
	local list = ClassInfo.getFunctionList(classname)
	if (list) then return list[fnname] end
end

LuxModule.registerclassfunction("getFunctionList",
		[[(table functiondescriptions):(string classname) - Returns a table
		where each key is a functionname and the value is the description for
		the class. This includes also all functions of the interfaces or
		parent classes of the class.]])
function ClassInfo.getFunctionList (classname)
	local fnlist = {}
	local cls = fpubclass.class(classname)
	if (cls) then
		local function enlist (cls,listnotinherited)
			local n = fpubclass.functioninfo(cls)
			for i=0,n-1 do
				local name,descr = fpubclass.functioninfo(cls,i)
				if listnotinherited or fpubclass.inherited(cls,name) then
                    fnlist[name] = descr
				end
			end
			local ifcnt = fpubclass.interface(cls)
			for i=0,ifcnt-1 do
				local cls = fpubclass.interface(cls,i)
				if (cls) then enlist(cls) end
			end
			local p = fpubclass.parent(cls)
			if (p) then enlist(p) end
		end
		enlist(cls,true)
		return fnlist
	else
		local function enlist (classname)
			local name,modname,descr,fnhelp,parent,interfaces = LuxModule.getClassInfo(classname)
			if (not fnhelp) then return end
			for i,v in pairs(fnhelp) do fnlist[i] = v end
			for i,v in ipairs(interfaces or {}) do enlist(v) end
			if (parent) then return enlist(parent) end
		end
		enlist(classname)
		return fnlist
	end
end
