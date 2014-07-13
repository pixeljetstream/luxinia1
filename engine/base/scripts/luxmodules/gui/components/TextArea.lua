TextArea = {
	pressedskin = "textarea_hover",
	hoveredskin = "textarea_hover",
	defaultskin = "textarea",
	focusedskin = "textarea_hover",
}
setmetatable(TextArea,{__index = Component})

LuxModule.registerclass("gui","TextArea",
	[[A textarea for textblocks.]],TextArea,{},"Component")

function TextArea.new(class,x,y,w,h)
	local self = Component.new(class,x,y,w,h)
	self:addKeyListener(
		KeyListener.new(function (kl,keyevent) self:onKeyTyped(keyevent) end, true)
	)

	self.dataTextField = {
		text = {},
		cursorpos = 0,
	}
	
	self:setSkin("default")

	return self
end

function TextArea:createVisibles ()
	Component.createVisibles(self)
end


function TextArea:deleteVisibles ()
	Component.deleteVisibles()
end

function TextAray:setVisibility (visible)
	Component.setVisibility(visible)
end