#include <algorithm>
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

void rm_surface::event_dispatcher(rm_widget* p_elem)
{
  RMGUI_UNUSED(p_elem);
}

void rm_surface::keybd_dispatcher(rm_widget* p_elem, int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state)
{
  p_elem->on_keybd(sc, vk, state);
  /* element has childs? */
  if (p_elem->get_elem_flags().has_childs() && p_elem->get_elem_flags().has_notify_childs()) {
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
  rmgui_vector2& cursor_pos)
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

  if (p_elem->get_elem_flags().has_childs() &&
    p_elem->get_elem_flags().has_notify_childs()) {
    for (size_t i = 0; i < p_elem->get_num_childs(); i++) {
      if (!mouse_dispatcher(p_elem->get_child(i), event, vk, state, cursor_pos)) {
        return false;
      }
    }
  }
  return true; //continue handling next
}

void rm_surface::build_draw_cache_recursive(rm_widget* p_elem)
{
  ///* is visible? */
  if (p_elem->get_elem_flags().has_visible()) {
    /* add element to draw path container */
    layer_draw_cache* player = get_layer_by_zindex(p_elem->get_zindex());
    if(player)
      player->add_widget(p_elem);

    /* element has childs? */
    if (p_elem->get_elem_flags().has_childs()) {
      /* recursive enum childs */
      for (size_t i = 0; i < p_elem->get_num_childs(); i++) {
        /* enter recursively */
        build_draw_cache_recursive(p_elem->get_child(i));
      }
    }
  }
}

void rm_surface::rebuild_draw_cache()
{
  build_draw_cache_recursive(this);
}

layer_draw_cache *rm_surface::get_layer_by_zindex(int zid)
{
  layer_draw_cache* pcache;
  auto it = std::find_if(m_layers.begin(), m_layers.end(),
    [zid](layer_draw_cache *player) {
      return player->get_zindex() == zid;
    }
  );

  if (it != m_layers.end())
    return *it;
  
  pcache = new (std::nothrow)layer_draw_cache(zid);
  if (!pcache)
    return nullptr;

  m_layers.push_back(pcache);
  std::sort(m_layers.begin(), m_layers.end(), 
    [](layer_draw_cache *pa, layer_draw_cache* pb) {
      return pa->get_zindex() < pb->get_zindex();
    }
  );
  return pcache;
}

void rm_surface::draw(float dt)
{
  m_delta_time = dt;
  nvgBeginFrame(m_pctx, m_relative.width, m_relative.height, 1.f);
  /* drawing layers */
  for (size_t i = 0; i < m_layers.size(); i++) {
    /* draw elements in layer */
    layer_draw_cache *pdraw_cache = m_layers[i];
    assert(pdraw_cache && "pdraw_cache was nullptr");
    for (size_t j = 0; j < pdraw_cache->size(); j++) {
      rm_widget* pwidget = pdraw_cache->get_widget(j);
      assert(pwidget && "pwidget was nullptr");
      rm_rect& outer_rect = pwidget->get_absolute();
      nvgSave(m_pctx);

      /* disabled scissoring? */
      if (!pwidget->get_elem_flags().is_set(EXGUI_FLAG_DISABLE_SCISSOR))
        nvgScissor(m_pctx, outer_rect.x, outer_rect.y, outer_rect.width, outer_rect.height);

      nvgTranslate(m_pctx, outer_rect.x, outer_rect.y);
      pwidget->on_draw(m_pctx);
      //nvgResetTransform(m_pctx);
      nvgResetScissor(m_pctx);
      nvgRestore(m_pctx);
    }
    pdraw_cache->clear();
  }
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
  rmgui_vector2 mouse_pos(x, y);
  mouse_dispatcher(this, event, vk, state, mouse_pos);
}

void rm_surface::resize(int width, int height)
{
  m_relative.width = m_absolute.width = width;
  m_relative.height = m_absolute.height = height;
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

rm_surface::rm_surface(NVGcontext* p_ctx, int width, int height, irmgui_sysdf* p_sysdf) : rm_widget(0, 0, width, height, nullptr, "ui_root_node")
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
    m_relative.x, m_relative.y, m_relative.width, m_relative.height,
    p_style->get_corner_radius(LEFT_TOP), p_style->get_corner_radius(RIGHT_TOP),
    p_style->get_corner_radius(RIGHT_BOTTOM), p_style->get_corner_radius(LEFT_BOTTOM));
  nvgFill(p_ctx);
}

bool rm_window::on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos)
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

bool rmgui_timer::has_elapsed(irmgui_sysdf* p_sysdf)
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