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
  m_bbox.from_rect(m_absolute);
  //nvgBeginPath(p_ctx);
  //nvgRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height);
  //nvgFillColor(p_ctx, nvgRGBA(200, 200, 200, 255));
  //nvgFill(p_ctx);

  if (m_image && m_image->imageId != -1) {
    NVGpaint imgPaint = nvgImagePattern(p_ctx,
      m_relative.x, m_relative.y,
      m_relative.width, m_relative.height,
      0.0f, m_image->imageId, 1.0f);
    nvgBeginPath(p_ctx);
    nvgRoundedRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 4.0f);
    nvgFillPaint(p_ctx, imgPaint);
    nvgFill(p_ctx);
  }
}

bool rm_image_button::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
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
  m_bbox.from_rect(m_absolute);
  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, 20.0f);

  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 4.0f);
  NVGcolor fillColor = nvgRGBA(100, 100, 250, 255);
  if (m_elem_flags.is_hovered())
    fillColor = nvgRGBA(120, 120, 255, 255);
  nvgFillColor(p_ctx, fillColor);
  nvgFill(p_ctx);

  nvgTextAlign(p_ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(255, 255, 255, 255));
  float cx = m_relative.x + m_relative.width / 2.0f;
  float cy = m_relative.y + m_relative.height / 2.0f;
  nvgText(p_ctx, cx, cy, m_text.c_str(), nullptr);
}

bool rm_button::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
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
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
  nvgFillColor(p_ctx, nvgRGBA(255, 255, 255, 255));
  nvgText(p_ctx, m_relative.x, m_relative.y, m_text.c_str(), nullptr);
}

rm_text_input::rm_text_input(rm_widget* p_parent, int x, int y, int width, int height,
  uint32_t flags, float blink_cursor_interval)
  : rm_widget(x, y, width, height, p_parent, "ui_text_input"), m_active(false)
{
  m_blink_state = false;
  m_timer.set_interval(blink_cursor_interval);
  m_text_offset = 0.f;
  m_flags = flags;
}

rm_text_input::~rm_text_input() {}

void rm_text_input::on_draw(NVGcontext* p_ctx) {
  m_bbox.from_rect(m_absolute);
  nvgFontFaceId(p_ctx, get_font());

  //nvgScissor(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height); //NOTE: K.D. added 12.03.2025

  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 4.0f);
  NVGcolor bgColor = m_active ? nvgRGBA(255, 255, 255, 255) : nvgRGBA(230, 230, 230, 255);
  nvgFillColor(p_ctx, bgColor);
  nvgFill(p_ctx);

  nvgStrokeColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  nvgStroke(p_ctx);

  nvgFontSize(p_ctx, 18.0f);
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));

  float textY = m_relative.y + m_relative.height / 2.0f;

  if (!(m_flags & RMGUI_TEXT_INPUT_MULTILINE)) {
    float availableWidth = m_relative.width - 10;
    float textWidth = nvgTextBounds(p_ctx, 0, 0, m_text.c_str(), nullptr, nullptr);
    if (textWidth > availableWidth)
      m_text_offset = textWidth - availableWidth;
    else
      m_text_offset = 0;

    nvgText(p_ctx, m_relative.x + 5 - m_text_offset, textY, m_text.c_str(), nullptr);
  }
  else {
    float availableWidth = m_relative.width - 10;
    nvgTextBox(p_ctx, m_relative.x + 5, m_relative.y + 5, availableWidth, m_text.c_str(), nullptr);
  }

  if (m_timer.has_elapsed(get_sysdf())) {
    m_blink_state = !m_blink_state;
  }

  if (m_active && m_blink_state) {
    float tw = nvgTextBounds(p_ctx, 0, 0, m_text.c_str(), nullptr, nullptr);
    nvgBeginPath(p_ctx);
    nvgMoveTo(p_ctx, m_relative.x + 5 - m_text_offset + tw + 2, m_relative.y + 4);
    nvgLineTo(p_ctx, m_relative.x + 5 - m_text_offset + tw + 2, m_relative.y + m_relative.height - 4);
    nvgStrokeColor(p_ctx, nvgRGBA(0, 0, 0, 255));
    nvgStroke(p_ctx);
  }
  //nvgResetScissor(p_ctx);
}


bool rm_text_input::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN) {
    m_active = m_bbox.inside(cursor_pos);
    return false;
  }
  return true;
}

