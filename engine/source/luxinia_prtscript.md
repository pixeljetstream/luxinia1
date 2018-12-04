# Particle Script .PRT

## Definition
A particlesystem script allows easier setup of particles and defines how they behave. Particles are always drawn last within a scene.

When a script is used, the emitter, or source partile for trails, will hand over following fixed attributes:
* position
* direction
Depending on this information the values defined in a script are relative to.

Emitter & Particle must be defined in a script, the rest is optional

* Variances are computed as followed:\
out = random between [in-variance,in+variance]
* Agefx are uint 0-99 = 0%-99% of lifetime\
Agetextures are not loaded into resource textures, but only for initialization. If you want to change the values afterwards, use api calls on the particlesystem

The header is necessary\
current layout version:\
luxinia_ParticleSys_v120

### Syntax
All commands are case sensitive and closed by `;` or "newline". A keyword and its arguments may not span multiple lines.\
`<id>` ranges from 0 to 7 \
`<vector4>` is (float,float,float,float) \
`<bool>` is 1 = true, 0 = false

### Branching
You can do branching with following commands. Enclose the branches in curly brackets `{ }`

* `IF:<condition string>`
* `ELSEIF:<condition string>`
* `ELSE`

The condition string can be set from luxinia API with `resource.condition`, or it may be part of a "define" in the Cg Compiler string.\
You can also negate a statement with !`<condition string>`.\
Following conditions are automatically set if applicable:
  * `CAP_POINTSPRITES` is automatically set when point sprites (ie textured POINTS) are available.

Be aware that the parser is not fully rock solid, so at best use
`COMMAND{<newline> <what><newline> }<newline>` , when problems occur.

### Annotations & Comments
You can add annotations anywhere in the file and later query them after load. 
* `<<_ "name";` serves as start  and is skipped by branching or comments.
* `_>>` denotes the end of the custom string.
Anything between the two will become part of the annotation, so use with caution.

``` cpp 
// comment until lineend

// multi-line annotation
<<_ "A"	
test = a * b
// this will be part of annotation as well
blahblubb
_>>

/* block commenting ...
<<_ "B"; min=25,max=90 _>>;	
//inlined annotation but ignored, due to active block comment
*/


```

C-style comments with `//` and `/*...*/` may not start within command & keywords. 
So start comments always after `;` in a line that contains command words.


----
## Example

``` cpp 
luxinia_ParticleSys_v110
RenderFlag{
	// this is a comment blah
	blendmode VID_ADD;
	sort;
}
Emitter{
	type VID_POINT;	
	size 10;
	spread (0,45);
	rate 170;
	velocity 30;
	count 100;
	maxoffsetdist 200;
	/*	blockcomment
		...
	*/
}
Particle{
	type VID_POINT;
	size	1;
	life	5000;
}
Texture{
	numtex 2;
	TEX 0	"test0.jpg";
	TEX 50	"test1.jpg";
}
Color{
	numcolor 1;
	RGBA	100 (1.0,0.3,0.3,0.5);
}
Forces{
	gravity 1000 (0.0,-10.0,0.0) 5;
}
SubSystem{
	trail "blubb.prt" 500 VID_DIR 40 100;
} 
```


----
## RenderFlag
they are applied to all particles and are optional.

* `nodraw;`\
this system wont be drawn but subsystems would. Useful if you want to hide the source system for trailsubsystems
* `lit;`\
particles are lit (default off)
* `depthmask;`\
will write into depthmask (ie. depth information stored, default off)
* `nodepthtest;`\
will do no depth testing (ie. always on top when drawn)
* `alphamask;`\
quick for: alphafunc GL_GREATER	(0.0)
* `sort;`\
will sort the particles by z-value\
if blendfunc is defined then back to front\
if alphafunc is set then front to back\
if both are set front to back is taken
* `novistest;`\
if set we will do no visibility testing but always update the emitters
* `alphafunc <enum func> (<float ref>);`\
to cap transparency of an object, same as in [[Api/Shader | ShaderScript]]
* `blendmode <enum blendmode>`;\
to make soft blends 
  * `blendmode`\
