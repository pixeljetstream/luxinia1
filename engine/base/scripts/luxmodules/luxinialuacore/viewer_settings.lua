local refs = {}
local fns = {}
for i=1,4 do refs[i] = {} end

refs[4][1] = 0.11627907305956
refs[4][2] = 0.11153298616409
refs[4][3] = 0.105204872787
refs[4][4] = 0

refs[3][1] = "bsp"
refs[3][2] = "ms3d"
refs[3][3] = "3ds"
refs[3][4] = "md3"
refs[3][5] = "obj"
refs[3][6] = "b3d"
refs[3][7] = "fbx"
refs[3][8] = "dae"

refs[2]["frametime"] = 33.33
refs[2]["cache16"] = false
refs[2]["bincos"] = 0.1
refs[2]["swaxis"] = 1
refs[2]["swrevert"] = 0
refs[2]["stripify"] = false
refs[2]["optimize"] = false
refs[2]["tancos"] = 0.1
refs[2]["nthframe"] = 1
refs[2]["tangents"] = false
refs[2]["swsign"] = 1
refs[2]["nof3d"] = false
refs[2]["convextensions"] = refs[3]
refs[2]["fixcyl"] = false
refs[2]["swflip"] = 0
refs[2]["noma"] = false
refs[2]["keepsky"] = false

refs[1]["luxf3d"] = refs[2]
refs[1]["bgcolor"] = refs[4]
refs[1]["controlspeed"] = 0.33732292451054

return refs[1]
