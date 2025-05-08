#include <glad.h>
#include <glfw3.h>
#include "rmgui.h"
#include "rmgui_controls.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include <iostream>
#include <vector>
#include "blendish_test.h"

// Глобальный указатель на корневую поверхность GUI для использования в колбэках
static rm_surface* g_gui = nullptr;

// GLFW‑колбэк для движения курсора (координаты принимаются как есть – (0,0) в левом верхнем углу)
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  g_gui->mouse(EXGUI_MOUSE_EVENT_MOVE, EXGUI_KEY_NONE, DOWN, (int)xpos, (int)ypos);
}

// GLFW‑колбэк для обработки нажатий мыши
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    // Передаем состояние в зависимости от action
    EXGUI_KEY_STATE state = (action == GLFW_PRESS ? DOWN : UP);
    g_gui->mouse(EXGUI_MOUSE_EVENT_CLICK, EXGUI_KEY_NONE, state, (int)xpos, (int)ypos);
  }
}

// GLFW‑колбэк для ввода символов (текстовый ввод)
void char_callback(GLFWwindow* window, unsigned int codepoint) {
  g_gui->textinput((int)codepoint);
}

// GLFW‑колбэк для обработки нажатий клавиш
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    switch (key)
    {
    case GLFW_KEY_BACKSPACE:
      g_gui->textinput(8); //send backspace
      break;
    case GLFW_KEY_ENTER:
      g_gui->textinput('\n'); //return
      break;

    default:
      break;
    }   
    
    g_gui->keybd(scancode, EXGUI_KEY_NONE, DOWN);
  }
}

GLFWwindow* initWindow(int width, int height, const char* title) {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return nullptr;
  }
  // Используем OpenGL 3.3 Core Profile
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return nullptr;
  }
  glfwMakeContextCurrent(window);
  if (!gladLoadGL()) {
    std::cerr << "Failed to initialize GLAD\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return nullptr;
  }
  return window;
}

class csysdf : public irmgui_sysdf
{
public:
  virtual void get_cursor_pos(int* p_dst_x, int* p_dst_y) {}
  virtual void set_cursor_pos(int x, int h) {}
  virtual uint32_t num_monitors() {
    return 0;
  }

  virtual void get_monitor_info(uint32_t monitor_idx, uint32_t* p_dst_DPI, uint32_t* p_w, uint32_t* p_h) {}
  virtual bool get_clipboard_data_info(EXGUI_CB_DATA_TYPE& dst_data, size_t& dst_size) { return true; }
  virtual const uint8_t* get_clipboard_data_ex(EXGUI_CB_DATA_TYPE& dst, size_t& size) { return nullptr; }
  virtual void set_clipboard_data_ex(const uint8_t* p_src, size_t size) {}
  virtual float get_time() {
    return static_cast<float>(glfwGetTime());
  }
} instance;

template<class t>
t max(t a, t b)
{
  if (a > b)
    return a;
  if (b > a)
    return b;

  return a;
}

template<class t>
t clamp(t a, t mm, t mx)
{
  if (a < mm)
    return a;
  if (a > mx)
    return mx;
  return a;
}

