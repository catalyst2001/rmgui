#pragma once
#include <SDL3/SDL.h>

class SDL_initializer
{
public:
	SDL_initializer(SDL_InitFlags flags=SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	~SDL_initializer();
};

