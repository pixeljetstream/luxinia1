local keyListeners = {}

KeyEvent = {
	KEY_RELEASED = 2,
	KEY_PRESSED = 3,
	KEY_TYPED = 4,
	STATENAMES = {
		"Not a valid state",
		"RELEASED",
		"PRESSED",
		"TYPED"
	}
}

LuxModule.registerclass("luxinialuacore","KeyEvent",
	[[Keyevents are produced if the use types a key.]],KeyEvent,
	{
		KEY_RELEASED = "[int] - constant if key was released",
		KEY_PRESSED = "[int] - constant if key was pressed",
		KEY_TYPED = "[int] - constant if key was typed",
		STATENAMES = "[table] - a list with names for constants",
		key = "{[string]} - key that was typed",
		keycode = "{[int]} - code of the key that was typed",
		state = "{[int]} - state of the button (KEY_PRESSED,...)",
		since = "{[int]} - frameid since when the key is hold down",
		new = [[(KeyEvent):(string key, int keycode, int since, int state) -
			creates a new KeyEvent with the given parameters.]],
		toString = "(string):(KeyEvent) - returns simple string representation"
	})

function KeyEvent.new (key, keycode, since, state)
	local self = {
		key = key,
		keycode = keycode,
		since = since,
		state = state
	}
	setmetatable(self,{__index = KeyEvent,__newindex =
		function () error("Cannot modify a Keyevent") end,
		__tostring = function(self) return self:toString() end
	})
	return self
end

function KeyEvent:toString()
	return string.format("[KeyEvent %s %i %s(%i)]",self.key,self.keycode,
		KeyEvent.STATENAMES[self.state],self.state)
end


KeyListener = {}
LuxModule.registerclass("luxinialuacore","KeyListener",
	[[A Keylistener receives keyevent callbacks on KeyEvents that are
	produced by a specific component.]],KeyListener,
	{
		["1"] = "{[function]} - function that is called",
		["2"] = "{[boolean]} - if true, function is called on releases",
		["3"] = "{[boolean]} - if true, function is called on pressed",
		["4"] = "{[boolean]} - if true, function is called on types",
	}
)

LuxModule.registerclassfunction("new",
	[[(KeyListener):(function callback, boolean onType,
		onPress, onRelease) - creates a keylistener that calls the callback
		function if the Keyeventtype matches the given filter.

		Callbackfunction signature:
		 function listener (KeyListener, KeyEvent)
	]])
function KeyListener.new (callbackFN, types,presses,releases)
	local self = {
		callbackFN,
		releases,
		presses,
		types
	}

	setmetatable(self,{
		__index = KeyListener,
		__tostring = function (self) return self:toString() end
	})
	return self
end

function KeyListener.eventcall(self,keyevent)
	if (self[keyevent.state]) then self[1](self,keyevent) end
end

function KeyListener:toString ()
	local tab = {}
	if (self[2]) then table.insert(tab,"release") end
	if (self[3]) then table.insert(tab,"press") end
	if (self[4]) then table.insert(tab,"type") end
	return string.format("[KeyListener %s %s]",tostring(self[1]),table.concat(tab,","))
end

--------------------------------------------------------------------------------

Keyboard = {}

LuxModule.registerclass("luxinialuacore","Keyboard",
	[[
	The Keyboard class handles events from the input class of the luxinia
	core classes. It provides a system for callbackmechanisms and other
	utility functions.
	]],Keyboard,
	{})

