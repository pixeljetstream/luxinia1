LuxModule.register("lux_ext",
[[The LuaExt module contains lua code and descriptions that are from mainly 
from other lua projects. We always refer to the original sources and we 
want to point out here that most stuff in this module ist not written by us.

We include this here for completeness of the documentation inside Lua. 

We only list here software and documentations from projects that are published 
as GPL or MIT license. In case that you disagree with this, please tell us. 

!!Lua (from http://www.lua.org):
The standard Lua libraries provide useful functions that are implemented directly 
through the C API. Some of these functions provide essential services to the 
language (e.g., type and getmetatable); others provide access to "outside" services 
(e.g., I/O); and others could be implemented in Lua itself, but are quite 
useful or have critical performance requirements that deserve an implementation 
in C (e.g., sort).

All libraries are implemented through the official C API and are provided as 
separate C modules. Currently, Lua has the following standard libraries:

* basic library;
* package library;
* string manipulation;
* table manipulation;
* mathematical functions (sin, log, etc.);
* input and output;
* operating system facilities;
* debug facilities. 

Except for the basic and package libraries, each library provides all its 
functions as fields of a global table or as methods of its objects.

!!Sockets (from http://www.cs.princeton.edu/~diego/professional/luasocket/)
LuaSocket is a Lua extension library that is composed by two parts: a C core 
that provides support for the TCP and UDP transport layers, and a set of Lua 
modules that add support for functionality commonly needed by applications that 
deal with the Internet.

The core support has been implemented so that it is both efficient and simple to 
use. It is available to any Lua application once it has been properly 
initialized by the interpreter in use. The code has been tested and runs well 
on several Windows and Unix platforms.

Among the support modules, the most commonly used implement the SMTP 
(sending e-mails), HTTP (WWW access) and FTP (uploading and downloading 
files) client protocols. These provide a very natural and generic interface 
to the functionality defined by each protocol. In addition, you will find 
that the MIME (common encodings), URL (anything you could possible want to do 
with one) and LTN12 (filters, sinks, sources and pumps) modules can be 
very handy.

The library is available under the same terms and conditions as the Lua 
language, the MIT license. The idea is that if you can use Lua in a project, 
you should also be able to use LuaSocket.

Copyright © 2004-2005 Diego Nehab. All rights reserved.
Author: Diego Nehab 

!!BitOp (from http://bitop.luajit.org/)
Lua BitOp is a C extension module for Lua 5.1 which adds bitwise operations on numbers.

Lua BitOp is Copyright © 2008-2009 Mike Pall. Lua BitOp is free software, 
released under the MIT/X license (same license as the Lua core). 

]])

dofile("lua.lua")
dofile("net.lua")
dofile("bit.lua")