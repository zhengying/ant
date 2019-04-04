﻿#define LUA_LIB
#define GLM_ENABLE_EXPERIMENTAL

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include "linalg.h"
	#include "math3d.h"
}

#include "meshbase/meshbase.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vector_relational.hpp>

#include <vector>
#include <array>
#include <cstring>

extern bool default_homogeneous_depth();
extern glm::vec3 to_viewdir(const glm::vec3 &e);

template<class ValueType>
static inline void
push_vec(lua_State *L, int num, ValueType &v) {
	lua_createtable(L, num, 0);
	for (int ii = 0; ii < num; ++ii) {
		lua_pushnumber(L, v[ii]);
		lua_seti(L, -2, ii + 1);
	}
};

template<class ValueType>
static inline void
fetch_vec(lua_State *L, int index, int num, ValueType &value) {
	for (int ii = 0; ii < num; ++ii) {
		lua_geti(L, index, ii + 1);
		value[ii] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
}

template<class ValueType>
static inline void
fetch_vec(lua_State *L, int index, const char* name, int num, ValueType &value) {
	lua_getfield(L, index, name);
	fetch_vec(L, -1, num, value);
	lua_pop(L, 1);
};

struct Frustum {
	float l, r, t, b, n, f;
	bool ortho;
};

static inline void
pull_frustum(lua_State *L, int index, Frustum &f) {
	lua_getfield(L, 2, "type");
	const char* type = lua_tostring(L, -1);
	lua_pop(L, 1);

	f.ortho = strcmp(type, "ortho") == 0;

	lua_getfield(L, index, "n");
	f.n = luaL_optnumber(L, -1, 0.1f);
	lua_pop(L, 1);
	lua_getfield(L, index, "f");
	f.f = luaL_optnumber(L, -1, 100.0f);
	lua_pop(L, 1);

	if (lua_getfield(L, index, "fov") == LUA_TNUMBER) {
		float fov = lua_tonumber(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, index, "aspect");
		float aspect = luaL_checknumber(L, -1);
		lua_pop(L, 1);
		float ymax = f.n * tanf(fov * (M_PI / 360));
		float xmax = ymax * aspect;
		f.l = -xmax;
		f.r = xmax;
		f.b = -ymax;
		f.t = ymax;

	} else {
		lua_pop(L, 1);
		float* fv = &f.l;

		const char* elemnames[] = {
			"l", "r", "t", "b",
		};

		for (auto name : elemnames) {
			lua_getfield(L, index, name);
			*fv++ = lua_tonumber(L, -1);
			lua_pop(L, 1);
		}
	}
}

static inline glm::mat4x4
projection_mat(const Frustum &f) {
	if (f.ortho) {
		auto orthLH = default_homogeneous_depth() ? glm::orthoLH_NO<float> : glm::orthoLH_ZO<float>;
		return orthLH(f.l, f.r, f.b, f.t, f.n, f.f);
	}

	auto frustumLH = default_homogeneous_depth() ? glm::frustumLH_NO<float> : glm::frustumLH_ZO<float>;
	return frustumLH(f.l, f.r, f.b, f.t, f.n, f.f);
}

static inline void
pull_aabb(lua_State *L, int index, AABB &aabb) {
	fetch_vec(L, index, "min", 3, aabb.min);
	fetch_vec(L, index, "max", 3, aabb.max);
}

static inline void
pull_sphere(lua_State *L, int index, BoundingSphere &sphere) {
	fetch_vec(L, index, "center", 3, sphere.center);	
	lua_getfield(L, 3, "radius");
	sphere.radius = lua_tonumber(L, -1);
	lua_pop(L, 1);
}

static inline void
pull_obb(lua_State *L, int index, OBB &obb) {
	for (int ii = 0; ii < 4; ++ii)
		fetch_vec(L, index, 4, obb.m[ii]);
}

static inline void
push_aabb(lua_State *L, const AABB &aabb) {
	lua_createtable(L, 0, 2);
	{
		push_vec(L, 3, aabb.min);
		lua_setfield(L, -2, "min");

		push_vec(L, 3, aabb.max);
		lua_setfield(L, -2, "max");
	}
}

static inline void
push_sphere(lua_State *L, const BoundingSphere &sphere) {
	lua_createtable(L, 0, 2);
	{
		push_vec(L, 3, sphere.center);
		lua_setfield(L, -2, "center");

		lua_pushnumber(L, sphere.radius);
		lua_setfield(L, -2, "radius");
	}
}

static inline void
push_obb(lua_State *L, const OBB &obb) {
	lua_createtable(L, 16, 0);
	{
		for (int ii = 0; ii < 4; ++ii) {
			for (int jj = 0; jj < 4; ++jj) {
				lua_pushnumber(L, obb.m[ii][jj]);
				lua_seti(L, -2, ii * 4 + jj + 1);
			}
		}
	}
}

static inline void
frustum_planes_intersection_points(std::array<glm::vec4, 6> &planes, std::array<glm::vec3, 8> &points) {
	enum PlaneName {
		left = 0, right,
		top, bottom,
		near, far
	};
	enum FrustumPointName {
		ltn = 0, rtn, ltf, rtf,
		lbn, rbn, lbf, rbf,
	};

	auto calc_intersection_point = [](auto p0, auto p1, auto p2) {
		auto crossp0p1 = (glm::cross(glm::vec3(p0), glm::vec3(p1)));
		auto t = p0.w * (glm::cross(glm::vec3(p1), glm::vec3(p2))) +
			p1.w * (glm::cross(glm::vec3(p2), glm::vec3(p0))) +
			p2.w * crossp0p1;

		return t / glm::dot(crossp0p1, glm::vec3(p2));
	};

	uint8_t defines[8][3] = {
		{PlaneName::left, PlaneName::top, PlaneName::near},
		{PlaneName::right, PlaneName::top, PlaneName::near},

		{PlaneName::left, PlaneName::top, PlaneName::far},
		{PlaneName::right, PlaneName::top, PlaneName::far},

		{PlaneName::left, PlaneName::bottom, PlaneName::near},
		{PlaneName::right, PlaneName::bottom, PlaneName::near},

		{PlaneName::left, PlaneName::bottom, PlaneName::far},
		{PlaneName::right, PlaneName::bottom, PlaneName::far},
	};

	for (int ii = 0; ii < 8; ++ii) {
		int idx0 = defines[ii][0], idx1 = defines[ii][1], idx2 = defines[ii][2];
		points[ii] = calc_intersection_point(planes[idx0], planes[idx1], planes[idx2]);
	}
}


static inline void 
extract_planes(std::array<glm::vec4, 6> &planes, const glm::mat4x4 &projMat, bool normalize) {
	// Left clipping plane
	planes[0][0] = projMat[3][0] + projMat[0][0];
	planes[0][1] = projMat[3][1] + projMat[0][1];
	planes[0][2] = projMat[3][2] + projMat[0][2];
	planes[0][3] = projMat[3][3] + projMat[0][3];
	// Right clipping plane
	planes[1][0] = projMat[3][0] - projMat[0][0];
	planes[1][1] = projMat[3][1] - projMat[0][1];
	planes[1][2] = projMat[3][2] - projMat[0][2];
	planes[1][3] = projMat[3][3] - projMat[0][3];
	// Top clipping plane
	planes[2][0] = projMat[3][0] - projMat[1][0];
	planes[2][1] = projMat[3][1] - projMat[1][1];
	planes[2][2] = projMat[3][2] - projMat[1][2];
	planes[2][3] = projMat[3][3] - projMat[1][3];
	// Bottom clipping plane
	planes[3][0] = projMat[3][0] + projMat[1][0];
	planes[3][1] = projMat[3][1] + projMat[1][1];
	planes[3][2] = projMat[3][2] + projMat[1][2];
	planes[3][3] = projMat[3][3] + projMat[1][3];
	// Near clipping plane
	if (default_homogeneous_depth()) {		
		planes[4][0] = projMat[3][0] + projMat[2][0];
		planes[4][1] = projMat[3][1] + projMat[2][1];
		planes[4][2] = projMat[3][2] + projMat[2][2];
		planes[4][3] = projMat[3][3] + projMat[2][3];
	} else {
		planes[4][0] = projMat[0][2];
		planes[4][1] = projMat[1][2];
		planes[4][2] = projMat[2][2];
		planes[4][3] = projMat[3][2];
	}

	// Far clipping plane
	planes[5][0] = projMat[3][0] - projMat[2][0];
	planes[5][1] = projMat[3][1] - projMat[2][1];
	planes[5][2] = projMat[3][2] - projMat[2][2];
	planes[5][3] = projMat[3][3] - projMat[2][3];
	// Normalize the plane equations, if requested
	if (normalize){
		for (auto &p : planes) {
			auto len = glm::length(glm::vec3(p));
			if (glm::abs(len) >= glm::epsilon<float>())
				p /= len;
		}
			
		//NormalizePlane(p_planes[0]);
		//NormalizePlane(p_planes[1]);
		//NormalizePlane(p_planes[2]);
		//NormalizePlane(p_planes[3]);
		//NormalizePlane(p_planes[4]);
		//NormalizePlane(p_planes[5]);
	}
}

static inline void
calc_extreme_value(float v, float min, float max, float &tmin, float &tmax) {
	if (v > 0.f) {
		tmin += v * min;
		tmax += v * max;
	} else {
		tmin += v * max;
		tmax += v * min;
	}
}


static int
plane_intersect(const glm::vec4 &plane, const AABB &aabb) {
	float minD = 0, maxD = 0;
	for (int ii = 0; ii < 3; ++ii) {
		calc_extreme_value(plane[ii], aabb.min[ii], aabb.max[ii], minD, maxD);
	}
	
	if (minD >= plane.w)
		return 1;

	if (maxD <= plane.w)
		return -1;

	return 0;
}

static int
plane_intersect(const glm::vec4 &plane, const BoundingSphere &sphere) {
	assert(false && "not implement");
	return 0;
}

static inline void
pull_table_to_mat(lua_State *L, int index, glm::mat4x4 &matProj) {
	for (int ii = 0; ii < 16; ++ii) {
		lua_geti(L, 1, ii + 1);
		matProj[ii / 4][ii % 4] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
}


static inline void 
pull_frustum_planes(lua_State *L, std::array<glm::vec4, 6> &planes, int index) {	
	const int type = lua_type(L, index);

	glm::mat4x4 matProj;
	if (type == LUA_TTABLE) {
		const size_t tlen = lua_rawlen(L, 1);

		if (tlen == 0) {
			for (int iPlane = 0; iPlane < 6; ++iPlane) {				
				for (int ii = 0; ii < 4; ++ii) {
					lua_geti(L, 1, iPlane * 4 + ii + 1);
					planes[iPlane][ii] = lua_tonumber(L, -1);
					lua_pop(L, 1);
				}
			}
			return;
		}

		if (tlen == 0) {
			Frustum f;
			pull_frustum(L, 1, f);

			matProj = projection_mat(f);
		} else if (tlen == 16) {
			pull_table_to_mat(L, 1, matProj);
		}
	} else if (type == LUA_TUSERDATA || type == LUA_TLIGHTUSERDATA) {
		matProj = *(reinterpret_cast<const glm::mat4x4*>(lua_touserdata(L, 1)));
	}

	extract_planes(planes, matProj, true);
}

static int
lextract_planes(lua_State *L) {
	std::array<glm::vec4, 6> planes;
	pull_frustum_planes(L, planes, 1);

	lua_createtable(L, 4 * 6, 0);
	for (int iPlane = 0; iPlane < 6; ++iPlane){
		for (int ii = 0; ii < 4; ++ii) {
			lua_pushnumber(L, planes[iPlane][ii]);
			lua_seti(L, -2, iPlane * 4 + ii + 1);
		}		
	}

	return 1;
}

template<class BoundingType>
static inline const char*
planes_intersect(const std::array<glm::vec4, 6> &planes, const BoundingType &aabb) {
	for (const auto &p : planes) {
		const int r = plane_intersect(p, aabb);
		if (r < 0)
			return "outside";

		if (r == 0)
			return "intersect";
	}

	return "inside";
}

static int
lintersect(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	std::array<glm::vec4, 6> planes;
	pull_frustum_planes(L, planes, 1);

	const char* intersectresult = nullptr;

	const char*boundingtype = luaL_checkstring(L, 2);
	if (strcmp(boundingtype, "aabb") == 0) {
		AABB aabb;
		pull_aabb(L, 3, aabb);

		intersectresult = planes_intersect(planes, aabb);
	} else if (strcmp(boundingtype, "sphere") == 0){
		BoundingSphere sphere;
		pull_sphere(L, 3, sphere);
		intersectresult = planes_intersect(planes, sphere);
	} else {
		luaL_error(L, "not support bounding type:%s", boundingtype);
	}

	lua_pushstring(L, intersectresult);
	return 1;
}

static inline void
transform_aabb(const glm::mat4x4 &trans, AABB &aabb) {
	const glm::vec3 pos = trans[3];
	AABB result(pos, pos);

	for (int icol = 0; icol < 3; ++icol)
		for (int irow = 0; irow < 3; ++irow) {
			calc_extreme_value(trans[icol][irow], aabb.min[irow], aabb.max[irow], result.min[irow], result.max[irow]);
		}
	aabb = result;
}

static int
ltransform_aabb(lua_State *L) {	
	struct boxstack *BS = (struct boxstack *)luaL_checkudata(L, 1, LINALG);
	struct lastack *LS = BS->LS;
	
	int type;
	const glm::mat4x4 *trans = (const glm::mat4x4 *)lastack_value(LS, get_stack_id(L, LS, 2), &type);

	luaL_checktype(L, 3, LUA_TTABLE);
	AABB aabb;
	pull_aabb(L, 3, aabb);
	transform_aabb(*trans, aabb);
	push_aabb(L, aabb);
	return 1;
}

static int
lfrustum_points(lua_State *L) {
	std::array<glm::vec4, 6> planes;
	pull_frustum_planes(L, planes, 1);

	std::array<glm::vec3, 8> points;
	frustum_planes_intersection_points(planes, points);

	lua_createtable(L, 3 * 8, 0);
	for (int ipoint = 0; ipoint < 8; ++ipoint)
		for (int ii = 0; ii < 3; ++ii) {
			lua_pushnumber(L, points[ipoint][ii]);
			lua_seti(L, -2, ipoint * 3 + ii+1);
		}
	return 1;
}

static int
fetch_bounding(lua_State *L, int index, Bounding &bounding) {	
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "aabb");
	pull_aabb(L, -1, bounding.aabb);
	lua_pop(L, 1);

	lua_getfield(L, index, "sphere");
	pull_sphere(L, -1, bounding.sphere);
	lua_pop(L, 1);

	const int fieldtype = lua_getfield(L, index, "obb");
	if (fieldtype != LUA_TNIL)
		pull_obb(L, -1, bounding.obb);
	lua_pop(L, 1);

	return 1;
}

static int
push_bounding(lua_State *L, const Bounding &boundiing) {
	lua_newtable(L);
	{
		push_aabb(L, boundiing.aabb);
		lua_setfield(L, -2, "aabb");

		push_sphere(L, boundiing.sphere);
		lua_setfield(L, -2, "sphere");

		push_obb(L, boundiing.obb);
		lua_setfield(L, -2, "obb");
	}
	return 1;
}

static int
lmerge_boundings(lua_State *L) {
	struct boxstack* bs = (struct boxstack*)lua_touserdata(L, 1);
	struct lastack *LS = bs->LS;

	luaL_checktype(L, 2, LUA_TTABLE);
	const int numboundings = (int)lua_rawlen(L, 2);
	Bounding scenebounding;

	for (int ii = 0; ii < numboundings; ++ii) {
		lua_geti(L, 2, ii + 1);

		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "bounding");
		Bounding bounding;
		fetch_bounding(L, -1, bounding);
		lua_pop(L, 1);

		const int fieldtype = lua_getfield(L, -1, "transform");
		if (fieldtype != LUA_TNIL) {
			int type;
			const glm::mat4x4 *trans = (const glm::mat4x4 *)lastack_value(LS, get_stack_id(L, LS, -1), &type);
			transform_aabb(*trans, bounding.aabb);
		}
		lua_pop(L, 1);

		lua_pop(L, 1);

		scenebounding.aabb.Merge(bounding.aabb);
	}

	scenebounding.sphere.Init(scenebounding.aabb);
	scenebounding.obb.Init(scenebounding.aabb);
	return push_bounding(L, scenebounding);
}

extern "C"{
	LUAMOD_API int
	luaopen_math3d_baselib(lua_State *L){
		luaL_Reg l[] = {			
			{ "intersect",		lintersect },
			{ "extract_planes", lextract_planes},			
			{ "frustum_points", lfrustum_points},

			{ "transform_aabb", ltransform_aabb},
			{ "merge_boundings", lmerge_boundings},
			{ NULL, NULL },
		};

		luaL_newlib(L, l);

		return 1;
	}
}