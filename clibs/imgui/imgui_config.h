#pragma once

#define IMGUI_USE_WCHAR32 1

#if !defined(NDEBUG)

#include <assert.h>

void IM_THROW(const char* err);

#define IM_ASSERT(_EXPR) do { \
		if (!(_EXPR)) {       \
			IM_THROW(#_EXPR); \
			assert(_EXPR);    \
		}                     \
	} while(0)

#endif