void rm_text_input::on_text_input(int sym) {
  if (m_active) {

    //TODO: K.D. CREATE CLASS rmgui_textbuffer

    printf("keycode: %d\n", sym);
    if (sym == 8) { // backspace
      if (!m_text.empty())
        m_text.pop_back();
    }
    else {
      m_text.push_back(static_cast<char>(sym));
    }
  }
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
  m_bbox.from_rect(m_absolute);
  nvgFontFaceId(p_ctx, get_font());

  /* paint background */
  nvgBeginPath(p_ctx);
  nvgRoundedRectVarying(p_ctx,
    m_relative.x, m_relative.y, m_relative.height, m_relative.height,
    m_pstyle->get_corner_radius(LEFT_TOP),
    m_pstyle->get_corner_radius(RIGHT_TOP),
    m_pstyle->get_corner_radius(RIGHT_BOTTOM),
    m_pstyle->get_corner_radius(LEFT_BOTTOM)
  );
  nvgFillColor(p_ctx, m_pstyle->get_background_color());
  nvgFill(p_ctx);
  nvgStrokeWidth(p_ctx, 1.f);
  nvgStrokeColor(p_ctx, m_pstyle->get_border_color());
  nvgStroke(p_ctx);

  if (m_checked) {
    /* draw mark */
    nvgFontFaceId(p_ctx, m_icon_font);
    nvgFontSize(p_ctx, 16.f);
    nvgTextAlign(p_ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(p_ctx, m_pstyle->get_text_color());
    xo = m_relative.height / 2.f;
    yo = m_relative.height / 2.f;
    nvgText(p_ctx, m_relative.x + xo, m_relative.y + yo, ICON_FA_CHECK, nullptr);
  }

  nvgFontFaceId(p_ctx, get_font());
  nvgFontSize(p_ctx, m_pstyle->get_font_size());
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, m_pstyle->get_text_color());

  const rmgui_vector2& text_offsets = m_pstyle->get_text_offsets();
  nvgText(p_ctx, m_relative.x + m_relative.height + text_offsets.x, (m_relative.y + m_relative.height / 2.0f) + text_offsets.y, m_label.c_str(), nullptr);
}

bool rm_checkbox::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == UP && m_bbox.inside(cursor_pos)) {
    m_checked = !m_checked;
    std::cout << "Checkbox \"" << m_label << "\" now " << (m_checked ? "checked" : "unchecked") << std::endl;
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
  m_bbox.from_rect(m_absolute);
  nvgFontFaceId(p_ctx, get_font());

  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 4.0f);
  nvgFillColor(p_ctx, nvgRGBA(180, 180, 180, 255));
  nvgFill(p_ctx);
  nvgStrokeColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  nvgStroke(p_ctx);

  nvgFontSize(p_ctx, 18.0f);
  nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));
  if (!m_items.empty() && m_selected >= 0 && m_selected < (int)m_items.size())
    nvgText(p_ctx, m_relative.x + 5, m_relative.y + m_relative.height / 2.0f, m_items[m_selected].get_name(), nullptr);

  if (m_expanded) {
    for (size_t i = 0; i < m_items.size(); i++) {
      rm_combo_item& item = m_items[i];
      float itemY = m_relative.y + m_relative.height * (1 + i);
      nvgBeginPath(p_ctx);
      nvgRect(p_ctx, m_relative.x, itemY, m_relative.width, m_relative.height);
      nvgFillColor(p_ctx, nvgRGBA(200, 200, 200, 255));
      nvgFill(p_ctx);
      nvgStrokeColor(p_ctx, nvgRGBA(0, 0, 0, 255));
      nvgStroke(p_ctx);
      nvgTextAlign(p_ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
      nvgFillColor(p_ctx, nvgRGBA(0, 0, 0, 255));
      nvgText(p_ctx, m_relative.x + 5, itemY + m_relative.height / 2.0f, item.get_name(), nullptr);
    }
  }
}

bool rm_combobox::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
  if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN) {
    if (m_bbox.inside(cursor_pos)) {
      m_expanded = true;
      return false;
    }

    if (m_expanded) {
      float itemYStart = m_absolute.y + m_relative.height;
      float itemHeight = m_relative.height;
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
    //printf("rm_combobox: absorb events\n");
    return false; //absorb the event
  }
  return true;
}

void rm_slider::compute_value(rmgui_vector2& cursor_pos)
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
  m_thumb_size = m_relative.height / 2.5f;
  m_inner_rect.x = m_relative.x + m_thumb_size;
  m_inner_rect.y = m_relative.y;
  m_inner_rect.width = m_relative.width - m_thumb_size * 2.f;
  m_inner_rect.height = m_relative.height;
}

rm_slider::rm_slider(rm_widget* p_parent, int x, int y, int width, int height, float min, float max, float initial, rm_slider_callback pcallback)
  : rm_widget(x, y, width, height, p_parent, "ui_slider", EXGUI_FLAG_DEFAULT|EXGUI_FLAG_GLOBAL), 
  m_min(min), m_max(max), m_value(initial), m_dragging(false), m_pcallback(pcallback)
{
  compute_inner_and_thumb();
}

rm_slider::~rm_slider() {}

void rm_slider::on_draw(NVGcontext* p_ctx) {
  m_bbox.from_rect(m_absolute);

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
}

bool rm_slider::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
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
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y,
    m_relative.width, m_relative.height, m_round);
  nvgFill(p_ctx);

  // paint progres bar
  float identity_percent = m_percent / 100.f; // 0.f-1.f
  rm_rect percent_rect = m_relative;
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
}

bool rm_progress_base::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos)
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
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y,
    m_relative.width, m_relative.height, m_round);
  nvgFill(p_ctx);

  // paint progres bar
  float identity_percent = m_percent / 100.f; // 0.f-1.f
  rm_rect percent_rect = m_relative;
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
}

