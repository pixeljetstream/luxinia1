TitleFrame = {
	skinnames = {
		defaultskin = "titleframe",
		hoveredskin = "titleframe_hover",
		pressedskin = "titleframe_pressed",
		pressedhoveredskin = "titleframe_hover_pressed",
		focusedskin = "titleframe_focus",
		focusedhoveredskin = "titleframe_focus_hover",
		focusedpressedskin = "titleframe_focus_pressed",
		focusedhoveredpressedskin = "titleframe_focus_hover_pressed",
	}
}
LuxModule.registerclass("gui","TitleFrame",
	[[TitleFrames are containers with an initial skin. Following skinnames are being used:

	* titleframe
	* titleframe_hover
	* ...]],TitleFrame,{}
	,"Container")
setmetatable(TitleFrame,{__index = Container})


LuxModule.registerclassfunction("new",
	[[(TitleFrame):(table class, int x,y,w,h,[Skin2D skin],[string title]) -
	creates a container with the given bounds. If no skin is given the
	default skin for frames is being used.]])
function TitleFrame.new (class,x,y,w,h,skin,title)
	local self = Container.new(class,x,y,w,h)

	self:setSkin("default")
	--self.titlelabel = self:add(Label:new(8,2,w-16,20))

	self:setTitle(title or "")
	self:getSkin():setLabelPosition(10,5)

	return self
end

LuxModule.registerclassfunction("setTitle",
	[[(TitleFrame):(TitleFrame,text) - sets titlecaption]])
function TitleFrame:setTitle(title)
	--self.titlelabel:setText(title)
	self:getSkin():setLabelText(title)
end

LuxModule.registerclassfunction("setTitleAlign",
	[[(TitleFrame):(TitleFrame,align) - sets titlecaption alignment. Can be either
	Skin2D.ALIGN.LEFT, Skin2D.ALIGN.RIGHT or Skin2D.ALIGN.CENTERED or a number
	for the absolute x coordinate.]])
function TitleFrame:setTitleAlign(align)
	self:getSkin():setLabelPosition(align,5)
end

--function TitleFrame:setBounds(x,y,w,h)
--	self.titlelabel:setSize(w-16,20)
--	Container.setBounds(self,x,y,w,h)
--end
