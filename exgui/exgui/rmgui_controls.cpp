#include "rmgui_controls.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "stb_image.h"

/**
* drawImage
* 
* sx, sy, sw, sh - sprite location on texture
* x, y, w, h - position and size of the sprite rectangle on screen
* 
* source 'https://github.com/memononen/nanovg/issues/348'
*/
void drawSprite(NVGcontext* vg, int image, float alpha,
  float sx, float sy, float sw, float sh, // sprite location on texture
  float x, float y, float w, float h, // position and size of the sprite rectangle on screen
  float lt, float rt, float lb, float rb) 
{
  float ax, ay;
  int iw, ih;
  NVGpaint img;

  nvgImageSize(vg, image, &iw, &ih);

  // Aspect ration of pixel in x an y dimensions. This allows us to scale
  // the sprite to fill the whole rectangle.
  ax = w / sw;
  ay = h / sh;

  img = nvgImagePattern(vg, x - sx * ax, y - sy * ay, (float)iw * ax, (float)ih * ay, 0, image, alpha);
  nvgBeginPath(vg);
  nvgRoundedRectVarying(vg, x, y, w, h, lt, rt, rb, lb);
  nvgFillPaint(vg, img);
  nvgFill(vg);
}

rmgui_image::rmgui_image() : imageId(-1), width(0), height(0), channels(0) {}
rmgui_image::~rmgui_image() {}

bool rmgui_image::load(const std::string& filename, NVGcontext* ctx) {
  unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
  if (!data) {
    std::cerr << "Failed to load image: " << filename << std::endl;
    return false;
  }
  int flags = NVG_IMAGE_NEAREST;
  imageId = nvgCreateImageRGBA(ctx, width, height, flags, data);
  stbi_image_free(data);
  if (imageId == 0) {
    std::cerr << "Failed to create NVG image from: " << filename << std::endl;
    return false;
  }
  return true;
}

rm_image_button::rm_image_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& imageFile)
  : rm_widget(x, y, width, height, p_parent, "ui_image_button")
{
  m_image = new rmgui_image();
  rm_surface* surface = dynamic_cast<rm_surface*>(get_root());
  if (surface) {
    NVGcontext* context = surface->get_context();
    if (!m_image->load(imageFile, context)) {
      std::cerr << "Image button: Failed to load image " << imageFile << std::endl;
    }
  }
}

rm_image_button::~rm_image_button() {
  if (m_image) {
    rm_surface* surface = dynamic_cast<rm_surface*>(get_root());
    if (surface && m_image->imageId != -1) {
      nvgDeleteImage(surface->get_context(), m_image->imageId);
    }
    delete m_image;
  }
}

void rm_image_button::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented this
  //nvgBeginPath(p_ctx);
  //nvgRect(p_ctx, 0.f, 0.f, m_relative.width, m_relative.height);
  //nvgFillColor(p_ctx, nvgRGBA(200, 200, 200, 255));
  //nvgFill(p_ctx);

  if (m_image && m_image->imageId != -1) {
    NVGpaint imgPaint = nvgImagePattern(p_ctx,
      0.f, 0.f,
      m_size.x, m_size.y,
      0.0f, m_image->imageId, 1.0f);
    nvgBeginPath(p_ctx);
    nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, 4.0f);
    nvgFillPaint(p_ctx, imgPaint);
    nvgFill(p_ctx);
  }
  rm_widget::on_draw(p_ctx);
}

bool rm_image_button::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
    std::cout << "Image Button clicked!" << std::endl;
    return false;
  }
  return true;
}

rm_button::rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text)
  : rm_widget(x, y, width, height, p_parent, "ui_button"), m_text(text)
{
}

rm_button::~rm_button() {}

void rm_button::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute);  //NOTE: K.D. commented this
  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, 20.0f);

  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, 4.0f);
  NVGcolor fillColor = nvgRGBA(100, 100, 250, 255);
  if (m_elem_flags.is_hovered())
    fillColor = nvgRGBA(120, 120, 255, 255);
  nvgFillColor(p_ctx, fillColor);
  nvgFill(p_ctx);

  nvgTextAlign(p_ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(255, 255, 255, 255));
  float cx = m_size.x / 2.0f;
  float cy = m_size.y / 2.0f;
  nvgText(p_ctx, cx, cy, m_text.c_str(), nullptr);
  rm_widget::on_draw(p_ctx);
}

bool rm_button::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
    std::cout << "Button \"" << m_text << "\" clicked!" << std::endl;
    return false;
  }
  return true;
}

rm_label::rm_label(rm_widget* p_parent, int x, int y, const std::string& text)
  : rm_widget(x, y, 200, 30, p_parent, "ui_label"), m_text(text)
{
}

rm_label::~rm_label() {}

void rm_label::on_draw(NVGcontext* p_ctx) {
  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, 18.0f);
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(255, 255, 255, 255));
  nvgText(p_ctx, 0.f, m_size.y * 0.5f, m_text.c_str(), nullptr);
  rm_widget::on_draw(p_ctx);
}

rm_text_input::rm_text_input(rm_widget* p_parent, int x, int y, int width, int height,
  rm_text_input_style* pstyle, uint32_t flags, float blink_cursor_interval)
  : rm_widget(x, y, width, height, p_parent, "ui_text_input", EXGUI_FLAG_DEFAULT|EXGUI_FLAG_GLOBAL), m_active(false), m_ctrl_pressed(false), m_dragging(false), 
  m_scroll_offset(0.f), m_last_click_time(0.0), m_last_click_pos({ 0,0 }), m_asc(0.f), m_line_h(0.f){
  set_style(pstyle);
  m_blink_state = false;
  m_timer.set_interval(blink_cursor_interval);
  m_flags = flags;
}

rm_text_input::~rm_text_input() {}

