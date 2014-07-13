class "Tooltipevent"

function Tooltipevent : Tooltipevent (source, x,y)
	self.source = source
	local mx,my = MouseCursor.pos()
	self.x = x or mx
	self.y = y or my
end

function Tooltipevent : getSource ()
	return self.source
end

function Tooltipevent : getMousePos ()
	return self.x,self.y
end

function Tooltipevent : getX()
	return self.x
end

function Tooltipevent : getY()
	return self.y
end

function Tooltipevent : consume ()
	self.consumed = true
end

function Tooltipevent : isConsumed()
	return self.consumed
end
