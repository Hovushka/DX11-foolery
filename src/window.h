#pragma once

#include <SDL.h>
#include <SDL_syswm.h>

struct Window {
	SDL_Window* SDL = nullptr;
	SDL_SysWMinfo sysWMInfo = {};

	void shout(const char*, const char* title="An error has occured");
};