`VID_MODULATE` out = self * in\
`VID_MODULATE2` out = self * in * 2\
`VID_ADD` out = self + in\
`VID_AMODADD` out = self * self.alpha + in\
`VID_DECAL` out = self * self.alpha + in * (1-self.alpha)
* `nocull;`\
disables backface culling.
* `layer <int layer>;`\
in which l3dset layer (0-15) the system should be drawn.


----
## Emitter
How the particles are emitted is defined in here. What you define is just the default emitter. During runtime
when you create new emitters you can change their type and properties individually.

### Mandatory:
* `type <type enum>;`\
what the source of particles is
  * `VID_POINT`\
just a simple point\
startvelocity = emitterdirection * velocity
  * `VID_CIRCLE`\
a flat circle area with its normal in emitterdirection\
the size must be specified, particle will spawn somewhere within the circle\
startvelocity = emitterdirection * velocity
  * `VID_SPHERE`\
along the sphere surface, radius must be specified\
startvelocity = sphere's normal * velocity
  * `VID_RECTANGLE`\
along the axis aligned rectangle surface. Uses world axis for the plane. needs axis, width and height set\
startvelocity = emitterdirection * velocity
  * `VID_RECTANGLELOCAL`\
along the local aligned rectangle surface. Uses local X- and Z- axis of emitter for the rectangle plane. needs width and height set\
startvelocity = emitterdirection * velocity
  * `VID_MODEL`\
spawns on a random vertex of given model\
startvelocity = vertex normal * velocity			
* `count <num uint>;`\
total particles that can be displayed, you should somewhat find a good optimum how many you really need. Maximum is 8192
* `size <radius float>;`\
sphere or circle radius
* `width <radius float>;`\
width of VID_RECTANGLE
* `height <radius float>;`\
height of VID_RECTANGLE
* `axis <int 0-2>;`\
axis of the plane's normal for VID_RECTANGLE, 0 = X, 1 = Y, 2 = Z
* `model  "<modelfile string>";`\
modelfile of VID_MODEL
* `rate  <rate uint>;`\
how many particles per second one emitter spawns

### Optional
* `velocity <vel float>;`\
the starting velocity, units per second
* `endtime <time uint>;`\
time in ms after creation it will stop emission, if 0 then constantly spawned, or user controlled
* `maxoffsetdist <dist float>;`\
random offset from original spawnposition in velocity direction
* `velocityvar <var float>;`\
variance to velocity
* `flipdirection <percent float>;`\
percent of how many particles will use opposite direction
* `spread (<inner degrees uint>,<outer degrees uint>);`\
variance added to emitterdirection, maximum angle a random direction can have from original direction, particle direction will be between inner and outer angle.
* `restarttime <time uint>;`\
time in ms after endtime when particles are spawned again
* `restarts <num uint>;`\
how often restarts will happen 1-254, 255 = infinite


----
## Particle
This defines the appearance of the particle and certain variances to it.

### Mandatory
* `type <geo enum>;`\
this defines the particle appearance
  * `VID_POINT`\
a simple point, implementation may vary on gl_driver. When gl_driver allows point sprites a single texture can be used. Particlesize will be in pixels on screen at 640x480. Particles cannot have individual sizes.
  * `VID_QUAD`\
a flat quad facing towards camera, e.g part |  `>` eye. origin at center
  * `VID_TRIANGLE`\
a flat triangle facing towards camera
  * `VID_HSPHERE`\
a 5-sided hemisphere facing towards camera, e.g part ) `>` eye
  * `VID_SPHERE`\
a sphere (non textureable)
  * `VID_MODEL`\
a model, must be specified, particles will randomly pick a mesh inside the model and use its material, very expensive
* `model "<modelfile string>";`\
if VID_MODEL was picked as particle type, tell which model to use
* `life <time uint>;`\
time in ms until particle death
* `size <size float>;`\
size of particle (pixel,radius,width, model scale)