Keyboard.keycode = {
	UP 		= 283,
	DOWN 	= 284,
	LEFT 	= 285,
	RIGHT 	= 286,
	MINUS	= 45,
	PLUS 	= 43,
	ENTER 	= 294,
	F1 		= 258,
	F2 		= 259,
	F3 		= 260,
	F4 		= 261,
	F5 		= 262,
	F6 		= 263,
	F7 		= 264,
	F8 		= 265,
	F9 		= 266,
	F10 	= 267,
	F11 	= 268,
	F12 	= 269,
	ESC 	= 257,
	INS 	= 296,
	BACKSPACE = 295,
	DEL 	= 297,
	PAGEUP	= 298,
	PAGEDOWN = 299,
	HOME 	= 300,
	END		= 301,
	LCTRL 	= 289,
	RCTRL	= 290,
	ALT		= 291,
	LSHIFT	= 287,
	RSHIFT	= 288,
	TAB		= 291,
	ALTGR	= 292,
	SPACE	= 32,
	NUM0	= 302,
	NUM1	= 303,
	NUM2	= 304,
	NUM3	= 305,
	NUM4	= 306,
	NUM5	= 307,
	NUM6	= 308,
	NUM7	= 309,
	NUM8	= 310,
	NUM9	= 311,
	NUMDIV = 312,
	NUMMUL = 313,
	NUMMINUS = 314,
	NUMPLUS = 315,
	NUMCOMMA = 316,
	NUMENTER = 318,
	APPEXIT = 319,
}
do
	local c = '0'
	for i=48,48+10 do
		Keyboard.keycode[c] = i
		c = string.char(string.byte(c)+1)
	end

	local c,i = string.char(string.byte('A')-1),64
	repeat
		i = i + 1
		c = string.char(string.byte(c)+1)
		Keyboard.keycode[c] = i
	until c=='Z'
end

LuxModule.registerclassfunction("keycode",
		"[table] - List of keycode constants")
for i,v in pairs(Keyboard.keycode) do
	LuxModule.registerclassfunction("keycode."..i,
		"[int] - "..string.lower(i).." key keycode")
end

LuxModule.registerclassfunction("addKeyListener",
	[[():(KeyListener listener) - adds the keylistener to the listener list]])
function Keyboard.addKeyListener(keylistener)
	table.insert(keyListeners,keylistener)
end

LuxModule.registerclassfunction("removeKeyListener",
	[[():(KeyListener listener) - removes keylistener from the listener list]])
function Keyboard.removeKeyListener(keylistener)
	for i,v in pairs(keyListeners) do
		if (v == keylistener) then
			table.remove(keyListeners,i)
			return
		end
	end
end


local pressedkeys = {}
local bindings = {}
local keybindsenabled = true

LuxModule.registerclassfunction("enableKeyBinds",
		"():(on) - Enables or disables the keybinding (if console.active()==true, the binding is disabled automaticly). "..
		"This function is currently used by the GUI, which means that it may not be safe to disable "..
		"the keybinds in combination with using the GUI.")
function Keyboard.enableKeyBinds(on)
	keybindsenabled = on and true
end

LuxModule.registerclassfunction("keyBindsEnabled",
	"(on):() - returns enabled state of the keybinding")
function Keyboard.keyBindsEnabled()
	return keybindsenabled 
end

LuxModule.registerclassfunction("popKeys",
	"boolean value - if set to false, no keys are popped (input.popkey) and thus, no keyevents are handled. "..
	"This function can be used for handling manually popping keys.")
Keyboard.popKeys = true

--table.setn(pressedkeys,1024)
--table.setn(bindings,1024)

local function think ()
	local abs,rel = system.time()
	local lastkeys = {}
	local waspressed = {}

	local n = 0
	while (Keyboard.popKeys) do
		local inp,keycode,inpstate = input.popkey();
		if inp then
			--inp = string.char(inp):upper():byte()
		end
		if keycode and keycode<256 then
			--keycode = string.char(keycode):upper():byte()
		end
		n = n+1
		assert(n<256,"Internal Mousequeue error, inform devs please")
		
		if (not inp) then break end
		--table.insert(lastkeys,{inp,inpstate})
		local event,e2
		local frame = pressedkeys[inp] or {Timer.frame(),inp}
		if (inpstate) then
			e2 = KeyEvent.new(string.char(inp),keycode,frame[1],KeyEvent.KEY_TYPED)
			if (pressedkeys[keycode]) then

			else
				event = KeyEvent.new(string.char(inp),keycode,frame[1],KeyEvent.KEY_PRESSED)
			end
			pressedkeys[keycode] = frame
			waspressed[keycode] = frame
		else
			event = KeyEvent.new(string.char(inp),keycode,frame[1],KeyEvent.KEY_RELEASED)
--			_print(inp,keycode)
			pressedkeys[keycode] = nil
		end
		for i,v in ipairs(keyListeners) do
			if (event and v[event.state]) then
				v[1](v,event)
			end
			if (e2 and v[e2.state]) then
				xpcall(function () v[1](v,e2) end,
					function (err)
						print(debug.traceback(err,1))
					end)
			end
		end
