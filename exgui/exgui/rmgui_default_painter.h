#pragma once

#include "rmgui_theme.h"

#include <cstddef>
#include <string>
#include <vector>

struct RmButtonVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
};

struct RmTabVisual {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  RmTabPlacement placement = RmTabPlacement::top;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool selected = false;
  bool focused = false;
  bool closable = false;
  bool close_hovered = false;
};

struct RmMenuSurfaceVisual {
  float width = 0.0f;
  float height = 0.0f;
  bool popup = false;
};

struct RmMenuItemVisual {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool root_item = false;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool opened = false;
  bool separator = false;
  bool has_submenu = false;
};

struct RmLabelVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
};

struct RmTextInputVisual {
  float width = 0.0f;
  float height = 0.0f;
  float scroll_offset = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  size_t cursor = 0;
  size_t selection_start = 0;
  size_t selection_end = 0;
  bool multiline = false;
  bool enabled = true;
  bool hovered = false;
  bool dragging = false;
  bool focused = false;
  bool caret_visible = false;
};

struct RmTextInputLineLayout {
  std::string text;
  size_t text_start = 0;
  std::vector<size_t> byte_offsets;
  std::vector<float> glyph_positions;
  float baseline = 0.0f;
};

struct RmTextInputLayout {
  std::vector<RmTextInputLineLayout> lines;
  float ascender = 0.0f;
  float descender = 0.0f;
  float line_height = 0.0f;
  float scroll_offset = 0.0f;
};

struct RmNumberInputVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool focused = false;
  bool increment_hovered = false;
  bool increment_pressed = false;
  bool decrement_hovered = false;
  bool decrement_pressed = false;
};

struct RmCheckboxVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
  bool checked = false;
};

struct RmRadioButtonVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
  bool checked = false;
};

struct RmComboBoxVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool placeholder = false;
  bool enabled = true;
  bool hovered = false;
  bool focused = false;
  bool expanded = false;
};

struct RmComboBoxPopupVisual {
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct RmComboBoxItemVisual {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool selected = false;
};

struct RmListViewSurfaceVisual {
  float width = 0.0f;
  float height = 0.0f;
  bool focused = false;
  bool enabled = true;
};

struct RmListViewRowVisual {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool selected = false;
};

struct RmSliderVisual {
  float width = 0.0f;
  float height = 0.0f;
  float fraction = 0.0f;
  bool enabled = true;
  bool hovered = false;
  bool dragging = false;
  bool focused = false;
};

struct RmScrollbarVisual {
  float width = 0.0f;
  float height = 0.0f;
  float thumb_offset = 0.0f;
  float thumb_length = 0.0f;
  bool vertical = true;
  bool enabled = true;
  bool hovered = false;
  bool dragging = false;
  bool focused = false;
};

struct RmProgressVisual {
  float width = 0.0f;
  float height = 0.0f;
  float fraction = 0.0f;
  bool enabled = true;
};

struct RmSwitchVisual {
  float width = 0.0f;
  float height = 0.0f;
  float animation_progress = 0.0f;
  bool enabled = true;
  bool hovered = false;
  bool pressed = false;
  bool focused = false;
};

class RmDefaultControlPainter {
public:
  static void draw_button(NVGcontext& context, const RmButtonVisual& visual,
    const RmButtonStyle& style);
  static void draw_tab_bar(NVGcontext& context, float x, float y, float width,
    float height, const RmTabStyle& style);
  static void draw_tab_page(NVGcontext& context, float x, float y, float width,
    float height, const RmTabStyle& style);
  static void draw_tab(NVGcontext& context, const RmTabVisual& visual,
    const RmTabStyle& style);
  static void draw_menu_surface(NVGcontext& context,
    const RmMenuSurfaceVisual& visual, const RmMenuStyle& style);
  static void draw_menu_item(NVGcontext& context,
    const RmMenuItemVisual& visual, const RmMenuStyle& style);
  static void draw_label(NVGcontext& context, const RmLabelVisual& visual,
    const RmLabelStyle& style);
  static RmTextInputLayout layout_text_input(NVGcontext& context,
    const RmTextInputVisual& visual, const RmTextInputStyle& style);
  static size_t hit_test_text_input(const RmTextInputLayout& layout,
    float x, float y, const RmTextInputStyle& style);
  static void draw_text_input(NVGcontext& context,
    const RmTextInputVisual& visual, const RmTextInputLayout& layout,
    const RmTextInputStyle& style);
  static void draw_number_input(NVGcontext& context,
    const RmNumberInputVisual& visual, const RmNumberInputStyle& style);
  static void draw_checkbox(NVGcontext& context, const RmCheckboxVisual& visual,
    const RmCheckboxStyle& style);
  static void draw_radiobutton(NVGcontext& context,
    const RmRadioButtonVisual& visual, const RmRadioButtonStyle& style);
  static void draw_combobox(NVGcontext& context, const RmComboBoxVisual& visual,
    const RmComboBoxStyle& style);
  static void draw_combobox_popup(NVGcontext& context,
    const RmComboBoxPopupVisual& visual, const RmComboBoxStyle& style);
  static void draw_combobox_item(NVGcontext& context,
    const RmComboBoxItemVisual& visual, const RmComboBoxStyle& style);
  static void draw_listview_surface(NVGcontext& context,
    const RmListViewSurfaceVisual& visual, const RmListViewStyle& style);
  static void draw_listview_row(NVGcontext& context,
    const RmListViewRowVisual& visual, const RmListViewStyle& style);
  static void draw_slider(NVGcontext& context, const RmSliderVisual& visual,
    const RmSliderStyle& style);
  static void draw_scrollbar(NVGcontext& context, const RmScrollbarVisual& visual,
    const RmScrollbarStyle& style);
  static void draw_progress(NVGcontext& context, const RmProgressVisual& visual,
    const RmProgressStyle& style);
  static void draw_switch(NVGcontext& context, const RmSwitchVisual& visual,
    const RmSwitchStyle& style);
};
