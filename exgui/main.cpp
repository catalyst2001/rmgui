#include <glad.h>
#include <glfw3.h>
#include "rmgui.h"
#include "rmgui_controls.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include <iostream>
#include <vector>
#include "blendish_test.h"

#define NOMINMAX
#include <Windows.h> //for Sleep
#define sleep(ms) Sleep(ms)

static rm_surface* g_gui = nullptr;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  g_gui->mouse(RM_MOUSE_EVENT_MOVE, RM_KEY_NONE, DOWN, (int)xpos, (int)ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    RM_KEY_STATE state = (action == GLFW_PRESS ? DOWN : UP);
    g_gui->mouse(RM_MOUSE_EVENT_CLICK, RM_KEY_NONE, state, (int)xpos, (int)ypos);
  }
}
// FIXME: d2 move to utils 
RM_KEY translate_glfw_key(int k) {
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

void char_callback(GLFWwindow* window, unsigned int codepoint) {
  g_gui->textinput(codepoint);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  RM_KEY vk = translate_glfw_key(key);
  RM_KEY_STATE state = (action == GLFW_PRESS ? DOWN : action == GLFW_RELEASE ? UP : REPEAT);
  g_gui->keybd(scancode, vk, state);
}

GLFWwindow* initWindow(int width, int height, const char* title) {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return nullptr;
  }

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
    const char *pstring = glfwGetClipboardString(nullptr);
    size = strlen(pstring);
    return (uint8_t*)pstring;
  }

  virtual void set_clipboard_data_ex(const uint8_t* p_src, size_t size) {
    glfwSetClipboardString(nullptr, (const char *)p_src);
  }

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

void testtrb()
{
  auto gen_rand_string = []() -> const char* {
    size_t i;
    const size_t arrlen = 32+1;
    static char buf[arrlen];
    for (i = 0; i < arrlen-1; i++)
      buf[i] = 64 + (rand() % 90);
    buf[i] = 0;
    return buf;
  };

  rm_line_ring_buffer ringbuf(32, 512, 32);
  while (1) {
    ringbuf.append_text(gen_rand_string());
    for (size_t i = 0; i < ringbuf.get_num_lines(); i++) {
      printf("[%zd] %s\n", i+1, ringbuf.get_output_line(i)->get_cstr());
    }
    sleep(1);
    system("cls");
  }
}

