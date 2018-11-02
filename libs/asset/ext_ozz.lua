local vfs = require "vfs"

return function(filename)
	local realfilename = vfs.realpath(filename)

	local function find_tagop(readops)
		local f = io.open(realfilename, "rb")
		
		--print(endless)
		for tag, op in pairs(readops) do
			f:seek("set")
			-- luacheck: ignore endless
			local endless = f:read(1)
			local c = f:read(#tag)
			if c == tag then
				f:close()
				return op
			end
		end
		
		f:close()		
	end

	local readops = {
		["ozz-animation"] = function ()
			local animodule = require "hierarchy.animation"
			return animodule.new_ani(realfilename)
		end,
		["ozz-skeleton"] = function()
			local hiemodule = require "hierarchy"
			return hiemodule.build(realfilename)
		end,
		["ozz-sample-Mesh"] = function()
			local animodule = require "hierarchy.animation"
			return animodule.new_ozzmesh(realfilename)
		end,
	}

	local readop = find_tagop(readops)
	if readop then
		return {
			handle = readop()
		}
	end
	error("not support type")
	return nil
end