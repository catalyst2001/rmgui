#include "backend/rmgui_glfw_opnegl33.h"
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
  a = rm_max(gx, gy) - 0.5f;
  a = rm_clamp(a, 0.0f, 1.0f);
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
   
  rm_slider* slider = new rm_slider(pwindow, 50, 400, 400, 40, nullptr, 0.0f, 100.0f, 50.0f,
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
    debug_widget(rm_widget *pparent) : rm_widget(0, 0, 800, 800, pparent, "debug_widget") {}
    void on_draw(NVGcontext* pctx) override {
      /*rm_vec2 pos(50.f, 50.f);
      rm_vec2 size(100.f, 100.f);
      rm_color shadow_color(0, 0, 0, 255);
      rm_utl::draw_shadow(pctx, pos, size, rm_vec2(0.f, 1.0f), 3.f, shadow_color, 5.f, 5.f);*/

    }
  } dbg_widget(gui);

  rm_menu* pmenu = new rm_menu(gui, 20, "");
  rm_menu* psubmenu0 = pmenu->create_submenu("File", 0, 0);
  pmenu->create_submenu("Settings", 1, 0);
  pmenu->create_submenu("Elements", 2, 0);
  pmenu->create_submenu("Control", 3, 0);

  psubmenu0->add_item("Open project", 0);
  psubmenu0->add_item("Close project", 0);
  psubmenu0->add_separator();
  psubmenu0->add_item("Create project", 0);


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

  rm_tabcontrol_ex* ptabctl = new rm_tabcontrol_ex(gui, 0, 105, 800, 600, TCF_NONE, TC_DEFAULT, &tabstyle);
  tc::tab* ptab01 = ptabctl->add_tab("Main page", 0, 10);

  static rm_window_style wstyle;
  rm_window *pwindow = new rm_window(ptab01->get_page_widget(), 0, 0, 300, 300);
  pwindow->set_style(&wstyle);


  tc::tab* ptab11 = ptabctl->add_tab("Main page asdasda", 0, 10);
  tc::tab* ptab12 = ptabctl->add_tab("Page 2 asdasdasd", 1, 10);

  rm_flexbox_layout* pflexlayout = new rm_flexbox_layout();
  pflexlayout->set_dir(rm_flex_direction::Column);
  pflexlayout->set_paddings(10.f);
  pflexlayout->set_margins(0.f);
  pflexlayout->set_align_content(rm_flex_align::Center);
  pflexlayout->set_align_items(rm_flex_align::Center);
  pflexlayout->set_gap_main(10.f);
  pflexlayout->set_gap_cross(10.f);
  pflexlayout->set_justify(rm_flex_justify::SpaceEvenly);
  pflexlayout->set_wrap(rm_flex_wrap::NoWrap);
  pflexlayout->set_fill_x(rm_flex_fill::Clamp);
  pflexlayout->set_fill_y(rm_flex_fill::Clamp);

  rm_window* pdiv = new rm_window(ptab11->get_page_widget(), 10, 10, 200, 300);
  pdiv->set_style(&wstyle);
  pdiv->set_layout(pflexlayout);

  static rm_checkbox_style checkstyle;
  rm_checkbox *pcb = new rm_checkbox(pdiv, 10, 100, 200, &checkstyle, "This is checkbox",
    [](rm_checkbox* pcheckbox) {
      printf("checkbox is %s\n", pcheckbox->is_checked() ? "checked" : "unchecked");
      return true;
    });
  ptabctl->select_tab(0, 0);

  static rm_radiobutton_style style_rb;
  style_rb.set_all_corners_radius(8.f);
  style_rb.set_border_width_inner(3.f);
  style_rb.set_border_width_outer(1.5f);
  style_rb.set_border_active_outer(nvgRGB(57, 76, 195));
  style_rb.set_border_active_inner(nvgRGB(40, 60, 196));
  style_rb.set_border_inactive(nvgRGB(255, 255, 255));
  style_rb.set_border_width_inactive(1.5f);
  style_rb.set_bg_inner(nvgRGBA(33, 36, 71, 68.85f));
  style_rb.set_circle_radius(9.5f);

  new rm_radiobutton(pdiv, 10, 10, 150, 25, &style_rb, "Holding",
    [](rm_radiobutton* rb) {
      return true;
    });

  new rm_radiobutton(pdiv, 10, 70, 150, 25, &style_rb, "Always",
    [](rm_radiobutton* rb) {
      return true;
    });
  //rm_radiobutton* first = rm_radiobutton::get_groups()[pdiv][0];
  //first->set_allow_uncheck(true);
  rm_radiobutton::select_default(pdiv, 0);


  static rm_switch_style style_switch;
  style_switch.set_track_height(30.f);
  style_switch.set_padding(4.f);
  style_switch.set_track_on(nvgRGB(53, 77, 230));
  style_switch.set_track_off(nvgRGB(28, 41, 103));
  style_switch.set_knob_color(nvgRGB(255, 255, 255));
  style_switch.set_anim_time(0.25f);
  //style_switch->set_shadow_size(12.f);
  style_switch.set_all_corners_radius(style_switch.get_track_height() * 0.5f);

  rm_switch* sw = new rm_switch(pdiv, 10, 120, 60, &style_switch, false,
    [](rm_switch* sw) {
      //if (sw->is_on())
      //  printf("switch enabled \n");
    });

  //sw->set_on(true, 1);

  static rm_slider_style style_slider;
  style_slider.set_track_height(7.f);
  style_slider.set_padding(9.8f);
  style_slider.set_track_bg(nvgRGB(109, 119, 213));
  style_slider.set_track_fill(nvgRGB(53, 79, 206));
  style_slider.set_thumb_radius(7.f);
  style_slider.set_thumb_color(nvgRGB(255, 255, 255));
  style_slider.set_thumb_border_color(nvgRGB(57, 76, 195));
  style_slider.set_thumb_border_width(3.5f);
  style_slider.set_all_corners_radius(style_slider.get_track_height() * 0.5f);
  rm_slider* slider = new rm_slider(pdiv, 10, 160, 400, 40, &style_slider, 0.0f, 100.0f, 50.0f,
    [](rm_slider* psilder) {
      //psilder->get_userptr<rm_output_text>()->printf("slider value changed: %f", psilder->get_value());
    }
  );

  static rm_listview_style lv_style;
  lv_style.set_row_height(30.f);
  lv_style.set_text_padding(12.f);
  lv_style.set_background_color(nvgRGB(250, 250, 250));
  lv_style.set_text_color(nvgRGB(30, 30, 30));
  lv_style.set_hover_color(nvgRGB(230, 230, 255));
  lv_style.set_selected_color(nvgRGB(180, 200, 255));
  lv_style.set_font_size(14.f);

  rm_listview *list = new rm_listview(pdiv, 20, 210, 200, 150, &lv_style,
    [](rm_listview* lv, size_t idx) {
      printf("Selected item #%zu: %s\n", idx, lv->get_selected_index() == idx ? lv->get_items()[idx].c_str() : "");
    }
  );

  list->add_item("First option");
  list->add_item("Second option");
  list->add_item("Third option");
  list->add_item("Another item");

  /* performing layout */
  pdiv->perform_layout();
}

int main() {
  //testtrb();
  //return 0;

  g_gui = create_window(-1, -1, 1200, 700, "test window");
  if (!g_gui)
    return 1;

  set_vsync(g_gui);

  rm_font default_font = g_gui->load_font("Verdana.ttf", "default");
  if (!default_font.is_valid()) {
    printf("can't load font!\n");
  }

  rm_image image_pat = g_gui->load_image("ipat.png", NVG_IMAGE_REPEATX);
  if (!image_pat.is_valid()) {
    printf("can't load image\n");
  }

  g_gui->set_font(default_font);
  example_widgets(g_gui);

  glDisable(GL_DEPTH_TEST);

  float last_time = 0.f;
  float current_time = 1.f;
  while (!glfwWindowShouldClose(g_gui->get_syswindow<GLFWwindow*>())) {
    glfwPollEvents();
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(g_gui->get_syswindow<GLFWwindow*>(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(34/255.f, 32 / 255.f, 33 / 255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    last_time = current_time;
    current_time = g_gui->get_sysdf()->get_time();
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
    glfwSwapBuffers(g_gui->get_syswindow<GLFWwindow*>());
  }

  destroy_window(g_gui);

  return 0;
}