### Optional
* `instancemesh "<string meshname>";`\
if VID_MODEL is used, you can create instancing effects using a single mesh from the model.\
Only small meshes (quads/triangles,max 32 vertices and 96 indices) can be instanced. If successful,\
all particles will use the same mesh.\
The purpose of using a single instancemesh is speeding up rendering of lightweight meshes.
* `originoffset (<x float>,<y float>,<z float>);`\
if VID_QUAD,TRIANGLE,INSTANCEMESH,HSPHERE are used you can offset the drawing center by this vector
* `sizevar <percent float>;`\
size variance, minimum size is 1/10 of original
* `lifevar <percent float>;`\
variance in the lifetime 
* `rotate  <degrees uint> <dir enum>;`\
rotates particle around center. 
  * `degrees`\
degrees per second
  * `direction`\
`VID_POS, VID_NEG, VID_RAND`
* `rotatevar <percent float>;`\
variance to rotation degree
* `rotateoffset <degrees uint>;`\
angle on start is random(0 - degrees)
* `rotatevelocity;`\
if this flag is set rotation is linked to particle velocity
* `dieonfrontplane;`\
if flag is set particle will die when behind camera
* `camrotfix;`\
if set we will rotate particles based in which direction the camera looks. Useful for getting rid of billboard alignment in top down situation.\
overriden by rotatevelocity. Another solution is using Luxinia Api and make the particles globally aligned.
* `noagedeath;`\
if flag is set particle will not die if too old, but get age of 0 again
* `pointsmooth;`\
if flag is set particles will be rendered as antialiased round dots
* `pointparams (<float minsize>,<float maxsize>)(<float alphathresh>)(<float const>,<float lin>,<float square>);`\
for points special parameters can be set to control size by distance attenuation.\
outsize = size * 1/(const + lin*distance + square*distance*distance) 
  * `minsize,maxsize`\
outsize will be capped by these values 
  * `alphathresh` enables a fading out of color.alpha when size gets below thresh
  * `cons,lin,square`\
distance attenuation parameters

`Age-Based Effects`
* `sizeage3 <age1 uint> (<size1 float>) <age2 uint> (<size2 float>) <age3 uint> (<size3 float>);`\
can control the size changing over time, size will grow/shrink depending on the values\
there is 3 size groups, each size group contains
  * `age`\
time in percent of totallife
  * `size`\
size multiplier of the particle
* `speedage3 <age1 uint> (<velm1 float>) <age2 uint> (<velm2 float>) <age3 uint> (<velm3 float>);`\
scale velocity based on age		
* `rotateage3 <age1 uint> (<rotm1 float>) <age2 uint> (<rotm2 float>) <age3 uint> (<rotm3 float>);`\
can control the rotation changing over time. It works as multiplier of original rotation.
* `sizeagetex "<texturefile string>" <row uint> (<scale float>) <subtract float>;`\
can control the size changing over time, size will grow/shrink depending on the values, pixel 0 = start, 127 = end.
  * `texturefile`\
The texture must be 128 pixels wide, and ideally be a .tga file or other losless format.
  * `row`\
the row within the texture to use (you can combine multiple effects in one texture)
  * `scale`\
each value in the texture represents 0.0 - 1.0 (we take the R value in RGB/RGBA). you can scale this\
value with the given parameter. e.g. scale = 2 you will get 0.0 - 2.0.
  * `subtract`\
this optional value will be subtracted from texture*scale, that way you can create sign changes if needed;
* `rotateagetex "<texturefile string>" <row uint> (<scale float>) <subtract float>;`\
can control the rotation changing over time. It works as multiplier of original rotation.
* `speedagetex "<texturefile string>" <row uint> (<scale float>) <subtract float>;`\
scale velocity based on age


----
## Color
In here the color of the particles are set. If no color is set default will be white.

