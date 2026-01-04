#pragma once

#include "rmgui.h"

class rm_effects : public rm_widget {
public:
  rm_effects(rm_widget* p_parent, float x, float y, float width, float height);
  ~rm_effects();

  void resize(float width, float height) override;

protected:
  void on_draw(NVGcontext* ctx) override;

private:
  NVGhandle m_bgImage;
  int m_bgWidth;
  int m_bgHeight;
  bool m_bgDirty;

  void ensure_background(NVGcontext* ctx);
  void release_background(NVGcontext* ctx);
  void draw_glass_showcase(NVGcontext* ctx, float x, float y, float w, float h);
  void draw_blur_gallery(NVGcontext* ctx, float x, float y, float w);
};
