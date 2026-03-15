#include "SDL_initializer.h"
#include "backend/rmgui_sdl3_opengl33.h"
#include "rmgui_controls.h"
#include "blend_ui.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include <SDL3/SDL_main.h>

static rm_surface* g_gui = nullptr;

int main(int argc, char** argv)
{
	SDL_initializer init(SDL_INIT_VIDEO);

	g_gui = create_sdl3_window(-1, -1, 1280, 720, "rmgui designer");
	if (!g_gui)
		return 1;

	set_sdl3_vsync(g_gui);

	rm_font default_font = g_gui->load_font("Verdana.ttf", "default");
	if (!default_font.isValid()) {
		SDL_Log("can't load font!");
	}

	g_gui->set_font(default_font);

	bndSetFont(g_gui->get_context()->findFont("default"));

	rm_image icon_sheet = g_gui->load_image("blender_icons16.png", 0);
	if (icon_sheet.isValid()) {
		bndSetIconImage(icon_sheet);
	}

	glDisable(GL_DEPTH_TEST);

	float last_time = 0.f;
	float current_time = g_gui->get_sysdf()->get_time();

	while (sdl3_poll_events(g_gui)) {
		glClearColor(34 / 255.f, 32 / 255.f, 33 / 255.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		last_time = current_time;
		current_time = g_gui->get_sysdf()->get_time();
		float dt = current_time - last_time;

		g_gui->draw(dt);
		sdl3_swap(g_gui);
	}

	destroy_sdl3_window(g_gui);
	return 0;
}