### Mandatory
* `numcolor <number uint>;`\
the number of colors a particle can have, must be specified first, minimum is 1
* `RGBA <age uint> (<R cfloat>,<G cfloat>,<B cfloat>,<A cfloat>);`\
  * `age`\
in percent of totallife, if just one color is given this is ignored **`R G B A`\
is color/alpha value
* `RGBAagetex "<texturefilename string>" <uint row>`;\
instead of numcolor & RGBA this single command can be used. The 128 pixel wide texture contains all color values, 0 = start, 127 = end. 
  * `row`\
the pixelrow to use within the texture;


### Optional
* `noage;`\
if flag is set colors will be picked randomly for a new particle
* `interpolate;`\
if flag is set then colors will be interpolated between ages
* `RGBAvar (<percentR float>,<percentG float>,<percentB float>,<percentA float>);`\
to add random variance to each color value, +/- SRC * var + SRC


----
## Texture
Same as color, if non is set we assume no texture given
textures cannot have texshaders applied, and must all be same size,same format.
Mind that depending on graphics card errors can come up with too many squence images.

### Mandatory:
* `numtex  <number uint>;`\
the number of textures, must be specified first
* `MATERIAL/TEX/TEXALPHA <age uint> "<texname string>";`
  * `age`\
in percent of totallife, if just one texture is given this is ignored
  * `texname`\
is the filename in a relative path
  * `TEX` RGB/RGBA
  * `TEXCOMBINE1D/2D_16/2D_64` RGB/RGBA\
we assume always the first specified texture is used, and has already horizontally(1d) or square(2d) combined textures in it.
  * `TEXALPHA` ALPHA
  * `MATERIAL`\
a single material is used for all textures, the textures used in the material are combined images by the user and must contain power of 2 subimages. using combine draw with materials is not allowed. All should be of same type (the last will be used for all)

### Optional
* `eventimed;`\
if flag was set before you start with the textures, they will all get same difference in time.\
when you use TEXCOMBINEx or MATERIAL you wont have to specify the other textures, ie a single entry will be enough
* `noage;`\
if flag is set random texture is picked for new particle
* `sequence;`\
if flag is set the defined textures are taken as a loopable sequence time delay of first is added to last\
e.g: 10 "a" 20 "b" 30 "c" `>`  10 "a" 20 "b" 30 "c" 40 "a" 50 "b" 60 "c" ... until 100
* `agetex "<texturefile string>" <row uint>;`\
which texture is used at what timestep is encoded in the texture, we use the pixel values and modulo them with numtex.
  * `row`
the pixelrow within the texture, in (RGB/RGBA textures R is used)


----
## Forces
Forces influence the particle motion, maximum is 32 forces per system.

There are more force types creatable by Luxinia Api such as
* `magnet`\
range based attraction/repulsion of particles. Uses quadratic distance falloff to target for effect strength.
* `target`\
similar to magnet, but without range falloff.

Standard forces:
* `gravity (<x float>,<y float>,<z float>) <turbulence float> [<uint starttime>] [<uint endtime>] ["<string name>"];`\
contains information of gravity, gravity is constant acceleration
  * `(x,y,z)`\
define direction and strength in world coordinates
  * `turbulence`\
is a wavey translation
* `wind (<x float>,<y float>,<z float>) <turbulence float> <airres float> [<uint starttime>] [<uint endtime>] ["<string name>"];`
  * `(x,y,z)`\
define direction and strength in world coords, x,y ground plane
  * `turbulence`\
is a wavey translation
  * `airres`\
air resistance a particle wind is either accelerating or slowing down depending on particle velocity same as above

### Optional
  * `starttime`\
the minimum age a particle must have to be affected
  * `endtime`\
the maximum age to be affected by force
  * `name`\
the name handle (less than 16 characters). Useful if you want to access the force from Luxinia api.


----
## SubSystem
With this other particle systems can be generated, maximum 8. The subsystems are drawn as listed after the original system, however if combinedraw flag is set they will get drawn with a single call. 

