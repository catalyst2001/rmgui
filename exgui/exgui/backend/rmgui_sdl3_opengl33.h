#pragma once
#include <glad.h>
#include <SDL3/SDL.h>
#include "rmgui.h"

rm_surface* create_sdl3_window(int posx, int posy, int width, int height, const char* title);
void destroy_sdl3_window(rm_surface* psurface);
void set_sdl3_vsync(rm_surface* psurface, bool interval = true);
bool sdl3_poll_events(rm_surface* psurface);
void sdl3_swap(rm_surface* psurface);
