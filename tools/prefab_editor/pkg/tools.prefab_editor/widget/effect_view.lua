local ecs = ...
local world = ecs.world
local w = world.w
local iefk          = ecs.require "ant.efk|efk"
local imgui         = require "imgui"
local uiproperty    = require "widget.uiproperty"
local hierarchy     = require "hierarchy_edit"
local EffectView    = {}

function EffectView:_init()
    if self.inited then
        return
    end
    self.inited = true
    self.group_property = uiproperty.Group({label = "Effect"}, {
        uiproperty.EditText({label = "Path", readonly = true}, {
            getter = function() return self:on_get_path() end
        }),
        uiproperty.Float({label = "Speed", min = 0.01, max = 10.0, speed = 0.01}, {
            getter = function() return self:on_get_speed() end,
            setter = function(v) self:on_set_speed(v) end
        }),
        uiproperty.Bool({label = "AutoPlay"}, {
            getter = function() return self:on_get_auto_play() end,
            setter = function(v) self:on_set_auto_play(v) end
        }),
        -- uiproperty.Bool({label = "Loop"}, {
        --     getter = function() return self:on_get_loop() end,
        --     setter = function(v) self:on_set_loop(v) end
        -- }),
        uiproperty.Button({label = "Play", sameline = true}, {
            click = function() self:on_play() end
        }),
        uiproperty.Button({label = "Stop"}, {
            click = function() self:on_stop() end
        }),
    })
end

function EffectView:set_eid(eid)
    if self.eid == eid then
        return
    end
    if not eid then
        self.eid = nil
        return
    end
    local e <close> = world:entity(eid, "efk?in")
    if not e.efk then
        self.eid = nil
        return
    end
    self.eid = eid
    self:update()
end

function EffectView:update()
    if not self.eid then
        return
    end
    self.group_property:update()
end

function EffectView:show()
    if not self.eid then
        return
    end
    imgui.cursor.Separator()
    self.group_property:show()
end

function EffectView:on_play()
    iefk.play(self.eid)
end

function EffectView:on_stop()
    iefk.stop(self.eid)
end

function EffectView:on_get_speed()
    local tpl = hierarchy:get_template(self.eid)
    return tpl.template.data.efk.speed or 1.0
end

function EffectView:on_get_path()
    local tpl = hierarchy:get_template(self.eid)
    return tpl.template.data.efk.path
end

function EffectView:on_set_speed(value)
    local template = hierarchy:get_template(self.eid)
    template.template.data.efk.speed = value
    iefk.set_speed(self.eid, value)
    world:pub { "PatchEvent", self.eid, "/data/efk/speed", value }
end

function EffectView:on_get_auto_play()
    local template = hierarchy:get_template(self.eid)
    return template.template.data.efk.auto_play
end

function EffectView:on_set_auto_play(value)
    local template = hierarchy:get_template(self.eid)
    template.template.data.efk.auto_play = value
    world:pub { "PatchEvent", self.eid, "/data/efk/auto_play", value }
end

-- function EffectView:on_get_loop()
--     local template = hierarchy:get_template(self.eid)
--     return template.template.data.efk.loop
-- end

-- function EffectView:on_set_loop(value)
--     local template = hierarchy:get_template(self.eid)
--     template.template.data.efk.loop = value
--     iefk.set_loop(self.eid, value)
--     world:pub { "PatchEvent", self.eid, "/data/efk/loop", value }
-- end

return function ()
    EffectView:_init()
    return EffectView
end