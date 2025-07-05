#pragma once
#include <glad.h>
#include <glfw3.h>
#include "rmgui.h"

rm_surface* create_window(int posx, int posy, int width, int height, const char* title);
void destroy_window(rm_surface* psurface);
void set_vsync(rm_surface* psurface, bool interval = 1);