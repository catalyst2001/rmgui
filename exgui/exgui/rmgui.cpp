#include <algorithm>
#include <cstdarg>

#include "rmgui.h"

void rm_widget::move_recursive(rm_widget* pwidget, float newx, float newy)
{
  pwidget->m_absolute.x += newx;
  pwidget->m_absolute.y += newy;
  for (size_t i = 0; i < pwidget->get_num_childs(); i++) {
    rm_widget* pchild = pwidget->get_child(i);
    assert(pchild && "pchild was nullptr");
    move_recursive(pchild, newx, newy);
  }
}

bool rm_widget::add_child(rm_widget* p_child)
{
  /* prevent add nullptr */
  if (!p_child)
    return false;

  /* prevent enter infinity loop in any event and stack overflowing */
  if (this == p_child)
    return false;

  /* if element with no childs */
  if(!m_elem_flags.has_childs())
    return false;

  /* have parent? */
  p_child->grab_globals_from(this);

  /* need events handling highest priority? */
  if (p_child->get_elem_flags().is_set(EXGUI_FLAG_HIGHEST_PRIORITY)) {
    /* add child first in list */
    m_childs.insert(m_childs.begin(), p_child);
  }
  else {
    /* add child last */
    m_childs.push_back(p_child);
  }
  root_update();
  return true;
}

bool rm_widget::remove_child(rm_widget* p_child)
{
  /* prevent remove nullptr */
  if (!p_child)
    return false;

  /* if element with no childs */
  if (!m_elem_flags.has_childs())
    return false;

  _childs_vec::iterator it = std::find(m_childs.begin(), m_childs.end(), p_child);
  if (it != m_childs.end()) {
    /* child found */
    m_childs.erase(it);
    p_child->on_event(PARENT_CHANGED, this);
    p_child->set_parent(nullptr);
  }
  return true;
}

rm_widget* rm_widget::find_child_by_classname(const char* pclassname) const
{
  return nullptr;
}

void rm_widget::set_parent(rm_widget* p_parent)
{
  m_pparent = p_parent;
  root_update();
}

void rm_widget::resize(int width, int height)
{
  rm_vec2 start(width, height);
  m_size.init(width, height);
  m_bbox.init(start, m_size);
}

void rm_surface::event_dispatcher(rm_widget* p_elem)
{
  RM_UNUSED(p_elem);
}

void rm_surface::keybd_dispatcher(rm_widget* p_elem, int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state)
{
  p_elem->on_keybd(sc, vk, state);
  /* element has childs? */
  if (p_elem->get_elem_flags().has_active() && 
    p_elem->get_elem_flags().has_childs() && 
    p_elem->get_elem_flags().has_notify_childs()) {
    /* recursive enum childs */
    for (size_t i = 0; i < p_elem->get_num_childs(); i++) {
      /* enter recursively */
      keybd_dispatcher(p_elem->get_child(i), sc, vk, state);
    }
  }
}

#if 0
void rmgui_surface::text_input_dispatcher(rmgui_widget* p_elem, int sym)
{
  p_elem->on_text_input(sym);
  /* element has childs? */
  if (p_elem->get_elem_flags().has_childs() && p_elem->get_elem_flags().has_notify_childs()) {
    /* recursive enum childs */
    for (size_t i = 0; i < p_elem->get_num_childs(); i++) {
      /* enter recursively */
      text_input_dispatcher(p_elem->get_child(i), sym);
    }
  }
}
#endif

