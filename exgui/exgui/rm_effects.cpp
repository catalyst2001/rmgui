#include "rm_effects.h"

#include <cmath>
#include <vector>

namespace {
float rm_effects_random(unsigned int seed) {
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return (float)(seed & 0xFFFF) / 65535.0f;
}
}

rm_effects::rm_effects(rm_widget* p_parent, float x, float y, float width, float height)
  : rm_widget(x, y, width, height, p_parent, "effects"),
    m_bgImage(0),
    m_bgWidth(0),
    m_bgHeight(0),
    m_bgDirty(true) {}

rm_effects::~rm_effects() {
  rm_surface* root = get_root();
  if (root) {
    release_background(root->get_context());
  }
}

void rm_effects::resize(float width, float height) {
  rm_widget::resize(width, height);
  m_bgDirty = true;
}

void rm_effects::release_background(NVGcontext* ctx) {
  if (m_bgImage.isValid() && ctx != nullptr) {
    ctx->deleteImage(m_bgImage);
  }
  m_bgImage.invalidate();
  m_bgWidth = 0;
  m_bgHeight = 0;
}

void rm_effects::ensure_background(NVGcontext* ctx) 
{
  if (ctx == nullptr)
    return;

  if (m_bgDirty && m_bgImage.isValid()) {
    ctx->deleteImage(m_bgImage);
    m_bgImage.invalidate();
  }

  if (m_bgImage.isValid())
    return;

  const int w = 512;
  const int h = 512;
  std::vector<unsigned char> pixels(w * h * 4);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const float fx = x / (float)(w - 1);
      const float fy = y / (float)(h - 1);
      const float wave = 0.5f + 0.5f * sinf(fx * 8.0f + fy * 4.0f);
      const unsigned int seed = (unsigned int)(x * 1973u + y * 9277u) ^ 0x68bc21u;
      const float noise = ((float)(seed & 255u) / 255.0f) - 0.5f;

      float r = 20.0f + 120.0f * fx + 60.0f * wave + 18.0f * noise;
      float g = 18.0f + 90.0f * fy + 40.0f * wave + 14.0f * noise;
      float b = 40.0f + 140.0f * (1.0f - fx) + 50.0f * wave + 12.0f * noise;

      r = rm_clamp(r, 0.0f, 255.0f);
      g = rm_clamp(g, 0.0f, 255.0f);
      b = rm_clamp(b, 0.0f, 255.0f);

      const int idx = (y * w + x) * 4;
      pixels[idx + 0] = static_cast<unsigned char>(r);
      pixels[idx + 1] = static_cast<unsigned char>(g);
      pixels[idx + 2] = static_cast<unsigned char>(b);
      pixels[idx + 3] = 255;
      }
    }

    m_bgImage = ctx->createImageRGBA(w, h, NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY, pixels.data());
    m_bgWidth = w;
    m_bgHeight = h;
    m_bgDirty = false;
  }

void rm_effects::draw_glass_showcase(NVGcontext* ctx, float x, float y, float w, float h) {
  // Get the built-in glass shader and create a glass paint.
  NVGhandle glassShader = ctx->getBuiltinGlassShader();
  NVGcolor tint = NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.14f);
  NVGpaint glass = NVGpaint::glass(0.0f, 0.0f, m_size.x, m_size.y, m_bgImage, glassShader, tint, 0.9f, 1.0f);

  ctx->beginPath();
  ctx->roundedRect(x, y, w, h, 18.0f);
  ctx->fillPaint(glass);
  ctx->fill();

  // Border
  ctx->StrokeWidth(1.0f);
  ctx->strokeColor(NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.42f));
  ctx->beginPath();
  ctx->roundedRect(x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, 17.5f);
  ctx->stroke();

  ctx->setFontFace("default");
  ctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

  NVGblurStyle titleBlur;
  titleBlur.type = NVG_BLUR_LIQUID;
  titleBlur.radius = 6.0f;
  titleBlur.strength = 0.7f;
  titleBlur.steps = 12;
  titleBlur.rings = 2;
  titleBlur.jitter = 0.35f;
  titleBlur.color = NVGcolor::RGBA(255, 255, 255, 235);

  ctx->setFontSize(30.0f);
  ctx->textBlur(x + 20.0f, y + 20.0f, "Liquid Glass", nullptr, titleBlur);

  NVGblurStyle subBlur = titleBlur;
  subBlur.type = NVG_BLUR_GAUSSIAN;
  subBlur.radius = 3.0f;
  subBlur.strength = 0.45f;
  subBlur.jitter = 0.0f;
  subBlur.color = NVGcolor::RGBA(200, 220, 255, 200);
  ctx->setFontSize(16.0f);
  ctx->textBlur(x + 20.0f, y + 72.0f, "Blurred text + frosted panel", nullptr, subBlur);

  ctx->fillColor(NVGcolor::RGBA(220, 220, 220, 200));
  ctx->text(x + 20.0f, y + 112.0f, "Backend-agnostic effects demo", nullptr);
}

