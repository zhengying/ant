local ecs = ...
local world = ecs.world
local w = world.w

local water_test_sys = ecs.system "water_test_system"
function water_test_sys:init()
    ecs.create_instance "/pkg/ant.test.features/assets/entities/water.prefab"
end