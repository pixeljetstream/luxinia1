LuaConsole =
{
	lines = CyclicQueue.new(1000,true),
	input = AutoTable.new(),
	lastinput = AutoTable.new(),
	inputModified = true,
	inputHistory = CyclicQueue.new(1000,true),
	browseHistory = 1, scrollLine = 1,
	cursor = 1,
	activeplugins = {},
	applications = {},
	runningApplication = nil,
}

do
	LuxModule.registerclass("luxinialuacore","LuaConsole",
		"The console is a programming and debugging tool within the runtime environment of Luxinia. "..
		"The LuaConsole provides interactive access on "..
		"Luxinia.",
		LuaConsole,{
			lines = "[CyclicQueue] - A cyclic array object obtained from the CyclicQueue class. Stores 1000 lines per default.",
			input = "[AutoTable] - A 'normal' table that is obtained from the AutoTable class. Represents userinput as array",
			lastinput = "[AutoTable] - The last line of input the user made before started scrolling through the history.",
			inputModified = "[boolean] - True if the input was changed by user, so the input must be stored in 'lastinput'",
			inputHistory = "[CyclicQueue] - cyclic array with defaultsize = 1000 of all user (successful) inputs he made.",
			browseHistory = "[int] - position where the user is in history",
			scrollLine = "[int] - position in output display.",
			cursor = "[int] - position of inputcursor, must be >0 and <= size of input + 1.",
			activeplugins = "[table] - table of all plugin functions.",
			applications = "[table] - table of all consoleapplications.",
			runningApplication = "[table] - currently running consoleapplication.",
		})

	_print = print
	-- splits a string in lines and returns an iterator for each line
	local function stringLines (str)
		local start = 1
		local endl = string.byte("\n")
		local len = string.len(str)
		return function ()
			if (not endl) then return end
			local run = start
			while (run<len) do
				if (string.byte(str,run)==endl) then
					local s = start
					start = run + 1
					return string.sub(str,s,run-1)
				end
				run = run + 1
			end
			endl = nil
			if (run<=len) then
				return string.sub(str,start)
			end
		end
	end

	LuxModule.registerclassfunction("setConsolePlugin",
		[[(any key):(function fn) - Add a pluginfunction to the console. All.
		Plugins are being executed at the end of a drawcycle which means that
		additional characters can be set on the console, i.e. interactive help.]])
	function LuaConsole.setConsolePlugin (key, fun)
		LuaConsole.activeplugins[key] = fun
	end

	LuaConsole.setConsolePlugin("help",
		function (selfkey, posinfo, inputline, cursor)
			local chr = inputline[cursor-1]
			local line = table.concat(inputline)
			if (not chr) then return end
			local len = table.getn(inputline)
			local nd = cursor - 1
			while (nd>1 and inputline[nd]<=' ') do
				nd = nd - 1
			end

			repeat
				if (type(inputline[nd+1])~='string' or inputline[nd+1]<=')' or inputline[nd+1]==',') then break end
				nd = nd + 1
			until (nd==len)


			local start = nd
			while (start>1 and inputline[start - 1]>')' and inputline[start - 1]~=',') do
				start = start -1
			end


			local word = {}
			local tx = string.sub(line,start,nd)
			local class,fn = string.match(tx,"^([%w%d]+)[.:%s]*([%w%d]*)")

			if (not class and not fn) then return end

			local w = 35
			function printinfo(str)
					str = str or "<no info available>"
					str = str:gsub("\n\n\t*","\v\v")
					str = str:gsub("\n\t*%*","\v*")
					str = str:gsub("\n\t* ","\vs")
					str = str:gsub("\n\t*","")
					str = str:gsub("\v\v","\n")
					str = str:gsub("\v%*","\n *")
					str = str:gsub("\vs","\n ")
					local len = string.len(str)
					local i = 0
					local ln = 0
					repeat
						console.pos(console.width()-w-1,ln)
						ln = ln + 1
						while (str:sub(i,i)==" ") do i = i + 1 end
						local nd = i+w
						while (str:sub(nd,nd)~=" " and str:sub(nd,nd)~="\n" and nd>i and nd<len) do nd = nd - 1 end
						nd = nd>i and nd>0 and nd or i+w
						local line = string.sub(str,i,nd)
						local p = string.find(line,"\n")
						if (p) then
							line = string.sub(str,i,i+p-1)
							i = i+ p
						else
							i = nd +1
						end
						console.put(line,0,255,210,0)
					until i>len or ln+2 > console.height()
					if (console.height()<ln+2) then
						console.pos(console.width()-w-1,ln)
						console.put("...",1,255,210,0)
					end
			end
			local description = ClassInfo.getClassDescription(class)
			if (not description) then
				local obj = _G[class]
				class = ClassInfo.getClassName(obj)
				if (class) then
					description = ClassInfo.getClassDescription(class)
				else return end
			end
			local fnlist = ClassInfo.getFunctionList(class)



			if (not fnlist or not fnlist[fn]) then
				local out = {class,": ",description,"\n\n",string.rep("-",w+1)}

				for i,v in UtilFunctions.pairsByKeys(fnlist or {}) do
					if (string.sub(i,1,string.len(fn))==fn) then
						local str = string.format("%s: %s",tostring(i),tostring(v))
						if (string.len(str)+2>w) then
							str = string.sub(str,1,w-5).."..."
							table.insert(out,str.."\n\n")
						else
							table.insert(out,str.."\n\n")
						end
					end
				end
				printinfo(table.concat(out))
			else
				printinfo(class..": "..fn.."\n\n"..string.rep("-",w+1)..fnlist[fn])
			end
		end )

	function LuaConsole.setApplication (key, conapp)
		LuaConsole.applications[key] = conapp
	end

	function LuaConsole.print (...)
		local arg = {...}
		for i = 1,select("#",...) do
			arg[i] = tostring(arg[i])
			--local rest = math.mod(string.len(arg[i]),4)
			arg[i] = arg[i] .. "\t"
		end
		local out = table.concat(arg)

		for l in stringLines(out) do
			--_print(l)
			--_print("step",console.width())
			for i=1,string.len(l)+1,console.width() do
				LuaConsole.lines(string.sub(l,i,console.width()+i-1))
			end
		end
		io.flush()
	end

	function LuaConsole.prompt (line)
		if (not line) then return "+>" end
		return "->"
	end

	function LuaConsole.calcInputLines ()
		local inputlines = 1
		local pw = string.len(LuaConsole.prompt())
		local w = console.width() - pw

		local chars = 0
		for i,v in ipairs(LuaConsole.input) do
			chars = chars + ((v=="\t" and 4-math.mod(chars+1+pw,4)) or 1)
			if (chars>=w or v == "\n") then
				inputlines = inputlines + 1
				chars = 0
			end
		end
		return inputlines
	end

	function LuaConsole.draw ()
		local input = table.concat(LuaConsole.input)
		local inputlines = LuaConsole.calcInputLines()
		local posinfo = {lines={}, input = {}}

		console.clear()
		console.pos(0,0)
		for i=console.height()-inputlines-1+LuaConsole.scrollLine,LuaConsole.scrollLine,-1 do
			local line = LuaConsole.lines[i]
			if (line) then
				console.put(line)
				if (string.len(line)<console.width()) then
					local x,y = console.pos()
					console.pos(0,y+1)
				end
			end
		end

		local p = LuaConsole.prompt()
		local cursoratend = table.getn(LuaConsole.input)<=LuaConsole.cursor
		local line = 0
		if (cursoratend) then LuaConsole.input(" ") end
		console.put(p)
		for i,v in ipairs(LuaConsole.input) do
			local x,y = console.pos()
			if (v == "\n") then
				console.pos(0,y+1)
				line = line + 1
				console.put(LuaConsole.prompt(line))
			else
				if (v=="\n") then
					x = x + 4 - math.mod(x+1,4)
					console.pos(x,y)
				end
				console.put(v,i == LuaConsole.cursor and 4 or 0,255,255,0)
			end
		end
		if (cursoratend) then table.remove(LuaConsole.input) end

		for a,b in pairs(LuaConsole.activeplugins) do
			b(a, posinfo, LuaConsole.input, LuaConsole.cursor)
		end
	end

	function LuaConsole.cmdExec ()
		local input = table.concat(LuaConsole.input)
		local chunk, err = loadstring (input)

		LuaConsole.browseHistory=1
		LuaConsole.lastInput = nil

		if (not chunk) then
			if (string.find(err,"end' expected ") or string.find(err,"unexpected symbol near `{'") or string.find(err,"<eof>")) then
				LuaConsole.input("\n")
				LuaConsole.cursor = LuaConsole.cursor+1
				return
			else
				return LuaConsole.print(err)
			end
		else
			local chunk2,err = loadstring("return "..input)
			if (not err) then chunk = chunk2 end
			LuaConsole.print(input)
			xpcall(function ()
				local ret = {chunk()}
				LuaConsole.inputHistory(input)
				LuaConsole.input = AutoTable.new ()
				LuaConsole.inputModified = true
				LuaConsole.print(unpack(ret))
				LuaConsole.cursor = 1
			end, function (err)
				LuaConsole.print(debug.traceback(err))
			end)
		end
	end

	function LuaConsole.browseInputHistory (dir)
		if (LuaConsole.browseHistory <= 2 and dir<0) then
			if (not LuaConsole.inputModified) then
				LuaConsole.input = LuaConsole.lastInput or LuaConsole.input
			end
			LuaConsole.inputModified = false
			LuaConsole.browseHistory = 1
			LuaConsole.cursor = math.max(1,table.getn(LuaConsole.input)+1)
			return
		end

		if (LuaConsole.inputModified) then
			--_print("back",table.concat(LuaConsole.input))
			LuaConsole.inputModified = false
			LuaConsole.lastInput = LuaConsole.input
		end
		LuaConsole.input = AutoTable.new()

		LuaConsole.browseHistory = math.max(1,math.min(LuaConsole.browseHistory + dir,LuaConsole.inputHistory.count+1))


		local inp = LuaConsole.inputHistory[LuaConsole.browseHistory-1]
		--_print(LuaConsole.browseHistory, LuaConsole.inputHistory[1],inp)

		if (inp) then
			for i=1,string.len(inp) do
				local chr = string.sub(inp,i,i)
				LuaConsole.input(chr)
			end
			--for a,b in LuaConsole.input do _print(a,"'"..b.."'") end
		end

		LuaConsole.cursor = math.max(1,table.getn(LuaConsole.input)+1)
	end

	local wasinactive = true
	local mousemode = false
	function LuaConsole.think ()
		if (not console.active()) then
			if (not wasinactive) then
				input.showmouse(mousemode)
			end
			wasinactive = true
			return
		end
		if (wasinactive) then
			wasinactive = false
			mousemode = input.showmouse()
			input.showmouse(true)
		end

		while (true) do
			local popped = console.popstd()
			if (not popped) then break end
			LuaConsole.print(popped)
		end
		while (true) do
			local popped = console.poperr()
			if (not popped) then break end
			LuaConsole.print(popped)
		end

		while (true) do
			local key,code = console.popKey()
			if (not key) then break end

			if (key == "\b") then
				LuaConsole.cursor = math.max(1,LuaConsole.cursor-1)
				table.remove(LuaConsole.input,LuaConsole.cursor)
			elseif (code==13) then LuaConsole.cmdExec ()
			elseif (code<255) then
				table.insert(LuaConsole.input,LuaConsole.cursor,key)
				LuaConsole.cursor = LuaConsole.cursor+1
				LuaConsole.inputModified = true
			elseif (code == 297) then table.remove(LuaConsole.input,LuaConsole.cursor) LuaConsole.cursor = math.min(table.getn(LuaConsole.input)+1,LuaConsole.cursor)
			elseif (code == 283) then LuaConsole.browseInputHistory(1) -- up
			elseif (code == 284) then LuaConsole.browseInputHistory(-1) -- down
			elseif (code == 286) then LuaConsole.cursor = math.min(table.getn(LuaConsole.input)+1,LuaConsole.cursor + 1)
			elseif (code == 285) then LuaConsole.cursor = math.max(1,LuaConsole.cursor - 1)
			elseif (code == 301) then -- home
				LuaConsole.cursor = table.getn(LuaConsole.input)+1
			elseif (code == 300) then
				for i = LuaConsole.cursor,1,-1 do
					if (LuaConsole.input[i]=="\n") then break end
					LuaConsole.cursor = i
				end
			else
			--	_print(code)
			end
		end

		LuaConsole.draw()
	end

	Timer.set("_core_luaconsole",LuaConsole.think)

	print = function (...) LuaConsole.print(...) _print(...) end

end

