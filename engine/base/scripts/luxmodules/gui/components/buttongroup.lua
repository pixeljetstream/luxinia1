ButtonGroup = {}
LuxModule.registerclass("gui","ButtonGroup",
	[[A ButtonGroup is a list of Buttons that are informed when another button 
	was pressed. A ButtonGroup has only one active button per time. The 
	ButtonGroup is assigning ActionListeners to the buttons to receive calls 
	on clickactions. ]],
	ButtonGroup,
	{})

LuxModule.registerclassfunction("new",	
	[[(ButtonGroup):() - creates a new Buttongroup]])	
function ButtonGroup.new ()
	local self = { list = {} }
	setmetatable(self,{__index = ButtonGroup})
	return self
end

local function void () end  -- a function that does nothing

LuxModule.registerclassfunction("addButton",	
	[[(Button):(Button, [fnOnPush], [fnOnRelease], [boolean toggle]) - adds a button to the list. 
	If this button is pushed or released (because another button was pressed),
	the corresponding functions are being called. The signature of the functions
	is: 
	 function react ([theclickedbutton], [previousbutton])
	You can use this functions to react on changes in the Buttongroup. Note that
	if the currently active button is pressed again, no event is generated here.
	
	If toggle is true, clicking the active element will deselect the group.
	
	This function makes the given button automaticly pushable.
	 button:setPushable(true)
	]])	
function ButtonGroup:addButton (button, onPush, onRelease, toggle)
	assert(button and type(button) == "table", "Argument 1 must be a button")
	self.list[button] = {button = button, onPush = onPush or void, 
		onRelease = onRelease or void, 
		toggle = toggle,
		alistener = function () self:click(button) end}	
	button:addActionListener(self.list[button].alistener)
	button:setPushable(true)
	return button
end

LuxModule.registerclassfunction("remove",	
	[[():(button) - removes the button from the buttongroup]])
function ButtonGroup:removeButton (button)
	assert(self.list[button], "Button is not member of the buttongroup")
	button:removeActionListener(self.list[button].alistener)
	self.list[button] = nil
end

LuxModule.registerclassfunction("delete",	
	[[():() - deltes the group removing all the actionlisteners and so on]])	
function ButtonGroup:delete()
	for i,v in pairs(self.list) do 
		self:removeButton(v)
	end
end

LuxModule.registerclassfunction("click",	
	[[():(button) - called by the actionlisteners. Calls automaticly the 
	assigned functiosn from the addButton function. Does nothing if the 
	button is currently marked as active (and is pushed). Calling the click 
	function with nil will deselect any button (except if you added the button as toggleable).]])	
function ButtonGroup:click(button)
	if (button~=nil and self.active == self.list[button] and (button:isPushed() or self.active.toggle))
	then 
		if (button and self.list[button]==self.active and self.active.toggle) then
			button = nil
		else
			return 
		end
	end
	
	local prev = self.active
	if (prev) then prev.button:setPushState(false) prevbtn = prev.button end
	if (button) then 
		button:setPushState(true) 
		self.active = self.list[button]
	else 
		self.active = nil
	end
	
	if (prev) then prev.onRelease(button,prevbtn) end
	if (self.active) then self.active.onPush(button,prevbtn) end
	
	self:onClicked(button,prevbtn)
end

LuxModule.registerclassfunction("onClicked",	
	[[():([theclickedbutton],[previousbutton]) - function that is called when
	a clickevent occured. This method does nothing and can be overloaded without 
	consequence.]])	
function ButtonGroup:onClicked (current, old) end