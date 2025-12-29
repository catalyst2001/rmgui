#include "rmgui_glfw_opnegl33.h"
#include <nanovg_gl.h>
#include <utility>

class csysdf : public irm_sysdf
{
public:
  virtual void get_cursor_pos(int* p_dst_x, int* p_dst_y) {}
  virtual void set_cursor_pos(int x, int h) {}
  virtual uint32_t num_monitors() {
    return 0;
  }

  virtual void get_monitor_info(uint32_t monitor_idx, uint32_t* p_dst_DPI, uint32_t* p_w, uint32_t* p_h) {}
  virtual bool get_clipboard_data_info(RM_CB_DATA_TYPE& dst_data, size_t& dst_size) {
    return false;
  }

  virtual const uint8_t* get_clipboard_data_ex(RM_CB_DATA_TYPE& dst, size_t& size) {
    dst = RM_CLIPBOARD_DATA_TYPE_TEXT;
    const char* pstring = glfwGetClipboardString(nullptr);
    size = strlen(pstring);
    return (uint8_t*)pstring;
  }

  virtual void set_clipboard_data_ex(const uint8_t* p_src, size_t size) {
    glfwSetClipboardString(nullptr, (const char*)p_src);
  }

  virtual float get_time() {
    return static_cast<float>(glfwGetTime());
  }
} instance;

