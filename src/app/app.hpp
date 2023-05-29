#ifndef CSP_APP_APP_HPP_INCLUDED_
#define CSP_APP_APP_HPP_INCLUDED_ 1
#pragma once

#include "types.hpp"

namespace App {
	i32 Init(u16 w, u16 h, const char* title = "csprite");
	void NewFrame();
	void EndFrame();
	void Release();
	void* GetWindow();
};

#endif // CSP_APP_APP_HPP_INCLUDED_
