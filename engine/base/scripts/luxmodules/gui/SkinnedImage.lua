SkinnedImage = {}
setmetatable(SkinnedImage,{__index = Skin2D})

LuxModule.registerclass("gui","SkinnedImage",
	[[A skinned image is an image that uses a special texture to create a 
	2D rectangle visual surface.
	
	The skin is created from 9 areas on the texture:
	
	 1 2 3
	 4 5 6
	 7 8 9
	
	The areas 1,3,7 and 9 (the corners) are not stretched, while
	2 and 8 are stretched horizontaly while 4 and 6 are stretched
	verticaly. Area 5 is stretched in both dimensions, depending
	on the chosen dimension of the image. 
	
	The SkinnedImage class can handle multiple verticaly aligned
	skins on one texture that can be displayed one per time.]],
	SkinnedImage,{},"Skin2D")

LuxModule.registerclassfunction("new",
	[[(SkinableImage):(string imagepath, int width, height, top, right, 
	bottom, left, n) - Creates an stretched image. Use the imagesize for
	the width and height parameter, while top,right,bottom and left should
	be the widths and heights of the areas 2,6,8 and 4. 
	
	n denotes the number of skins on the image. I.e if you have a button
	with two states, you can put them both verticaly aligend next to each other
	on the same texture and pass 2 as n. One image at time can be selected to be 
	displayed.
	
	Be aware that the filtering is switched of for the texture.]])
function SkinnedImage.new (imgpath, imwidth,imheight,top,right,bottom,left,n)
	local self = {}
	setmetatable(self, {__index = SkinnedImage})

	local com =rendersystem.texcompression()
	rendersystem.texcompression(false)
	self.image = type(imgpath)=="string" and texture.load(imgpath,false,false) 
		or imgpath
	rendersystem.texcompression(com)
	local sectors = {}
	self.sectors = sectors
	self.l2d = {}

	for k=1,n do
		sectors[k] = {
			{top/imheight, right/imwidth, bottom/imheight, left/imwidth, 1/n,1},
			{top,right,bottom,left}
		}

		local l2d = l2dimage.new("skinned image",self.image)
		l2d:link(self.l2dparent)
		
		l2d:scissor(true)
		l2d:scissorlocal(true)
		l2d:scissorparent(true)
		l2d:scissorsize(w,h)
		l2d:rfNodraw(true)
		table.insert(self.l2d,l2d)

		-- 0   1   2   3
		-- 
		-- 4   5   6   7
		--
		-- 8   9  10  11
		-- 
		--12  13  14  15		
		local indices = {
			0, 1, 5, 4,  1, 2, 6, 5,  2, 3, 7, 6,
			4, 5, 9, 8,  5, 6,10, 9,  6, 7,11,10,
			8, 9,13,12,  9,10,14,13, 10,11,15,14
		}
		
		l2d:usermesh(vertextype.vertex32normals(),16,table.getn(indices))
		l2d:indexCount(table.getn(indices))
		l2d:vertexCount(16)
		l2d:scale(1,1,1)

		l2d:indexPrimitivetype(primitivetype.quads())

		
		for i,v in ipairs(indices) do
			l2d:indexValue(i-1,v)
			--local j = i*4
			--l2d:indexPrimitive(i,
			--	indices[j+1],indices[j+2],indices[j+3],indices[j+4]
			--)
		end

		local sz = sectors[k][1]
		local w,h = sz[5],sz[6]
		local x = (k-1)/n
		--print(x,unpack(sectors[k][1]))
		local pos = {
			{0,0},		{sz[4],0},		{w-sz[2],0},		{w,0},
			{0,sz[1]},	{sz[4],sz[1]},	{w-sz[2],sz[1]},	{w,sz[1]},
			{0,h-sz[3]},{sz[4],h-sz[3]},{w-sz[2],h-sz[3]},	{w,h-sz[3]},
			{0,h},		{sz[4],h},		{w-sz[2],h},		{w,h}
		}
		
		for i=0,15 do
			l2d:vertexPos(i,math.mod(i,4)/3,math.floor(i/4)/3,0)
			l2d:vertexTexcoord (i,pos[i+1][1]+x,1-pos[i+1][2],0)
			l2d:vertexColor(i,1,1,1,1)
			l2d:vertexNormal(i,0,1,0)
		end
		l2d:indexMinmax()
		l2d:rfBlend(true)
		l2d:rsBlendmode(blendmode.decal())
		l2d:rfNodraw(true)
	end
	self.scale = {1,1}	
	return self
end

		-- 0   1   2   3
		-- 
		-- 4   5   6   7
		--
		-- 8   9  10  11
		-- 
		--12  13  14  15		
LuxModule.registerclassfunction("setSize",
	[[():(SkinnedImage, int w,h) - Resizes the image to the given size. 
	This will not scale the image but will respect the dimensions of the 
	borders.]])
function SkinnedImage:setSize(w,h)
	local sectors = self.sectors
	w = w / self.scale[1]
	h = h / self.scale[2]
	if (self.w == w and self.h == h) then return end
	for i,l2d in pairs(self.l2d) do
		local sz = sectors[i][2]
		
		local pos = {
			{0,0},		{sz[4],0},		{w-sz[2],0},		{w,0},
			{0,sz[1]},	{sz[4],sz[1]},	{w-sz[2],sz[1]},	{w,sz[1]},
			{0,h-sz[3]},{sz[4],h-sz[3]},{w-sz[2],h-sz[3]},	{w,h-sz[3]},
			{0,h},		{sz[4],h},		{w-sz[2],h},		{w,h}
		}
		for i,v in pairs(pos) do
			l2d:vertexPos(i-1,v[1],v[2],0)
		end
	end
	self.w,self.h = w,h
end

LuxModule.registerclassfunction("setSize",
	"():(SkinnedImage, int x,y,[z]) - moves the image(s) to the given coordinates.")
function SkinnedImage:setLocation(x,y,z)
	for i,l2d in pairs(self.l2d) do
		l2d:pos(x,y,z or 0)
	end
end

LuxModule.registerclassfunction("setScale",
	"():(SkinnedImage, int x,y,[z]) - applies a scaling to the image(s)")
function SkinnedImage:setScale(x,y,z)
	self.scale = {x,y}
	for i,l2d in pairs(self.l2d) do
		l2d:scale(x,y,z or 1)
	end
end

LuxModule.registerclassfunction("select",
	[[():(SkinnedImage, [int n]) - shows the n'ths image (images are enumerated
	from left to right from 1 to n). If n is not a valid number or is not passed,
	all images will be hid.]])
function SkinnedImage:select(n)
	for i,l2d in pairs(self.l2d) do
		l2d:rfNodraw(i~=n)
	end
end

LuxModule.registerclassfunction("delete",
	[[():(SkinnedImage) - deletes all l2dimages]])
function SkinnedImage:delete()
	for i,l2d in pairs(self.l2d) do
		l2d:delete()
	end
end
