-- html doc
function HLconverthtmlpage (page)
	page = page:match("<body.->(.*)</body.->")
	page = page:gsub("<br/*>\n","<br/>")
	page = page:gsub("\n"," ")
	repeat
		local n
		page,n = page:gsub("%s%s"," ")
	until n == 0
	page = page:gsub("&nbsp;"," ")
	page = page:gsub("<br/*>","\n")
	page = page:gsub("<a.-href=(.-)>(.-)</a>",function (ln,tx) return "$$link"..ln:gsub("'",""):gsub('"','').."$$\v009"..tx.."$$link$$\v000" end)
	return page:gsub("^%s*(.*)%s*$","%1") -- trim the string
end

function HLgetdocpage (file,title)
	local path = system.projectpath().."tutscripts/"..file
	local fp = io.open(path)
	local tx = (fp:read("*a") or "")
	fp:close()
	
	local t = tx:match("<title>(.+)</title>") or file
	
	if title then title:setText(" "..t) end

	return HLconverthtmlpage(tx),t
end

function HLaction (title,actions)
	
	return function (self,what,mouseevent,zone)
		if zone and what == "clicked" then
			local fn = zone.description:match("function:(.+)")
			if fn then return actions[fn]() end
			self:setText(HLgetdocpage(zone.description,title))
			
			self.prevzone = nil
			self.prevzonetext = nil
			return
		end
		if zone~=self.prevzone and self.prevzone then
			self.prevzone.l2d:text(self.prevzonetext)
			self.prevzone = nil
		end
		if zone and self.prevzone~=zone then
			if prevzone then
				prevzone.l2d:text(self.prevzonetext)
			end
			self.prevzone = zone
			local text = zone.l2d:text()
			self.prevzonetext = text
			text = text:gsub("\v990","\v009")
			local front,link,back = text:sub(0,zone.strstart),
				text:sub(zone.strstart+1,zone.strend),text:sub(zone.strend+1)
			--print(link)
			link = link:gsub("\v009","\v900")
			text = front..link..back
			zone.l2d:text(text)
		end
	end

end