void rm_text_input::on_draw(NVGcontext* p_ctx) {
  const std::string& txt = m_buffer.str();
  float desc;
  std::vector<std::string> lines;
  std::vector<size_t>      starts;

  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, m_pstyle->get_font_size());
  nvgTextMetrics(p_ctx, &m_asc, &desc, &m_line_h);

  // background & border
  nvgBeginPath(p_ctx);
  nvgRoundedRectVarying(p_ctx, 0.f, 0.f, m_size.x, m_size.y,
    m_pstyle->get_corner_radius(LEFT_TOP),
    m_pstyle->get_corner_radius(RIGHT_TOP),
    m_pstyle->get_corner_radius(RIGHT_BOTTOM),
    m_pstyle->get_corner_radius(LEFT_BOTTOM));
  nvgFillColor(p_ctx, m_active
    ? m_pstyle->get_active_bgr_color()
    : m_pstyle->get_unactive_bgr_color());
  nvgFill(p_ctx);
  nvgStrokeColor(p_ctx, m_pstyle->get_border_color());
  nvgStrokeWidth(p_ctx, m_pstyle->get_border_width());
  nvgStroke(p_ctx);

  bool multiline = (m_flags & RMGUI_TEXT_INPUT_MULTILINE);
  float offset = 0.f;
  size_t ci = m_buffer.has_selection() ? m_buffer.sel_end : m_buffer.pos();
  if (!multiline) {
    // single-line glyph positions
    m_glyph_positions.clear();
    m_glyph_positions.push_back(0.f);
    for (size_t i = 1; i <= txt.size(); ++i) {
      float w = nvgTextBounds(p_ctx, 0, 0, txt.substr(0, i).c_str(), nullptr, nullptr);
      m_glyph_positions.push_back(w);
    }

    float avail = m_size.x - 10.f;
    float caret_x = m_glyph_positions[ci];

    if (caret_x - m_scroll_offset > avail)
      m_scroll_offset = caret_x - avail;
    else if (caret_x < m_scroll_offset)
      m_scroll_offset = caret_x;

    offset = m_scroll_offset;
  }
  else {
    lines.reserve(8);
    starts.reserve(8);
    size_t pos = 0;
    while (pos <= txt.size()) {
      size_t nl = txt.find('\n', pos);
      if (nl == std::string::npos) nl = txt.size();
      starts.push_back(pos);
      lines.push_back(txt.substr(pos, nl - pos));
      pos = nl + 1;
    }

    m_line_starts = starts;
    m_line_glyphs.clear();
    m_line_glyphs.reserve(lines.size());
    float max_w = 0.f;

    for (auto& line : lines) {
      std::vector<float> gp;
      gp.reserve(line.size() + 1);
      gp.push_back(0.f);
      for (size_t i = 1; i <= line.size(); ++i) {
        float w = nvgTextBounds(p_ctx, 0, 0,
          line.substr(0, i).c_str(),
          nullptr, nullptr);
        gp.push_back(w);
      }
      max_w = std::max(max_w, gp.back());
      m_line_glyphs.push_back(std::move(gp));
    }

    float avail = m_size.x - 10.f;
    float max_offset = std::max(0.f, max_w - avail);
    m_scroll_offset = std::clamp(m_scroll_offset, 0.f, max_offset);
    offset = m_scroll_offset;
  }

  // update blink state
  if (m_timer.has_elapsed(get_sysdf())) {
    m_blink_state = !m_blink_state;
  }

  // draw selection
  if (m_buffer.has_selection()) {
    nvgFillColor(p_ctx, m_pstyle->get_selection_color());
    if (!multiline) {
      size_t a = std::min(m_buffer.sel_start, m_buffer.sel_end);
      size_t b = std::max(m_buffer.sel_start, m_buffer.sel_end);
      float x0 = 5.f - offset + m_glyph_positions[a] + m_pstyle->get_text_offset();
      float x1 = 5.f - offset + m_glyph_positions[b] + m_pstyle->get_text_offset();
      float baseline = m_size.y * 0.5f + (m_asc + desc) * 0.5f;
      nvgBeginPath(p_ctx);
      nvgRect(p_ctx, x0, baseline - m_asc, x1 - x0, m_asc - desc);
      nvgFill(p_ctx);
    }
    else {
      size_t sel_a = std::min(m_buffer.sel_start, m_buffer.sel_end);
      size_t sel_b = std::max(m_buffer.sel_start, m_buffer.sel_end);
      float top_pad = 5.f + m_asc;
      for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        size_t ls = starts[i], le = ls + line.size();
        if (sel_a < le && sel_b > ls) {
          size_t a = std::max(sel_a, ls) - ls;
          size_t b = std::min(sel_b, le) - ls;
          std::string pre = line.substr(0, a);
          std::string sel = line.substr(a, b - a);
          float x0 = 5.f - offset + nvgTextBounds(p_ctx, 0, 0, pre.c_str(), nullptr, nullptr);
          float w = nvgTextBounds(p_ctx, 0, 0, sel.c_str(), nullptr, nullptr);
          float y0 = top_pad + i * m_line_h - m_asc;
          nvgBeginPath(p_ctx);
          nvgRect(p_ctx, x0 + m_pstyle->get_text_offset(), y0, w, m_line_h);
          nvgFill(p_ctx);
        }
      }
    }
  }

  // draw text and cursor
  nvgFillColor(p_ctx, m_pstyle->get_text_color());
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  if (!multiline) {
    float baseline = m_size.y * 0.5f + (m_asc + desc) * 0.5f;
    nvgText(p_ctx, 5.f - offset + m_pstyle->get_text_offset(),
      baseline, txt.c_str(), nullptr);
    if (m_active && m_blink_state) {
      float cw = m_glyph_positions[ci];
      nvgBeginPath(p_ctx);
      nvgMoveTo(p_ctx, 5.f - offset + cw + m_pstyle->get_text_offset() + 1, baseline - m_asc);
      nvgLineTo(p_ctx, 5.f - offset + cw + m_pstyle->get_text_offset() + 1, baseline - desc);
      nvgStrokeWidth(p_ctx, m_pstyle->get_blink_width());
      nvgStrokeColor(p_ctx, m_pstyle->get_blink_color());
      nvgStroke(p_ctx);
    }
  }
  else {
    float top_pad = 5.f + m_asc;
    for (size_t i = 0; i < lines.size(); ++i) {
      float y = top_pad + i * m_line_h;
      nvgText(p_ctx, 5.f - offset + m_pstyle->get_text_offset(), y, lines[i].c_str(), nullptr);
    }

    if (m_active && m_blink_state) {
      int cli = int(std::upper_bound(m_line_starts.begin(), m_line_starts.end(), ci) - m_line_starts.begin()) - 1;
      size_t off = ci - m_line_starts[cli];
      float cx = m_line_glyphs[cli][off];
      float cy = 5.f + m_asc + cli * m_line_h;
      nvgBeginPath(p_ctx);
      nvgMoveTo(p_ctx, 5.f - offset + cx + m_pstyle->get_text_offset() + 1, cy - m_asc);
      nvgLineTo(p_ctx, 5.f - offset + cx + m_pstyle->get_text_offset() + 1, cy - m_asc + m_line_h);
      nvgStrokeWidth(p_ctx, m_pstyle->get_blink_width());
      nvgStrokeColor(p_ctx, m_pstyle->get_blink_color());
      nvgStroke(p_ctx);
    }
  }

  rm_widget::on_draw(p_ctx);
}

