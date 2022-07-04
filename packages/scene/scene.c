#define LUA_LIB

#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#include "component.h"
#include "luaecs.h"
#include "set.h"
#include "scene.h"

static int
scene_changed(lua_State *L) {
	struct ecs_world* world = getworld(L, 1);
	struct ecs_context* ecs = world->ecs;

	int i;
	struct set change_set;
	set_init(&change_set);
	for (i=0;entity_iter(ecs, COMPONENT_SCENE_UPDATE, i);i++) {
		//struct scene * s = (struct scene *)entity_sibling(ecs, TAG_SCENE_UPDATE, i, COMPONENT_SCENE);
		//printf("Changes %d : %d %s\n", (int)e->id, (int)v->parent, change ? "true" : "false");
		if (entity_sibling(ecs, COMPONENT_SCENE_UPDATE, i, COMPONENT_SCENE_CHANGED)) {
			component_id* id = (component_id*)entity_sibling(ecs, COMPONENT_SCENE_UPDATE, i, COMPONENT_ID);
			if (id == NULL) {
				return luaL_error(L, "Entity id not found");
			}
			set_insert(&change_set, *id);
		} else {
			struct component_scene * s = (struct component_scene *)entity_sibling(ecs, COMPONENT_SCENE_UPDATE, i, COMPONENT_SCENE);
			if (s){
				if (set_exist(&change_set, s->parent)) {
					component_id* id = (component_id*)entity_sibling(ecs, COMPONENT_SCENE_UPDATE, i, COMPONENT_ID);
					set_insert(&change_set, *id);
					entity_enable_tag(ecs, COMPONENT_SCENE_UPDATE, i, COMPONENT_SCENE_CHANGED);
				}
			}
		}
	}

	set_deinit(&change_set);
	return 0;
}

LUAMOD_API int
luaopen_system_scene(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "scene_changed", scene_changed },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

/*
testscene.lua

local scene = require "scene.core"

local ecs = require "ecs"

local w = ecs.world()

w:register {
	name = "scene",
	"parent:int64",
}

w:register {
	name = "entityid",
	type = "int64",
}

w:register {
	name = "change"
}

local context = w:context {
	"scene_update",
	"scene",
	"id",
	"scene_changed"
}



--[[
    1
   / \
  2   3
 / \
4   5
]]

w:new {
	entityid = 1,
	scene = {
		parent = 0,
	}
}


w:new {
	entityid = 2,
	scene = {
		parent = 1,
	}
}

w:new {
	entityid = 3,
	scene = {
		parent = 1,
	}
}

w:new {
	entityid = 4,
	scene = {
		parent = 2,
	}
}

w:new {
	entityid = 5,
	scene = {
		parent = 2,
	}
}

local function keys(t)
	local r =  {}
	for _, key in ipairs(t) do
		r[key] = true
	end
	return r
end

local changeset = keys { 2, 3 }

for v in w:select "entityid:in scene:in change?out" do
	if changeset[v.entityid] then
		v.change = true
	end
end

local function print_changes()
	for v in w:select "change entityid:in" do
		print(v.entityid, "CHANGE")
	end
end

print_changes()

print "Update"

scene.update_changes(context)

print_changes()

*/