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
  NVGhandle glassShader = ctx->getBuiltinGlassShader();

  // Set background size uniform (same for all panels, widget-local coords)
  NVGhandle bgSizeUniform = ctx->findUniform(glassShader, "glassBgSize");
  if (bgSizeUniform.isValid()) {
    float bgSize[2] = { m_size.x, m_size.y };
    ctx->setUniformData(bgSizeUniform, bgSize, sizeof(bgSize));
  }

  // Cache uniform handles
  NVGhandle uBlur     = ctx->findUniform(glassShader, "glassBlurRadius");
  NVGhandle uRefract  = ctx->findUniform(glassShader, "glassRefractionStrength");
  NVGhandle uSat      = ctx->findUniform(glassShader, "glassSaturation");
  NVGhandle uContrast = ctx->findUniform(glassShader, "glassContrast");
  NVGhandle uChroma   = ctx->findUniform(glassShader, "glassChromaStrength");
  NVGhandle uCorner   = ctx->findUniform(glassShader, "glassCornerRadius");
  NVGhandle uFBias    = ctx->findUniform(glassShader, "glassFresnelBias");
  NVGhandle uFScale   = ctx->findUniform(glassShader, "glassFresnelScale");
  NVGhandle uFPower   = ctx->findUniform(glassShader, "glassFresnelPower");
  NVGhandle uSpecInt  = ctx->findUniform(glassShader, "glassSpecularIntensity");
  NVGhandle uSpecSize = ctx->findUniform(glassShader, "glassSpecularSize");

  // Helper: set a float uniform
  auto setF = [&](NVGhandle u, float v) {
    if (u.isValid()) ctx->setUniformData(u, &v, sizeof(v));
  };

  // Liquid-glass parameters matching CSS: liquid-glass(refraction, cornerPx [, chroma])
  struct GlassPreset {
    const char* title;
    const char* subtitle;
    float blurRadius;        // blur(Npx)
    float refractionStr;     // liquid-glass arg 1
    float cornerRadius;      // liquid-glass arg 2 (normalized)
    float saturation;        // saturate()
    float contrast;          // contrast()
    float chromaStrength;    // liquid-glass arg 3 (0=off, 1=on)
    NVGcolor tint;           // color-overlay(color, alpha)
    float fresnelScale;
    float fresnelPower;
    float specIntensity;
  };

  const GlassPreset presets[] = {
    // Row 1: no chroma
    { "liquid-glass() + blur()", "Raw",
      0.4f, 0.15f, 0.25f, 1.1f, 1.0f, 0.0f,
      NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.02f),
      0.06f, 1.5f, 0.02f },

    { "liquid-glass() + blur()", "Dark",
      0.4f, 0.15f, 0.25f, 1.1f, 1.1f, 0.0f,
      NVGcolor::RGBAf(0.0f, 0.0f, 0.0f, 0.12f),
      0.05f, 1.5f, 0.015f },

    { "liquid-glass() + blur()", "Light",
      0.4f, 0.15f, 0.25f, 1.1f, 1.1f, 0.0f,
      NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.12f),
      0.08f, 1.5f, 0.03f },

    // Row 2: chroma enabled
    { "liquid-glass() + blur()", "Chroma Raw",
      0.4f, 0.15f, 0.25f, 1.1f, 1.0f, 0.6f,
      NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.02f),
      0.06f, 1.5f, 0.02f },

    { "liquid-glass() + blur()", "Chroma Dark",
      0.4f, 0.15f, 0.25f, 1.1f, 1.1f, 0.6f,
      NVGcolor::RGBAf(0.0f, 0.0f, 0.0f, 0.12f),
      0.05f, 1.5f, 0.015f },

    { "liquid-glass() + blur()", "Chroma Light",
      0.4f, 0.15f, 0.25f, 1.1f, 1.1f, 0.6f,
      NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.12f),
      0.08f, 1.5f, 0.03f },
  };

  // Helper lambda to draw a single glass panel
  auto drawPanel = [&](float px, float py, float pw, float ph, const GlassPreset& p) {
    // Set per-panel shader uniforms
    setF(uBlur, p.blurRadius);
    setF(uRefract, p.refractionStr);
    setF(uCorner, p.cornerRadius);
    setF(uSat, p.saturation);
    setF(uContrast, p.contrast);
    setF(uChroma, p.chromaStrength);
    setF(uFBias, 0.0f);
    setF(uFScale, p.fresnelScale);
    setF(uFPower, p.fresnelPower);
    setF(uSpecInt, p.specIntensity);
    setF(uSpecSize, 0.35f);

    // Paint encodes panel rect + panel origin (per-draw in frag UBO)
    NVGpaint glass = NVGpaint::glass(px, py, pw, ph, m_size.x, m_size.y,
                                     m_bgImage, glassShader, p.tint, 1.0f);

    ctx->beginPath();
    ctx->roundedRect(px, py, pw, ph, 18.0f);
    ctx->fillPaint(glass);
    ctx->fill();

    // Border (thin, subtle)
    ctx->StrokeWidth(1.0f);
    ctx->strokeColor(NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.18f));
    ctx->beginPath();
    ctx->roundedRect(px + 0.5f, py + 0.5f, pw - 1.0f, ph - 1.0f, 17.5f);
    ctx->stroke();

    // Title text with blur
    ctx->setFontFace("default");
    ctx->setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    NVGblurStyle titleBlur;
    titleBlur.type = NVG_BLUR_LIQUID;
    titleBlur.radius = 5.0f;
    titleBlur.strength = 0.65f;
    titleBlur.steps = 10;
    titleBlur.rings = 2;
    titleBlur.jitter = 0.3f;
    titleBlur.color = NVGcolor::RGBA(255, 255, 255, 230);
    ctx->setFontSize(20.0f);
    ctx->textBlur(px + pw * 0.5f, py + 14.0f, p.title, nullptr, titleBlur);

    // Subtitle
    ctx->setFontSize(16.0f);
    ctx->textBlur(px + pw * 0.5f, py + 38.0f, p.subtitle, nullptr, titleBlur);
  };

  // Layout: 2 rows x 3 columns
  const float gapX = 14.0f;
  const float gapY = 14.0f;
  const float pw = (w - gapX * 2.0f) / 3.0f;
  const float ph = (h - gapY) * 0.5f;

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    float px = x + col * (pw + gapX);
    float py = y + row * (ph + gapY);
    drawPanel(px, py, pw, ph, presets[i]);
  }

  // Restore defaults
  setF(uBlur, 1.5f);
  setF(uRefract, 0.6f);
  setF(uCorner, 0.2f);
  setF(uSat, 1.2f);
  setF(uContrast, 1.05f);
  setF(uChroma, 1.0f);
  setF(uFBias, 0.0f);
  setF(uFScale, 0.12f);
  setF(uFPower, 1.5f);
  setF(uSpecInt, 0.08f);
  setF(uSpecSize, 0.3f);
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
  const float panelX = pad;
  const float panelY = pad;
  const float glassW = rm_min(780.0f, m_size.x - pad * 2.0f);
  const float glassH = 380.0f;

  if (glassW <= 0.0f || glassH <= 0.0f) {
    return;
  }

  draw_glass_showcase(ctx, panelX, panelY, glassW, glassH);

  // Glow demo to the right of the glass panels, if there's room
  const float glowX = panelX + glassW + 24.0f;
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

  ctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  draw_blur_gallery(ctx, pad, panelY + glassH + 64.0f, m_size.x - pad * 2.0f);
}