void rm_text_input::on_keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state)
{
  if (!m_active) {
    return;
  }

  if (vk == EXGUI_KEY_LCTRL || vk == EXGUI_KEY_RCTRL) {
    m_ctrl_pressed = (state != EXGUI_KEY_STATE::UP);
    return;
  }

  if ((state == EXGUI_KEY_STATE::DOWN || state == EXGUI_KEY_STATE::REPEAT)) {
    if (!m_ctrl_pressed) {
      bool handled = true;
      switch (vk) {
      case EXGUI_KEY_BACKSPACE:
        m_buffer.backspace();
        break;
      case EXGUI_KEY_DELETE:
        m_buffer.delete_forward();
        break;
      case EXGUI_KEY_LEFT:
        m_buffer.move_cursor_left();
        break;
      case EXGUI_KEY_RIGHT:
        m_buffer.move_cursor_right();
        break;
      case EXGUI_KEY_UP:
        m_buffer.move_cursor_up();
        break;
      case EXGUI_KEY_DOWN:
        m_buffer.move_cursor_down();
        break;
      default:
        handled = false;
      }
      if (handled) {
        m_timer.reset(m_psysdf);
        m_blink_state = true;
        return;
      }
    }
    else {
      switch (vk) {
      case EXGUI_KEY_Z:
        m_buffer.undo(); 
        break;
      case EXGUI_KEY_Y:
        m_buffer.redo(); 
        break;
      }
    }
  }

  if (state != EXGUI_KEY_STATE::DOWN)
    return;

  if (m_ctrl_pressed && vk == EXGUI_KEY_A) {
    m_buffer.select_all();
    m_timer.reset(m_psysdf);
    m_blink_state = false;
    return;
  }

  if (m_ctrl_pressed) {
    switch (vk) {
    case EXGUI_KEY_C:
      m_buffer.copy_all(m_psysdf); 
      break;

    case EXGUI_KEY_V:
      m_buffer.paste(m_psysdf);
      break;

    case EXGUI_KEY_X: 
      if (m_buffer.has_selection())
      m_buffer.cut_selection(m_psysdf);
      break;

    default:
      return;
    }

    m_timer.reset(m_psysdf);
    m_blink_state = false;
    return;
  }

  if (vk == EXGUI_KEY_ENTER && (m_flags & RMGUI_TEXT_INPUT_MULTILINE)) {
    m_buffer.insert_cp('\n');
    m_timer.reset(m_psysdf);
    m_blink_state = false;
    return;
  }
}

bool rm_text_input::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  bool inside = m_bbox.inside(cursor_pos);
  float offset = m_pstyle->get_text_offset();
  float text_draw_x = m_absolute.x + 5.f + offset;
  float local_x = cursor_pos.x - text_draw_x;
  float local_y = cursor_pos.y - m_absolute.y;
  const double now = m_psysdf->get_time();
  bool multiline = (m_flags & RMGUI_TEXT_INPUT_MULTILINE);

  if (event == EXGUI_MOUSE_EVENT_CLICK && state == EXGUI_KEY_STATE::DOWN) {
    if (inside) {
      float dx = cursor_pos.x - m_last_click_pos.x;
      float dy = cursor_pos.y - m_last_click_pos.y;
      double dt = now - m_last_click_time;
      if (dt <= DOUBLE_CLICK_THRESHOLD && (dx * dx + dy * dy) <= CLICK_MOVE_THRESHOLD * CLICK_MOVE_THRESHOLD)
      {
        m_buffer.select_all();
        m_timer.reset(m_psysdf);
        return true;
      }
      m_active = true;
      m_dragging = true;
      size_t idx = multiline ? hit_test_index(local_x + m_scroll_offset, local_y) : hit_test_index(local_x);
      m_buffer.set_cursor(idx);
      m_buffer.sel_start = idx;
      m_buffer.sel_end = idx;
      m_blink_state = true;
      m_timer.reset(m_psysdf);
      m_last_click_time = now;
      m_last_click_pos = cursor_pos;
      return true;
    }
    else {
      m_active = false;
      m_dragging = false;
      m_buffer.clear_selection();
      return true;
    }
  }

  if (event == EXGUI_MOUSE_EVENT_CLICK && state == EXGUI_KEY_STATE::DOWN) {
    m_buffer.select_all();
    m_timer.reset(m_psysdf);
    m_blink_state = false;
    return true;
  }

  if (event == EXGUI_MOUSE_EVENT_MOVE && state == EXGUI_KEY_STATE::DOWN && m_dragging) {
    size_t idx = multiline ? hit_test_index(local_x + m_scroll_offset, local_y) : hit_test_index(local_x);
    m_buffer.sel_end = idx;
    if (!multiline) {
      ensure_visible(idx);
    }
    return true;
  }

  if (event == EXGUI_MOUSE_EVENT_CLICK && state == EXGUI_KEY_STATE::UP) {
    if (m_dragging) {
      if (m_buffer.sel_start == m_buffer.sel_end)
        m_buffer.clear_selection();
      m_dragging = false;
      return true;
    }
    return true;
  }
  return true;
}

