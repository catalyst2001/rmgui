#pragma once

#include "rmgui_theme.h"

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

struct RmLabelVisual {
  float width = 0.0f;
  float height = 0.0f;
  NVGhandle font;
  const char* text = nullptr;
  bool enabled = true;
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

struct RmSliderVisual {
  float width = 0.0f;
  float height = 0.0f;
  float fraction = 0.0f;
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
  static void draw_label(NVGcontext& context, const RmLabelVisual& visual,
    const RmLabelStyle& style);
  static void draw_checkbox(NVGcontext& context, const RmCheckboxVisual& visual,
    const RmCheckboxStyle& style);
  static void draw_slider(NVGcontext& context, const RmSliderVisual& visual,
    const RmSliderStyle& style);
  static void draw_progress(NVGcontext& context, const RmProgressVisual& visual,
    const RmProgressStyle& style);
  static void draw_switch(NVGcontext& context, const RmSwitchVisual& visual,
    const RmSwitchStyle& style);
};
