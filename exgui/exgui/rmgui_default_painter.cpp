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

void RmDefaultControlPainter::draw_tab_bar(NVGcontext& context, float x, float y,
  float width, float height, const RmTabStyle& style)
{
  context.beginPath();
  context.rect(x, y, std::max(0.0f, width), std::max(0.0f, height));
  context.fillColor(style.bar_background);
  context.fill();
}

void RmDefaultControlPainter::draw_tab_page(NVGcontext& context, float x, float y,
  float width, float height, const RmTabStyle& style)
{
  context.beginPath();
  context.rect(x, y, std::max(0.0f, width), std::max(0.0f, height));
  context.fillColor(style.page_background);
  context.fill();
  if (style.show_page_border && style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.page_border);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_tab(NVGcontext& context,
  const RmTabVisual& visual, const RmTabStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.pressed);
  const RmStateColors& backgrounds = visual.selected
    ? style.selected_background : style.background;
  const RmStateColors& borders = visual.selected
    ? style.selected_border : style.border;
  const RmStateColors& text = visual.selected
    ? style.selected_text : style.text;
  const float half_border = style.border_width * 0.5f;

  context.beginPath();
  context.roundedRect(visual.x + half_border, visual.y + half_border,
    std::max(0.0f, visual.width - style.border_width),
    std::max(0.0f, visual.height - style.border_width), style.corner_radius);
  context.fillColor(backgrounds.resolve(state));
  context.fill();
  if (style.border_width > 0.0f && borders.resolve(state).a > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(borders.resolve(state));
    context.stroke();
  }

  if (visual.selected && style.show_indicator && style.indicator_thickness > 0.0f) {
    context.beginPath();
    switch (visual.placement) {
    case RmTabPlacement::bottom:
      context.rect(visual.x, visual.y, visual.width, style.indicator_thickness);
      break;
    case RmTabPlacement::left:
      context.rect(visual.x + visual.width - style.indicator_thickness, visual.y,
        style.indicator_thickness, visual.height);
      break;
    case RmTabPlacement::right:
      context.rect(visual.x, visual.y, style.indicator_thickness, visual.height);
      break;
    case RmTabPlacement::top:
    default:
      context.rect(visual.x, visual.y + visual.height - style.indicator_thickness,
        visual.width, style.indicator_thickness);
      break;
    }
    context.fillColor(style.indicator);
    context.fill();
  }

  if (visual.focused && visual.selected && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(visual.x + style.focus_ring_width * 0.5f,
      visual.y + style.focus_ring_width * 0.5f,
      std::max(0.0f, visual.width - style.focus_ring_width),
      std::max(0.0f, visual.height - style.focus_ring_width), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  const float close_region = visual.closable
    ? style.close_size + style.horizontal_padding * 0.5f : 0.0f;
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  context.fillColor(text.resolve(state));
  context.text(visual.x + (visual.width - close_region) * 0.5f,
    visual.y + visual.height * 0.5f, visual.text ? visual.text : "", nullptr);

  if (visual.closable) {
    const float cx = visual.x + visual.width - style.horizontal_padding * 0.5f
      - style.close_size * 0.5f;
    const float cy = visual.y + visual.height * 0.5f;
    const float radius = style.close_size * 0.28f;
    context.beginPath();
    context.moveTo(cx - radius, cy - radius);
    context.lineTo(cx + radius, cy + radius);
    context.moveTo(cx + radius, cy - radius);
    context.lineTo(cx - radius, cy + radius);
    context.StrokeWidth(visual.close_hovered ? 2.0f : 1.5f);
    context.strokeColor(style.close_icon.resolve(
      visual.close_hovered ? RmVisualState::hovered : state));
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_menu_surface(NVGcontext& context,
  const RmMenuSurfaceVisual& visual, const RmMenuStyle& style)
{
  if (visual.popup && style.shadow_size > 0.0f) {
    const NVGcolor transparent = NVGcolor::RGBA(0, 0, 0, 0);
    const NVGpaint shadow = NVGpaint::boxGradient(0.0f, 3.0f,
      visual.width, visual.height, style.corner_radius * 2.0f,
      style.shadow_size, style.shadow, transparent);
    context.save();
    context.resetScissor();
    context.beginPath();
    context.rect(-style.shadow_size, -style.shadow_size,
      visual.width + style.shadow_size * 2.0f,
      visual.height + style.shadow_size * 2.0f);
    context.roundedRect(0.0f, 0.0f, visual.width, visual.height,
      style.corner_radius);
    context.pathWinding(NVG_HOLE);
    context.fillPaint(shadow);
    context.fill();
    context.restore();
  }

  context.beginPath();
  if (visual.popup)
    context.roundedRect(0.0f, 0.0f, visual.width, visual.height,
      style.corner_radius);
  else
    context.rect(0.0f, 0.0f, visual.width, visual.height);
  context.fillColor(visual.popup ? style.popup_background : style.bar_background);
  context.fill();
  if (visual.popup && style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.popup_border);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_menu_item(NVGcontext& context,
  const RmMenuItemVisual& visual, const RmMenuStyle& style)
{
  if (visual.separator) {
    const float y = visual.y + visual.height * 0.5f;
    context.beginPath();
    context.moveTo(visual.x + style.separator_margin, y);
    context.lineTo(visual.x + visual.width - style.separator_margin, y);
    context.StrokeWidth(style.separator_thickness);
    context.strokeColor(style.separator);
    context.stroke();
    return;
  }

  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered || visual.opened, visual.pressed);
  const NVGcolor background = style.item_background.resolve(state);
  if (background.a > 0.0f) {
    context.beginPath();
    context.roundedRect(visual.x, visual.y, visual.width, visual.height,
      style.corner_radius * 0.65f);
    context.fillColor(background);
    context.fill();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign((visual.root_item ? NVG_ALIGN_CENTER : NVG_ALIGN_LEFT) |
    NVG_ALIGN_MIDDLE);
  context.fillColor(style.item_text.resolve(state));
  const float text_x = visual.root_item
    ? visual.x + visual.width * 0.5f
    : visual.x + style.horizontal_padding;
  context.text(text_x, visual.y + visual.height * 0.5f,
    visual.text ? visual.text : "", nullptr);

  if (visual.has_submenu && !visual.root_item) {
    const float size = style.submenu_indicator_size;
    const float cx = visual.x + visual.width - style.horizontal_padding;
    const float cy = visual.y + visual.height * 0.5f;
    context.beginPath();
    context.moveTo(cx - size * 0.5f, cy - size);
    context.lineTo(cx + size * 0.5f, cy);
    context.lineTo(cx - size * 0.5f, cy + size);
    context.closePath();
    context.fillColor(style.item_icon.resolve(state));
    context.fill();
  }
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

void RmDefaultControlPainter::draw_radiobutton(NVGcontext& context,
  const RmRadioButtonVisual& visual, const RmRadioButtonStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.pressed);
  const NVGcolor row_background = style.background.resolve(state);
  if (row_background.a > 0.0f) {
    context.beginPath();
    context.roundedRect(0.0f, 0.0f, visual.width, visual.height,
      style.corner_radius);
    context.fillColor(row_background);
    context.fill();
  }

  const float size = std::min(style.indicator_size, visual.height);
  const float radius = size * 0.5f;
  const float cx = style.horizontal_padding + radius;
  const float cy = visual.height * 0.5f;
  if (style.shadow_size > 0.0f) {
    const NVGcolor transparent = NVGcolor::RGBA(0, 0, 0, 0);
    context.beginPath();
    context.circle(cx, cy + style.shadow_offset, radius + style.shadow_size);
    context.fillPaint(NVGpaint::radialGradient(cx, cy + style.shadow_offset,
      radius, radius + style.shadow_size, style.shadow, transparent));
    context.fill();
  }

  context.beginPath();
  context.circle(cx, cy, std::max(0.0f, radius - style.border_width * 0.5f));
  context.fillColor(style.indicator_background.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.indicator_border.resolve(state));
    context.stroke();
  }
  if (visual.checked && style.mark_radius > 0.0f) {
    context.beginPath();
    context.circle(cx, cy, std::min(style.mark_radius,
      std::max(0.0f, radius - style.border_width)));
    context.fillColor(style.mark.resolve(state));
    context.fill();
  }
  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.circle(cx, cy, radius + style.focus_ring_width * 0.5f);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.text.resolve(state));
  context.text(style.horizontal_padding + size + style.text_gap,
    visual.height * 0.5f, visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_combobox(NVGcontext& context,
  const RmComboBoxVisual& visual, const RmComboBoxStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.expanded);
  const float half_border = style.border_width * 0.5f;

  context.beginPath();
  context.roundedRect(half_border, half_border,
    std::max(0.0f, visual.width - style.border_width),
    std::max(0.0f, visual.height - style.border_width), style.corner_radius);
  context.fillColor(style.field_background.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.field_border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f,
      style.focus_ring_width * 0.5f,
      std::max(0.0f, visual.width - style.focus_ring_width),
      std::max(0.0f, visual.height - style.focus_ring_width),
      style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(visual.placeholder && visual.enabled
    ? style.placeholder_text : style.field_text.resolve(state));
  context.text(style.horizontal_padding, visual.height * 0.5f,
    visual.text ? visual.text : "", nullptr);

  const float size = style.indicator_size;
  const float cx = visual.width - style.horizontal_padding - size;
  const float cy = visual.height * 0.5f;
  context.beginPath();
  if (visual.expanded) {
    context.moveTo(cx - size, cy + size * 0.5f);
    context.lineTo(cx, cy - size * 0.5f);
    context.lineTo(cx + size, cy + size * 0.5f);
  }
  else {
    context.moveTo(cx - size, cy - size * 0.5f);
    context.lineTo(cx, cy + size * 0.5f);
    context.lineTo(cx + size, cy - size * 0.5f);
  }
  context.StrokeWidth(std::max(1.0f, style.border_width));
  context.strokeColor(style.indicator.resolve(state));
  context.stroke();
}

void RmDefaultControlPainter::draw_combobox_popup(NVGcontext& context,
  const RmComboBoxPopupVisual& visual, const RmComboBoxStyle& style)
{
  if (style.shadow_size > 0.0f) {
    const NVGcolor transparent = NVGcolor::RGBA(0, 0, 0, 0);
    const NVGpaint shadow = NVGpaint::boxGradient(0.0f, visual.y + 3.0f,
      visual.width, visual.height, style.corner_radius * 2.0f,
      style.shadow_size, style.shadow, transparent);
    context.save();
    context.resetScissor();
    context.beginPath();
    context.rect(-style.shadow_size, visual.y - style.shadow_size,
      visual.width + style.shadow_size * 2.0f,
      visual.height + style.shadow_size * 2.0f);
    context.roundedRect(0.0f, visual.y, visual.width, visual.height,
      style.corner_radius);
    context.pathWinding(NVG_HOLE);
    context.fillPaint(shadow);
    context.fill();
    context.restore();
  }

  context.beginPath();
  context.roundedRect(0.0f, visual.y, visual.width, visual.height,
    style.corner_radius);
  context.fillColor(style.popup_background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.popup_border);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_combobox_item(NVGcontext& context,
  const RmComboBoxItemVisual& visual, const RmComboBoxStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered, false);
  const NVGcolor background = style.item_background.resolve(state);
  if (background.a > 0.0f) {
    context.beginPath();
    context.roundedRect(visual.x, visual.y, visual.width, visual.height,
      style.corner_radius * 0.65f);
    context.fillColor(background);
    context.fill();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.item_text.resolve(state));
  context.text(visual.x + style.horizontal_padding,
    visual.y + visual.height * 0.5f, visual.text ? visual.text : "", nullptr);

  if (visual.selected) {
    const float size = style.indicator_size;
    const float cx = visual.x + visual.width - style.horizontal_padding - size;
    const float cy = visual.y + visual.height * 0.5f;
    context.beginPath();
    context.moveTo(cx - size, cy);
    context.lineTo(cx - size * 0.25f, cy + size * 0.7f);
    context.lineTo(cx + size, cy - size * 0.7f);
    context.StrokeWidth(style.selected_mark_width);
    context.strokeColor(style.selected_mark);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_listview_surface(NVGcontext& context,
  const RmListViewSurfaceVisual& visual, const RmListViewStyle& style)
{
  context.beginPath();
  context.roundedRect(0.0f, 0.0f, visual.width, visual.height,
    style.corner_radius);
  context.fillColor(style.background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f,
      style.focus_ring_width * 0.5f,
      std::max(0.0f, visual.width - style.focus_ring_width),
      std::max(0.0f, visual.height - style.focus_ring_width),
      style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_listview_row(NVGcontext& context,
  const RmListViewRowVisual& visual, const RmListViewStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.pressed);
  const RmStateColors& backgrounds = visual.selected
    ? style.selected_background : style.row_background;
  const RmStateColors& text = visual.selected
    ? style.selected_text : style.row_text;
  const NVGcolor background = backgrounds.resolve(state);
  if (background.a > 0.0f) {
    context.beginPath();
    context.roundedRect(visual.x, visual.y, visual.width, visual.height,
      style.row_corner_radius);
    context.fillColor(background);
    context.fill();
  }
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(text.resolve(state));
  context.text(visual.x + style.horizontal_padding,
    visual.y + visual.height * 0.5f, visual.text ? visual.text : "", nullptr);
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

void RmDefaultControlPainter::draw_scrollbar(NVGcontext& context,
  const RmScrollbarVisual& visual, const RmScrollbarStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.dragging);
  const float half_border = style.border_width * 0.5f;
  context.beginPath();
  context.roundedRect(half_border, half_border,
    std::max(0.0f, visual.width - style.border_width),
    std::max(0.0f, visual.height - style.border_width), style.corner_radius);
  context.fillColor(style.track.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.track_border.resolve(state));
    context.stroke();
  }

  const float thumb_x = visual.vertical ? style.padding : visual.thumb_offset + style.padding;
  const float thumb_y = visual.vertical ? visual.thumb_offset + style.padding : style.padding;
  const float thumb_width = visual.vertical
    ? std::max(0.0f, visual.width - style.padding * 2.0f)
    : std::max(0.0f, visual.thumb_length - style.padding * 2.0f);
  const float thumb_height = visual.vertical
    ? std::max(0.0f, visual.thumb_length - style.padding * 2.0f)
    : std::max(0.0f, visual.height - style.padding * 2.0f);
  context.beginPath();
  context.roundedRect(thumb_x, thumb_y, thumb_width, thumb_height,
    style.corner_radius);
  context.fillColor(style.thumb.resolve(state));
  context.fill();
  if (style.thumb_border_width > 0.0f) {
    context.StrokeWidth(style.thumb_border_width);
    context.strokeColor(style.thumb_border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.roundedRect(style.focus_ring_width * 0.5f,
      style.focus_ring_width * 0.5f,
      std::max(0.0f, visual.width - style.focus_ring_width),
      std::max(0.0f, visual.height - style.focus_ring_width),
      style.corner_radius);
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
