FileSystem = {}
LuxModule.registerclass("luxinialuacore","FileSystem",
			[[Luxinia used an internal way to gain access to the directory 
			structures that was located in the luxinia.dir function that 
			was undocumented. The function returned a table of filenames. 
			For plattformindependance and other reasons, we decided to use 
			the Lua File System which is part of the Kepler project 
			(http://www.keplerproject.org/luafilesystem/). You can use the 
			lfs lib or this class. This class is a pure wrapper for the 
			lfs lib and provides only additional documentation in the way 
			the other classes are described. The function descriptions are 
			extracts from the lfs documentation that can be found at the 
			given link.]],
			FileSystem,{})
			
require "lfs"

LuxModule.registerclassfunction("attributes",
	[[
	([mixed attributevalues]):(string filepath, [string attributename]) - 
	Returns a table with the file attributes corresponding to filepath (or nil 
	followed by an error message in case of error). If the second optional 
	argument is given, then only the value of the named attribute is returned 
	(this use is equivalent to lfs.attributes(filepath).aname, but the table is 
	not created and only one attribute is retrieved from the O.S.). The 
	attributes are described as follows; attribute mode is a string, all the 
	others are numbers, and the time related attributes use the same time 
	reference of os.time:

	* dev: on Unix systems, this represents the device that the inode resides on. 
		On Windows systems, represents the drive number of the disk containing the file
	* ino: on Unix systems, this represents the inode number. On Windows systems 
		this has no meaning mode string representing the associated protection 
		mode (the values could be file, directory, link, socket, named pipe, 
		char device, block device or other)
	* nlink: number of hard links to the file
	* uid: user-id of owner (Unix only, always 0 on Windows)
	* gid: group-id of owner (Unix only, always 0 on Windows)
	* rdev: on Unix systems, represents the device type, for special file inodes. 
		On Windows systems represents the same as dev
	* access: time of last access
	* modification: time of last data modification
	* change: time of last file status change
	* size: file size, in bytes
	* blocks: block allocated for file; (Unix only)
	* blksize: optimal file system I/O blocksize; (Unix only) 
	]])
FileSystem.attributes = lfs.attributes

LuxModule.registerclassfunction("chdir",
	[[([boolean success], [string error]):(path) -
	Changes the current working directory to the given path.
	Returns true in case of success or nil plus an error string.]])
FileSystem.chdir = lfs.chdir

LuxModule.registerclassfunction("currentdir",
	[[([string dir], [string error]):() - Returns a string with the current 
	working directory or nil  plus an error string.]])
FileSystem.currentdir = lfs.currentdir

LuxModule.registerclassfunction("dir",
	[[([function iterator], [string error]):(string path) - 
	Lua iterator over the entries of a given directory. Each time the 
	iterator is called it returns a string with an entry of the directory; 
	nil is returned when there is no more entries. Raises an error if path 
	is not a directory.]])
FileSystem.dir = lfs.dir

LuxModule.registerclassfunction("lock",
	[[([boolean success], [string error]):(filehandle, mode[, start[, length] ]) -
	Locks a file or a part of it. This function works on open files; the file 
	handle should be specified as the first argument. The string mode could be 
	either r (for a read/shared lock) or w (for a write/exclusive lock). The 
	optional arguments start and length can be used to specify a starting point 
	and its length; both should be numbers.

	Returns true if the operation was successful; in case of error, it returns 
	nil plus an error string.]])
FileSystem.lock = lfs.lock

LuxModule.registerclassfunction("mkdir",
	[[([boolean success],[string error]):(string dirname) -
	Creates a new directory. The argument is the name of the new directory.
	Returns true if the operation was successful; in case of error, it returns 
	nil plus an error string.]])
FileSystem.mkdir = lfs.mkdir

LuxModule.registerclassfunction("rmdir",
	[[([boolean success],[string error]):(string dirname) -
	Removes an existing directory. The argument is the name of the directory.
	Returns true if the operation was successful; in case of error, it returns 
	nil plus an error string.]])
FileSystem.rmdir = lfs.rmdir

LuxModule.registerclassfunction("touch",
	[[([boolean success],[string error]):(string filepath [, atime [, mtime] ]) -
	Set access and modification times of a file. This function is a bind to 
	utime function. The first argument is the filename, the second argument 
	(atime) is the access time, and the third argument (mtime) is the 
	modification time. Both times are provided in seconds (which should be 
	generated with Lua standard function os.date). If the modification time 
	is omitted, the access time provided is used; if both times are omitted, 
	the current time is used.

	Returns true if the operation was successful; in case of error, it returns 
	nil plus an error string.]])
FileSystem.touch = lfs.touch

LuxModule.registerclassfunction("unlock",
	[[([boolean success],[string error]):(filehandle[, start[, length] ]) -
	Unlocks a file or a part of it. This function works on open files; the 
	file handle should be specified as the first argument. The optional 
	arguments start and length can be used to specify a starting point and its 
	length; both should be numbers.

	Returns true if the operation was successful; in case of error, it returns 
	nil plus an error string. ]])
FileSystem.unlock = lfs.unlock

LuxModule.registerclassfunction("isDir",
	[[(boolean isdir):(file) - return true if the given file is an directory
	]])
function FileSystem.isDir(dir)
	return FileSystem.attributes(dir,"mode")=="directory"
end