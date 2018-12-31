function CCaddRot(camact)
	local mcon = Container:new(0,0,window.refsize())
	local mx,my = 0,0
	local listener = MouseListener.new(
		function (self,me)
			local root = Container.getRootContainer()
			if me:isPressed() then
				mx,my = me.x,me.y
				MouseCursor.pos(mx,my)
				mcon:lockMouse()
				return
			end
			if me:isDragged() and not me:isPressed() and Component.getMouseLock() == mcon then
				local dx,dy = me.x-mx,me.y-my
				if dx==0 and dy==0 then return end
				MouseCursor.pos(mx,my)
				local a,b,c = camact:rotdeg()
				camact:rotdeg(math.max(-90,math.min(90,a-dy*0.5)),b,c-dx*0.5)
			elseif not me:isPressed() then
				mcon:unlockMouse()
			end
		end
	)
	
	
	mcon:addMouseListener(listener)
	Container.getRootContainer():add(mcon)
	
	function mcon:onParentLayout(nx,ny,nw,nh)
		self:setBounds(nx,ny,nw,nh)
	end
	
	return mcon
end