RM_KEY translate_glfw_key(int k)
{
  switch (k) {
  case GLFW_KEY_A: return RM_KEY_A;
  case GLFW_KEY_B: return RM_KEY_B;
  case GLFW_KEY_C: return RM_KEY_C;
  case GLFW_KEY_D: return RM_KEY_D;
  case GLFW_KEY_E: return RM_KEY_E;
  case GLFW_KEY_F: return RM_KEY_F;
  case GLFW_KEY_G: return RM_KEY_G;
  case GLFW_KEY_H: return RM_KEY_H;
  case GLFW_KEY_I: return RM_KEY_I;
  case GLFW_KEY_J: return RM_KEY_J;
  case GLFW_KEY_K: return RM_KEY_K;
  case GLFW_KEY_L: return RM_KEY_L;
  case GLFW_KEY_M: return RM_KEY_M;
  case GLFW_KEY_N: return RM_KEY_N;
  case GLFW_KEY_O: return RM_KEY_O;
  case GLFW_KEY_P: return RM_KEY_P;
  case GLFW_KEY_Q: return RM_KEY_Q;
  case GLFW_KEY_R: return RM_KEY_R;
  case GLFW_KEY_S: return RM_KEY_S;
  case GLFW_KEY_T: return RM_KEY_T;
  case GLFW_KEY_U: return RM_KEY_U;
  case GLFW_KEY_V: return RM_KEY_V;
  case GLFW_KEY_W: return RM_KEY_W;
  case GLFW_KEY_X: return RM_KEY_X;
  case GLFW_KEY_Y: return RM_KEY_Y;
  case GLFW_KEY_Z: return RM_KEY_Z;

  case GLFW_KEY_0: return RM_KEY_0;
  case GLFW_KEY_1: return RM_KEY_1;
  case GLFW_KEY_2: return RM_KEY_2;
  case GLFW_KEY_3: return RM_KEY_3;
  case GLFW_KEY_4: return RM_KEY_4;
  case GLFW_KEY_5: return RM_KEY_5;
  case GLFW_KEY_6: return RM_KEY_6;
  case GLFW_KEY_7: return RM_KEY_7;
  case GLFW_KEY_8: return RM_KEY_8;
  case GLFW_KEY_9: return RM_KEY_9;

  case GLFW_KEY_ESCAPE:       return RM_KEY_ESCAPE;
  case GLFW_KEY_ENTER:        return RM_KEY_ENTER;
  case GLFW_KEY_TAB:          return RM_KEY_TAB;
  case GLFW_KEY_BACKSPACE:    return RM_KEY_BACKSPACE;
  case GLFW_KEY_INSERT:       return RM_KEY_INSERT;
  case GLFW_KEY_DELETE:       return RM_KEY_DELETE;
  case GLFW_KEY_RIGHT:        return RM_KEY_RIGHT;
  case GLFW_KEY_LEFT:         return RM_KEY_LEFT;
  case GLFW_KEY_DOWN:         return RM_KEY_DOWN;
  case GLFW_KEY_UP:           return RM_KEY_UP;
  case GLFW_KEY_PAGE_UP:      return RM_KEY_PAGE_UP;
  case GLFW_KEY_PAGE_DOWN:    return RM_KEY_PAGE_DOWN;
  case GLFW_KEY_HOME:         return RM_KEY_HOME;
  case GLFW_KEY_END:          return RM_KEY_END;

  case GLFW_KEY_CAPS_LOCK:    return RM_KEY_CAPS_LOCK;
  case GLFW_KEY_SCROLL_LOCK:  return RM_KEY_SCROLL_LOCK;
  case GLFW_KEY_NUM_LOCK:     return RM_KEY_NUM_LOCK;
  case GLFW_KEY_PRINT_SCREEN: return RM_KEY_PRINT_SCREEN;
  case GLFW_KEY_PAUSE:        return RM_KEY_PAUSE;

  case GLFW_KEY_F1:  return RM_KEY_F1;
  case GLFW_KEY_F2:  return RM_KEY_F2;
  case GLFW_KEY_F3:  return RM_KEY_F3;
  case GLFW_KEY_F4:  return RM_KEY_F4;
  case GLFW_KEY_F5:  return RM_KEY_F5;
  case GLFW_KEY_F6:  return RM_KEY_F6;
  case GLFW_KEY_F7:  return RM_KEY_F7;
  case GLFW_KEY_F8:  return RM_KEY_F8;
  case GLFW_KEY_F9:  return RM_KEY_F9;
  case GLFW_KEY_F10: return RM_KEY_F10;
  case GLFW_KEY_F11: return RM_KEY_F11;
  case GLFW_KEY_F12: return RM_KEY_F12;


  case GLFW_KEY_LEFT_CONTROL:  return RM_KEY_LCTRL;
  case GLFW_KEY_RIGHT_CONTROL: return RM_KEY_RCTRL;

  default:
    return RM_KEY_NONE;
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  rm_surface* psurface = reinterpret_cast<rm_surface*>(glfwGetWindowUserPointer(window));
  assert(psurface && "cursor_position_callback(): psurface was nullptr!");
  psurface->mouse(RM_MOUSE_EVENT_MOVE, RM_KEY_NONE, DOWN, (int)xpos, (int)ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  rm_surface* psurface = reinterpret_cast<rm_surface*>(glfwGetWindowUserPointer(window));
  assert(psurface && "mouse_button_callback(): psurface was nullptr!");
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    RM_KEY_STATE state = (action == GLFW_PRESS ? DOWN : UP);
    psurface->mouse(RM_MOUSE_EVENT_CLICK, RM_KEY_NONE, state, (int)xpos, (int)ypos);
  }
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
  rm_surface* psurface = reinterpret_cast<rm_surface*>(glfwGetWindowUserPointer(window));
  assert(psurface && "char_callback(): psurface was nullptr!");
  psurface->textinput(codepoint);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  rm_surface* psurface = reinterpret_cast<rm_surface*>(glfwGetWindowUserPointer(window));
  assert(psurface && "key_callback(): psurface was nullptr!");
  RM_KEY vk = translate_glfw_key(key);
  RM_KEY_STATE state = (action == GLFW_PRESS ? DOWN : action == GLFW_RELEASE ? UP : REPEAT);
  psurface->keybd(scancode, vk, state);
}

GLFWwindow* init_opengl33_window(int width, int height, const char* title) {
  if (!glfwInit()) {
    rm_perr("init_opengl33_window(): failed to initialize GLFW");
    return nullptr;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* pwindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!pwindow) {
    rm_perr("init_opengl33_window(): failed to create GLFW window");
    glfwTerminate();
    return nullptr;
  }

  glfwMakeContextCurrent(pwindow);

  if (!gladLoadGL()) {
    rm_perr("init_opengl33_window(): failed to initialize GLAD");
    glfwDestroyWindow(pwindow);
    glfwTerminate();
    return nullptr;
  }

  return pwindow;
}

rm_surface* create_window(int posx, int posy, int width, int height, const char* title)
{
  /* init window */
  GLFWwindow* pwindow = init_opengl33_window(width, height, title);
  if (!pwindow){
    rm_perr("create_window(): failed init window");
    return nullptr;
  }

  /* set win pos */
  if(posx != -1 && posy != -1)
    glfwSetWindowPos(pwindow, posx, posy);

  /* create nvg ctx */
  auto pctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!pctx) {
    rm_perr("create_window(): failed to create nvg context");
    glfwDestroyWindow(pwindow);
    glfwTerminate();
    return nullptr;
  }

  /* create surface */
  rm_surface* psurface = new rm_surface(std::move(pctx), width, height, &instance, pwindow);

  /* save surface */
  glfwSetWindowUserPointer(pwindow, psurface);

  /* set callbacks */
  glfwSetCursorPosCallback(pwindow, cursor_position_callback);
  glfwSetMouseButtonCallback(pwindow, mouse_button_callback);
  glfwSetCharCallback(pwindow, char_callback);
  glfwSetKeyCallback(pwindow, key_callback);

  /* return surface */
  return psurface;
}

void destroy_window(rm_surface* psurface)
{
  assert(psurface && "destroy_window(): psurface was nullptr!");
  assert(psurface->get_context() && "destroy_window(): nvg context was nullptr!");
  assert(psurface->get_syswindow<GLFWwindow*>() && "destroy_window(): GLFWwindow was nullptr!");

  glfwDestroyWindow(psurface->get_syswindow<GLFWwindow*>());
  delete psurface;
  glfwTerminate();
}

void set_vsync(rm_surface* psurface, bool interval)
{
  assert(psurface && "set_vsync(): psurface was nullptr!");

  glfwSwapInterval(interval);
}
