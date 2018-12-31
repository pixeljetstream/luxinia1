-- TODO OpenCL binding

if (true) then
	return NOTREADY()
end

-- first load opencl binding
require "luaopencl"
-- and some helper functions dealing with
-- swig-generated bindings
require "swigutils"
sw = swigutils

cl = luaopencl
cl.utilInitExtensions()

function cl.utilGetDevice()
	local ids = cl.cl_platform_idArray(8)
	local err,idcnt = cl.clGetPlatformIDs(8,ids:cast())

	if (err ~= 0 or idcnt < 1) then return end

	local buf = sw.buffer:new(1024,"binary")

	local device = nil
	local units = 0
	for i=1,idcnt do
		local pf = ids[i-1]
		local err = cl.clGetPlatformInfo(pf,
			cl.CL_PLATFORM_PROFILE,
			buf:storage())

		local devids = cl.cl_device_idArray(16)

		buf:astype("string")
		if (err == 0 and buf:data() == "FULL_PROFILE") then
			-- iterate devices
			local err,dcnt = cl.clGetDeviceIDs(pf,cl.CL_DEVICE_TYPE_GPU,16,devids:cast())

			buf:astype("uint")
			for n=1,dcnt do
				local dev = devids[n-1]
				cl.clGetDeviceInfo(dev,cl.CL_DEVICE_GLOBAL_MEM_SIZE,buf:storage())
				print ("CL_DEVICE_GLOBAL_MEM_SIZE",buf:at(0))
				cl.clGetDeviceInfo(dev,cl.CL_DEVICE_MAX_COMPUTE_UNITS,buf:storage())
				print ("CL_DEVICE_MAX_COMPUTE_UNITS",buf:at(0))
				if (buf:at(0) > units) then
					units = buf:at(0)
					device = dev
				end
			end

		end

	end

	return device,units
end

function cl.utilGetDevices(cxGPUContext)

end

function cl.utilGetMaxFlopsDev(cxGPUContext)
	--[[
    -- get the list of GPU devices associated with context
    cl.clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, 0, NULL, &szParmDataBytes);
    cdDevices = (cl_device_id*) malloc(szParmDataBytes);
    size_t device_count = szParmDataBytes / sizeof(cl_device_id);

    clGetContextInfo(cxGPUContext, CL_CONTEXT_DEVICES, szParmDataBytes, cdDevices, NULL);

    cl_device_id max_flops_device = cdDevices[0];
    int max_flops = 0;
    
    size_t current_device = 0;
    
    -- CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint compute_units;
    clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);

    -- CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint clock_frequency;
    clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);
    
    max_flops = compute_units * clock_frequency;
    ++current_device;

    while( current_device < device_count )
    {
        // CL_DEVICE_MAX_COMPUTE_UNITS
        cl_uint compute_units;
        clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);

        // CL_DEVICE_MAX_CLOCK_FREQUENCY
        cl_uint clock_frequency;
        clGetDeviceInfo(cdDevices[current_device], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);
        
        int flops = compute_units * clock_frequency;
        if( flops > max_flops )
        {
            max_flops        = flops;
            max_flops_device = cdDevices[current_device];
        }
        ++current_device;
    }

    free(cdDevices);

    return max_flops_device;
	]]
end

--dev = cl.utilGetDevice()

ctx = cl.clCreateContextFromType(cl.utilGetCLContextGLProperties(), cl.CL_DEVICE_TYPE_GPU, nil,nil);



