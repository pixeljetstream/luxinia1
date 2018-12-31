
function ARCreateStaticVBOs(vtype,vertcount,indexcount,doallocs)

	local vsize = vtype:memsize()*vertcount
	local vbo = vidbuffer.new(vidtype.vertex(),vidhint.draw(1),
								vsize)

	
	-- indices are either 16bit or 32bit depending on vertexcount
	local is16bit = vertcount <= 65536
	local isize = (is16bit and 2 or 4)*indexcount
	local ibo = vidbuffer.new(vidtype.index(),vidhint.draw(1),
								isize)
	
		
	if (doallocs) then
		ibo:localalloc(isize)
		vbo:localalloc(vsize)
	end
	
	return vbo,ibo,is16bit
end