#pragma region TEMPLATE1
void drawParagraph(struct NVGcontext* vg, float x, float y, float width, float height, float mx, float my)
{
  struct NVGtextRow rows[3];
  struct NVGglyphPosition glyphs[100];
  const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.🎉";
  const char* start;
  const char* end;
  int nrows, i, nglyphs, j, lnum = 0;
  float lineh;
  float caretx, px;
  float bounds[4];
  float a;
  float gx = 0.0f, gy = 0.0f;
  int gutter = 0;
  NVG_NOTUSED(height);

  nvgSave(vg);

  nvgFontSize(vg, 18.0f);
  nvgFontFace(vg, "default");
  nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  nvgTextMetrics(vg, NULL, NULL, &lineh);

  // The text break API can be used to fill a large buffer of rows,
  // or to iterate over the text just few lines (or just one) at a time.
  // The "next" variable of the last returned item tells where to continue.
  start = text;
  end = text + strlen(text);
  for (nrows = nvgTextBreakLines(vg, start, end, width, rows, 3); 0 != nrows; nrows = nvgTextBreakLines(vg, start, end, width, rows, 3))
  {
    for (i = 0; i < nrows; i++) {
      struct NVGtextRow* row = &rows[i];
      int hit = mx > x && mx < (x + width) && my >= y && my < (y + lineh);

      nvgBeginPath(vg);
      nvgFillColor(vg, nvgRGBA(255, 255, 255, hit ? 64 : 16));
      nvgRect(vg, x, y, row->width, lineh);
      nvgFill(vg);

      nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
      nvgText(vg, x, y, row->start, row->end);

      if (hit) {
        caretx = (mx < x + row->width / 2) ? x : x + row->width;
        px = x;
        nglyphs = nvgTextGlyphPositions(vg, x, y, row->start, row->end, glyphs, 100);
        for (j = 0; j < nglyphs; j++) {
          float x0 = glyphs[j].x;
          float x1 = (j + 1 < nglyphs) ? glyphs[j + 1].x : x + row->width;
          float tgx = x0 * 0.3f + x1 * 0.7f;
          if (mx >= px && mx < tgx)
            caretx = glyphs[j].x;
          px = tgx;
        }
        nvgBeginPath(vg);
        nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
        nvgRect(vg, caretx, y, 1, lineh);
        nvgFill(vg);

        gutter = lnum + 1;
        gx = x - 10;
        gy = y + lineh / 2;
      }
      lnum++;
      y += lineh;
    }
    // Keep going...
    start = rows[nrows - 1].next;
  }

  if (gutter)
  {
    char txt[16];
    snprintf(txt, sizeof(txt), "%d", gutter);
    nvgFontSize(vg, 13.0f);
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

    nvgTextBounds(vg, gx, gy, txt, NULL, bounds);

    nvgBeginPath(vg);
    nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
    nvgRoundedRect(vg
      , round(bounds[0]) - 4.0f
      , round(bounds[1]) - 2.0f
      , round(bounds[2] - bounds[0]) + 8.0f
      , round(bounds[3] - bounds[1]) + 4.0f
      , (round(bounds[3] - bounds[1]) + 4.0f) / 2.0f - 1.0f
    );
    nvgFill(vg);

    nvgFillColor(vg, nvgRGBA(32, 32, 32, 255));
    nvgText(vg, gx, gy, txt, NULL);
  }

  y += 20.0f;

  nvgFontSize(vg, 13.0f);
  nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  nvgTextLineHeight(vg, 1.2f);

  nvgTextBoxBounds(vg, x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL, bounds);

  // Fade the tooltip out when close to it.
  gx = abs((mx - (bounds[0] + bounds[2]) * 0.5f) / (bounds[0] - bounds[2]));
  gy = abs((my - (bounds[1] + bounds[3]) * 0.5f) / (bounds[1] - bounds[3]));
  a = max(gx, gy) - 0.5f;
  a = clamp(a, 0.0f, 1.0f);
  nvgGlobalAlpha(vg, a);

  nvgBeginPath(vg);
  nvgFillColor(vg, nvgRGBA(220, 220, 220, 255));
  nvgRoundedRect(vg
    , round(bounds[0] - 2.0f)
    , round(bounds[1] - 2.0f)
    , round(bounds[2] - bounds[0]) + 4.0f
    , round(bounds[3] - bounds[1]) + 4.0f
    , 3.0f
  );
  px = float((int)((bounds[2] + bounds[0]) / 2));
  nvgMoveTo(vg, px, bounds[1] - 10);
  nvgLineTo(vg, px + 7, bounds[1] + 1);
  nvgLineTo(vg, px - 7, bounds[1] + 1);
  nvgFill(vg);

  nvgFillColor(vg, nvgRGBA(0, 0, 0, 220));
  nvgTextBox(vg, x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL);

  nvgRestore(vg);
}
#pragma endregion

