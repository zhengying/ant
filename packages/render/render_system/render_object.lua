local ecs = ...
local serialize = import_package "ant.serialize"

local rendercore = ecs.clibs "render.core"
local math3d = require "math3d"

local iro = ecs.interface "irender_object"
function iro.mesh(p)
end

local ro = ecs.component "render_object"
local function init_ro()
    return {
        worldmat = math3d.NULL,
        prog        = 0xffffffff,
        mesh        = 0,
        depth       = 0,
        discardflags= 0xff,
        materials   = rendercore.queue_materials(),
    }
end

function ro.init(r)
    assert(not r)
    return init_ro()
end

function ro.remove(r)
    r.materials = nil
end

local ra = ecs.component "render_args2"
function ra.init(v)
    v.visible_id = 0
    v.cull_id = 0
    v.viewid = 0
    v.material_idx = 0
end