* `combinedraw;`\
a special flag that will put all particles into a single draw call. however all textures must be same type (RGB/RGBA) and only the renderflag of the original system is used\
Also it will use particletype of root system, if subsystems have subsystems themselves combinedraw will be ignored for all. 

* `trail "<psysname string>" <delay uint> <dir enum> <minage uint> <nth uint>;`\
particle will leave a trail of of the specified particlesys. Its Subsystems will not be used, when it is used as a trail.
  * `psysname`\
filename of subsystem
  * `delay`\
time in ms until subsys is launched
  * `dir`\
`VID_DIR` particle's direction\
`VID_ODIR` opposite of direction
  * `minage`\
minage of particle to leave a trail
  * `nth`\
every nth particle will create a trail

* `normal "<psysname string>" <delay uint>;`\
psys will be generated after a time given, direction & position is same as original
  * `psysname`\
filename of subsystem
  * `delay`\
time in ms until subsys is launched

* `translated "psys.prt" <delay uint> (<px float>,<py float>,<pz float>) (<dx float>,<dy float>,<dz float>);`\
similar to normal system but with specific position and direction, position is relative to original position
  * `delay`\
time in ms until subsys is launched
  * `(px,py,pz)`\
position vector
  * `(dx,dy,dz)`\
direction vector is multiplied with original direction, eg. if you want the same direction then (1,1,1) would be used, if opposite (-1,-1,-1)


----
## History

- **1st Draft 20.6.2004:**  \
This is just a basic idea about functionality. Depending on how hard it will be to do all, and how fast things can be done, the actual system may change. Also its not sure how the Trail system is done, it may done completely different, ie code uses point emitters from a different particle system.

- **2nd Draft 21.6.2004:**  \
Some more changes, mostly moving variables from code into the script to make them fixed. Also throwing the old trail out and instead added a subsystem of particles that will be loaded and controlled by this system.

- **3rd Draft 8.6.2004:**  \
Minor fiddling with the subsystems, mostly just changing minor stuff. It is yet unsure how force turbulences will look like. The other uncertainty is particles' directions

- **4th Draft 20.9.2004:**  \
subsystems can now be thrown into the same sorted drawlist using "combinedraw" flag

- **5th Draft 22.9.2004:**  \
added particle rotation

- **6th Draft 27.9.2004:**  \
added texture sequence

- **7th Draft 22.12.2004:**  \
vectors are packed in () and separated by ,

- **8th Draft 13.1.2005:**  \
added rotation age effect

- **9th Draft 28.1.2005:**  \
use of VID blendmodes

- **10th Draft 04.4.2005:**  \
added flipdirection percentage

- **11th Draft 11.6.2005:**  \
die on backplane

- **12th Draft 14.6.2005:**  \
added start offset, point sprites, point smoothing and pointparams

- **14th Draft 10.8.2005:**  \
added MATERIAL as texture type

- **15th Draft 6.9.2005:**  \
trails allow any emitter type now

- **16th Draft 12.11.2005:**  \
removed delay in forces, for simplicity / better parallelism

- **17th Draft 10.3.2006:**  \
removed magnets, those are handled by lua

- **18th Draft 13.3.2006:**  \
changed the way forces are dealt with, timed forces are back

- **19th Draft 4.6.2006:**  \
agebased effects can now be read from the texture

- **20th Draft 14.6.2006:**  \
added originoffset vector for billboards

- **21st Draft 20.6.2006:**  \
particle mesh instancing added

- **22nd Draft 10.8.2006:**  \
added "preprocessor" branching

- **23rd Draft 11.9.2007:**  \
added layer renderflag

- **24th Draft 24.1.2008:**  \
proper block commenting with # { ... # }

- **25th Draft 23.4.2008:**  \
commenting now in C-Style, along with raised version number
// and /* ... */

- **26th Draft 18.5.2008:**  \
annotation system added

- **27th Draft 20.7.2008:**  \
CAP_POINTSPRITES added

