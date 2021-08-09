#pragma once

#include <window.h>

#include <SDL.h>

void Window::shout(const char* message, const char* title) {
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_WARNING, 
		title, 
		message, 
		this->SDL
	);
}
