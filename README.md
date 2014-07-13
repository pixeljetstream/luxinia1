luxinia
=======

Source code for http://www.luxinia.de game engine.

The project is not in active development anymore, this repository exists for archive purposes or if people want to branch from it.

Building and structure
----------------------

Cross-platform libraries are used, however only MSVC2008 solutions and Windows 32-bit have been used for development.

Requires to be merged with http://www.luxinia.de/download/luxinia1support.7z which contains third-party code and dependencies. Pre-built for Windows 32-bit.

The SDK consist of multiple parts. 
If not mentioned otherwise, the bottom license applies (NET-BSD)

### backend

Released under MIT license. Newer version might be found at https://github.com/pixeljetstream/luxinia2

* luxplatform:
Core lib for file operations and compiler specific defines/asserts

* luxmath:
Most vector math related functions are stored here. 

* luxcore: Some generic data containers and algorithms are defined here. 
The object reference system (smart/weak pointers) is also heavily
used by luxinia.


### engine
The final runtime package (binaries) has its own license. The
source of the engine uses either NETBSD or MIT style licenses.

* base:
The standard lua code that ships with any runtime distribution.
Its source code is released under MIT license.

* include & source:
Actual engine source code.


### frontends

* luxinia_glfw:
Standard exe that provides a window manager to luxinia. Luxinia's
window manager is more or less identical to GLFW.

* luxinia_lua:
Allows you to define your own window manager through lua functions.
GLFW is used for joystick input. Was used to run luxinia
inside wxWidgets.

### dependencies
### plugins
### thirdpartylibs
are found pre-built inside http://www.luxinia.de/download/luxinia1support.7z

Engine architecture
-------------------

A brief overview on the internal components, best combined with the documentation found at http://luxinia.de/doc/frames/

![arch overview](https://github.com/pixeljetstream/luxinia1/raw/master/engine/source/luxinia_architecture.png)

Luxinia 1 Engine SDK License
----------------------------
Copyright: 2004-2011 Christoph Kubisch and Eike Decker. 
All rights reserved. 
http://www.luxinia.de

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this 
  list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this 
  list of conditions and the following disclaimer in the documentation and/or 
  other materials provided with the distribution.
* Neither the name of LUXINIA nor the names of its contributors may
  be used to endorse or promote products derived from this software without 
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE.

