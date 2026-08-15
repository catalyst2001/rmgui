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

bool is_utf8_continuation(unsigned char value)
{
  return (value & 0xc0u) == 0x80u;
}

void build_text_line_layout(NVGcontext& context, RmTextInputLineLayout& line)
{
  line.byte_offsets.clear();
  line.glyph_positions.clear();
  line.byte_offsets.push_back(0);
  line.glyph_positions.push_back(0.0f);
  for (size_t index = 0; index < line.text.size();) {
    ++index;
    while (index < line.text.size() &&
      is_utf8_continuation(static_cast<unsigned char>(line.text[index])))
      ++index;
    line.byte_offsets.push_back(index);
    line.glyph_positions.push_back(context.textBounds(0.0f, 0.0f,
      line.text.c_str(), line.text.c_str() + index, nullptr));
  }
}

float text_x_at(const RmTextInputLineLayout& line, size_t global_offset)
{
  const size_t local_offset = global_offset <= line.text_start
    ? 0 : std::min(global_offset - line.text_start, line.text.size());
  const auto found = std::lower_bound(line.byte_offsets.begin(),
    line.byte_offsets.end(), local_offset);
  if (found == line.byte_offsets.end())
    return line.glyph_positions.empty() ? 0.0f : line.glyph_positions.back();
  return line.glyph_positions[static_cast<size_t>(found - line.byte_offsets.begin())];
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

RmTextInputLayout RmDefaultControlPainter::layout_text_input(NVGcontext& context,
  const RmTextInputVisual& visual, const RmTextInputStyle& style)
{
  RmTextInputLayout layout;
  const std::string text = visual.text ? visual.text : "";
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.textMetrics(&layout.ascender, &layout.descender, &layout.line_height);

  size_t start = 0;
  do {
    const size_t newline = visual.multiline ? text.find('\n', start) : std::string::npos;
    const size_t end = newline == std::string::npos ? text.size() : newline;
    RmTextInputLineLayout line;
    line.text_start = start;
    line.text = text.substr(start, end - start);
    build_text_line_layout(context, line);
    layout.lines.push_back(std::move(line));
    if (newline == std::string::npos)
      break;
    start = newline + 1;
  } while (start <= text.size());

  if (layout.lines.empty()) {
    layout.lines.emplace_back();
    build_text_line_layout(context, layout.lines.back());
  }

  if (visual.multiline) {
    for (size_t index = 0; index < layout.lines.size(); ++index)
      layout.lines[index].baseline = style.vertical_padding + layout.ascender +
        static_cast<float>(index) * layout.line_height;
  }
  else {
    layout.lines.front().baseline = visual.height * 0.5f +
      (layout.ascender + layout.descender) * 0.5f;
  }

  const float available = std::max(0.0f,
    visual.width - style.horizontal_padding * 2.0f);
  float maximum_width = 0.0f;
  for (const RmTextInputLineLayout& line : layout.lines) {
    if (!line.glyph_positions.empty())
      maximum_width = std::max(maximum_width, line.glyph_positions.back());
  }
  const float maximum_scroll = std::max(0.0f, maximum_width - available);
  layout.scroll_offset = std::clamp(visual.scroll_offset, 0.0f, maximum_scroll);

  if (!visual.multiline && !layout.lines.empty()) {
    const float caret = text_x_at(layout.lines.front(), visual.cursor);
    if (caret - layout.scroll_offset > available)
      layout.scroll_offset = caret - available;
    else if (caret < layout.scroll_offset)
      layout.scroll_offset = caret;
    layout.scroll_offset = std::clamp(layout.scroll_offset, 0.0f, maximum_scroll);
  }
  return layout;
}

size_t RmDefaultControlPainter::hit_test_text_input(const RmTextInputLayout& layout,
  float x, float y, const RmTextInputStyle& style)
{
  if (layout.lines.empty())
    return 0;

  size_t line_index = 0;
  if (layout.lines.size() > 1 && layout.line_height > 0.0f) {
    const float line = (y - style.vertical_padding) / layout.line_height;
    line_index = static_cast<size_t>(std::clamp(static_cast<int>(line), 0,
      static_cast<int>(layout.lines.size()) - 1));
  }
  const RmTextInputLineLayout& line = layout.lines[line_index];
  const float local_x = x - style.horizontal_padding + layout.scroll_offset;
  const auto found = std::lower_bound(line.glyph_positions.begin(),
    line.glyph_positions.end(), local_x);
  size_t glyph = static_cast<size_t>(found - line.glyph_positions.begin());
  if (glyph > 0 && glyph < line.glyph_positions.size()) {
    const float left_distance = local_x - line.glyph_positions[glyph - 1];
    const float right_distance = line.glyph_positions[glyph] - local_x;
    if (left_distance < right_distance)
      --glyph;
  }
  glyph = std::min(glyph, line.byte_offsets.size() - 1);
  return line.text_start + line.byte_offsets[glyph];
}

void RmDefaultControlPainter::draw_text_input(NVGcontext& context,
  const RmTextInputVisual& visual, const RmTextInputLayout& layout,
  const RmTextInputStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled,
    visual.hovered, visual.dragging);
  const float width = std::max(0.0f, visual.width);
  const float height = std::max(0.0f, visual.height);

  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, width - 1.0f),
    std::max(0.0f, height - 1.0f), style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.roundedRect(inset, inset, std::max(0.0f, width - inset * 2.0f),
      std::max(0.0f, height - inset * 2.0f), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  context.save();
  context.intersectScissor(style.horizontal_padding, style.vertical_padding,
    std::max(0.0f, width - style.horizontal_padding * 2.0f),
    std::max(0.0f, height - style.vertical_padding * 2.0f));
  const float origin_x = style.horizontal_padding - layout.scroll_offset;
  const size_t selection_first = std::min(visual.selection_start,
    visual.selection_end);
  const size_t selection_last = std::max(visual.selection_start,
    visual.selection_end);

  if (selection_first != selection_last) {
    context.fillColor(style.selection);
    for (const RmTextInputLineLayout& line : layout.lines) {
      const size_t line_first = line.text_start;
      const size_t line_last = line.text_start + line.text.size();
      if (selection_last <= line_first || selection_first >= line_last)
        continue;
      const size_t first = std::max(selection_first, line_first);
      const size_t last = std::min(selection_last, line_last);
      const float x0 = origin_x + text_x_at(line, first);
      const float x1 = origin_x + text_x_at(line, last);
      context.beginPath();
      context.roundedRect(x0, line.baseline - layout.ascender,
        std::max(0.0f, x1 - x0), layout.line_height,
        style.selection_corner_radius);
      context.fill();
    }
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  context.fillColor(style.text.resolve(state));
  for (const RmTextInputLineLayout& line : layout.lines)
    context.text(origin_x, line.baseline, line.text.c_str(), nullptr);

  if (visual.focused && visual.enabled && visual.caret_visible) {
    for (const RmTextInputLineLayout& line : layout.lines) {
      const size_t line_last = line.text_start + line.text.size();
      if (visual.cursor < line.text_start || visual.cursor > line_last)
        continue;
      const float caret_x = origin_x + text_x_at(line, visual.cursor);
      context.beginPath();
      context.moveTo(caret_x, line.baseline - layout.ascender);
      context.lineTo(caret_x, line.baseline - layout.descender);
      context.StrokeWidth(style.caret_width);
      context.strokeColor(style.caret);
      context.stroke();
      break;
    }
  }
  context.restore();
}

void RmDefaultControlPainter::draw_number_input(NVGcontext& context,
  const RmNumberInputVisual& visual, const RmNumberInputStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered, false);
  const float width = std::max(0.0f, visual.width);
  const float height = std::max(0.0f, visual.height);
  const float button_width = std::min(style.button_width, width);
  const float button_x = width - button_width;
  const float half_height = height * 0.5f;

  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, width - 1.0f),
    std::max(0.0f, height - 1.0f), style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();

  const RmVisualState increment_state = resolve_state(visual.enabled,
    visual.increment_hovered, visual.increment_pressed);
  context.beginPath();
  context.roundedRectVarying(button_x, 0.5f, button_width - 0.5f,
    std::max(0.0f, half_height - 0.5f), 0.0f, style.corner_radius,
    0.0f, 0.0f);
  context.fillColor(style.button_background.resolve(increment_state));
  context.fill();

  const RmVisualState decrement_state = resolve_state(visual.enabled,
    visual.decrement_hovered, visual.decrement_pressed);
  context.beginPath();
  context.roundedRectVarying(button_x, half_height, button_width - 0.5f,
    std::max(0.0f, half_height - 0.5f), 0.0f, 0.0f,
    style.corner_radius, 0.0f);
  context.fillColor(style.button_background.resolve(decrement_state));
  context.fill();

  if (style.separator_width > 0.0f) {
    context.beginPath();
    context.moveTo(button_x, 1.0f);
    context.lineTo(button_x, height - 1.0f);
    context.moveTo(button_x, half_height);
    context.lineTo(width - 1.0f, half_height);
    context.StrokeWidth(style.separator_width);
    context.strokeColor(style.separator);
    context.stroke();
  }

  const float icon_half = style.icon_size * 0.5f;
  const float icon_x = button_x + button_width * 0.5f;
  const auto draw_chevron = [&](float center_y, bool upward,
    RmVisualState icon_state) {
    const float direction = upward ? -1.0f : 1.0f;
    context.beginPath();
    context.moveTo(icon_x - icon_half, center_y - direction * icon_half * 0.4f);
    context.lineTo(icon_x, center_y + direction * icon_half * 0.6f);
    context.lineTo(icon_x + icon_half, center_y - direction * icon_half * 0.4f);
    context.StrokeWidth(std::max(1.0f, style.separator_width * 1.5f));
    context.strokeColor(style.button_icon.resolve(icon_state));
    context.stroke();
  };
  draw_chevron(half_height * 0.5f, true, increment_state);
  draw_chevron(half_height + half_height * 0.5f, false, decrement_state);

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.text.resolve(state));
  context.save();
  context.intersectScissor(style.horizontal_padding, 0.0f,
    std::max(0.0f, button_x - style.horizontal_padding * 2.0f), height);
  context.text(style.horizontal_padding, height * 0.5f,
    visual.text ? visual.text : "", nullptr);
  context.restore();

  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, width - 1.0f),
    std::max(0.0f, height - 1.0f), style.corner_radius);
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border.resolve(state));
    context.stroke();
  }

  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.roundedRect(inset, inset, std::max(0.0f, width - inset * 2.0f),
      std::max(0.0f, height - inset * 2.0f), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
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

void RmDefaultControlPainter::draw_treeview_surface(NVGcontext& context,
  const RmTreeViewSurfaceVisual& visual, const RmTreeViewStyle& style)
{
  if (style.draw_background || (style.draw_border && style.border_width > 0.0f)) {
    context.beginPath();
    context.roundedRect(0.5f, 0.5f, std::max(0.0f, visual.width - 1.0f),
      std::max(0.0f, visual.height - 1.0f), style.corner_radius);
  }
  if (style.draw_background) {
    context.fillColor(style.background);
    context.fill();
  }
  if (style.draw_border && style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.roundedRect(inset, inset,
      std::max(0.0f, visual.width - inset * 2.0f),
      std::max(0.0f, visual.height - inset * 2.0f), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_treeview_row(NVGcontext& context,
  const RmTreeViewRowVisual& visual, const RmTreeViewStyle& style)
{
  const RmVisualState state = resolve_state(
    visual.enabled, visual.hovered, visual.pressed);
  const RmStateColors& backgrounds = visual.selected
    ? style.selected_background : style.row_background;
  const RmStateColors& text_colors = visual.selected
    ? style.selected_text : style.row_text;
  const float branch_x = style.horizontal_padding +
    static_cast<float>(visual.depth) * style.indent;
  float text_x = branch_x + style.indent;
  const float center_y = visual.y + style.row_height * 0.5f;
  const char* text = visual.text ? visual.text : "";

  if (visual.icon.isValid())
    text_x += style.icon_size + style.icon_text_gap;

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

  float background_x = style.horizontal_padding;
  float background_y = visual.y;
  float background_width = std::max(0.0f,
    visual.width - style.horizontal_padding * 2.0f);
  float background_height = style.row_height;
  if (visual.selected) {
    const float text_width = context.textBounds(0.0f, 0.0f, text, nullptr,
      nullptr);
    background_x = std::max(style.horizontal_padding,
      text_x - style.selection_horizontal_padding);
    background_width = std::min(
      text_width + style.selection_horizontal_padding * 2.0f,
      std::max(0.0f,
        visual.width - style.horizontal_padding - background_x));
    background_height = std::min(style.row_height,
      style.font_size + style.selection_vertical_padding * 2.0f);
    background_y = visual.y + (style.row_height - background_height) * 0.5f;
  }

  const NVGcolor background = backgrounds.resolve(state);
  if (background.a > 0.0f && background_width > 0.0f &&
    background_height > 0.0f) {
    context.beginPath();
    context.roundedRect(background_x, background_y, background_width,
      background_height, style.row_corner_radius);
    context.fillColor(background);
    context.fill();
    if (visual.selected && style.selection_border_width > 0.0f) {
      context.StrokeWidth(style.selection_border_width);
      context.strokeColor(style.selection_border);
      context.stroke();
    }
  }

  if (style.show_guides && visual.depth > 0) {
    context.beginPath();
    for (size_t depth = 0; depth < visual.depth; ++depth) {
      const float guide_x = style.horizontal_padding +
        (static_cast<float>(depth) + 0.5f) * style.indent;
      context.moveTo(guide_x, visual.y);
      context.lineTo(guide_x, visual.y + style.row_height);
    }
    const float parent_guide_x = branch_x - style.indent * 0.5f;
    const float branch_end_x = visual.expandable
      ? branch_x + style.indent * 0.5f - style.expander_size * 0.5f
      : branch_x + style.indent;
    context.moveTo(parent_guide_x, center_y);
    context.lineTo(branch_end_x, center_y);
    context.StrokeWidth(1.0f);
    context.strokeColor(style.guide);
    context.stroke();
  }

  const float expander_x = branch_x + style.indent * 0.5f;
  if (visual.expandable) {
    const float half = style.expander_size * 0.5f;
    context.beginPath();
    if (visual.expanded) {
      context.moveTo(expander_x - half, center_y - half * 0.5f);
      context.lineTo(expander_x, center_y + half * 0.5f);
      context.lineTo(expander_x + half, center_y - half * 0.5f);
    }
    else {
      context.moveTo(expander_x - half * 0.5f, center_y - half);
      context.lineTo(expander_x + half * 0.5f, center_y);
      context.lineTo(expander_x - half * 0.5f, center_y + half);
    }
    context.StrokeWidth(style.expander_stroke_width);
    context.strokeColor(style.expander.resolve(state));
    context.stroke();
  }

  if (visual.icon.isValid()) {
    const float icon_x = branch_x + style.indent;
    const float icon_y = center_y - style.icon_size * 0.5f;
    context.beginPath();
    context.rect(icon_x, icon_y, style.icon_size, style.icon_size);
    context.fillPaint(NVGpaint::imagePattern(icon_x, icon_y, style.icon_size,
      style.icon_size, 0.0f, visual.icon, visual.enabled ? 1.0f : 0.5f));
    context.fill();
  }

  context.fillColor(text_colors.resolve(state));
  context.text(text_x, center_y, text, nullptr);
}

void RmDefaultControlPainter::draw_treeview_tooltip(NVGcontext& context,
  const RmTreeViewTooltipVisual& visual, const RmTreeViewStyle& style)
{
  const char* text = visual.text ? visual.text : "";
  if (*text == '\0' || visual.available_width <= 0.0f ||
    visual.available_height <= 0.0f)
    return;

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  const float text_width = context.textBounds(0.0f, 0.0f, text, nullptr, nullptr);
  const float width = std::min(visual.available_width,
    text_width + style.tooltip_horizontal_padding * 2.0f);
  const float height = std::min(visual.available_height,
    style.font_size + style.tooltip_vertical_padding * 2.0f);
  const float x = std::clamp(visual.anchor_x, 0.0f,
    std::max(0.0f, visual.available_width - width));
  const float preferred_y = visual.anchor_y + style.tooltip_offset;
  const float y = std::clamp(preferred_y, 0.0f,
    std::max(0.0f, visual.available_height - height));

  context.beginPath();
  context.roundedRect(x, y, width, height, style.tooltip_corner_radius);
  context.fillColor(style.tooltip_background);
  context.fill();
  if (style.tooltip_border_width > 0.0f) {
    context.StrokeWidth(style.tooltip_border_width);
    context.strokeColor(style.tooltip_border);
    context.stroke();
  }
  context.fillColor(style.tooltip_text);
  context.text(x + style.tooltip_horizontal_padding, y + height * 0.5f,
    text, nullptr);
}

void RmDefaultControlPainter::draw_propertyview_surface(NVGcontext& context,
  const RmPropertyViewSurfaceVisual& visual, const RmPropertyViewStyle& style)
{
  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, visual.width - 1.0f),
    std::max(0.0f, visual.height - 1.0f), style.corner_radius);
  context.fillColor(style.background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.roundedRect(inset, inset,
      std::max(0.0f, visual.width - inset * 2.0f),
      std::max(0.0f, visual.height - inset * 2.0f), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_propertyview_group(NVGcontext& context,
  const RmPropertyViewGroupVisual& visual, const RmPropertyViewStyle& style)
{
  context.beginPath();
  context.rect(0.5f, visual.y, std::max(0.0f, visual.width - 1.0f),
    style.group_height);
  context.fillColor(style.group_background);
  context.fill();

  const float center_y = visual.y + style.group_height * 0.5f;
  const float indicator_x = style.horizontal_padding;
  const float half = style.choice_indicator_size;
  context.beginPath();
  if (visual.expanded) {
    context.moveTo(indicator_x, center_y - half * 0.5f);
    context.lineTo(indicator_x + half, center_y + half * 0.5f);
    context.lineTo(indicator_x + half * 2.0f, center_y - half * 0.5f);
  }
  else {
    context.moveTo(indicator_x + half * 0.5f, center_y - half);
    context.lineTo(indicator_x + half * 1.5f, center_y);
    context.lineTo(indicator_x + half * 0.5f, center_y + half);
  }
  context.StrokeWidth(1.0f);
  context.strokeColor(style.group_text);
  context.stroke();

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.group_font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.group_text);
  context.text(indicator_x + half * 2.0f + style.editor_padding, center_y,
    visual.text ? visual.text : "", nullptr);

  if (style.grid_width > 0.0f) {
    context.beginPath();
    context.moveTo(0.0f, visual.y + style.group_height);
    context.lineTo(visual.width, visual.y + style.group_height);
    context.StrokeWidth(style.grid_width);
    context.strokeColor(style.grid);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_propertyview_row(NVGcontext& context,
  const RmPropertyViewRowVisual& visual, const RmPropertyViewStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled, visual.hovered,
    visual.pressed);
  const float split_x = std::clamp(visual.width * style.name_column_ratio,
    0.0f, visual.width);
  const float height = style.row_height;
  const float center_y = visual.y + height * 0.5f;
  const bool invalid = visual.error && *visual.error;

  context.beginPath();
  context.rect(0.5f, visual.y, std::max(0.0f, visual.width - 1.0f), height);
  context.fillColor(style.row_background.resolve(state));
  context.fill();

  context.beginPath();
  context.rect(split_x, visual.y, std::max(0.0f, visual.width - split_x),
    height);
  context.fillColor(invalid ? style.invalid_background :
    style.value_background.resolve(state));
  context.fill();

  if (invalid && style.border_width > 0.0f) {
    const float inset = style.border_width * 0.5f;
    context.beginPath();
    context.rect(split_x + inset, visual.y + inset,
      std::max(0.0f, visual.width - split_x - inset * 2.0f),
      std::max(0.0f, height - inset * 2.0f));
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.invalid_border);
    context.stroke();
  }
  else if (visual.selected && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.rect(split_x + inset, visual.y + inset,
      std::max(0.0f, visual.width - split_x - inset * 2.0f),
      std::max(0.0f, height - inset * 2.0f));
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }

  if (style.grid_width > 0.0f) {
    context.beginPath();
    context.moveTo(split_x, visual.y);
    context.lineTo(split_x, visual.y + height);
    context.moveTo(0.0f, visual.y + height);
    context.lineTo(visual.width, visual.y + height);
    context.StrokeWidth(style.grid_width);
    context.strokeColor(style.grid);
    context.stroke();
  }

  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.name_text);
  context.text(style.horizontal_padding, center_y,
    visual.name ? visual.name : "", nullptr);

  const float value_x = split_x + style.editor_padding;
  const char* value = visual.value ? visual.value : "";
  context.fillColor(invalid ? style.invalid_text : style.value_text);
  context.text(value_x, center_y, value, nullptr);
  if (visual.editing) {
    const float caret_x = value_x + context.textBounds(0.0f, 0.0f, value,
      nullptr, nullptr) + 1.0f;
    context.beginPath();
    context.moveTo(caret_x, visual.y + style.editor_padding);
    context.lineTo(caret_x, visual.y + height - style.editor_padding);
    context.StrokeWidth(1.0f);
    context.strokeColor(invalid ? style.invalid_border : style.focus_ring);
    context.stroke();
  }

  if (visual.choice) {
    const float x = visual.width - style.horizontal_padding -
      style.choice_indicator_size;
    const float half = style.choice_indicator_size;
    context.beginPath();
    context.moveTo(x - half, center_y - half * 0.5f);
    context.lineTo(x, center_y + half * 0.5f);
    context.lineTo(x + half, center_y - half * 0.5f);
    context.StrokeWidth(1.0f);
    context.strokeColor(style.choice_indicator);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_propertyview_choice(NVGcontext& context,
  const RmPropertyViewChoiceVisual& visual, const RmPropertyViewStyle& style)
{
  const NVGcolor background = visual.hovered || visual.selected
    ? style.value_background.hovered : style.value_background.normal;
  context.beginPath();
  context.rect(visual.x, visual.y, visual.width, visual.height);
  context.fillColor(background);
  context.fill();
  context.StrokeWidth(style.grid_width);
  context.strokeColor(style.grid);
  context.stroke();
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  context.fillColor(style.value_text);
  context.text(visual.x + style.editor_padding,
    visual.y + visual.height * 0.5f, visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_output_text_surface(NVGcontext& context,
  const RmOutputTextSurfaceVisual& visual, const RmOutputTextStyle& style)
{
  context.beginPath();
  context.roundedRect(0.5f, 0.5f, std::max(0.0f, visual.width - 1.0f),
    std::max(0.0f, visual.height - 1.0f), style.corner_radius);
  context.fillColor(style.background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
  if (visual.focused && visual.enabled && style.focus_ring_width > 0.0f) {
    const float inset = style.focus_ring_width * 0.5f;
    context.beginPath();
    context.roundedRect(inset, inset,
      std::max(0.0f, visual.width - inset * 2.0f),
      std::max(0.0f, visual.height - inset * 2.0f), style.corner_radius);
    context.StrokeWidth(style.focus_ring_width);
    context.strokeColor(style.focus_ring);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_output_text_line(NVGcontext& context,
  const RmOutputTextLineVisual& visual, const RmOutputTextStyle& style)
{
  NVGcolor text = style.text;
  if (!visual.enabled)
    text.a *= 0.5f;
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.font_size);
  context.setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  context.fillColor(text);
  context.text(style.horizontal_padding, visual.y,
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

void RmDefaultControlPainter::draw_toolstrip_surface(NVGcontext& context,
  const RmToolStripSurfaceVisual& visual, const RmToolStripStyle& style)
{
  const float half = style.border_width * 0.5f;
  context.beginPath();
  context.roundedRect(half, half, std::max(0.0f, visual.width - style.border_width),
    std::max(0.0f, visual.height - style.border_width), style.corner_radius);
  context.fillColor(style.background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_toolstrip_group(NVGcontext& context,
  const RmToolStripGroupVisual& visual, const RmToolStripStyle& style)
{
  if (visual.label_bounds.width <= 0.0f || visual.label_bounds.height <= 0.0f)
    return;
  context.beginPath();
  context.moveTo(visual.label_bounds.x, visual.label_bounds.y);
  context.lineTo(visual.label_bounds.x + visual.label_bounds.width,
    visual.label_bounds.y);
  context.StrokeWidth(style.border_width);
  context.strokeColor(style.separator);
  context.stroke();
  context.setFontFaceId(static_cast<int>(visual.font.getValue()));
  context.setFontSize(style.group_font_size);
  context.setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  context.fillColor(style.group_text);
  context.text(visual.label_bounds.x + visual.label_bounds.width * 0.5f,
    visual.label_bounds.y + visual.label_bounds.height * 0.5f,
    visual.text ? visual.text : "", nullptr);
}

void RmDefaultControlPainter::draw_toolstrip_button(NVGcontext& context,
  const RmToolStripButtonVisual& visual, const RmToolStripStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled,
    visual.hovered, visual.pressed);
  const NVGcolor background = visual.selected
    ? style.selected_background : style.button_background.resolve(state);
  const NVGcolor border = visual.selected
    ? style.selected_border : style.button_border.resolve(state);
  const float border_width = visual.selected
    ? style.selected_border_width : style.border_width;
  const float half = border_width * 0.5f;
  context.beginPath();
  context.roundedRect(visual.bounds.x + half, visual.bounds.y + half,
    std::max(0.0f, visual.bounds.width - border_width),
    std::max(0.0f, visual.bounds.height - border_width), style.corner_radius);
  context.fillColor(background);
  context.fill();
  if (border_width > 0.0f) {
    context.StrokeWidth(border_width);
    context.strokeColor(border);
    context.stroke();
  }

  const float inset = std::max(3.0f, visual.bounds.height * 0.18f);
  if (visual.icon.isValid()) {
    const float extent = std::max(0.0f,
      std::min(visual.bounds.width, visual.bounds.height) - inset * 2.0f);
    const float x = visual.bounds.x + (visual.bounds.width - extent) * 0.5f;
    const float y = visual.bounds.y + (visual.bounds.height - extent) * 0.5f;
    context.beginPath();
    context.rect(x, y, extent, extent);
    context.fillPaint(NVGpaint::imagePattern(x, y, extent, extent, 0.0f,
      visual.icon, visual.enabled ? 1.0f : 0.45f));
    context.fill();
  } else {
    context.setFontFaceId(static_cast<int>(visual.font.getValue()));
    context.setFontSize(style.font_size);
    context.setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    context.fillColor(style.button_text.resolve(state));
    context.text(visual.bounds.x + visual.bounds.width * 0.5f,
      visual.bounds.y + visual.bounds.height * 0.5f,
      visual.text ? visual.text : "", nullptr);
  }
}

void RmDefaultControlPainter::draw_rebar(NVGcontext& context,
  const RmRebarVisual& visual, const RmRebarStyle& style)
{
  const float half = style.border_width * 0.5f;
  context.beginPath();
  context.roundedRect(half, half, std::max(0.0f, visual.width - style.border_width),
    std::max(0.0f, visual.height - style.border_width), style.corner_radius);
  context.fillColor(style.background);
  context.fill();
  if (style.border_width > 0.0f) {
    context.StrokeWidth(style.border_width);
    context.strokeColor(style.border);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_rebar_band(NVGcontext& context,
  const RmRebarBandVisual& visual, const RmRebarStyle& style)
{
  const float axis = visual.vertical ? visual.bounds.y : visual.bounds.x;
  const float cross = visual.vertical ? visual.bounds.x : visual.bounds.y;
  const float cross_extent = visual.vertical
    ? visual.bounds.width : visual.bounds.height;
  const float line_axis = axis + style.gripper_extent * 0.35f;
  for (int i = 0; i < 2; ++i) {
    context.beginPath();
    if (visual.vertical) {
      context.moveTo(cross + 2.0f + i * 3.0f, line_axis);
      context.lineTo(cross + cross_extent - 2.0f, line_axis);
    } else {
      context.moveTo(line_axis, cross + 2.0f + i * 3.0f);
      context.lineTo(line_axis, cross + cross_extent - 2.0f);
    }
    context.StrokeWidth(style.separator_width);
    context.strokeColor(style.gripper);
    context.stroke();
  }
}

void RmDefaultControlPainter::draw_splitter(NVGcontext& context,
  const RmSplitterVisual& visual, const RmSplitterStyle& style)
{
  const RmVisualState state = resolve_state(visual.enabled,
    visual.hovered, visual.dragging);
  context.beginPath();
  context.roundedRect(0.0f, 0.0f, visual.width, visual.height,
    style.corner_radius);
  context.fillColor(style.background.resolve(state));
  context.fill();

  const float center_x = visual.width * 0.5f;
  const float center_y = visual.height * 0.5f;
  const float half = style.grip_extent * 0.5f;
  for (int i = -1; i <= 1; ++i) {
    context.beginPath();
    if (visual.vertical) {
      const float y = center_y + i * style.grip_width * 2.0f;
      context.moveTo(center_x - half, y);
      context.lineTo(center_x + half, y);
    } else {
      const float x = center_x + i * style.grip_width * 2.0f;
      context.moveTo(x, center_y - half);
      context.lineTo(x, center_y + half);
    }
    context.StrokeWidth(style.grip_width);
    context.strokeColor(style.grip);
    context.stroke();
  }

  if (visual.focused && style.focus_ring_width > 0.0f) {
    context.beginPath();
    context.rect(style.focus_ring_width * 0.5f,
      style.focus_ring_width * 0.5f,
      std::max(0.0f, visual.width - style.focus_ring_width),
      std::max(0.0f, visual.height - style.focus_ring_width));
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
