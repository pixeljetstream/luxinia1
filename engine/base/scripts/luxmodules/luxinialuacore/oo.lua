-- object oriented module system
OO = {}
LuxModule.registerclass("luxinialuacore","OO",
		[[The objectoriented class provides two functions which
		allows a more objectoriented like design using lua functions.

		It implements a simple inheritance model. An object is a table
		with a metatable which includes all class information.

		All functions from this class are available as global variables
		with the same name.

		!!Examples:

		 class "Foo" -- a simple class
		 print (Foo)
		   -- output: [Class: Foo]
		 fobj = new(Foo) -- creates a new object of type foo
		 print(fobj) -- print out our class object
		   -- output: [Object: Foo 3]

		 -- overfloating printing function now
		 function Foo:toString()
		   return "Foo object printout!"
		 end
		 print(fobj) -- will print "Foo object printout!"

		 -- creating a class that is derived from Foo
		 class "Test" : extends "Foo"
		 tobj = new(Test)
		 print(tobj) -- will also print "Foo object printout!",
		   -- because the toString function is derived from "Foo"

		!!Constructors

		The constructors have the same name as the class has.
		The default constructor will delegate its call to the
		parent's constructor, however not so if the constructor
		is overloaded.

		Calling the parent constructor is being done by calling
		its classname like a method.

		 class "Foo"
		 function Foo:Foo (v1,v2)
		   print("Foo constructor",v1,v2)
		 end

		 class "Bar" : extends "Foo"
		 new(Bar,1,2)  -- will delegate the constructor call to
		   -- Foo:Foo, passing the arguments 1,2 as v1 and v2,
		   -- resulting in a printout which looks like this:
		   -- Foo constructor 1 2

		Calling the constructors is done by calling the parent's
		method. This can be problematic in some situations, but
		these are not real world examples (i.e. if the object has a
		variable with a name of a parent's class). Such situations
		can also be avoided either by not using capital letters
		for object variables while saving them for class names
		(which is similiar to Java's naming convention).

		 class "Foo"
		 function Foo:Foo ()
		   Foo:Object () -- call the constructor of Object
		     -- as every object is derived from this class
		 end

		!!Compatibility

		The function 'new', which acts as a constructor is
		compatible with older code - if a table that is passed as class
		has a field named 'new', it will accept it as argument
		and will call this function with the passed table as first argument
		and returns its return.

		This allows us to use the 'new' constructor also for
		GUI code:

		 btn = new(Button,10,30,20,100,"Button")

		!!Default parent class: Object

		The default parent class is the Object class, a class
		that implements some basic operations.

		The classes are also derived from the Object type - extending
		the Object class with additional functions (which is always possible)
		will also result in changing the class objects.

		!!Variables

		Any variable (functions are variables) can be defined any time.
		If a request on an object for a variable is made, it will return
		its variable value or will look in the class (or parent class and so on)
		if the variable is defined there. Accessing nil values is the worst case,
		since the whole class tree is looked up. Avoid nil value access by setting
		a variable value to false, if it is looked up more than once and turns out
		to be a performance bottleneck.

		Example:

		 class "Foo"
		 obj = new (Foo)

		 print(obj.x) -- prints nothing or nil
		 Foo.x = 10
		 print(obj.x) -- prints 10
		 obj.x = "bar"
		 print(obj.bar) -- prints "bar"
		 obj.x = nil
		 print(obj.x) -- prints 10
		 Foo.x = nil
		 print(obj.x) -- prints nothing again

		Variables in classes can be used as default values, as
		any object that has no such variable defined will automaticly
		return the value of the class' variable. Variables are
		inherited to all its child classes, also to objects that
		have been defined earlier.

		!!Overloading

		When a class is extended by another class, it can always redefine
		a function, i.e.

		 class "Foo"
		 function Foo:dosomething () print "do something foo" end

		 class "Bar" : extends "Foo"
		 function Bar:dosomehting() print "do something bar" end

		We can also overload or redefine functions when objects have already
		been created. All objects will call automaticly the new function code.

		However, objects can also overload functions or variables:

		 class "Foo"
		 function Foo:dosomething () print "do something foo" end

		 o = new (Foo)
		 function o:dosomething() print "Object code" end
		 print(o:dosomething) -- prints out "Object code"

		!!Spaces

		Classes can be defined inside other tables:

		 somewhere = {}
		 class ("Foo",somewhere)
		 obj = new (somewhere.Foo)

		 class ("Bar",somewhere) : extends (somewhere.Foo)

		The default space is _G, which is why every class is
		automaticly available in the global table.
		]],OO,{
			new = [[
				(object) : (class,...) - creates an object and calls
				class[classname](obj,...)
			]],
			class = [[
				(table) : (name,space = _G) -
				creates a clase in the given space (default _G) and
				returns a table, which contains a function named
				"extends", which accepts a class to be extended, i.e.

				 class "Foo" : extends "Object"

				per default, every class is derived from Object.
			]],
			classparent = [[
				([class parent]) : (class) - returns the parent
				of the class or none if it is the Object class.
			]]

		})



local function toString (self)
   return self.toString and self:toString() or "no known tostring method"
end

local idcnt = 0

local function toString (self) return self.toString and self:toString() or tostring(getmetatable(self).class) end


local classlist = {}

local function luaobjectfactory (class,...)
	assert(not class or classlist[class] or class.new,
		"The given argument ("..tostring(class).." is not a class, nor is it a compatible table")
	local self = {}
	idcnt = idcnt + 1
	local mt = {__index = class, id = idcnt, __tostring = toString, class=class}
	setmetatable(self,mt)
	setmetatable(mt,{__index = class})

	local cname = (class or {}).name
	if cname and class[cname] then
		class[cname](self,...)
	elseif class and class.new then
		self = class.new(class,...)
	end
	return self
end

function new (class,...)
	if type(class)=="string" then class = assert(_G[class],"unknown classname: "..class) end
	if (not class) or (not class.__objectfactory) then
		return luaobjectfactory(class,...)
	end
	return class.__objectfactory(class,...)
end

OO.new = new

function class(name,space)
	space = space or _G
	local self = new()
	local mt = getmetatable(self)
	function mt.__call(tab,...)return new(self,...) end
	self.name = name
	space[name] = self
	classlist[self] = name
	local mod = {}

	local selfclass = self

	function mt.__tostring(self)
		return "[Class: "..(getmetatable(self).class or self).name.."]"
	end
	function mod:extends (o)
		--warning(classlist[class],"The argument to be extended is not a registered class")
		mt.__index = assert(space[o] or o,"cannot extend "..tostring(o).." since it does not exist")
		if mt.__index.new or mt.__index.__objectfactory then
			local pclass = mt.__index
			print("using custom obj factory for ",selfclass)
			local function newobj (class,...)
				if pclass.__lux__ then -- a luxinia class
--					print "LUX CLASS"
					local self = class.new(...)
					--local mt = self.__mt
					--local mtidx = mt.__index
					--function mt:__index(self,key)
						--if
					--end
					return self
				else
--					print "LUA CLASS"
					local self = class:new(...)
					return self
				end
			end
			selfclass.__objectfactory = mt.__index.new and newobj or mt.__index.__objectfactory
--			print(selfclass.__objectfactory)
		end
	end
	if (space[name] or name)~=Object then mt.__index = Object end

	self[name] = function (self,...) -- default construct
		super (selfclass) (self,...)
	end
	if name~="Object" then mod:extends(Object) end

	return mod
end

OO.class = class

function classparent (class)
	class = type(class)=="table" and class or _G[class]
	assert(type(class)=="table", "Argument missing or not table")
	assert(getmetatable(class) and class.name and classlist[class],"Given table does not seem to be a class")
	return getmetatable(class).__index
end

function super (class)
	local p = classparent(class)
	return p and (p[p.name] or p.new) or function () end
end

OO.classparent = classparent

class "Object"

function Object:toString()
	return "[Object: "..(getmetatable(self).class or self).name.." "..getmetatable(self).id.."]"
end

function Object:getTypeName ()
	return (getmetatable(self).class or {}).name or self.name
end

function Object:getClass()
	return getmetatable(self).class or Object
end

function Object:getParentClass()
	return (getmetatable(getmetatable(self).class) or {}).__index or (classlist[self] and classparent(self))
end


--class "Foo" : extends "actornode"
--new ("Foo","hu")


LuxModule.registerclass("luxinialuacore","Object",
	[[
	Parent class of all OO classes. The object oriented system
	is an implementation for an object oriented like programming
	in lua. The Object class provides some basic functionality.
	]],Object,{
		toString = [[
			(string):(object) - returns a string code for the given
			object, you can overload this in order to provide your
			own identification of objects.
		]],
		id = [[
			int id - a counter that is incremented by 1 whenever an object is
			created
		]],
		getTypeName = [[
			(string):(object) - returns name of the class
		]],
		getClass = [[
			(class):(object) - returns class object
		]],
		getParentClass = [[
			([class]):(object) - returns the parent class, if it exists (Object
			does not have a parent class, any other class does).
		]]
	})
