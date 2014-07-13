
LuxModule.registerclass("luxinialuacore","DataRegistry",
	[[The DataRegistry class provides a simple method to save projectdependent
	data like highscores, savegames etc. in a directory of luxinia itself (for now).
	The location of this data container might change in future to be stored
	in a user specific directory of the OS.

	The class provides simplified functions to load and save tables
	as well as providing direct access for file operations.

	Separating the user data and the projectdata helps to keep the project
	directory clean. Keep in mind that it becomes impossible to save
	data in the project directory if the project is compressed
	into a single archive. Thus, this system should be favored instead
	of direct file access.

	The DataRegistry awaits a so called projectkey, which should be
	a string that identifies the project. A directory in the data
	directory with this name is then created. The additional
	filename can contain directorie relations as well. If the
	location does not exist, it will automaticly be created.]],
	{},
	{
		delete = [[():(projectkey,file) - deletes the given file in the project
		directory. Directories are only deleted, if they are empty.]],
		deleteall = [[():(projectkey,[dirpath]) - deletes the whole project or
		the given subdirectories.]],
		dir = [[(iterator):(projectkey,[dirpath]) - returns a directory
		content iterator for the  given directory.]],
		makepath = [[():(path) - makes sure that the given path exists]],
		getpath= [[(path):(string projectkey,[string file]) -
		returns a valid path that points to the given project path or
		to the file in that projectpath]],
		loadtable = [[([tab]):(projectkey,file,[table defaulttable]) -
		returns defaulttable if no matching file could  be loaded, otherwise
		the file's content is loaded]],
		savetable = [[():(projectkey,file,table data) -
		writes the given data table to the file located in the projectpath]],
		open = [[(filehandle,[errormsg]):(projectkey,file,[mode]) -
		returns the filehandle to the given file or nil and the errormessage
		if this could not be done]],
	})

DataRegistry = {}
DataRegistry.savepath = "datareg/"
function DataRegistry.makepath (path)
	path = path:gsub("[^/\\]*$","")
	local d = ""
	for dd in path:gmatch("[^/\\]+") do
		d = d..dd.."/"
		if not lfs.attributes(d) then
			lfs.mkdir(d)
		end
	end
end

function DataRegistry.getpath(projectkey,file)
	return (DataRegistry.savepath.."/"..projectkey..(file and "/"..file or "/")):gsub("/+","/")
end

function DataRegistry.save(projectkey,file,data)
	DataRegistry.makepath(DataRegistry.getpath(projectkey,file))
	local fp = assert(io.open(DataRegistry.getpath(projectkey,file),"w"))
	fp:write(data)
	fp:close()
end

function DataRegistry.load(projectkey,file,defdata)
	if not lfs.attributes(DataRegistry.savepath..projectkey) then
		return defdata
	end
	return io.open(DataRegistry.getpath(projectkey,file)):read("*a")
end


function DataRegistry.loadtable(projectkey,file,deftab)
	if not lfs.attributes(DataRegistry.savepath..projectkey) then
		return deftab
	end
	return dofile(DataRegistry.getpath(projectkey,file))
end

function DataRegistry.savetable(projectkey,file,tab)
	local ser = UtilFunctions.serialize (tab)
	DataRegistry.makepath(DataRegistry.getpath(projectkey,file))
	local fp = assert(io.open(DataRegistry.getpath(projectkey,file),"w"))
	fp:write(ser)
	fp:close()
end


function DataRegistry.open(projectkey,file,mode)
	DataRegistry.makepath(DataRegistry.getpath(projectkey))
	return io.open(DataRegistry.getpath(projectkey,file),mode)
end

function DataRegistry.delete(projectkey,file)
	return os.remove(DataRegistry.getpath(projectkey,file))
end

function DataRegistry.deleteAll(projectkey,file)
	local function clear(dir)
		--print("clearing",dir)
		for name in lfs.dir(dir) do
			if name ~="." and name~=".." then
				if lfs.attributes(dir.."/"..name,"mode")=="directory" then
					clear(dir.."/"..name)
					--print("remove",dir.."/"..name)
				end
				print("remove",dir.."/"..name)
				print(os.remove(dir.."/"..name))
			end
		end
	end
	if #projectkey==0 or projectkey=="." or projectkey==".." then return end
	clear(DataRegistry.getpath(projectkey,file))
	os.remove(DataRegistry.getpath(projectkey))
end

function DataRegistry.dir(projectkey,file)
	return lfs.dir(DataRegistry.getpath(projectkey,file))
end