bool rm_surface::mouse_dispatcher(rm_widget* p_elem,
  EXGUI_MOUSE_EVENT event,
  EXGUI_KEY vk,
  EXGUI_KEY_STATE state,
  rm_vec2& cursor_pos)
{
  bool b_call_next = true;
  bool b_cursor_inside = p_elem->get_bbox().inside(cursor_pos);
  bool b_global_receive_events = p_elem->get_elem_flags().is_set(EXGUI_FLAG_GLOBAL);
  if (b_cursor_inside || b_global_receive_events) {

    b_call_next = p_elem->on_mouse(event, vk, state, cursor_pos);
    if (event == EXGUI_MOUSE_EVENT_CLICK && state == DOWN && p_elem != this) {
      if (!(b_global_receive_events && !b_cursor_inside)) {
        m_pfocus = p_elem;
        printf("updated focus to element %s\n", p_elem->get_classname());
      }
    }
  }

  p_elem->m_elem_flags.toggle_bits(EXGUI_FLAG_HOVERED, b_cursor_inside);
  if (!b_call_next)
    return false; //this event was break by p_elem
    
  rm_vec2 local = p_elem->cursor_to_local(cursor_pos);
  const rm_rect& content = p_elem->get_content_area();
  rm_vec2 child_cursor{
      local.x - content.x,
      local.y - content.y
  };
  if (p_elem->get_elem_flags().has_active() && 
    p_elem->get_elem_flags().has_childs() &&
    p_elem->get_elem_flags().has_notify_childs()) {
    for (size_t i = 0; i < p_elem->get_num_childs(); i++) {
      if (!mouse_dispatcher(p_elem->get_child(i), event, vk, state, child_cursor)) {
        return false;
      }
    }
  }
  return true; //continue handling next
}

void rm_surface::draw_recursive(rm_widget* pwidget, float dt)
{
  assert(pwidget && "pwidget was nullptr");
  if (!pwidget->get_elem_flags().has_visible())
    return; //invisible

  rm_vec2& abs_pos = pwidget->get_absolute();
  rm_vec2& size = pwidget->get_size();
  rm_rect& content = pwidget->get_content_area();

  nvgSave(m_pctx);

  /* disabled scissoring? */
  if (!pwidget->get_elem_flags().is_set(EXGUI_FLAG_DISABLE_SCISSOR))
    nvgScissor(m_pctx, abs_pos.x, abs_pos.y, size.x, size.y);

  nvgTranslate(m_pctx, abs_pos.x + content.x, abs_pos.y + content.y);
  pwidget->on_draw(m_pctx);

  /* element has childs? */
  if (pwidget->get_elem_flags().has_childs()) {
    /* recursive enum childs */
    for (size_t i = 0; i < pwidget->get_num_childs(); i++) {
      /* enter recursively */
      draw_recursive(pwidget->get_child(i), dt);
    }
  }
  //nvgResetTransform(m_pctx);
  nvgResetScissor(m_pctx);
  nvgRestore(m_pctx);

#ifdef RMGUI_DEBUG_DRAW
  /* draw absolute position for debug */
  nvgBeginPath(m_pctx);
  nvgFillColor(m_pctx, nvgRGB(0, 0, 255));
  nvgCircle(m_pctx, abs_pos.x, abs_pos.y, 2.f);
  nvgFill(m_pctx);
#endif
}

//layer_draw_cache *rm_surface::get_layer_by_zindex(int zid)
//{
//  layer_draw_cache* pcache;
//  auto it = std::find_if(m_layers.begin(), m_layers.end(),
//    [zid](layer_draw_cache *player) {
//      return player->get_zindex() == zid;
//    }
//  );
//
//  if (it != m_layers.end())
//    return *it;
//  
//  pcache = new (std::nothrow)layer_draw_cache(zid);
//  if (!pcache)
//    return nullptr;
//
//  m_layers.push_back(pcache);
//  std::sort(m_layers.begin(), m_layers.end(), 
//    [](layer_draw_cache *pa, layer_draw_cache* pb) {
//      return pa->get_zindex() < pb->get_zindex();
//    }
//  );
//  return pcache;
//}

void rm_surface::draw(float dt)
{
  m_delta_time = dt;
  nvgBeginFrame(m_pctx, m_size.x, m_size.y, 1.f);
  draw_recursive(this, dt);
  nvgEndFrame(m_pctx);
}

void rm_surface::keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state)
{
  keybd_dispatcher(this, sc, vk, state);
}

void rm_surface::textinput(int sym)
{
#if 0
  text_input_dispatcher(this, sym);
#endif
  if (m_pfocus)
    m_pfocus->on_text_input(sym);
}

