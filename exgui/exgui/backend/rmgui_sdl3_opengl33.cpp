#include "rmgui_sdl3_opengl33.h"
#include <nanovg_gl.h>
#include <utility>

class csdl3sysdf : public irm_sysdf
{
  SDL_Window* m_pwindow;
public:
  csdl3sysdf() : m_pwindow(nullptr) {}
  void set_window(SDL_Window* pw) { m_pwindow = pw; }

  virtual void get_cursor_pos(int* p_dst_x, int* p_dst_y) {
    float fx, fy;
    SDL_GetMouseState(&fx, &fy);
    if (p_dst_x) *p_dst_x = (int)fx;
    if (p_dst_y) *p_dst_y = (int)fy;
  }
  virtual void set_cursor_pos(int x, int y) {
    if (m_pwindow)
      SDL_WarpMouseInWindow(m_pwindow, (float)x, (float)y);
  }
  virtual uint32_t num_monitors() {
    int count = 0;
    SDL_GetDisplays(&count);
    return (uint32_t)count;
  }
  virtual void get_monitor_info(uint32_t monitor_idx, uint32_t* p_dst_DPI, uint32_t* p_w, uint32_t* p_h) {
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    if (!displays || (int)monitor_idx >= count) return;
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displays[monitor_idx]);
    if (mode) {
      if (p_w) *p_w = (uint32_t)mode->w;
      if (p_h) *p_h = (uint32_t)mode->h;
    }
    if (p_dst_DPI) {
      float dpi = SDL_GetDisplayContentScale(displays[monitor_idx]);
      *p_dst_DPI = (uint32_t)(dpi * 96.f);
    }
    SDL_free(displays);
  }
  virtual bool get_clipboard_data_info(RM_CB_DATA_TYPE& dst_data, size_t& dst_size) {
    if (SDL_HasClipboardText()) {
      dst_data = RM_CLIPBOARD_DATA_TYPE_TEXT;
      char* text = SDL_GetClipboardText();
      dst_size = text ? strlen(text) : 0;
      SDL_free(text);
      return true;
    }
    return false;
  }
  virtual const uint8_t* get_clipboard_data_ex(RM_CB_DATA_TYPE& dst, size_t& size) {
    dst = RM_CLIPBOARD_DATA_TYPE_TEXT;
    /* SDL_GetClipboardText returns a copy that we must keep alive.
       Use a static buffer so the pointer stays valid until next call. */
    static std::string s_clipboard;
    char* text = SDL_GetClipboardText();
    s_clipboard = text ? text : "";
    SDL_free(text);
    size = s_clipboard.size();
    return (const uint8_t*)s_clipboard.c_str();
  }
  virtual void set_clipboard_data_ex(const uint8_t* p_src, size_t size) {
    std::string tmp((const char*)p_src, size);
    SDL_SetClipboardText(tmp.c_str());
  }
  virtual float get_time() {
    return (float)SDL_GetTicks() / 1000.f;
  }
};

static csdl3sysdf g_sdl3_sysdf;

