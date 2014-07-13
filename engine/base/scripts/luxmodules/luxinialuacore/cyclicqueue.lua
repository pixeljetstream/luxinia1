
CyclicQueue = {}
LuxModule.registerclass("luxinialuacore","CyclicQueue",
		"The CyclicQueue is a utility class for storing cyclic fields. "..
		"This is usefull for creating histories. The index operator ([]) can "..
		"be used to index the queue. I.e. mylist[1] will always return the "..
		"elment that was inserted last.",CyclicQueue,
		{
			size = "{[int]} - size of the list",
			history = "{[table]} - the table containing the entries",
			start = "{[int]} - position of current element",
			count = "{[int]} - number of entries in list",
			overwrite = "{[boolean]} - if false, an exception is thrown if the list is full",

			new = "(CyclicQueue):(int size, [boolean overwrite]) - "..
				"Creates a CyclicQueue object of given size. If overwrite is false (default), "..
				"an error is thrown if the array is full.",
			push = "():(CyclicQueue,value) - pushes a value on the list.",
			pop = "([value]):(CyclicQueue) - pops a value from the queue. If no element is in the list, nothing is returned",
			top = "([value]):(CyclicQueue) - returns the latest inserted element"
		}
	)

function CyclicQueue.new (size, overwrite)
	local self = {}

	self.size = size
	self.history = {}
	self.start = 0
	self.count = 0
	self.overwrite = overwrite or false
--	table.setn(self.history,size)

	setmetatable(self, CyclicQueue)

	return self
end

function CyclicQueue:push (value)
	if (self.size == self.count) then
		if (not self.overwrite) then error(string.format("CyclicQueue overflow"),2) end
		self.start = math.mod(self.start + 1,self.size)
	else
		self.start = math.mod(self.start + 1,self.size)
		self.count = self.count + 1
	end
	self.history[self.start] = value
end

function CyclicQueue:pop ()
	if (self.count == 0) then return nil end
	self.count = self.count - 1
	local index = math.mod(self.start-self.count+self.size,self.size)

	return self.history[index]
end

function CyclicQueue:top ()
	if (self.count == 0) then return nil end
	return self.history[self.start]
end

function CyclicQueue:bottom ()
	if (self.count == 0) then return nil end
	return self.history[math.mod(self.start-self.count+self.size,self.size)]
end

function CyclicQueue:__index (n)
	if (type(n) == "number") then
		local index = math.mod(self.start-n+1+self.size,self.size)
		return rawget(self.history,index)
	end
	if (rawget(self,n)) then return rawget(self,n) end
	return CyclicQueue[n]
end

function CyclicQueue:__newindex(n,value)
	rawset(self,n,value)
end

function CyclicQueue:__call (...)
	if (select("#",...)>0) then
		for i=1,select('#',...) do
			self:push(select(i,...))
		end
	else
		return CyclicQueue.pop(self)
	end
end
