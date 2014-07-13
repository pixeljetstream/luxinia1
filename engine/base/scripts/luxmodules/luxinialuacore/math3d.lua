ExtMath = {
	tmpmatrix = matrix4x4.new()
}
LuxModule.registerclass("luxinialuacore","ExtMath",
		[[The Extended Math class contains some generic math operations. Vectors and 
		matrices are passed as indexed tables from 1-3 or 1-16. Matrices are stored column-major. 
		Compared to the Luxinia CoreApi functions, the operations here are written and processed entirely in Lua]],ExtMath,{})

LuxModule.registerclassfunction("v3min",
	"(table vector):(table a, b) - returns minimum vector a and b")
function ExtMath.v3min (a,b)
	return {math.min(a[1],b[1]),math.min(a[2],b[2]),math.min(a[3],b[3])}
end

LuxModule.registerclassfunction("v3max",
	"(table vector):(table a, b) - returns maximum vector a and b")
function ExtMath.v3max (a,b)
	return {math.max(a[1],b[1]),math.max(a[2],b[2]),math.max(a[3],b[3])}
end

LuxModule.registerclassfunction("v3cross",
	"(table vector):(table a, b) - returns cross product of 3d vector a and b")
function ExtMath.v3cross (a,b)
	return {a[2]*b[3]-a[3]*b[2],a[3]*b[1]-a[1]*b[3],a[1]*b[2]-a[2]*b[1]}
end

LuxModule.registerclassfunction("v3dot",
	[[(float dot):(table v1, v2) - returns dotproduct for 2 3vectors]])
function ExtMath.v3dot (a,b)
	return (a[1]*b[1]+a[2]*b[2]+a[3]*b[3])
end

LuxModule.registerclassfunction("v3sqlen",
	[[(float len):(table vec) - square distance. This function is a little bit 
	faster than the len function which returns the squareroot of this function.
	If you only want to compare distances (i.e. is this point inside my distance),
	you can square the distance you would like to compare with. This is worth 
	the effort since it does not make much difference in code and work.]])
function ExtMath.v3sqlen (a)
	return (a[1]*a[1]+a[2]*a[2]+a[3]*a[3])
end

LuxModule.registerclassfunction("v3len",
	[[(float len):(table vec) - euklid length of vecotr (a²+b²+c²)^0.5]])
function ExtMath.v3len (a)
	return math.sqrt(a[1]*a[1]+a[2]*a[2]+a[3]*a[3])
end

LuxModule.registerclassfunction("v3normalize",
	[[(table normalized,float origlength):(table vec) - returns normalized 
	vector 3d vector and the original length of the vector]])
function ExtMath.v3normalize (a)
	local d = math.sqrt(a[1]*a[1]+a[2]*a[2]+a[3]*a[3])
	if (d == 0) then return {0,0,0},0 end
	local o,d = d,1/d
	return {a[1]*d,a[2]*d,a[3]*d},o
end

LuxModule.registerclassfunction("v3mul",
	"(table out):(table a,b) - returns componentwise multiplication a * b")
function ExtMath.v3mul (a,b)
	return {a[1]*b[1],a[2]*b[2],a[3]*b[3]}
end

LuxModule.registerclassfunction("v4mul",
	"(table out):(table a,b) - returns componentwise multiplication a * b")
function ExtMath.v4mul (a,b)
	return {a[1]*b[1],a[2]*b[2],a[3]*b[3],a[4]*b[4]}
end

LuxModule.registerclassfunction("v3div",
	"(table out):(table a,b) - returns componentwise multiplication a / b")
function ExtMath.v3div (a,b)
	return {a[1]/b[1],a[2]/b[2],a[3]/b[3]}
end

LuxModule.registerclassfunction("v4div",
	"(table out):(table a,b) - returns componentwise multiplication a / b")
function ExtMath.v4div (a,b)
	return {a[1]/b[1],a[2]/b[2],a[3]/b[3],a[4]/b[4]}
end


LuxModule.registerclassfunction("v3sub",
	"(table difference):(table a,b) - returns subraction a - b")
function ExtMath.v3sub (a,b)
	return {a[1]-b[1],a[2]-b[2],a[3]-b[3]}
end

