local imgui     = require "imgui_wrap"
local widget    = imgui.widget
local flags     = imgui.flags
local windows   = imgui.windows
local util      = imgui.util
local cursor    = imgui.cursor
local enum      = imgui.enum
local IO      = imgui.IO
local hub       = import_package "ant.editor".hub

local GuiBase = require "gui_base"
local gui_input = require "gui_input"
local GuiCanvas = GuiBase.derive("GuiCanvas")
local scene         = import_package "ant.scene".util
local ru = import_package "ant.render".util
--local map_imgui   = import_package "ant.editor".map_imgui

local dbgutil = require "common.debugutil"

local DEFAULT_FPS = 30

GuiCanvas.GuiName = "GuiCanvas"

local function get_time()
    return os.clock()
end

function GuiCanvas:_init()
    GuiBase._init(self)
    self.win_flags = flags.Window { "NoCollapse","NoClosed","NoScrollbar"}
    self.rect = {x=0,y=0,w=600,h=400}
    self.title_id = "Scene###Scene"
    self.vp_dirty = false
    self.time_count = 0
    self.cur_frame_time = 0.0
    self:set_fps(DEFAULT_FPS)
end

function GuiCanvas:set_fps(fps)
    assert(fps>0)
    self.fps = fps
    self.frame_time = 1/fps
end

function GuiCanvas:bind_world( world, world_update)
    self.world = world
    self.world_update = world_update

    self.next_frame_time = 0
    self.time_count = 0
    self.last_update = nil
end

function GuiCanvas:on_close_click()
    --dont close
end

function GuiCanvas:before_update()
    windows.SetNextWindowSize(self.rect.w,self.rect.h, "f")
    windows.PushStyleVar(enum.StyleVar.WindowPadding,0,0)
end

local focus_flag = flags.Focused {"ChildWindows"}
function GuiCanvas:on_update(delta)
    local w,h = windows.GetContentRegionAvail()
    local x,y = cursor.GetCursorScreenPos()
    local r = self.rect
    if w~=r.w or h~=r.h or x~=r.x or y ~= r.y then
        self.vp_dirty = self.vp_dirty or (w~=r.w) or (h~=r.h)
        -- if  (w~=r.w) or (h~=r.h) then
        --     log.trace(">>>>>>",w,r.w,h,r.h)
        -- end
        self.rect = {x=x,y=y,w=w,h=h}
    end
    if self.world then
        local world_tex =  ru.get_main_view_rendertexture(self.world)
        if world_tex then
            widget.ImageButton(world_tex,w,h,{frame_padding=0,bg_col={0,0,0,1}})
        end
    end
    if IO.WantCaptureMouse then
        --todo:split mouse and keyboard
        self:on_dispatch_msg()
    end
    if self.world_update then
        self:_update_world(delta)
    end
end

function GuiCanvas:_update_world(delta)
    local now = self.time_count + delta
    self.time_count = now
    if now >= self.next_frame_time then
        if self.last_update then
            self.cur_frame_time = now - self.last_update
        end
        self.next_frame_time = now + self.frame_time
        self.last_update = now
        --update world
        dbgutil.try(self.world_update)
    end

end

local mouse_what = {
	'LEFT', 'RIGHT', 'MIDDLE'
}

local mouse_state = {
	'DOWN', 'MOVE', 'UP'
}

local function which_mouse_state(call, pressed)
    if call.mouse_move then
        return "MOVE"
    end

    if call.mouse_click then
        return pressed and "DOWN" or "UP"
    end

    return "UNKNOWN"
end

function GuiCanvas:on_dispatch_msg()
    --todo:split mouse and keyboard``
    if self.world == nil then
        return
    end

    local gui_input = gui_input
    local in_mouse = gui_input.mouse
    local in_key = gui_input.key_state
    local key_down = gui_input.key_down
    local called = gui_input.called
    local rect = self.rect
    local rx,ry = 0,0
    if in_mouse.x then
        rx,ry = in_mouse.x - rect.x,in_mouse.y - rect.y
    end
    local focus = windows.IsWindowFocused(focus_flag)
    local hovered = windows.IsWindowHovered(focus_flag)

    local msgqueue = self.world.args.mq

    if focus then
        for what = 0,4 do
            if called[what] then
                msgqueue:push("mouse", 
                    mouse_what[what] or 'UNKNOWN', 
                    which_mouse_state(called, in_mouse[what]), rx, ry)
            end
        end
    end
    
    if hovered and called.mouse_wheel then
        msgqueue:push("mouse_wheel", rx, ry, in_mouse.scroll)
    end
    
    if focus and #key_down > 0 then
        local status = {
            CTRL = in_key.ctrl,
            ALT = in_key.alt,
            SHIFT = in_key.shift,
            SYS = in_key.sys,
        }
        for _,record in ipairs(key_down) do
            msgqueue:push("keyboard", record[1], record[2], status)
        end
    end
    local mouse_pressed =  gui_input.is_mouse_pressed(gui_input.MouseLeft)
    if not mouse_pressed and self.vp_dirty then
        self.vp_dirty = false
        msgqueue:push("resize", rect.w, rect.h)
    end
end
function GuiCanvas:after_update()
    windows.PopStyleVar()
end


return GuiCanvas