size_t rm_text_input::hit_test_index(float px, float py) const
{
  float x = px + m_scroll_offset - m_pstyle->get_text_offset();

  if (!(m_flags & RMGUI_TEXT_INPUT_MULTILINE)) {
    auto it = std::lower_bound(m_glyph_positions.begin(), m_glyph_positions.end(), x);
    size_t idx = it - m_glyph_positions.begin();
    if (idx >= m_glyph_positions.size())
      idx = m_glyph_positions.size() - 1;
    return idx;
  }

  float top_pad = 5.f + m_asc;
  int   li = int((py - top_pad + (m_line_h * 0.5f)) / m_line_h);
  li = std::clamp(li, 0, int(m_line_starts.size()) - 1);

  size_t start = m_line_starts[li];
  size_t end = (li + 1 < m_line_starts.size())
    ? m_line_starts[li + 1] - 1
    : m_buffer.str().size();

  const auto& gp = m_line_glyphs[li];
  auto it2 = std::lower_bound(gp.begin(), gp.end(), x);
  size_t ci = it2 - gp.begin();
  if (ci >= gp.size()) ci = gp.size() - 1;

  return start + ci;
}

void rm_text_input::ensure_visible(size_t idx) {
  float avail = m_size.x - 10.f;

  if (idx >= m_glyph_positions.size()) 
    idx = m_glyph_positions.size() - 1;
  float x = m_glyph_positions[idx];

  if (x - m_scroll_offset > avail)
    m_scroll_offset = x - avail;
  else if (x < m_scroll_offset)
    m_scroll_offset = x;
}

void rm_text_input::on_text_input(int sym) {
  if (m_active) {
    //printf("keycode: %d\n", sym);
    if (sym >= 32) {
      m_buffer.insert_cp(sym);
    }
  }
  m_timer.reset(m_psysdf);
  m_blink_state = false;
}

rm_checkbox::rm_checkbox(rm_widget* p_parent, int x, int y, int width, rm_checkbox_style* pstyle, const std::string& label, rm_checkbox_cb pcallback)
  : rm_widget(x, y, width, pstyle->get_check_size(), p_parent, "ui_checkbox"), m_checked(false), m_label(label)
{
  set_style(pstyle);
  set_callback(pcallback);
  m_icon_font = m_proot->find_font("fontawesome");
  assert(m_icon_font.is_valid() && "'fontawesome' not loaded");
}

rm_checkbox::~rm_checkbox() {}

void rm_checkbox::on_draw(NVGcontext* p_ctx) {
  float xo, yo;
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented this
  nvgFontFaceId(p_ctx, get_font());

  /* paint background */
  nvgBeginPath(p_ctx);
  nvgRoundedRectVarying(p_ctx,
    0.5f, 0.5f, m_size.y-1.f, m_size.y-1.f,
    m_pstyle->get_corner_radius(LEFT_TOP),
    m_pstyle->get_corner_radius(RIGHT_TOP),
    m_pstyle->get_corner_radius(RIGHT_BOTTOM),
    m_pstyle->get_corner_radius(LEFT_BOTTOM)
  );
  nvgFillColor(p_ctx, m_pstyle->get_background_color());
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, m_pstyle->get_border_width());
  nvgStrokeColor(p_ctx, m_pstyle->get_border_color());
  nvgStroke(p_ctx);

  if (m_checked) {
    /* draw mark */
    nvgFontFaceId(p_ctx, m_icon_font);
    nvgFontSize(p_ctx, 16.f);
    nvgTextAlign(p_ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(p_ctx, m_pstyle->get_text_color());
    xo = m_size.y / 2.f;
    yo = m_size.y / 2.f;
    nvgText(p_ctx, xo, yo, ICON_FA_CHECK, nullptr);
  }

  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, m_pstyle->get_font_size());
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, m_pstyle->get_text_color());

  const rm_vec2& text_offsets = m_pstyle->get_text_offsets();
  nvgText(p_ctx, m_size.y + text_offsets.x, (m_size.y / 2.0f) + text_offsets.y, m_label.c_str(), nullptr);
  rm_widget::on_draw(p_ctx);
}

bool rm_checkbox::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == UP && m_bbox.inside(cursor_pos)) {
    m_checked = !m_checked;
    if (is_valid_callback())
      get_callback()(this);

    return false;
  }
  return true;
}

rm_combobox::rm_combobox(rm_widget* p_parent, int x, int y, int width, int height, rm_combobox_cb pcallback)
  : rm_widget(x, y, width, height, p_parent, "ui_combobox", EXGUI_FLAG_DEFAULT|EXGUI_FLAG_GLOBAL|EXGUI_FLAG_DISABLE_SCISSOR|EXGUI_FLAG_HIGHEST_PRIORITY), m_selected(0), m_expanded(false)
{
  set_callback(pcallback);
  set_zindex(999); //topmost
}

rm_combobox::~rm_combobox() {}

size_t rm_combobox::add_item(const char* pitem, void* puserdata)
{
  m_items.push_back({ pitem, puserdata });
  return m_items.size() - 1;
}

size_t rm_combobox::find_item(const char* pitem)
{
  auto it = std::find_if(m_items.begin(), m_items.end(),
    [pitem](rm_combo_item &item) {
      return !strcmp(pitem, item.get_name());
    }
  );

  if (it != m_items.end())
    return it - m_items.begin();

  return rm_combobox::kinvalid_index;
}

