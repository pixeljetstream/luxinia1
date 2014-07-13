AutoTable = {}
LuxModule.registerclass("luxinialuacore","AutoTable",
		"The AutoTable class provides a constructor for a table that offers "..
		"a simple way to insert new values. Although the AutoTable object is a "..
		"lua table that can be used like any normal table, you can use the table "..
		"like a function. Any provided argument is inserted at the end of the "..
		"table. In any case, the number of elements of the table is returned.",AutoTable,{})
function AutoTable.new (copy)
	local self = {}
	for a,b in pairs(copy or {}) do self[a] = b end
	setmetatable(self,AutoTable.metatable)
	return self
end

function AutoTable:onCall (...)
	for i=1,select('#',...) do
		local c = select(i,...)
		if (c) then
			table.insert(self,c)
		end
	end
	return table.getn(self)
end

AutoTable.metatable = {__call = AutoTable.onCall}
