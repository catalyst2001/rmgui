#include <algorithm>
#include <cstdarg>
#include <utility>

#include "rmgui.h"

void rm_widget::move_childs_relative(rm_widget* pwidget, rm_vec2 deltapos)
{
	for (size_t i = 0; i < pwidget->get_num_childs(); i++) {
		rm_widget* pchild = pwidget->get_child(i);
		assert(pchild && "pchild was nullptr");
		pchild->m_pos_of_parent += deltapos;
		pchild->m_bbox.init(pchild->m_pos_of_parent, pchild->m_size);
		move_childs_relative(pchild, deltapos);
	}
}

void rm_widget::move_to(rm_widget* proot_widget, float xpos, float ypos)
{
	assert(proot_widget && "proot_widget was nullptr");
	rm_vec2 oldpos = proot_widget->m_pos_of_parent;
	//rm_vec2 delta = newpos - oldpos;
	proot_widget->m_pos_of_parent = /*newpos*/rm_vec2(xpos, ypos);
	proot_widget->m_bbox.init(proot_widget->m_pos_of_parent, proot_widget->m_size);
	//TODO: K.D. move_childs_relative is not needed because children are already moving relative to their parent's position! Right?
	//move_childs_relative(proot_widget, delta);
}

void rm_widget::resize_nolayout(float width, float height)
{
	rm_vec2 constrained = constrain_size(rm_vec2(width, height));
	m_size = constrained;
	m_bbox.init(m_pos_of_parent, m_size);
	m_content_area.init(0.f, 0.f, m_size.x, m_size.y);
}

void rm_widget::move_relative(rm_vec2& delta)
{
	m_pos_of_parent += delta;
	m_bbox.init(m_pos_of_parent, m_size);
}

bool rm_widget::perform_layout()
{
	/* layout attached? */
	if (m_playout) {
		/* try measure */
		if (m_playout->measure(this)) {
			/* perform layout */
			return m_playout->perform(this);
		}
	}
	/* layout not attached or measure or perform failed */
	return false;
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
	if (!m_elem_flags.has_childs())
		return false;

	/* have parent? */
	p_child->grab_globals_from(this);

	/* need events handling highest priority? */
	if (p_child->get_elem_flags().is_set(RM_FLAG_HIGHEST_PRIORITY)) {
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
		p_child->dispatch_event(PARENT_CHANGED_EVENT, this, nullptr);
		p_child->set_parent(nullptr);
	}
	return true;
}

rm_widget* rm_widget::find_child(const char* pclassname) const
{
	for (auto pchild : m_childs) {
		assert(pchild && "pchild was nullptr!");
		if (pchild->classname_is(pclassname)) {
			return pchild;
		}
	}
	return nullptr;
}

void rm_widget::set_parent(rm_widget* p_parent)
{
	m_pparent = p_parent;
	root_update();
}

void rm_widget::resize(float width, float height)
{
	resize_nolayout(width, height);
	perform_layout();
}

void rm_surface::keybd_dispatcher(rm_widget* p_elem, int sc, RM_KEY vk, RM_KEY_STATE state)
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
	RM_MOUSE_EVENT event,
	RM_KEY vk,
	RM_KEY_STATE state,
	rm_vec2& cursor_pos)
{
	bool b_call_next = true;
	bool b_cursor_inside = p_elem->get_bbox().inside(cursor_pos);
	bool b_global_receive_events = p_elem->get_elem_flags().is_set(RM_FLAG_GLOBAL);
	if (b_cursor_inside || b_global_receive_events) {
		b_call_next = p_elem->on_mouse(event, vk, state, cursor_pos, m_delta_cursor);
		if (event == RM_MOUSE_EVENT_CLICK && state == DOWN && p_elem != this) {
			if (!(b_global_receive_events && !b_cursor_inside)) {
				m_pfocus = p_elem;
				printf("updated focus to element %s\n", p_elem->get_classname());
			}
		}
	}

	p_elem->m_elem_flags.toggle_bits(RM_FLAG_HOVERED, b_cursor_inside);
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

	rm_vec2& abs_pos = pwidget->get_pos_of_parent();
	rm_vec2& size = pwidget->get_size();
	rm_rect& content = pwidget->get_content_area();

	m_pctx->save();

	/* disabled scissoring? */
	if (!pwidget->get_elem_flags().is_set(RM_FLAG_DISABLE_SCISSOR))
		m_pctx->scissor(abs_pos.x, abs_pos.y, size.x, size.y);

	m_pctx->setZIndex(pwidget->get_zindex());
	m_pctx->translate(abs_pos.x + content.x, abs_pos.y + content.y);
	pwidget->on_draw(m_pctx.get());

	/* element has childs? */
	if (pwidget->get_elem_flags().has_childs()) {
		/* recursive enum childs */
		for (size_t i = 0; i < pwidget->get_num_childs(); i++) {
			/* enter recursively */
			draw_recursive(pwidget->get_child(i), dt);
		}
	}
	//m_pctx->ResetTransform();
	m_pctx->resetScissor();
	m_pctx->restore();

#ifdef RMGUI_DEBUG_DRAW
	/* draw absolute position for debug */
	m_pctx->BeginPath();
	m_pctx->FillColor(NVGcolor::RGB(0, 0, 255));
	m_pctx->Circle(abs_pos.x, abs_pos.y, 2.f);
	m_pctx->Fill();
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
	m_pctx->beginFrame(m_size.x, m_size.y, m_device_pixel_ratio);
	draw_recursive(this, dt);
	m_pctx->endFrame();
}

