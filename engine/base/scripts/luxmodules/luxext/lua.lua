Clipboard = require("clipboard")
Clipboard.getText,Clipboard.setText = Clipboard.gettext,Clipboard.settext
LuxModule.registerclass("lux_ext","Clipboard",
[[The Clipboard class allows primitive text access on the clipboard.]],
Clipboard,{
getText = "(string):() - returns a string that is currently in the clipboard",
setText = "():(string) - copies a string in the clipboard",
}
)

LuxModule.registerclass("lux_ext","debug",
[[This library provides the functionality of the debug interface to Lua programs.
You should exert care when using this library. The functions provided here should
be used exclusively for debugging and similar tasks, such as profiling. Please
resist the temptation to use them as a usual programming tool: They can be very
slow. Moreover, several of its functions violate some assumptions about Lua code
(e.g., that variables local to a function cannot be accessed from outside or that
userdata metatables cannot be changed by Lua code) and therefore can compromise
otherwise secure code.

All functions in this library are provided inside the debug table. ]],
debug,
{
	debug = [[():() -
			Enters an interactive mode with the user, running each string that
			the user enters. Using simple commands and other debug facilities,
			the user can inspect global and local variables, change their values,
			evaluate expressions, and so on. A line containing only the word
			cont finishes this function, so that the caller continues its
			execution.

			Note that commands for debug.debug are not lexically nested
			within any function, and so have no direct access to
			local variables. ]],
	getfenv = [[(table):(o) - Returns the environment of object o.]],
	gethook = [[(?):() -
				Returns the current hook settings, as three values:
				the current hook function, the current hook mask, and
				the current hook count (as set by the debug.sethook function).]],
	getinfo = [[(?):(function [, what]) -
				Returns a table with information about a function. You can
				give the function directly, or you can give a number as the
				value of function, which means the function running at level
				function of the call stack: Level 0 is the current function
				(getinfo itself); level 1 is the function that called getinfo;
				and so on. If function is a number larger than the number of
				active functions, then getinfo returns nil.

				The returned table contains all the fields returned by
				lua_getinfo, with the string what describing which fields to
				fill in. The default for what is to get all information available.
				If present, the option `f´ adds a field named func with
				the function itself.

				For instance, the expression debug.getinfo(1,"n").name returns a
				name of the current function, if a reasonable name can be found,
				and debug.getinfo(print) returns a table with all available
				information about the print function. ]],
	getlocal = [[(?):(level, local) -
				This function returns the name and the value of the local
				variable with index local of the function at level level of
				the stack. (The first parameter or local variable has index 1,
				and so on, until the last active local variable.) The function
				returns nil if there is no local variable with the given index,
				and raises an error when called with a level out of range.
				(You can call debug.getinfo to check whether the level is valid.)

				Variable names starting with `(´ (open parentheses) represent
				internal variables (loop control variables, temporaries, and C
				function locals). ]],
	getmetatable = [[(table):(object) -
				Returns the metatable of the given object or nil if it does
				not have a metatable. ]],
	getregistry = [[(table):() - Returns the registry table (see 3.5). ]],
	getupvalue = [[(?):(func, up) -
				This function returns the name and the value of the upvalue with
				index up of the function func. The function returns nil if there
				is no upvalue with the given index. ]],
	setfenv = [[(?):(object, table) -
				Sets the environment of the given object to the given table. ]],
	sethook = [[(?):(hook, mask [, count]) -
				Sets the given function as a hook. The string mask and the
				number count describe when the hook will be called. The string
				mask may have the following characters, with the given meaning:
				* "c" --- The hook is called every time Lua calls a function;
				* "r" --- The hook is called every time Lua returns from a function;
				* "l" --- The hook is called every time Lua enters a new line of code.
				With a count different from zero, the hook is called after every
				count instructions.

				When called without arguments, debug.sethook turns off the hook.

				When the hook is called, its first parameter is a string
				describing the event that has triggered its call: "call",
				"return" (or "tail return"), "line", and "count". For line
				events, the hook also gets the new line number as its second
				parameter. Inside a hook, you can call getinfo with level 2 to
				get more information about the running function (level 0 is the
				getinfo function, and level 1 is the hook function), unless the
				event is "tail return". In this case, Lua is only simulating the
				return, and a call to getinfo will return invalid data. ]],
	setlocal = [[(?):(level, local, value) -
				This function assigns the value value to the local variable
				with index local of the function at level level of the stack.
				The function returns nil if there is no local variable with the
				given index, and raises an error when called with a level out of
				range. (You can call getinfo to check whether the level is valid.)
				Otherwise, it returns the name of the local variable. ]],
	setmetatable = [[(?):(object, table) -
				Sets the metatable for the given object to the given table
				(which can be nil). ]],
	setupvalue = [[(?):(func, up, value) -
				This function assigns the value value to the upvalue with
				index up of the function func. The function returns nil if
				there is no upvalue with the given index. Otherwise, it
				returns the name of the upvalue. ]],
	traceback = [[(?):([message]) - Returns a string with a traceback of the call
				stack. An optional message string is appended at the beginning
				of the traceback. This function is typically used with xpcall
				to produce better error messages. ]],
})


LuxModule.registerclass("lux_ext","os",
[[This library is implemented through table os.]],
os,
{
	clock = [[(int seconds):() -
			Returns an approximation of the amount in seconds of CPU time
			used by the program.]],
	date  = [[(string date)([format [, time] ]) -
			Returns a string or a table containing date and time, formatted
			according to the given string format.

			If the time argument is present, this is the time to be formatted
			(see the os.time function for a description of this value).
			Otherwise, date formats the current time.

			If format starts with `!´, then the date is formatted in Coordinated
			Universal Time. After this optional character, if format is *t,
			then date returns a table with the following fields: year (four digits),
			month (1--12), day (1--31), hour (0--23), min (0--59), sec (0--61),
			wday (weekday, Sunday is 1), yday (day of the year), and isdst
			(daylight saving flag, a boolean).

			If format is not *t, then date returns the date as a string,
			formatted according to the same rules as the C function strftime.

			When called without arguments, date returns a reasonable date and
			time representation that depends on the host system and on the
			current locale (that is, os.date() is equivalent to os.date("%c")).]],
	difftime = [[(?):(t2, t1) -
			Returns the number of seconds from time t1 to time t2. In POSIX,
			Windows, and some other systems, this value is exactly t2-t1. ]],
	execute  = [[(?):([command]) -
			This function is equivalent to the C function system. It passes
			command to be executed by an operating system shell. It returns a
			status code, which is system-dependent. If command is absent, then
			it returns nonzero if a shell is available and zero otherwise. ]],
	exit = [[(?):([code]) -
			Calls the C function exit, with an optional code, to terminate
			the host program. The default value for code is the success code. ]],
	getenv = [[(?):(varname) -
			Returns the value of the process environment variable varname, or
			nil if the variable is not defined.]],
	remove = [[(?):(filename) -
			Deletes the file or directory with the given name. Directories must
			be empty to be removed. If this function fails, it returns nil,
			plus a string describing the error. ]],
	rename = [[(?):(oldname, newname) -
			Renames file or directory named oldname to newname. If this
			function fails, it returns nil, plus a string describing the error.]],
	setlocale = [[(?):(locale [, category]) -
			Sets the current locale of the program. locale is a string specifying
			a locale; category is an optional string describing which category to
			change: "all", "collate", "ctype", "monetary", "numeric", or "time";
			the default category is "all". The function returns the name of
			the new locale, or nil if the request cannot be honored. ]],
	time = [[(int time):([table]) -
			Returns the current time when called without arguments, or a time
			representing the date and time specified by the given table. This
			table must have fields year, month, and day, and may have fields
			hour, min, sec, and isdst (for a description of these fields,
			see the os.date function).

			The returned value is a number, whose meaning depends on your system.
			In POSIX, Windows, and some other systems, this number counts the
			number of seconds since some given start time (the "epoch"). In
			other systems, the meaning is not specified, and the number returned
			by time can be used only as an argument to date and difftime. ]],
	tmpname = [[(string):() -
			Returns a string with a file name that can be used for a
			temporary file. The file must be explicitly opened before its use
			and explicitly removed when no longer needed. ]],

})

LuxModule.registerclass("lux_ext","io",
[[
The I/O library provides two different styles for file manipulation. The first
one uses implicit file descriptors; that is, there are operations to set a
default input file and a default output file, and all input/output operations
are over these default files. The second style uses explicit file descriptors.

When using implicit file descriptors, all operations are supplied by table io.
When using explicit file descriptors, the operation io.open returns a file
descriptor and then all operations are supplied as methods of the file
descriptor.

The table io also provides three predefined file descriptors with their usual
meanings from C: io.stdin, io.stdout, and io.stderr.

Unless otherwise stated, all I/O functions return nil on failure (plus an error
message as a second result) and some value different from nil on success.
]],
io,
{
	close = [[(?):([file]) -
			Equivalent to file:close(). Without a file, closes the default
			output file. ]],
	flush = [[(?):() - Equivalent to file:flush over the default output file. ]],
	input = [[(?):([file]) -
			When called with a file name, it opens the named file (in text
			mode), and sets its handle as the default input file. When
			called with a file handle, it simply sets this file handle as the
			default input file. When called without parameters, it returns
			the current default input file.

			In case of errors this function raises the error, instead of
			returning an error code. ]],
	lines = [[(?):([filename]) -
			Opens the given file name in read mode and returns an iterator
			function that, each time it is called, returns a new line from
			the file. Therefore, the construction

			 for line in io.lines(filename) do ... end

			will iterate over all lines of the file. When the iterator
			function detects the end of file, it returns nil (to finish
			the loop) and automatically closes the file.

			The call io.lines() (without a file name) is equivalent to
			io.input():lines(); that is, it iterates over the lines of
			the default input file. In this case it does not close the
			file when the loop ends. ]],
	open = [[(?):(filename [, mode]) -
			This function opens a file, in the mode specified in the string mode.
			It returns a new file handle, or, in case of errors, nil plus an
			error message.

			The mode string can be any of the following:

			* "r" --- read mode (the default);
			* "w" --- write mode;
			* "a" --- append mode;
			* "r+" --- update mode, all previous data is preserved;
			* "w+" --- update mode, all previous data is erased;
			* "a+" --- append update mode, previous data is preserved, writing is
			only allowed at the end of file.

			The mode string may also have a `b´ at the end, which is needed in
			some systems to open the file in binary mode. This string is exactly
			what is used in the standard C function fopen.]],
	output = [[(?):([file]) - Similar to io.input, but operates
			over the default output file.]],
	popen = [[(?):([prog [, mode] ]) -
			Starts program prog in a separated process and returns a file
			handle that you can use to read data from this program
			(if mode is "r", the default) or to write data to this program
			(if mode is "w").

			This function is system dependent and is not available on all platforms.]],
	read = [[(?):(format1, ...) - Equivalent to io.input():read. ]],
	tmpfile = [[(?):() - Returns a handle for a temporary file. This file is
			opened in update mode and it is automatically removed when the
			program ends. ]],
	type = [[(obj):() - Checks whether obj is a valid file handle.
			Returns the string "file" if obj is an open file handle,
			"closed file" if obj is a closed file handle, or nil if obj is
			not a file handle.]],
	write = [[(?):(value1, ...) - Equivalent to io.output():write. ]],
	["file:close"] = [[(?):() - Closes file. Note that files are automatically closed
			when their handles are garbage collected, but that takes an
			unpredictable amount of time to happen. ]],
	["file:flush"] = [[(?):() - Saves any written data to file. ]],
	["file:lines"] = [[(?):() - Returns an iterator function that, each time it
			is called, returns a new line from the file. Therefore,
			the construction

			 for line in file:lines() do ... end

			will iterate over all lines of the file. (Unlike io.lines,
			this function does not close the file when the loop ends.)]],
	["file:read"] = [[(?):(format1, ...) -
			Reads the file file, according to the given formats, which specify
			what to read. For each format, the function returns a string
			(or a number) with the characters read, or nil if it cannot
			read data with the specified format. When called without formats, it
			uses a default format that reads the entire next line (see below).

			The available formats are

			* "*n" reads a number; this is the only format that returns a
			number instead of a string.
			* "*a" reads the whole file, starting at the current position.
			On end of file, it returns the empty string.
			* "*l" reads the next line (skipping the end of line), returning
			nil on end of file. This is the default format.
			* number reads a string with up to this number of characters,
			returning nil on end of file. If number is zero, it reads nothing
			and returns an empty string, or nil on end of file. ]],
	["file:seek"] = [[():([whence] [, offset]) -
			Sets and gets the file position, measured from the beginning of
			the file, to the position given by offset plus a base specified by
			the string whence, as follows:

			* "set" --- base is position 0 (beginning of the file);
			* "cur" --- base is current position;
			* "end" --- base is end of file;

			In case of success, function seek returns the final file position,
			measured in bytes from the beginning of the file. If this function
			fails, it returns nil, plus a string describing the error.

			The default value for whence is "cur", and for offset is 0. Therefore,
			the call file:seek() returns the current file position, without
			changing it; the call file:seek("set") sets the position to the
			beginning of the file (and returns 0); and the call file:seek("end")
			sets the position to the end of the file, and returns its size. ]],
	["file:setvbuf"] = [[(?):(mode [, size]) -
			Sets the buffering mode for an output file.
			There are three available modes:

			* "no" --- no buffering; the result of any output operation appears
			immediately.
			* "full" --- full buffering; output operation is performed only when
			the buffer is full (or when you explicitly flush the file (see 5.7)).
			* "line" --- line buffering; output is buffered until a newline is
			output or there is any input from some special files (such as a
			terminal device).

			For the last two cases, sizes specifies the size of the buffer,
			in bytes. The default is an appropriate size.]],
	["file:write"] = [[(?):(value1, ...) - Writes the value of each of its
			arguments to the file. The arguments must be strings or numbers.
			To write other values, use tostring or string.format before write.]],

})

LuxModule.registerclass("lux_ext","math",
[[This library is an interface to the standard C math library. It provides all
its functions inside the table math. The library provides the following functions:

       math.abs     math.acos    math.asin    math.atan    math.atan2
       math.ceil    math.cos     math.cosh    math.deg     math.exp
       math.floor   math.fmod    math.frexp   math.ldexp   math.log
       math.log10   math.max     math.min     math.modf    math.pow
       math.rad     math.random  math.randomseed           math.sin
       math.sinh    math.sqrt    math.tan     math.tanh

plus a variable math.pi and a variable math.huge, with the value HUGE_VAL.
Most of these functions are only interfaces to the corresponding functions in
the C library. All trigonometric functions work in radians. The functions
math.deg and math.rad convert between radians and degrees.

The function math.max returns the maximum value of its numeric arguments.
Similarly, math.min computes the minimum. Both can be used with 1, 2, or
more arguments.

The function math.modf corresponds to the modf C function. It returns two
values: The integral part and the fractional part of its argument. The
function math.frexp also returns 2 values: The normalized fraction and the
exponent of its argument.

The functions math.random and math.randomseed are interfaces to the simple
random generator functions rand and srand that are provided by ANSI C.
(No guarantees can be given for their statistical properties.) When called
without arguments, math.random returns a pseudo-random real number in the
range [0,1). When called with a number n, math.random returns a pseudo-random
integer in the range [1,n]. When called with two arguments, l and u,
math.random returns a pseudo-random integer in the range [l,u].
The math.randomseed function sets a "seed" for the pseudo-random generator:
Equal seeds produce equal sequences of numbers. ]],
math,
{})

function math.sqdist3(a,b)
	local dx,dy,dz = a[1]-b[1],a[2]-b[2],a[3]-b[3]
	return dx*dx + dy*dy + dz*dz
end

function math.round(x)
	return math.floor(x+0.5)
end

LuxModule.registerclass("lux_ext","table",
[[This library provides generic functions for table manipulation. It provides
all its functions inside the table table.

Most functions in the table library assume that the table represents an array
or a list. For these functions, when we talk about the "length" of a table we
mean the result of the length operator. ]],table,
{
	concat = [[(string):(table [, sep [, i [, j] ] ]) -
			Returns table[i]..sep..table[i+1] ... sep..table[j]. The default
			value for sep is the empty string, the default for i is 1, and
			the default for j is the length of the table. If i is greater than
			j, returns the empty string.]],
	insert = [[(?):(table, [pos,] value) -
			Inserts element value at position pos in table, shifting up
			other elements to open space, if necessary. The default value
			for pos is n+1, where n is the length of the table,
			so that a call table.insert(t,x) inserts x at the end of table t. ]],
	maxn = [[(int n):(table) - Returns the largest positive numerical index of
			the given table, or zero if the table has no positive numerical
			indices. (To do its job this function does a linear traversal of
			the whole table.) ]],
	remove = [[(value):(table [, pos]) -
			Removes from table the element at position pos, shifting down other
			elements to close the space, if necessary. Returns the value of the
			removed element. The default value for pos is n, where n is the length
			of the table, so that a call table.remove(t) removes the last
			element of table t. ]],
	sort = [[(?):(table [, comp]) -
			Sorts table elements in a given order, in-place, from table[1] to
			table[n], where n is the length of the table. If comp is given,
			then it must be a function that receives two table elements, and
			returns true when the first is less than the second (so that not
			comp(a[i+1],a[i]) will be true after the sort). If comp is not
			given, then the standard Lua operator < is used instead.

			The sort algorithm is not stable; that is, elements considered equal
			by the given order may have their relative positions changed by the
			sort.]],
	reverse = [[(table):(table) - reverses the indexes of the table (inplace).
			Returns the same table that was passed.]],
	copy = [[(table):(table) - makes a simple copy of that table, without respect to refered
			tables or values inside of that table]],
	removen = [[(value1, value2, ...):(table,int n[, int at) -
			removes n elements from. If 'at' is given, the function removes all
			values from that index on, decrementing the at index with each remove.
			The order of the removed objects is "normal", which means that it is
			not changed.]],
	merge = [[(table):(table1,table2,...) - copies all passed tables in one table one after each other,
			overwriting previous keys with new values.]],
	count = [[(int):(table) - returns total number of elements via complete traversion]],
	keyintersection = [[(table):(table1,table2,...) - returns a table that is a copy of table1
			without keys that are missing in the following tables.]],
	print = [[():(table) - prints out all keys and their values - useful for debugging]]
})

function table.reverse (tab)
	local n = table.getn(tab)
	for i=1,math.floor(n/2) do
		tab[i] = tab[n-i+1]
	end
	return tab
end

function table.count (tab)
	local cnt = 0
	for i,v in pairs(tab) do cnt = cnt + 1 end
	return cnt
end

function table.copy (tab)
	local copy = {}
	for i,v in pairs(tab) do copy[i] = v end
	return copy
end

function table.removen (tab,i,at)
	local rem = {}
	local n = #tab
	at = at or (n-i)
	for j=at+1,at+i do
		table.insert(rem,tab[j])
	end
	for j=1,i do table.remove(tab,at+1) end
	return unpack(rem)
end

function table.merge (...)
	local tab = {}
	
	for i=1,select('#',...) do
		local v = select(i,...)
		for i,v in pairs(v) do
			tab[i] = v
		end
	end
	return tab
end

function table.keyintersection (...)
	local tab
	for i=1,select('#',...) do
		local v = select(i,...)
		if (type(v)=="table") then

			if (tab) then
				for a,b in pairs(tab) do
					if (not v[a]) then tab[a] = nil end
				end
			else
				tab = table.copy(v)
			end
		end
	end
	return tab or {}
end

function table.print(tab,maxd)
	maxd = maxd or 100
	local norec = {}

	local function out (key,tab,ind,dep)
		if dep>maxd then return end
		--print(ind..tostring(key).." = "..tostring(tab))
		local function pr (pre,tab)
			print(ind..pre.." = "..(type(tab)=='table' and ("("..#tab..") ") or "")..tostring(tab))
			if type(tab)=="table" and not norec[tab] then
				norec[tab] = true
				for i,v in pairs(tab) do
					out(i,v,ind.."  ",dep + 1)
				end
			end
		end
		pr("key  ",key)
		pr("  val",tab)
	end
	out("table",tab,"",0)
end


GarbageCollector = {}
LuxModule.registerclass("lux_ext","GarbageCollector",
[[The GarbageCollector is a wrapper for the collectgarbage function for simplification.]],
GarbageCollector,
{
	stop = [[():() - stops the garbage collector.]],
	restart = [[():() - restarts the garbage collector.]],
	collect = [[():() - performs a full garbage-collection cycle.]],
	count = [[(double mem):() - returns the total memory in use by Lua (in Kbytes).]],
	step = [[(boolean):(int n) - performs a garbage-collection step. The step "size" is
			controlled by arg (larger values mean more steps) in a non-specified
			way. If you want to control the step size you must tune experimentally
			the value of arg. Returns true if the step finished a collection cycle.]],
	setPause = [[(float previous):(float pause) - sets arg/100 as the new value
			for the pause of the collector]],
	setStepMul = [[(float previous):(float mulstepsize) - sets arg/100 as the
			new value for the step multiplier of the collector.]],
})
function GarbageCollector.stop() collectgarbage("stop") end
function GarbageCollector.restart() collectgarbage("restart") end
function GarbageCollector.collect() collectgarbage("collect") end
function GarbageCollector.count() return collectgarbage("count") end
function GarbageCollector.step(n) return collectgarbage("step",n) end
function GarbageCollector.setPause(p) return collectgarbage("setpause",p) end
function GarbageCollector.setStepMul(s) return collectgarbage("setstepmul",s) end

basicfunctions = {}
LuxModule.registerclass("lux_ext","baselib",
[[All functions in this "class" are global values that can be used directly.

The basic library provides some core functions to Lua. If you do not include
this library in your application, you should check carefully whether you need
to provide implementations for some of its facilities.]],basicfunctions,
{
	assert = [[(arg1,arg2,...):(boolean condition, [string msg]) - Issues an error when the
		value of its argument v is false (i.e., nil or false); otherwise, returns
		all its arguments. message is an error message; when absent, it defaults
		to "assertion failed!"]],
	collectgarbage = [[([table/value]):(string opt [, arg]) -
			This function is a generic interface to the garbage collector.
			It performs different
			functions according to its first argument, opt:
			* "stop" --- stops the garbage collector.
			* "restart" --- restarts the garbage collector.
			* "collect" --- performs a full garbage-collection cycle.
			* "count" --- returns the total memory in use by Lua (in Kbytes).
			* "step" --- performs a garbage-collection step. The step "size" is
			controlled by arg (larger values mean more steps) in a non-specified
			way. If you want to control the step size you must tune experimentally
			the value of arg. Returns true if the step finished a collection cycle.
			* "setpause" --- sets arg/100 as the new value for the pause of the
			collector.
			* "setstepmul" --- sets arg/100 as the new value for the step multiplier
			of the collector. ]],
	dofile = [[([returnvalues,...]):(string file, [arg1, ...]) -
			Opens the named file and executes its contents as a Lua chunk. When
			called without arguments, dofile executes the contents of the standard
			input (stdin). Returns all values returned by the chunk. In case of
			errors, dofile propagates the error to its caller (that is, dofile
			does not run in protected mode).

			The original lua dofile required relative pathes to the file location.
			This is different in Luxinia: Luxinia looks for a file recursivly in all
			pathes relative to each file that was called. ]],
	error = [[():(string message [, int level]) -
			Terminates the last protected function called and returns message as
			the error message. Function error never returns.

			Usually, error adds some information about the error position at the
			beginning of the message. The level argument specifies how to get
			the error position. With level 1 (the default), the error position
			is where the error function was called. Level 2 points the error to
			where the function that called error was called; and so on. Passing
			a level 0 avoids the addition of error position information to the
			message. ]],
	_G = [[[table] - A global variable (not a function) that holds the global
			environment (that is, _G._G = _G). Lua itself does not use this
			variable; changing its value does not affect any environment, nor
			vice-versa. (Use setfenv to change environments.)]],
	getfenv = [[([table]):(function f) - Returns the current environment in use
			by the function. f can be a Lua function or a number that specifies
			the function at that stack level: Level 1 is the function calling
			getfenv. If the given function is not a Lua function, or if f is 0,
			getfenv returns the global environment. The default for f is 1.]],
	getmetatable = [[([table]):(object) - If object does not have a metatable,
			returns nil. Otherwise, if the object's metatable has a "__metatable"
			field, returns the associated value. Otherwise, returns the metatable
			of the given object. ]],
	ipairs = [[(function iterator, table t, 0):(table t) -
			Returns three values: an iterator function, the table t, and 0,
			so that the construction

			 for i,v in ipairs(t) do ... end

			will iterate over the pairs (1,t[1]), (2,t[2]), ..., up to the first
			integer key with a nil value in the table.

			See @@next@@ for the caveats of modifying the table during its traversal.]],
	load 	= [[():(func [, string chunkname]) -
			Loads a chunk using function func to get its pieces. Each call to
			func must return a string that concatenates with previous results.
			A return of nil (or no value) signals the end of the chunk.

			If there are no errors, returns the compiled chunk as a function;
			otherwise, returns nil plus the error message. The environment of the
			returned function is the global environment.

			chunkname is used as the chunk name for error messages and debug
			information. ]],
	loadfile = [[(function f):([string path]) -
			Similar to load, but gets the chunk from file filename or from the
			standard input, if no file name is given.]],
	loadstring = [[(function f):(string [, chunkname]) -
			Similar to load, but gets the chunk from the given string.

			To load and run a given string, use the idiom

			 assert(loadstring(s))()
			]],
	next = [[(?):(table [, index]) -
			Allows a program to traverse all fields of a table. Its first
			argument is a table and its second argument is an index in this
			table. next returns the next index of the table and its associated
			value. When called with nil as its second argument, next returns
			an initial index and its associated value. When called with the
			last index, or with nil in an empty table, next returns nil. If
			the second argument is absent, then it is interpreted as nil.
			In particular, you can use next(t) to check whether a table is empty.

			Lua has no declaration of fields. There is no difference between a field
			not present in a table or a field with value nil. Therefore,
			next only considers fields with non-nil values. The order in which
			the indices are enumerated is not specified, even for numeric
			indices. (To traverse a table in numeric order, use a numerical
			for or the ipairs function.)

			The behavior of next is undefined if, during the traversal, you
			assign any value to a non-existent field in the table. You may
			however modify existing fields. In particular, you may clear
			existing fields. ]],
	pairs = [[(?):(t) - Returns three values: the next function, the table t,
			and nil, so that the construction

			 for k,v in pairs(t) do ... end

			will iterate over all key--value pairs of table t.

			See next for the caveats of modifying the table during its traversal.]],
	pcall = [[(?):(f, arg1, arg2, ...) -
			Calls function f with the given arguments in protected mode. This
			means that any error inside f is not propagated; instead, pcall
			catches the error and returns a status code. Its first result is the
			status code (a boolean), which is true if the call succeeds without
			errors. In such case, pcall also returns all results from the call,
			after this first result. In case of any error, pcall returns false
			plus the error message.]],
	print = [[(?):(e1, e2, ...) -
			Receives any number of arguments, and prints their values to stdout,
			using the tostring function to convert them to strings. print is not
			intended for formatted output, but only as a quick way to show a
			value, typically for debugging. For formatted output, use
			string.format.]],
	rawequal = [[(?):(v1, v2) -
			Checks whether v1 is equal to v2, without invoking any metamethod.
			Returns a boolean.]],
	rawget = [[(?):(table, index) -
			Gets the real value of table[index], without invoking any metamethod.
			table must be a table and index any value different from nil.]],
	rawset = [[(?):(table, index, value) -
			Sets the real value of table[index] to value, without invoking any
			metamethod. table must be a table, index any value different from
			nil, and value any Lua value.]],
	select = [[(?):(index, ...) -
			If index is a number, returns all arguments after argument number
			index. Otherwise, index must be the string "#", and select returns
			the total number of extra arguments it received.]],
	setfenv = [[(?):(f, table) -
			Sets the environment to be used by the given function. f can be a
			Lua function or a number that specifies the function at that stack
			level: Level 1 is the function calling setfenv. setfenv returns the
			given function.

			As a special case, when f is 0 setfenv changes the environment of
			the running thread. In this case, setfenv returns no values. ]],
	tonumber = [[(?):(e [, base]) -
			Tries to convert its argument to a number. If the argument is
			already a number or a string convertible to a number, then tonumber
			returns this number; otherwise, it returns nil.

			An optional argument specifies the base to interpret the numeral.
			The base may be any integer between 2 and 36, inclusive. In bases
			above 10, the letter `A´ (in either upper or lower case) represents
			10, `B´ represents 11, and so forth, with `Z´ representing 35. In
			base 10 (the default), the number may have a decimal part, as well
			as an optional exponent part (see 2.1). In other bases, only unsigned
			integers are accepted. ]],
	tostring = [[(?):(e) -
			Receives an argument of any type and converts it to a string in a
			reasonable format. For complete control of how numbers are converted,
			use string.format.

			If the metatable of e has a "__tostring" field, then tostring calls
			the corresponding value with e as argument, and uses the result of
			the call as its result. ]],
	type = [[(?):(v) -
			Returns the type of its only argument, coded as a string. The possible
			results of this function are "nil" (a string, not the value nil),
			"number", "string", "boolean, "table", "function", "thread", and
			"userdata".]],
	unpack = [[(?):(list [, i [, j] ]) -
			Returns the elements from the given table. This function is equivalent to

			 return list[i], list[i+1], ..., list[j]

			except that the above code can be written only for a fixed number of
			elements. By default, i is 1 and j is the length of the list, as
			defined by the length operator.]],
	_VERSION = [[ [string] - A global variable (not a function) that holds a
			string containing the current interpreter version. The current
			contents of this variable is "Lua 5.1".]],
	xpcall = [[(?):(f, err) -
			This function is similar to pcall, except that you can set a
			new error handler.

			xpcall calls function f in protected mode, using err as the error
			handler. Any error inside f is not propagated; instead, xpcall
			catches the error, calls the err function with the original error
			object, and returns a status code. Its first result is the status
			code (a boolean), which is true if the call succeeds without errors.
			In this case, xpcall also returns all results from the call, after
			this first result. In case of any error, xpcall returns false plus
			the result from err. ]],
})



LuxModule.registerclass("lux_ext","coroutine",
[[The operations related to coroutines comprise a sub-library of the basic
library and come inside the table coroutine. See 2.11 for a general description
of coroutines.]],coroutine,
{
	create = [[(?):(function f) -
			Creates a new coroutine, with body f. f must be a Lua function.
			Returns this new coroutine, an object with type "thread". ]],
	resume = [[(?):(co [, val1, ..., valn]) -
			Starts or continues the execution of coroutine co. The first time
			you resume a coroutine, it starts running its body. The values val1,
			..., valn are passed as the arguments to the body function. If the
			coroutine has yielded, resume restarts it; the values val1, ...,
			valn are passed as the results from the yield.

			If the coroutine runs without any errors, resume returns true plus
			any values passed to yield (if the coroutine yields) or any values
			returned by the body function (if the coroutine terminates). If
			there is any error, resume returns false plus the error message. ]],
	running = [[(?):() - Returns the running coroutine, or nil when
			called by the main thread. ]],
	status = [[(?):(co) - Returns the status of coroutine co, as a string:
			"running", if the coroutine is running (that is, it called status);
			"suspended", if the coroutine is suspended in a call to yield, or
			if it has not started running yet; "normal" if the coroutine is
			active but not running (that is, it has resumed another coroutine);
			and "dead" if the coroutine has finished its body function, or if
			it has stopped with an error. ]],
	wrap = [[(?):(f) -
			Creates a new coroutine, with body f. f must be a Lua function.
			Returns a function that resumes the coroutine each time it is called.
			Any arguments passed to the function behave as the extra arguments
			to resume. Returns the same values returned by resume, except the
			first boolean. In case of error, propagates the error. ]],
	yield = [[(?):([val1, ..., valn]) -
			Suspends the execution of the calling coroutine. The coroutine cannot
			be running a C function, a metamethod, or an iterator. Any arguments
			to yield are passed as extra results to resume. ]],
})



LuxModule.registerclass("lux_ext","package",
[[The package library provides basic facilities for loading and building
modules in Lua. It exports two of its functions directly in the global
environment: require and module. Everything else is exported in a table package.]],
package,
{
	cpath = [[[string] -
			The path used by require to search for a C loader.

			Lua initializes the C path package.cpath in the same way it initializes
			the Lua path package.path, using the environment variable LUA_CPATH
			(plus another default path defined in luaconf.h). ]],
	loaded = [[[table] -
			A table used by require to control which modules are already loaded.
			When you require a module modname and package.loaded[modname] is
			not false, require simply returns the value stored there. ]],
	loadlib = [[(?):(string libname, string funcname) -
			Dynamically links the host program with the C library libname.
			Inside this library, looks for a function funcname and returns this
			function as a C function. (So, funcname must follow the protocol
			(see lua_CFunction)).

			This is a low-level function. It completely bypasses the package
			and module system. Unlike require, it does not perform any path
			searching and does not automatically adds extensions. libname must
			be the complete file name of the C library, including if necessary
			a path and extension. funcname must be the exact name exported by
			the C library (which may depend on the C compiler and linker used).

			This function is not supported by ANSI C. As such, it is only
			available on some platforms (Windows, Linux, Mac OS X, Solaris,
			BSD, plus other Unix systems that support the dlfcn standard). ]],
	path = [[[string] -
			The path used by require to search for a Lua loader.

			At start-up, Lua initializes this variable with the value of the
			environment variable LUA_PATH or with a default path defined in
			luaconf.h, if the environment variable is not defined. Any ";;"
			in the value of the environment variable is replaced by the default
			path.

			A path is a sequence of templates separated by semicolons. For each
			template, require will change each interrogation mark in the
			template by filename, which is modname with each dot replaced by a
			"directory separator" (such as "/" in Unix); then it will try to
			load the resulting file name. So, for instance, if the Lua path
			is

			  "./?.lua;./?.lc;/usr/local/?/init.lua"

			the search for a Lua loader for module foo will try to load the
			files ./foo.lua, ./foo.lc, and /usr/local/foo/init.lua,
			in that order.]],
	preload = "[table] - A table to store loaders for specific modules (see require).",
	seeall = [[(?):(module) - Sets a metatable for module with its __index field
			referring to the global environment, so that this module inherits
			values from the global environment. To be used as an option to
			function module. ]]
})




LuxModule.registerclass("lux_ext","string",
[[From the official Lua Reference (http://www.lua.org):

This library provides generic functions for string manipulation, such as
finding and extracting substrings, and pattern matching. When indexing a
string in Lua, the first character is at position 1 (not at 0, as in C).
Indices are allowed to be negative and are interpreted as indexing backwards,
from the end of the string. Thus, the last character is at position -1, and so on.

The string library provides all its functions inside the table string. It
also sets a metatable for strings where the __index field points to the
metatable itself. Therefore, you can use the string functions in
object-oriented style. For instance, string.byte(s, i) can be written as
s:byte(i).

!!Patterns

A character class is used to represent a set of characters. The following
combinations are allowed in describing a character class:

* x (where x is not one of the magic characters ^$()%.[]*+-?) --- represents the
character x itself.
* . --- (a dot) represents all characters.
* %a --- represents all letters.
* %c --- represents all control characters.
* %d --- represents all digits.
* %l --- represents all lowercase letters.
* %p --- represents all punctuation characters.
* %s --- represents all space characters.
* %u --- represents all uppercase letters.
* %w --- represents all alphanumeric characters.
* %x --- represents all hexadecimal digits.
* %z --- represents the character with representation 0.
* %x (where x is any non-alphanumeric character) --- represents the character x.
This is the standard way to escape the magic characters. Any punctuation character
(even the non magic) can be preceded by a `%´ when used to represent itself in a pattern.

* [set] --- represents the class which is the union of all characters in set. A
range of characters may be specified by separating the end characters of the range
with a `-´. All classes %x described above may also be used as components in set.
All other characters in set represent themselves. For example, [%w_] (or [_%w])
represents all alphanumeric characters plus the underscore, [0-7] represents the
octal digits, and [0-7%l%-] represents the octal digits plus the lowercase letters
plus the `-´ character.

The interaction between ranges and classes is not defined. Therefore, patterns
like [%a-z] or [a-%%] have no meaning.

* [^set] --- represents the complement of set, where set is interpreted as above.

For all classes represented by single letters (%a, %c, etc.), the corresponding
uppercase letter represents the complement of the class. For instance, %S represents
all non-space characters.

The definitions of letter, space, and other character groups depend on the current
locale. In particular, the class [a-z] may not be equivalent to %l.

A pattern item may be

* a single character class, which matches any single character in the class;
* a single character class followed by `*´, which matches 0 or more repetitions
of characters in the class. These repetition items will always match the longest
possible sequence;
* a single character class followed by `+´, which matches 1 or more repetitions
of characters in the class. These repetition items will always match the longest
possible sequence;
* a single character class followed by `-´, which also matches 0 or more repetitions
of characters in the class. Unlike `*´, these repetition items will always match the
shortest possible sequence;
* a single character class followed by `?´, which matches 0 or 1 occurrence of a
character in the class;
* %n, for n between 1 and 9; such item matches a substring equal to the n-th
captured string (see below);
* %bxy, where x and y are two distinct characters; such item matches strings
that start with x, end with y, and where the x and y are balanced. This means
that, if one reads the string from left to right, counting +1 for an x and -1
for a y, the ending y is the first y where the count reaches 0. For instance,
the item %b() matches expressions with balanced parentheses.

A pattern is a sequence of pattern items. A `^´ at the beginning of a pattern
anchors the match at the beginning of the subject string. A `$´ at the end of
a pattern anchors the match at the end of the subject string. At other positions,
`^´ and `$´ have no special meaning and represent themselves.

A pattern may contain sub-patterns enclosed in parentheses; they describe
captures. When a match succeeds, the substrings of the subject string that
match captures are stored (captured) for future use. Captures are numbered
according to their left parentheses. For instance, in the pattern "(a*(.)%w(%s*))",
the part of the string matching "a*(.)%w(%s*)" is stored as the first
capture (and therefore has number 1); the character matching "." is captured
with number 2, and the part matching "%s*" has number 3.

As a special case, the empty capture () captures the current string position
(a number). For instance, if we apply the pattern "()aa()" on the string "flaaap",
there will be two captures: 3 and 5.

A pattern cannot contain embedded zeros. Use %z instead. ]],
string,{
	byte 	= [[(int c1,...):(string str, [int i [,int j] ]) -
Returns the internal numerical codes of the characters s[i], s[i+1], ..., s[j].
The default value for i is 1; the default value for j is i.

Note that numerical codes are not necessarily portable across platforms. ]],

	char 	= [[(string str):(int i1, int i2, ...) -
Receives 0 or more integers. Returns a string with length equal to the number of
arguments, in which each character has the internal numerical code equal to its
corresponding argument.

Note that numerical codes are not necessarily portable across platforms.
]],
	dump 	= [[(string binarybytecode):(function f) -
Returns a string containing a binary representation of the given function,
so that a later loadstring on this string returns a copy of the function.
function must be a Lua function without upvalues.]],
--[=[
	dumpstripped = [[(string binarybytecode):(function f) -
Returns a string containing a binary representation of the given function,
so that a later loadstring on this string returns a copy of the function.

Any debug information is stripped out here. ]],
]=]
	find 	= [[([int start, int end, string captures,...]):
(string s, string pattern, [int init[, plain] ]) -
Looks for the first match of pattern in the string s.

If it finds a match, then find returns the indices of s where this occurrence
starts and ends; otherwise, it returns nil. A third, optional numerical argument
init specifies where to start the search; its default value is 1 and may be
negative. A value of true as a fourth, optional argument plain turns off the
pattern matching facilities, so the function does a plain "find substring"
operation, with no characters in pattern being considered "magic". Note that
if plain is given, then init must be given as well.

If the pattern has captures, then in a successful match the captured values
are also returned, after the two indices. ]],

	format 	= [[(string formatted):(string format, e1, e2 ...) -
Returns a formatted version of its variable number of arguments following the
description given in its first argument (which must be a string). The format
string follows the same rules as the printf family of standard C functions.
The only differences are that the options/modifiers *, l, L, n, p, and h are
not supported and that there is an extra option, q. The q option formats a
string in a form suitable to be safely read back by the Lua interpreter: The
string is written between double quotes, and all double quotes, newlines,
embedded zeros, and backslashes in the string are correctly escaped when
written. For instance, the call

 string.format('%q', 'a string with "quotes" and \n new line')

will produce the string:

"a string with \"quotes\" and \
 new line"

The options c, d, E, e, f, g, G, i, o, u, X, and x all expect a number as
argument, whereas q and s expect a string.

This function does not accept string values containing embedded zeros. ]],

	gmatch 	= [[(function iterator):(string s, string pattern) -
Returns an iterator function that, each time it is called, returns the next
captures from pattern over string s.

If pattern specifies no captures, then the whole match is produced in each call.

As an example, the following loop

  s = "hello world from Lua"
  for w in string.gmatch(s, "%a+") do
    print(w)
  end

will iterate over all the words from string s, printing one per line. The next
example collects all pairs key=value from the given string into a table:

  t = {}
  s = "from=world, to=Lua"
  for k, v in string.gmatch(s, "(%w+)=(%w+)") do
    t[k] = v
  end
]],
	gsub	= [[():() -
Returns a copy of s in which all occurrences of the pattern have been replaced
by a replacement string specified by repl, which may be a string, a table, or
a function. gsub also returns, as its second value, the total number of
substitutions made.

If repl is a string, then its value is used for replacement. The character %
works as an escape character: Any sequence in repl of the form %n, with n
between 1 and 9, stands for the value of the n-th captured substring (see below).
The sequence %0 stands for the whole match. The sequence %% stands for a single %.

If repl is a table, then the table is queried for every match, using the first
capture as the key; if the pattern specifies no captures, then the whole match
is used as the key.

If repl is a function, then this function is called every time a match occurs,
with all captured substrings passed as arguments, in order; if the pattern
specifies no captures, then the whole match is passed as a sole argument.

If the value returned by the table query or by the function call is a string or
a number, then it is used as the replacement string; otherwise, if it is false
or nil, then there is no replacement (that is, the original match is kept in
the string).

The optional last parameter n limits the maximum number of substitutions
to occur. For instance, when n is 1 only the first occurrence of pattern
is replaced.

Here are some examples:

   x = string.gsub("hello world", "(%w+)", "%1 %1")
   --> x="hello hello world world"

   x = string.gsub("hello world", "%w+", "%0 %0", 1)
   --> x="hello hello world"

   x = string.gsub("hello world from Lua", "(%w+)%s*(%w+)", "%2 %1")
   --> x="world hello Lua from"

   x = string.gsub("home = $HOME, user = $USER", "%$(%w+)", os.getenv)
   --> x="home = /home/roberto, user = roberto"

   x = string.gsub("4+5 = $return 4+5$", "%$(.-)%$", function (s)
         return loadstring(s)()
       end)
   --> x="4+5 = 9"

   local t = {name="lua", version="5.1"}
   x = string.gsub("$name%-$version.tar.gz", "%$(%w+)", t)
   --> x="lua-5.1.tar.gz"
]],
	len 	= [[(int n):(string s) - Receives a string and returns its length. The empty
				string "" has length 0. Embedded zeros are counted, so "a\000bc\000"
				has length 5.]],
	lower 	= [[(string lowered):(string s) - Receives a string and returns a
				copy of this string with all uppercase letters changed to
				lowercase. All other characters are left unchanged. The definition
				of what an uppercase letter is depends on the current locale.]],
	match 	= [[([string ...]):(string s, string pattern, [int init]) -
				Looks for the first match of pattern in the string s.
				If it finds one, then match returns the captures from the pattern;
				otherwise it returns nil. If pattern specifies no captures, then
				the whole match is returned. A third, optional numerical argument
				init specifies where to start the search; its default value is 1
				and may be negative.]],
	rep 	= [[(string repeated):(string s, int n) - Returns a string that is the
				concatenation of n copies of the string s.]],
	reverse	= [[(string reversed):(string s) - Returns a string that is the
				string s reversed.]],
	sub 	= [[(string str):(string s, int i, [int j]) - Returns the substring
				of s that starts at i and continues
				until j; i and j may be negative. If j is absent, then it is
				assumed to be equal to -1 (which is the same as the string length).
				In particular, the call string.sub(s,1,j) returns a prefix of s
				with length j, and string.sub(s, -i) returns a suffix of s with length i.]],
	upper 	= [[(string lower):(string str) - Receives a string and returns a
				copy of this string with all lowercase letters changed to
				uppercase. All other characters are left unchanged.
				The definition of what a lowercase letter is depends on
				the current locale.]],
	exec = [[(boolean success,...):(string cmd, [table env,[table param] ]) -
				executes the given cmd string and returns true if it was
				successful plus the return values from the function.
				Otherwise the compilation error is returned as
				second argument. ]],
--[=[
	bin2char	= [[(number):(string) - converts single char in a number between -127 and 128]],
	bin2short	= [[(number):(string) - converts 2 chars in a number between -32767 and 32768]],
	bin2int		= [[(number):(string) - converts 4 chars in a number between -2147483647 and 2147483648]],
	bin2float	= [[(number):(string) - converts 4 chars in a floating point number with float precisision]],
	bin2double	= [[(number):(string) - converts 8 chars in a floating point number with double precisision]],
	char2bin	= [[(string):(number) - converts number in single char for numbers between -127 and 128 - rounded, no overflowcheck done]],
	short2bin	= [[(string):(number) - converts number in 2 chars for numbers between -32767 and 32768 - rounded, no overflowcheck done]],
	int2bin		= [[(string):(number) - converts number in 4 chars for numbers between -2147483647 and 2147483647 - rounded, no overflowcheck done]],
	float2bin	= [[(string):(number) - converts number in 4 chars with float precision (32 bit)]],
	double2bin	= [[(string):(number) - converts number in 8 chars with double precision (64bit)]],
]=]
	l2dlen		= [[(len):(string) - returns length of string without invisible characters to l2dstring - in case of multiline
					the longes line count is returned]],
	l2dheight	= [[(height):(string) - returns number of \ns in text]],
})


function string.exec (str,env,param)
	local fn,err = loadstring(str)
	if (err) then return nil,err end
	if (env) then setfenv(fn,env) end

	local res = {fn(unpack(param or {}))}
	return true,unpack(res)
end

--[[
string.dumpstripped = luxinia.dumpstripped
string.bin2char,string.char2bin = luxinia.bin2char,luxinia.char2bin
string.bin2short,string.short2bin = luxinia.bin2short,luxinia.short2bin
string.bin2int,string.int2bin = luxinia.bin2int,luxinia.int2bin
string.bin2float,string.float2bin = luxinia.bin2float,luxinia.float2bin
string.bin2double,string.double2bin = luxinia.bin2double,luxinia.double2bin
]]
function string.l2dlen (str)
	local cnt = 0
	local longest = 0
	local i = 1
	while i<=str:len() do
		--print(i)
		if (str:sub(i,i)=="\v") then
			if (str:sub(i,i+4):match("\v%d%d%d")) then
				i = i + 3
			elseif(str:sub(i,i+1):match("\v[bsu]")) then
				i = i + 1
			else
				i = i + 1
			end
		elseif (str:sub(i,i)=="\n") then
			cnt = 0
		elseif(str:byte(i)>=(" "):byte()) then
			cnt = cnt + 1
		end
		i = i + 1
		longest = math.max(longest,cnt)
	end
	return longest
end

function string.l2dheight (str)
	local cnt = 1
	for i,v in str:gmatch("\n") do
		cnt = cnt + 1
	end
	return cnt
end