int main() {
  const int winWidth = 800, winHeight = 600;
  GLFWwindow* window = initWindow(winWidth, winHeight, "Test rmgui Controls");
  if (!window) return -1;

  glfwSwapInterval(1);

  NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!vg) {
    std::cerr << "Failed to create NanoVG context\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  rm_surface* gui = new rm_surface(vg, winWidth, winHeight, &instance);
  rm_font default_font = gui->load_font("Gogh_Regular.ttf", "default");
  if (!default_font.is_valid()) {
    printf("can't load font!\n");
  }

  rm_image image_pat = gui->load_image("ipat.png", NVG_IMAGE_REPEATX);
  if (!image_pat.is_valid()) {
    printf("can't load image\n");
  }

  gui->set_font(default_font);
  g_gui = gui;

  static rm_window_style default_style;
  default_style.apply_defaults();
  rm_window* pwindow = new rm_window(gui, 10, 10, 500, 500);
  pwindow->set_style(&default_style);
  pwindow->set_zindex(-1);

  rm_image_button* imgButton = new rm_image_button(pwindow, 50, 50, 200, 40, "idle-button-login.png");
  rm_button* textButton = new rm_button(pwindow, 300, 50, 200, 100, "Test Button");
  rm_label* label = new rm_label(pwindow, 50, 200, "this is rm_label");

  rm_text_input* textInput = new rm_text_input(pwindow, 300, 200, 200, 40, RMGUI_TEXT_INPUT_SINGLELINE);

  static rm_checkbox_style style;
  style.set_font_size(10.f);
  style.set_background_color(nvgRGB(20, 20, 20));
  style.set_border_color(nvgRGB(80, 80, 80));
  style.set_mark_color(nvgRGB(111, 111, 255));
  style.set_corner_radius(LEFT_TOP, 4.f);
  style.set_corner_radius(RIGHT_TOP, 4.f);
  style.set_corner_radius(RIGHT_BOTTOM, 4.f);
  style.set_corner_radius(LEFT_BOTTOM, 4.f);
  rm_checkbox* checkbox = new rm_checkbox(pwindow, 50, 300, 100, &style, u8"рашн текст");
  rm_checkbox* checkbox2 = new rm_checkbox(pwindow, 150, 300, 100, &style, u8"рашн текст");

  rm_combobox* combobox = new rm_combobox(pwindow, 300, 300, 200, 40);
  combobox->add_item("Item 1");
  combobox->add_item("Item 2");
  combobox->add_item("Item 3");

  rm_slider* slider = new rm_slider(pwindow, 50, 400, 400, 40, 0.0f, 100.0f, 50.0f,
    [](rm_slider *psilder) {
      printf("slider value changed: %f\n", psilder->get_value());
    }
  );
  rm_progress_base* progress = new rm_progress_base(pwindow, 50, 350, 300, 10, 0.f, 5.f);
  rm_progress_image* progress2 = new rm_progress_image(pwindow, 50, 350 + 10 + 5, 300, 10, image_pat, 0.f, 1.f, 0.f, 5.f);
 
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetKeyCallback(window, key_callback);

  glDisable(GL_DEPTH_TEST);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    float percent = fabsf(sinf(instance.get_time())) * 100.f;
    progress->set_percent(percent);
    progress2->set_percent(slider->get_value());

    gui->resize(fbWidth, fbHeight);
    gui->rebuild_draw_cache();
    gui->draw();

#if 0
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    nvgBeginFrame(vg, w, h, 1.f);
    drawBlendish(vg, 0, 0, 400, 400, instance.get_time());
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    drawParagraph(vg, 10, 10, w, h, (float)mx, (float)my);
    nvgEndFrame(vg);
#endif
    glfwSwapBuffers(window);
  }

  delete imgButton;
  delete textButton;
  delete label;
  delete textInput;
  delete checkbox;
  delete combobox;
  delete slider;
  delete gui;
  nvgDeleteGL3(vg);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