void rm_surface::keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
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

void rm_surface::mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, int x, int y)
{
	rm_vec2 mouse_pos(x, y);
	m_delta_cursor = mouse_pos - m_last_cursor;
	mouse_dispatcher(this, event, vk, state, mouse_pos);
	m_last_cursor = mouse_pos;
}

rm_image rm_surface::load_image_from_memory(const void* psrc, size_t srclen, int flags)
{
	return m_pctx->createImageMem(flags, (uint8_t*)psrc, (int)srclen);
}

rm_image rm_surface::load_image(const char* pfilename, int flags)
{
	return m_pctx->createImage(pfilename, flags);
}

void rm_surface::free_image(rm_image& image)
{
	if (image.isValid()) {
		m_pctx->deleteImage(image);
		image.invalidate();
	}
}

rm_font rm_surface::load_font_from_memory(const void* psrc_ttf_mem, size_t srclen, const char* pfontname)
{
	rm_font font = find_font(pfontname);
	if (!font.isValid())
		return rm_font(m_pctx->createFontMem(pfontname, (uint8_t*)psrc_ttf_mem, static_cast<int>(srclen), 0));

	return font;
}

rm_font rm_surface::load_font(const char* pfilename, const char* pfontname)
{
	rm_font font = find_font(pfontname);
	if (!font.isValid())
		return rm_font(m_pctx->createFont(pfontname, pfilename));

	return font;
}

rm_font rm_surface::find_font(const char* pfontname)
{
	//TODO KD: refactor fontstash
	return rm_font(m_pctx->findFont(pfontname));
}

void rm_surface::free_font(rm_font& font)
{
	//NOTE: K.D. nvg not free fonts
}

void rm_surface::set_device_pixel_ratio(float ratio)
{
	m_device_pixel_ratio = ratio;
}

float rm_surface::get_device_pixel_ratio()
{
	return m_device_pixel_ratio;
}

void rm_surface::get_text_bounds(rm_bbox& dst,
	const char* ptext,
	rm_font hfont,
	rm_vec2 start)
{
	NVGcontext* pctx = get_context();
	assert(pctx && "pctx was nullptr!");
	pctx->save();
	pctx->setFontFaceId((int)hfont.getValue()); //FIXME: wait fontstash refactoring!
	pctx->textBounds(start.x, start.y, ptext, nullptr, dst.array);
	pctx->restore();
}

float rm_surface::get_text_width(const char* ptext, rm_font hfont, rm_vec2 start)
{
	rm_bbox bbox;
	get_text_bounds(bbox, ptext, hfont, start);
	return bbox.get_width();
}

float rm_surface::get_text_height(const char* ptext, rm_font hfont, rm_vec2 start)
{
	rm_bbox bbox;
	get_text_bounds(bbox, ptext, hfont, start);
	return bbox.get_height();
}

rm_surface::rm_surface(std::unique_ptr<NVGcontext> pctx, int width, int height, irm_sysdf* p_sysdf, void* psyswindow) : rm_widget(0, 0, width, height, nullptr, "ui_root_node")
{
	/* root surface is freely resizable (viewport) */
	set_min_size(rm_vec2(0.f, 0.f));
	set_max_size(rm_vec2(0.f, 0.f));

	m_psyswindow = psyswindow;
	m_psysdf = p_sysdf;
	set_root(this);
	m_pctx = std::move(pctx);
	m_pfocus = nullptr;
	m_delta_time = 0.f;
	m_device_pixel_ratio = 1.f;
	rm_font font = load_font_from_memory(fontawesomewebfont, FONT_SIZE, "fontawesome");
	assert(font.isValid() && "font is invalid");
}

rm_surface::~rm_surface()
{
}

