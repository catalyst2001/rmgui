#include "rmgui_default_painter.h"

#include <algorithm>

namespace {

RmVisualState resolve_state(const RmButtonVisual& visual)
{
  if (!visual.enabled)
    return RmVisualState::disabled;
  if (visual.pressed)
    return RmVisualState::pressed;
  if (visual.hovered)
    return RmVisualState::hovered;
  return RmVisualState::normal;
}

RmVisualState resolve_state(bool enabled, bool hovered, bool pressed)
{
  if (!enabled)
    return RmVisualState::disabled;
  if (pressed)
    return RmVisualState::pressed;
  if (hovered)
    return RmVisualState::hovered;
  return RmVisualState::normal;
}

NVGcolor lerp_color(const NVGcolor& from, const NVGcolor& to, float amount)
{
  amount = std::clamp(amount, 0.0f, 1.0f);
  NVGcolor result;
  result.r = from.r + (to.r - from.r) * amount;
  result.g = from.g + (to.g - from.g) * amount;
  result.b = from.b + (to.b - from.b) * amount;
  result.a = from.a + (to.a - from.a) * amount;
  return result;
}

} // namespace

void RmDefaultControlPainter::draw_button(NVGcontext& context, const RmButtonVisual& visual,
  const RmButtonStyle& style)
{
  const RmVisualState state = resolve_state(visual);
  const float half_border = style.border_width * 0.5f;

  context.beginPath();
  context.roundedRect(half_border, half_border,
    visual.width - style.border_width, visual.height - style.border_width,
    style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f, style.focus_ring_width * 0.5f,
      visual.width - style.focus_ring_width, visual.height - style.focus_ring_width,
      style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  context.fillColor(style.text.resolve(state));
  context.text(visual.width * 0.5f, visual.height * 0.5f,
    visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_label(NVGcontext& context, const RmLabelVisual& visual,
  const RmLabelStyle& style)
{
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(visual.enabled ? style.text : style.disabled_text);
  context.text(0.0f, visual.height * 0.5f, visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_checkbox(NVGcontext& context,
  const RmCheckboxVisual& visual, const RmCheckboxStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered, visual.pressed);
  const float box_size = std::min(style.box_size, visual.height);
  const float box_y = (visual.height - box_size) * 0.5f;
  const float half_border = style.border_width * 0.5f;

  context.beginPath();
  context.roundedRect(half_border, box_y + half_border,
    box_size - style.border_width, box_size - style.border_width,
    style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border.resolve(state));
    context.stroke();
  }

  if (visual.checked) {
    context.beginPath();
    context.moveTo(box_size * 0.22f, box_y + box_size * 0.52f);
    context.lineTo(box_size * 0.43f, box_y + box_size * 0.72f);
    context.lineTo(box_size * 0.79f, box_y + box_size * 0.29f);
    context.StrokeWidth(style.mark_width);
    context.strokeColor(style.mark.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f,
      box_y + style.focus_ring_width * 0.5f,
      box_size - style.focus_ring_width,
      box_size - style.focus_ring_width,
      style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.text.resolve(state));
  context.text(box_size + style.text_gap, visual.height * 0.5f,
    visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_slider(NVGcontext& context,
  const RmSliderVisual& visual, const RmSliderStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered, visual.dragging);
  const float fraction = std::clamp(visual.fraction, 0.0f, 1.0f);
  const float track_width = std::max(0.0f, visual.width - style.padding * 2.0f);
  const float track_y = (visual.height - style.track_height) * 0.5f;
  const float track_radius = style.track_height * 0.5f;
  const float thumb_x = style.padding + track_width * fraction;
  const float thumb_y = visual.height * 0.5f;

  context.beginPath();
  context.roundedRect(style.padding, track_y, track_width, style.track_height, track_radius);
  context.fillColor(style.track.resolve(state));
  context.fill();

  if (fraction > 0.0f) {
    context.beginPath();
    context.roundedRect(style.padding, track_y, track_width * fraction,
      style.track_height, track_radius);
    context.fillColor(style.fill.resolve(state));
    context.fill();
  }

  context.beginPath();
  context.circle(thumb_x, thumb_y, style.thumb_radius);
  context.fillColor(style.thumb.resolve(state));
  context.fill();
  if (style.thumb_border_width > 0.0f) {
    context.StrokeWidth(style.thumb_border_width);
    context.strokeColor(style.thumb_border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.circle(thumb_x, thumb_y, style.thumb_radius + style.focus_ring_width);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_progress(NVGcontext& context,
  const RmProgressVisual& visual, const RmProgressStyle& style)
{
  const RmVisualState state = visual.enabled
    ? RmVisualState::normal : RmVisualState::disabled;
  const float width = std::max(0.0f, visual.width);
  const float height = std::max(0.0f, visual.height);
  const float fraction = std::clamp(visual.fraction, 0.0f, 1.0f);
  const float fill_width = width * fraction;

  context.beginPath();
  context.roundedRect(0.0f, 0.0f, width, height, style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();

  if (fill_width <= 0.0f)
    return;

  const NVGpaint fill = NVGpaint::linearGradient(0.0f, 0.0f,
    fill_width, 0.0f, style.fill_start.resolve(state), style.fill_end.resolve(state));
  context.beginPath();
  context.roundedRect(0.0f, 0.0f, fill_width, height, style.corner_radius);
  context.fillPaint(fill);
  context.fill();
}

void RmDefaultControlPainter::draw_switch(NVGcontext& context,
  const RmSwitchVisual& visual, const RmSwitchStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered, visual.pressed);
  const float width = std::max(0.0f, visual.width);
  const float height = std::max(0.0f, visual.height);
  const float progress = std::clamp(visual.animation_progress, 0.0f, 1.0f);
  const float eased = progress * progress * (3.0f - 2.0f * progress);

  if (style.shadow_size > 0.0f) {
    const NVGcolor transparent = NVGcolor::RGBA(0, 0, 0, 0);
    const NVGpaint shadow = NVGpaint::boxGradient(0.0f, style.shadow_offset,
      width, height, style.corner_radius * 2.0f, style.shadow_size * 2.0f,
      style.shadow, transparent);
    context.save();
    context.resetScissor();
    context.beginPath();
    context.rect(-style.shadow_size, -style.shadow_size + style.shadow_offset,
      width + style.shadow_size * 2.0f, height + style.shadow_size * 2.0f);
    context.roundedRect(1.0f, 1.0f, std::max(0.0f, width - 2.0f),
      std::max(0.0f, height - 2.0f), style.corner_radius);
    context.pathWinding(NVG_HOLE);
    context.fillPaint(shadow);
    context.fill();
    context.restore();
  }

  const NVGcolor track = lerp_color(style.track_off.resolve(state),
    style.track_on.resolve(state), eased);
  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, width - 1.0f),
    std::max(0.0f, height - 1.0f), style.corner_radius);
  context.fillColor(track);
  context.fill();

  const float radius = std::min(style.knob_radius, height * 0.5f);
  const float start_x = style.padding + radius;
  const float end_x = std::max(start_x, width - style.padding - radius);
  const float knob_x = start_x + (end_x - start_x) * eased;
  const float knob_y = height * 0.5f;
  context.beginPath();
  context.circle(knob_x, knob_y, radius);
  context.fillColor(style.knob.resolve(state));
  context.fill();

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f,
      style.focus_ring_width * 0.5f,
      std::max(0.0f, width - style.focus_ring_width),
      std::max(0.0f, height - style.focus_ring_width), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}
