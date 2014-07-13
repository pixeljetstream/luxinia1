do
	window.resizemode(2)
	require "lfs"
	local realprint = print
	local path = lfs.currentdir() -- stupid wxlua print replace with messagebox output
	lfs.chdir(path.."/base/lib/wx")
	wx = require "wx"
	lfs.chdir(path)
	print = realprint
	
	dofile ("base/lib/wx/wxluxfrontend.lua")
	
	wxframe = wx.wxFrame(
				wx.NULL,
			    wx.wxID_ANY,
			    "wx.GLCanvas test",
			    wx.wxDefaultPosition,
			    wx.wxSize(640, 480))
	
	mainwindow = {}
	local self = mainwindow
	self.this = wxframe
	local this = self.this
	
	self.m_mgr = wxaui.wxAuiManager()
    self.m_mgr:SetManagedWindow(this);

    this:CreateStatusBar();
    this:GetStatusBar():SetStatusText("Ready");
    
	-- create a simple file menu
	local fileMenu = wx.wxMenu()
	fileMenu:Append(wx.wxID_EXIT, "E&xit", "Quit the program")
	-- create a simple help menu
	local helpMenu = wx.wxMenu()
	helpMenu:Append(wx.wxID_ABOUT, "&About","About")
	
	-- create a menu bar and append the file and help menus
	local menuBar = wx.wxMenuBar()
	menuBar:Append(fileMenu, "&File")
	menuBar:Append(helpMenu, "&Help")
	-- attach the menu bar into the frame
	wxframe:SetMenuBar(menuBar)

	                
	glcanvas = wx.wxGLCanvas(
					wxframe,
					wx.wxID_ANY,
					wx.wxDefaultPosition,
					wx.wxDefaultSize,
					wx.wxSUNKEN_BORDER)
    self.m_mgr:AddPane(glcanvas, wxaui.wxAuiPaneInfo():
                  Name("test1"):Caption("Luxinia Renderer"):
                  Center():
                  CloseButton(false):MaximizeButton(true));
	wxframe:Show(true)
	
	this:SetMinSize(wx.wxSize(800,600));
	
    html = wx.wxLuaHtmlWindow(this, wx.wxID_ANY,
                               wx.wxDefaultPosition,
                               wx.wxSize(400,300));
	local function getHTMLhelp(link)
		local page = {
			'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd"><html>',
			'<head><style type="text/css">\n'..
			'h1 {font-size:12pt;border-width: 1; border: solid; text-align: center}\n'..
			'</style></head>',
			"<body>"
		}
		local function out (...)
			for i=1,select("#",...) do
				table.insert(page,tostring(select(i,...)))
			end
		end
		
		local function getclasstable()
			local tab = {}
			for i=1,fpubclass.class() do
				local class = fpubclass.class(i)
				if (class) then
					local fns = {}
					local cnt = 0
					local p = class:parent()
					for k=0,class:functioninfo()-1 do
						local name,descript = class:functioninfo(k)
						fns[name] = descript
						cnt = cnt + 1
					end
					tab[fpubclass.class(i):name()] = {
						parentclassname = (p and p:name() or ""),
						description = fpubclass.class(i):description(),
						functions = fns,
						functioncount = cnt,
						classid = i,
					}
				end
			end
			return tab
		end
	
		-- generates a graph of the classes provided by luxinia
		local function getclasshierarchy()
			local hierarchy = {}
			local tab = getclasstable()
			local childs = {}
			local classcount,functioncount = 0,0
	
			for i,j in pairs(tab) do
				local cl = fpubclass.class(i)
				local parent = cl:parent()
				--print(parent)
				local function interfaces()
					local tab = {}
					for i=0,cl:interface()-1 do --parent and (parent:interface()-1) or -1 do
						--print (parent:name(),"implements",parent:interface(i):name())
	
						tab[i+1] = cl:interface(i):name()
					end
					return tab
				end
				local interfaces = interfaces()
				parent = parent and parent:name() or nil
				tab[i] = {
					name = i, description = j.description,
					childs = nil, parent = nil,
					functions = j.functions, interfaces = interfaces,
					safefunctions = j.safefunctions,
					allclasses = tab
				}
				if (parent and parent~=i) then
					childs[parent] = childs[parent] or {}
					childs[parent][i]=tab[i]
				end
				classcount = classcount + 1
				functioncount = functioncount + j.functioncount
			end
	
			for name,class in pairs(tab) do
				for i,interface in pairs(class.interfaces or {}) do
					tab[interface].isInterface = true
					tab[interface].implementors = tab[interface].implementors or {}
					table.insert(tab[interface].implementors,class)
				end
			end
	
	
			--for i,v in pairs(tab) do print(i,v) end
			for i,j in pairs(childs) do
				if (tab[i]) then
					tab[i].childs = j
					for a,b in pairs(j) do
						b.parent = tab[i]
	
					end
				end
			end
	
			for i,j in pairs(tab) do
				if (not j.parent) then hierarchy[i] = j end
			end
			return hierarchy,tab
		end
	
		-- create a table with all classes from a hierarchy structure
		local function hierarchy2classes (hierarchy)
			local classes = {}
			local function fill (level)
				for a,b in pairs(level) do
					classes[string.lower(a)] = true
					fill(b.childs or {})
				end
			end
			fill (hierarchy)
			--for a,b in classes do _print(a) end
			return classes
		end

		
		local tab,list = getclasshierarchy()
			
		if link == "main" then
			out "<h1>Main Index</h1>"
			local function printlist (tab,ind)
				for i,k in pairs(tab) do
					out (ind.."-<a href='class:"..k.name.."'>"..k.name.."</a><br>")
					if k.childs then 
						printlist(k.childs,ind.."&nbsp;") 
					end
				end
			end
			printlist(tab,"")
		end
		local class = link:match("class:(.*)")
		if class and list[class] then
			out ("<h1>",class,"</h1>")
			local i = list[class]
			out (i.description)
			out ("<hr>")
			out "<ul>"
			for f,info in pairs(i.functions) do
				out("<li><b>"..f.."</b><br>"..info)
			end
			out "</ul>"
		end
		out "<hr><a href='main'>main</a></body></html>"
		return table.concat(page)
	end
	
	function html:OnLinkClicked(link)
		html:SetPage(getHTMLhelp(tostring(link:GetHref())))
    end
	html:SetPage (getHTMLhelp "main")
	self.m_mgr:AddPane(html, wxaui.wxAuiPaneInfo():
                  Caption("HTML Help"):Left():CloseButton(false):MaximizeButton(true):MinimizeButton(true));
	
                  
                  
	self.m_mgr:GetArtProvider():SetMetric(wxaui.wxAUI_DOCKART_GRADIENT_TYPE, wxaui.wxAUI_GRADIENT_VERTICAL);
	self.m_mgr:Update();

	-- connect the selection event of the exit menu item to an
	-- event handler that closes the window
	wxframe:Connect(wx.wxID_EXIT, wx.wxEVT_COMMAND_MENU_SELECTED,
			function(event)
				system.exit(true)
				wxframe:Close(true)
			end)
	
	wxframe:Connect(wx.wxEVT_CLOSE_WINDOW, 
			function(event)
				system.exit(true)
			end)
	
	-- connect the selection event of the about menu item
	wxframe:Connect(wx.wxID_ABOUT, wx.wxEVT_COMMAND_MENU_SELECTED,
			function (event)
				wx.wxMessageBox('This is the "About" dialog.',
				"About",
				wx.wxOK + wx.wxICON_INFORMATION,wxframe)
			end)

	wxluxfrontend.connectinput(glcanvas)

	wxframe:Connect(wx.wxEVT_IDLE,
		function(event)
			--glcanvas:SetCurrent(glcanvas:GetContext())
			wx.wxSafeYield() -- kills some gui stuff
			luxfrontend.lrunframe()
		 end
	)
         
	wxframe:Connect(wx.wxEVT_TIMER,
		function(event)
			--glcanvas:SetCurrent(glcanvas:GetContext())
			luxfrontend.lrunframe()
		end
	)
	wxtimer = wx.wxTimer(wxframe)
	wxtimer:Start(1)
	
	
	local function wxresize(w,h)
		glcanvas:SetClientSize(w,h)
		wxframe:Fit()
	end
            
            
    ------------
    -- Main
    luxfrontend.setPostInit(
		function()
			if (wxluxfrontend.size) then
				wxresize(unpack(wxluxfrontend.size))
			end
			wx.wxGetApp():MainLoop()
		end
	)
            
            
    ------------
    -- Windowing
            
    luxfrontend.setOpenWindow(
    	function (width, height, redbits, 
			greenbits, bluebits, alphabits, 
			depthbits, stencilbits, mode)
			print ">>>>>>>>>>>>>>>>>>>>>open window"
			
			if (wxluxfrontend.size) then
				wxresize(width, height)
			else
				wxluxfrontend.size = {width,height}
			end
			
			glcanvas:SetCurrent(glcanvas:GetContext())
			
			return true
		end
	)
	luxfrontend.setOpenWindowHint( 
		function (target, hint)
			-- target
			-- LUXI_REFRESH_RATE, LUXI_STEREO, LUXI_WINDOW_NO_RESIZE,
			-- LUXI_FSAA_SAMPLES
		end
	)
	luxfrontend.setCloseWindow( 
		function ()
			-- ignore only has relevance for fullscreen stuff
		end
	)
	
	luxfrontend.setSetWindowTitle( 
		function (str)
			wxframe:SetTitle(str)
		end
	)
	
	luxfrontend.setGetWindowSize( 
		function ()
			local gls = glcanvas:GetClientSize()
			return gls:GetWidth(),gls:GetHeight()
		end
	)
	luxfrontend.setSetWindowSize( 
		function (w,h)
			if (not wxluxfrontend.resizer) then
				wxresize(w,h)
			end
		end
	)
	
	luxfrontend.setGetWindowPos(
		function ()
			local p = wxframe:ClientToScreen(wx.wxPoint(0,0))
			return p:GetXY()
		end
	)
	
	luxfrontend.setSetWindowPos(
		function (x,y)
			wxframe:SetSize(x,y,wx.wxDefaultCoord,wx.wxDefaultCoord,wx.wxSIZE_USE_EXISTING)
		end
	)
	
	luxfrontend.setSwapBuffers( 
		function ()
			glcanvas:SwapBuffers()
		end
	)
	
	luxfrontend.setGetWindowParam( 
		function (param)
			-- LUXI_OPENED,LUXI_ACTIVE,LUXI_FSAA_SAMPLES
			if (param == luxfrontend.LUXI_OPENED) then
				return (not wxframe:IsIconized())
			elseif( param == luxfrontend.LUXI_ACTIVE) then
				return wxframe:IsActive()
			elseif( param == luxfrontend.LUXI_FSAA_SAMPLES) then
				return 0
			end
			
			return true
		end
	)
	
	luxfrontend.setPrint(
		function (str)
			print("LXI:> ",str)
		end
	)
end