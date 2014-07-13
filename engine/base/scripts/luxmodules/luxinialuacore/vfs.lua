
require "zip"

VFSmapper = {
	callback = {},
	zips = {},
	zipfp = {},
	origLuaDoFile = dofile,
	origLuaLoadFile = loadfile,
}

local function add (tab,value)
	table.insert(tab,value)
end

local function remove(tab,value)
	for i=#tab,1,-1 do
		if tab[i]==value then
			table.remove(tab,i)
		end
	end
end

function VFSmapper.addCallback(func)
	add(VFSmapper.callback,func)
	return func
end

function VFSmapper.removeCallback(func)
	remove(VFSmapper.callback,func)
end

local lfs = require "lfs"
local lfsd = lfs.dir

function VFSmapper.dir(file)
	local it =  lfsd(file)
	if it() then
		return lfsd(file)
	end

	local ppath = system.projectpath()
	if not ppath then return end
	file = file:gsub("[\\/]+","/"):gsub("^/*","")
	ppath = ppath:gsub("[\\/]+","/"):gsub("^/*","")
	print ("zipload2",file)
	if file:sub(1,#ppath)==ppath then
		file = file:sub(#ppath)
		file = file:gsub("[\\/]+","/"):gsub("^/*","")
		file = file.."/"
		file = file:gsub("/+$","/")
	end

	for i,v in ipairs(VFSmapper.zips) do
		print("checking ",v)
		local fp = VFSmapper.zipfp[v]
		local zfp = fp:files()
		local dirlist = {}
		for entry in zfp do
			if entry.filename:sub(1,#file):lower()==file:lower() then
				local f =entry.filename:sub(#file+1)--:gsub("^/*","")
				--print("FILE:",file,entry.filename,f)

				if not f:match("/")then
					dirlist[#dirlist+1] = f
				end
			end
		end
		if #dirlist>0 then
			local i = 0
			return function ()
				i = i + 1
				return dirlist[i]
			end
		end
	end
	return function() end
end

lfs.dir = VFSmapper.dir


function VFSmapper.addZip(file)
	if VFSmapper.zipfp[file] then
		return true
	end
	local fp,err = zip.open(file)
	if not fp then return false,err end

	add(VFSmapper.zips,file)
	VFSmapper.zipfp[file] = fp

	return true
end

function VFSmapper.removeZip(file)
	remove(VFSmapper.zips,file)
	if VFSmapper.zipfp[file] then
		VFSmapper.zipfp[file]:close()
		VFSmapper.zipfp[file] = nil
	end
end

function VFSmapper.ziploader(file,read)
	--print ("load from zip ?",file)
	local ppath = system.projectpath()
	if not ppath then return end
	file = file:gsub("[\\/]+","/"):gsub("^/*","")
	ppath = ppath:gsub("[\\/]+","/"):gsub("^/*","")
	if file:sub(1,#ppath)==ppath then
		file = file:sub(#ppath)
		file = file:gsub("[\\/]+","/"):gsub("^/*","")
	end
	for i,v in ipairs(VFSmapper.zips) do
	--	print("checking ",v)
		local fp = VFSmapper.zipfp[v]
		local zfp = fp:open(file)
		if zfp then
			if read then return zfp:read"*a" else return true end
		end
	end
end

VFSmapper.addCallback(VFSmapper.ziploader)

local function opener (name)
	for i,v in ipairs(VFSmapper.callback) do
		local content = v(name,true)
		if content then return content end
	end
	local fp,err = io.open(name,"rb")
	if fp then
		return fp:read("*a"),fp:close()
	end
end

function VFSmapper.fileexists (name)
	for i,v in ipairs(VFSmapper.callback) do
		local content = v(name)
		if content~=nil then return content end
	end

	--print ("check:",name)
	local fp = io.open(name,"rb")
	if fp then
		fp:close()
		return true
	else
		return false
	end
end

fileloader.setfileexists(VFSmapper.fileexists)
fileloader.setfileopen(opener)


local lf,df = loadfile,dofile
function loadfile(file)
	if VFSmapper.fileexists(file) then
		return loadstring(opener(file),"@"..file)
	else return lf(file) end
end

function dofile(file,...)
	return assert(loadfile(file))(...)
end

for i,v in ipairs(luxarg) do
	if v:lower():match("%.lxz$") or v:lower():match("%.zip$") then
		VFSmapper.addZip(v)
	end
end
