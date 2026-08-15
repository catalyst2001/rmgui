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
