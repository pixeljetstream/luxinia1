LuxModule.register("gui",
	[[
	The GUI module handles all kinds of graphical user interface representations
	and actions.

	If you want to write your own GUI classes based on the Component class,
	you need to call the supermethods of the classes. I.e. if you overload
	the mousePressed method of the button, you need to call the mousePressed
	method of the button within you method:

	 function MyButtonClass:mousePressed(event)
	   Button.mousePressed(self,event)
	   -- do your stuff here now
	 end

	If you forget to do this, your class will not work as expected.
	Super-constructors should be called too, ie

	 function MyButtonClass.new (class, x,y,w,h, ...)
	   local self = Button.new(class, x,y,w,h)
	   -- do your stuff now
	 end
	]])


dofile("mouse.lua")
dofile("Rectangle.lua")
dofile("icon.lua")
dofile("imageicon.lua")
dofile("l3dicon.lua")
dofile("l2dicon.lua")
dofile("skin2d.lua")
dofile("stretchedimageskin.lua")
dofile "tooltipevent.lua"
dofile("Component.lua")
dofile("Container.lua")
	dofile("components/ContainerResizer.lua")
	dofile("components/ContainerMover.lua")
	dofile("components/Button.lua")
	dofile("components/TextField.lua")
	dofile("components/Label.lua")
	dofile("components/GroupFrame.lua")
	dofile("components/titleframe.lua")
	dofile("components/ListBox.lua")
	dofile("components/ComboBox.lua")
	dofile("components/ScrollBar.lua")
--	dofile("components/TextArea.lua")
	dofile("components/ImageComponent.lua")
	dofile("components/MultiLineLabel.lua")
	dofile("components/Slider.lua")
	dofile("components/Checkbox.lua")
	dofile("components/buttongroup.lua")
	dofile("components/TreeView.lua")
	
dofile("guihelpers.lua")