void rm_surface::mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, int x, int y)
{
  rm_vec2 mouse_pos(x, y);
  mouse_dispatcher(this, event, vk, state, mouse_pos);
}

rm_image rm_surface::load_image_from_memory(const void* psrc, size_t srclen, int flags)
{
  rm_image handle;
  set_handle_value(handle, nvgCreateImageMem(m_pctx, flags, (uint8_t*)psrc, (int)srclen));
  return handle;
}

rm_image rm_surface::load_image(const char* pfilename, int flags)
{
  rm_image handle;
  set_handle_value(handle, nvgCreateImage(m_pctx, pfilename, flags));
  return handle;
}

void rm_surface::free_image(rm_image& image)
{
  if (image.is_valid()) {
    nvgDeleteImage(m_pctx, image.get_handle());
    set_handle_value(image, image.get_invalid()); //NOTE: K.D. invalidate
  }
}

rm_font rm_surface::load_font_from_memory(const void* psrc_ttf_mem, size_t srclen, const char* pfontname)
{
  rm_font font = find_font(pfontname);
  if (!font.is_valid())
    set_handle_value(font, nvgCreateFontMem(m_pctx, pfontname, (uint8_t * )psrc_ttf_mem, static_cast<int>(srclen), 0));

  return font;
}

rm_font rm_surface::load_font(const char* pfilename, const char* pfontname)
{
  rm_font font = find_font(pfontname);
  if (!font.is_valid())
    set_handle_value(font, nvgCreateFont(m_pctx, pfontname, pfilename));

  return font;
}

rm_font rm_surface::find_font(const char* pfontname)
{
  rm_font font;
  set_handle_value(font, nvgFindFont(m_pctx, pfontname));
  return font;
}

void rm_surface::free_font(rm_font& font)
{
  //NOTE: K.D. nvg not free fonts
}

rm_surface::rm_surface(NVGcontext* p_ctx, int width, int height, irm_sysdf* p_sysdf) : rm_widget(0, 0, width, height, nullptr, "ui_root_node")
{
  m_psysdf = p_sysdf;
  set_root(this);
  m_pctx = p_ctx;
  m_pfocus = nullptr;
  m_delta_time = 0.f;
  load_font_from_memory(fontawesomewebfont, FONT_SIZE, "fontawesome");
}

rm_surface::~rm_surface()
{
}

void rm_window::on_draw(NVGcontext* p_ctx)
{
  rm_window_style* p_style = get_style();
  assert(p_style && "rmgui_window::on_draw(): window style is not set! Use rmgui_window::set_style(rmgui_wi1ndow_style *)");
  int b_is_active = (int)(get_elem_flags().is_focused() || get_elem_flags().is_hovered());
  /* draw window background */
  nvgBeginPath(p_ctx);
  nvgFillColor(p_ctx, p_style->get_background_color(b_is_active));
  nvgRoundedRectVarying(p_ctx,
    0.f, 0.f, m_size.x, m_size.y,
    p_style->get_corner_radius(LEFT_TOP), p_style->get_corner_radius(RIGHT_TOP),
    p_style->get_corner_radius(RIGHT_BOTTOM), p_style->get_corner_radius(LEFT_BOTTOM));
  nvgFill(p_ctx);
}

bool rm_window::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos)
{
  //move(
  //  cursor_pos.x - m_absolute.x,
  //  cursor_pos.y - m_absolute.y
  //);
  return true;
}

rm_window::rm_window(rm_widget* p_parent, int x, int y, int width, int height, uint32_t flags, uint32_t uflags, void* p_userptr) :
  rm_widget(x, y, width, height, p_parent, "ui_window", flags, uflags, p_userptr)
{
  set_zindex(-1);
}

rm_window::~rm_window()
{
}

bool rmgui_timer::has_elapsed(irm_sysdf* p_sysdf)
{
  float current_time = p_sysdf->get_time();
  if (current_time > m_next_time) {
    m_curr_time = current_time;
    m_next_time = m_curr_time + m_interval;
    return true;
  }
  return false;
}