void rm_effects::draw_blur_gallery(NVGcontext* ctx, float x, float y, float w) {
  struct BlurSample {
    const char* label;
    NVGblurType type;
    float radius;
    float angle;
    float length;
    float jitter;
    int blades;
    NVGcolor color;
  };

  const BlurSample samples[] = {
    { "Gaussian blur", NVG_BLUR_GAUSSIAN, 5.0f, 0.0f, 1.0f, 0.0f, 0, NVGcolor::RGBA(250, 220, 200, 220) },
    { "Motion blur", NVG_BLUR_MOTION, 8.0f, 0.35f, 1.2f, 0.0f, 0, NVGcolor::RGBA(200, 230, 255, 220) },
    { "Radial blur", NVG_BLUR_RADIAL, 6.0f, 0.28f, 1.0f, 0.0f, 0, NVGcolor::RGBA(220, 200, 255, 220) },
    { "Linear blur", NVG_BLUR_LINEAR, 7.0f, -0.6f, 1.0f, 0.0f, 0, NVGcolor::RGBA(200, 255, 220, 220) },
    { "Bokeh blur", NVG_BLUR_BOKEH, 6.0f, 0.0f, 1.0f, 0.35f, 6, NVGcolor::RGBA(255, 215, 160, 220) },
    { "Liquid blur", NVG_BLUR_LIQUID, 6.0f, 0.0f, 1.0f, 0.45f, 0, NVGcolor::RGBA(200, 240, 255, 220) },
  };

  const int columns = (w >= 560.0f) ? 2 : 1;
  const float colW = (columns == 2) ? (w * 0.5f) : w;
  const float rowH = 44.0f;

  ctx->setFontFace("default");
  ctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  ctx->setFontSize(14.0f);
  ctx->fillColor(NVGcolor::RGBA(240, 240, 240, 200));
  ctx->text(x, y - 22.0f, "Text blur types", nullptr);

  for (int i = 0; i < (int)(sizeof(samples) / sizeof(samples[0])); ++i) {
    const int col = i % columns;
    const int row = i / columns;
    const float sx = x + colW * col;
    const float sy = y + rowH * row;

    NVGblurStyle blur;
    blur.type = samples[i].type;
    blur.radius = samples[i].radius;
    blur.strength = 0.65f;
    blur.steps = 12;
    blur.rings = 3;
    blur.angle = samples[i].angle;
    blur.length = samples[i].length;
    blur.jitter = samples[i].jitter;
    blur.blades = samples[i].blades;
    blur.color = samples[i].color;

    ctx->setFontSize(22.0f);
    ctx->textBlur(sx, sy, samples[i].label, nullptr, blur);
  }
}

void rm_effects::on_draw(NVGcontext* ctx) {
  ensure_background(ctx);
  if (ctx == nullptr || !m_bgImage.isValid()) {
    return;
  }

  NVGpaint bg = NVGpaint::imagePattern(0.0f, 0.0f, m_size.x, m_size.y, 0.0f, m_bgImage, 1.0f);
  ctx->beginPath();
  ctx->rect(0.0f, 0.0f, m_size.x, m_size.y);
  ctx->fillPaint(bg);
  ctx->fill();

  const float pad = 24.0f;
  const float panelW = rm_min(420.0f, m_size.x - pad * 2.0f);
  const float panelH = 220.0f;
  const float panelX = pad;
  const float panelY = pad;

  if (panelW <= 0.0f || panelH <= 0.0f) {
    return;
  }

  

  const float glowX = panelX + panelW + 40.0f;
  const float glowW = m_size.x - glowX - pad;
  if (glowW > 120.0f) {
    const float glowH = 140.0f;
    const float glowY = panelY + 24.0f;

    NVGglowStyle glow;
    glow.radius = 20.0f;
    glow.intensity = 0.9f;
    glow.color = NVGcolor::RGBA(90, 160, 255, 220);
    ctx->glowRect(glowX, glowY, glowW, glowH, 18.0f, glow);

    ctx->beginPath();
    ctx->roundedRect(glowX, glowY, glowW, glowH, 18.0f);
    ctx->fillColor(NVGcolor::RGBA(18, 20, 28, 210));
    ctx->fill();
    ctx->StrokeWidth(1.0f);
    ctx->strokeColor(NVGcolor::RGBA(120, 170, 255, 120));
    ctx->stroke();

    ctx->setFontSize(18.0f);
    ctx->fillColor(NVGcolor::RGBA(200, 220, 255, 220));
    ctx->setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    ctx->text(glowX + glowW * 0.5f, glowY + glowH * 0.5f, "Neon Glow", nullptr);
  }

  draw_glass_showcase(ctx, panelX + 200, panelY, panelW, panelH);

  ctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  draw_blur_gallery(ctx, pad, panelY + panelH + 64.0f, m_size.x - pad * 2.0f);
}
