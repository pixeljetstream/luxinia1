local timers, disabled, coroutines = {},{},{}
local frame = 0
local defaultSize = 32
setmetatable(coroutines,{__mode = "kv"})

--cocnt = true
if cocnt then
	local cc = coroutine.create
	local cn = 0
	local cm = 0
	function coroutine.create(fn,...)
--		local trace = debug.traceback()
		return cc(function()
			cn = cn + 1
			if cm<cn then
				cm = cn
				print(cm)
--				print(trace)
			end
			--if cn>10 then print(cn) end
			xpcall(fn,debug.traceback) cn = cn - 1 end)
	end
end

Timer = {}
do
	local timeractiontodo = {}
	local timerfinally = {}
	LuxModule.registerclass("luxinialuacore","Timer",
		[[Luxinia does automaticly call a function named 'think' in a table called 'luxinia'
		each frame. The Timer class registers this function and provides a mechanism
		to add additional function that should be called each frame.

		The function is being disabled if it throws an error.

		The timer has been changed during 0.93 to use coroutines. This is quite useful:
		you can yield a function at any point of time during execution if it is
		called by a timer or timertask. The execution is then continued later.
		For timers, this is the next period, for TimerTasks, please read the documentation
		on the TimerTask class. You can create endless loops just as this:

		 Timer.set("endless loop",
		 	function ()
		 		while true do
		 			print "tick"
		 			coroutine.yield()
		 		end
		 	end, 50)

		The code sample above will create a timertask that is executed about every
		50 ms and will always print "tick" out.

		Coroutines are useful utilities to create AIs, agents or gameloops.
		You do not have to change a state of a game function for example:

		 Timer.set("game",
		 	function ()
		 		local gamestarts,gamefinished
		 		repeat
		 			-- do something about intro menu etc. stuff
		 			coroutine.yield()
		 		until gamestarts

		 		repeat
		 			-- make some gamelogic stuff, spawing actors, etc
		 			-- set gamefinished to true or break loop if game is finished
		 			coroutine.yield()
		 		until gamefinished
		 	end, 1000/30)

		The code sample above will be executed every 30 fps. You can also nest the
		loops in loops, for example for each level. Instead of writing different
		functions that represent your state of game and that are selected for
		execution, you can use this method to create a linear way of writing a
		game.
		]],Timer,
		{
			set = [[(function fn):(string description,function fn[, int interval], [int maxcnt]) -
				set a function to be called each frame if interval is not passed.
				If interval is a number (minimum 1), the function will be called
				in the intervals in milliseconds. The timer automaticly repeates
				calls if the timer is late. If the maxcnt argument is passed,
				the timer won't be called more often than maxcnt per frame -
				if a timer is behind time, this prevents the system too freeze
				if the timerfunction is taking longer than its given interval.

				fn will be called with the local time of the function as first
				argument and the number of the repeated calls within
				this frame as second argument (@@fn(int localtime, int repeatedcalls)@@).
				The localtime is increased by the value of interval each time
				a call is done.

				The description string can be used to remove a timer. The timer
				is identified by the function value and multiple timers can share
				the same description string (and can all be removed with one remove
				call this way).

				Returns the passed function.]],
			remove = "():(string/function key) - removes a timer function from the Timer list.",
			think = [[
				():() - Function that is called each frame. If you switch to
				manual frame processing within lua, you should call this
				function each frame you do, since other modules are dependent
				on the timer function (i.e. the Keyboard class).
			]],
			frame = [[(int):() - returns the frame (or the number of thinkcycles)]],
			enable = [[
				():(string/function key) - enables a disabled timerfunction again
			]],
			finally = [[
				():(key,function) - the finally function is a list of
				functions that should be executed when all timers are finished.
				The timer and the finish functions are called in no particular
				order so this is just a method to do some stuff at the end
				of the timer calls (the timertasks are finished till then too).
				A key can
				only be registered one time, after that it will overwrite the
				previous function.

				The finally function registrations are only valid for the
				current frame, so you must reregister it if required.

				This functionality exists to provide help on optimized updating
				- you may want to collect information on for the real update
				function of an object that can then update all information
				alltogether instead of doing the same again and again updating
				ony tiny changes.

				Registering finally functions in a finally function call works.
				The new registered function is called once all previously
				registered functions have been executed.
				If you keep on registering finally functions, this means that
				the finally calls will never stop!
			]]
		})

	local time

	do
		local last=0
		local avgclockdif,avgtimedif = 0,0
		local lastclock,lasttime=0,0
		local now , frame = 0,0
		function time ()
			local t,c = system.time(),os.clock()*1000
			if t == last then return now end

			frame = frame + 1
			local da,db = t-lasttime,c-lastclock

			--if (frame%100==0) then print(avgclockdif,avgtimedif) end

			lasttime,lastclock = t,c
			avgclockdif,avgtimedif = (avgclockdif*0.8 + db*0.2), (avgtimedif*0.8 + da*0.2)
			now = lasttime + avgclockdif*0.8 + avgtimedif*0.2

			return now
		end
	end

	local timerfinallycnt = 0
	function Timer.finally(key,fncall)
		assert (key and fncall,"key and function required")
		timerfinally[key] = fncall
		timerfinallycnt = timerfinallycnt + 1
	end

	-- protective coroutine environment for function
	-- the coroutines table stores all functions and their coroutine
	-- counterparts
	local function cocall (co,description,...)
		local ret = {coroutine.resume(co,...)}
		local noerr,msg = unpack(ret)
		if not noerr then
			print("Error calling "..(tostring(description) or "unnamed").." function:")
			print(msg)
			print("Coroutine status: ",coroutine.status(co))
			print(("-"):rep(70))
			print(debug.traceback(co))
			co = nil
		else
			if coroutine.status (co) == "dead" then
				co = nil
			end
		end
		return noerr,co,ret
	end

	local function tcall(id,description,fn,...)
		local co = coroutines[id] or coroutine.create(fn,defaultSize)
		local noerr,co,ret = cocall(co,description,...)
		coroutines[id] = co
		return noerr,co,unpack(ret)
	end

	function Timer.think ()
		local t = time()
		frame = frame + 1

		local sorted = {}
		for v,i in pairs(timers) do
			table.insert(sorted, {i,v})
		end
		table.sort(sorted,function(a,b) return a[1].id<b[1].id end)
		
		for i,v in ipairs(sorted) do
			local i,v = v[1],v[2]
			if i.co and coroutine.status(i.co) == "dead" then i.co = nil end
			i.co = i.co or coroutine.create(v,defaultSize)
			if (i.interval) then
				local cnt = 0
				--print(t-i.time,i.interval)
				if (i.settime) then
					i.settime = nil
					i.time = t-i.interval-1
				end
				local maxcnt = frame ==2 and 1 or i.maxcnt
				while (t-i.time>i.interval and cnt < maxcnt) do
					if i.co and coroutine.status(i.co) == "dead" then i.co = nil end
					i.co = i.co or coroutine.create(v,defaultSize)
					if (not i.removed) and (not cocall(i.co,i[1],i.time,cnt)) then
						disabled[v] = i
						timers[v] = nil
						break
					end
					cnt = cnt + 1
					i.time = i.time + i.interval
				end
				if cnt>=maxcnt then i.settime = true end
			else
				if not cocall(i.co,i[1],v) then
					disabled[v] = i
					timers[v] = nil
				end
			end

		end
		for i,v in ipairs(timeractiontodo) do
			v.fn(unpack(v.arg))
		end
		timeractiontodo = {}

		while timerfinallycnt>0 do
			local fin = timerfinally
			timerfinally = {}
			timerfinallycnt = 0
			for i,v in pairs(fin) do
				v()
			end
		end
	end

	function Timer.printTimers()
		for i,v in pairs(timers) do
			print(i,v)
		end
	end

	function Timer.frame ()
		return frame
	end


	local id = 0
	local function set (description,fn, interval,maxcnt)
		id = id + 1
		timers[fn] = {description,
			interval = interval and math.max(1,interval),
			time= system.time()
				, maxcnt = maxcnt or 100000, id = id}
		disabled[fn] = nil

	end

	local function remove (key)
		if (type(key)=="function") then
			timers[key] = nil
			disabled[key] = nil
		else
			for i,v in pairs(timers) do
				if (v[1] == key) then
					timers[i] = nil
				end
			end
			for i,v in pairs(disabled) do
				if (v[1] == key) then
					timers[i] = nil
				end
			end
		end
	end

	local function enable (key)
		if (disabled(key)=="function") then
			timers[key] = disabled[key]
		else
			for i,v in pairs(disabled) do
				if (v == key) then
					timers[i] = v
					disabled[i] = nil
				end
			end
		end
	end

	function Timer.set (description, fn,interval,maxcnt)
		table.insert(timeractiontodo,{fn = set, arg = {description,fn,interval,maxcnt}})
		return fn
	end

	function Timer.enable (key)
		table.insert(timeractiontodo,{fn = enable, arg = {key}})
	end

	function Timer.remove (key)
		if (type(key)=="function") then
			timers[key].removed = true
		else
			for i,v in pairs(timers) do
				if (v[1] == key) then
					timers[i].removed = true
				end
			end
		end

		table.insert(timeractiontodo,{fn = remove, arg = {key}})
	end


	TimerTask = {}

	LuxModule.registerclass("luxinialuacore","TimerTask",
		[[A TimerTask is a function that is executed at a certain point of time.
		When you create a new task, you specify a time in the future at which
		the function should be executed. This class also offers a very simple
		system of scheduling for a little loadbalancing.

		TimerTasks are useful when you want to execute certain actions over
		a timer of at a point of time. For example, when an enemy is killed, you
		can create a timertask function that will remove the enmey after some time -
		or you can check then if a certain condition is set and react on that.

		!Samples:

		 TimerTask.new(function () print "time over" end, 1000)
		  -- prints "time over" in about a second


		 TimerTask.new(function () print "next frame" end)
		  -- prints "next frame" in the next frame


		 TimerTask.new(
		 	function ()
		 		print "from now on 30 secs"
		 		coroutine.yield(15000)
		 		print "... 15 seconds"
		 		coroutine.yield(5000)
		 		for i=10,1,-1 do
		 			print (i)
		 			coroutine.yield(1000)
		 		end
		 		print "bang"
		 	end,1
		 ) -- a countdown that starts in a millisecond


		]],
		TimerTask,
		{		});
	local tasks = {}
	local nextframe = {}

	local pausetasks = false
	local tasktime = 1
	local tasksync

	LuxModule.registerclassfunction("new",
		[[([TimerTask]):(function f, [int time, [int variance, [int weight,
		[boolean absolute] ] ] ]) - Creates a new Task and returns it if the
		task is scheduled. The time is measured in milliseconds.

		The function will be called when the current time is larger or same as
		the time that was used for scheduling, which is as close as possible.
		Be aware that a single frame may use 3-5 ms, so multiple tasks with
		different schedule times may be called within the same frame!

		If no time is passed, the function will be called on the beginning of
		the next frame.

		You can specify a variance in which the function should be called.
		The function is then scheduled between time
		and time+variance. You can specify a weight of that task and the
		scheduling will try to place the task at a time where the totalweight
		is at a minimum. The algorithm is pretty primitive and no rescheduling
		is done here.

		If absolute is true, the moment of time is absolute, otherwise it is
		relative in the future.

		A taskobject will become garbage once its scheduling time lies in the
		past - it is like an entry in a "calendar" which becomes unimportant once
		the date passed.

		Once a task is scheduled, you cannot change the point of time when
		it will be executed.

		Note: The timertasks create coroutines that are then executed. You can
		yield a function that is called as timertask at any time, which means that
		the execution is continued later. If you pass no argument to the
		yield call (see coroutine.yield), the execution at this point is continued
		at the next frame. If you pass a number, the execution is continued
		at this point of time in the future.
		]])

	local timertaskid = 1
	function TimerTask.new (func,at,var,weight,absolute,corot)
		--print(debug.traceback())
		local self = {
			func=func,
			at=at and math.floor((not absolute and (at+tasktime)) or at),
			var=var or 0,
			id = timertaskid,
			corot = corot}
		timertaskid = timertaskid + 1
		if (self.at and self.at<=tasktime) then return nil end
		var = math.max(1,var or 1)
		weight = math.min(weight or 1,0)
		local minn,mini
		if at then
			for i=1,var do
				if (not tasks[self.at+i-1]) then
					tasks[self.at+i-1] = tasks[self.at+i-1] or {}

					table.insert(tasks[self.at+i-1],self)
					tasks[self.at+i-1].weight = weight
					return self
				end
				local n = tasks[self.at+i-1].weight
				if (not minn or minn>n) then
					minn,mini = n,i
				end
			end
		end

		if at then
			table.insert(tasks[self.at+mini-1],self)

			tasks[self.at+mini-1].weight = minn + weight
		else
			table.insert(nextframe,self)
		end
		return self
	end

	LuxModule.registerclassfunction("getTaskTime",[[
		(int time):() - The TimerTask scheduling is using a own time (which can
		be paused). The returned time is the number of milliseconds that have
		passed since luxinia started minus the time when the scheduling was
		started. If you pass an absolute time, you have to calculate the
		time with this time.]])
	function TimerTask.getTaskTime()
		return tasktime
	end

	LuxModule.registerclassfunction("when",
		"(int time):() - Returns absolute moment of time when the function is called.")
	function TimerTask.when ()
		return self.at
	end

	LuxModule.registerclassfunction("disable",
		"():() - disables a task. It won't be executed anymore.")
	function TimerTask:disable()
		self.disabled = true
	end

	LuxModule.registerclassfunction("reenable",
		[[():() - reenable the task. This is senseless once the date lies in the
		past.]])
	function TimerTask:reenable()
		self.disabled = false
	end

	LuxModule.registerclassfunction("isValid",
		"(boolean):() - returns true if the task is still scheduled in the future")
	function TimerTask:isValid ()
		return self.at>tasktime
	end

	LuxModule.registerclassfunction("pauseAll",
		"():(boolean pause) - Pauses the TimerTask scheduler. When "..
		"scheduling is paused, no functions will be executed. If pause is "..
		"is false, the scheduling is continued at the point when the system was "..
		"paused.")
	function TimerTask.pauseAll(paused)
		pausetasks = paused
	end

	LuxModule.registerclassfunction("deleteAll",
		"():() - deletes all Tasks that are scheduled.")
	function TimerTask.deleteAll ()
		tasks = {}
	end

	LuxModule.registerclassfunction("tick",
		"():() - called from the timer each frame.");
	function TimerTask.tick ()
		local t = system.time()
		tasksync = tasksync or t
		if (pausetasks) then
			tasksync = t
			return
		end

		while (tasksync<=t) do
			tasksync = tasksync + 1
			tasktime = tasktime + 1
			if (tasks[tasktime]) then
				for task,v in ipairs(tasks[tasktime]) do
					if not v.disabled then
						local ret = {tcall(v.id,"Timer task",v.func,v,tasktime)}
						if ret[1] and ret[2] then
							local at, var,weight = select(4,unpack(ret))

							--print (unpack(ret))
							--print(at,var,weight)
							local t = TimerTask.new(v.func,at,var,weight,false,ret[2])
							t.id = v.id
						end
					end
					--[=[ code before coroutines
					if (not v.disabled) then
						xpcall(
							function ()
								v:func(tasktime)
							end,
							function (err)
								print ("timer task error")
								print(err)
								print(debug.traceback())
							end
						)
					end]=]
				end

				tasks[tasktime] = nil
			end
		end

	end

	Timer.set("_core_timertask",function () TimerTask.tick() end,1,1)

	if (false) then
		local cnt = 1
		function dotest(w)
			local i = cnt + 1
			cnt = i
			local function test (task, tasktime)
				print(i.." OK "..tasktime)
				TimerTask.new(test,600,2,w)
			end
			TimerTask.new(test,600)
		end
		for i=1,5 do dotest(i) end
	end

	luxinia.think =
	function (...)
		local nf = nextframe
		nextframe = {}
		for i,task in ipairs(nf) do
			if not task.disabled then
				task.co = task.co or coroutine.create(task.func,defaultSize)
				local noerr = cocall (task.co)
				if noerr and coroutine.status(task.co) ~= "dead" then
					table.insert(nextframe,task)
				end
			else
				print "killing that darn coroutine"
			end
		end
		Timer.think(...)
	end
end