void rm_combobox::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
  nvgFontFaceId(p_ctx, get_font());

  //bgr
  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, 4.0f);
  nvgFillColor(p_ctx, nvgRGBA(180, 180, 180, 255));
  nvgFill(p_ctx);
  nvgStrokeColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  nvgStroke(p_ctx);

  nvgFontSize(p_ctx, 18.0f);
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  if (!m_items.empty() && m_selected >= 0 && m_selected < (int)m_items.size())
    nvgText(p_ctx, 5.f, m_size.y / 2.0f, m_items[m_selected].get_name(), nullptr);

  // arrow
  float x = 0, y = 0, w = m_size.x, h = m_size.y;
  float r = 4.0f;
  float ax = x + w - h * 0.5f, ay = y + h * 0.5f, sz = 5.f;
  nvgBeginPath(p_ctx);
  if (!m_expanded) {
    nvgMoveTo(p_ctx, ax - sz, ay - sz * 0.5f);
    nvgLineTo(p_ctx, ax + sz, ay - sz * 0.5f);
    nvgLineTo(p_ctx, ax, ay + sz * 0.5f);
  }
  else {
    nvgMoveTo(p_ctx, ax - sz, ay + sz * 0.5f);
    nvgLineTo(p_ctx, ax + sz, ay + sz * 0.5f);
    nvgLineTo(p_ctx, ax, ay - sz * 0.5f);
  }
  nvgClosePath(p_ctx);
  nvgFillColor(p_ctx, nvgRGBA(50, 50, 50, 255));
  nvgFill(p_ctx);

  if (m_expanded) {
    for (size_t i = 0; i < m_items.size(); i++) {
      rm_combo_item& item = m_items[i];
      float itemY = m_size.y * (1.f + i);
      rm_bbox bbox;
      rm_vec2 item_pos(m_absolute.x, m_absolute.y + itemY);
      bbox.init(item_pos, m_size);
      nvgBeginPath(p_ctx);
      nvgRect(p_ctx, 0.f, itemY, m_size.x, m_size.y);
      nvgFillColor(p_ctx, bbox.inside(m_cursor) ? nvgRGBA(80, 80, 80, 255) : nvgRGBA(200, 200, 200, 255));
      nvgFill(p_ctx);
      nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
      nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));
      nvgText(p_ctx, 5.f, itemY + m_size.y / 2.0f, item.get_name(), nullptr);
    }
  }
  rm_widget::on_draw(p_ctx);
}

bool rm_combobox::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  m_cursor = cursor_pos;
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN) {
    if (m_bbox.inside(cursor_pos)) {
      if (m_expanded) {
        m_expanded = false;
        return false;
      }
      m_expanded = true;
      return false;
    }

    if (m_expanded) {
      float itemYStart = m_absolute.y + m_size.y;
      float itemHeight = m_size.y;
      int index = (int)((cursor_pos.y - itemYStart) / itemHeight);
      if (index >= 0 && index < (int)m_items.size()) {
        m_selected = index;
        if (is_valid_callback()) {
          get_callback()(this, &m_items[m_selected], (size_t)m_selected);
        }
      }
      m_expanded = false;
      return false;
    }
  }

  if (m_expanded) {
    return false; //absorb the event
  }
  return true;
}

void rm_slider::compute_value(rm_vec2& cursor_pos)
{
  float localX = cursor_pos.x - (m_absolute.x + m_thumb_size);
  float fraction = localX / m_inner_rect.width;
  fraction = std::max(0.f, std::min(1.f, fraction));
  m_value = m_min + fraction * (m_max - m_min);
  if (m_last_value != m_value) {
    m_last_value = m_value;
    if (m_pcallback) {
      m_pcallback(this);
    }
  }
}

void rm_slider::compute_inner_and_thumb()
{
  m_thumb_size = m_size.y / 2.5f;
  m_inner_rect.x = m_thumb_size;
  m_inner_rect.y = 0.f;
  m_inner_rect.width = m_size.x - m_thumb_size * 2.f;
  m_inner_rect.height = m_size.y;
}

rm_slider::rm_slider(rm_widget* p_parent, int x, int y, int width, int height, float min, float max, float initial, rm_slider_callback pcallback)
  : rm_widget(x, y, width, height, p_parent, "ui_slider", EXGUI_FLAG_DEFAULT|EXGUI_FLAG_GLOBAL), 
  m_min(min), m_max(max), m_value(initial), m_dragging(false), m_pcallback(pcallback)
{
  compute_inner_and_thumb();
}

rm_slider::~rm_slider() {}

void rm_slider::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented

  float trackY = m_inner_rect.y + m_inner_rect.height / 2.0f;
  nvgBeginPath(p_ctx);
  nvgMoveTo(p_ctx, m_inner_rect.x, trackY);
  nvgLineTo(p_ctx, m_inner_rect.x + m_inner_rect.width, trackY);
  nvgStrokeColor(p_ctx, nvgRGBA(150, 150, 150, 255));
  nvgStroke(p_ctx);

  float fraction = (m_value - m_min) / (m_max - m_min);
  float thumbX = m_inner_rect.x + fraction * m_inner_rect.width;

  nvgBeginPath(p_ctx);
  nvgCircle(p_ctx, thumbX, trackY, m_inner_rect.height / 2.5f);
  nvgFillColor(p_ctx, nvgRGBA(100, 100, 250, 255));
  nvgFill(p_ctx);
  rm_widget::on_draw(p_ctx);
}

bool rm_slider::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
    m_dragging = true;
    compute_value(cursor_pos);
    return false;
  }
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == UP) {
    m_dragging = false;
    return false;
  }
  if (m_dragging && event == EXGUI_MOUSE_EVENT_MOVE) {
    compute_value(cursor_pos);
    return false;
  }
  return true;
}

rm_progress_base::rm_progress_base(rm_widget* p_parent, int x, int y, int width, int height, float inital, float corner_round) :
  rm_widget(x, y, width, height, p_parent, "ui_progress_base")
{
  m_round = corner_round;
  m_percent = inital;
}

rm_progress_base::~rm_progress_base()
{
}

void rm_progress_base::on_draw(NVGcontext* p_ctx)
{
  // paint background
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, nvgRGB(0, 0, 0));
  nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, m_round);
  nvgFill(p_ctx);

  // paint progres bar
  float identity_percent = m_percent / 100.f; // 0.f-1.f
  rm_rect percent_rect(0.f, 0.f, m_size);
  percent_rect.width *= identity_percent;

  NVGpaint paint = nvgLinearGradient(p_ctx, 
    percent_rect.x, percent_rect.y,
    percent_rect.x + percent_rect.width, percent_rect.y,
    nvgRGB(232, 43, 231), nvgRGB(40, 5, 229));

  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx,
    percent_rect.x, percent_rect.y,
    percent_rect.width, percent_rect.height, m_round);
  nvgFillPaint(p_ctx, paint);
  nvgFill(p_ctx);
  rm_widget::on_draw(p_ctx);
}

bool rm_progress_base::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos)
{
  return true;
}

rm_progress_image::rm_progress_image(rm_widget* p_parent, int x, int y, int width, int height, 
  rm_image img, float patangle, float patalpha, float inital, float corner_round) :
  rm_progress_base(p_parent, x, y, width, height, inital, corner_round)
{
  m_angle = patangle;
  m_alpha = patalpha;
  m_image = img;
}

rm_progress_image::~rm_progress_image()
{
}