static RM_KEY translate_sdl3_key(SDL_Keycode k)
{
  switch (k) {
  case SDLK_A: return RM_KEY_A;
  case SDLK_B: return RM_KEY_B;
  case SDLK_C: return RM_KEY_C;
  case SDLK_D: return RM_KEY_D;
  case SDLK_E: return RM_KEY_E;
  case SDLK_F: return RM_KEY_F;
  case SDLK_G: return RM_KEY_G;
  case SDLK_H: return RM_KEY_H;
  case SDLK_I: return RM_KEY_I;
  case SDLK_J: return RM_KEY_J;
  case SDLK_K: return RM_KEY_K;
  case SDLK_L: return RM_KEY_L;
  case SDLK_M: return RM_KEY_M;
  case SDLK_N: return RM_KEY_N;
  case SDLK_O: return RM_KEY_O;
  case SDLK_P: return RM_KEY_P;
  case SDLK_Q: return RM_KEY_Q;
  case SDLK_R: return RM_KEY_R;
  case SDLK_S: return RM_KEY_S;
  case SDLK_T: return RM_KEY_T;
  case SDLK_U: return RM_KEY_U;
  case SDLK_V: return RM_KEY_V;
  case SDLK_W: return RM_KEY_W;
  case SDLK_X: return RM_KEY_X;
  case SDLK_Y: return RM_KEY_Y;
  case SDLK_Z: return RM_KEY_Z;

  case SDLK_0: return RM_KEY_0;
  case SDLK_1: return RM_KEY_1;
  case SDLK_2: return RM_KEY_2;
  case SDLK_3: return RM_KEY_3;
  case SDLK_4: return RM_KEY_4;
  case SDLK_5: return RM_KEY_5;
  case SDLK_6: return RM_KEY_6;
  case SDLK_7: return RM_KEY_7;
  case SDLK_8: return RM_KEY_8;
  case SDLK_9: return RM_KEY_9;

  case SDLK_ESCAPE:       return RM_KEY_ESCAPE;
  case SDLK_RETURN:       return RM_KEY_ENTER;
  case SDLK_TAB:          return RM_KEY_TAB;
  case SDLK_BACKSPACE:    return RM_KEY_BACKSPACE;
  case SDLK_INSERT:       return RM_KEY_INSERT;
  case SDLK_DELETE:       return RM_KEY_DELETE;
  case SDLK_RIGHT:        return RM_KEY_RIGHT;
  case SDLK_LEFT:         return RM_KEY_LEFT;
  case SDLK_DOWN:         return RM_KEY_DOWN;
  case SDLK_UP:           return RM_KEY_UP;
  case SDLK_PAGEUP:       return RM_KEY_PAGE_UP;
  case SDLK_PAGEDOWN:     return RM_KEY_PAGE_DOWN;
  case SDLK_HOME:         return RM_KEY_HOME;
  case SDLK_END:          return RM_KEY_END;

  case SDLK_CAPSLOCK:     return RM_KEY_CAPS_LOCK;
  case SDLK_SCROLLLOCK:   return RM_KEY_SCROLL_LOCK;
  case SDLK_NUMLOCKCLEAR: return RM_KEY_NUM_LOCK;
  case SDLK_PRINTSCREEN:  return RM_KEY_PRINT_SCREEN;
  case SDLK_PAUSE:        return RM_KEY_PAUSE;

  case SDLK_LCTRL:        return RM_KEY_LCTRL;
  case SDLK_RCTRL:        return RM_KEY_RCTRL;

  case SDLK_F1:  return RM_KEY_F1;
  case SDLK_F2:  return RM_KEY_F2;
  case SDLK_F3:  return RM_KEY_F3;
  case SDLK_F4:  return RM_KEY_F4;
  case SDLK_F5:  return RM_KEY_F5;
  case SDLK_F6:  return RM_KEY_F6;
  case SDLK_F7:  return RM_KEY_F7;
  case SDLK_F8:  return RM_KEY_F8;
  case SDLK_F9:  return RM_KEY_F9;
  case SDLK_F10: return RM_KEY_F10;
  case SDLK_F11: return RM_KEY_F11;
  case SDLK_F12: return RM_KEY_F12;

  default:
    return RM_KEY_NONE;
  }
}

rm_surface* create_sdl3_window(int posx, int posy, int width, int height, const char* title)
{
  /* request OpenGL 3.3 core */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* pwindow = SDL_CreateWindow(title, width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!pwindow) {
    rm_perr("create_sdl3_window(): SDL_CreateWindow failed: %s", SDL_GetError());
    return nullptr;
  }

  if (posx != -1 && posy != -1)
    SDL_SetWindowPosition(pwindow, posx, posy);

  SDL_GLContext glctx = SDL_GL_CreateContext(pwindow);
  if (!glctx) {
    rm_perr("create_sdl3_window(): SDL_GL_CreateContext failed: %s", SDL_GetError());
    SDL_DestroyWindow(pwindow);
    return nullptr;
  }
  SDL_GL_MakeCurrent(pwindow, glctx);

  /* load GL function pointers via glad */
  if (!gladLoadGL()) {
    rm_perr("create_sdl3_window(): gladLoadGL failed");
    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(pwindow);
    return nullptr;
  }

  /* create NanoVG context */
  auto pctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!pctx) {
    rm_perr("create_sdl3_window(): nvgCreateGL3 failed");
    SDL_GL_DestroyContext(glctx);
    SDL_DestroyWindow(pwindow);
    return nullptr;
  }

  g_sdl3_sysdf.set_window(pwindow);

  rm_surface* psurface = new rm_surface(std::move(pctx), width, height, &g_sdl3_sysdf, pwindow);
  return psurface;
}