rm_window::WSC rm_window::get_active_size_corner()
{
	assert(!(m_active_resizes & (WCF_LRESIZE | WCF_RRESIZE)) && "L&R impossible sizeboxes activity!");
	assert(!(m_active_resizes & (WCF_TRESIZE | WCF_BRESIZE)) && "T&B impossible sizeboxes activity!");
	/* handle corners */
	if (m_active_resizes & (WCF_LRESIZE | WCF_TRESIZE))
		return SC_LEFT_TOP;
	if (m_active_resizes & (WCF_TRESIZE | WCF_RRESIZE))
		return SC_RIGHT_TOP;
	if (m_active_resizes & (WCF_RRESIZE | WCF_BRESIZE))
		return SC_RIGHT_BOTTOM;
	if (m_active_resizes & (WCF_LRESIZE | WCF_BRESIZE))
		return SC_LEFT_BOTTOM;

	return SC_NO_CORNER;
}

void rm_window::handle_sizeboxes(const rm_vec2& parent_local)
{
	rm_vec2 min, max;
	m_active_resizes = WCF_NONE;
	rm_bbox curr_bbox;
	rm_bbox ext = get_bbox();
	printf("handle_sizeboxes: AABB (%f %f) (%f %f) mouse(%f %f)\n",
		ext.min.x, ext.min.y,
		ext.max.x, ext.max.y,
		parent_local.x, parent_local.y
	);

	// Use window-local coordinates for hit-testing to avoid offsets from parent's transforms
	rm_vec2 local(parent_local.x - m_pos_of_parent.x, parent_local.y - m_pos_of_parent.y); // cursor relative to window top-left
	float width = m_size.x;
	float height = m_size.y;

	// left: tall rect around left edge
	if (m_flags & WCF_LRESIZE) {
		curr_bbox.init(rm_vec2(-m_size_drag_width, -m_size_drag_width),
			rm_vec2(m_size_drag_width * 2.f, height + m_size_drag_width * 2.f));
		if (curr_bbox.inside(local))
			m_active_resizes |= WCF_LRESIZE;
	}

	// right: tall rect around right edge
	if (m_flags & WCF_RRESIZE) {
		curr_bbox.init(rm_vec2(width - m_size_drag_width, -m_size_drag_width),
			rm_vec2(m_size_drag_width * 2.f, height + m_size_drag_width * 2.f));
		if (curr_bbox.inside(local))
			m_active_resizes |= WCF_RRESIZE;
	}

	// top: wide rect around top edge
	if (m_flags & WCF_TRESIZE) {
		curr_bbox.init(rm_vec2(-m_size_drag_width, -m_size_drag_width),
			rm_vec2(width + m_size_drag_width * 2.f, m_size_drag_width * 2.f));
		if (curr_bbox.inside(local))
			m_active_resizes |= WCF_TRESIZE;
	}

	// bottom: wide rect around bottom edge
	if (m_flags & WCF_BRESIZE) {
		curr_bbox.init(rm_vec2(-m_size_drag_width, height - m_size_drag_width),
			rm_vec2(width + m_size_drag_width * 2.f, m_size_drag_width * 2.f));
		if (curr_bbox.inside(local))
			m_active_resizes |= WCF_BRESIZE;
	}

	/* save all parameters if window resizing */
	if (m_active_resizes != WCF_NONE) {
		m_state_flags |= WSF_RESIZE;
		m_resize_start_pos = m_pos_of_parent;
		m_resize_start_size = m_size;
		m_resize_start_mouse = parent_local;
	}
}

void rm_window::on_draw(NVGcontext* pctx)
{
	rm_window_style* p_style = get_style();
	assert(p_style && "rmgui_window::on_draw(): window style is not set! Use rmgui_window::set_style(rmgui_wi1ndow_style *)");
	int b_is_active = (int)((get_elem_flags().is_focused() || get_elem_flags().is_hovered() || (m_state_flags & WSF_DRAG)));

	rm_vec2 pos(0.f, 0.f);
	NVGcolor shadow_color = NVGcolor::RGBA(0, 0, 0, 63);
	rm_utl::draw_shadow(pctx, pos, m_size, rm_vec2(0.f, 1.f), 5.0f, shadow_color, 8.f, get_style()->get_corner_radius(LEFT_TOP));

	/* draw window background */
	pctx->beginPath();
	pctx->fillColor(p_style->get_background_color(b_is_active));
	pctx->roundedRectVarying(
		0.f, 0.f, m_size.x, m_size.y,
		p_style->get_corner_radius(LEFT_TOP), p_style->get_corner_radius(RIGHT_TOP),
		p_style->get_corner_radius(RIGHT_BOTTOM), p_style->get_corner_radius(LEFT_BOTTOM));
	pctx->fill();
}