void rm_scroll_base::orient_detect(const rm_rect& background)
{
  m_orientation = (background.width > background.height) ? RM_ORIENT_HORZ : RM_ORIENT_VERT;
}

void rm_scroll_base::draw_scroll(NVGcontext* p_ctx, const rm_rect& back, rm_scroll_style* pstyle, float pos)
{
  rm_rect thumb_rect;
  assert(m_orientation != RM_ORIENT_AUTO && "[K.D.] m_orientation have undefined value! You called rm_scroll_base::orient_detect() from init/resize?");
  if (m_orientation == RM_ORIENT_HORZ) {
    thumb_rect.x = back.width * pos;
    thumb_rect.y = back.y;
    thumb_rect.width = pstyle->get_thumb_size();
    thumb_rect.height = back.height;
  }
  else {
    thumb_rect.x = back.x;
    thumb_rect.y = back.height * pos;
    thumb_rect.width = back.width;
    thumb_rect.height = pstyle->get_thumb_size();
  }

  /* paint background */
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_background_color());
  nvgRoundedRect(p_ctx, back.x, back.y, back.width, back.height, pstyle->get_scroll_corner_radius());
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

void rm_scroll_base::draw_scroll(NVGcontext* p_ctx, rm_scroll_style* pstyle, rm_rect content, const rm_rect& window, float thumb_thickness, float pos)
{
  rm_rect thumb_rect;
  float   thumb_size;
  assert(m_orientation != RM_ORIENT_AUTO && "[K.D.] m_orientation have undefined value! You called rm_scroll_base::orient_detect() from init/resize?");
  if (m_orientation == RM_ORIENT_HORZ) {
    if (content.width <= window.width)
      content.width = window.width;

    float scroll_area = window.width;
    thumb_size = (window.width / content.width) * scroll_area;
    thumb_size = std::max(thumb_thickness, thumb_size); // thumb min width

    float max_offset = scroll_area - thumb_size;
    float thumb_x = window.x + pos * max_offset;

    thumb_rect.x = thumb_x;
    thumb_rect.y = window.y + (window.height - thumb_thickness) * 0.5f;
    thumb_rect.width = thumb_size;
    thumb_rect.height = thumb_thickness;
  }
  else {
    if (content.height <= window.height)
      content.height = window.height;

    float scroll_area = window.height;
    thumb_size = (window.height / content.height) * scroll_area;
    thumb_size = std::max(thumb_thickness, thumb_size); // thumb min height

    float max_offset = scroll_area - thumb_size;
    float thumb_y = window.y + pos * max_offset;

    thumb_rect.x = window.x + (window.width - thumb_thickness) * 0.5f;
    thumb_rect.y = thumb_y;
    thumb_rect.width = thumb_thickness;
    thumb_rect.height = thumb_size;
  }

  // paint background
  float round_radius = (window.height / 2.f) * pstyle->get_scroll_corner_radius();
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, pstyle->get_scroll_background_color());
  nvgRoundedRect(p_ctx, window.x, window.y, window.width, window.height, round_radius);
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
  rmgui_vector2 pos(m_relative.width / 2.f, m_relative.height / 2.f);
  nvgTranslate(p_ctx, pos.x, pos.y);
  nvgRotate(p_ctx, m_angle);
  nvgScale(p_ctx, m_scale, m_scale);
  nvgTranslate(p_ctx, -pos.x, -pos.y);
  nvgBeginPath(p_ctx);
  NVGpaint imgPaint = nvgImagePattern(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 0.0f, m_image, 1.0f);
  nvgBeginPath(p_ctx);
  nvgRoundedRect(p_ctx, m_relative.x, m_relative.y, m_relative.width, m_relative.height, 4.0f);
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
  rm_widget* pother_scroll = find_other_scrollbars();
  rm_rect& parent_rel = m_pparent->get_relative();
  rm_rect& parent_abs = m_pparent->get_absolute();
  m_relative.x = 0;
  m_relative.y = 0;
  if (get_orient() == RM_ORIENT_HORZ) {
    m_relative.width = parent_rel.width;
    m_relative.height = m_pstyle->get_thumb_size();
    m_absolute.x = parent_abs.x;
    m_absolute.y = parent_abs.y + parent_rel.height - m_pstyle->get_thumb_size();
  }
  else {
    m_relative.width = m_pstyle->get_thumb_size();
    m_relative.height = parent_rel.height;
    m_absolute.x = parent_abs.x + parent_rel.width - m_pstyle->get_thumb_size();
    m_absolute.y = parent_abs.y;
  }
  m_absolute.width = m_relative.width;
  m_absolute.height = m_relative.height;
}

void rm_scrollbar::on_draw(NVGcontext* p_ctx)
{
  rm_rect content_rect(0, 0, 1000, 1000);
  draw_scroll(p_ctx, m_pstyle, content_rect, m_relative, 20, m_position);
}

bool rm_scrollbar::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos)
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