void rm_progress_image::on_draw(NVGcontext* p_ctx)
{
  int iw, ih;
  // paint background
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, nvgRGB(0, 0, 0));
  nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, m_round);
  nvgFill(p_ctx);

  // paint progres bar
  float identity_percent = m_percent / 100.f; // 0.f-1.f
  rm_rect percent_rect(0.f, 0.f, m_size);
  percent_rect.width *= identity_percent;

  nvgImageSize(p_ctx, m_image, &iw, &ih);
  NVGpaint paint = nvgImagePattern(p_ctx, 0, 0, percent_rect.width, percent_rect.height, m_angle, m_image, m_alpha);
  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx,
    percent_rect.x, percent_rect.y,
    percent_rect.width, percent_rect.height, m_round);
  nvgFillPaint(p_ctx, paint);
  nvgFill(p_ctx);

  drawSprite(p_ctx, m_image, m_alpha, 0.f, 0.f, 13.f, 15.f, percent_rect.x, percent_rect.y, percent_rect.width, percent_rect.height, m_round, m_round, m_round, m_round);
  rm_widget::on_draw(p_ctx);
}

void rm_scroll_base::orient_detect(const rm_rect& background)
{
  m_orientation = (background.width > background.height) ? RM_ORIENT_HORZ : RM_ORIENT_VERT;
}

void rm_scroll_base::draw_scroll(NVGcontext* p_ctx, const rm_vec2& back, rm_scroll_style* pstyle, float pos)
{
  rm_rect thumb_rect;
  assert(m_orientation != RM_ORIENT_AUTO && "[K.D.] m_orientation have undefined value! You called rm_scroll_base::orient_detect() from init/resize?");
  if (m_orientation == RM_ORIENT_HORZ) {
    thumb_rect.x = back.x * pos;
    thumb_rect.y = back.y;
    thumb_rect.width = pstyle->get_thumb_size();
    thumb_rect.height = back.y;
  }
  else {
    thumb_rect.x = back.x;
    thumb_rect.y = back.y * pos;
    thumb_rect.width = back.x;
    thumb_rect.height = pstyle->get_thumb_size();
  }

  /* paint background */
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_background_color());
  nvgRoundedRect(p_ctx, 0, 0, back.x, back.y, pstyle->get_scroll_corner_radius());
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, pstyle->get_background_stroke_width());
  nvgStrokeColor(p_ctx, pstyle->get_scroll_background_border_color());
  nvgStroke(p_ctx);

  /* paint thumb */
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_thumb_color());
  nvgRoundedRect(p_ctx, thumb_rect.x, thumb_rect.y, thumb_rect.width, thumb_rect.height, pstyle->get_scroll_corner_radius());
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, pstyle->get_thumb_stroke_width());
  nvgStrokeColor(p_ctx, pstyle->get_scroll_thumb_border_color());
  nvgStroke(p_ctx);
}

void rm_scroll_base::draw_scroll(NVGcontext* p_ctx, rm_scroll_style* pstyle, rm_vec2 content,
  const rm_vec2& window, float thumb_thickness, float pos)
{
  rm_rect thumb_rect;
  float   thumb_size;
  assert(m_orientation != RM_ORIENT_AUTO && "[K.D.] m_orientation have undefined value! You called rm_scroll_base::orient_detect() from init/resize?");
  if (m_orientation == RM_ORIENT_HORZ) {
    if (content.x <= window.x)
      content.x = window.x;

    float scroll_area = window.x;
    thumb_size = (window.x / content.x) * scroll_area;
    thumb_size = std::max(thumb_thickness, thumb_size); // thumb min width

    float max_offset = scroll_area - thumb_size;
    float thumb_x = pos * max_offset;

    thumb_rect.x = thumb_x;
    thumb_rect.y = (window.y - thumb_thickness) * 0.5f;
    thumb_rect.width = thumb_size;
    thumb_rect.height = thumb_thickness;
  }
  else {
    if (content.y <= window.y)
      content.y = window.y;

    float scroll_area = window.y;
    thumb_size = (window.y / content.y) * scroll_area;
    thumb_size = std::max(thumb_thickness, thumb_size); // thumb min height

    float max_offset = scroll_area - thumb_size;
    float thumb_y = pos * max_offset;

    thumb_rect.x = (window.x - thumb_thickness) * 0.5f;
    thumb_rect.y = thumb_y;
    thumb_rect.width = thumb_thickness;
    thumb_rect.height = thumb_size;
  }

  // paint background
  float round_radius = (window.y / 2.f) * pstyle->get_scroll_corner_radius();
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_background_color());
  nvgRoundedRect(p_ctx, 0, 0, window.x, window.y, round_radius);
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, pstyle->get_background_stroke_width());
  nvgStrokeColor(p_ctx, pstyle->get_scroll_background_border_color());
  nvgStroke(p_ctx);

  // paint thumb
  round_radius = (thumb_rect.height / 2.f) * pstyle->get_scroll_corner_radius();
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_thumb_color());
  nvgRoundedRect(p_ctx, thumb_rect.x, thumb_rect.y, thumb_rect.width, thumb_rect.height, round_radius);
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, pstyle->get_thumb_stroke_width());
  nvgStrokeColor(p_ctx, pstyle->get_scroll_thumb_border_color());
  nvgStroke(p_ctx);
}

void rm_animation::on_draw(NVGcontext* p_ctx)
{
  rm_vec2 pos(m_size.x / 2.f, m_size.y / 2.f);
  rm_widget::on_draw(p_ctx);
  nvgTranslate(p_ctx, pos.x, pos.y);
  nvgRotate(p_ctx, m_angle);
  nvgScale(p_ctx, m_scale, m_scale);
  nvgTranslate(p_ctx, -pos.x, -pos.y);
  nvgBeginPath(p_ctx);
  NVGpaint imgPaint = nvgImagePattern(p_ctx, 0.f, 0.f, m_size.x, m_size.y, 0.0f, m_image, 1.0f);
  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y, 4.0f);
  nvgFillPaint(p_ctx, imgPaint);
  nvgFill(p_ctx);
  m_angle += m_speed * m_proot->get_delta_time();
}