bool rm_window::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
	rm_vec2& cursor_pos, rm_vec2 /*delta*/)
{
	constexpr float drag_height = 30.f;
	rm_vec2 zero(0.f, 0.f);
	// cursor_pos passed to this handler is already in parent's local coordinates
	rm_vec2 parent_local = cursor_pos;
	if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
		handle_sizeboxes(parent_local);
		if (m_state_flags & WSF_RESIZE)
			return true;

		rm_bbox header(zero, rm_vec2(m_size.x, drag_height));
		if (header.inside(cursor_to_local(cursor_pos))) {
			m_state_flags |= WSF_DRAG;
			m_drag_start_pos = m_pos_of_parent;
			// store mouse pos relative to parent at drag start (cursor_pos already parent-local)
			m_drag_start_mouse = parent_local;
			return true;
		}
	}

	if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
		m_state_flags &= ~WSF_DRAG;
		m_state_flags &= ~WSF_RESIZE;
		m_active_resizes = WCF_NONE;
		return true;
	}

	if (event == RM_MOUSE_EVENT_MOVE) {
		const rm_vec2& ps = m_pparent->get_size();
		if (m_state_flags & WSF_RESIZE) {
			const rm_vec2& wmin = get_min_size();
			const rm_vec2& wmax = get_max_size();
			float min_w = (wmin.x > 0.f) ? wmin.x : 50.0f;
			float min_h = (wmin.y > 0.f) ? wmin.y : 50.0f;
			float max_w = (wmax.x > 0.f) ? wmax.x : ps.x;
			float max_h = (wmax.y > 0.f) ? wmax.y : ps.y;

			rm_vec2 delta = parent_local - m_resize_start_mouse;
			if (m_active_resizes & WCF_LRESIZE) {
				float newW = std::max(m_resize_start_size.x - delta.x, min_w);
				newW = std::clamp(newW, min_w, std::min(max_w, ps.x - m_resize_start_pos.x));
				float newX = m_resize_start_pos.x + (m_resize_start_size.x - newW);
				newX = std::clamp(newX, 0.0f, ps.x - newW);
				m_pos_of_parent.x = newX;
				m_size.x = newW;
			}

			if (m_active_resizes & WCF_RRESIZE) {
				float newW = std::max(m_resize_start_size.x + delta.x, min_w);
				newW = std::clamp(newW, min_w, std::min(max_w, ps.x - m_pos_of_parent.x));
				m_size.x = newW;
			}

			if (m_active_resizes & WCF_TRESIZE) {
				float newH = std::max(m_resize_start_size.y - delta.y, min_h);
				newH = std::clamp(newH, min_h, std::min(max_h, ps.y - m_resize_start_pos.y));
				float newY = m_resize_start_pos.y + (m_resize_start_size.y - newH);
				newY = std::clamp(newY, 0.0f, ps.y - newH);
				m_pos_of_parent.y = newY;
				m_size.y = newH;
			}

			if (m_active_resizes & WCF_BRESIZE) {
				float newH = std::max(m_resize_start_size.y + delta.y, min_h);
				newH = std::clamp(newH, min_h, std::min(max_h, ps.y - m_pos_of_parent.y));
				m_size.y = newH;
			}

			m_bbox.init(m_pos_of_parent, m_size);
			if (m_active_resizes)
				perform_layout();

			return true;
		}

		if (m_state_flags & WSF_DRAG) {
			rm_vec2 delta = parent_local - m_drag_start_mouse;
			rm_vec2 np;
			np.x = m_drag_start_pos.x + delta.x;
			np.y = m_drag_start_pos.y + delta.y;
			np.x = std::clamp(np.x, 0.0f, ps.x - m_size.x);
			np.y = std::clamp(np.y, 0.0f, ps.y - m_size.y);
			m_pos_of_parent = np;
			m_bbox.init(m_pos_of_parent, m_size);
			return true;
		}
	}
	return true;
}

rm_window::rm_window(rm_widget* p_parent, int x, int y, int width, int height, uint32_t flags) :
	rm_widget(x, y, width, height, p_parent, "ui_window", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL), m_flags(flags), m_state_flags(WSF_NONE), m_active_resizes(WSF_NONE), m_pstyle(nullptr), m_size_drag_width(5.f)
{
	/* window constraints are computed by layout from children, not fixed to initial size */
	set_min_size(rm_vec2(0.f, 0.f));
	set_max_size(rm_vec2(0.f, 0.f));
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
	psysdf->set_clipboard_data_ex((const uint8_t*)to_clipboard.data(), to_clipboard.size());
	text.erase(a, b - a);
	cursor = a;
	clear_redo();
	clear_selection();
}

void rmgui_textbuffer::copy_all(irm_sysdf* psysdf)
{
	psysdf->set_clipboard_data_ex((const uint8_t*)text.c_str(), text.size());
}

