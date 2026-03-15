#include <stdexcept>
#include "SDL_initializer.h"

SDL_initializer::SDL_initializer(SDL_InitFlags flags)
{
	char text[512];
	if (!SDL_Init(flags)) {
		SDL_snprintf(text, sizeof(text), "SDL_Init(%x) failed! Error: \"%s\"", flags, SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Critical error", text, nullptr);
		throw std::runtime_error(text);
	}
}

SDL_initializer::~SDL_initializer()
{
	SDL_Quit();
}
