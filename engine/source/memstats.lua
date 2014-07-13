data = dofile("memgeneric_stats.lua")

stats = {
	cntfunc = {},
	cntfile = {},
	cntbins = {},
	
	bytesfunc = {},
	bytesfile = {},
	bytesbins = {},
}

local function cnt(tab,key)
	local old = tab[key]
	tab[key] = old and {old[1]+1} or {1}
end

local function bytes(tab,key,v)
	local old = tab[key]
	if (old) then
		old[1] = old[1] + v
		old[2] = old[2] + 1
		old[3] = math.min(old[3],v)
		old[4] = math.max(old[4],v)
	else
		old = {v,1,v,v}
	end
	tab[key] = old
end


do
	local corr = 0
	local tab = {}
	for i,v in ipairs(data) do
		tab[v.file] = true
	end
	
	-- convert to indexable
	local nt = {}
	for i,v in pairs(tab) do
		table.insert(nt,i)
	end
	
	-- find common substring
	local test = nt[1]
	local found = true
	local cnt = #nt
	
	while (found) do
		corr = corr + 1
		pat = string.sub(test,1,corr)
		
		for i=1,cnt do
			if (not string.find(nt[i],pat,1,true) ) then
				found = false
				break
			end
		end
	end
		
	function fixfile(f)
		return string.sub(f,corr,-1)
	end
end

for i,v in ipairs(data) do
	v.file = fixfile(v.file)
	
	cnt(stats.cntfunc,v.file..":"..tostring(v.line))
	cnt(stats.cntfile,v.file)
	cnt(stats.cntbins,v.size)
	
	bytes(stats.bytesfunc,v.file..":"..tostring(v.line),v.size)
	bytes(stats.bytesfile,v.file,v.size)
	bytes(stats.bytesbins,v.size,v.size)
end

for i,v in pairs(stats) do
	if (string.find(i,"bytes",1,true)) then
		if (i ~= "bytesbins") then
			for n,d in pairs(v) do
				d[5] = math.ceil(d[1]/d[2])
			end
		else
			for n,d in pairs(v) do
				d[4] = nil
				d[3] = nil
				d[2] = nil
			end
		end
	end
end

out = io.open("memgeneric_report.txt","wb")

local function convtable(tab, cmp)
	-- convert to sortable
	local nt = {}
	for i,v in pairs(tab) do
		table.insert(nt,{id=i,v=v})
	end
	table.sort(nt,cmp)
	
	return nt
end

function dumptable(name,tab)
	local nt = convtable(tab,function(a,b) return a.v[1] > b.v[1] end)
	out:write(name.."\n")
	
	-- dump descending
	for i,v in ipairs(nt) do
		out:write(tostring(v.id)..";")
		for i,n in ipairs(v.v) do
			out:write("\t"..tostring(n)..";")
		end
		out:write("\n")
	end
	out:write("\n\n")
end


statsnames = convtable(stats,function(a,b) return a.id < b.id end)
for i,v in ipairs(statsnames) do
	dumptable(v.id,v.v)
end