void rmgui_textbuffer::paste(irm_sysdf* psysdf)
{
	size_t clipboard_data_size;
	RM_CB_DATA_TYPE clipboard_dtype;
	const char* pclipboard_text = nullptr;
	if (has_selection())
		delete_selection();

	pclipboard_text = (const char*)psysdf->get_clipboard_data_ex(clipboard_dtype, clipboard_data_size);
	if (!pclipboard_text)
		return;

	/* skip binary data*/
	if (!clipboard_data_size || clipboard_dtype != RM_CLIPBOARD_DATA_TYPE_TEXT)
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

bool rm_line_ring_buffer::append_text(const std::string& content)
{
	rm_rb_line* pline;
	size_t      off = 0;
	size_t      last_off = 0;

	if (content.length()) {
		std::string token;
		rm_string_tokenizer tokenizer(content, '\n');
		while (tokenizer.get_token(token)) {
			pline = get_line_for_write();
			if (!pline)
				return false;

			//printf("substr: %s\n", token.c_str());
			pline->set_string(token.c_str());
		}
		return true;
	}
	return false;
}

//bool rm_line_ring_buffer::append_text(const char* pformat, ...)
//{
//  va_list     argptr;
//  std::string content;
//  content.resize(8096);
//  va_start(argptr, pformat);
//  format_string(content, pformat, argptr);
//  va_end(argptr);
//  return append_text(content);
//}

void rm_line_ring_buffer::rm_rb_line::sym_widths_recompute(NVGcontext* pctx)
{
	//);
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

rm_string_tokenizer::rm_string_tokenizer(const std::string& target, char delim) :
	m_delim(delim), m_next_token_available(true), m_last_pos(0), m_cur_pos(0), m_str(target) {
}

bool rm_string_tokenizer::get_token(std::string& dst)
{
	if (!m_next_token_available)
		return false;

	m_cur_pos = m_str.find_first_of(m_delim, m_last_pos);
	if (m_cur_pos == std::string::npos) {
		m_next_token_available = false;
		/* delims not found in string */
		if (!m_last_pos) {
			dst = m_str;
			return true; //source string
		}
		dst = m_str.substr(m_last_pos);
		return true;
	}
	dst = m_str.substr(m_last_pos, m_cur_pos - m_last_pos);
	m_last_pos = m_cur_pos + 1;
	return true;
}

void rm_utl::draw_frame(NVGcontext* pctx, rm_vec2& pos, rm_vec2& size,
	uint32_t mode,
	const NVGcolor& background,
	const NVGcolor& stroke, float stroke_width,
	const NVGcolor colors[],
	const rm_corners_style* pcstyle,
	float shadow_offset)
{
	float sign = (mode == RM_BFRM_MODE_RAISED) ? 1.f : -1.f;
	const NVGcolor& scolor = colors[RM_BFRM_MODE_SUNKEN];
	const NVGcolor& sdwcolor = colors[RM_BFRM_MODE_RAISED];
	/* paint background */
	pctx->beginPath();
	pctx->save(); //TODO: K.D. save state for store old scissor
	pctx->fillColor(background);
	pctx->strokeColor(stroke);
	pctx->roundedRectVarying(pos.x, pos.y, size.x, size.y,
		pcstyle->get_top_left(), pcstyle->get_top_right(),
		pcstyle->get_bottom_right(), pcstyle->get_bottom_left()
	);
	pctx->fill();
	pctx->stroke();
	//pctx->Scissor( pos.x, pos.y, size.x - stroke_width, size.y- stroke_width);

	pctx->fillColor(background);
	pctx->strokeColor(stroke);
	pctx->roundedRectVarying(pos.x + sign, pos.y + sign, size.x, size.y,
		pcstyle->get_top_left(), pcstyle->get_top_right(),
		pcstyle->get_bottom_right(), pcstyle->get_bottom_left()
	);
	pctx->stroke();

	pctx->restore();
}

void rm_utl::draw_shadow(NVGcontext* pctx, rm_vec2 pos, rm_vec2& size, rm_vec2 dir, float offset_scale,
	const NVGcolor& shadow_color, float shadow_size, float corner_radius, bool draw_shadow_center) {
	float len = dir.length();
	rm_vec2 nd = (len > 0.f) ? rm_vec2{ dir.x / len, dir.y / len } : rm_vec2{ 0, 1 };

	float offx = nd.x * offset_scale;
	float offy = nd.y * offset_scale;

	NVGpaint paint = NVGpaint::boxGradient(
		pos.x + offx,
		pos.y + offy,
		size.x,
		size.y,
		corner_radius * 2.0f,
		shadow_size * 2.0f,
		shadow_color,
		rm_utl::get_transparent()
	);

	pctx->save();
	pctx->StrokeWidth(0.f);
	pctx->resetScissor();

	pctx->beginPath();
	pctx->rect(
		pos.x - shadow_size + offx,
		pos.y - shadow_size + offy,
		size.x + 2 * shadow_size,
		size.y + 2 * shadow_size);

	pctx->roundedRect(pos.x + 1.0f, pos.y + 1.0f, size.x - 1.0f * 2.f, size.y - 1.0f * 2.f, corner_radius);
	if (!draw_shadow_center)
		pctx->pathWinding(NVG_HOLE);

	pctx->fillPaint(paint);
	pctx->fill();
	pctx->restore();
}

void rm_utl::draw_edge(NVGcontext* pctx, rm_vec2 pos, rm_vec2& size,
	const rm_corners_style* pcstyle, const rm_color& suncolor, const rm_color& shadowcolor)
{
	/* light */
	pctx->beginPath();
	pctx->strokeColor(suncolor);
	pctx->roundedRectVarying(
		pos.x, pos.y + 1.f, size.x, size.y,
		pcstyle->get_top_left(),
		pcstyle->get_top_right(),
		pcstyle->get_bottom_right(),
		pcstyle->get_bottom_left());
	pctx->stroke();

	/* light */
	pctx->beginPath();
	pctx->strokeColor(shadowcolor);
	pctx->roundedRectVarying(
		pos.x, pos.y, size.x, size.y,
		pcstyle->get_top_left(),
		pcstyle->get_top_right(),
		pcstyle->get_bottom_right(),
		pcstyle->get_bottom_left());
	pctx->stroke();
}

rm_flexbox_layout::rm_flexbox_layout(rm_flex_direction dir,
	rm_flex_wrap wrap,
	rm_flex_justify justify,
	rm_flex_align align_items,
	rm_flex_align align_content,
	rm_flex_fill fillx,
	rm_flex_fill filly,
	rm_rect padding,
	rm_rect margin,
	float gap_main,
	float gap_cross,
	float min_size,
	float max_size) :
	m_dir(dir),
	m_wrap(wrap),
	m_justify(justify),
	m_align_items(align_items),
	m_align_content(align_content),
	m_fill_x(fillx),
	m_fill_y(filly),
	m_padding(padding),
	m_margin(margin),
	m_gap_main(gap_main),
	m_gap_cross(gap_cross),
	m_min_size(min_size),
	m_max_size(max_size)
{
}

bool rm_flexbox_layout::measure(rm_widget* pwidget)
{
	if (!pwidget)
		return false;

	const size_t child_count = pwidget->get_num_childs();
	if (child_count == 0)
		return true;

	bool is_row = (m_dir == rm_flex_direction::Row || m_dir == rm_flex_direction::RowReverse);

	float total_min_main = 0.f;
	float total_min_cross = 0.f;
	float total_max_main = 0.f;
	float total_max_cross = 0.f;
	bool all_have_max = true;
	size_t visible_count = 0;

	for (size_t i = 0; i < child_count; ++i) {
		rm_widget* child = pwidget->get_child(i);
		if (!child->is_visible())
			continue;

		const rm_vec2& cmin = child->get_min_size();
		const rm_vec2& cmax = child->get_max_size();

		float child_min_main  = is_row ? cmin.x : cmin.y;
		float child_min_cross = is_row ? cmin.y : cmin.x;
		float child_max_main  = is_row ? cmax.x : cmax.y;
		float child_max_cross = is_row ? cmax.y : cmax.x;

		/* main axis: sum of minimums */
		total_min_main += child_min_main;
		/* cross axis: max of minimums */
		total_min_cross = std::max(total_min_cross, child_min_cross);

		/* max: sum along main, max along cross */
		if (child_max_main > 0.f)
			total_max_main += child_max_main;
		else
			all_have_max = false;

		if (child_max_cross > 0.f)
			total_max_cross = std::max(total_max_cross, child_max_cross);

		visible_count++;
	}

	/* add gaps between children */
	if (visible_count > 1) {
		float gaps = (float)(visible_count - 1) * m_gap_main;
		total_min_main += gaps;
		if (all_have_max)
			total_max_main += gaps;
	}

	/* add padding */
	float pad_main  = is_row ? (m_padding.left + m_padding.right)  : (m_padding.top + m_padding.bottom);
	float pad_cross = is_row ? (m_padding.top + m_padding.bottom) : (m_padding.left + m_padding.right);

	total_min_main += pad_main;
	total_min_cross += pad_cross;
	if (all_have_max) {
		total_max_main += pad_main;
		total_max_cross += pad_cross;
	}

	/* set parent's min_size from children */
	rm_vec2 computed_min;
	if (is_row)
		computed_min.init(total_min_main, total_min_cross);
	else
		computed_min.init(total_min_cross, total_min_main);
	pwidget->set_min_size(computed_min);

	/* set parent's max_size from children (only if all children are bounded) */
	if (all_have_max) {
		rm_vec2 computed_max;
		if (is_row)
			computed_max.init(total_max_main, total_max_cross);
		else
			computed_max.init(total_max_cross, total_max_main);
		pwidget->set_max_size(computed_max);
	} else {
		pwidget->set_max_size(rm_vec2(0.f, 0.f));
	}

	return true;
}

bool rm_flexbox_layout::perform(rm_widget* pwidget)
{
	if (!pwidget)
		return false;

	const size_t child_count = pwidget->get_num_childs();
	if (child_count == 0)
		return true;

	// 1) Compute parent's content rectangle (subtract padding)
	rm_vec2 parent_size = pwidget->get_size();
	const float pad_left = m_padding.left;
	const float pad_right = m_padding.right;
	const float pad_top = m_padding.top;
	const float pad_bottom = m_padding.bottom;

	const float content_x = pad_left;
	const float content_y = pad_top;
	const float content_width = std::max(0.0f, parent_size.x - pad_left - pad_right);
	const float content_height = std::max(0.0f, parent_size.y - pad_top - pad_bottom);

	bool is_row = (m_dir == rm_flex_direction::Row || m_dir == rm_flex_direction::RowReverse);
	bool reverse = (m_dir == rm_flex_direction::RowReverse || m_dir == rm_flex_direction::ColumnReverse);
	bool do_wrap = (m_wrap != rm_flex_wrap::NoWrap);

	// helper: re-read actual widget size after resize (constraints may have clamped it)
	auto sync_size = [&](rm_widget* child, float& main_sz, float& cross_sz) {
		rm_vec2 actual = child->get_size();
		main_sz = is_row ? actual.x : actual.y;
		cross_sz = is_row ? actual.y : actual.x;
	};

	// 2) First pass: collect each child's size; apply per-child Clamp only if the child alone exceeds content area
	struct ChildInfo { rm_widget* w; float main; float cross; };
	std::vector<ChildInfo> infos;
	infos.reserve(child_count);

	for (size_t i = 0; i < child_count; ++i) {
		rm_widget* child = pwidget->get_child(i);
		if (!child->is_visible())
			continue;

		rm_vec2 sz = child->get_size();
		float child_main = is_row ? sz.x : sz.y;
		float child_cross = is_row ? sz.y : sz.x;

		// 2.a) If fill==Clamp on main axis, clamp child's main size to content
		if (is_row && m_fill_x == rm_flex_fill::Clamp) {
			if (child_main > content_width) {
				child->resize(content_width, sz.y);
				sync_size(child, child_main, child_cross);
			}
		}
		else if (!is_row && m_fill_y == rm_flex_fill::Clamp) {
			if (child_main > content_height) {
				child->resize(sz.x, content_height);
				sync_size(child, child_main, child_cross);
			}
		}

		// 2.b) If fill==Clamp on cross axis, clamp child's cross size to content
		sz = child->get_size();
		if (is_row && m_fill_y == rm_flex_fill::Clamp) {
			if (child_cross > content_height) {
				child->resize(sz.x, content_height);
				sync_size(child, child_main, child_cross);
			}
		}
		else if (!is_row && m_fill_x == rm_flex_fill::Clamp) {
			if (child_cross > content_width) {
				child->resize(content_width, sz.y);
				sync_size(child, child_main, child_cross);
			}
		}

		infos.push_back({ child, child_main, child_cross });
	}

	// 3) Position children, handling Clamp-on-overflow or wrap
	float offset_main = 0.0f;
	float offset_cross = 0.0f;
	float line_cross_size = 0.0f;
	bool wrap_reverse = (m_wrap == rm_flex_wrap::WrapReverse);

	const float max_main = is_row ? content_width : content_height;
	const float max_cross = is_row ? content_height : content_width;

	auto in_bounds = [&](float pos, float size, float limit) {
		return (pos + size) <= limit;
		};

	auto advance_line = [&]() {
		offset_main = 0.0f;
		offset_cross += line_cross_size + m_gap_cross;
		line_cross_size = 0.0f;
		};

	size_t start = 0, end = infos.size();
	int step = 1;
	if (reverse) {
		start = infos.size() - 1;
		end = SIZE_MAX; // loop until idx becomes SIZE_MAX (underflow)
		step = -1;
	}

	for (size_t idx = start; idx != end; idx = size_t((int)idx + step)) {
		ChildInfo& info = infos[idx];
		rm_widget* child = info.w;

		float child_main = info.main;
		float child_cross = info.cross;

		// 3.a) On main-axis overflow: if Clamp mode, shrink to remaining space; otherwise wrap if enabled
		bool clamp_main = (is_row ? (m_fill_x == rm_flex_fill::Clamp) : (m_fill_y == rm_flex_fill::Clamp));

		if (!in_bounds(offset_main, child_main, max_main)) {
			if (clamp_main) {
				// Shrink child_main to fit exactly remaining space
				float remain = max_main - offset_main;
				if (remain < 0.0f) remain = 0.0f;
				if (is_row) {
					child->resize(remain, child->get_size().y);
				}
				else {
					child->resize(child->get_size().x, remain);
				}
				sync_size(child, child_main, child_cross);
				info.main = child_main;
				// Recompute cross if Stretch and cross Clamp not applied
				if (!is_row && m_fill_x != rm_flex_fill::Clamp && m_align_items == rm_flex_align::Stretch) {
					// For column, stretching cross means width = content_width
					child->resize(content_width, child_main);
					sync_size(child, child_main, child_cross);
					info.main = child_main;
					info.cross = child_cross;
				}
			}
			else if (do_wrap) {
				// Wrap to next line
				advance_line();
			}
			// else: no wrap, no clamp → child overflows freely
		}

		// 3.b) Compute cross-axis overflow and possible Clamp on cross axis for this child
		bool clamp_cross = (is_row ? (m_fill_y == rm_flex_fill::Clamp) : (m_fill_x == rm_flex_fill::Clamp));
		if (!in_bounds(offset_cross + 0.0f, child_cross, max_cross) && clamp_cross) {
			// Squeeze child_cross to remaining cross space in the first line if offset_cross is zero,
			// or to max_cross if offset_cross > 0.
			float limit = (offset_cross == 0.0f) ? max_cross : (max_cross - offset_cross);
			if (limit < 0.0f) limit = 0.0f;
			if (is_row) {
				child->resize(child_main, limit);
			}
			else {
				child->resize(limit, child_main);
			}
			sync_size(child, child_main, child_cross);
			info.main = child_main;
			info.cross = child_cross;
		}

		// 3.c) Compute cross offset inside its “line” based on align_items
		float cross_free = ((is_row ? content_height : content_width) - child_cross);
		if (cross_free < 0.0f) cross_free = 0.0f;
		float offset_cross_child = 0.0f;
		switch (m_align_items) {
		case rm_flex_align::Auto:
		case rm_flex_align::FlexStart:
			offset_cross_child = 0.0f;
			break;
		case rm_flex_align::Center:
			offset_cross_child = cross_free * 0.5f;
			break;
		case rm_flex_align::FlexEnd:
			offset_cross_child = cross_free;
			break;
		case rm_flex_align::Stretch:
			if (is_row && m_fill_y != rm_flex_fill::Clamp) {
				child->resize(child_main, content_height);
				sync_size(child, child_main, child_cross);
				info.main = child_main;
				info.cross = child_cross;
			}
			else if (!is_row && m_fill_x != rm_flex_fill::Clamp) {
				child->resize(content_width, child_main);
				sync_size(child, child_main, child_cross);
				info.main = child_main;
				info.cross = child_cross;
			}
			offset_cross_child = 0.0f;
			break;
		default:
			offset_cross_child = 0.0f;
			break;
		}

		// 3.d) Determine final position for this child
		float main_pos = offset_main;
		float cross_pos = offset_cross + offset_cross_child;

		rm_vec2 child_pos;
		if (is_row) {
			child_pos.init(content_x + main_pos, content_y + cross_pos);
		}
		else {
			child_pos.init(content_x + cross_pos, content_y + main_pos);
		}
		child->move(child_pos);

		// 3.e) Update line_cross_size to the max cross dimension on this line
		line_cross_size = std::max(line_cross_size, child_cross);

		// 3.f) Advance offset_main for next child
		offset_main += child_main + m_gap_main;
	}

	// 4) If WrapReverse is set, reflect lines in the cross axis
	if (do_wrap && wrap_reverse) {
		// Total cross used = offset_cross (all previous lines) + line_cross_size (last line)
		float total_cross_used = offset_cross + line_cross_size;
		for (auto& info : infos) {
			rm_widget* child = info.w;
			rm_vec2 pos = child->get_pos_of_parent();
			float cx = is_row ? (pos.y - content_y) : (pos.x - content_x);
			float dimension = is_row ? info.cross : info.cross;
			float reflected = ((is_row ? content_height : content_width) - (cx + dimension));
			if (is_row) {
				child->move({ pos.x, content_y + reflected });
			}
			else {
				child->move({ content_x + reflected, pos.y });
			}
		}
	}

	return true;
}

bool rm_flexbox_layout::reset(rm_widget* pwidget)
{
	return true;
}