rm_animation::rm_animation(rm_widget* p_parent, int x, int y, int width, int height, rm_image img, float start_angle, float scale, float speed) :
  rm_widget(x, y, width, height, p_parent, "ui_animation"), m_image(img), m_speed(speed), m_angle(start_angle), m_scale(scale)
{
}

rm_animation::~rm_animation()
{
}

rm_widget* rm_scrollbar::find_other_scrollbars()
{
  for (size_t i = 0; i < m_pparent->get_num_childs(); i++) {
    rm_widget* pchild = m_pparent->get_child(i);
    if (pchild != this && !strcmp(pchild->get_classname(), "ui_scrollbar")) {
      return pchild;
    }
  }
  return nullptr;
}

void rm_scrollbar::adjust_position()
{
  /* set position of parent */
  //rm_widget* pother_scroll = find_other_scrollbars();
  //rm_vec2& parent_abs = m_pparent->get_absolute();
  rm_vec2& parent_size = m_pparent->get_size();
  if (get_orient() == RM_ORIENT_HORZ) {
    m_size.init(parent_size.x, m_pstyle->get_thumb_size());
    m_absolute.init(m_absolute.x, m_absolute.y + parent_size.y - m_pstyle->get_thumb_size());
  }
  else {
    m_size.init(m_pstyle->get_thumb_size(), parent_size.y);
    m_absolute.init(m_absolute.x + parent_size.x - m_pstyle->get_thumb_size(), m_absolute.y);
  }
}

void rm_scrollbar::on_draw(NVGcontext* p_ctx)
{
  rm_vec2 content_rect(1000, 1000);
  draw_scroll(p_ctx, m_pstyle, content_rect, m_size, m_pstyle->get_thumb_size(), m_position);
  rm_widget::on_draw(p_ctx);
}

bool rm_scrollbar::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos)
{
  return true;
}

rm_scrollbar::rm_scrollbar(rm_widget* p_parent, RM_ORIENT orient, rm_scroll_style* p_style, float inital_pos) :
  rm_widget(0, 0, 0, 0, p_parent, "ui_scrollbar", EXGUI_FLAG_DEFAULT|EXGUI_FLAG_GLOBAL), m_position(inital_pos)
{
  set_style(p_style);
  set_orient(orient);
  adjust_position();
}

rm_scrollbar::~rm_scrollbar()
{
}

rm_tabcontrol::rm_tabcontrol(rm_widget* p_parent, int x, int y, int width, int height,
  rm_tabcontrol_cb cb)
  : rm_widget(x, y, width, height, p_parent, "ui_tabcontrol",
    EXGUI_FLAG_DEFAULT, 0, nullptr),
  m_selected(0)
{
  set_callback(cb);
  set_zindex(998);
}

rm_widget* rm_tabcontrol::add_tab(const char* pname, void* puserdata)
{
  rm_vec2 widget_pos, widget_size;
  get_widget_size(widget_pos, widget_size);
  rm_widget* page = new rm_widget(
    int(widget_pos.x), int(widget_pos.y), int(widget_size.x), int(widget_size.y),
    this,
    "ui_tabpage",
    EXGUI_FLAG_DEFAULT,
    0, nullptr
  );

  m_tabs.emplace_back(pname, page, puserdata);
  update_children_active();
  return page;
}

rm_widget* rm_tabcontrol::find_tab(const char* pname)
{
  auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
    [pname](rm_tab_item& item) { return strcmp(item.get_name(), pname) == 0; }
  );

  if (it != m_tabs.end())
    return it->get_page();

  return nullptr;
}

void rm_tabcontrol::set_selected_index(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(m_tabs.size()))
    return;
  m_selected = idx;
  update_children_active();
  if (is_valid_callback())
    get_callback()(this, &m_tabs[m_selected], m_selected);
}

void rm_tabcontrol::get_tabcontrol_size(rm_vec2& dst)
{
  if (m_pstyle->is_horizontal()) {
    dst.init(m_size.x, m_pstyle->get_tab_height());
    return;
  }
  dst.init(m_pstyle->get_tab_height(), m_size.y);
}

void rm_tabcontrol::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
  nvgFontFaceId(p_ctx, get_font());

  // background
  //nvgBeginPath(p_ctx);
  //nvgRoundedRect(p_ctx, 0.f, 0.f, m_relative.width, m_relative.height, 4.0f);
  //nvgFillColor(p_ctx, m_pstyle->get_background_color());
  //nvgFill(p_ctx);
  //nvgStrokeColor(p_ctx, m_pstyle->get_border_color());
  //nvgStroke(p_ctx);
  bool is_horizontal = m_pstyle->is_horizontal();
  size_t num_tabs = m_tabs.size();
  if (!num_tabs)
    return;

  rm_vec2 tab_size;
  get_one_tab_size(tab_size);

  nvgFontSize(p_ctx, m_pstyle->get_font_size());
  nvgTextAlign(p_ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

  for (size_t i = 0; i < num_tabs; ++i) {
    bool is_first = i == 0;
    bool is_last = i == num_tabs - 1;
    float x = (is_horizontal ? i * tab_size.x : 0.f);
    float y = (is_horizontal ? 0.f : i * tab_size.y);

    nvgBeginPath(p_ctx);
    nvgFillColor(p_ctx, (int(i) == m_selected) ? m_pstyle->get_selected_color() : m_pstyle->get_unselected_color());
    nvgRoundedRectVarying(p_ctx, x, y, tab_size.x, tab_size.y,
      is_first ? m_pstyle->get_corner_radius(LEFT_TOP) : 0.f,
      (is_horizontal ? is_last : is_first) ? m_pstyle->get_corner_radius(RIGHT_TOP) : 0.f,
      is_last ? m_pstyle->get_corner_radius(RIGHT_BOTTOM) : 0.f,
      (is_horizontal ? is_first : is_last) ? m_pstyle->get_corner_radius(LEFT_BOTTOM) : 0.f);
    nvgFill(p_ctx);
    nvgStrokeColor(p_ctx, m_pstyle->get_border_color());
    nvgStroke(p_ctx);
    nvgFillColor(p_ctx, m_pstyle->get_text_color());
    nvgText(p_ctx, (x + tab_size.x * 0.5f) + m_pstyle->get_text_offsets().x,
      (y + tab_size.y * 0.5f) + m_pstyle->get_text_offsets().y,
      m_tabs[i].get_name(),
      NULL);
    rm_widget::on_draw(p_ctx);
  }
}

