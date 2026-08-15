#include "backend/rmgui_glfw_opnegl33.h"
#include "rmgui_controls.h"
#include "rm_effects.h"
#if defined(RMGUI_ENABLE_BLENDISH_DEMO)
#include "blend_ui.h"
#include "blendish_test.h"
#endif
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include <iostream>
#include <cmath>

#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h> //for Sleep
#ifdef RGB
#undef RGB
#endif
#define sleep(ms) Sleep(ms)

static rm_surface* g_gui = nullptr;

class rm_theme_preview_panel : public rm_widget {
  RmThemeRef m_theme;

public:
  rm_theme_preview_panel(rm_widget* p_parent, int x, int y, int width, int height,
    RmThemeRef p_theme) :
    rm_widget(x, y, width, height, p_parent, "rm_theme_preview_panel"),
    m_theme(p_theme ? std::move(p_theme) : RmThemeSnapshot::default_theme()) {
  }

  void on_draw(NVGcontext* pctx) override {
    const RmThemeTokens& tokens = m_theme->tokens;
    pctx->beginPath();
    pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, tokens.radius.large);
    pctx->fillColor(tokens.colors.surface);
    pctx->fill();

    pctx->beginPath();
    pctx->roundedRect(0.5f, 0.5f, m_size.x - 1.f, m_size.y - 1.f,
      tokens.radius.large);
    pctx->strokeColor(tokens.colors.border);
    pctx->StrokeWidth(tokens.controls.border_width);
    pctx->stroke();

    rm_widget::on_draw(pctx);
  }
};

static void create_theme_preview(rm_widget* p_parent, int x, int y,
  const RmThemeDocument& p_document) {
  const RmThemeCompileResult compile_result = RmThemeCompiler::compile(p_document);
  const RmThemeRef theme = compile_result.theme;
  rm_theme_preview_panel* ppanel = new rm_theme_preview_panel(
    p_parent, x, y, 350, 465, theme);

  new rm_label(ppanel, 20, 18, p_document.name, theme);
  new rm_label(ppanel, 20, 46, "Button variants", theme);
  new rm_button(ppanel, 20, 72, 96, 34, "Primary", theme,
    RmButtonVariant::primary);
  new rm_button(ppanel, 127, 72, 96, 34, "Secondary", theme,
    RmButtonVariant::secondary);
  new rm_button(ppanel, 234, 72, 96, 34, "Outline", theme,
    RmButtonVariant::outline);
  new rm_button(ppanel, 20, 114, 96, 34, "Subtle", theme,
    RmButtonVariant::subtle);
  new rm_button(ppanel, 127, 114, 96, 34, "Delete", theme,
    RmButtonVariant::destructive);
  rm_button* pdisabled_button = new rm_button(
    ppanel, 234, 114, 96, 34, "Disabled", theme);
  pdisabled_button->set_enabled(false);

  new rm_label(ppanel, 20, 164, "Selection", theme);
  rm_checkbox* pcheckbox = new rm_checkbox(
    ppanel, 20, 190, 160, "Remember choice", nullptr, theme);
  pcheckbox->set_checked(true);
  rm_checkbox* pdisabled_checkbox = new rm_checkbox(
    ppanel, 185, 190, 145, "Unavailable", nullptr, theme);
  pdisabled_checkbox->set_enabled(false);

  new rm_label(ppanel, 20, 232, "Toggle", theme);
  rm_switch* pswitch = new rm_switch(ppanel, 20, 260, 58, true, nullptr, theme);
  pswitch->set_on(true, false);
  rm_switch* pdisabled_switch = new rm_switch(
    ppanel, 98, 260, 58, false, nullptr, theme);
  pdisabled_switch->set_enabled(false);

  new rm_label(ppanel, 20, 310, "Value", theme);
  new rm_slider(ppanel, 20, 336, 310, 34, 0.f, 100.f, 64.f, nullptr, theme);
  new rm_progress(ppanel, 20, 392, 310, 12, 0.68f, theme);
  new rm_label(ppanel, 20, 424, "Theme tokens compile into immutable styles", theme);
}

static rm_tabcontrol* create_tabs_preview(rm_widget* p_parent, int x, int y,
  int width, int height, const char* p_title, RmThemeRef theme,
  RmTabVariant variant, RmTabPlacement placement)
{
  rm_tabcontrol* ptabs = new rm_tabcontrol(p_parent, x, y, width, height,
    nullptr, theme, variant, placement);
  rm_widget* pfirst = ptabs->add_tab("Overview", 10, false, true);
  rm_widget* psecond = ptabs->add_tab("Source.cpp", 11, true);
  rm_widget* pthird = ptabs->add_tab("Properties", 12, true);
  new rm_label(pfirst, 18, 18, p_title, theme);
  new rm_label(psecond, 18, 18, "Closable document page", theme);
  new rm_label(pthird, 18, 18, "Placement and appearance are independent", theme);
  return ptabs;
}

