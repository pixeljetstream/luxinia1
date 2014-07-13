local cfg = {
	applylistener = {}
}
cfg.config = {}

function cfg.select(key,value)
	cfg.config[key] = value
end

function cfg.onApply(fn)
	table.insert(cfg.applylistener,fn)
end


function cfg.winres (n)
	local ww,wh = window.width(),window.height()
	if (n) then
		local r = {{640,480},{800,600},{1024,768},{1280,1024},{1600,1200}}
		local w,h = unpack(r[n])
		window.width(w)
		window.height(h)
		return
	end

	return cfg.config.winres or ({
		[ww.."x"..wh] = 1,
		["640x480"]=1,
		["800x600"]=2,
		["1024x768"]=3,
		["1280x1024"]=4,
		["1600x1200"]=5})[ww.."x"..wh]
end


function cfg.bpp(n)
	if n then
		window.bpp((n+1)*8)
		return
	end
	return cfg.config.bpp or (window.bpp()/8-1)
end

function cfg.detail(n)
	if n then return rendersystem.detail(n) end
	return cfg.config.detail or rendersystem.detail()
end

function cfg.maxfps(n)
	if n then return system.maxfps((n-1)*15) end
	return cfg.config.maxfps or math.floor(system.maxfps()/15)+1
end

local function bool(name,fn)
	assert(fn,"no valid function for: "..name)
	cfg[name] = function (on)
		if on ==nil then
			if cfg.config[name]~=nil then
				return cfg.config[name]
			else
				return fn()
			end
		end
		fn(on)
	end
end


function cfg.colordepth(n)
	if not n then
		return cfg.config.colordepth or
			window.bpp()/8-1
	else
		window.bpp((n+1)*8)
	end
end

function cfg.stencilbits(n)
	if not n then
		return cfg.config.stencilbits or
			window.stencilbits()/8-1
	else
		window.stencilbits((n+1)*8)
	end
end

function cfg.depthbits(n)
	if not n then
		return cfg.config.depthbits or
			window.depthbits()/8-1
	else
		window.depthbits((n+1)*8)
	end
end

function cfg.sounddevice(dev)
	if not dev then
		return cfg.config.sounddevice or sound.getdevicename()
	else
		cfg.config.sounddevice = sound.setdevice(dev)
	end
end

function cfg.multisamples(n)
	if not n then
		local ms = window.multisamples()/2
		return cfg.config.multisamples or (ms < 5 and ms+1 or 6)

	else
		window.multisamples( n < 6 and ((n-1)*2) or 16)
	end
end

function cfg.vsynch(n)
	if n==nil then
		local ms = rendersystem.swapinterval() > 0
		return cfg.config.vsynch or (ms)

	else
		rendersystem.swapinterval(n and 1 or 0)
	end
end

bool("anisotropic",rendersystem.texanisotropic)
bool("fullscreen",window.fullscreen)
bool("texcompression",rendersystem.texcompression)
bool("usevbos",rendersystem.usevbos)
bool("nosound",system.nosound)
bool("nodefaultgpuprogs",rendersystem.nodefaultgpuprogs)
bool("noglextensions",rendersystem.noglextensions)
local function registerDraw(name)
	assert(render["draw"..name],"no valid function found: "..name)
	bool("draw"..name,render["draw"..name])
	return registerDraw
end
registerDraw "all" "axis" "bonelimits" "bonenames" "lights" "camaxis"
	"fps" "nogui" "stats" "normals" "projectors" "shadowvolumes" "stencilbuffer"
	"wire"
	
function cfg.drawpgraph(n)
	if (n==nil) then
		local gs = {}
		local ms = false
		for name,v in pairs(pgraph) do
			if (name:match("get(.+)")) then
				if (v():draw()) then
					ms = true
					break
				end
			end
		end
		
		return cfg.config.drawpgraph or (ms)
	else
		pgraph.draw(n)
	end
end

function cfg.apply()
	saveprojecthistory()
	for i,fn in ipairs(cfg.applylistener) do
		fn()
	end
	for name,val in pairs(cfg.config) do
		cfg[name](val)
	end
	--local x,y = window.pos()
	window.update()
	--window.pos(x,y)
	MouseCursor.showMouse(true)
end

return cfg
