dofile "base/scripts/luxmodules/luxinialuacore/oo.lua"
--dofile("base/scripts/luxmodules/luxinialuacore/grouparray.lua")
dofile("base/scripts/luxmodules/luxinialuacore/luxiniaparam.lua")
dofile("base/scripts/luxmodules/luxinialuacore/timer.lua")
dofile("base/scripts/luxmodules/luxinialuacore/autotable.lua")
dofile("base/scripts/luxmodules/luxinialuacore/cyclicqueue.lua")
dofile("base/scripts/luxmodules/luxinialuacore/cameractrl.lua")
dofile("base/scripts/luxmodules/luxinialuacore/utilfunctions.lua")
dofile("base/scripts/luxmodules/luxinialuacore/autodoc.lua")
dofile("base/scripts/luxmodules/luxinialuacore/keyboard.lua")
dofile("base/scripts/luxmodules/luxinialuacore/luaconsole.lua")
dofile("base/scripts/luxmodules/luxinialuacore/compat.lua")
dofile("base/scripts/luxmodules/luxinialuacore/debugoutput.lua")
dofile("base/scripts/luxmodules/luxinialuacore/math3d.lua")
dofile("base/scripts/luxmodules/luxinialuacore/classinfo.lua")
dofile("base/scripts/luxmodules/luxinialuacore/filesystem.lua")
dofile("base/scripts/luxmodules/luxinialuacore/hashspace.lua")
--dofile("base/scripts/luxmodules/luxinialuacore/dreg.lua")
dofile("base/scripts/luxmodules/luxinialuacore/fxutils.lua")

---- additional documentations
if (VFSmapper) then
LuxModule.registerclass("luxinialuacore","VFSmapper",
[[The Virtual File System mapper package provides access on files by
which may either exist in directories of the harddisc or in zip file
archives. Overloads loadfile/dofile in order to load lua source
code from zip archives as well. Further filehandlers can be added
during runtime which allows loading files from different locations
(i.e. Network) as well.
]],VFSmapper,{
	addCallback = [[(function func):(function func) - adds a callback
	function that is called when a file should be loaded. It receives
	two arguments: the filname to be looked for and a boolean value if
	it should be read and returned:

	 function func (filename, read)
	   return read and (read the file content) or (true if it exists)
	 end

	If the function returns nil (nothing), further callback functions
	are called. If it returns false, it is assumed that the file does
	not exist. If it returns true, the file exists and it should be
	possible to return the content as well if needed.

	Luxinia checks if a file exists before it trys to open it. If
	this callback returns true, it MUST be able to return the filecontent
	if requested. Otherwise Luxinia might crash. This callback might
	be extended in near future to determine the filetype as well by
	returning the filetype.]],

	removeCallback = [[():(function func) - removes the given function
	from the callback list.]],

	dir = [[(func iterator):(dirname) - iterates the content of the given
	directoryname. Even if the directory does not exist, it will return
	a function that returns nil. Overloads lfs/FileSystem.dir to use this
	function.]],

	addZip = [[(result,[error]):(file) - adds a file to the list
	of zip archives that are browsed for requested files. If the
	given file could not be opened as zip file, it returns false
	and the reported error string.]],

	removeZip = [[():(file) - remove a zipfile from the list]],

	ziploader = [[(exists/filecontent):(file,read) - callback function
	for that is automaticly added to the list of callbacks. Handles
	the zipfile loading.]]
})
end

if (tls) then
LuxModule.registerclass("luxinialuacore","tls",
	[[TLS stands for 'threaded lua state' and is
	part of the luacore inside luxinia, which is the reason why
	it is not documented in the common way.

	Luxinia is running in a single thread, and only
	the main thread has the functionality to access
	the core library. It is not possible for a thread
	to execute any function of the core directly.

	If a new threading state is created, luxinia creates
	two new states, where one is running as a complete new
	thread, including all the default libraries that are
	included in lua, however without any additional functionality.

	A threaded state can reload DLLs at any point and thus can
	access most other functions that are provided by the external lua libs.

	This is for example the filesystem, the xml parser, and all networking
	functionality and so on.

	One of the created luastates acts as a communicator between
	the main thread and the slave thread, which is running
	in the second luastate. The communicator or exchange state
	is always used exclusivly, which means that if the main thread
	or the slave thread is accessing it, any other access is being
	blocked. The exchange state is loaded without library functions
	and thus cannot execute much code, however, it is possible to
	execute code within that state, which can is injected as a
	string and can come from one of both threads. The
	results of the execution is then returned to the executing thread.

	The current implementation is limited to the exchange of
	numbers and strings at the moment.

	It is important that the mainthread keeps a reference to the
	thread, otherwise the thread is stopped when the garbage collector
	destroys the thread.

	The package paths are automaticly copied from the main thread.

	!!Example:

	This example creates a new state and let it execute
	a loop, sleeping half a second and printing out the clock's time,
	until it is stopped by setting the requested variable value to nil:


	 state = tls.new()
	 state:xset("run",1)
	 state:start(
	 "	while xget 'run' do "..
	 "		sleep(.5) print(os.clock()) "..
	 "	end "..
	 "	print 'finished'")
	 TimerTask.new(function () state:xset("run",nil) end,2000)
	]],{},{})
LuxModule.registerclassfunction("new",
[[
	(tls):() - creates a new threaded lua state object.
]])
LuxModule.registerclassfunction("start",
[[
	([boolean success],[string errormsg]):(tls,string code) -
	Starts the thread, running the given code. Once the execution
	is finished, the thread will terminate normaly. It can
	then be restarted, the states are not modified in that case.

	If an error occures, the function returns nil plus the error message.

	It is not possible to start the tls if it is still running.

	The started thread has a few global functions available, that allows
	it to control its execution:

	* (return_values) xdostring (string code) - executes a piece of code within the exchange state
	* ([nil,errormsg]) xset (key,value) - sets a key/value pair in the exchange state
	* (value / [nil,type]) xget (key) - returns a value for the given key in the exchange state
	* () xlock () - locks the mutex of the exchange thread, pausing any access on the exchange state
	* () xunlock () - unlocks the mutex of the exchange thread
	* () sleep (seconds) - sleeps for the given amount of time
	* () killme () - destroys the thread, effectivly kills the execution instantly



]])
LuxModule.registerclassfunction("xget",
[[
	(boolean value / [nil, typename]):(tls, key) - returns a
	value for the key from the global environment table of the exchangestate
]])
LuxModule.registerclassfunction("xset",
[[
	([nil, error]):(tls, key,value) - sets a
	value for the key in the global environment table of the exchangestate
]])
LuxModule.registerclassfunction("xdostring",
[[
	([true,... / nil, error]):(string code) - executes a piece of
	code in the exchange thread, returning true in case of success and
	the returned values of the code or nil and the error message
]])
LuxModule.registerclassfunction("xlock",
[[
	():(tls) - locks the exchange state until it is unlocked. The
	other thread cannot access the exchange thread then. Use the
	lock when you need to make some atomic changes to the exchange
	state.
]])
LuxModule.registerclassfunction("xunlock",
[[
	():(tls) - unlocks the exchange state
]])
LuxModule.registerclassfunction("running",
[[
	(boolean isrunning):(tls) - returns true if the thread is still running
]])
LuxModule.registerclassfunction("stop",
[[
	():(tls) - destroys the thread, but waits until the mutex was
	unlocked. Don't use this function unless really required, as
	it might damage the luastate which can lead to a crash once
	it is reused.
]])
end


