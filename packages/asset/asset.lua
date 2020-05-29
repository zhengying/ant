local resource = import_package "ant.resource"
local datalist = require "datalist"

local assetmgr = {}
assetmgr.__index = assetmgr

local ext_bin = {
	fx      = true,
	texture = true,
	ozz     = true,
	meshbin = true,
}

local ext_tmp = {
	rendermesh 	= true,
	glbmesh   	= true,
	dynamicfx   = true,
}

local function split(str)
    local r = {}
    str:gsub('[^|]*', function (w) r[#r+1] = w end)
    return r
end

local glb = {}

function assetmgr.unload_glb(filename)
	local cr  = import_package "ant.compile_resource"
	local lst = glb[filename]
	if not lst then
		return
	end
	for _, f in ipairs(lst) do
		resource.unload(f)
		cr.clean(f)
	end
    cr.clean(filename)
	glb[filename] = nil
end

local function glb_load(path)
	local lst = split(path)
	if #lst <= 1 then
		return
	end
	local t = glb[lst[1]]
	if t then
		t[#t+1] = path
	else
		glb[lst[1]] = {path}
	end
end

local function glb_unload(path)
	local lst = split(path)
	if #lst <= 1 then
		return
	end
	local t = glb[lst[1]]
	if not t then
		return
	end
	for i, v in ipairs(t) do
		if v == path then
			table.remove(t, i)
			if #t == 0 then
				glb[lst[1]] = nil
			end
			break
		end
	end
end

local function resource_load(fullpath, resdata, lazyload)
	local filename = fullpath:match "[^:]+"
	resource.load(filename, resdata, lazyload)
    return resource.proxy(fullpath)
end

function assetmgr.load(key, resdata)
    return resource_load(key, resdata, false)
end

local function absolute_path(base, path)
	if path:sub(1,1) == "/" or not base then
		return path
	end
	return base .. (path:match "^%./(.+)$" or path)
end

function assetmgr.resource(world, path)
	local fullpath = absolute_path(world._current_path, path)
    return resource_load(fullpath, world, true)
end

--TODO
function assetmgr.load_fx(fullpath)
    return resource_load(fullpath, nil, true)
end

local function valid_component(w, name)
	local tc = w._class.component[name]
	return tc and tc.init
end

local function resource_init(w, name, filename)
	local cr = import_package "ant.compile_resource"
	local data = cr.read_file(filename)
	w._current_path = filename:match "^(.-)[^/|]*$"
	local res = datalist.parse(data, function(v)
		return w:component_init(v[1], v[2])
	end)
	w._current_path = nil
	if valid_component(w, name) then
		return w:component_init(name, res)
	end
	return res
end

local function resource_delete(w, name, v)
	w:component_delete(name, v)
end

function assetmgr.init()
	local function loader(filename, data)
		local ext = filename:match "[^.]*$"
		if ext_tmp[ext] then
			return require("ext_" .. ext).loader(data)
		end
		glb_load(filename)
		if ext_bin[ext] then
			return require("ext_" .. ext).loader(filename)
		end
		local world = data
		return resource_init(world, ext, filename)
	end
	local function unloader(filename, data, res)
		local ext = filename:match "[^.]*$"
		if ext_tmp[ext] then
			require("ext_" .. ext).unloader(res)
			return
		end
		glb_unload(filename)
		if ext_bin[ext] then
			require("ext_" .. ext).unloader(res)
			return
		end
		local world = data
		resource_delete(world, ext, res)
	end
	resource.register(loader, unloader)
end

assetmgr.patch = resource.patch

local resource_cache = {}
function assetmgr.generate_resource_name(restype, name)
	local key = ("//res.%s/%s"):format(restype, name)
	local idx = resource_cache[key]
	if idx == nil then
		resource_cache[key] = 0
		return key
	end

	idx = idx + 1
	resource_cache[key] = idx
	local n, ext = name:match "([^.]+)%.(.+)$"
	return ("//res.%s/%s%d.%s"):format(restype, n, idx, ext)
end

return assetmgr
