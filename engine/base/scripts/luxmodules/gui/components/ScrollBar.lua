ImgSlider = {}
setmetatable(ImgSlider,{__index = Container})
function ImgSlider.new(class,x,y,w,h,fgcolor,bgcolor)
	local self = {}
		
	self = Container.new(class,x,y,w,h)
	self.imgsld = self:add(ImageComponent:new(2,2,w-4,h-4,matsurface.vertexcolor()))
	
	--self.imgsld = self:add(Label:new(2,2,w-4,h-4,"+++"))
	local clr = fgcolor or {1,1,1,1}
	self.imgsld:color(unpack(clr))
	
	self.imgbg = self:add(ImageComponent:new(0,0,w,h,matsurface.vertexcolor()))
	--self.imgbg = self:add(Label:new(0,0,w,h,"-----"))
	clr = bgcolor or {0,0,0,1}
	self.imgbg:color(unpack(clr))
	
	self:setScrollsize(1,2)

	return self
end

function ImgSlider:setScrollsize(viscnt,maxcnt)
	self.viscnt = viscnt
	self.maxcnt = maxcnt
	self.sliderincrement = maxcnt~=viscnt and (1/(maxcnt-viscnt)) or 1
	
	local w,h = self:getSize()
	self.imgsld:setSize((viscnt*w/maxcnt)-4,h-4)
end

function ImgSlider:setSliderPos(pos,nocallback)
	local prev = self.sliderpos
	self.sliderpos = math.min(1,math.max(0,pos))
	self.sliderpos = math.floor(self.sliderpos/self.sliderincrement+0.45)*self.sliderincrement
	

	if (not nocallback and self.onValueChanged and self.sliderpos~=prev) then
		if (not self:onValueChanged(self.sliderpos,prev)) then
			self.sliderpos = prev
			return
		end
	end
	
	
	local x = (self:getWidth()-4-self.imgsld:getWidth())*self.sliderpos
	
	self.imgsld:setLocation(x+2,2)
end

function ImgSlider:setSliderTo(x,y)
	local iw = self.imgsld:getWidth()
	local w = self:getWidth()
	local x = x-iw/2
	x = math.min(math.max(0,x/(w-iw)),1)
	self:setSliderPos(x)
end

function ImgSlider:mousePressed(me)
	self:lockMouse()
	Container.mousePressed(self,me)
	self:setSliderTo(me.x,me.y)
	if self:isFocusable() then self:focus() end
end

function ImgSlider:isFocusable() return self:isVisible() end

function ImgSlider:mouseReleased(me)
print "RELEASE === "
	Container.mouseReleased(self,me)
	self:unlockMouse()
end

function ImgSlider:mouseMoved(me)
	Container.mouseMoved(self,me)
	print(me.x,me.y)
	if (me:isDragged() and self:getMouseLock() == self) then
		self:setSliderTo(me.x,me.y)
	end
end


function ImgSlider:mouseMoved(me)
	Container.mouseMoved(self,me)
	print(me.x,me.y)
	if (me:isDragged() and self:getMouseLock() == self) then
		self:setSliderTo(me.x,me.y)
	end
end