bool rm_tabcontrol::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) {
  int     idx;
  rm_vec2 tabcontrol_size;
  get_tabcontrol_size(tabcontrol_size);
  m_bbox.init(m_absolute, tabcontrol_size);
  rm_vec2 local_mouse_pos = cursor_to_local(cursor_pos);
  size_t num_tabs = m_tabs.size();
  rm_vec2 sz;
  get_one_tab_size(sz);
  float axis = m_pstyle->is_horizontal() ? local_mouse_pos.x : local_mouse_pos.y;
  if (num_tabs && event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
    idx = int(axis / (m_pstyle->is_horizontal() ? sz.x : sz.y));
    if (idx >= 0 && idx < int(num_tabs)) {
      set_selected_index(idx);
      return false;
    }
  }
  return true;
}

void rm_tabcontrol::update_children_active()
{
  rm_widget* pwidget;
  for (size_t t = 0; t < get_num_childs(); ++t) {
    bool is_active = int(t) == m_selected;
    pwidget = get_child(t);
    pwidget->show(is_active);
    pwidget->set_enabled(is_active);
  }
}

void rm_tabcontrol::get_widget_size(rm_vec2& dst_pos, rm_vec2& dst_size)
{
  assert(m_pstyle && "m_pstyle was nullptr");
  rm_vec2 tab_size;
  get_tabcontrol_size(tab_size);
  if (m_pstyle->is_horizontal()) {
    dst_pos.init(0.f, tab_size.y);
    dst_size.init(tab_size.x, m_size.y - tab_size.y);
  }
  else {
    dst_pos.init(tab_size.x, 0.f);
    dst_size.init(m_size.x - tab_size.x, m_size.y);
  }
}

void rm_tabcontrol::get_one_tab_size(rm_vec2& dst_size)
{
  assert(m_pstyle && "m_pstyle was nullptr");
  size_t num_tabs = m_tabs.size();
  if (num_tabs) {
    bool is_horizontal = m_pstyle->is_horizontal();
    if (is_horizontal) {
      dst_size.x = m_size.x / float(num_tabs);
      dst_size.y = m_pstyle->get_tab_height();
    }
    else {
      dst_size.x = m_pstyle->get_tab_height();
      dst_size.y = m_size.y / float(num_tabs);
    }
  }
}

void rm_treeview::on_draw(NVGcontext* p_ctx) {
  //m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, m_rowHeight * 0.8f);
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

  // background
  nvgBeginPath(p_ctx);
  nvgRect(p_ctx, 0.f, 0.f, m_size.x, m_size.y);
  nvgFillColor(p_ctx, nvgRGBA(245, 245, 245, 255));
  nvgFill(p_ctx);

  float y = 0.f;
  for (auto root : m_roots) {
    y = draw_node(p_ctx, root, 0.f, y);
    if (y > m_size.y)
      break; // clip
  }
  rm_widget::on_draw(p_ctx);
}

float rm_treeview::draw_node(NVGcontext* p_ctx, rm_tree_node* node, float x, float y) {
  // background if selected
  if (node == m_selected) {
    nvgBeginPath(p_ctx);
    nvgRect(p_ctx, x, y, m_size.x - x, m_rowHeight);
    nvgFillColor(p_ctx, nvgRGBA(200, 230, 255, 255));
    nvgFill(p_ctx);
  }
  // expand/collapse icon
  if (!node->children.empty()) {
    const float sz = m_rowHeight * 0.5f;
    float cx = x + (m_indent - sz) * 0.5f;
    float cy = y + (m_rowHeight - sz) * 0.5f;
    nvgBeginPath(p_ctx);

    //TODO: k.d replace by icons
    if (node->expanded) {
      // draw '-'
      nvgMoveTo(p_ctx, cx, cy + sz / 2);
      nvgLineTo(p_ctx, cx + sz, cy + sz / 2);
    }
    else {
      // draw '+'
      nvgMoveTo(p_ctx, cx, cy + sz / 2);
      nvgLineTo(p_ctx, cx + sz, cy + sz / 2);
      nvgMoveTo(p_ctx, cx + sz / 2, cy);
      nvgLineTo(p_ctx, cx + sz / 2, cy + sz);
    }
    nvgStrokeColor(p_ctx, nvgRGBA(100, 100, 100, 255));
    nvgStroke(p_ctx);
  }
  // draw text
  float tx = x + m_indent;
  float ty = y + m_rowHeight * 0.5f;
  nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  nvgText(p_ctx, tx, ty, node->name.c_str(), nullptr);

  y += m_rowHeight;
  // draw children
  if (node->expanded) {
    for (auto child : node->children) {
      y = draw_node(p_ctx, child, x + m_indent, y);
      if (y > m_size.y)
        break;
    }
  }
  return y;
}

bool rm_treeview::on_mouse(EXGUI_MOUSE_EVENT event,
  EXGUI_KEY vk,
  EXGUI_KEY_STATE state,
  rm_vec2& cursor_pos)
{
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN)
  {
    rm_tree_node* hitNode = nullptr;
    float y = m_absolute.y;
    if (hit_test(cursor_pos,
      nullptr,
      m_absolute.x,
      y,
      hitNode)
      && hitNode)
    {
      if (!hitNode->children.empty()) {
        hitNode->expanded = !hitNode->expanded;
        root_update();
      }
      m_selected = hitNode;
      if (is_valid_callback())
        get_callback()(this, m_selected);
      return false;
    }
  }
  return true;
}

bool rm_treeview::hit_test(const rm_vec2& pos,
  rm_tree_node* node,
  float x,
  float& y,
  rm_tree_node*& out)
{
  if (!node) {
    for (auto root : m_roots) {
      if (hit_test(pos, root, x, y, out)) return true;
    }
    return false;
  }

  if (pos.x >= x && pos.x <= x + m_size.x &&
    pos.y >= y && pos.y < y + m_rowHeight)
  {
    out = node;
    return true;
  }

  y += m_rowHeight;

  if (node->expanded) {
    for (auto child : node->children) {
      if (hit_test(pos, child, x + m_indent, y, out)) return true;
    }
  }

  return false;
}