#pragma region TEMPLATE1
void drawParagraph(NVGcontext* vg, float x, float y, float width, float height, float mx, float my)
{
  struct NVGtextRow rows[3];
  struct NVGglyphPosition glyphs[100];
  const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.";
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

  vg->save();

  vg->setFontSize( 18.0f);
  vg->setFontFace( "default");
  vg->setTextAlign( NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  vg->textMetrics( NULL, NULL, &lineh);

  // The text break API can be used to fill a large buffer of rows,
  // or to iterate over the text just few lines (or just one) at a time.
  // The "next" variable of the last returned item tells where to continue.
  start = text;
  end = text + strlen(text);
  for (;;) {
    nrows = vg->textBreakLines(start, end, width, rows, 3);
    if (nrows <= 0) {
      break;
    }
    for (i = 0; i < nrows; i++) {
      struct NVGtextRow* row = &rows[i];
      int hit = mx > x && mx < (x + width) && my >= y && my < (y + lineh);

      vg->beginPath();
      vg->fillColor( NVGcolor::RGBA(255, 255, 255, hit ? 64 : 16));
      vg->rect( x, y, row->width, lineh);
      vg->fill();

      vg->fillColor( NVGcolor::RGBA(255, 255, 255, 255));
      vg->text( x, y, row->start, row->end);

      if (hit) {
        caretx = (mx < x + row->width / 2) ? x : x + row->width;
        px = x;
        nglyphs = vg->textGlyphPositions( x, y, row->start, row->end, glyphs, 100);
        for (j = 0; j < nglyphs; j++) {
          float x0 = glyphs[j].x;
          float x1 = (j + 1 < nglyphs) ? glyphs[j + 1].x : x + row->width;
          float tgx = x0 * 0.3f + x1 * 0.7f;
          if (mx >= px && mx < tgx)
            caretx = glyphs[j].x;
          px = tgx;
        }
        vg->beginPath();
        vg->fillColor( NVGcolor::RGBA(255, 192, 0, 255));
        vg->rect( caretx, y, 1, lineh);
        vg->fill();

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
    vg->setFontSize( 13.0f);
    vg->setTextAlign( NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);

    vg->textBounds( gx, gy, txt, NULL, bounds);

    vg->beginPath();
    vg->fillColor( NVGcolor::RGBA(255, 192, 0, 255));
    vg->roundedRect( round(bounds[0]) - 4.0f
      , round(bounds[1]) - 2.0f
      , round(bounds[2] - bounds[0]) + 8.0f
      , round(bounds[3] - bounds[1]) + 4.0f
      , (round(bounds[3] - bounds[1]) + 4.0f) / 2.0f - 1.0f
    );
    vg->fill();

    vg->fillColor( NVGcolor::RGBA(32, 32, 32, 255));
    vg->text( gx, gy, txt, NULL);
  }

  y += 20.0f;

  vg->setFontSize( 13.0f);
  vg->setTextAlign( NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  vg->setTextLineHeight( 1.2f);

  vg->textBoxBounds( x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL, bounds);

  // Fade the tooltip out when close to it.
  gx = abs((mx - (bounds[0] + bounds[2]) * 0.5f) / (bounds[0] - bounds[2]));
  gy = abs((my - (bounds[1] + bounds[3]) * 0.5f) / (bounds[1] - bounds[3]));
  a = rm_max(gx, gy) - 0.5f;
  a = rm_clamp(a, 0.0f, 1.0f);
  vg->GlobalAlpha( a);

  vg->beginPath();
  vg->fillColor( NVGcolor::RGBA(220, 220, 220, 255));
  vg->roundedRect( round(bounds[0] - 2.0f)
    , round(bounds[1] - 2.0f)
    , round(bounds[2] - bounds[0]) + 4.0f
    , round(bounds[3] - bounds[1]) + 4.0f
    , 3.0f
  );
  px = float((int)((bounds[2] + bounds[0]) / 2));
  vg->moveTo( px, bounds[1] - 10);
  vg->lineTo( px + 7, bounds[1] + 1);
  vg->lineTo( px - 7, bounds[1] + 1);
  vg->fill();

  vg->fillColor( NVGcolor::RGBA(0, 0, 0, 220));
  vg->textBox( x, y, 150, "Hover your mouse over the text to see calculated caret position.", NULL);

  vg->restore();
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

void example_core_widgets(rm_surface* gui)
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
  style_inp.set_border_color(NVGcolor::RGB(0, 0, 255));
  style_inp.set_border_width(2.f);
  style_inp.set_rounded_selection(0);
  //style_inp.set_active_bgr_color({ 0,0,0 });
  //style_inp.set_blink_width(1.f);

  //RMGUI_TEXT_INPUT_MULTILINE RMGUI_TEXT_INPUT_SINGLELINE
  rm_text_input* textInput = new rm_text_input(pwindow, 300, 100, 200, 20, &style_inp, RMGUI_TEXT_INPUT_SINGLELINE);

  RmThemeDocument checkbox_theme_document = RmThemeDocument::dark_theme();
  checkbox_theme_document.name = "Compact checkbox demo";
  checkbox_theme_document.tokens.typography.control = 14.f;
  checkbox_theme_document.tokens.colors.control = NVGcolor::RGB(20, 20, 20);
  checkbox_theme_document.tokens.colors.border = NVGcolor::RGB(80, 80, 80);
  checkbox_theme_document.tokens.colors.accent = NVGcolor::RGB(111, 111, 255);
  checkbox_theme_document.tokens.controls.border_width = 1.f;
  const RmThemeRef checkbox_theme = RmThemeCompiler::compile(checkbox_theme_document).theme;

  rm_checkbox* checkbox = new rm_checkbox(pwindow, 20, 40 + 30 + 20, 100, "Enable",
    [](rm_checkbox* pcheckbox) -> bool {
      pcheckbox->get_userptr<rm_output_text>()->printf("%s time output", pcheckbox->is_checked() ? "Enabled" : "Disabled");
      return true;
    }, checkbox_theme
  );

  rm_checkbox* checkbox2 = new rm_checkbox(pwindow, 150, 40 + 30 + 20, 100,
    u8"Включить", nullptr, checkbox_theme);

  rm_tabcontrol* tabs = new rm_tabcontrol(pwindow, 10, 190, 300, 300,
    [](rm_tabcontrol* ctrl, rm_tab_item* item, size_t idx) {
      printf("Tab %zu active: %s\n", idx, item->get_name());
    }
  );

  rm_widget* t0 = tabs->add_tab("Home");
  t0->get_content_area().y = 10;

  rm_widget* t1 = tabs->add_tab("Settings", 1, true);
  t1->get_content_area().y = 100;

  rm_widget* t2 = tabs->add_tab("Test1");
  t2->get_content_area().y = 100;

  rm_label* home_label = new rm_label(t0, 0, 0, "Welcome to the Home tab");

  rm_button* home_btn = new rm_button(t0, 10, 25, 120, 30, "Home Action");
  rm_checkbox* setting_chk = new rm_checkbox(t1, 10, 0, 150,
    "Enable Feature", nullptr, checkbox_theme);
  //rm_text_input* setting_input = new rm_text_input(t1, 10, 25, 200, 20, RMGUI_TEXT_INPUT_SINGLELINE);

  rm_vec2& size = pwindow->get_size();
  rm_output_text* potext = new rm_output_text(pwindow,
    1.0f, size.y - 210.0f, pwindow->get_size().x, 200.0f);
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

  rm_slider* slider = new rm_slider(pwindow, 50, 400, 400, 40, 0.0f, 100.0f, 50.0f,
    [](rm_slider* psilder) {
      psilder->get_userptr<rm_output_text>()->printf("slider value changed: %f", psilder->get_value());
    }
  );
  slider->set_userptr(potext);

  RmThemeDocument progress_theme_document = RmThemeDocument::dark_theme();
  progress_theme_document.name = "Progress demo";
  progress_theme_document.tokens.controls.progress_corner_radius = 5.f;
  const RmThemeRef progress_theme = RmThemeCompiler::compile(progress_theme_document).theme;
  rm_progress* progress = new rm_progress(pwindow, 50, 350, 300, 10, 0.f, progress_theme);
  //rm_progress_image* progress2 = new rm_progress_image(
  //  pwindow, 50, 365, 300, 10, image_pat, 0.f, 1.f, 0.f, progress_theme);
}

void example_widgets(rm_surface* gui)
{
  static class debug_widget : public rm_widget {
  public:
    debug_widget(rm_widget *pparent) : rm_widget(0, 0, 800, 800, pparent, "debug_widget",
      RM_FLAG_DEFAULT, 0, nullptr, RmChildOwnership::borrowed) {}
    void on_draw(NVGcontext* pctx) override {
      /*rm_vec2 pos(50.f, 50.f);
      rm_vec2 size(100.f, 100.f);
      rm_color shadow_color(0, 0, 0, 255);
      rm_utl::draw_shadow(pctx, pos, size, rm_vec2(0.f, 1.0f), 3.f, shadow_color, 5.f, 5.f);*/

    }
  } dbg_widget(gui);

  rm_menu* pmenu = new rm_menu(gui,
    [](rm_menu*, uint32_t menuid, uint32_t itemid) {
      printf("Menu command: menu=%u item=%u\n", menuid, itemid);
    });
  rm_menu* psubmenu0 = pmenu->create_submenu("File", 0, 0);
  rm_menu* psettings = pmenu->create_submenu("Settings", 1, 0);
  rm_menu* pelements = pmenu->create_submenu("Elements", 2, 0);
  pmenu->create_submenu("Control", 3, 0);

  psubmenu0->add_item("New project", 0);
  psubmenu0->add_item("Open project...", 1);
  rm_menu* precent = psubmenu0->create_submenu("Recent projects", 0, 2);
  precent->add_item("rmgui-demo.rmg", 0);
  precent->add_item("material-editor.rmg", 1);
  precent->add_item("layout-tests.rmg", 2);
  precent->add_separator();
  precent->add_item("Clear recent list", 3);
  psubmenu0->add_separator();
  psubmenu0->add_item("Save", 3);
  psubmenu0->add_item("Save as...", 4);
  psubmenu0->add_separator();
  psubmenu0->add_item("Exit", 5);
  psettings->add_item("Preferences", 0);
  psettings->add_item("Shortcuts", 1);
  rm_menu* pappearance = psettings->create_submenu("Appearance", 1, 2);
  pappearance->add_item("Dark theme", 0);
  pappearance->add_item("Light theme", 1);
  rm_menu* pscale = pappearance->create_submenu("Interface scale", 1, 2);
  pscale->add_item("100%", 0);
  pscale->add_item("125%", 1);
  pscale->add_item("150%", 2);
  rm_menu* ptheme_menu = pelements->create_submenu("Theme", 2, 0);
  ptheme_menu->add_item("Dark", 0);
  ptheme_menu->add_item("Light", 1);


  RmThemeDocument shell_theme_document = RmThemeDocument::dark_theme();
  shell_theme_document.name = "ExGUI shell";
  const RmThemeRef shell_theme = RmThemeCompiler::compile(shell_theme_document).theme;
  rm_tabcontrol* ptabctl = new rm_tabcontrol(gui, 0, 105, 800, 600,
    nullptr, shell_theme, RmTabVariant::underline, RmTabPlacement::top);
  rm_widget* ptab01 = ptabctl->add_tab("Window", 0);

  static rm_window_style wstyle;
  rm_window *pwindow = new rm_window(ptab01, 0, 0, 300, 300);
  pwindow->set_style(&wstyle);

  rm_widget* ptab11 = ptabctl->add_tab("Controls", 1);
  rm_widget* ptab12 = ptabctl->add_tab("Effects", 2);
  rm_widget* ptab_theme = ptabctl->add_tab("Themes", 3);
  rm_widget* ptab_tabs = ptabctl->add_tab("Tabs", 4);
#if defined(RMGUI_ENABLE_BLENDISH_DEMO)
  rm_widget* ptab_bui = ptabctl->add_tab("Blendish Controls", 5);
#endif

  rm_widget* effects_page = ptab12;
  rm_vec2& effects_size = effects_page->get_size();
  const float effects_pad = 12.0f;
  const float effects_w = rm_max(0.0f, effects_size.x - effects_pad * 2.0f);
  const float effects_h = rm_max(0.0f, effects_size.y - effects_pad * 2.0f);
  new rm_effects(effects_page, effects_pad, effects_pad, effects_w, effects_h);

  RmThemeDocument dark_theme_document = RmThemeDocument::dark_theme();
  dark_theme_document.name = "Midnight";
  dark_theme_document.tokens.colors.accent = NVGcolor::RGB(91, 124, 250);
  dark_theme_document.tokens.colors.accent_hovered = NVGcolor::RGB(116, 145, 255);
  dark_theme_document.tokens.colors.accent_pressed = NVGcolor::RGB(70, 98, 220);

  RmThemeDocument light_theme_document = RmThemeDocument::light_theme();
  light_theme_document.name = "Cloud";
  light_theme_document.tokens.colors.accent = NVGcolor::RGB(39, 103, 216);
  light_theme_document.tokens.colors.accent_hovered = NVGcolor::RGB(56, 122, 234);
  light_theme_document.tokens.colors.accent_pressed = NVGcolor::RGB(29, 80, 173);

  rm_widget* ptheme_page = ptab_theme;
  create_theme_preview(ptheme_page, 20, 30, dark_theme_document);
  create_theme_preview(ptheme_page, 410, 30, light_theme_document);

  create_tabs_preview(ptab_tabs, 20, 20, 360, 230, "Document tabs",
    shell_theme, RmTabVariant::document, RmTabPlacement::top);
  create_tabs_preview(ptab_tabs, 400, 20, 360, 230, "Underline / bottom",
    shell_theme, RmTabVariant::underline, RmTabPlacement::bottom);
  create_tabs_preview(ptab_tabs, 20, 280, 360, 230, "Segmented tabs",
    shell_theme, RmTabVariant::segmented, RmTabPlacement::top);
  create_tabs_preview(ptab_tabs, 400, 280, 360, 230, "Tool tabs / left",
    shell_theme, RmTabVariant::tool, RmTabPlacement::left);

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

  rm_window* pdiv = new rm_window(ptab11, 10, 10, 200, 300);
  pdiv->set_style(&wstyle);
  pdiv->set_layout(pflexlayout);

  RmThemeDocument controls_theme_document = RmThemeDocument::dark_theme();
  controls_theme_document.name = "ExGUI control gallery";
  controls_theme_document.tokens.colors.accent = NVGcolor::RGB(53, 77, 230);
  controls_theme_document.tokens.colors.accent_hovered = NVGcolor::RGB(73, 99, 245);
  controls_theme_document.tokens.colors.accent_pressed = NVGcolor::RGB(42, 61, 190);
  controls_theme_document.tokens.colors.control = NVGcolor::RGB(38, 46, 83);
  controls_theme_document.tokens.colors.control_hovered = NVGcolor::RGB(49, 59, 102);
  controls_theme_document.tokens.colors.control_pressed = NVGcolor::RGB(28, 35, 68);
  controls_theme_document.tokens.controls.switch_track_height = 30.f;
  controls_theme_document.tokens.controls.switch_padding = 4.f;
  controls_theme_document.tokens.controls.slider_track_height = 7.f;
  controls_theme_document.tokens.controls.slider_padding = 9.8f;
  controls_theme_document.tokens.controls.slider_thumb_radius = 7.f;
  controls_theme_document.tokens.controls.slider_thumb_border_width = 3.5f;
  controls_theme_document.tokens.controls.combobox_item_height = 32.f;
  controls_theme_document.tokens.controls.combobox_popup_gap = 5.f;
  controls_theme_document.tokens.controls.combobox_indicator_size = 5.f;
  controls_theme_document.tokens.animation.normal = 0.25f;
  const RmThemeRef controls_theme = RmThemeCompiler::compile(controls_theme_document).theme;

  rm_theme_preview_panel* pcombo_panel = new rm_theme_preview_panel(
    ptab11, 240, 10, 360, 260, controls_theme);
  new rm_label(pcombo_panel, 20, 18, "ComboBox states", controls_theme);
  rm_combobox* pcombo = new rm_combobox(pcombo_panel, 20, 54, 320, 36,
    [](rm_combobox*, rm_combo_item* pitem, size_t index) {
      printf("Combobox item #%zu selected: %s\n", index, pitem->get_name());
    }, controls_theme);
  pcombo->add_item("C++ desktop application");
  pcombo->add_item("GUI designer project");
  pcombo->add_item("NanoVG render graph");
  pcombo->add_item("Theme package");
  pcombo->set_selected_index(1);

  rm_combobox* pempty_combo = new rm_combobox(
    pcombo_panel, 20, 112, 320, 36, nullptr, controls_theme);
  pempty_combo->set_placeholder("Choose a workspace preset");

  rm_combobox* pdisabled_combo = new rm_combobox(
    pcombo_panel, 20, 170, 320, 36, nullptr, controls_theme);
  pdisabled_combo->add_item("Unavailable configuration");
  pdisabled_combo->set_enabled(false);
  new rm_label(pcombo_panel, 20, 222,
    "Mouse, arrows, Home/End, Enter, Esc and F4", controls_theme);
  rm_scrollbar* pscrollbar = new rm_scrollbar(
    pcombo_panel, RM_ORIENT_VERT, 0.35f,
    [](rm_scrollbar*, float position) {
      printf("Scrollbar position: %.3f\n", position);
    }, controls_theme);
  pscrollbar->set_content_metrics(900.f, 260.f);

  rm_checkbox *pcb = new rm_checkbox(pdiv, 10, 100, 200, "This is checkbox",
    [](rm_checkbox* pcheckbox) {
      printf("checkbox is %s\n", pcheckbox->is_checked() ? "checked" : "unchecked");
      return true;
    }, controls_theme);

  ptabctl->set_selected_index(1);

  new rm_radiobutton(pdiv, 10, 10, 150, 30, "Holding",
    [](rm_radiobutton* rb) {
      return true;
    }, controls_theme);

  new rm_radiobutton(pdiv, 10, 70, 150, 30, "Always",
    [](rm_radiobutton* rb) {
      return true;
    }, controls_theme);
  rm_radiobutton::select_default(pdiv, 0);

  rm_switch* sw = new rm_switch(pdiv, 10, 120, 60, false,
    [](rm_switch* sw) {
      //if (sw->is_on())
      //  printf("switch enabled \n");
    }, controls_theme);
  sw->set_max_size({ 60.f, sw->get_size().y });

  //sw->set_on(true, 1);

  rm_slider* slider = new rm_slider(pdiv, 10, 160, 200, 40, 0.0f, 100.0f, 50.0f,
    [](rm_slider* psilder) {
      //psilder->get_userptr<rm_output_text>()->printf("slider value changed: %f", psilder->get_value());
    }, controls_theme
  );
  slider->set_max_size({ 200*2, 40 });

  rm_listview *list = new rm_listview(pdiv, 20, 210, 200, 150,
    [](rm_listview* lv, size_t idx) {
      printf("Selected item #%zu: %s\n", idx, lv->get_selected_index() == idx ? lv->get_items()[idx].c_str() : "");
    }, controls_theme
  );
  list->set_max_size({ 200 * 2, 150 * 2 });

  list->add_item("First option");
  list->add_item("Second option");
  list->add_item("Third option");
  list->add_item("Another item");

#if defined(RMGUI_ENABLE_BLENDISH_DEMO)
  // Isolated Blendish visualization experiment. Kept out of the default build.
  {
    rm_widget* bui_page = ptab_bui;

    // ---------- bui_window (overlapped) with its own menu bar ----------
    bui_window* bui_wnd = new bui_window(gui, 10, 10, 760, 500,
        BUI_WINDOW_OVERLAPPED, "Blendish Demo", -1);

    bui_menubar* menubar = new bui_menubar(bui_wnd, 0, 0, (int)bui_wnd->get_size().x,
        [](bui_menubar* pmenu, int sub_id, int item_id) {
          printf("[bui_menubar] submenu=%d item=%d\n", sub_id, item_id);
        });
    int mf = menubar->add_submenu("File", 0, BND_ICONID(0, 8));
    menubar->add_item(mf, "New", 0, BND_ICONID(0, 0));
    menubar->add_item(mf, "Open...", 1, BND_ICONID(1, 0));
    menubar->add_item(mf, "Save", 2, BND_ICONID(2, 0));
    menubar->add_item(mf, "Save As...", 3);
    menubar->add_separator(mf);
    menubar->add_item(mf, "Quit", 4);

    int me = menubar->add_submenu("Edit", 1, BND_ICONID(1, 8));
    menubar->add_item(me, "Undo", 0);
    menubar->add_item(me, "Redo", 1);
    menubar->add_separator(me);
    menubar->add_item(me, "Cut", 2);
    menubar->add_item(me, "Copy", 3);
    menubar->add_item(me, "Paste", 4);

    int mv = menubar->add_submenu("View", 2);
    menubar->add_item(mv, "Zoom In", 0, BND_ICONID(5, 10));
    menubar->add_item(mv, "Zoom Out", 1, BND_ICONID(6, 10));
    menubar->add_separator(mv);
    menubar->add_item(mv, "Reset View", 2);

    int mh = menubar->add_submenu("Help", 3);
    menubar->add_item(mh, "About", 0);

    // ---------- Vertical toolbox (left-anchored) ----------
    bui_toolbox* toolbox_v = new bui_toolbox(bui_wnd, 0, 0, 28,
        BUI_ANCHOR_LEFT,
        [](bui_toolbox* tb, int tool_id, int subtool_id, bool is_default) {
          printf("[bui_toolbox] tool=%d subtool=%d default=%d\n", tool_id, subtool_id, is_default);
        });
    toolbox_v->add_tool(0, BND_ICONID(0, 10), "Select");
    toolbox_v->add_tool(1, BND_ICONID(1, 10), "Move");
    toolbox_v->add_multitool(1, 10, BND_ICONID(2, 10), "Rotate");
    toolbox_v->add_multitool(1, 11, BND_ICONID(3, 10), "Scale");
    toolbox_v->add_tool(2, BND_ICONID(4, 10), "Draw");
    toolbox_v->add_separator();
    toolbox_v->add_tool(3, BND_ICONID(5, 10), "Erase");
    toolbox_v->set_active_id(0);

    float cx = 40.f, cy = 10.f; // current layout cursor (offset for toolbox)

    // ---------- Column 1: Basic controls ----------
    // Menu label (section header)
    new bui_menu_label(bui_wnd, (int)cx, (int)cy, 200, "Basic Controls");
    cy += BND_WIDGET_HEIGHT + 4.f;

    // Separator
    new bui_separator(bui_wnd, (int)cx, (int)cy, 200);
    cy += 8.f;

    // Option buttons (checkboxes)
    bui_option_button* opt1 = new bui_option_button(bui_wnd, (int)cx, (int)cy, 200, (int)BND_WIDGET_HEIGHT,
        "Enable Feature", false,
        [](bui_option_button* btn, bool checked) { printf("[bui] Enable Feature: %s\n", checked ? "ON" : "OFF"); });
    cy += BND_WIDGET_HEIGHT + 4.f;

    bui_option_button* opt2 = new bui_option_button(bui_wnd, (int)cx, (int)cy, 200, (int)BND_WIDGET_HEIGHT,
        "Auto Save", true,
        [](bui_option_button* btn, bool checked) { printf("[bui] Auto Save: %s\n", checked ? "ON" : "OFF"); });
    cy += BND_WIDGET_HEIGHT + 4.f;

    // Text field
    new bui_text_field(bui_wnd, (int)cx, (int)cy, 200, (int)BND_WIDGET_HEIGHT, "Hello World", -1, BND_CORNER_NONE,
        [](bui_text_field* pf, const char* text) { printf("[bui] Text: %s\n", text); });
    cy += BND_WIDGET_HEIGHT + 4.f;

    // Choice button (dropdown)
    bui_choice_button* choice = new bui_choice_button(bui_wnd, (int)cx, (int)cy, 200, (int)BND_WIDGET_HEIGHT,
        -1, BND_CORNER_NONE, [](bui_choice_button* btn, int idx) { printf("[bui] Choice: %d\n", idx); });
    choice->add_item("Option A");
    choice->add_item("Option B");
    choice->add_item("Option C");
    choice->set_selected(0);
    cy += BND_WIDGET_HEIGHT + 4.f;

    // Radio button group
    bui_radio_button* radio = new bui_radio_button(bui_wnd, (int)cx, (int)cy, 200, (int)(BND_WIDGET_HEIGHT * 3 - 4),
        false, [](bui_radio_button* btn, int sel) { printf("[bui] Radio: %d\n", sel); });
    radio->add_item("Point");
    radio->add_item("Edge");
    radio->add_item("Face");
    radio->set_selected(0);
    cy += BND_WIDGET_HEIGHT * 3.f;

    // ---------- Column 2: Numeric controls ----------
    float col2_x = 230.f;
    float col2_y = 10.f;

    new bui_menu_label(bui_wnd, (int)col2_x, (int)col2_y, 240, "Numeric Controls");
    col2_y += BND_WIDGET_HEIGHT + 4.f;

    new bui_separator(bui_wnd, (int)col2_x, (int)col2_y, 240);
    col2_y += 8.f;

    // Number fields (stacked with shared corners)
    new bui_number_field(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_WIDGET_HEIGHT,
        "X", 1.0f, -100.f, 100.f, 0.1f, 2, BND_CORNER_DOWN);
    col2_y += BND_WIDGET_HEIGHT - 2.f;
    new bui_number_field(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_WIDGET_HEIGHT,
        "Y", 0.5f, -100.f, 100.f, 0.1f, 2, BND_CORNER_ALL);
    col2_y += BND_WIDGET_HEIGHT - 2.f;
    new bui_number_field(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_WIDGET_HEIGHT,
        "Z", -2.3f, -100.f, 100.f, 0.1f, 2, BND_CORNER_TOP);
    col2_y += BND_WIDGET_HEIGHT + 8.f;

    // Slider
    new bui_slider(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_WIDGET_HEIGHT,
        "Opacity", 0.75f, 0.f, 1.f, 2, BND_CORNER_NONE,
        [](bui_slider* s, float v) { printf("[bui] Opacity: %.2f\n", v); });
    col2_y += BND_WIDGET_HEIGHT + 4.f;

    new bui_slider(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_WIDGET_HEIGHT,
        "Scale", 50.f, 0.f, 100.f, 1, BND_CORNER_NONE, nullptr);
    col2_y += BND_WIDGET_HEIGHT + 8.f;

    // Horizontal scrollbar
    new bui_scrollbar(bui_wnd, (int)col2_x, (int)col2_y, 240, (int)BND_SCROLLBAR_HEIGHT,
        false, 0.3f, nullptr);
    col2_y += BND_SCROLLBAR_HEIGHT + 8.f;

    // Knobs
    new bui_knob(bui_wnd, (int)col2_x, (int)col2_y, 50,
        "Volume", 0.7f, 0.f, 1.f, nullptr);
    new bui_knob(bui_wnd, (int)col2_x + 70, (int)col2_y, 50,
        "Pan", 0.5f, -1.f, 1.f, nullptr);
    new bui_knob(bui_wnd, (int)col2_x + 140, (int)col2_y, 50,
        "Gain", 0.3f, 0.f, 1.f, nullptr);
    col2_y += 70.f;

    // ---------- Column 3: Color & interactive ----------
    float col3_x = 490.f;
    float col3_y = 10.f;

    new bui_menu_label(bui_wnd, (int)col3_x, (int)col3_y, 200, "Color & Picker");
    col3_y += BND_WIDGET_HEIGHT + 4.f;

    new bui_separator(bui_wnd, (int)col3_x, (int)col3_y, 200);
    col3_y += 8.f;

    // Color buttons
    new bui_color_button(bui_wnd, (int)col3_x, (int)col3_y, 40, 40,
        NVGcolor::RGB(255, 80, 80), BND_CORNER_NONE, nullptr);
    new bui_color_button(bui_wnd, (int)col3_x + 44, (int)col3_y, 40, 40,
        NVGcolor::RGB(80, 200, 80), BND_CORNER_NONE, nullptr);
    new bui_color_button(bui_wnd, (int)col3_x + 88, (int)col3_y, 40, 40,
        NVGcolor::RGB(80, 120, 255), BND_CORNER_NONE, nullptr);
    new bui_color_button(bui_wnd, (int)col3_x + 132, (int)col3_y, 40, 40,
        NVGcolor::RGB(255, 200, 50), BND_CORNER_NONE, nullptr);
    col3_y += 48.f;

    // Color picker
    new bui_color_picker(bui_wnd, (int)col3_x, (int)col3_y, 180,
        NVGcolor::RGB(200, 100, 50), nullptr);
    col3_y += 190.f;

    // Toolbar
    bui_toolbar* toolbar = new bui_toolbar(bui_wnd, (int)col3_x, (int)col3_y, 200, (int)BND_WIDGET_HEIGHT,
        false, [](bui_toolbar* tb, int id) { printf("[bui] Toolbar: %d\n", id); });
    toolbar->add_button(0, BND_ICONID(0, 10), nullptr);
    toolbar->add_button(1, BND_ICONID(1, 10), nullptr);
    toolbar->add_button(2, BND_ICONID(2, 10), nullptr);
    toolbar->add_separator();
    toolbar->add_button(3, BND_ICONID(3, 10), nullptr);
    toolbar->add_button(4, BND_ICONID(4, 10), nullptr);
    toolbar->set_active_id(1);
    col3_y += BND_WIDGET_HEIGHT + 8.f;

    // Radio Toolbar (horizontal)
    bui_radio_toolbar* rtoolbar = new bui_radio_toolbar(bui_wnd, (int)col3_x, (int)col3_y, 200, (int)BND_WIDGET_HEIGHT,
        false, [](bui_radio_toolbar* tb, int id) { printf("[bui] RadioToolbar: %d\n", id); });
    rtoolbar->add_button(0, BND_ICONID(6, 10), "File");
    rtoolbar->add_button(1, BND_ICONID(7, 10), "Edit");
    rtoolbar->add_separator();
    rtoolbar->add_button(2, BND_ICONID(8, 10), "View");
    rtoolbar->set_active_id(0);
    col3_y += BND_WIDGET_HEIGHT + 8.f;

    // Radio Toolbar (vertical)
    bui_radio_toolbar* rtoolbar_v = new bui_radio_toolbar(bui_wnd, (int)col3_x, (int)col3_y, 120, (int)(BND_WIDGET_HEIGHT * 4 - 3),
        true, [](bui_radio_toolbar* tb, int id) { printf("[bui] RadioToolbarV: %d\n", id); });
    rtoolbar_v->add_button(0, BND_ICONID(0, 10), nullptr);
    rtoolbar_v->add_button(1, BND_ICONID(1, 10), nullptr);
    rtoolbar_v->add_button(2, BND_ICONID(2, 10), nullptr);
    rtoolbar_v->add_button(3, BND_ICONID(3, 10), nullptr);
    rtoolbar_v->set_active_id(2);
    col3_y += BND_WIDGET_HEIGHT * 4.f + 8.f;

    // ---------- Bottom: Node editor area ----------
    float node_y = rm_max(cy, col2_y) + 10.f;

    new bui_menu_label(bui_wnd, 10, (int)node_y, 200, "Node Editor");
    node_y += BND_WIDGET_HEIGHT + 4.f;

    new bui_separator(bui_wnd, 10, (int)node_y, 680);
    node_y += 8.f;

    // Create nodes
    bui_node* node1 = new bui_node(bui_wnd, 30, (int)node_y, 150,
        "Mix Shader", -1, NVGcolor::RGBA(180, 60, 60, 255), nullptr);
    node1->add_input("Fac", NVGcolor::RGB(200, 200, 200));
    node1->add_input("Shader", NVGcolor::RGB(80, 200, 80));
    node1->add_input("Shader", NVGcolor::RGB(80, 200, 80));
    node1->add_output("Shader", NVGcolor::RGB(80, 200, 80));

    bui_node* node2 = new bui_node(bui_wnd, 250, (int)node_y + 30, 150,
        "Diffuse BSDF", -1, NVGcolor::RGBA(60, 120, 180, 255), nullptr);
    node2->add_input("Color", NVGcolor::RGB(200, 200, 80));
    node2->add_input("Roughness", NVGcolor::RGB(200, 200, 200));
    node2->add_input("Normal", NVGcolor::RGB(120, 120, 200));
    node2->add_output("BSDF", NVGcolor::RGB(80, 200, 80));

    bui_node* node3 = new bui_node(bui_wnd, 470, (int)node_y + 10, 150,
        "Material Output", -1, NVGcolor::RGBA(100, 100, 100, 255), nullptr);
    node3->add_input("Surface", NVGcolor::RGB(80, 200, 80));
    node3->add_input("Volume", NVGcolor::RGB(80, 200, 80));
    node3->add_input("Displacement", NVGcolor::RGB(200, 200, 200));

    // Node wire connections (added last so it draws on top of nodes)
    bui_node_wires* wires = new bui_node_wires(bui_wnd);
    // Mix Shader output → Material Output "Surface"
    wires->add_connection(node1->get_output(0), node3->get_input(0));
    // Diffuse BSDF output → Mix Shader "Shader" input #1
    wires->add_connection(node2->get_output(0), node1->get_input(1));

    // Splitter at the bottom
    float splitter_y = node_y + 180.f;
    new bui_splitter(bui_wnd, 10, (int)splitter_y, 680, 60,
        true, 0.5f, 6.f, nullptr);

    // Panel (collapsible)
    bui_panel* panel = new bui_panel(bui_wnd, (int)col3_x, (int)(col3_y + 10.f), 200, 120,
        "Properties", -1, nullptr);
    new bui_option_button(panel, 10, 30, 180, (int)BND_WIDGET_HEIGHT,
        "Smooth Shading", true, nullptr);
    new bui_number_field(panel, 10, 56, 180, (int)BND_WIDGET_HEIGHT,
        "Subdiv", 2.f, 0.f, 6.f, 1.f, 0, BND_CORNER_NONE, nullptr);

    // Vertical scrollbar
    new bui_scrollbar(bui_wnd, (int)(col3_x + 205.f), (int)(col3_y + 10.f), (int)BND_SCROLLBAR_WIDTH, 120,
        true, 0.4f, nullptr);

    // ---------- Popup window demo ----------
    bui_window* popup_wnd = new bui_window(gui, 10, 520, 200, 80,
        BUI_WINDOW_POPUP);
    new bui_menu_label(popup_wnd, 5, 5, 190, "Popup Window");
    new bui_option_button(popup_wnd, 5, 30, 190, (int)BND_WIDGET_HEIGHT,
        "Popup Option", false, nullptr);

    // Select this tab by default
    ptabctl->set_selected_index(5);
  }
#endif

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
  if (!default_font.isValid()) {
    printf("can't load font!\n");
  }

  rm_image image_pat = g_gui->load_image("ipat.png", NVG_IMAGE_REPEATX);
  if (!image_pat.isValid()) {
    printf("can't load image\n");
  }

  g_gui->set_font(default_font);

#if defined(RMGUI_ENABLE_BLENDISH_DEMO)
  // Initialize the isolated Blendish demo.
  bndSetFont(g_gui->get_context()->findFont("default"));

  // Load blendish icon sheet (Blender 2.6 compatible, 602x640 px grid)
  // Download from Blender sources: blender_icons16.png
  rm_image icon_sheet = g_gui->load_image("blender_icons16.png", 0);
  if (icon_sheet.isValid()) {
    bndSetIconImage(icon_sheet);
  } else {
    printf("warning: blender_icons16.png not found, icons will not be displayed\n");
  }
#endif

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
    g_gui->resize(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    g_gui->draw(dt);

#if 0
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    vg->BeginFrame( w, h, 1.f);
    drawBlendish(vg, 0, 0, 400, 400, instance.get_time());
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    drawParagraph(vg, 10, 10, w, h, (float)mx, (float)my);
    vg->EndFrame();
#endif
    glfwSwapBuffers(g_gui->get_syswindow<GLFWwindow*>());
  }

  destroy_window(g_gui);

  return 0;
}
