GroupFrame = {}
LuxModule.registerclass("gui","GroupFrame",
	[[Groupframes are containers with an initial skin]],GroupFrame,{}
	,"Container")
setmetatable(GroupFrame,{__index = Container})


LuxModule.registerclassfunction("new", 
	[[(GroupFrame):(table class, int x,y,w,h,[Skin2D skin]) - 
	creates a container with the given bounds. If no skin is given the 
	default skin for frames is being used.]])
function GroupFrame.new (class,x,y,w,h,skin)
	local self = Container.new(class,x,y,w,h)
	
	self:setSkin("default")

	return self
end