void rmgui_utl::draw_border_frame(NVGcontext* ctx, rm_rect& rect, uint32_t mode, const NVGcolor colors[])
{
  const NVGcolor& color = colors[mode];

  //nvgBeginPath(ctx);
  //nvgStrokeWidth(ctx, 1.0f);
  //nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + (mPushed ? 0.5f : 1.5f), mSize.x() - 1,
  //  mSize.y() - 1 - (mPushed ? 0.0f : 1.0f), mTheme->mButtonCornerRadius);
  //nvgStrokeColor(ctx, mTheme->mBorderLight);
  //nvgStroke(ctx);

  //nvgBeginPath(ctx);
  //nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1,
  //  mSize.y() - 2, mTheme->mButtonCornerRadius);
  //nvgStrokeColor(ctx, mTheme->mBorderDark);
  //nvgStroke(ctx);
}

void rmgui_textbuffer::insert_cp(uint32_t cp)
{
  if (has_selection()) 
    delete_selection();

  save_undo();
  std::string u8;

  if (cp < 0x80) 
    u8.push_back(char(cp));
  else if (cp < 0x800) {
    u8.push_back(char(0xC0 | (cp >> 6)));
    u8.push_back(char(0x80 | (cp & 0x3F)));
  }
  else if (cp < 0x10000) {
    u8.push_back(char(0xE0 | (cp >> 12)));
    u8.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
    u8.push_back(char(0x80 | (cp & 0x3F)));
  }
  else {
    u8.push_back(char(0xF0 | (cp >> 18)));
    u8.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
    u8.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
    u8.push_back(char(0x80 | (cp & 0x3F)));
  }

  text.insert(cursor, u8);
  cursor += u8.size();
  clear_redo();
  clear_selection();
}

void rmgui_textbuffer::backspace()
{
  if (has_selection()) { 
    delete_selection(); 
    return; 
  }

  if (cursor == 0) 
    return;

  save_undo();
  size_t i = cursor - 1;
  while (i > 0 && (text[i] & 0xC0) == 0x80) --i;
  text.erase(i, cursor - i);
  cursor = i;
  clear_redo();
  clear_selection();
}

//void rmgui_textbuffer::cut_all()
//{
//  if (text.empty())
//    return;
//
//  save_undo();
//  clipboard = text;
//  text.clear();
//  cursor = 0;
//  clear_redo();
//  clear_selection();
//}

void rmgui_textbuffer::cut_selection(irm_sysdf* psysdf)
{
  if (!has_selection()) 
    return;

  save_undo();
  size_t a = std::min(sel_start, sel_end);
  size_t b = std::max(sel_start, sel_end);
  std::string to_clipboard = text.substr(a, b - a);
  psysdf->set_clipboard_data_ex((const uint8_t *)to_clipboard.data(), to_clipboard.size());
  text.erase(a, b - a);
  cursor = a;
  clear_redo();
  clear_selection();
}

void rmgui_textbuffer::copy_all(irm_sysdf* psysdf)
{
  psysdf->set_clipboard_data_ex((const uint8_t *)text.c_str(), text.size());
}

void rmgui_textbuffer::paste(irm_sysdf* psysdf)
{
  size_t clipboard_data_size;
  EXGUI_CB_DATA_TYPE clipboard_dtype;
  const char* pclipboard_text = nullptr;
  if (has_selection()) 
    delete_selection();

  pclipboard_text = (const char*)psysdf->get_clipboard_data_ex(clipboard_dtype, clipboard_data_size);
  if (!pclipboard_text)
    return;

  /* skip binary data*/
  if (!clipboard_data_size || clipboard_dtype != EXGUI_CLIPBOARD_DATA_TYPE_TEXT)
    return;

  save_undo(); 
  text.insert(cursor, pclipboard_text);
  cursor += clipboard_data_size;
  clear_redo(); 
  clear_selection();
}

void rmgui_textbuffer::undo()
{
  if (undos.empty())
    return;

  redos.push_back({ text, cursor, sel_start, sel_end });
  auto s = undos.back();
  undos.pop_back();
  text = s.text;
  cursor = s.cur;
  sel_start = s.sel_start;
  sel_end = s.sel_end;
}