void destroy_sdl3_window(rm_surface* psurface)
{
  assert(psurface && "destroy_sdl3_window(): psurface was nullptr!");
  SDL_Window* pw = psurface->get_syswindow<SDL_Window*>();
  SDL_GLContext glctx = SDL_GL_GetCurrentContext();
  delete psurface;
  if (glctx) SDL_GL_DestroyContext(glctx);
  if (pw) SDL_DestroyWindow(pw);
}

void set_sdl3_vsync(rm_surface* psurface, bool interval)
{
  (void)psurface;
  SDL_GL_SetSwapInterval(interval ? 1 : 0);
}

bool sdl3_poll_events(rm_surface* psurface)
{
  assert(psurface && "sdl3_poll_events(): psurface was nullptr!");
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_EVENT_QUIT:
      return false;

    case SDL_EVENT_WINDOW_RESIZED: {
      int w = e.window.data1;
      int h = e.window.data2;
      psurface->resize((float)w, (float)h);
      glViewport(0, 0, w, h);
      break;
    }

    case SDL_EVENT_MOUSE_MOTION:
      psurface->mouse(RM_MOUSE_EVENT_MOVE, RM_KEY_NONE, DOWN,
          (int)e.motion.x, (int)e.motion.y);
      break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      RM_KEY vk = RM_KEY_NONE;
      switch (e.button.button) {
      case SDL_BUTTON_LEFT:   vk = RM_KEY_LMOUSE; break;
      case SDL_BUTTON_MIDDLE: vk = RM_KEY_MMOUSE; break;
      case SDL_BUTTON_RIGHT:  vk = RM_KEY_RMOUSE; break;
      default: break;
      }
      if (vk != RM_KEY_NONE) {
        RM_KEY_STATE state = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? DOWN : UP;
        psurface->mouse(RM_MOUSE_EVENT_CLICK, vk, state,
            (int)e.button.x, (int)e.button.y);
      }
      break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      RM_KEY vk = translate_sdl3_key(e.key.key);
      RM_KEY_STATE state = (e.type == SDL_EVENT_KEY_DOWN) ?
          (e.key.repeat ? REPEAT : DOWN) : UP;
      psurface->keybd((int)e.key.scancode, vk, state);
      break;
    }

    case SDL_EVENT_TEXT_INPUT:
      /* SDL3 text input delivers UTF-8 strings */
      if (e.text.text) {
        const char* p = e.text.text;
        while (*p) {
          uint32_t cp = 0;
          uint8_t c = (uint8_t)*p;
          if (c < 0x80) {
            cp = c; p += 1;
          } else if ((c & 0xE0) == 0xC0) {
            cp = (c & 0x1F) << 6;
            cp |= ((uint8_t)p[1] & 0x3F);
            p += 2;
          } else if ((c & 0xF0) == 0xE0) {
            cp = (c & 0x0F) << 12;
            cp |= ((uint8_t)p[1] & 0x3F) << 6;
            cp |= ((uint8_t)p[2] & 0x3F);
            p += 3;
          } else if ((c & 0xF8) == 0xF0) {
            cp = (c & 0x07) << 18;
            cp |= ((uint8_t)p[1] & 0x3F) << 12;
            cp |= ((uint8_t)p[2] & 0x3F) << 6;
            cp |= ((uint8_t)p[3] & 0x3F);
            p += 4;
          } else {
            p += 1;
            continue;
          }
          psurface->textinput((int)cp);
        }
      }
      break;
    }
  }
  return true;
}

void sdl3_swap(rm_surface* psurface)
{
  SDL_Window* pw = psurface->get_syswindow<SDL_Window*>();
  if (pw)
    SDL_GL_SwapWindow(pw);
}
