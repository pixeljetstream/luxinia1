
AutoDoc = {}
LuxModule.registerclass("luxinialuacore","AutoDoc",
[[
The AutoDoc system creates documentation files from the information
provided from Luxinia and the modulekernel. The AutoDoc class provides
a system that can be extended to create help files that fits your use.

For creating your own documentation, you have to set up a template table.
This table may contain string, tables and functions.

]],AutoDoc,{})

do
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

	LuxModule.registerclassfunction("templateFunctions",
		"[table] - contains a number of default functions that can be used within template pages.")
	AutoDoc.templateFunctions = {}
	LuxModule.registerclassfunction("templateFunctions.hierarchylist",
[[
(function):(string format, string sublistfmt, [boolean api]) - returns a
function that returns a string of a hierarchical listing of all class in a list
form. If api is true, the apihierarchy is used, if its false or not used,
the modulehierarchy is used.

The passed string is used for each element in the list, the sublistfmt is
used for childs in the hierarchy.

* $name is replaced by the name of the element or (in the sublistfmt) with the
childlist of the element.
* $ifinterface=xyz$; will show xyz if the class is an interface class
* $list will insert the sublist in the sublistfmt
]])
	function AutoDoc.templateFunctions.hierarchylist(listfmt,sublistfmt,api)
		--[=[
			listfmt = "$name $ifinterface=...$;"
			sublistfmt = "$list $ifmodule=...$;"
		]=]
		local function createlist(self,apihierarchy,modulehierarchy)
			local function enlist (tab)
				local list = AutoTable.new()
				for i,v in UtilFunctions.pairsByKeys(tab) do
					--if (v.isInterface) then
					local str = string.gsub(listfmt,"$ifinterface=(.-)$;",v.isInterface and "%1" or "")

					list((string.gsub(str,"$name",i)))
					--end

					if (v.childs) then
						list((string.gsub(sublistfmt,"$list",enlist(v.childs))))
					end
				end
				return table.concat(list)
			end

			local sublistfmtnomod = string.gsub(sublistfmt,"$ifmodule=.-$;","")
			local sublistfmtorig = sublistfmt
			sublistfmt = sublistfmtnomod

			if (api) then return enlist(apihierarchy) end

			local list = AutoTable.new()
			local modfmt = string.gsub(listfmt,"$ifinterface=(.-)$;","")
			--modfmt = string.gsub(modfmt,"$ifmodule=(.-)$;","%1")

			for a,b in pairs(modulehierarchy) do
				list((string.gsub(modfmt,"$name",a)))
				list((string.gsub(sublistfmt,"$list",enlist(b))))
			end
			return table.concat(list)
		end
		return createlist
	end

	LuxModule.registerclassfunction("templateFunctions.descriptor",
[[
(function):(string classfmt, interfacefmt,childfmt,functionfmt,modulefmt) -
returns a function that can be used in the template system.

* classfmt: the strings $classname,$classparent, $classdescription, $interfacelist, $childlist,
$functionlist in the classfmt string will be replaced by the actual meaning
* interfacefmt: $interfacename
* childfmt: function (class, depth): function that should return a string
* functionfmt: function (fname,description,cleandesc)
]])
	function AutoDoc.templateFunctions.descriptor (classfmt,interfacefmt,childfmt,functionfmt,modulefmt)
		--[=[
			modulefmt = "$modulename ... $classlist ... $moduledescription"
			classfmt = "$classname ... $classdescription ... $interfacelist ... $childlist ... $functionlist"
			interfacefmt = "$interfacename"
			childfmt = function (class,depth)
			functionfmt = function (fnname,description,cleandesc)
		]=]
		local function createlist(self,apihierarchy,modulehierarchy)
			local function enlist (tab)
				local list = AutoTable.new()
				for i,v in UtilFunctions.pairsByKeys(tab) do
					local iflist = AutoTable.new()
					local fnlist = AutoTable.new()

					if (interfacefmt) then
						if (v.interfaces and table.getn(v.interfaces)>0) then
							for index,name in pairs(v.interfaces) do
								iflist((string.gsub(interfacefmt,"$interfacename",name)))
							end
						end
					end
					if (functionfmt) then
						for name,desc in pairs(v.functions or {}) do
							local argument,cleandesc,match = string.gfind(desc,"(.+)%-(.+)$")()
							local ret,args = string.gfind(argument or "","(%(.*%)):(%(.*%))")()

							fnlist(functionfmt(name,desc,cleandesc or desc,ret,args))
						end
					end
					local replace = {
						classparent = v.parent and v.parent.name or "",
						classname = i,
						classdescription = v.description,
						interfacelist = table.concat(iflist),
						functionlist = table.concat(fnlist),
						childlist = childlist and table.concat(childlist) or ""
					}
					local str = string.gsub(classfmt, "$(%w+)",function (name)
						--print("rep",name,"->",replace[name])
						return replace[name] or name
					end)
					list(str)
					--print(table.concat(list))

					if (v.childs) then
						list(enlist(v.childs))
					end
				end
				return table.concat(list)
			end
			if (not modulefmt) then return enlist(apihierarchy) end
			local list = AutoTable.new()

			for a,b in pairs(modulehierarchy) do
				local replace = { modulename = a, classlist = enlist(b), moduledescription = LuxModule.getLuxModuleDescription(a) }

				list((string.gsub(modulefmt,"$(%w+)",function (name) return replace[name] or name end)))
			end
			return table.concat(list)
		end
		return createlist
	end

	LuxModule.registerclassfunction("templatePages",
		"[table] - contains a number of default templates for creating documentation pages")
	AutoDoc.templatePages = {}