void rmgui_textbuffer::redo()
{
  if (redos.empty())
    return;

  undos.push_back({ text, cursor, sel_start, sel_end });
  auto s = redos.back();
  redos.pop_back();
  text = s.text;
  cursor = s.cur;
  sel_start = s.sel_start;
  sel_end = s.sel_end;
}

void rmgui_textbuffer::move_cursor_left()
{
  clear_selection();
  if (cursor == 0) 
    return;

  size_t i = cursor - 1;
  while (i > 0 && (text[i] & 0xC0) == 0x80) --i;
  cursor = i;
}

void rmgui_textbuffer::move_cursor_right()
{
  clear_selection();
  if (cursor >= text.size()) 
    return;

  size_t i = cursor + 1;
  while (i < text.size() && (text[i] & 0xC0) == 0x80) ++i;
  cursor = i;
}

void rmgui_textbuffer::move_cursor_up()
{
  clear_selection();
  size_t pos = cursor;
  auto prev_nl = (pos == 0 ? std::string::npos : text.rfind('\n', pos - 1));

  if (prev_nl == std::string::npos)
    return;

  size_t line0_start = prev_nl + 1;
  size_t col = pos - line0_start;

  auto prev2_nl = (prev_nl == 0 ? std::string::npos : text.rfind('\n', prev_nl - 1));
  size_t line1_start = (prev2_nl == std::string::npos ? 0 : prev2_nl + 1);
  size_t line1_end = prev_nl;
  size_t len1 = line1_end - line1_start;

  cursor = line1_start + std::min(col, len1);
}

void rmgui_textbuffer::move_cursor_down()
{
  clear_selection();
  size_t pos = cursor;
  auto next_nl = text.find('\n', pos);
  if (next_nl == std::string::npos)
    return;

  size_t line0_start = (pos == 0 ? 0 : text.rfind('\n', pos - 1) + 1);
  size_t col = pos - line0_start;

  size_t line1_start = next_nl + 1;
  auto next2_nl = text.find('\n', line1_start);
  size_t line1_end = (next2_nl == std::string::npos ? text.size() : next2_nl);
  size_t len1 = line1_end - line1_start;

  cursor = line1_start + std::min(col, len1);
}

void rmgui_textbuffer::delete_forward()
{
  if (has_selection()) {
    delete_selection();
    return;
  }

  if (cursor == 0)
    return;

  save_undo();

  size_t i = cursor - 1;
  while (i > 0 && (text[i] & 0xC0) == 0x80) {
    --i;
  }
  text.erase(i, cursor - i);
  cursor = i;

  clear_redo();
  clear_selection();
}

void rmgui_textbuffer::save_undo()
{
  undos.push_back({ text, cursor, sel_start, sel_end });
  if (undos.size() > 100)
    undos.erase(undos.begin());
}

void rmgui_textbuffer::delete_selection()
{
  if (!has_selection())
    return;

  save_undo();
  size_t a = std::min(sel_start, sel_end);
  size_t b = std::max(sel_start, sel_end);
  text.erase(a, b - a);
  cursor = a;
  clear_redo();
  clear_selection();
}

rm_line_ring_buffer::rm_rb_line::rm_rb_line() : m_cursor(0), m_sel_begin(0), m_sel_end(0)
{
}



bool rm_line_ring_buffer::rm_rb_line::set_string(const char* pstr)
{
  RM_HANDLE_EXCEPTIONS(false,
    m_line.assign(pstr);
  m_cursor = m_line.length();
    )
    return true;
}

bool rm_line_ring_buffer::rm_rb_line::insert_from_cursor(const char* pstr)
{
  RM_HANDLE_EXCEPTIONS(false,
    if (m_cursor < m_line.length()) {
      m_line.insert(m_cursor, pstr);
    }
    else {
      m_line.append(pstr);
    }
      )
    return false;
}

rm_line_ring_buffer::rm_rb_line* rm_line_ring_buffer::get_line_for_write()
{
  rm_rb_line* pline = get_line(m_start_line);
  m_start_line = (m_start_line + 1) % get_num_lines();
  pline->clear();
  return pline;
}