void test_old(rm_surface* gui)
{
  static rm_window_style default_style;
  default_style.apply_defaults();
  rm_window* pwindow = new rm_window(gui, 20, 20, 1024, 768);
  pwindow->set_style(&default_style);
  pwindow->set_zindex(-1);

  rm_image_button* imgButton = new rm_image_button(pwindow, 20, 20, 200, 40, "idle-button-login.png");
  rm_button* textButton = new rm_button(pwindow, 200 + 20 + 10, 20, 200, 40, "Test Button");
  rm_label* label = new rm_label(pwindow, 20, 40 + 30, "this is rm_label");

  static rm_text_input_style style_inp;
  //style_inp.set_text_offsets(10.0f);
  style_inp.set_all_corners_radius(4.f);
  style_inp.set_border_color(nvgRGB(0, 0, 255));
  style_inp.set_border_width(2.f);
  style_inp.set_rounded_selection(0);
  //style_inp.set_active_bgr_color({ 0,0,0 });
  //style_inp.set_blink_width(1.f);

  //RMGUI_TEXT_INPUT_MULTILINE RMGUI_TEXT_INPUT_SINGLELINE
  rm_text_input* textInput = new rm_text_input(pwindow, 300, 100, 200, 20, &style_inp, RMGUI_TEXT_INPUT_SINGLELINE);

  static rm_checkbox_style style;
  style.set_font_size(14.f);
  style.set_background_color(nvgRGB(20, 20, 20));
  style.set_border_color(nvgRGB(80, 80, 80));
  style.set_mark_color(nvgRGB(111, 111, 255));
  style.set_border_width(1.f);

  //style.set_corner_radius(LEFT_TOP, 4.f);
  //style.set_corner_radius(RIGHT_TOP, 4.f);
  //style.set_corner_radius(RIGHT_BOTTOM, 4.f);
  //style.set_corner_radius(LEFT_BOTTOM, 4.f);
  rm_checkbox* checkbox = new rm_checkbox(pwindow, 20, 40 + 30 + 20, 100, &style, "Enable",
    [](rm_checkbox* pcheckbox) -> bool {
      pcheckbox->get_userptr<rm_output_text>()->printf("%s time output", pcheckbox->is_checked() ? "Enabled" : "Disabled");
      return true;
    }
  );

  rm_checkbox* checkbox2 = new rm_checkbox(pwindow, 150, 40 + 30 + 20, 100, &style, u8"Включить");

  static rm_tabcontrol_style tabcontrol_style;
  tabcontrol_style.set_all_corners_radius(4.f);
  tabcontrol_style.set_horizontal(!!true);
  tabcontrol_style.set_tab_height(30);
  rm_tabcontrol* tabs = new rm_tabcontrol(pwindow, 10, 190, 300, 300,
    [](rm_tabcontrol* ctrl, rm_tab_item* item, size_t idx) {
      printf("Tab %zu active: %s\n", idx, item->get_name());
    }
  );
  tabs->set_style(&tabcontrol_style);

  rm_widget* t0 = tabs->add_tab("Home");
  t0->get_content_area().y = 10;

  rm_widget* t1 = tabs->add_tab("Settings");
  t1->get_content_area().y = 100;

  rm_widget* t2 = tabs->add_tab("Test1");
  t2->get_content_area().y = 100;

  rm_label* home_label = new rm_label(t0, 0, 0, "Welcome to the Home tab");

  rm_button* home_btn = new rm_button(t0, 10, 25, 120, 30, "Home Action");
  rm_checkbox* setting_chk = new rm_checkbox(t1, 10, 0, 150, &style, "Enable Feature");
  //rm_text_input* setting_input = new rm_text_input(t1, 10, 25, 200, 20, RMGUI_TEXT_INPUT_SINGLELINE);

  rm_vec2& size = pwindow->get_size();
  rm_output_text* potext = new rm_output_text(pwindow,
    1.f, size.y - 200.f - 10.f, pwindow->get_size().x, 200.f);
#if 0
  rm_treeview* tree = new rm_treeview(290, 230, 200, 200, pwindow,
    [](rm_treeview* tv, rm_tree_node* node) {
      printf("Selected node: %s\n", node->name.c_str());
    }
  );
  rm_tree_node* root1 = tree->add_root("Root 1");
  rm_tree_node* root2 = tree->add_root("Root 2");
  root1->add_child("Child 1.1");
  rm_tree_node* sub = root1->add_child("Child 1.2");
  sub->add_child("Child 1.2.1");
  root2->add_child("Child 1.2");
  root1->expanded = true;
#endif
  checkbox->set_userptr(potext);

  //rm_animation* anim = new rm_animation(pwindow, 20, 100, 100, 100, image_pat);
  //anim->set_speed(8.f);
  //anim->set_scale(0.5f);

  //rm_animation* anim2 = new rm_animation(pwindow, 20 + 100, 100, 100, 100, image_pat);
  //anim2->set_speed(-8.f);
  //anim2->set_scale(0.5f);

  rm_combobox* combobox = new rm_combobox(pwindow, 300, 100 + 50, 200, 20);
  combobox->add_item("Item 1");
  combobox->add_item("Item 2");
  combobox->add_item("Item 3");

  //static rm_scroll_style scrollbar_style;
  //scrollbar_style.set_scroll_corner_round(1.f);
  //scrollbar_style.set_scroll_thumb_color(nvgRGB(90, 90, 90));
  //scrollbar_style.set_thumb_size(10);
  //rm_scrollbar* pscroll = new rm_scrollbar(pwindow, RM_ORIENT_VERT, &scrollbar_style);
   
  rm_slider* slider = new rm_slider(pwindow, 50, 400, 400, 40, 0.0f, 100.0f, 50.0f,
    [](rm_slider* psilder) {
      psilder->get_userptr<rm_output_text>()->printf("slider value changed: %f", psilder->get_value());
    }
  );
  slider->set_userptr(potext);

  rm_progress_base* progress = new rm_progress_base(pwindow, 50, 350, 300, 10, 0.f, 5.f);
  //rm_progress_image* progress2 = new rm_progress_image(pwindow, 50, 350 + 10 + 5, 300, 10, image_pat, 0.f, 1.f, 0.f, 5.f);
}

