-- Using lua lanes for threading
-- lualanes manual: 
--      http://akauppi.googlepages.com/lanes-2.0.3-manual.html
-- demonstrates usage of threads in combination with luasocket
-- for downloading a web page in an extra thread


frame = Container.getRootContainer():add(
  GroupFrame:new(0,0,window.width(),window.height()))
local tx = frame:add(Label:new(5,5,window.width()-20,window.height()-20,"this may take a while..."))


require "lanes"

local function downloadthread(n)
	-- we cannot access ANY functions or class of the Luxinia core
	-- we also cannot acces any variables outside
	-- the scope of this function definition
	
	-- redirect package search paths to luxinia\base\lib
	package.cpath = [[?.dll;.\base\lib\?.dll;.\base\lib\?\?.dll;./?.so;base/lib/?.so;]]
	package.path = [[?.lua;.\base\lib\?.lua;.\base\lib\?\?.lua;]]
	
	-- libs must be loaded EXPLICITLY (even if we required them before
	-- outside this scope)
	
	http = require "socket.http"
	
	web = http.request("http://www.luxinia.de/test/luxtest.php")
	
	-- do some senseless work to stretch time
	-- otherwise loading is pretty much instantly
	sum = 0
	n = n or 0
	for i = 1,n do
		sum = sum + i
	end
	
	return web,sum
	
end

-- use lanes to prepare the func
-- by default no default libs are loaded

local threadwrapper = lanes.gen ("*", downloadthread )
-- kick off thread
-- reduce the number passed if loading takes
-- too long (or remove entirely)
local threadhandle = threadwrapper(10000000)


TimerTask.new(function()
	local page
	local n = 1
	repeat
		tx:setText("this may take a while"..string.rep(".",n))
		n = (n%15) + 1
		
		coroutine.yield(100)
		
		-- Dont wait for join (pass 0), as we
		-- waited via coroutine yield, otherwise
		-- we would lock up the application in 
		-- a busy wait.
		-- The way we currently do allows
		-- us to update the text above
		-- in an animated fashion.
		
		page = threadhandle:join(0)
	until page
	tx:setText(page)
end)