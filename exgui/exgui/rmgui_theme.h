#pragma once

#include "nanovg.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class RmVisualState {
  normal,
  hovered,
  pressed,
  disabled
};

enum class RmThemeMode {
  dark,
  light
};

struct RmStateColors {
  NVGcolor normal;
  NVGcolor hovered;
  NVGcolor pressed;
  NVGcolor disabled;

  NVGcolor resolve(RmVisualState state) const noexcept {
    switch (state) {
    case RmVisualState::hovered: return hovered;
    case RmVisualState::pressed: return pressed;
    case RmVisualState::disabled: return disabled;
    case RmVisualState::normal:
    default: return normal;
    }
  }
};

struct RmButtonStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors text;
  NVGcolor focus_ring;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float font_size = 0.0f;
};

struct RmLabelStyle {
  NVGcolor text;
  NVGcolor disabled_text;
  float font_size = 0.0f;
};

struct RmCheckboxStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors mark;
  RmStateColors text;
  NVGcolor focus_ring;
  float box_size = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float mark_width = 0.0f;
  float text_gap = 0.0f;
  float font_size = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmSliderStyle {
  RmStateColors track;
  RmStateColors fill;
  RmStateColors thumb;
  RmStateColors thumb_border;
  NVGcolor focus_ring;
  float track_height = 0.0f;
  float padding = 0.0f;
  float thumb_radius = 0.0f;
  float thumb_border_width = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmProgressStyle {
  RmStateColors background;
  RmStateColors fill_start;
  RmStateColors fill_end;
  float corner_radius = 0.0f;
};

struct RmSwitchStyle {
  RmStateColors track_off;
  RmStateColors track_on;
  RmStateColors knob;
  NVGcolor focus_ring;
  NVGcolor shadow;
  float track_height = 0.0f;
  float padding = 0.0f;
  float knob_radius = 0.0f;
  float corner_radius = 0.0f;
  float animation_duration = 0.0f;
  float shadow_offset = 0.0f;
  float shadow_size = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmColorTokens {
  NVGcolor accent;
  NVGcolor accent_hovered;
  NVGcolor accent_pressed;
  NVGcolor accent_disabled;
  NVGcolor accent_secondary;
  NVGcolor surface;
  NVGcolor surface_elevated;
  NVGcolor control;
  NVGcolor control_hovered;
  NVGcolor control_pressed;
  NVGcolor control_disabled;
  NVGcolor border;
  NVGcolor border_hovered;
  NVGcolor border_pressed;
  NVGcolor border_disabled;
  NVGcolor text;
  NVGcolor text_muted;
  NVGcolor text_disabled;
  NVGcolor text_on_accent;
  NVGcolor focus_ring;
  NVGcolor shadow;
};

struct RmSpacingTokens {
  float xsmall = 4.0f;
  float small = 8.0f;
  float medium = 12.0f;
  float large = 16.0f;
  float xlarge = 24.0f;
};

struct RmRadiusTokens {
  float small = 3.0f;
  float medium = 6.0f;
  float large = 10.0f;
  float pill = 999.0f;
};

struct RmTypographyTokens {
  float caption = 13.0f;
  float body = 16.0f;
  float control = 16.0f;
  float title = 20.0f;
};

struct RmAnimationTokens {
  float fast = 0.12f;
  float normal = 0.20f;
  float slow = 0.32f;
};

struct RmControlMetricsTokens {
  float border_width = 1.0f;
  float focus_ring_width = 2.0f;
  float checkbox_size = 20.0f;
  float checkbox_mark_width = 2.25f;
  float slider_track_height = 4.0f;
  float slider_padding = 8.0f;
  float slider_thumb_radius = 10.0f;
  float slider_thumb_border_width = 2.0f;
  float progress_corner_radius = 4.0f;
  float switch_track_height = 24.0f;
  float switch_padding = 4.0f;
  float switch_knob_radius = 11.0f;
  float switch_shadow_offset = 5.0f;
  float switch_shadow_size = 7.0f;
};

struct RmThemeTokens {
  RmColorTokens colors;
  RmSpacingTokens spacing;
  RmRadiusTokens radius;
  RmTypographyTokens typography;
  RmAnimationTokens animation;
  RmControlMetricsTokens controls;
};

struct RmThemeDocument {
  std::string name;
  RmThemeMode mode = RmThemeMode::dark;
  RmThemeTokens tokens;

  static RmThemeDocument dark_theme();
  static RmThemeDocument light_theme();
};

struct RmThemeSnapshot {
  std::string name;
  RmThemeMode mode = RmThemeMode::dark;
  RmThemeTokens tokens;
  RmButtonStyle button;
  RmLabelStyle label;
  RmCheckboxStyle checkbox;
  RmSliderStyle slider;
  RmProgressStyle progress;
  RmSwitchStyle switch_control;

  static std::shared_ptr<const RmThemeSnapshot> default_theme();
};

using RmThemeRef = std::shared_ptr<const RmThemeSnapshot>;

enum class RmThemeDiagnosticSeverity {
  warning,
  error
};

struct RmThemeDiagnostic {
  RmThemeDiagnosticSeverity severity = RmThemeDiagnosticSeverity::warning;
  std::string property;
  std::string message;
};

struct RmThemeCompileResult {
  RmThemeRef theme;
  std::vector<RmThemeDiagnostic> diagnostics;

  bool succeeded() const noexcept {
    if (!theme)
      return false;
    for (const RmThemeDiagnostic& diagnostic : diagnostics) {
      if (diagnostic.severity == RmThemeDiagnosticSeverity::error)
        return false;
    }
    return true;
  }
};

class RmThemeCompiler {
  static void sanitize_metric(float& value, float minimum, float maximum,
    const char* p_property, std::vector<RmThemeDiagnostic>& diagnostics)
  {
    const float sanitized = std::clamp(value, minimum, maximum);
    if (sanitized == value)
      return;
    value = sanitized;
    diagnostics.push_back({ RmThemeDiagnosticSeverity::warning, p_property,
      "Value was outside the supported range and has been clamped." });
  }

  static void sanitize_color(NVGcolor& color, const char* p_property,
    std::vector<RmThemeDiagnostic>& diagnostics)
  {
    const NVGcolor original = color;
    color.r = std::clamp(color.r, 0.0f, 1.0f);
    color.g = std::clamp(color.g, 0.0f, 1.0f);
    color.b = std::clamp(color.b, 0.0f, 1.0f);
    color.a = std::clamp(color.a, 0.0f, 1.0f);
    if (original.r != color.r || original.g != color.g ||
      original.b != color.b || original.a != color.a) {
      diagnostics.push_back({ RmThemeDiagnosticSeverity::warning, p_property,
        "Color components were clamped to the normalized RGBA range." });
    }
  }

  static void sanitize_tokens(RmThemeTokens& tokens,
    std::vector<RmThemeDiagnostic>& diagnostics)
  {
    NVGcolor* colors[] = {
      &tokens.colors.accent, &tokens.colors.accent_hovered,
      &tokens.colors.accent_pressed, &tokens.colors.accent_disabled,
      &tokens.colors.accent_secondary, &tokens.colors.surface,
      &tokens.colors.surface_elevated, &tokens.colors.control,
      &tokens.colors.control_hovered, &tokens.colors.control_pressed,
      &tokens.colors.control_disabled, &tokens.colors.border,
      &tokens.colors.border_hovered, &tokens.colors.border_pressed,
      &tokens.colors.border_disabled, &tokens.colors.text,
      &tokens.colors.text_muted, &tokens.colors.text_disabled,
      &tokens.colors.text_on_accent, &tokens.colors.focus_ring,
      &tokens.colors.shadow
    };
    for (NVGcolor* p_color : colors)
      sanitize_color(*p_color, "tokens.colors", diagnostics);

    sanitize_metric(tokens.spacing.xsmall, 0.0f, 128.0f, "tokens.spacing.xsmall", diagnostics);
    sanitize_metric(tokens.spacing.small, 0.0f, 128.0f, "tokens.spacing.small", diagnostics);
    sanitize_metric(tokens.spacing.medium, 0.0f, 128.0f, "tokens.spacing.medium", diagnostics);
    sanitize_metric(tokens.spacing.large, 0.0f, 128.0f, "tokens.spacing.large", diagnostics);
    sanitize_metric(tokens.spacing.xlarge, 0.0f, 128.0f, "tokens.spacing.xlarge", diagnostics);
    sanitize_metric(tokens.radius.small, 0.0f, 1024.0f, "tokens.radius.small", diagnostics);
    sanitize_metric(tokens.radius.medium, 0.0f, 1024.0f, "tokens.radius.medium", diagnostics);
    sanitize_metric(tokens.radius.large, 0.0f, 1024.0f, "tokens.radius.large", diagnostics);
    sanitize_metric(tokens.radius.pill, 0.0f, 4096.0f, "tokens.radius.pill", diagnostics);
    sanitize_metric(tokens.typography.caption, 1.0f, 256.0f, "tokens.typography.caption", diagnostics);
    sanitize_metric(tokens.typography.body, 1.0f, 256.0f, "tokens.typography.body", diagnostics);
    sanitize_metric(tokens.typography.control, 1.0f, 256.0f, "tokens.typography.control", diagnostics);
    sanitize_metric(tokens.typography.title, 1.0f, 256.0f, "tokens.typography.title", diagnostics);
    sanitize_metric(tokens.animation.fast, 0.0001f, 10.0f, "tokens.animation.fast", diagnostics);
    sanitize_metric(tokens.animation.normal, 0.0001f, 10.0f, "tokens.animation.normal", diagnostics);
    sanitize_metric(tokens.animation.slow, 0.0001f, 10.0f, "tokens.animation.slow", diagnostics);
    sanitize_metric(tokens.controls.border_width, 0.0f, 32.0f, "tokens.controls.border_width", diagnostics);
    sanitize_metric(tokens.controls.focus_ring_width, 0.0f, 32.0f, "tokens.controls.focus_ring_width", diagnostics);
    sanitize_metric(tokens.controls.checkbox_size, 1.0f, 256.0f, "tokens.controls.checkbox_size", diagnostics);
    sanitize_metric(tokens.controls.checkbox_mark_width, 0.0f, 32.0f, "tokens.controls.checkbox_mark_width", diagnostics);
    sanitize_metric(tokens.controls.slider_track_height, 1.0f, 256.0f, "tokens.controls.slider_track_height", diagnostics);
    sanitize_metric(tokens.controls.slider_padding, 0.0f, 256.0f, "tokens.controls.slider_padding", diagnostics);
    sanitize_metric(tokens.controls.slider_thumb_radius, 1.0f, 256.0f, "tokens.controls.slider_thumb_radius", diagnostics);
    sanitize_metric(tokens.controls.slider_thumb_border_width, 0.0f, 32.0f, "tokens.controls.slider_thumb_border_width", diagnostics);
    sanitize_metric(tokens.controls.progress_corner_radius, 0.0f, 1024.0f, "tokens.controls.progress_corner_radius", diagnostics);
    sanitize_metric(tokens.controls.switch_track_height, 1.0f, 256.0f, "tokens.controls.switch_track_height", diagnostics);
    sanitize_metric(tokens.controls.switch_padding, 0.0f, 256.0f, "tokens.controls.switch_padding", diagnostics);
    sanitize_metric(tokens.controls.switch_knob_radius, 1.0f, 256.0f, "tokens.controls.switch_knob_radius", diagnostics);
    sanitize_metric(tokens.controls.switch_shadow_offset, 0.0f, 256.0f, "tokens.controls.switch_shadow_offset", diagnostics);
    sanitize_metric(tokens.controls.switch_shadow_size, 0.0f, 256.0f, "tokens.controls.switch_shadow_size", diagnostics);
  }

public:
  static RmThemeCompileResult compile(const RmThemeDocument& document)
  {
    RmThemeCompileResult result;
    auto p_theme = std::make_shared<RmThemeSnapshot>();
    p_theme->name = document.name.empty() ? "Unnamed theme" : document.name;
    p_theme->mode = document.mode;
    p_theme->tokens = document.tokens;
    if (document.name.empty()) {
      result.diagnostics.push_back({ RmThemeDiagnosticSeverity::warning,
        "name", "An unnamed theme was compiled with a generated display name." });
    }
    sanitize_tokens(p_theme->tokens, result.diagnostics);

    const RmThemeTokens& tokens = p_theme->tokens;
    const RmColorTokens& colors = tokens.colors;
    const RmControlMetricsTokens& controls = tokens.controls;

    p_theme->button.background = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->button.border = { colors.accent_hovered, colors.focus_ring,
      colors.accent_pressed, colors.border_disabled };
    p_theme->button.text = { colors.text_on_accent, colors.text_on_accent,
      colors.text_on_accent, colors.text_disabled };
    p_theme->button.focus_ring = colors.focus_ring;
    p_theme->button.corner_radius = tokens.radius.medium;
    p_theme->button.border_width = controls.border_width;
    p_theme->button.focus_ring_width = controls.focus_ring_width;
    p_theme->button.font_size = tokens.typography.control;

    p_theme->label.text = colors.text;
    p_theme->label.disabled_text = colors.text_disabled;
    p_theme->label.font_size = tokens.typography.body;

    p_theme->checkbox.background = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->checkbox.border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->checkbox.mark = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->checkbox.text = { colors.text, colors.text, colors.text, colors.text_disabled };
    p_theme->checkbox.focus_ring = colors.focus_ring;
    p_theme->checkbox.box_size = controls.checkbox_size;
    p_theme->checkbox.corner_radius = tokens.radius.small;
    p_theme->checkbox.border_width = controls.border_width;
    p_theme->checkbox.mark_width = controls.checkbox_mark_width;
    p_theme->checkbox.text_gap = tokens.spacing.small;
    p_theme->checkbox.font_size = tokens.typography.control;
    p_theme->checkbox.focus_ring_width = controls.focus_ring_width;

    p_theme->slider.track = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->slider.fill = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->slider.thumb = { colors.text_on_accent, colors.text_on_accent,
      colors.control_pressed, colors.text_disabled };
    p_theme->slider.thumb_border = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.border_disabled };
    p_theme->slider.focus_ring = colors.focus_ring;
    p_theme->slider.track_height = controls.slider_track_height;
    p_theme->slider.padding = controls.slider_padding;
    p_theme->slider.thumb_radius = controls.slider_thumb_radius;
    p_theme->slider.thumb_border_width = controls.slider_thumb_border_width;
    p_theme->slider.focus_ring_width = controls.focus_ring_width;

    p_theme->progress.background = { colors.control, colors.control,
      colors.control, colors.control_disabled };
    p_theme->progress.fill_start = { colors.accent, colors.accent,
      colors.accent, colors.accent_disabled };
    p_theme->progress.fill_end = { colors.accent_secondary, colors.accent_secondary,
      colors.accent_secondary, colors.accent_disabled };
    p_theme->progress.corner_radius = controls.progress_corner_radius;

    p_theme->switch_control.track_off = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->switch_control.track_on = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->switch_control.knob = { colors.text_on_accent, colors.text_on_accent,
      colors.control_pressed, colors.text_disabled };
    p_theme->switch_control.focus_ring = colors.focus_ring;
    p_theme->switch_control.shadow = colors.shadow;
    p_theme->switch_control.track_height = controls.switch_track_height;
    p_theme->switch_control.padding = controls.switch_padding;
    p_theme->switch_control.knob_radius = controls.switch_knob_radius;
    p_theme->switch_control.corner_radius = controls.switch_track_height * 0.5f;
    p_theme->switch_control.animation_duration = tokens.animation.normal;
    p_theme->switch_control.shadow_offset = controls.switch_shadow_offset;
    p_theme->switch_control.shadow_size = controls.switch_shadow_size;
    p_theme->switch_control.focus_ring_width = controls.focus_ring_width;

    result.theme = std::move(p_theme);
    return result;
  }
};

inline RmThemeDocument RmThemeDocument::dark_theme()
{
  RmThemeDocument document;
  document.name = "RmGUI Dark";
  document.mode = RmThemeMode::dark;
  RmColorTokens& colors = document.tokens.colors;
  colors.accent = NVGcolor::RGBA(76, 103, 235, 255);
  colors.accent_hovered = NVGcolor::RGBA(96, 125, 249, 255);
  colors.accent_pressed = NVGcolor::RGBA(55, 76, 196, 255);
  colors.accent_disabled = NVGcolor::RGBA(82, 91, 135, 255);
  colors.accent_secondary = NVGcolor::RGBA(151, 89, 232, 255);
  colors.surface = NVGcolor::RGBA(21, 23, 29, 255);
  colors.surface_elevated = NVGcolor::RGBA(31, 34, 42, 255);
  colors.control = NVGcolor::RGBA(47, 50, 61, 255);
  colors.control_hovered = NVGcolor::RGBA(58, 62, 76, 255);
  colors.control_pressed = NVGcolor::RGBA(38, 41, 51, 255);
  colors.control_disabled = NVGcolor::RGBA(43, 45, 52, 255);
  colors.border = NVGcolor::RGBA(83, 88, 105, 255);
  colors.border_hovered = NVGcolor::RGBA(112, 126, 190, 255);
  colors.border_pressed = NVGcolor::RGBA(76, 93, 180, 255);
  colors.border_disabled = NVGcolor::RGBA(68, 70, 78, 255);
  colors.text = NVGcolor::RGBA(241, 243, 248, 255);
  colors.text_muted = NVGcolor::RGBA(177, 181, 193, 255);
  colors.text_disabled = NVGcolor::RGBA(124, 127, 137, 255);
  colors.text_on_accent = NVGcolor::RGBA(255, 255, 255, 255);
  colors.focus_ring = NVGcolor::RGBA(143, 184, 255, 235);
  colors.shadow = NVGcolor::RGBA(0, 0, 0, 90);
  return document;
}

inline RmThemeDocument RmThemeDocument::light_theme()
{
  RmThemeDocument document;
  document.name = "RmGUI Light";
  document.mode = RmThemeMode::light;
  RmColorTokens& colors = document.tokens.colors;
  colors.accent = NVGcolor::RGBA(50, 83, 210, 255);
  colors.accent_hovered = NVGcolor::RGBA(65, 101, 225, 255);
  colors.accent_pressed = NVGcolor::RGBA(39, 64, 169, 255);
  colors.accent_disabled = NVGcolor::RGBA(145, 155, 197, 255);
  colors.accent_secondary = NVGcolor::RGBA(125, 69, 190, 255);
  colors.surface = NVGcolor::RGBA(245, 247, 251, 255);
  colors.surface_elevated = NVGcolor::RGBA(255, 255, 255, 255);
  colors.control = NVGcolor::RGBA(255, 255, 255, 255);
  colors.control_hovered = NVGcolor::RGBA(246, 248, 255, 255);
  colors.control_pressed = NVGcolor::RGBA(230, 235, 252, 255);
  colors.control_disabled = NVGcolor::RGBA(232, 234, 239, 255);
  colors.border = NVGcolor::RGBA(164, 170, 184, 255);
  colors.border_hovered = NVGcolor::RGBA(77, 105, 215, 255);
  colors.border_pressed = NVGcolor::RGBA(50, 83, 210, 255);
  colors.border_disabled = NVGcolor::RGBA(195, 198, 205, 255);
  colors.text = NVGcolor::RGBA(31, 34, 42, 255);
  colors.text_muted = NVGcolor::RGBA(91, 96, 109, 255);
  colors.text_disabled = NVGcolor::RGBA(142, 146, 156, 255);
  colors.text_on_accent = NVGcolor::RGBA(255, 255, 255, 255);
  colors.focus_ring = NVGcolor::RGBA(74, 124, 237, 220);
  colors.shadow = NVGcolor::RGBA(21, 27, 43, 55);
  return document;
}

inline RmThemeRef RmThemeSnapshot::default_theme()
{
  static const RmThemeRef theme = RmThemeCompiler::compile(
    RmThemeDocument::dark_theme()).theme;
  return theme;
}