const char* rm_line_ring_buffer::format_string(std::string& dst, const char* pformat, va_list argptr)
{
  vsnprintf(&dst[0], dst.size(), pformat, argptr);
  return dst.c_str();
}

rm_line_ring_buffer::rm_line_ring_buffer(size_t ringbuf_size, size_t line_limit, size_t num_output_lines) :
  m_lines_buf(ringbuf_size),
  m_start_line(0),
  m_num_output_lines(num_output_lines),
  m_sel_line_begin(0),
  m_sel_line_end(0)
{
}

void rm_line_ring_buffer::set_selection(size_t beginline, size_t endline)
{
  m_sel_line_begin = rm_min(beginline, endline);
  m_sel_line_end = rm_max(endline, beginline);
}

void rm_line_ring_buffer::get_selection(size_t& beginline, size_t& endline)
{
  beginline = m_sel_line_begin;
  endline = m_sel_line_end;
}

bool rm_line_ring_buffer::get_selection_text_size(size_t& dstlen)
{
  assert(m_sel_line_begin < m_sel_line_end && "line positions is not reordered!");


  return false;
}

bool rm_line_ring_buffer::copy_selection(std::string& dst)
{
  assert(m_sel_line_begin < m_sel_line_end && "line positions is not reordered!");
  dst.clear();
  for (size_t i = m_sel_line_begin; i < m_sel_line_end; i++) {
    const rm_rb_line* pline = get_line(i);
    RM_HANDLE_EXCEPTIONS(false,
      dst.append(pline->get_string());
    );
  }
  return true;
}

bool rm_line_ring_buffer::clipboard_copy(irm_sysdf* psdf)
{
  std::string content;
  if (copy_selection(content)) {
    psdf->set_clipboard_data_ex((const uint8_t*)content.c_str(), content.length());
    return true;
  }
  return false;
}

void rm_line_ring_buffer::set_num_output_lines(size_t numlines)
{
  m_num_output_lines = numlines;
  if (m_num_output_lines >= get_num_lines())
    m_num_output_lines = get_num_lines();
}

const rm_line_ring_buffer::rm_rb_line* rm_line_ring_buffer::get_output_line(size_t idx)
{
  assert(idx < m_num_output_lines && "output line index out of bounds");
  size_t lineidx = m_start_line + idx;
  return &m_lines_buf[lineidx % m_num_output_lines];
}

bool rm_line_ring_buffer::append_text(std::string& content)
{
  //rm_rb_line* pline;
  //size_t      off = 0;
  //size_t      last_off = 0;
  //if (content.length()) {
  //  while (1) {
  //    off = content.find_first_of('\n', last_off);
  //    if(off != std::string::npos)

  //    size_t count = off - last_off;
  //    std::string tok = content.substr(last_off, count);
  //    last_off = off+1;
  //    pline = get_line_for_write();
  //    if (!pline)
  //      return false;

  //    printf("substr: %s\n", tok.c_str());
  //    pline->set_string(tok.c_str());
  //  }
  //}
  return true;
}

bool rm_line_ring_buffer::append_text(const char* pformat, ...)
{
  va_list     argptr;
  std::string content;
  content.resize(8096);
  va_start(argptr, pformat);
  format_string(content, pformat, argptr);
  va_end(argptr);
  return append_text(content);
}

void rm_line_ring_buffer::rm_rb_line::sym_widths_recompute(NVGcontext* pctx)
{
  //nvgTextBounds();
}

bool rm_line_ring_buffer::rm_rb_line::get_substring_from_selection(std::string& dst)
{
  assert(m_sel_begin < m_sel_end && "selection cursors is not reordered!");
  assert(m_sel_begin < m_line.size() && "m_sel_begin selection out of bounds");
  assert(m_sel_end < m_line.size() && "m_sel_end selection out of bounds");
  size_t begin = rm_min(m_sel_begin, m_sel_end);
  size_t end = rm_max(m_sel_end, m_sel_begin);
  size_t count = end - begin;
  RM_HANDLE_EXCEPTIONS(false,
    dst = m_line.substr(m_sel_begin, count);
  )
    return true;
}