LuxModule.registerclassfunction("v4sub",
	"(table difference):(table a,b) - returns subraction a - b")
function ExtMath.v4sub (a,b)
	return {a[1]-b[1],a[2]-b[2],a[3]-b[3],a[4]-b[4]}
end

LuxModule.registerclassfunction("v3scale",
	"(table out):(table a,float f) - returns component wise a * f")
function ExtMath.v3scale (a,f)
	return {a[1]*f,a[2]*f,a[3]*f}
end

LuxModule.registerclassfunction("v4scale",
	"(table out):(table a,float f) - returns component wise a * f")
function ExtMath.v4scale (a,f)
	return {a[1]*f,a[2]*f,a[3]*f,a[4]*f}
end

LuxModule.registerclassfunction("v3scaledadd",
	"(table out):(table a,table b,float f) - returns a + (b * f)")
function ExtMath.v3scaledadd (a,b,f)
	return {a[1]+b[1]*f,a[2]+b[2]*f,a[3]+b[3]*f}
end

LuxModule.registerclassfunction("v4scaledadd",
	"(table out):(table a,table b, float f) - returns a + (b * f)")
function ExtMath.v4scaledadd (a,b,f)
	return {a[1]+b[1]*f,a[2]+b[2]*f,a[3]+b[3]*f,a[4]+b[4]*f}
end

LuxModule.registerclassfunction("v3sqdist",
	"(float sqdist):(table v1,v2) - return square distance |(v1-v2)|² of vectors")
function ExtMath.v3sqdist (a,b)
	local a = {a[1]-b[1],a[2]-b[2],a[3]-b[3]}
	return (a[1]*a[1]+a[2]*a[2]+a[3]*a[3])
end

LuxModule.registerclassfunction("v3dist",
	"(float sqdist):(table v1,v2) - return distance |(v1-v2)| of vectors")
function ExtMath.v3dist (a,b)
local a = {a[1]-b[1],a[2]-b[2],a[3]-b[3]}
	return math.sqrt(a[1]*a[1]+a[2]*a[2]+a[3]*a[3])
end

LuxModule.registerclassfunction("v3add",
	"(table sum):(table a,b) - returns addition a + b")
function ExtMath.v3add (a,b)
	return {a[1]+b[1],a[2]+b[2],a[3]+b[3]}
end

LuxModule.registerclassfunction("v3axisrotate",
	"(table point):(table axis,number angle, table vec) - rotates vector vec around angle of the given axis")
function ExtMath.v3axisrotate(axis, angle, vec)
	ExtMath.tmpmatrix:setaxisangle(axis[1],axis[2],axis[3],angle)
	return {ExtMath.tmpmatrix:transform(unpack(vec))}
end

LuxModule.registerclassfunction("v4add",
	"(table sum):(table a,b) - returns addition a + b")
function ExtMath.v4add (a,b)
	return {a[1]+b[1],a[2]+b[2],a[3]+b[3],a[4]+b[4]}
end


LuxModule.registerclassfunction("v3interpolate",
	"(table):(float fraction,table from, table to) - returns interpolated vector between from and to")
function ExtMath.v3interpolate (t,a,b)
	return {ExtMath.interpolate(t,a[1],b[1]),ExtMath.interpolate(t,a[2],b[2]),ExtMath.interpolate(t,a[3],b[3])}
end

LuxModule.registerclassfunction("v4interpolate",
	"(table):(float fraction,table from, table to) - returns interpolated vector between from and to")
function ExtMath.v4interpolate (t,a,b)
	return {ExtMath.interpolate(t,a[1],b[1]),ExtMath.interpolate(t,a[2],b[2]),ExtMath.interpolate(t,a[3],b[3]),ExtMath.interpolate(t,a[4],b[4])}
end

LuxModule.registerclassfunction("v3spline",
	"(table):(float fraction, table prevfrom, table from, table to, table nextto) - returns catmull-rom spline interpolated vector between from and to")
function ExtMath.v3spline (t,p,a,b,n)
	return {ExtMath.spline(t,p[1],a[1],b[1],n[1]),ExtMath.spline(t,p[2],a[2],b[2],n[2]),ExtMath.spline(t,p[3],a[3],b[3],n[3])}
end