--		_print(inp)
	end

	if (not console.active() and keybindsenabled) then
		local function firebind (keycode,since )
		
			if keycode and keycode<256 then
				keycode = string.char(keycode):upper():byte()
			end
			if (bindings[keycode] and bindings[keycode][3]-abs<0) then
				xpcall (
					bindings[keycode][2],
					function (err)
						print("Error in keybinding: ")
						print(err)
						print(debug.traceback())
					end
				)
				if (bindings[keycode]) then
					bindings[keycode][3] = abs+bindings[keycode][1]
				end
			end
		end
		for keycode,since in pairs(pressedkeys) do
			firebind(keycode,since)
		end
		
		for keycode,since in pairs(waspressed) do
			firebind(keycode,since)
		end
	end
end

Timer.set("_core_keyboard",think)

LuxModule.registerclassfunction("getPressedKeys",
		[[([table keys,codes]):() - returns two tables: One with all pressed keynames and
		one with the keycode values. This only includes keys that are listed in the
		keycode table of the Keyboard class. It is also a slow method since it polls
		every keycode one by one.]])
function Keyboard.getPressedKeys ()
	local keys = {}
	local codes = {}
	for i,v in pairs(Keyboard.keycode) do
		if (input.iskeydown(v)) then table.insert(keys,i) table.insert(codes,v) end
	end
	return keys,codes
end

LuxModule.registerclassfunction("isKeyDown",
	[[(boolean isdown):(int/string code) - this is the same function as input.iskeydown.
	It is only included here for consistence. This method is insensitive to
	modifier keys such as SHIFT or ALT. If you poll your keycodes for game actions
	you should use this method.

	The function accepts either the numerical keycode as used in the Keyboard.keycode
	table or the nameentry in Keyboard.keycode (i.e. "LSHIFT").

	This function also handles "CTRL" and "SHIFT" as special codes - since
	the left and right shift / ctrl keys are different keys and must
	be polled each - however, if you just pass these strings as arguments,
	the function will return true if either the left or right key is
	pressed.

	Since 0.93a, this function is case insensitive.]])
Keyboard.isKeyDown = function (x)
	if (type(x)~="number") then
		x = x:upper()
		if x == "CTRL" then
			return Keyboard.isKeyDown("LCTRL") or Keyboard.isKeyDown("RCTRL")
		elseif x == "SHIFT" then
			return Keyboard.isKeyDown("LSHIFT") or Keyboard.isKeyDown("RSHIFT")
		end
		x = Keyboard.keycode[x:upper()]
	end
	assert(x,"arg1 must be named key as string or number")
	return input.iskeydown(x)
end


LuxModule.registerclassfunction("setKeyBinding",
	[[():(int/string keycode, [function, [int delay] ]) - adds keybinding for this
	key and calls the given function every delay milliseconds while the key is
	pressed. The keycode can be a string that describes the key same as the
	keycode names (i.e. "W" or "DEL"). If only a keycode is provided, the
	keybinding is removed.]])
function Keyboard.setKeyBinding(keycode,fn,delay)
	if (type(keycode)=="string") then keycode = Keyboard.keycode[keycode:upper()] end
	assert(type(keycode)=="number", "keycode number expected")
	delay = delay or 200
	delay = delay<=0 and 1000000 or delay
	if (not fn) then
		bindings[keycode] = nil
	else
		bindings[keycode] = {}
		bindings[keycode][1] = delay
		bindings[keycode][2] = fn
		bindings[keycode][3] = 0
	end
end


LuxModule.registerclassfunction("removeKeyBinding",
	[[():(int keycode/string keycode) - removes keybinding for this key]])
function Keyboard.removeKeyBinding(keycode)
	if (type(keycode)=="string") then keycode = Keyboard.keycode[keycode:upper()] end
	assert(type(keycode)=="number", "keycode number expected")
	bindings[keycode] = nil
end


LuxModule.registerclassfunction("removeAllKeyBindings",
	[[():() - removes all keybindings, and defaults APPEXIT key back to system.exit call]])
function Keyboard.removeAllKeyBindings()
	bindings = {}
	Keyboard.setKeyBinding(Keyboard.keycode.APPEXIT,function() print("EXIT") system.exit() end,400)
end


Keyboard.setKeyBinding(Keyboard.keycode.APPEXIT,function() print("EXIT") system.exit() end,400)