--------------------------------------------------------------------------------
-- api Doc index - a collection of all info in a simple single file
	LuxModule.registerclassfunction("templates.templatePages.apiDocIndex",
		"[table] - API documentation in a single file.")

	AutoDoc.templatePages.apiDocIndex = {
		"%file=index.txt%",
		AutoDoc.templateFunctions.descriptor (
			[[
{$classparent:$classname}
$classdescription
@$interfacelist@

$functionlist
--------
]],
			"$interfacename, ",nil,
			function (name, desc)
				return "%"..name.."%\n"..desc.."\n--------\n"
			end,
			[[
::$modulename::
$moduledescription
--------
$classlist
--------
]]
		),
		[[::Luxinia Core Classes::
--------
]],
		AutoDoc.templateFunctions.descriptor (
			[[
{$classparent:$classname}
$classdescription
@$interfacelist@

$functionlist
--------
]],
			"$interfacename, ",nil,
			function (name, desc)
				return "%"..name.."%\n"..desc.."\n--------\n"
			end
		)
	}


AutoDoc.templatePages.wxideapi = {
	"%file=luxiniaapi.lua%",
	function (self,apihierarchy,modulehierarchy,classes)
		local out = {}
		local function writeclass (name,tab)
			local funcs = {}
			for i,v in pairs(tab.functions) do
				local ret,arg,desc = v:match("(%b())%s*:%s*(%b())%s*%-(.+)")
				if not ret then 
					funcs[#funcs+1] = ("[%q] = {type='value', description = %q}"):format(i,v)
				else
					funcs[#funcs+1] = ("[%q] = {type='function', description = %q, args=%q, returns=%q}"):
						format(i,desc,arg,ret)
				end
			end
			out[#out+1] = ("%s = {type='class',description=%q,childs={%s}}\n"):format(name,tab.description,table.concat(funcs,",\n"))
			--if tab.childs then 
			--	for n,c in pairs(tab.childs) do	
			--		writeclass(n,c)
			--	end
			--end
		end
		for key,value in pairs(classes) do
			writeclass(key,value)
			--writeclass(key,value)
		end
		return table.concat(out)
	end
}

--------------------------------------------------------------------------------
-- api frame html doc template
	LuxModule.registerclassfunction("templates.templatePages.apiHTMLFrameDoc",
		"[table] - Documentation of api and module pages as frames.")
	local htmlheader = [[<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN"
		"http://www.w3.org/TR/html4/frameset.dtd"><html><head>]]
	AutoDoc.templatePages.apiHTMLFrameDoc = {
		"%file=index.html%",htmlheader,
[[
<title>Luxinia API Documentation</title>
</head>
<frameset cols="250,*">
  <frame name="navigation" src="navigation.html">
  <frame name="browse" src="docinfo.html">
  <noframes>
    Your browser doesn't support frames.
  </noframes>
</frameset>
</html>
]],
		"%file=navigation.html%",htmlheader,
[[
<title>Luxinia API Documentation</title>
<link rel='stylesheet' type='text/css' href='overview.css'>
</head>
<body>
<h1><a href='docinfo.html' target='browse'>Luxinia Documentation</a></h1>
<h2>LuxModules</h2>
]],
		"<ul class='modlist'>",
		AutoDoc.templateFunctions.hierarchylist(
				"<li>$ifinterface=<i>$;<a href='$name.html' target='browse'>"..
				"$name</a>$ifinterface=</i>$;</li>","<ul>$list</ul>"
			),
		"</ul>",
		"<h2>Core API</h2>",
		"<ul>",
		AutoDoc.templateFunctions.hierarchylist(
				"<li>$ifinterface=<i>$;<a href='$name.html' target='browse'>"..
				"$name</a>$ifinterface=</i>$;</li>","<ul>$list</ul>",true
			),
		"</ul>",

[[
</body>
</html>
]],
		"%file=docinfo.html%",htmlheader,
[[
<title>Luxinia API Documentation</title>
<link rel='stylesheet' type='text/css' href='classview.css'>
</head>
<body>
<h1>Luxinia API Documentation</h1>
]],
"<h5>",({system.version()})[1]," - ",({system.version()})[2],"</h5>",
[[
<p>
	This is the Luxinia API documentation. This documentation is
	automaticly created by the AutoDoc class of the luxinialuacore module. You
	can create custom templates that fits your use. You can customize the .css
	files apart from the lua files - these are not automaticly created and will
	not be overwritten if the help files is recreated.<br/>
	It is necessary to update the documenation if you download a developer build
	or if no documentation is available. Please make sure that you always use
	the documentation of the Luxinia executable that you are using.
</p>
<h2>Reading the documentation</h2>
<p>
	Lua does not have strict typing of variables, and it does not know
	float, integer, short or char values as C does. If such types are
	required by a function, you can pass any numbervalue. Use <b>tonumber</b>
	if your given variable is not a number but a string for example.
	You don't have to round your numbers if a int is required - this is
	done automaticly.<br/>
	<b>Overview on numbertypes</b>
	<ul>
		<li> Lua number: double value 64bit floating number
		<li> float: Any number value, 32bit floating number (used in Luxinia)
		<li> integer (int):  rounded number between 2147483648 and -2147483647
		<li> short: rounded number between 32768 and -32767
		<li> char: rounded number between 128 and -127 or 0 and 255 - this
			depends if the function uses a signed or unsigned number.
	</ul>
	Other values cannot be casted.
</p>
<p>
	The arguments of functions and their return values are described in this
	way by example:
	<pre>afunctionname (int x, y, [float z,int/string bla,float])</pre>
	Optional arguments are enclosed in cornered brackets.
	Types of arguments are optional and not always given. Type descriptions
	are sometimes only given for the first element of this type and trailing
	arguments are assumed to be of the same type. The parameter name
	can also be just the type of the argument. If different types can
	be passed, a slash is used to tell all the different types.
</p>
<p>
	The return values are described in the same way as the arguments.
	In most cases, more than one variable is returned.
</p>
<h2>Naming conventions</h2>
<p>
	The Luxinia naming convention divides between module classes and core classes.
	This is quite usefull since modules and core classes are different in its
	purpose.
</p>
<p>
	Core classes are provided by the luxinia core itself. This means that
	each function is implemented in C and should be considered to be "fast".
	The core function handle all problems that are computational expensive,
	such as the rendering and physics calculations.
</p>
<p>
	LuxModules provide interface functions and classes to the core classes and
	additional utility functions that are written in Lua. This is useful for
	most projects.
</p>
<h2>Modifying the documentation's appearance</h2>
<p>
	You can modify the documentation in several ways. In any case you
	need to understand how HTML and CSS works.
	<ul>
		<li>Edit the css files in doc/frames of the luxinia path<br/>
		The CSS files are not created automaticly and can be changed by you
		using the CSS commands.
		<li>Create new <a href='AutoDoc.html'>AutoDoc</a> templates<br/>
		The documentation is created automaticly by the
		<a href='AutoDoc.html'>AutoDoc</a> class and you can create your own
		templates that do so, too. Read the documentation of the
		<a href='AutoDoc.html'>AutoDoc</a> class and learn by reading the
		given examples in the base/scripts directory of Luxinia.
		<li>Do not edit the HTML files. <br/>
		The HTML files are created automaticly which means that all your changes
		will be overwritten if you edit them manually. You can not change the
		descriptions of the classes itself.
	</ul>
</p>
</body>
</html>
]],
		function (self,apihierarchy,modulehierarchy,classes)
			local list = AutoTable.new()
			local highlights = {
				boolean = true,
				--string = true,
				int = true,
				char = true,
				float = true,
				--table = true,
				short = true,
				String = true,
				["function"] = true,
			}

			local luahighlight = {
				{"%-%-[^\n]+", "comment"},
				{"\".-[^\\]\"", "codestr"},
				{"function", "keyword1"},
				{"local", "keyword2"},
				{"end", "keyword2"},
				{"for", "keyword2"},
				{"do", "keyword2"},
				{"while", "keyword2"},
				{"until", "keyword2"},
				{"if", "keyword2"},
				{"in", "keyword2"},
				{"then", "keyword2"},
				{"else", "keyword2"},
				{"elseif", "keyword2"},
				{"repeat", "keyword2"},
				{"setmetatable", "keyword3"},
				{"getmetatable", "keyword3"},
				{"print", "keyword3"},
				{"true", "keyword4"},
				{"false", "keyword4"},

			}
			local function count (tab) local i=0
				for x,v in pairs(tab) do i = i+1 end return i end

			local function highlight (str,notkey1,notclass)
				-- leading tabs are filtered
				str = string.gsub(str,"\n\t+","\n")
				-- replace html characters with their HTML counterparts
				str = str:gsub("&","&amp;"):gsub("<","&lt;"):gsub(">","&gt;")
				-- a leading space will start a <pre></pre> tagged sequence
				str = str.."\n."
				str = string.gsub(str,"\n( .-)(\n%S)",
					function (tx,suffix)
						local keep = {}
						for i,tab in ipairs(luahighlight) do
							local a,b = unpack(tab)
							a = "(%W)("..a..")(%W)"
							--if (b=="comment" and tx:match("btn")) then
							--	print(a)
							--	print(tx)
							--end

							tx =
								string.gsub(tx,a,
								function (pre,str,suf)
									table.insert(keep,"<span class='"..b.."'>"..str.."</span>")
									return pre..string.char(254)..#keep..string.char(254)..suf
								end)
							--if (b=="comment" and tx:match("btn")) then print("\n") print(tx) end
						end
						tx = tx:gsub(string.char(254).."(%d+)"..string.char(254),function (n) return keep[tonumber(n)] end)
						return "<pre>"..tx.."</pre>"..suffix
					end
				)
				str = string.gsub(str,"@@(.-)@@","<code>%1</code>")
				str = string.gsub(str,"%[img ([^ %]]+)([^%]]*)%]","<img src='../images/%1' %2/>")

				str = string.gsub(str,"\n(!+)(.-)\n", function (hcount, title)
						local tag = "h"..string.len(hcount)
						return "<"..tag.." class='desctitle'>"..title.."</"..tag..">\n"
					end)

				str = string.sub(str,0,-2)
				-- make a list if a line starts with a *
				local rows = {}
				str = string.gsub(str,"[^\n]+",function (line)
					local stars,line = line:match("^([%*#]*)(.*)$")
					if #stars>0 then line = "<li>"..line  end
					while #rows<#stars do
						local ul =stars:sub(#rows,#rows)=='*'
						local t = ul and '<ul>' or '<ol>'
						line = (ul and '<ul>' or '<ol>')..line
						table.insert(rows,ul and '</ul>' or '</ol>')
					end
					while #rows>#stars do line = table.remove(rows)..line end
					return line
				end)
				while #rows>0 do str = str..table.remove(rows) end

				-- a double newline will result in a linebreak
				str = string.gsub(str,"\n\n+","\n\n\n")
				str = string.gsub(str,"\n*(.-)\n\n","<p>%1</p>")
				str = string.gsub(str,"&lt;br&gt;","<br/>")
				str = string.gsub(str,"&lt;br/&gt;","<br/>")

				--str = string.gsub(str,"\n"," ")


				return string.gsub(str,"(%w+)", function (word)
						if (highlights[word] and not notkey1) then
							return "<span class='keyword1'>"..word.."</span>"
						end
						-- find out if the word is a registered class. Maybe the
						-- word is written in plural, so try it again with a letter less.
						local sword = classes[string.sub(word,0,-2)] and string.sub(word,0,-2) or word
						if (classes[sword] and word~=notclass) then
							return "<a class='keyword2' href='"..sword..
								".html'>"..word.."</a>"
						end
						return word
					end)
			end


			local function enlist (tab,module,classcnt,fncnt)
				classcnt = classcnt or 0
				fncnt = fncnt or 0
				for i,class in pairs(tab) do
					classcnt = classcnt + 1
					local name = class.name
					list(string.format("%%file=%s.html%%",name))
					list(htmlheader,"<title>Luxinia API Documentation</title>")
					list("<link rel='stylesheet' type='text/css' href='classview.css'>")
					list("</head><body>")
					list("<h1>Class: ",name,"</h1>")
					if (module) then
						list("<h3 class='modtitle'>LuxModule: <a href='",module,".html'>",module,"</a></h3>")
					end
					list("<p>",highlight(class.description,true,name),"</p>")
					if (class.parent or class.childs) then
						list("<h2>Hierarchy</h2>")
						list("<pre>")
						local browse = class
						local browsedup = {}
						while (browse) do
							table.insert(browsedup,browse)
							browse = browse.parent
						end
						local indent = ""
						for i=table.getn(browsedup),1,-1 do
							list(indent,"o-+ <a href='",browsedup[i].name,".html'>",
								browsedup[i].name,"</a>\n")
							indent = indent .. "  "
						end

						local function rec (class,indent)
							for i,v,index,left in UtilFunctions.pairsByKeys(class.childs or {}) do
								list(indent,v.childs and "o-+ " or "o-- ",
									"<a href='",v.name,".html'>",v.name,"</a>\n")
								if (v.childs) then rec(v,indent..(left==nil and "  " or "| ")) end
							end
						end

						rec(class,indent)

						list("</pre>")
					end
					if (class.interfaces and table.getn(class.interfaces)>0) then
						list("<h2>Interfaces: </h2>")
						list("<ul>")
						table.sort(class.interfaces)
						for i,v in pairs(class.interfaces) do
							list("<li><a href='",v,".html'>",v,"</a>")
						end
						list("</ul>")
					end
					if (class.implementors) then
						list("<h2>Classes that implement this interface:</h2>")
						list("<ul>")
						table.sort(class.implementors, function (a,b) return a.name<b.name end)
						for indexx,iclass in ipairs(class.implementors) do
							list("<li><a href='",iclass.name,".html'>",iclass.name,"</a>")
						end
						list("</ul>")
					end
					if (class.functions and count(class.functions)>0) then
						list("<h2>Methods: </h2>")
						if (count(class.functions)>3) then
							list("<h3>Method overview:</h3>")
							list("<ul>")
							for i,v in UtilFunctions.pairsByKeys(class.functions) do
								list(string.format("<li><a href='#%s'>%s</a></li>\n",i,i))
							end
							list("</ul>")
							list("<hr>")
						end

						list("<dl>")
						local function printfun (i,v)
							local ret,arg,desc = string.gfind(v,"(%(.-%))%s*:%s*(%(.-%))%s*%-(.*)$")()
							local fnname = i
							if (i=="new") then fnname = "<span class='constructor'>"..i.."</span>" end
							if (arg) then
								--print(i,class.safefunctions[i])
								list("<dt><a name='",i,"'><b>",fnname,"</b></a> ",highlight(arg),
									"</dt><dd><i>returns:</i> ",highlight(ret),
									"<div class='fndescriptor'>",
									highlight(desc,true,name),"</div></dd>")
							else
								list("<dt><a name='"..i.."'><b>",fnname,"</b></a></dt><dd>",highlight(v),"</dd>")
							end
						end
						if (class.functions.new) then
							printfun("new",class.functions.new)
							fncnt = fncnt + 1
						end
						for i,v in UtilFunctions.pairsByKeys(class.functions) do
							if (i~="new") then
								if (v==nil) then
									_print(class.name,":",i)
								end
								printfun(i,v)
							end
							fncnt = fncnt + 1
						end
						list("</dl>")
					end
					if (class.parent) then
						list ("<h2>Inherited Methods: </h2>")
						local funlist = class.functions
						local function reclist (class)
							if (class.functions and count(class.functions)>0) then
								list("<h3 class='inheritedfunctions'>From <a href='",
									class.name,".html'>",class.name,"</a></h3>")
								list("<div class='inheritedfunctions'>")
								local tab = {}
								for i,v in UtilFunctions.pairsByKeys(class.functions) do
									local cl = fpubclass.class(class.name)
									if not cl or cl:inherited(i) then
										local mark = funlist[i] and " class='overloaded'" or ""
										table.insert(tab,string.format(
											"<a href='%s.html#%s'%s>%s</a>",class.name,i,mark,i))
									end
								end
								list(table.concat(tab,", "))
								list("</div>")
							end
							if (class.parent) then reclist(class.parent) end
						end
						reclist(class.parent)
					end
					list("</body></html>")
					classcnt,fncnt = enlist(class.childs or {},module,classcnt,fncnt)
				end
				return classcnt,fncnt
			end

			local classcnt,fncnt = 0,0
			for i,v in pairs(modulehierarchy) do
				list(string.format("%%file=%s.html%%",i))
				list(htmlheader)
				list("<link rel='stylesheet' type='text/css' href='classview.css'>")
				list("<title>Luxinia API Documentation</title></head><body>")
				list("<h1>LuxModule: ",i,"</h1>")
				list("<p>",highlight(LuxModule.getLuxModuleDescription(i)),"</p>")
				list("<h2>Class overview</h2>")
				local function enlistclasses (v)
					list("<ul>")
					for j,w in pairs(v) do
						list("<li>",highlight(j),"</li>")
						if (w.childs) then enlistclasses(w.childs) end
					end
					list("</ul>")
				end

				--classcnt,f = (c + classcnt, f + fcnt
				list("</body></html>")

				enlist(v,i)
			end
			--print("Classes/Functions in modules: ",classcnt,fcnt)

			print("Classes/Functions in core:    ",enlist(apihierarchy))

			return list
		end
	}

--------------------------------------------------------------------------------
-- api html page template
	LuxModule.registerclassfunction("templates.apiSingleHtmlDoc",
		"[table] - Template table for creating the HTML API of all native "..
		"Luxinia classes.")
	AutoDoc.templatePages.apiSingleHtmlDoc= {
		"<html><body>",
		"<h1>Luxinia API Documentation</h1>",
		"<h2>",({system.version()})[1]," - ",({system.version()})[2],"</h2>",
		"<ul>",
		AutoDoc.templateFunctions.hierarchylist(
				"<li><a href='#$name'>$name</a></li>","<ul>$list</ul>",true
			),
		"</ul>",
		AutoDoc.templateFunctions.descriptor (
			[[
				<div>
					<h1 class='class'><a name='$classname'>$classname</a></h1>
					<p>$classdescription</p>
					<p>$interfacelist</p>
					<div><ul>$functionlist</ul></div>
				</div>
			]],
			"$interfacename, ",nil,
			function (name, desc)
				return "<li>"..name.."<br/>"..desc.."</li>"
			end
		),
		"</body></html>"
	}

--------------------------------------------------------------------------------
-- module html page template
	LuxModule.registerclassfunction("templatePages.moduleHtmlDoc",
		"[table] - Template table for creating the HTML API of the modules.")
	AutoDoc.templatePages.moduleSingleHtmlDoc= {
		[[
		<html>
		<style>
			div.module {
				border:1px solid black;
				padding:1em;
				margin-bottom:1em;
			}
			h1.module {
				margin:0px;
			}
			h1,h2,h3 {
				border-bottom:1px solid black
			}
			h1,h2,h3,h4,h5,h6 {
				margin:0px
			}
			body {font-family:palatino linotype}
			</style>
		<body>
		<h1>Luxinia LuxModule API Documentation</h1>
		]],
		"<h4>",({system.version()})[1]," - ",({system.version()})[2],"</h4>",
		"<h2>Overview</h2>",
		"<ul>",
			AutoDoc.templateFunctions.hierarchylist(
				"<li><a href='#$name'>$name</a></li>","<ul>$list</ul>"
			),
		"</ul>",
		AutoDoc.templateFunctions.descriptor (
			[[
				<div>
					<h1 class='class'><a name='$classname'>$classname</a></h1>
					<p>$classdescription</p>
					<p>$interfacelist</p>
					<div><ul>$functionlist</ul></div>
				</div>
			]],
			"$interfacename, ",nil,
			function (name, desc,cleandesc,rets,args)
				assert(name and cleandesc, "name: "..tostring(name).." cleandesc:"..tostring(cleandesc))
				return table.concat(
					{"<li>",name,"<br/>",
					(args and "arguments: " or ""),args or "",args and "<br/>" or "",
					rets and "return: " or "",rets or "",rets and "<br/>" or "",

					cleandesc,"</li>"})
			end,
			[[
				<div class='module'>
					<h1 class='module'>$modulename</h1>
					<p>$moduledescription</p>
					<div>$classlist</div>
				</div>
			]]
		),
		"</body></html>"

	}

	LuxModule.registerclassfunction("createDoc",
[[
(string doc, table struct):(table template) -
Creates a string from a table that was passed that acts
as a template. The table may contain functions, tables and strings.

The table is converted
to a string that represents the final document. The table is therefore iterated
from 1 to n, where n is the number of number-indexed values in the table.
That means that you can use any kind of keys for storing additional information
that you need during the process. In each iteration,

* Strings are written to the output directly
* Table values are passed as argument to a recursive function call on createDoc
* Functions will be called with the table itself, the api hierarchy, the list of
modules and a table containing all classes with their names as keys. The
function may return itself a table (which is resolved as in the point above) or
a string.

The template may contain strings that are formated like "%file=abc%".
The string "abc" is then used as a key in the table that is additionally
returned. Per default, all output is saved to the key "main" in the table, as
long as no such instruction is used. You can use the created table with all
the outputstrings to write it to a file or another structure.
]])
	function AutoDoc.createDoc (template)
		local modules = LuxModule.modulenames()
		local api,classes = getclasshierarchy()
		local nm = {}
		local files = {}
		local file = "main"
		for a,b in pairs(modules) do
			local modclasses
			nm[b],modclasses = LuxModule.getclasshierarchy(b)
			for i,v in pairs(modclasses) do
				classes[i] = v
			end
		end
		modules = nm
		local function rec (template)
			local out = {}
			local tab = {}
			for i,v in ipairs(template) do
				files[file] = files[file] or {}
				if (type(v)=="string") then
					if (string.find(v,"^%%file=.*%%$")) then
						file = string.gfind(v,"%%file=(.*)%%")()
					else
						table.insert(files[file],v)
						table.insert(out,v)
					end
				elseif (type(v)=="function") then
					local ret = v(self,api,modules,classes)
					if (type(ret)=="table") then
						--table.insert(files[file],
						table.insert(out,(rec(ret)))
					elseif(type(ret)=="string") then
						table.insert(files[file],ret)
						table.insert(out,ret)
					end
				elseif (type(v)=="table") then
					table.insert(out,(rec(v)))
				end
			end
			return table.concat(out),tab
		end
		return rec(template),files
	end

		LuxModule.registerclassfunction("writeDoc",
[[
():(path, table template) - similiar to createDoc, except that the output
is written to files in a given output directory.
]])

	function AutoDoc.writeDoc (path,template)
		local _,docs = AutoDoc.createDoc(template)

		for i,v in pairs(docs) do
			if (table.getn(v)>0) then
				local f = io.open(path..i,"w")
				f:write(table.concat(v))
				f:close()
			end
		end
	end
end