LuxModule.registerclassfunction("v4spline",
	"(table):(float fraction, table prevfrom, table from, table to, table nextto) - returns catmull-rom spline interpolated vector between from and to")
function ExtMath.v4spline (t,p,a,b,n)
	return {ExtMath.spline(t,p[1],a[1],b[1],n[1]),ExtMath.spline(t,p[2],a[2],b[2],n[2]),ExtMath.spline(t,p[3],a[3],b[3],n[3]),ExtMath.spline(t,p[4],a[4],b[4],n[4])}
end

LuxModule.registerclassfunction("interpolate",
	"(float):(float fraction,float from, float to) - returns interpolated value between from and to")
function ExtMath.interpolate (t,a,b)
	return (a + ((b-a)*t))
end

LuxModule.registerclassfunction("spline",
	"(float):(float fraction,float prevfrom, float from, float to, float nextto) - returns catmull-rom spline interpolated value between from and to")
function ExtMath.spline (t,P0,P1,P2,P3)
	return 0.5 *( (2 * P1) + (-P0 + P2) * t + (2*P0 - 5*P1 + 4*P2 - P3) * t*t + (-P0 + 3*P1- 3*P2 + P3) * t*t*t)
end

LuxModule.registerclassfunction("v3polar",
	"(floar x,y,z):(float len,planeangle,heightangle) - converts polar coords to regular x,y,z")
function ExtMath.v3polar(l,p,h)
	local x,y,z
	z = math.cos(h)
	x = math.cos(p)*math.sin(h)
	y = math.sin(p)*math.sin(h)
	return x*l,y*l,z*l
end

LuxModule.registerclassfunction("anglediff",
	"(angle):(radians1, radians2) - calculates the shortest angledifference between two angles (which is a bit tricky i.e. -Math.PI - Math.PI == 0)")
function ExtMath.anglediff(a,b)
	a,b = a%(math.pi*2),b%(math.pi*2)
	local d = a - b
	if d>math.pi then d = d - math.pi*2 end
	if d<-math.pi then d = d + math.pi*2 end
	return d
end

LuxModule.registerclassfunction("nextpowerof2",
	"(number):(number) - calculates the next positive power of 2 number")
function ExtMath.nextpowerof2(a)
	local t = 1
	if (a <= t) then return t end
	while (t < a) do
		t = t * 2
	end
	return t
end


LuxModule.registerclassfunction("nearestpowerof2",
	"(number):(number) - calculates the nearest positive power of 2 number")
function ExtMath.nearestpowerof2(size)
	local power = 2

	if (size <= 1) then
		return 1
	end

	while (true) do
		-- we found the proper one
		if (size == power) then
			return power
		end
		if (size > power and size < (power * 2)) then
			-- find towards which it is closer
			if (size >= ((power+(power*2))/2)) then
				return power*2
			else
				return power
			end
		end
		power = power * 2
	end
end

LuxModule.registerclassfunction("quatslerp",
	[[(tabel qout):(number fracc, table q1, q2) - returns quaternion spherical interoplation]])
function ExtMath.quatslerp (f,a,b)
	local x,y,z,w = unpack(a)
	
	return {mathlib.quatslerp(f,x,y,z,w,unpack(b))}
end

LuxModule.registerclassfunction("quatslerpq",
	[[(tabel qout):(number fracc, table qprev, q0, q1, qnext) - returns quaternion spherical quadratic interoplation]])
function ExtMath.quatslerpq (f,qa,qb,qc,qd)
	local x,y,z,w = unpack(qa)
	local q,w,e,r = unpack(qb)
	local a,s,d,f = unpack(qc)
	
	return {mathlib.quatslerpq(f,x,y,z,w,q,w,e,r,a,s,d,f,unpack(qd))}
end

LuxModule.registerclassfunction("quatslerpqt",
	[[(tabel qout):(number fracc, table qprev, q0, q1, qnext) - returns quaternion spherical quadratic tangent interoplation]])
function ExtMath.quatslerpqt (f,qa,qb,qc,qd)
	local x,y,z,w = unpack(qa)
	local q,w,e,r = unpack(qb)
	local a,s,d,f = unpack(qc)
	
	return {mathlib.quatslerpqt(f,x,y,z,w,q,w,e,r,a,s,d,f,unpack(qd))}
end