void example_widgets(rm_surface* gui)
{
  static class debug_widget : public rm_widget, rm_tab_drawer {
  public:
    debug_widget(rm_widget *pparent) : rm_widget(20, 20, 800, 800, pparent, "debug_widget") {}
    void on_draw(NVGcontext* pctx) override {
      draw_tab_path(pctx, 0.f, 0.f, 100.f, 20.f, rm_vec2(0.f, 0.f), 4.f);
      nvgFillColor(pctx, nvgRGB(93, 93, 95));
      nvgStrokeWidth(pctx, 1.f);
      nvgStrokeColor(pctx, nvgRGB(102, 102, 104));
      nvgFill(pctx);
      nvgStroke(pctx);
    }
  };// dbg_widget(gui);


  static rm_tabcontrol_ex_style tabstyle;
  using tc = rm_tabcontrol_ex;
  tabstyle.set_text_color({ 255, 255, 255 });
  tabstyle.set_background_color({66, 66, 68});
  tabstyle.set_selected_color({93, 93, 95});
  tabstyle.set_text_offsets({ 10.f, 0.f });
  tabstyle.set_border_color({ 91, 91, 91 });
  tabstyle.set_all_corners_radius(0.f);
  tabstyle.set_tab_height(25.f);
  tabstyle.set_tab_corners_radius(2.f);
  tabstyle.set_tab_up_offsets({ 0.f, 0.f });

  rm_tabcontrol_ex* ptabctl = new rm_tabcontrol_ex(gui, 10, 10, 800, 600, TCF_NONE, TC_DEFAULT, &tabstyle);
  tc::tab* ptab01 = ptabctl->add_tab("Main page", 0, 10);
  tc::tab* ptab02 = ptabctl->add_tab("Page 2", 1, 10);
  tc::tab* ptab11 = ptabctl->add_tab("Main page asdasda", 0, 10);
  tc::tab* ptab12 = ptabctl->add_tab("Page 2 asdasdasd", 1, 10);
  

  static rm_checkbox_style checkstyle;
  rm_checkbox *pcb = new rm_checkbox(ptab12->get_page_widget(), 10, 100, 200, &checkstyle, "Checkbox on page 0",
    [](rm_checkbox* pcheckbox) {
      printf("checkbox is %s\n", pcheckbox->is_checked() ? "checked" : "unchecked");
      return true;
    });
  ptabctl->select_tab(0, 0);

}

int main() {
  //testtrb();
  //return 0;

  const int winWidth = 1280, winHeight = 1024;
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

  g_gui = new rm_surface(vg, winWidth, winHeight, &instance);
  rm_font default_font = g_gui->load_font("ARIALNI.TTF", "default");
  if (!default_font.is_valid()) {
    printf("can't load font!\n");
  }

  rm_image image_pat = g_gui->load_image("ipat.png", NVG_IMAGE_REPEATX);
  if (!image_pat.is_valid()) {
    printf("can't load image\n");
  }

  g_gui->set_font(default_font);
  example_widgets(g_gui);

  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetKeyCallback(window, key_callback);

  glDisable(GL_DEPTH_TEST);

  float last_time = 0.f;
  float current_time = 1.f;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(34/255.f, 32 / 255.f, 33 / 255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    last_time = current_time;
    current_time = instance.get_time();
    float dt = current_time - last_time;
    float sinabs = fabsf(sinf(current_time));
    float percent = sinabs * 100.f;
    g_gui->resize(fbWidth, fbHeight);
    g_gui->draw(dt);

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
  delete g_gui;
  nvgDeleteGL3(vg);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
