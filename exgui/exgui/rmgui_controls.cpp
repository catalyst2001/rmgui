#include "rmgui_controls.h"
#include "rmgui_default_painter.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <cctype>
#include <cmath>

#include "stb_image.h"

RmWindowGeometry rm_window::current_geometry() const noexcept
{
	return { m_pos_of_parent.x, m_pos_of_parent.y, m_size.x, m_size.y };
}

void rm_window::apply_geometry(const RmWindowGeometry& geometry)
{
	m_pos_of_parent.init(geometry.x, geometry.y);
	m_size.init(geometry.width, geometry.height);
	m_bbox.init(m_pos_of_parent, m_size);
	m_content_area.width = m_size.x;
	m_content_area.height = m_size.y;
	perform_layout();
}

void rm_window::on_draw(NVGcontext* pctx)
{
	RmDefaultControlPainter::draw_window(*pctx,
		{ m_size.x, m_size.y, is_enabled(), m_elem_flags.is_hovered(),
		  m_elem_flags.is_focused(), m_behaviour.is_dragging(),
		  m_behaviour.is_resizing() }, m_theme->window);
	rm_widget::on_draw(pctx);
}

void rm_window::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmWindowStyle& style = m_theme->window;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

bool rm_window::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
		const rm_vec2 local = cursor_to_local(cursor_pos);
		const uint32_t edges = rm_window_resize_edges(local.x, local.y,
			m_size.x, m_size.y, m_theme->window.resize_grip_extent, m_flags);
		const RmBehaviourUpdate update = edges != WCF_NONE
			? m_behaviour.begin_resize(edges, cursor_pos.x, cursor_pos.y,
				current_geometry())
			: (local.y >= 0.0f && local.y <= m_theme->window.titlebar_height
				? m_behaviour.begin_drag(cursor_pos.x, cursor_pos.y,
					current_geometry())
				: RmBehaviourUpdate{});
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}

	if (event == RM_MOUSE_EVENT_MOVE && m_behaviour.is_interacting() && m_pparent) {
		const rm_vec2& parent_size = m_pparent->get_size();
		const rm_vec2& minimum = get_min_size();
		const rm_vec2& maximum = get_max_size();
		const RmBehaviourUpdate update = m_behaviour.pointer_move(
			cursor_pos.x, cursor_pos.y, parent_size.x, parent_size.y,
			minimum.x > 0.0f ? minimum.x : 50.0f,
			minimum.y > 0.0f ? minimum.y : 50.0f,
			maximum.x, maximum.y);
		if (update.state_changed)
			apply_geometry(m_behaviour.geometry());
		return !update.handled;
	}

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.end_interaction();
		return !update.handled;
	}
	return true;
}

rm_window::rm_window(rm_widget* p_parent, int x, int y, int width, int height,
	uint32_t flags, RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "ui_window",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL),
	m_flags(flags & WCF_RESIZABLE),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_min_size(rm_vec2(0.0f, 0.0f));
	set_max_size(rm_vec2(0.0f, 0.0f));
}

/**
* drawImage
*
* sx, sy, sw, sh - sprite location on texture
* x, y, w, h - position and size of the sprite rectangle on screen
*
* source 'https://github.com/memononen/nanovg/issues/348'
*/
void drawSprite(NVGcontext* vg, NVGhandle image, float alpha,
	float sx, float sy, float sw, float sh, // sprite location on texture
	float x, float y, float w, float h, // position and size of the sprite rectangle on screen
	float lt, float rt, float lb, float rb)
{
	float ax, ay;
	int iw, ih;
	NVGpaint img;

	vg->getImageSize(image, &iw, &ih);

	// Aspect ration of pixel in x an y dimensions. This allows us to scale
	// the sprite to fill the whole rectangle.
	ax = w / sw;
	ay = h / sh;

	img = NVGpaint::imagePattern(x - sx * ax, y - sy * ay, (float)iw * ax, (float)ih * ay, 0, image, alpha);
	vg->beginPath();
	vg->roundedRectVarying(x, y, w, h, lt, rt, rb, lb);
	vg->fillPaint(img);
	vg->fill();
}

rmgui_image::rmgui_image() : imageId(), width(0), height(0), channels(0) {}
rmgui_image::~rmgui_image() {}

bool rmgui_image::load(const std::string& filename, NVGcontext* ctx) {
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
	if (!data) {
		std::cerr << "Failed to load image: " << filename << std::endl;
		return false;
	}
	int flags = NVG_IMAGE_NEAREST;
	imageId = ctx->createImageRGBA(width, height, flags, data);
	stbi_image_free(data);
	if (!imageId.isValid()) {
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
		if (surface && m_image->imageId.isValid()) {
			surface->get_context()->deleteImage(m_image->imageId);
		}
		delete m_image;
	}
}

void rm_image_button::on_draw(NVGcontext* pctx) {
	//m_bbox.from_rect(m_absolute); //NOTE: K.D. commented this
	//pctx->BeginPath();
	//pctx->Rect( 0.f, 0.f, m_relative.width, m_relative.height);
	//pctx->FillColor( NVGcolor::RGBA(200, 200, 200, 255));
	//pctx->Fill();

	if (m_image && m_image->imageId.isValid()) {
		NVGpaint imgPaint = NVGpaint::imagePattern(
			0.f, 0.f,
			m_size.x, m_size.y,
			0.0f, m_image->imageId, 1.0f);
		pctx->beginPath();
		pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, 4.0f);
		pctx->fillPaint(imgPaint);
		pctx->fill();
	}
	rm_widget::on_draw(pctx);
}

bool rm_image_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	if (event == RM_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
		std::cout << "Image Button clicked!" << std::endl;
		return false;
	}
	return true;
}

rm_button::rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text,
	RmThemeRef theme, RmButtonVariant variant)
	: rm_widget(x, y, width, height, p_parent, "ui_button"), m_text(text),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()), m_variant(variant)
{
}

rm_button::~rm_button() {}

void rm_button::on_draw(NVGcontext* pctx) {
	const RmButtonVisual visual{
		m_size.x,
		m_size.y,
		get_font(),
		m_text.c_str(),
		is_enabled(),
		m_elem_flags.is_hovered(),
		m_behaviour.is_pressed()
	};
	RmDefaultControlPainter::draw_button(*pctx, visual, m_theme->buttons.resolve(m_variant));
	rm_widget::on_draw(pctx);
}

void rm_button::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmButtonStyle& style = m_theme->buttons.resolve(m_variant);
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

bool rm_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	RM_UNUSED(delta);
	const bool inside = m_bbox.inside(cursor_pos);

	if (event == RM_MOUSE_EVENT_MOVE) {
		const auto update = m_behaviour.pointer_move(inside);
		return !update.handled;
	}

	if (event != RM_MOUSE_EVENT_CLICK || vk != RM_KEY_LMOUSE)
		return true;

	if (state == DOWN) {
		const auto update = m_behaviour.pointer_down(inside);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}

	if (state == UP) {
		const auto update = m_behaviour.pointer_up(inside);
		if (update.activated)
			std::cout << "Button \"" << m_text << "\" clicked!" << std::endl;
		return !update.handled;
	}

	return true;
}

void rm_button::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	const bool activation_key = vk == RM_KEY_ENTER || vk == RM_KEY_SPACE;
	const auto update = state == UP
		? m_behaviour.key_up(activation_key)
		: m_behaviour.key_down(activation_key);
	if (update.activated)
		std::cout << "Button \"" << m_text << "\" clicked!" << std::endl;
}

rm_label::rm_label(rm_widget* p_parent, int x, int y, const std::string& text, RmThemeRef theme)
	: rm_widget(x, y, 200, 30, p_parent, "ui_label"), m_text(text),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
}

rm_label::~rm_label() {}

void rm_label::on_draw(NVGcontext* pctx) {
	const RmLabelVisual visual{
		m_size.x,
		m_size.y,
		get_font(),
		m_text.c_str(),
		is_enabled()
	};
	RmDefaultControlPainter::draw_label(*pctx, visual, m_theme->label);
	rm_widget::on_draw(pctx);
}

rm_text_input::rm_text_input(rm_widget* p_parent, int x, int y, int width, int height,
	uint32_t flags, RmThemeRef theme, float blink_cursor_interval)
	: rm_widget(x, y, width, height, p_parent, "ui_text_input",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_OPAQUE),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_ctrl_pressed(false), m_blink_state(false), m_flags(flags),
	m_scroll_offset(0.0f), m_last_click_time(0.0), m_last_click_pos({ 0.0f, 0.0f })
{
	m_timer.set_interval(blink_cursor_interval);
}

rm_text_input::~rm_text_input() {}

void rm_text_input::reset_caret(bool visible)
{
	m_blink_state = visible;
	m_timer.reset(m_psysdf);
}

void rm_text_input::on_draw(NVGcontext* pctx)
{
	if (m_behaviour.is_active() && m_timer.has_elapsed(get_sysdf()))
		m_blink_state = !m_blink_state;

	const bool multiline = (m_flags & RMGUI_TEXT_INPUT_MULTILINE) != 0;
	const RmTextInputVisual visual{
		m_size.x,
		m_size.y,
		m_scroll_offset,
		get_font(),
		m_behaviour.text().c_str(),
		m_behaviour.cursor(),
		m_behaviour.selection_start(),
		m_behaviour.selection_end(),
		multiline,
		is_enabled(),
		m_elem_flags.is_hovered(),
		m_behaviour.is_dragging(),
		m_behaviour.is_active() && m_blink_state
	};
	m_layout = RmDefaultControlPainter::layout_text_input(
		*pctx, visual, m_theme->text_input);
	m_scroll_offset = m_layout.scroll_offset;
	RmDefaultControlPainter::draw_text_input(
		*pctx, visual, m_layout, m_theme->text_input);
	rm_widget::on_draw(pctx);
}

void rm_text_input::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmTextInputStyle& style = m_theme->text_input;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_text_input::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (!m_behaviour.is_active())
		return;

	if (vk == RM_KEY_LCTRL || vk == RM_KEY_RCTRL || vk == RM_KEY_CONTROL) {
		m_ctrl_pressed = state != RM_KEY_STATE::UP;
		return;
	}

	if (state != RM_KEY_STATE::DOWN && state != RM_KEY_STATE::REPEAT)
		return;

	RmBehaviourUpdate update;
	if (m_ctrl_pressed) {
		if (state == RM_KEY_STATE::DOWN) {
			switch (vk) {
			case RM_KEY_A:
				update = m_behaviour.select_all();
				break;
			case RM_KEY_C: {
				const std::string selected = m_behaviour.selected_text();
				if (!selected.empty()) {
					m_psysdf->set_clipboard_data_ex(
						reinterpret_cast<const uint8_t*>(selected.data()), selected.size());
					update.handled = true;
				}
				break;
			}
			case RM_KEY_X: {
				auto cut = m_behaviour.cut_selection();
				if (!cut.first.empty()) {
					m_psysdf->set_clipboard_data_ex(
						reinterpret_cast<const uint8_t*>(cut.first.data()), cut.first.size());
				}
				update = cut.second;
				break;
			}
			case RM_KEY_V: {
				size_t size = 0;
				RM_CB_DATA_TYPE type = RM_CLIPBOARD_DATA_TYPE_NONE;
				const char* text = reinterpret_cast<const char*>(
					m_psysdf->get_clipboard_data_ex(type, size));
				if (text && size > 0 && type == RM_CLIPBOARD_DATA_TYPE_TEXT)
					update = m_behaviour.insert_text(std::string(text, size));
				break;
			}
			case RM_KEY_Z:
				update = m_behaviour.undo();
				break;
			case RM_KEY_Y:
				update = m_behaviour.redo();
				break;
			default:
				break;
			}
		}
	}
	else {
		switch (vk) {
		case RM_KEY_BACKSPACE:
			update = m_behaviour.backspace();
			break;
		case RM_KEY_DELETE:
			update = m_behaviour.delete_forward();
			break;
		case RM_KEY_LEFT:
			update = m_behaviour.move_left();
			break;
		case RM_KEY_RIGHT:
			update = m_behaviour.move_right();
			break;
		case RM_KEY_UP:
			update = m_behaviour.move_up();
			break;
		case RM_KEY_DOWN:
			update = m_behaviour.move_down();
			break;
		case RM_KEY_HOME:
			update = m_behaviour.move_home();
			break;
		case RM_KEY_END:
			update = m_behaviour.move_end();
			break;
		case RM_KEY_ENTER:
			if ((m_flags & RMGUI_TEXT_INPUT_MULTILINE) != 0)
				update = m_behaviour.insert_codepoint('\n');
			break;
		default:
			break;
		}
	}

	if (update.handled || update.state_changed)
		reset_caret();
}

bool rm_text_input::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(vk);
	RM_UNUSED(delta);
	const bool inside = m_bbox.inside(cursor_pos);
	const float local_x = cursor_pos.x - m_pos_of_parent.x;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN && inside) {
		const double now = m_psysdf->get_time();
		const float dx = cursor_pos.x - m_last_click_pos.x;
		const float dy = cursor_pos.y - m_last_click_pos.y;
		const bool double_click = now - m_last_click_time <= DOUBLE_CLICK_THRESHOLD &&
			(dx * dx + dy * dy) <= CLICK_MOVE_THRESHOLD * CLICK_MOVE_THRESHOLD;
		const size_t index = hit_test_index(local_x, local_y);
		const RmBehaviourUpdate update =
			m_behaviour.pointer_down(index, double_click);
		if (update.handled) {
			get_root()->capture_pointer(this);
			reset_caret();
		}
		m_last_click_time = now;
		m_last_click_pos = cursor_pos;
		return !update.handled;
	}

	if (event == RM_MOUSE_EVENT_MOVE && m_behaviour.is_dragging()) {
		const size_t index = hit_test_index(local_x, local_y);
		const RmBehaviourUpdate update = m_behaviour.pointer_drag(index);
		if (update.state_changed) {
			ensure_visible(index);
			reset_caret();
		}
		return !update.handled;
	}

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up();
		return !update.handled;
	}
	return true;
}

size_t rm_text_input::hit_test_index(float px, float py) const
{
	return RmDefaultControlPainter::hit_test_text_input(
		m_layout, px, std::isnan(py) ? m_size.y * 0.5f : py, m_theme->text_input);
}

void rm_text_input::ensure_visible(size_t idx)
{
	if (m_layout.lines.empty())
		return;
	const RmTextInputLineLayout& line = m_layout.lines.front();
	const size_t local = idx <= line.text_start
		? 0 : std::min(idx - line.text_start, line.text.size());
	const auto found = std::lower_bound(
		line.byte_offsets.begin(), line.byte_offsets.end(), local);
	if (found == line.byte_offsets.end())
		return;
	const float x = line.glyph_positions[
		static_cast<size_t>(found - line.byte_offsets.begin())];
	const float available = std::max(0.0f,
		m_size.x - m_theme->text_input.horizontal_padding * 2.0f);
	if (x - m_scroll_offset > available)
		m_scroll_offset = x - available;
	else if (x < m_scroll_offset)
		m_scroll_offset = x;
}

void rm_text_input::on_text_input(int sym)
{
	if (sym >= 32) {
		const RmBehaviourUpdate update =
			m_behaviour.insert_codepoint(static_cast<uint32_t>(sym));
		if (update.state_changed)
			reset_caret();
	}
}

rm_checkbox::rm_checkbox(rm_widget* p_parent, int x, int y, int width,
	const std::string& label, rm_checkbox_cb pcallback, RmThemeRef theme)
	: rm_widget(x, y, width,
		static_cast<int>((theme ? theme : RmThemeSnapshot::default_theme())->checkbox.box_size),
		p_parent, "ui_checkbox"),
	m_behaviour(false), m_label(label),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_callback(pcallback);
}

rm_checkbox::~rm_checkbox() {}

void rm_checkbox::on_draw(NVGcontext* pctx) {
	const RmCheckboxVisual visual{
		m_size.x,
		m_size.y,
		get_font(),
		m_label.c_str(),
		is_enabled(),
		m_elem_flags.is_hovered(),
		m_behaviour.is_pressed(),
		m_behaviour.is_checked()
	};
	RmDefaultControlPainter::draw_checkbox(*pctx, visual, m_theme->checkbox);
	rm_widget::on_draw(pctx);
}

void rm_checkbox::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmCheckboxStyle& style = m_theme->checkbox;
	const float box_size = std::min(style.box_size, m_size.y);
	const float box_y = (m_size.y - box_size) * 0.5f;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, box_y, box_size, box_size }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

bool rm_checkbox::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	RM_UNUSED(delta);
	const bool inside = m_bbox.inside(cursor_pos);

	if (event == RM_MOUSE_EVENT_MOVE) {
		const auto update = m_behaviour.pointer_move(inside);
		return !update.handled;
	}

	if (event != RM_MOUSE_EVENT_CLICK || vk != RM_KEY_LMOUSE)
		return true;

	if (state == DOWN) {
		const auto update = m_behaviour.pointer_down(inside);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}

	if (state == UP) {
		const auto update = m_behaviour.pointer_up(inside);
		if (update.activated && is_valid_callback())
			get_callback()(this);
		return !update.handled;
	}

	return true;
}

void rm_checkbox::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	const bool activation_key = vk == RM_KEY_ENTER || vk == RM_KEY_SPACE;
	const auto update = state == UP
		? m_behaviour.key_up(activation_key)
		: m_behaviour.key_down(activation_key);
	if (update.activated && is_valid_callback())
		get_callback()(this);
}

rm_combobox::rm_combobox(rm_widget* p_parent, int x, int y, int width, int height,
	rm_combobox_cb pcallback, RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_combobox",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_DISABLE_SCISSOR),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_placeholder("Select an item")
{
	set_callback(pcallback);
	set_zindex(base_zindex);
	m_behaviour.set_count(0);
}

rm_combobox::~rm_combobox() {}

size_t rm_combobox::add_item(const char* pitem, void* puserdata)
{
	m_items.push_back({ pitem, puserdata });
	m_behaviour.set_count(m_items.size());
	return m_items.size() - 1;
}

bool rm_combobox::remove_item(size_t index)
{
	if (index >= m_items.size())
		return false;
	const size_t selected = m_behaviour.selected_index();
	m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
	m_behaviour.set_count(m_items.size());
	if (!m_items.empty() && selected != RmComboBoxBehaviour::invalid_index) {
		const size_t replacement = selected > index
			? selected - 1 : std::min(selected, m_items.size() - 1);
		m_behaviour.select(replacement);
	}
	sync_popup_layer();
	return true;
}

void rm_combobox::clear_items()
{
	m_items.clear();
	m_behaviour.set_count(0);
	sync_popup_layer();
}

size_t rm_combobox::find_item(const char* pitem)
{
	if (!pitem)
		return kinvalid_index;
	auto it = std::find_if(m_items.begin(), m_items.end(),
		[pitem](const rm_combo_item& item) {
			return !strcmp(pitem, item.get_name());
		}
	);

	if (it != m_items.end())
		return it - m_items.begin();

	return kinvalid_index;
}

float rm_combobox::popup_y() const
{
	return m_size.y + combo_style().popup_gap;
}

float rm_combobox::popup_height() const
{
	return combo_style().popup_padding * 2.f +
		combo_style().item_height * static_cast<float>(m_items.size());
}

size_t rm_combobox::hit_test_popup_item(const rm_vec2& local_cursor) const
{
	const RmComboBoxStyle& style = combo_style();
	if (local_cursor.x < 0.f || local_cursor.x > m_size.x)
		return RmComboBoxBehaviour::invalid_index;
	const float item_y = local_cursor.y - popup_y() - style.popup_padding;
	if (item_y < 0.f || style.item_height <= 0.f)
		return RmComboBoxBehaviour::invalid_index;
	const size_t index = static_cast<size_t>(item_y / style.item_height);
	return index < m_items.size() ? index : RmComboBoxBehaviour::invalid_index;
}

void rm_combobox::notify_selection()
{
	const size_t selected = m_behaviour.selected_index();
	if (selected < m_items.size() && is_valid_callback())
		get_callback()(this, &m_items[selected], selected);
}

void rm_combobox::sync_popup_layer()
{
	set_zindex(m_behaviour.is_expanded() ? popup_zindex : base_zindex);
}

void rm_combobox::on_enabled_changed(bool enabled)
{
	m_behaviour.set_enabled(enabled);
	sync_popup_layer();
}

bool rm_combobox::set_selected_index(size_t index, bool notify)
{
	const RmBehaviourUpdate update = m_behaviour.select(index);
	if (!update.handled)
		return false;
	if (notify && update.state_changed)
		notify_selection();
	return true;
}

void rm_combobox::on_draw(NVGcontext* pctx)
{
	const size_t selected = m_behaviour.selected_index();
	const bool has_selection = selected < m_items.size();
	const RmComboBoxVisual field_visual{
		m_size.x, m_size.y, get_font(),
		has_selection ? m_items[selected].get_name() : m_placeholder.c_str(),
		!has_selection, is_enabled(), m_elem_flags.is_hovered(),
		m_behaviour.is_expanded()
	};
	RmDefaultControlPainter::draw_combobox(*pctx, field_visual, combo_style());

	if (m_behaviour.is_expanded()) {
		const RmComboBoxStyle& style = combo_style();
		const float y = popup_y();
		RmDefaultControlPainter::draw_combobox_popup(*pctx,
			{ y, m_size.x, popup_height() }, style);
		for (size_t i = 0; i < m_items.size(); ++i) {
			const RmComboBoxItemVisual item_visual{
				style.popup_padding,
				y + style.popup_padding + style.item_height * static_cast<float>(i),
				std::max(0.f, m_size.x - style.popup_padding * 2.f),
				style.item_height, get_font(), m_items[i].get_name(), is_enabled(),
				m_behaviour.highlighted_index() == i,
				m_behaviour.selected_index() == i
			};
			RmDefaultControlPainter::draw_combobox_item(*pctx, item_visual, style);
		}
	}
	rm_widget::on_draw(pctx);
}

void rm_combobox::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmComboBoxStyle& style = combo_style();
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_combobox::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != DOWN)
		return;
	if (vk == RM_KEY_ESCAPE) {
		m_behaviour.close();
		sync_popup_layer();
		return;
	}
	if (vk == RM_KEY_F4) {
		m_behaviour.toggle();
		sync_popup_layer();
		return;
	}
	if (vk == RM_KEY_ENTER || vk == RM_KEY_SPACE) {
		if (m_behaviour.is_expanded()) {
			if (m_behaviour.commit_highlighted().activated)
				notify_selection();
		}
		else {
			m_behaviour.open();
		}
		sync_popup_layer();
		return;
	}
	if (vk == RM_KEY_UP || vk == RM_KEY_DOWN) {
		const int delta = vk == RM_KEY_DOWN ? 1 : -1;
		const RmBehaviourUpdate update = m_behaviour.is_expanded()
			? m_behaviour.highlight_relative(delta)
			: m_behaviour.select_relative(delta);
		if (update.activated)
			notify_selection();
		return;
	}
	if (vk == RM_KEY_HOME || vk == RM_KEY_END) {
		if (m_items.empty())
			return;
		const size_t index = vk == RM_KEY_HOME ? 0 : m_items.size() - 1;
		const RmBehaviourUpdate update = m_behaviour.is_expanded()
			? m_behaviour.highlight(index) : m_behaviour.select(index);
		if (!m_behaviour.is_expanded() && update.state_changed)
			notify_selection();
	}
}

bool rm_combobox::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;
	const rm_vec2 local_cursor = cursor_to_local(cursor_pos);
	const bool inside_field = local_cursor.x >= 0.f && local_cursor.x <= m_size.x &&
		local_cursor.y >= 0.f && local_cursor.y <= m_size.y;
	if (event == RM_MOUSE_EVENT_MOVE) {
		if (m_behaviour.is_expanded()) {
			m_behaviour.highlight(hit_test_popup_item(local_cursor));
			return false;
		}
		return true;
	}
	if (event != RM_MOUSE_EVENT_CLICK)
		return !m_behaviour.is_expanded();
	if (state == DOWN) {
		if (inside_field) {
			m_behaviour.toggle();
			sync_popup_layer();
			return false;
		}
		if (m_behaviour.is_expanded()) {
			const size_t index = hit_test_popup_item(local_cursor);
			if (index < m_items.size()) {
				m_behaviour.highlight(index);
				if (m_behaviour.commit_highlighted().activated)
					notify_selection();
			}
			else {
				m_behaviour.close();
			}
			sync_popup_layer();
			return false;
		}
	}
	return !m_behaviour.is_expanded();
}

void rm_slider::update_value_from_pointer(const rm_vec2& local_cursor)
{
	const float pad = m_theme->slider.padding;
	float track_w = m_size.x - pad * 2.f;
	const float fraction = track_w > FLT_EPSILON ? (local_cursor.x - pad) / track_w : 0.f;
	const float previous = m_behaviour.value();
	if (m_behaviour.is_dragging())
		m_behaviour.drag_to(fraction);
	else
		m_behaviour.begin_drag(fraction);
	if (m_pcallback && std::fabs(previous - m_behaviour.value()) > FLT_EPSILON)
		m_pcallback(this);
}

rm_slider::rm_slider(rm_widget* p_parent, int x, int y, int width, int height,
	float min, float max, float initial, rm_slider_callback pcallback, RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_slider", RM_FLAG_DEFAULT),
	m_behaviour(min, max, initial), m_pcallback(pcallback),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()) {
}

rm_slider::~rm_slider() {}

void rm_slider::on_draw(NVGcontext* pctx) {
	const RmSliderVisual visual{
		m_size.x,
		m_size.y,
		m_behaviour.fraction(),
		is_enabled(),
		m_elem_flags.is_hovered(),
		m_behaviour.is_dragging()
	};
	RmDefaultControlPainter::draw_slider(*pctx, visual, m_theme->slider);

	rm_widget::on_draw(pctx);
}

void rm_slider::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmSliderStyle& style = m_theme->slider;
	const float track_width = std::max(0.0f, m_size.x - style.padding * 2.0f);
	const float thumb_x = style.padding + track_width * m_behaviour.fraction();
	RmDefaultControlPainter::draw_circle_focus_ring(*pctx, thumb_x,
		m_size.y * 0.5f, style.thumb_radius + style.focus_ring_width,
		style.focus_ring_width, style.focus_ring);
}

bool rm_slider::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	RM_UNUSED(delta);
	rm_vec2 local = cursor_to_local(cursor_pos);
	if (event == RM_MOUSE_EVENT_CLICK && vk == RM_KEY_LMOUSE && state == DOWN && m_bbox.inside(cursor_pos)) {
		update_value_from_pointer(local);
		if (get_root())
			get_root()->capture_pointer(this);
		return false;
	}
	if (event == RM_MOUSE_EVENT_CLICK && vk == RM_KEY_LMOUSE && state == UP && m_behaviour.is_dragging()) {
		m_behaviour.end_drag();
		return false;
	}
	if (m_behaviour.is_dragging() && event == RM_MOUSE_EVENT_MOVE) {
		update_value_from_pointer(local);
		return false;
	}
	return true;
}

rm_progress::rm_progress(rm_widget* p_parent, int x, int y, int width, int height,
	float initial, RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "ui_progress"),
	m_behaviour(initial), m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{}

rm_progress::~rm_progress()
{
}

void rm_progress::on_draw(NVGcontext* pctx)
{
	RmDefaultControlPainter::draw_progress(*pctx,
		{ m_size.x, m_size.y, m_behaviour.fraction(), is_enabled() },
		m_theme->progress);
	rm_widget::on_draw(pctx);
}

rm_progress_image::rm_progress_image(rm_widget* p_parent, int x, int y, int width, int height,
	rm_image img, float pattern_angle, float pattern_alpha, float initial, RmThemeRef theme) :
	rm_progress(p_parent, x, y, width, height, initial, std::move(theme))
{
	m_angle = pattern_angle;
	m_alpha = pattern_alpha;
	m_image = img;
}

rm_progress_image::~rm_progress_image()
{
}

void rm_progress_image::on_draw(NVGcontext* pctx)
{
	// paint background
	pctx->beginPath();
	pctx->fillColor(m_theme->progress.background.resolve(
		is_enabled() ? RmVisualState::normal : RmVisualState::disabled));
	pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, m_theme->progress.corner_radius);
	pctx->fill();

	// paint progres bar
	rm_rect percent_rect(0.f, 0.f, m_size);
	percent_rect.width *= m_behaviour.fraction();

	if (percent_rect.width > 0.0f) {
		NVGpaint paint = NVGpaint::imagePattern(0, 0, percent_rect.width,
			percent_rect.height, m_angle, m_image, m_alpha);
		pctx->beginPath();
		pctx->roundedRect(
			percent_rect.x, percent_rect.y,
			percent_rect.width, percent_rect.height, m_theme->progress.corner_radius);
		pctx->fillPaint(paint);
		pctx->fill();

		drawSprite(pctx, m_image, m_alpha, 0.f, 0.f, 13.f, 15.f,
			percent_rect.x, percent_rect.y, percent_rect.width, percent_rect.height,
			m_theme->progress.corner_radius, m_theme->progress.corner_radius,
			m_theme->progress.corner_radius, m_theme->progress.corner_radius);
	}
	rm_widget::on_draw(pctx);
}

void rm_animation::on_draw(NVGcontext* pctx)
{
	rm_vec2 pos(m_size.x / 2.f, m_size.y / 2.f);
	rm_widget::on_draw(pctx);
	pctx->translate(pos.x, pos.y);
	pctx->rotate(m_angle);
	pctx->scale(m_scale, m_scale);
	pctx->translate(-pos.x, -pos.y);
	pctx->beginPath();
	NVGpaint imgPaint = NVGpaint::imagePattern(0.f, 0.f, m_size.x, m_size.y, 0.0f, m_image, 1.0f);
	pctx->beginPath();
	pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, 4.0f);
	pctx->fillPaint(imgPaint);
	pctx->fill();
	m_angle += m_speed * m_proot->get_delta_time();
}

rm_animation::rm_animation(rm_widget* p_parent, int x, int y, int width, int height, rm_image img, float start_angle, float scale, float speed) :
	rm_widget(x, y, width, height, p_parent, "ui_animation"), m_image(img), m_speed(speed), m_angle(start_angle), m_scale(scale)
{
}

rm_animation::~rm_animation()
{
}

void rm_scrollbar::adjust_geometry()
{
	if (!m_pparent)
		return;
	const rm_vec2& parent_size = m_pparent->get_size();
	const float thickness = m_theme->scrollbar.thickness;
	bool has_crossbar = false;
	for (size_t index = 0; index < m_pparent->get_num_childs(); ++index) {
		rm_scrollbar* p_other = dynamic_cast<rm_scrollbar*>(
			m_pparent->get_child(index));
		if (p_other && p_other != this &&
			p_other->get_orientation() != m_orientation &&
			(!m_scroll_target || p_other->get_scroll_target() == m_scroll_target)) {
			has_crossbar = true;
			break;
		}
	}
	if (is_vertical()) {
		move({ std::max(0.f, parent_size.x - thickness), 0.f });
		resize(thickness, std::max(0.0f,
			parent_size.y - (has_crossbar ? thickness : 0.0f)));
	}
	else {
		move({ 0.f, std::max(0.f, parent_size.y - thickness) });
		resize(std::max(0.0f,
			parent_size.x - (has_crossbar ? thickness : 0.0f)), thickness);
	}
}

void rm_scrollbar::sync_from_target()
{
	if (!m_scroll_target)
		return;
	m_scroll_target->update_content_extent_from_children();
	if (m_scroll_target == m_pparent) {
		bool has_horizontal = false;
		bool has_vertical = false;
		for (size_t index = 0; index < m_pparent->get_num_childs(); ++index) {
			rm_scrollbar* p_scrollbar = dynamic_cast<rm_scrollbar*>(
				m_pparent->get_child(index));
			if (!p_scrollbar || p_scrollbar->get_scroll_target() != m_scroll_target)
				continue;
			has_vertical |= p_scrollbar->get_orientation() == RM_ORIENT_VERT;
			has_horizontal |= p_scrollbar->get_orientation() == RM_ORIENT_HORZ;
		}
		const rm_rect current = m_scroll_target->get_content_area();
		const rm_vec2& target_size = m_scroll_target->get_size();
		const float thickness = m_theme->scrollbar.thickness;
		m_scroll_target->set_content_area({ current.x, current.y,
			std::max(0.0f, target_size.x - current.x -
				(has_vertical ? thickness : 0.0f)),
			std::max(0.0f, target_size.y - current.y -
				(has_horizontal ? thickness : 0.0f)) });
	}
	const rm_vec2& extent = m_scroll_target->get_content_extent();
	const rm_rect& viewport = m_scroll_target->get_content_area();
	const rm_vec2& offset = m_scroll_target->get_content_offset();
	const rm_vec2 maximum = m_scroll_target->get_max_content_offset();
	const float content_extent = is_vertical() ? extent.y : extent.x;
	const float viewport_extent = is_vertical() ? viewport.height : viewport.width;
	const float maximum_offset = is_vertical() ? maximum.y : maximum.x;
	const float current_offset = is_vertical() ? offset.y : offset.x;
	m_behaviour.set_viewport_fraction(content_extent > FLT_EPSILON
		? viewport_extent / content_extent : 1.0f);
	m_behaviour.set_position(maximum_offset > FLT_EPSILON
		? current_offset / maximum_offset : 0.0f);
}

void rm_scrollbar::apply_to_target()
{
	if (!m_scroll_target)
		return;
	rm_vec2 offset = m_scroll_target->get_content_offset();
	const rm_vec2 maximum = m_scroll_target->get_max_content_offset();
	if (is_vertical())
		offset.y = maximum.y * m_behaviour.position();
	else
		offset.x = maximum.x * m_behaviour.position();
	m_scroll_target->set_content_offset(offset);
}

void rm_scrollbar::notify_position()
{
	apply_to_target();
	if (is_valid_callback())
		get_callback()(this, m_behaviour.position());
}

void rm_scrollbar::on_draw(NVGcontext* pctx)
{
	sync_from_target();
	adjust_geometry();
	const RmScrollbarStyle& style = m_theme->scrollbar;
	const float length = track_length();
	RmDefaultControlPainter::draw_scrollbar(*pctx,
		{ m_size.x, m_size.y,
		  m_behaviour.thumb_offset(length, style.minimum_thumb_length),
		  m_behaviour.thumb_length(length, style.minimum_thumb_length),
		  is_vertical(), is_enabled(), m_elem_flags.is_hovered(),
		  m_behaviour.is_dragging() }, style);
	rm_widget::on_draw(pctx);
}

void rm_scrollbar::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmScrollbarStyle& style = m_theme->scrollbar;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_scrollbar::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != DOWN)
		return;
	RmBehaviourUpdate update;
	const bool decrease = vk == RM_KEY_LEFT || vk == RM_KEY_UP;
	const bool increase = vk == RM_KEY_RIGHT || vk == RM_KEY_DOWN;
	if (decrease || increase)
		update = m_behaviour.step(increase ? 0.05f : -0.05f);
	else if (vk == RM_KEY_PAGE_UP || vk == RM_KEY_PAGE_DOWN)
		update = m_behaviour.step(vk == RM_KEY_PAGE_DOWN
			? m_behaviour.viewport_fraction() : -m_behaviour.viewport_fraction());
	else if (vk == RM_KEY_HOME || vk == RM_KEY_END) {
		update = m_behaviour.set_position(vk == RM_KEY_END ? 1.f : 0.f);
		update.activated = update.state_changed;
	}
	if (update.activated)
		notify_position();
}

bool rm_scrollbar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;
	const RmScrollbarStyle& style = m_theme->scrollbar;
	const float pointer = pointer_axis(cursor_to_local(cursor_pos));
	if (event == RM_MOUSE_EVENT_MOVE && m_behaviour.is_dragging()) {
		const RmBehaviourUpdate update = m_behaviour.drag_to(
			pointer, track_length(), style.minimum_thumb_length);
		if (update.state_changed)
			notify_position();
		return false;
	}
	if (event != RM_MOUSE_EVENT_CLICK)
		return true;
	if (state == DOWN && m_bbox.inside(cursor_pos)) {
		const float previous = m_behaviour.position();
		const RmBehaviourUpdate update = m_behaviour.begin_drag(
			pointer, track_length(), style.minimum_thumb_length);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		if (std::fabs(previous - m_behaviour.position()) > FLT_EPSILON)
			notify_position();
		return !update.handled;
	}
	if (state == UP) {
		const RmBehaviourUpdate update = m_behaviour.end_drag();
		return !update.handled;
	}
	return true;
}

rm_scrollbar::rm_scrollbar(rm_widget* p_parent, RM_ORIENT orientation,
	float initial_position, rm_scrollbar_cb p_callback, RmThemeRef theme) :
	rm_widget(0, 0, 0, 0, p_parent, "ui_scrollbar", RM_FLAG_DEFAULT),
	m_orientation(orientation == RM_ORIENT_HORZ ? RM_ORIENT_HORZ : RM_ORIENT_VERT),
	m_behaviour(initial_position),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_callback(p_callback);
	set_fixed_to_viewport(true);
	adjust_geometry();
}

rm_scrollbar::~rm_scrollbar()
{
}

void rm_scrollbar::set_position(float position, bool notify)
{
	const RmBehaviourUpdate update = m_behaviour.set_position(position);
	if (!update.state_changed)
		return;
	apply_to_target();
	if (notify && is_valid_callback())
		get_callback()(this, m_behaviour.position());
}

void rm_scrollbar::set_content_metrics(float content_extent, float viewport_extent)
{
	const float fraction = content_extent > FLT_EPSILON
		? viewport_extent / content_extent : 1.f;
	m_behaviour.set_viewport_fraction(fraction);
}

void rm_scrollbar::bind_to(rm_widget* p_target)
{
	m_scroll_target = p_target;
	set_fixed_to_viewport(m_pparent == p_target);
	sync_from_target();
	adjust_geometry();
}

void rm_scrollbar::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	adjust_geometry();
}

const RmToolStripStyle& rm_toolstrip::toolstrip_style() const
{
	return m_behaviour.is_exclusive() ? m_theme->toolbox : m_theme->toolbar;
}

void rm_toolstrip::rebuild_layout()
{
	m_item_layout.clear();
	m_group_layout.clear();
	const RmToolStripStyle& style = toolstrip_style();
	const bool horizontal = m_orientation == RM_ORIENT_HORZ;
	float group_origin = style.group_padding;

	for (size_t group_index = 0; group_index < m_groups.size(); ++group_index) {
		const rm_tool_group& group = m_groups[group_index];
		const size_t cross_count = std::max<size_t>(1, group.cross_count);
		const size_t main_count = group.items.empty()
			? 0 : (group.items.size() + cross_count - 1) / cross_count;
		const float content_main = main_count > 0
			? main_count * style.button_extent + (main_count - 1) * style.button_gap : 0.0f;
		const float content_cross = cross_count * style.button_extent
			+ (cross_count - 1) * style.button_gap;
		const bool has_label = group.label_placement != RmToolGroupLabelPlacement::none
			&& !group.text.empty();
		const float label_height = has_label ? style.group_label_height : 0.0f;

		GroupLayout group_layout;
		if (horizontal) {
			const float width = content_main + style.group_padding * 2.0f;
			const float height = content_cross + label_height + style.group_padding * 2.0f;
			group_layout.bounds = { group_origin, style.group_padding, width, height };
			if (has_label) {
				const float label_y = group.label_placement == RmToolGroupLabelPlacement::top
					? style.group_padding : style.group_padding + content_cross;
				group_layout.label_bounds = { group_origin + style.group_padding,
					label_y, content_main, label_height };
			}
			const float item_y = style.group_padding +
				(has_label && group.label_placement == RmToolGroupLabelPlacement::top
					? label_height : 0.0f);
			for (size_t item = 0; item < group.items.size(); ++item) {
				const size_t main_slot = item / cross_count;
				const size_t cross_slot = item % cross_count;
				m_item_layout.push_back({ group_index, item, {
					group_origin + style.group_padding + main_slot *
						(style.button_extent + style.button_gap),
					item_y + cross_slot * (style.button_extent + style.button_gap),
					style.button_extent, style.button_extent } });
			}
			group_origin += width + style.group_gap;
		} else {
			const float width = content_cross + style.group_padding * 2.0f;
			const float height = content_main + label_height + style.group_padding * 2.0f;
			group_layout.bounds = { style.group_padding, group_origin, width, height };
			if (has_label) {
				const float label_y = group.label_placement == RmToolGroupLabelPlacement::top
					? group_origin + style.group_padding
					: group_origin + style.group_padding + content_main;
				group_layout.label_bounds = { style.group_padding, label_y,
					width, label_height };
			}
			const float item_y = group_origin + style.group_padding +
				(has_label && group.label_placement == RmToolGroupLabelPlacement::top
					? label_height : 0.0f);
			for (size_t item = 0; item < group.items.size(); ++item) {
				const size_t main_slot = item / cross_count;
				const size_t cross_slot = item % cross_count;
				m_item_layout.push_back({ group_index, item, {
					style.group_padding + cross_slot *
						(style.button_extent + style.button_gap),
					item_y + main_slot * (style.button_extent + style.button_gap),
					style.button_extent, style.button_extent } });
			}
			group_origin += height + style.group_gap;
		}
		m_group_layout.push_back(group_layout);
	}
	m_behaviour.set_count(m_item_layout.size());
}

size_t rm_toolstrip::hit_test_item(const rm_vec2& cursor_pos) const
{
	const rm_vec2 local = rm_vec2(cursor_pos.x - m_pos_of_parent.x,
		cursor_pos.y - m_pos_of_parent.y);
	for (size_t i = 0; i < m_item_layout.size(); ++i) {
		const ItemLayout& layout = m_item_layout[i];
		if (local.x >= layout.bounds.x && local.y >= layout.bounds.y &&
			local.x <= layout.bounds.x + layout.bounds.width &&
			local.y <= layout.bounds.y + layout.bounds.height) {
			const rm_tool_item& item = m_groups[layout.group].items[layout.item];
			return item.enabled ? i : RmToolStripBehaviour::invalid_index;
		}
	}
	return RmToolStripBehaviour::invalid_index;
}

rm_tool_item* rm_toolstrip::item_from_flat_index(size_t index)
{
	if (index >= m_item_layout.size())
		return nullptr;
	const ItemLayout& layout = m_item_layout[index];
	return &m_groups[layout.group].items[layout.item];
}

void rm_toolstrip::on_draw(NVGcontext* pctx)
{
	rebuild_layout();
	const RmToolStripStyle& style = toolstrip_style();
	RmDefaultControlPainter::draw_toolstrip_surface(*pctx,
		{ m_size.x, m_size.y }, style);
	for (size_t i = 0; i < m_group_layout.size(); ++i) {
		const rm_rect& bounds = m_group_layout[i].bounds;
		const rm_rect& label = m_group_layout[i].label_bounds;
		RmDefaultControlPainter::draw_toolstrip_group(*pctx,
			{ { bounds.x, bounds.y, bounds.width, bounds.height },
			  { label.x, label.y, label.width, label.height },
			  m_font, m_groups[i].text.c_str() }, style);
	}
	for (size_t i = 0; i < m_item_layout.size(); ++i) {
		const ItemLayout& layout = m_item_layout[i];
		const rm_tool_item& item = m_groups[layout.group].items[layout.item];
		RmDefaultControlPainter::draw_toolstrip_button(*pctx,
			{ { layout.bounds.x, layout.bounds.y, layout.bounds.width,
			    layout.bounds.height }, m_font, get_imagelist(), item.icon,
			  item.text.c_str(),
			  is_enabled() && item.enabled,
			  m_behaviour.hovered_index() == i,
			  m_behaviour.pressed_index() == i,
			  m_behaviour.selected_index() == i }, style);
	}
	rm_widget::on_draw(pctx);
}

void rm_toolstrip::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (!m_behaviour.is_exclusive() || state != RM_KEY_STATE::DOWN ||
		m_item_layout.empty())
		return;
	const bool forward = vk == RM_KEY_RIGHT || vk == RM_KEY_DOWN;
	const bool backward = vk == RM_KEY_LEFT || vk == RM_KEY_UP;
	if (!forward && !backward && vk != RM_KEY_ENTER && vk != RM_KEY_SPACE)
		return;
	size_t index = m_behaviour.selected_index();
	if (index >= m_item_layout.size())
		index = 0;
	else if (forward)
		index = (index + 1) % m_item_layout.size();
	else if (backward)
		index = (index + m_item_layout.size() - 1) % m_item_layout.size();
	const RmBehaviourUpdate update = m_behaviour.select(index);
	if (update.activated) {
		rm_tool_item* item = item_from_flat_index(index);
		if (item && is_valid_callback())
			get_callback()(this, item->id);
	}
}

bool rm_toolstrip::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	const size_t index = hit_test_item(cursor_pos);
	if (event == RM_MOUSE_EVENT_MOVE) {
		const RmBehaviourUpdate update = m_behaviour.pointer_move(index);
		return !update.handled;
	}
	if (event != RM_MOUSE_EVENT_CLICK || vk != RM_KEY_LMOUSE)
		return true;
	if (state == RM_KEY_STATE::DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (update.activated) {
			rm_tool_item* item = item_from_flat_index(index);
			if (item && is_valid_callback())
				get_callback()(this, item->id);
		}
		return !update.handled;
	}
	return true;
}

rm_toolstrip::rm_toolstrip(rm_widget* p_parent, int x, int y, int width,
	int height, RM_ORIENT orientation, bool exclusive,
	rm_toolstrip_cb callback, RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "rm_toolstrip", RM_FLAG_DEFAULT),
	m_behaviour(exclusive),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_orientation(orientation == RM_ORIENT_VERT ? RM_ORIENT_VERT : RM_ORIENT_HORZ)
{
	set_callback(callback);
	set_min_size(rm_vec2(0.0f, 0.0f));
	set_max_size(rm_vec2(0.0f, 0.0f));
}

size_t rm_toolstrip::add_group(const char* p_text,
	RmToolGroupLabelPlacement placement, size_t cross_count)
{
	rm_tool_group group;
	group.text = p_text ? p_text : "";
	group.label_placement = placement;
	group.cross_count = std::max<size_t>(1, cross_count);
	m_groups.push_back(std::move(group));
	rebuild_layout();
	return m_groups.size() - 1;
}

rm_tool_item* rm_toolstrip::add_tool(size_t group, uint32_t id,
	const char* p_text, const char* p_tooltip, rm_image_index icon,
	void* p_userdata)
{
	if (group >= m_groups.size())
		return nullptr;
	rm_tool_item item;
	item.id = id;
	item.text = p_text ? p_text : "";
	item.tooltip = p_tooltip ? p_tooltip : "";
	item.icon = icon;
	item.userdata = p_userdata;
	m_groups[group].items.push_back(std::move(item));
	rebuild_layout();
	return &m_groups[group].items.back();
}

bool rm_toolstrip::select_tool(uint32_t id, bool notify)
{
	if (!m_behaviour.is_exclusive())
		return false;
	for (size_t i = 0; i < m_item_layout.size(); ++i) {
		rm_tool_item* item = item_from_flat_index(i);
		if (item && item->id == id) {
			const RmBehaviourUpdate update = m_behaviour.select(i);
			if (notify && update.activated && is_valid_callback())
				get_callback()(this, id);
			return update.handled;
		}
	}
	return false;
}

uint32_t rm_toolstrip::get_selected_tool_id() const
{
	const size_t index = m_behaviour.selected_index();
	if (index >= m_item_layout.size())
		return std::numeric_limits<uint32_t>::max();
	const ItemLayout& layout = m_item_layout[index];
	return m_groups[layout.group].items[layout.item].id;
}

rm_tool_item* rm_toolstrip::find_tool(uint32_t id)
{
	for (rm_tool_group& group : m_groups)
		for (rm_tool_item& item : group.items)
			if (item.id == id)
				return &item;
	return nullptr;
}

void rm_toolstrip::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	rebuild_layout();
}

void rm_toolstrip::resize(float width, float height)
{
	rm_widget::resize(width, height);
	rebuild_layout();
}

rm_rebar::rm_rebar(rm_widget* p_parent, int x, int y, int width, int height,
	RM_ORIENT orientation, RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "rm_rebar", RM_FLAG_DEFAULT),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_orientation(orientation == RM_ORIENT_VERT ? RM_ORIENT_VERT : RM_ORIENT_HORZ)
{
}

void rm_rebar::layout_bands()
{
	if (m_layouting)
		return;
	m_layouting = true;
	const RmRebarStyle& style = m_theme->rebar;
	const bool horizontal = m_orientation == RM_ORIENT_HORZ;
	const float available_main = std::max(0.0f,
		(horizontal ? m_size.x : m_size.y) - style.band_padding * 2.0f);
	const float available_cross = std::max(0.0f,
		(horizontal ? m_size.y : m_size.x) - style.band_padding * 2.0f);
	float main = 0.0f;
	float cross = 0.0f;
	float row_cross = 0.0f;

	for (size_t band_index = 0; band_index < m_bands.size(); ++band_index) {
		Band& band = m_bands[band_index];
		if (!band.widget)
			continue;
		const rm_vec2 child_size = band.widget->get_size();
		float child_main = band.preferred_extent > 0.0f ? band.preferred_extent
			: (horizontal ? child_size.x : child_size.y);
		child_main = std::max(child_main, band.minimum_extent);
		float child_cross = horizontal ? child_size.y : child_size.x;
		child_cross = std::min(std::max(child_cross, 1.0f), available_cross);
		const float gripped_main = child_main + style.gripper_extent;
		if (main > 0.0f && main + style.band_gap + gripped_main > available_main) {
			main = 0.0f;
			cross += row_cross + style.row_gap;
			row_cross = 0.0f;
		}
		if (main > 0.0f)
			main += style.band_gap;
		if (band.stretch) {
			float remaining = 0.0f;
			for (size_t next_index = band_index + 1;
				next_index < m_bands.size(); ++next_index) {
				const Band& next = m_bands[next_index];
				if (!next.widget)
					continue;
				const rm_vec2 next_size = next.widget->get_size();
				const float next_preferred = next.preferred_extent > 0.0f
					? next.preferred_extent
					: (horizontal ? next_size.x : next_size.y);
				remaining += std::max(next_preferred, next.minimum_extent)
					+ style.gripper_extent + style.band_gap;
			}
			child_main = std::max(child_main,
				available_main - main - style.gripper_extent - remaining);
		}
		child_main = std::min(child_main,
			std::max(0.0f, available_main - main - style.gripper_extent));
		if (horizontal)
			band.bounds = { style.band_padding + main, style.band_padding + cross,
				child_main + style.gripper_extent, child_cross };
		else
			band.bounds = { style.band_padding + cross, style.band_padding + main,
				child_cross, child_main + style.gripper_extent };
		const rm_vec2 child_pos = horizontal
			? rm_vec2(band.bounds.x + style.gripper_extent, band.bounds.y)
			: rm_vec2(band.bounds.x, band.bounds.y + style.gripper_extent);
		band.widget->move(child_pos);
		band.widget->resize(horizontal ? child_main : child_cross,
			horizontal ? child_cross : child_main);
		main += child_main + style.gripper_extent;
		row_cross = std::max(row_cross, child_cross);
	}
	m_layouting = false;
}

void rm_rebar::on_draw(NVGcontext* pctx)
{
	layout_bands();
	RmDefaultControlPainter::draw_rebar(*pctx, { m_size.x, m_size.y },
		m_theme->rebar);
	for (const Band& band : m_bands)
		RmDefaultControlPainter::draw_rebar_band(*pctx,
			{ { band.bounds.x, band.bounds.y, band.bounds.width,
			    band.bounds.height }, m_orientation == RM_ORIENT_VERT },
			m_theme->rebar);
	rm_widget::on_draw(pctx);
}

bool rm_rebar::add_band(rm_widget* p_widget, float preferred_extent,
	float minimum_extent, bool stretch)
{
	if (!p_widget)
		return false;
	if (p_widget->get_parent() != this)
		p_widget->set_parent(this);
	p_widget->set_min_size(m_orientation == RM_ORIENT_HORZ
		? rm_vec2(std::max(0.0f, minimum_extent), 0.0f)
		: rm_vec2(0.0f, std::max(0.0f, minimum_extent)));
	p_widget->set_max_size(rm_vec2(0.0f, 0.0f));
	m_bands.push_back({ p_widget, preferred_extent, minimum_extent, stretch, {} });
	layout_bands();
	return true;
}

bool rm_rebar::remove_band(rm_widget* p_widget)
{
	const auto found = std::find_if(m_bands.begin(), m_bands.end(),
		[p_widget](const Band& band) { return band.widget == p_widget; });
	if (found == m_bands.end())
		return false;
	m_bands.erase(found);
	layout_bands();
	return true;
}

void rm_rebar::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	layout_bands();
}

void rm_rebar::resize(float width, float height)
{
	rm_widget::resize(width, height);
	layout_bands();
}

float rm_splitter::available_extent() const
{
	if (!m_pparent)
		return 0.0f;
	const rm_rect& area = m_pparent->get_content_area();
	const float total = m_orientation == RM_ORIENT_VERT ? area.width : area.height;
	return std::max(0.0f, total - m_theme->splitter.thickness);
}

void rm_splitter::update_limits()
{
	const float available = available_extent();
	if (available <= FLT_EPSILON) {
		m_behaviour.set_limits(0.0f, 1.0f);
		return;
	}
	const float minimum = std::clamp(m_min_first / available, 0.0f, 1.0f);
	const float maximum = std::clamp(1.0f - m_min_second / available,
		minimum, 1.0f);
	m_behaviour.set_limits(minimum, maximum);
}

void rm_splitter::apply_layout()
{
	if (!m_pparent || !m_pfirst || !m_psecond ||
		m_pfirst->get_parent() != m_pparent || m_psecond->get_parent() != m_pparent)
		return;
	update_limits();
	const rm_rect& area = m_pparent->get_content_area();
	const float available = available_extent();
	const float first_extent = std::round(available * m_behaviour.fraction());
	const float second_extent = std::max(0.0f, available - first_extent);
	const float thickness = m_theme->splitter.thickness;
	if (m_orientation == RM_ORIENT_VERT) {
		m_pfirst->move(rm_vec2(0.0f, 0.0f));
		m_pfirst->resize(first_extent, area.height);
		move(rm_vec2(area.x + first_extent, area.y));
		resize(thickness, area.height);
		m_psecond->move(rm_vec2(first_extent + thickness, 0.0f));
		m_psecond->resize(second_extent, area.height);
	} else {
		m_pfirst->move(rm_vec2(0.0f, 0.0f));
		m_pfirst->resize(area.width, first_extent);
		move(rm_vec2(area.x, area.y + first_extent));
		resize(area.width, thickness);
		m_psecond->move(rm_vec2(0.0f, first_extent + thickness));
		m_psecond->resize(area.width, second_extent);
	}
}

void rm_splitter::on_draw(NVGcontext* pctx)
{
	apply_layout();
	RmDefaultControlPainter::draw_splitter(*pctx,
		{ m_size.x, m_size.y, m_orientation == RM_ORIENT_VERT,
		  is_enabled(), m_elem_flags.is_hovered(), m_behaviour.is_dragging() },
		m_theme->splitter);
	rm_widget::on_draw(pctx);
}

void rm_splitter::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmSplitterStyle& style = m_theme->splitter;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, 0.0f,
		style.focus_ring_width, style.focus_ring);
}

bool rm_splitter::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (event == RM_MOUSE_EVENT_MOVE && m_behaviour.is_dragging() && m_pparent) {
		const rm_rect& area = m_pparent->get_content_area();
		const float pointer = m_orientation == RM_ORIENT_VERT
			? cursor_pos.x - area.x : cursor_pos.y - area.y;
		const float available = available_extent();
		const RmBehaviourUpdate update = m_behaviour.drag_to(
			available > FLT_EPSILON ? pointer / available : 0.0f);
		if (update.state_changed) {
			apply_layout();
			if (is_valid_callback())
				get_callback()(this, m_behaviour.fraction());
		}
		return false;
	}
	if (event != RM_MOUSE_EVENT_CLICK || vk != RM_KEY_LMOUSE)
		return true;
	if (state == RM_KEY_STATE::DOWN && m_bbox.inside(cursor_pos)) {
		const RmBehaviourUpdate update = m_behaviour.begin_drag();
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.end_drag();
		return !update.handled;
	}
	return true;
}

rm_splitter::rm_splitter(rm_widget* p_parent, rm_widget* p_first,
	rm_widget* p_second, RM_ORIENT orientation, float fraction,
	rm_splitter_cb callback, RmThemeRef theme) :
	rm_widget(0, 0, 0, 0, p_parent, "rm_splitter", RM_FLAG_DEFAULT),
	m_pfirst(p_first), m_psecond(p_second),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_orientation(orientation == RM_ORIENT_HORZ ? RM_ORIENT_HORZ : RM_ORIENT_VERT)
{
	set_callback(callback);
	set_fixed_to_viewport(true);
	m_behaviour.set_fraction(fraction);
	set_targets(p_first, p_second);
}

void rm_splitter::set_targets(rm_widget* p_first, rm_widget* p_second)
{
	m_pfirst = p_first;
	m_psecond = p_second;
	if (m_pfirst) {
		m_pfirst->set_min_size(rm_vec2(0.0f, 0.0f));
		m_pfirst->set_max_size(rm_vec2(0.0f, 0.0f));
	}
	if (m_psecond) {
		m_psecond->set_min_size(rm_vec2(0.0f, 0.0f));
		m_psecond->set_max_size(rm_vec2(0.0f, 0.0f));
	}
	apply_layout();
}

void rm_splitter::set_fraction(float fraction, bool notify)
{
	update_limits();
	const RmBehaviourUpdate update = m_behaviour.set_fraction(fraction);
	apply_layout();
	if (notify && update.state_changed && is_valid_callback())
		get_callback()(this, m_behaviour.fraction());
}

void rm_splitter::set_minimum_extents(float first, float second)
{
	m_min_first = std::max(0.0f, first);
	m_min_second = std::max(0.0f, second);
	apply_layout();
}

void rm_splitter::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	apply_layout();
}

rm_tabcontrol::rm_tabcontrol(rm_widget* p_parent, int x, int y, int width, int height,
	rm_tabcontrol_cb p_callback, RmThemeRef theme, RmTabVariant variant,
	RmTabPlacement placement) :
	rm_widget(x, y, width, height, p_parent, "ui_tabcontrol", RM_FLAG_DEFAULT),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_variant(variant), m_placement(placement), m_pclose_callback(nullptr),
	m_close_hovered(RmTabBehaviour::invalid_index),
	m_close_pressed(RmTabBehaviour::invalid_index)
{
	set_callback(p_callback);
}

const RmTabStyle& rm_tabcontrol::tab_style() const
{
	return m_theme->tabs.resolve(m_variant);
}

bool rm_tabcontrol::is_horizontal() const noexcept
{
	return m_placement == RmTabPlacement::top || m_placement == RmTabPlacement::bottom;
}

rm_rect rm_tabcontrol::get_bar_bounds() const
{
	const RmTabStyle& style = tab_style();
	switch (m_placement) {
	case RmTabPlacement::bottom:
		return { 0.f, std::max(0.f, m_size.y - style.tab_height), m_size.x, style.tab_height };
	case RmTabPlacement::left:
		return { 0.f, 0.f, style.vertical_bar_width, m_size.y };
	case RmTabPlacement::right:
		return { std::max(0.f, m_size.x - style.vertical_bar_width), 0.f,
			style.vertical_bar_width, m_size.y };
	case RmTabPlacement::top:
	default:
		return { 0.f, 0.f, m_size.x, style.tab_height };
	}
}

rm_rect rm_tabcontrol::get_page_bounds() const
{
	const RmTabStyle& style = tab_style();
	switch (m_placement) {
	case RmTabPlacement::bottom:
		return { 0.f, 0.f, m_size.x, std::max(0.f, m_size.y - style.tab_height) };
	case RmTabPlacement::left:
		return { style.vertical_bar_width, 0.f,
			std::max(0.f, m_size.x - style.vertical_bar_width), m_size.y };
	case RmTabPlacement::right:
		return { 0.f, 0.f, std::max(0.f, m_size.x - style.vertical_bar_width), m_size.y };
	case RmTabPlacement::top:
	default:
		return { 0.f, style.tab_height, m_size.x,
			std::max(0.f, m_size.y - style.tab_height) };
	}
}

void rm_tabcontrol::rebuild_tab_layout(NVGcontext* pctx)
{
	m_tab_bounds.clear();
	if (m_tabs.empty())
		return;

	const RmTabStyle& style = tab_style();
	const rm_rect bar = get_bar_bounds();
	m_tab_bounds.reserve(m_tabs.size());
	if (!is_horizontal()) {
		float y = bar.y;
		for (size_t i = 0; i < m_tabs.size(); ++i) {
			m_tab_bounds.emplace_back(bar.x, y, bar.width, style.tab_height);
			y += style.tab_height + style.gap;
		}
		return;
	}

	const float gaps = style.gap * static_cast<float>(m_tabs.size() - 1);
	if (style.fill_available_width) {
		const float width = std::max(0.f, bar.width - gaps) /
			static_cast<float>(m_tabs.size());
		float x = bar.x;
		for (size_t i = 0; i < m_tabs.size(); ++i) {
			m_tab_bounds.emplace_back(x, bar.y, width, bar.height);
			x += width + style.gap;
		}
		return;
	}

	pctx->save();
	pctx->setFontFaceId(static_cast<int>(get_font().getValue()));
	pctx->setFontSize(style.font_size);
	std::vector<float> widths;
	widths.reserve(m_tabs.size());
	float total_width = gaps;
	for (const rm_tab_item& item : m_tabs) {
		float bounds[4]{};
		pctx->textBounds(0.f, 0.f, item.get_name(), nullptr, bounds);
		float width = bounds[2] - bounds[0] + style.horizontal_padding * 2.f;
		if (item.is_closable() && !item.is_pinned())
			width += style.close_size + style.horizontal_padding * 0.5f;
		width = std::clamp(width, style.minimum_width, style.maximum_width);
		widths.push_back(width);
		total_width += width;
	}
	pctx->restore();

	if (total_width > bar.width && total_width > gaps) {
		const float scale = std::max(0.f, bar.width - gaps) / (total_width - gaps);
		for (float& width : widths)
			width = std::max(style.minimum_width, width * scale);
	}

	float x = bar.x;
	for (float width : widths) {
		m_tab_bounds.emplace_back(x, bar.y, width, bar.height);
		x += width + style.gap;
	}
}

void rm_tabcontrol::update_pages_geometry()
{
	const rm_rect page_bounds = get_page_bounds();
	for (rm_tab_item& item : m_tabs) {
		item.get_page()->move({ page_bounds.x, page_bounds.y });
		item.get_page()->resize(page_bounds.width, page_bounds.height);
	}
}

void rm_tabcontrol::update_children_active()
{
	const size_t selected = m_behaviour.selected_index();
	for (size_t i = 0; i < m_tabs.size(); ++i) {
		const bool active = i == selected;
		m_tabs[i].get_page()->show(active);
		m_tabs[i].get_page()->set_enabled(active);
	}
}

size_t rm_tabcontrol::hit_test_tab(const rm_vec2& local_cursor) const
{
	for (size_t i = 0; i < m_tab_bounds.size(); ++i) {
		const rm_rect& bounds = m_tab_bounds[i];
		if (local_cursor.x >= bounds.x && local_cursor.x <= bounds.x + bounds.width &&
			local_cursor.y >= bounds.y && local_cursor.y <= bounds.y + bounds.height)
			return i;
	}
	return RmTabBehaviour::invalid_index;
}

bool rm_tabcontrol::hit_test_close(size_t index, const rm_vec2& local_cursor) const
{
	if (index >= m_tabs.size() || index >= m_tab_bounds.size() ||
		!m_tabs[index].is_closable() || m_tabs[index].is_pinned())
		return false;
	const RmTabStyle& style = tab_style();
	const rm_rect& bounds = m_tab_bounds[index];
	const float x = bounds.x + bounds.width - style.horizontal_padding * 0.5f - style.close_size;
	const float y = bounds.y + (bounds.height - style.close_size) * 0.5f;
	return local_cursor.x >= x && local_cursor.x <= x + style.close_size &&
		local_cursor.y >= y && local_cursor.y <= y + style.close_size;
}

void rm_tabcontrol::notify_selection_changed()
{
	const size_t selected = m_behaviour.selected_index();
	if (is_valid_callback() && selected < m_tabs.size())
		get_callback()(this, &m_tabs[selected], selected);
}

rm_widget* rm_tabcontrol::add_tab(const char* p_name, uint32_t id,
	bool closable, bool pinned, void* p_userdata)
{
	const rm_rect page_bounds = get_page_bounds();
	rm_widget* p_page = new rm_widget(page_bounds.x, page_bounds.y,
		page_bounds.width, page_bounds.height, this, "ui_tabpage");
	return add_tab_widget(p_name, id, p_page, closable, pinned, p_userdata);
}

rm_widget* rm_tabcontrol::add_tab_widget(const char* p_name, uint32_t id,
	rm_widget* p_page, bool closable, bool pinned, void* p_userdata)
{
	if (!p_page)
		return nullptr;
	if (id == std::numeric_limits<uint32_t>::max())
		id = static_cast<uint32_t>(m_tabs.size());
	p_page->set_parent(this);
	m_tabs.emplace_back(p_name, id, p_page, closable, pinned, p_userdata);
	m_behaviour.set_count(m_tabs.size());
	update_pages_geometry();
	update_children_active();
	return p_page;
}

bool rm_tabcontrol::remove_tab(size_t index)
{
	if (index >= m_tabs.size())
		return false;
	rm_widget* p_page = m_tabs[index].get_page();
	m_behaviour.remove(index);
	m_tabs.erase(m_tabs.begin() + index);
	remove_child(p_page);
	delete p_page;
	update_pages_geometry();
	update_children_active();
	notify_selection_changed();
	return true;
}

rm_widget* rm_tabcontrol::find_tab(const char* p_name)
{
	const auto found = std::find_if(m_tabs.begin(), m_tabs.end(),
		[p_name](const rm_tab_item& item) {
			return strcmp(item.get_name(), p_name ? p_name : "") == 0;
		});
	return found != m_tabs.end() ? found->get_page() : nullptr;
}

rm_widget* rm_tabcontrol::find_tab(uint32_t id)
{
	const auto found = std::find_if(m_tabs.begin(), m_tabs.end(),
		[id](const rm_tab_item& item) { return item.get_id() == id; });
	return found != m_tabs.end() ? found->get_page() : nullptr;
}

bool rm_tabcontrol::set_selected_index(size_t index)
{
	const RmBehaviourUpdate update = m_behaviour.select(index);
	if (!update.handled)
		return false;
	update_children_active();
	if (update.state_changed)
		notify_selection_changed();
	return true;
}

void rm_tabcontrol::on_draw(NVGcontext* pctx)
{
	const RmTabStyle& style = tab_style();
	const rm_rect bar = get_bar_bounds();
	const rm_rect page = get_page_bounds();
	RmDefaultControlPainter::draw_tab_page(*pctx, page.x, page.y,
		page.width, page.height, style);
	RmDefaultControlPainter::draw_tab_bar(*pctx, bar.x, bar.y,
		bar.width, bar.height, style);
	rebuild_tab_layout(pctx);

	for (size_t i = 0; i < m_tabs.size(); ++i) {
		const rm_rect& bounds = m_tab_bounds[i];
		const RmTabVisual visual{
			bounds.x, bounds.y, bounds.width, bounds.height, get_font(),
			m_tabs[i].get_name(), m_placement, is_enabled(),
			m_behaviour.hovered_index() == i,
			m_behaviour.pressed_index() == i,
			m_behaviour.selected_index() == i,
			m_tabs[i].is_closable() && !m_tabs[i].is_pinned(),
			m_close_hovered == i
		};
		RmDefaultControlPainter::draw_tab(*pctx, visual, style);
	}
	rm_widget::on_draw(pctx);
}

void rm_tabcontrol::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	if (m_tab_bounds.size() != m_tabs.size())
		rebuild_tab_layout(pctx);
	const size_t selected = m_behaviour.selected_index();
	if (selected >= m_tab_bounds.size())
		return;
	const rm_rect& bounds = m_tab_bounds[selected];
	const RmTabStyle& style = tab_style();
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ bounds.x, bounds.y, bounds.width, bounds.height },
		style.corner_radius, style.focus_ring_width, style.focus_ring);
}

void rm_tabcontrol::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != DOWN || m_tabs.empty())
		return;
	RmBehaviourUpdate update;
	if (vk == RM_KEY_HOME)
		update = m_behaviour.select(0);
	else if (vk == RM_KEY_END)
		update = m_behaviour.select(m_tabs.size() - 1);
	else if ((is_horizontal() && vk == RM_KEY_LEFT) || (!is_horizontal() && vk == RM_KEY_UP))
		update = m_behaviour.select_relative(-1);
	else if ((is_horizontal() && vk == RM_KEY_RIGHT) || (!is_horizontal() && vk == RM_KEY_DOWN))
		update = m_behaviour.select_relative(1);
	if (update.state_changed) {
		update_children_active();
		notify_selection_changed();
	}
}

bool rm_tabcontrol::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;
	if (m_tab_bounds.size() != m_tabs.size() && m_proot)
		rebuild_tab_layout(m_proot->get_context());
	const rm_vec2 local_cursor = cursor_to_local(cursor_pos);
	const size_t index = hit_test_tab(local_cursor);

	if (event == RM_MOUSE_EVENT_MOVE) {
		m_close_hovered = hit_test_close(index, local_cursor)
			? index : RmTabBehaviour::invalid_index;
		const RmBehaviourUpdate update = m_behaviour.pointer_move(index);
		return !update.handled;
	}
	if (event != RM_MOUSE_EVENT_CLICK)
		return true;

	if (state == DOWN) {
		if (hit_test_close(index, local_cursor)) {
			m_close_pressed = index;
			if (get_root())
				get_root()->capture_pointer(this);
			return false;
		}
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}

	if (state == UP && m_close_pressed != RmTabBehaviour::invalid_index) {
		const size_t close_index = m_close_pressed;
		m_close_pressed = RmTabBehaviour::invalid_index;
		if (close_index == index && hit_test_close(index, local_cursor)) {
			const bool allow_close = !m_pclose_callback ||
				m_pclose_callback(this, &m_tabs[index], index);
			if (allow_close)
				remove_tab(index);
		}
		return false;
	}

	if (state == UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (update.activated) {
			update_children_active();
			notify_selection_changed();
		}
		return !update.handled;
	}
	return true;
}

void rm_tabcontrol::resize(float width, float height)
{
	rm_widget::resize(width, height);
	update_pages_geometry();
}

rm_treeview::rm_treeview(int x, int y, int width, int height,
	rm_widget* p_parent, rm_treeview_cb cb, RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_treeview",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_OPAQUE),
	m_selected(nullptr),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_expander_pressed(RmTreeViewBehaviour::invalid_index)
{
	set_callback(std::move(cb));
}

void rm_treeview::append_visible(rm_tree_node* p_node, size_t depth)
{
	if (!p_node)
		return;
	m_visible_rows.push_back({ p_node, depth });
	if (!p_node->expanded)
		return;
	for (rm_tree_node* p_child : p_node->children)
		append_visible(p_child, depth + 1);
}

void rm_treeview::rebuild_visible_rows()
{
	m_visible_rows.clear();
	for (rm_tree_node* p_root : m_roots)
		append_visible(p_root, 0);
	m_behaviour.set_count(m_visible_rows.size());

	m_behaviour.clear_selection();
	if (!m_selected)
		return;
	const auto selected = std::find_if(m_visible_rows.begin(), m_visible_rows.end(),
		[this](const VisibleRow& row) { return row.node == m_selected; });
	if (selected != m_visible_rows.end())
		m_behaviour.select(static_cast<size_t>(selected - m_visible_rows.begin()));
}

size_t rm_treeview::hit_test_row(const rm_vec2& cursor_pos) const
{
	if (!m_bbox.inside(cursor_pos) || m_theme->treeview.row_height <= 0.0f)
		return RmTreeViewBehaviour::invalid_index;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;
	if (local_y < 0.0f)
		return RmTreeViewBehaviour::invalid_index;
	const size_t index = static_cast<size_t>(
		local_y / m_theme->treeview.row_height);
	return index < m_visible_rows.size()
		? index : RmTreeViewBehaviour::invalid_index;
}

bool rm_treeview::hit_test_expander(const rm_vec2& cursor_pos, size_t index) const
{
	if (index >= m_visible_rows.size() ||
		m_visible_rows[index].node->children.empty())
		return false;
	const float local_x = cursor_pos.x - m_pos_of_parent.x;
	const float branch_x = m_theme->treeview.horizontal_padding +
		static_cast<float>(m_visible_rows[index].depth) * m_theme->treeview.indent;
	return local_x >= branch_x && local_x < branch_x + m_theme->treeview.indent;
}

void rm_treeview::select_index(size_t index, bool notify)
{
	if (index >= m_visible_rows.size())
		return;
	const rm_tree_node* previous = m_selected;
	const RmBehaviourUpdate update = m_behaviour.select(index);
	if (!update.handled)
		return;
	m_selected = m_visible_rows[index].node;
	if (notify && previous != m_selected && is_valid_callback())
		get_callback()(this, m_selected);
}

void rm_treeview::toggle_index(size_t index)
{
	if (index >= m_visible_rows.size())
		return;
	rm_tree_node* p_node = m_visible_rows[index].node;
	if (p_node->children.empty())
		return;
	set_expanded(p_node, !p_node->expanded);
}

bool rm_treeview::contains_node(const rm_tree_node* p_node) const
{
	if (!p_node)
		return false;
	const rm_tree_node* p_root = p_node;
	while (p_root->parent)
		p_root = p_root->parent;
	return std::find(m_roots.begin(), m_roots.end(), p_root) != m_roots.end();
}

bool rm_treeview::set_expanded(rm_tree_node* p_node, bool expanded)
{
	if (!contains_node(p_node) || p_node->children.empty() ||
		p_node->expanded == expanded)
		return false;
	p_node->expanded = expanded;
	rebuild_visible_rows();
	root_update();
	return true;
}

void rm_treeview::on_draw(NVGcontext* pctx)
{
	rebuild_visible_rows();
	const RmTreeViewStyle& style = m_theme->treeview;
	RmDefaultControlPainter::draw_treeview_surface(*pctx,
		{ m_size.x, m_size.y, is_enabled() }, style);

	const size_t hovered = m_behaviour.hovered_index();
	const size_t pressed = m_behaviour.pressed_index();
	for (size_t index = 0; index < m_visible_rows.size(); ++index) {
		const float y = static_cast<float>(index) * style.row_height;
		if (y >= m_size.y)
			break;
		const VisibleRow& row = m_visible_rows[index];
		const RmTreeViewRowVisual visual{
			y,
			m_size.x,
			get_font(),
			row.node->name.c_str(),
			get_imagelist(),
			row.node->current_icon(),
			row.depth,
			is_enabled(),
			hovered == index,
			pressed == index,
			row.node == m_selected,
			!row.node->children.empty(),
			row.node->expanded
		};
		RmDefaultControlPainter::draw_treeview_row(*pctx, visual, style);
	}
	if (hovered < m_visible_rows.size()) {
		const VisibleRow& row = m_visible_rows[hovered];
		if (!row.node->tooltip.empty()) {
			const float anchor_x = style.horizontal_padding +
				(static_cast<float>(row.depth) + 1.0f) * style.indent;
			const float anchor_y = (static_cast<float>(hovered) + 1.0f) *
				style.row_height;
			RmDefaultControlPainter::draw_treeview_tooltip(*pctx,
				{ anchor_x, anchor_y, m_size.x, m_size.y, get_font(),
					row.node->tooltip.c_str() }, style);
		}
	}
	rm_widget::on_draw(pctx);
}

void rm_treeview::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmTreeViewStyle& style = m_theme->treeview;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_treeview::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != RM_KEY_STATE::DOWN && state != RM_KEY_STATE::REPEAT)
		return;
	rebuild_visible_rows();
	if (m_visible_rows.empty())
		return;

	if (vk == RM_KEY_UP || vk == RM_KEY_DOWN) {
		const rm_tree_node* previous = m_selected;
		const RmBehaviourUpdate update =
			m_behaviour.select_relative(vk == RM_KEY_UP ? -1 : 1);
		if (update.handled) {
			m_selected = m_visible_rows[m_behaviour.selected_index()].node;
			if (previous != m_selected && is_valid_callback())
				get_callback()(this, m_selected);
		}
		return;
	}

	size_t selected = m_behaviour.selected_index();
	if (selected == RmTreeViewBehaviour::invalid_index) {
		select_index(0, true);
		return;
	}
	rm_tree_node* p_node = m_visible_rows[selected].node;
	switch (vk) {
	case RM_KEY_RIGHT:
		if (!p_node->children.empty()) {
			if (!p_node->expanded)
				toggle_index(selected);
			else if (selected + 1 < m_visible_rows.size())
				select_index(selected + 1, true);
		}
		break;
	case RM_KEY_LEFT:
		if (p_node->expanded)
			toggle_index(selected);
		else if (p_node->parent) {
			const auto parent = std::find_if(m_visible_rows.begin(),
				m_visible_rows.end(), [p_node](const VisibleRow& row) {
					return row.node == p_node->parent;
				});
			if (parent != m_visible_rows.end())
				select_index(static_cast<size_t>(parent - m_visible_rows.begin()), true);
		}
		break;
	case RM_KEY_ENTER:
	case RM_KEY_SPACE:
		toggle_index(selected);
		break;
	case RM_KEY_HOME:
		select_index(0, true);
		break;
	case RM_KEY_END:
		select_index(m_visible_rows.size() - 1, true);
		break;
	default:
		break;
	}
}

bool rm_treeview::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(vk);
	RM_UNUSED(delta);
	rebuild_visible_rows();
	const size_t index = hit_test_row(cursor_pos);
	if (event == RM_MOUSE_EVENT_MOVE) {
		const RmBehaviourUpdate update = m_behaviour.pointer_move(index);
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled) {
			m_expander_pressed = hit_test_expander(cursor_pos, index)
				? index : RmTreeViewBehaviour::invalid_index;
			get_root()->capture_pointer(this);
		}
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
		const size_t expander = m_expander_pressed;
		m_expander_pressed = RmTreeViewBehaviour::invalid_index;
		const rm_tree_node* previous = m_selected;
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (update.activated) {
			m_selected = m_visible_rows[index].node;
			if (expander == index && hit_test_expander(cursor_pos, index))
				toggle_index(index);
			if (previous != m_selected && is_valid_callback())
				get_callback()(this, m_selected);
		}
		return !update.handled;
	}
	return true;
}

rm_propertyview::rm_propertyview(rm_widget* p_parent, int x, int y,
	int width, int height, rm_property_changed_cb changed, RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_propertyview",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_OPAQUE),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_callback(changed);
}

void rm_propertyview::rebuild_visible_rows()
{
	m_visible_rows.clear();
	float y = 0.0f;
	const RmPropertyViewStyle& style = m_theme->propertyview;
	for (const auto& p_property : m_properties) {
		m_visible_rows.push_back({ nullptr, p_property.get(), y, style.row_height });
		y += style.row_height;
	}
	for (const auto& p_group : m_groups) {
		m_visible_rows.push_back({ p_group.get(), nullptr, y, style.group_height });
		y += style.group_height;
		if (!p_group->m_expanded)
			continue;
		for (const auto& p_property : p_group->m_properties) {
			m_visible_rows.push_back({ nullptr, p_property.get(), y, style.row_height });
			y += style.row_height;
		}
	}
	m_behaviour.set_count(m_visible_rows.size());
}

size_t rm_propertyview::hit_test_row(const rm_vec2& cursor_pos) const
{
	if (!m_bbox.inside(cursor_pos))
		return RmPropertyViewBehaviour::invalid_index;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;
	for (size_t index = 0; index < m_visible_rows.size(); ++index) {
		const VisibleRow& row = m_visible_rows[index];
		if (local_y >= row.y && local_y < row.y + row.height)
			return index;
	}
	return RmPropertyViewBehaviour::invalid_index;
}

float rm_propertyview::value_column_x() const
{
	return m_size.x * m_theme->propertyview.name_column_ratio;
}

size_t rm_propertyview::hit_test_choice(const rm_vec2& cursor_pos) const
{
	if (!m_choice_open || !m_pediting ||
		m_pediting->m_type != RmPropertyType::choice)
		return RmPropertyViewBehaviour::invalid_index;
	const auto row = std::find_if(m_visible_rows.begin(), m_visible_rows.end(),
		[this](const VisibleRow& item) { return item.property == m_pediting; });
	if (row == m_visible_rows.end())
		return RmPropertyViewBehaviour::invalid_index;
	const float local_x = cursor_pos.x - m_pos_of_parent.x;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;
	const float popup_y = row->y + row->height;
	const float item_height = m_theme->propertyview.row_height;
	if (local_x < value_column_x() || local_x >= m_size.x || local_y < popup_y)
		return RmPropertyViewBehaviour::invalid_index;
	const size_t index = static_cast<size_t>((local_y - popup_y) / item_height);
	return index < m_pediting->m_choices.size()
		? index : RmPropertyViewBehaviour::invalid_index;
}

void rm_propertyview::begin_edit(rm_property* p_property)
{
	if (!p_property)
		return;
	m_pediting = p_property;
	m_edit_buffer = p_property->m_value;
	m_choice_open = p_property->m_type == RmPropertyType::choice;
	m_choice_hovered = RmPropertyViewBehaviour::invalid_index;
	if (m_choice_open) {
		const auto selected = std::find(p_property->m_choices.begin(),
			p_property->m_choices.end(), p_property->m_value);
		if (selected != p_property->m_choices.end())
			m_choice_hovered = static_cast<size_t>(
				selected - p_property->m_choices.begin());
	}
}

bool rm_propertyview::commit_edit()
{
	if (!m_pediting)
		return false;
	rm_property* p_property = m_pediting;
	const std::string previous = p_property->m_value;
	p_property->m_value = m_edit_buffer;
	p_property->m_error = on_validate_property(*p_property, p_property->m_value);
	m_pediting = nullptr;
	m_choice_open = false;
	m_choice_hovered = RmPropertyViewBehaviour::invalid_index;
	const bool changed = previous != p_property->m_value;
	if (changed && p_property->m_error.empty() && is_valid_callback())
		get_callback()(this, p_property);
	return p_property->m_error.empty();
}

void rm_propertyview::cancel_edit()
{
	m_pediting = nullptr;
	m_edit_buffer.clear();
	m_choice_open = false;
	m_choice_hovered = RmPropertyViewBehaviour::invalid_index;
}

void rm_propertyview::choose_value(size_t index)
{
	if (!m_pediting || index >= m_pediting->m_choices.size())
		return;
	m_edit_buffer = m_pediting->m_choices[index];
	commit_edit();
}

std::string rm_propertyview::validate_builtin(const rm_property& property,
	std::string_view value) const
{
	if (property.m_type == RmPropertyType::text)
		return {};
	const std::string text(value);
	if (property.m_type == RmPropertyType::integer) {
		char* p_end = nullptr;
		errno = 0;
		std::strtoll(text.c_str(), &p_end, 10);
		if (text.empty() || errno == ERANGE || !p_end || *p_end != '\0')
			return "Expected an integer value";
		return {};
	}
	if (property.m_type == RmPropertyType::real) {
		char* p_end = nullptr;
		errno = 0;
		std::strtod(text.c_str(), &p_end);
		if (text.empty() || errno == ERANGE || !p_end || *p_end != '\0')
			return "Expected a real value";
		return {};
	}
	if (property.m_type == RmPropertyType::boolean) {
		std::string normalized = text;
		std::transform(normalized.begin(), normalized.end(), normalized.begin(),
			[](unsigned char value) { return static_cast<char>(std::tolower(value)); });
		if (normalized != "true" && normalized != "false" &&
			normalized != "0" && normalized != "1")
			return "Expected true, false, 0 or 1";
		return {};
	}
	if (property.m_type == RmPropertyType::choice &&
		std::find(property.m_choices.begin(), property.m_choices.end(), text) ==
		property.m_choices.end())
		return "Value is not in the allowed list";
	return {};
}

std::string rm_propertyview::on_validate_property(
	const rm_property& property, const std::string& value) const
{
	std::string error = validate_builtin(property, value);
	if (error.empty() && m_validation_callback)
		error = m_validation_callback(const_cast<rm_propertyview*>(this),
			&property, value.c_str());
	return error;
}

rm_property_group* rm_propertyview::add_group(const char* p_name, uint32_t id)
{
	if (id == std::numeric_limits<uint32_t>::max())
		id = m_next_item_id++;
	else if (id >= m_next_item_id)
		m_next_item_id = id + 1;
	auto p_group = std::make_unique<rm_property_group>(id,
		p_name ? p_name : "");
	rm_property_group* p_result = p_group.get();
	m_groups.push_back(std::move(p_group));
	rebuild_visible_rows();
	return p_result;
}

rm_property* rm_propertyview::add_property(const char* p_name,
	const char* p_value, RmPropertyType type, rm_property_group* p_group,
	uint32_t id, void* p_userdata)
{
	if (p_group && std::find_if(m_groups.begin(), m_groups.end(),
		[p_group](const auto& item) { return item.get() == p_group; }) == m_groups.end())
		return nullptr;
	if (id == std::numeric_limits<uint32_t>::max())
		id = m_next_item_id++;
	else if (id >= m_next_item_id)
		m_next_item_id = id + 1;
	auto p_property = std::make_unique<rm_property>(id, p_name ? p_name : "",
		p_value ? p_value : "", type, p_group, p_userdata);
	rm_property* p_result = p_property.get();
	if (p_group)
		p_group->m_properties.push_back(std::move(p_property));
	else
		m_properties.push_back(std::move(p_property));
	p_result->m_error = on_validate_property(*p_result, p_result->m_value);
	rebuild_visible_rows();
	return p_result;
}

rm_property* rm_propertyview::add_choice_property(const char* p_name,
	const char* p_value, std::vector<std::string> choices,
	rm_property_group* p_group, uint32_t id, void* p_userdata)
{
	rm_property* p_property = add_property(p_name, p_value,
		RmPropertyType::choice, p_group, id, p_userdata);
	if (!p_property)
		return nullptr;
	p_property->m_choices = std::move(choices);
	p_property->m_error = on_validate_property(*p_property, p_property->m_value);
	return p_property;
}

bool rm_propertyview::set_group_expanded(rm_property_group* p_group,
	bool expanded)
{
	const auto found = std::find_if(m_groups.begin(), m_groups.end(),
		[p_group](const auto& item) { return item.get() == p_group; });
	if (found == m_groups.end() || p_group->m_expanded == expanded)
		return false;
	p_group->m_expanded = expanded;
	rebuild_visible_rows();
	return true;
}

bool rm_propertyview::set_property_value(rm_property* p_property,
	std::string value, bool notify)
{
	if (!p_property)
		return false;
	const std::string previous = p_property->m_value;
	p_property->m_value = std::move(value);
	p_property->m_error = on_validate_property(*p_property, p_property->m_value);
	if (notify && previous != p_property->m_value && p_property->m_error.empty() &&
		is_valid_callback())
		get_callback()(this, p_property);
	return p_property->m_error.empty();
}

rm_property* rm_propertyview::get_selected_property() const noexcept
{
	const size_t selected = m_behaviour.selected_index();
	return selected < m_visible_rows.size() ? m_visible_rows[selected].property : nullptr;
}

void rm_propertyview::on_draw(NVGcontext* pctx)
{
	rebuild_visible_rows();
	const RmPropertyViewStyle& style = m_theme->propertyview;
	RmDefaultControlPainter::draw_propertyview_surface(*pctx,
		{ m_size.x, m_size.y, is_enabled() }, style);
	for (size_t index = 0; index < m_visible_rows.size(); ++index) {
		const VisibleRow& row = m_visible_rows[index];
		if (row.y >= m_size.y)
			break;
		if (row.group) {
			RmDefaultControlPainter::draw_propertyview_group(*pctx,
				{ row.y, m_size.x, get_font(), row.group->m_name.c_str(),
					row.group->m_expanded, is_enabled(),
					m_behaviour.hovered_index() == index }, style);
			continue;
		}
		const bool editing = row.property == m_pediting;
		RmDefaultControlPainter::draw_propertyview_row(*pctx,
			{ row.y, m_size.x, get_font(), row.property->m_name.c_str(),
				editing ? m_edit_buffer.c_str() : row.property->m_value.c_str(),
				row.property->m_error.c_str(), is_enabled(),
				m_behaviour.hovered_index() == index,
				m_behaviour.pressed_index() == index,
				m_behaviour.selected_index() == index, editing,
				row.property->m_type == RmPropertyType::choice }, style);
	}
	if (m_choice_open && m_pediting) {
		const auto row = std::find_if(m_visible_rows.begin(), m_visible_rows.end(),
			[this](const VisibleRow& item) { return item.property == m_pediting; });
		if (row != m_visible_rows.end()) {
			const float x = value_column_x();
			float y = row->y + row->height;
			for (size_t index = 0; index < m_pediting->m_choices.size(); ++index) {
				if (y + style.row_height > m_size.y)
					break;
				RmDefaultControlPainter::draw_propertyview_choice(*pctx,
					{ x, y, m_size.x - x, style.row_height, get_font(),
						m_pediting->m_choices[index].c_str(),
						m_choice_hovered == index,
						m_edit_buffer == m_pediting->m_choices[index] }, style);
				y += style.row_height;
			}
		}
	}
	rm_widget::on_draw(pctx);
}

void rm_propertyview::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmPropertyViewStyle& style = m_theme->propertyview;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_propertyview::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != RM_KEY_STATE::DOWN && state != RM_KEY_STATE::REPEAT)
		return;
	if (m_pediting) {
		if (vk == RM_KEY_ESCAPE) {
			cancel_edit();
			return;
		}
		if (m_choice_open) {
			if (vk == RM_KEY_UP || vk == RM_KEY_DOWN) {
				const int delta = vk == RM_KEY_UP ? -1 : 1;
				const int current = m_choice_hovered == RmPropertyViewBehaviour::invalid_index
					? (delta > 0 ? -1 : static_cast<int>(m_pediting->m_choices.size()))
					: static_cast<int>(m_choice_hovered);
				if (!m_pediting->m_choices.empty())
					m_choice_hovered = static_cast<size_t>(std::clamp(current + delta,
						0, static_cast<int>(m_pediting->m_choices.size()) - 1));
			}
			else if (vk == RM_KEY_ENTER &&
				m_choice_hovered != RmPropertyViewBehaviour::invalid_index)
				choose_value(m_choice_hovered);
			return;
		}
		if (vk == RM_KEY_BACKSPACE && !m_edit_buffer.empty()) {
			size_t start = m_edit_buffer.size() - 1;
			while (start > 0 && (static_cast<unsigned char>(m_edit_buffer[start]) & 0xc0u) == 0x80u)
				--start;
			m_edit_buffer.erase(start);
			m_pediting->m_error = on_validate_property(*m_pediting, m_edit_buffer);
		}
		else if (vk == RM_KEY_ENTER)
			commit_edit();
		return;
	}

	if (vk == RM_KEY_UP || vk == RM_KEY_DOWN) {
		m_behaviour.select_relative(vk == RM_KEY_UP ? -1 : 1);
		return;
	}
	if (vk == RM_KEY_ENTER) {
		const size_t selected = m_behaviour.selected_index();
		if (selected >= m_visible_rows.size())
			return;
		VisibleRow& row = m_visible_rows[selected];
		if (row.group)
			set_group_expanded(row.group, !row.group->m_expanded);
		else
			begin_edit(row.property);
	}
}

void rm_propertyview::on_text_input(int sym)
{
	if (!m_pediting || m_choice_open || sym < 32 || sym > 0x10ffff)
		return;
	const uint32_t codepoint = static_cast<uint32_t>(sym);
	if (codepoint < 0x80u)
		m_edit_buffer.push_back(static_cast<char>(codepoint));
	else if (codepoint < 0x800u) {
		m_edit_buffer.push_back(static_cast<char>(0xc0u | (codepoint >> 6)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
	}
	else if (codepoint < 0x10000u) {
		m_edit_buffer.push_back(static_cast<char>(0xe0u | (codepoint >> 12)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3fu)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
	}
	else {
		m_edit_buffer.push_back(static_cast<char>(0xf0u | (codepoint >> 18)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | ((codepoint >> 12) & 0x3fu)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3fu)));
		m_edit_buffer.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
	}
	m_pediting->m_error = on_validate_property(*m_pediting, m_edit_buffer);
}

bool rm_propertyview::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(vk);
	RM_UNUSED(delta);
	rebuild_visible_rows();
	const size_t choice = hit_test_choice(cursor_pos);
	if (m_choice_open) {
		if (event == RM_MOUSE_EVENT_MOVE) {
			m_choice_hovered = choice;
			return choice == RmPropertyViewBehaviour::invalid_index;
		}
		if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP &&
			choice != RmPropertyViewBehaviour::invalid_index) {
			choose_value(choice);
			return false;
		}
	}

	const size_t index = hit_test_row(cursor_pos);
	if (event == RM_MOUSE_EVENT_MOVE) {
		const RmBehaviourUpdate update = m_behaviour.pointer_move(index);
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled)
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (!update.activated || index >= m_visible_rows.size())
			return !update.handled;
		VisibleRow& row = m_visible_rows[index];
		if (row.group)
			set_group_expanded(row.group, !row.group->m_expanded);
		else if (cursor_pos.x - m_pos_of_parent.x >= value_column_x())
			begin_edit(row.property);
		else if (m_pediting && m_pediting != row.property)
			commit_edit();
		return false;
	}
	return true;
}

void rm_output_text::on_draw(NVGcontext* pctx)
{
	const RmOutputTextStyle& style = m_theme->output_text;
	RmDefaultControlPainter::draw_output_text_surface(*pctx,
		{ m_size.x, m_size.y, is_enabled() }, style);
	float y = style.vertical_padding;
	for (const std::string& line : m_behaviour.lines()) {
		if (y + style.line_height > m_size.y)
			break;
		RmDefaultControlPainter::draw_output_text_line(*pctx,
			{ y, get_font(), line.c_str(), is_enabled() }, style);
		y += style.line_height;
	}
	rm_widget::on_draw(pctx);
}

void rm_output_text::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmOutputTextStyle& style = m_theme->output_text;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

rm_output_text::rm_output_text(rm_widget* p_parent, float x, float y,
	float width, float height, size_t num_lines, RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_outputtext",
		RM_FLAG_DEFAULT | RM_FLAG_OPAQUE),
	m_behaviour(num_lines),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
}

void rm_output_text::printf(const char* pformat, ...)
{
	if (!pformat)
		return;
	va_list args;
	va_start(args, pformat);
	va_list measure_args;
	va_copy(measure_args, args);
	const int length = std::vsnprintf(nullptr, 0, pformat, measure_args);
	va_end(measure_args);
	if (length > 0) {
		std::vector<char> buffer(static_cast<size_t>(length) + 1);
		std::vsnprintf(buffer.data(), buffer.size(), pformat, args);
		m_behaviour.append_text(std::string(buffer.data(), static_cast<size_t>(length)));
	}
	va_end(args);
}

void rm_number_input::on_draw(NVGcontext* pctx)
{
	char text[64] = {};
	if (m_behaviour.type() == RmNumberInputType::integer)
		std::snprintf(text, sizeof(text), "%.0f", m_behaviour.value());
	else
		std::snprintf(text, sizeof(text), "%.6g", m_behaviour.value());

	const RmNumberInputPart hovered = m_behaviour.hovered_part();
	const RmNumberInputPart pressed = m_behaviour.pressed_part();
	const RmNumberInputVisual visual{
		m_size.x,
		m_size.y,
		get_font(),
		text,
		m_button_placement,
		is_enabled(),
		m_elem_flags.is_hovered(),
		hovered == RmNumberInputPart::increment,
		pressed == RmNumberInputPart::increment,
		hovered == RmNumberInputPart::decrement,
		pressed == RmNumberInputPart::decrement
	};
	RmDefaultControlPainter::draw_number_input(
		*pctx, visual, m_theme->number_input);
	rm_widget::on_draw(pctx);
}

void rm_number_input::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmNumberInputStyle& style = m_theme->number_input;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

RmNumberInputPart rm_number_input::hit_test_part(const rm_vec2& cursor_pos) const
{
	if (!m_bbox.inside(cursor_pos))
		return RmNumberInputPart::none;
	const float local_x = cursor_pos.x - m_pos_of_parent.x;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;
	const RmNumberInputGeometry geometry = rm_number_input_geometry(
		m_size.x, m_size.y, m_theme->number_input.button_width,
		m_button_placement);
	return geometry.hit_test(local_x, local_y);
}

void rm_number_input::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != RM_KEY_STATE::DOWN && state != RM_KEY_STATE::REPEAT)
		return;
	switch (vk) {
	case RM_KEY_UP:
		m_behaviour.step_by(1);
		break;
	case RM_KEY_DOWN:
		m_behaviour.step_by(-1);
		break;
	case RM_KEY_HOME:
		m_behaviour.set_value(m_behaviour.minimum());
		break;
	case RM_KEY_END:
		m_behaviour.set_value(m_behaviour.maximum());
		break;
	default:
		break;
	}
}

bool rm_number_input::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(vk);
	RM_UNUSED(delta);
	const RmNumberInputPart part = hit_test_part(cursor_pos);
	if (event == RM_MOUSE_EVENT_MOVE) {
		const RmBehaviourUpdate update = m_behaviour.pointer_move(part);
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(part);
		if (update.handled)
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(part);
		return !update.handled;
	}
	return true;
}

rm_number_input::rm_number_input(rm_widget* p_parent, int x, int y, int width, int height,
	RmNumberInputType type, float value, float step, float minval, float maxval,
	RmThemeRef theme, RmNumberInputButtonPlacement button_placement) :
	rm_widget(x, y, width, height, p_parent, "ui_number_input",
		RM_FLAG_DEFAULT | RM_FLAG_OPAQUE),
	m_behaviour(type, value, step, minval, maxval),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_button_placement(button_placement)
{
}



rm_menu::rm_menu(rm_widget* p_parent, rm_menu_fn p_callback, RmThemeRef theme) :
	rm_widget(0.f, 0.f, p_parent ? p_parent->get_size().x : 0.f, 1.f, p_parent,
		"ui_menu", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()),
	m_itemid(0), m_menuid(0), m_level(0), m_text_width(0.f),
	m_proot_menu(this), m_separator(false)
{
	assert(p_parent && "p_parent was nullptr!");
	set_callback(p_callback);
	set_zindex(1000);
	set_min_size({ 0.f, 0.f });
	set_max_size({ 0.f, 0.f });
	m_elem_flags.set_bit(RM_FLAG_DISABLE_SCISSOR);
	resize(p_parent->get_size().x, menu_style().bar_height);
	m_behaviour.set_count(0);
}

rm_menu::rm_menu(rm_menu* p_parent, const char* p_name, uint32_t menuid,
	uint32_t itemid, bool separator) :
	rm_widget(0.f, 0.f, 1.f, 1.f, p_parent, "ui_menu",
		RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_OPAQUE),
	m_theme(p_parent->m_theme), m_itemid(itemid), m_menuid(menuid),
	m_level(p_parent->m_level + 1), m_text(p_name ? p_name : ""),
	m_text_width(0.f), m_proot_menu(p_parent->m_proot_menu),
	m_separator(separator)
{
	set_zindex(1000 + static_cast<int>(m_level));
	set_min_size({ 0.f, 0.f });
	set_max_size({ 0.f, 0.f });
	m_elem_flags.set_bit(RM_FLAG_DISABLE_SCISSOR);
	if (m_proot)
		m_text_width = m_proot->get_text_width(m_text.c_str(), get_font());
	m_behaviour.set_count(0);
	hide();
}

rm_menu* rm_menu::create_submenu(const char* p_name,
	uint32_t menuid, uint32_t itemid)
{
	rm_menu* p_menu = new (std::nothrow)rm_menu(
		this, p_name, menuid, itemid, p_name == nullptr);
	if (!p_menu)
		return nullptr;
	m_behaviour.set_count(get_num_submenus());
	update_popup_geometry();
	return p_menu;
}

size_t rm_menu::get_num_submenus() const
{
	return m_childs.size();
}

rm_menu* rm_menu::get_submenu(size_t index)
{
	if (index >= m_childs.size())
		return nullptr;
	rm_widget* p_child = m_childs[index];
	return p_child && p_child->classname_is("ui_menu")
		? static_cast<rm_menu*>(p_child) : nullptr;
}

void rm_menu::rebuild_item_layout()
{
	m_item_bounds.clear();
	m_item_bounds.reserve(get_num_submenus());
	const RmMenuStyle& style = menu_style();
	if (is_root_menu()) {
		float x = 0.f;
		for (size_t i = 0; i < get_num_submenus(); ++i) {
			rm_menu* p_item = get_submenu(i);
			const float width = p_item->m_text_width + style.horizontal_padding * 2.f;
			m_item_bounds.emplace_back(x, 0.f, width, style.bar_height);
			x += width;
		}
		return;
	}

	float y = style.vertical_padding;
	for (size_t i = 0; i < get_num_submenus(); ++i) {
		rm_menu* p_item = get_submenu(i);
		const float height = p_item->is_separator()
			? style.vertical_padding * 2.f + style.separator_thickness
			: style.item_height;
		m_item_bounds.emplace_back(style.vertical_padding, y,
			std::max(0.f, m_size.x - style.vertical_padding * 2.f), height);
		y += height;
	}
}

void rm_menu::update_popup_geometry()
{
	const RmMenuStyle& style = menu_style();
	if (is_root_menu()) {
		if (m_pparent)
			resize(m_pparent->get_size().x, style.bar_height);
	}
	else {
		float width = style.popup_minimum_width;
		float height = style.vertical_padding * 2.f;
		for (size_t i = 0; i < get_num_submenus(); ++i) {
			rm_menu* p_item = get_submenu(i);
			width = std::max(width, p_item->m_text_width +
				style.horizontal_padding * 3.f + style.submenu_indicator_size * 2.f);
			height += p_item->is_separator()
				? style.vertical_padding * 2.f + style.separator_thickness
				: style.item_height;
		}
		resize(width, height);
	}
	for (size_t i = 0; i < get_num_submenus(); ++i)
		get_submenu(i)->update_popup_geometry();
	rebuild_item_layout();
}

size_t rm_menu::hit_test_item(const rm_vec2& local_cursor) const
{
	for (size_t i = 0; i < m_item_bounds.size(); ++i) {
		const rm_rect& bounds = m_item_bounds[i];
		if (local_cursor.x >= bounds.x && local_cursor.x <= bounds.x + bounds.width &&
			local_cursor.y >= bounds.y && local_cursor.y <= bounds.y + bounds.height)
			return i;
	}
	return RmMenuBehaviour::invalid_index;
}

size_t rm_menu::find_selectable(size_t start, int direction) const
{
	if (m_childs.empty())
		return RmMenuBehaviour::invalid_index;
	const size_t count = m_childs.size();
	size_t index = start == RmMenuBehaviour::invalid_index
		? (direction > 0 ? count - 1 : 0) : start;
	for (size_t attempt = 0; attempt < count; ++attempt) {
		index = direction > 0 ? (index + 1) % count : (index + count - 1) % count;
		const rm_menu* p_item = static_cast<const rm_menu*>(m_childs[index]);
		if (!p_item->is_separator())
			return index;
	}
	return RmMenuBehaviour::invalid_index;
}

void rm_menu::open_submenu(size_t index)
{
	if (index >= get_num_submenus())
		return;
	rm_menu* p_menu = get_submenu(index);
	if (!p_menu || p_menu->is_separator() || !p_menu->has_submenus())
		return;
	close_submenus();
	m_behaviour.open(index);
	update_popup_geometry();
	const rm_rect& bounds = m_item_bounds[index];
	if (is_root_menu())
		p_menu->move({ bounds.x, menu_style().bar_height });
	else
		p_menu->move({ m_size.x, bounds.y - menu_style().vertical_padding });
	p_menu->show();
}

void rm_menu::close_submenus()
{
	for (size_t i = 0; i < get_num_submenus(); ++i) {
		rm_menu* p_menu = get_submenu(i);
		p_menu->close_tree();
		p_menu->hide();
	}
	m_behaviour.close();
}

void rm_menu::close_tree()
{
	close_submenus();
	if (!is_root_menu())
		hide();
}

bool rm_menu::contains_visible_popup(const rm_vec2& local_cursor) const
{
	for (rm_widget* p_child_widget : m_childs) {
		rm_menu* p_child = static_cast<rm_menu*>(p_child_widget);
		if (!p_child->is_visible())
			continue;
		const rm_vec2& pos = p_child->m_pos_of_parent;
		const rm_vec2& size = p_child->m_size;
		const bool inside = local_cursor.x >= pos.x && local_cursor.x <= pos.x + size.x &&
			local_cursor.y >= pos.y && local_cursor.y <= pos.y + size.y;
		const rm_vec2 child_cursor(local_cursor.x - pos.x, local_cursor.y - pos.y);
		if (inside || p_child->contains_visible_popup(child_cursor))
			return true;
	}
	return false;
}

void rm_menu::activate_item(size_t index)
{
	if (index >= get_num_submenus())
		return;
	rm_menu* p_item = get_submenu(index);
	if (!p_item || p_item->is_separator())
		return;
	if (p_item->has_submenus()) {
		open_submenu(index);
		return;
	}
	if (m_proot_menu->is_valid_callback())
		m_proot_menu->get_callback()(m_proot_menu, p_item->m_menuid, p_item->m_itemid);
	m_proot_menu->close_tree();
}

void rm_menu::on_draw(NVGcontext* pctx)
{
	update_popup_geometry();
	const RmMenuStyle& style = menu_style();
	RmDefaultControlPainter::draw_menu_surface(*pctx,
		{ m_size.x, m_size.y, !is_root_menu() }, style);
	for (size_t i = 0; i < get_num_submenus(); ++i) {
		rm_menu* p_item = get_submenu(i);
		const rm_rect& bounds = m_item_bounds[i];
		const RmMenuItemVisual visual{
			bounds.x, bounds.y, bounds.width, bounds.height, get_font(),
			p_item->m_text.c_str(), is_root_menu(), is_enabled(),
			m_behaviour.highlighted_index() == i,
			m_behaviour.pressed_index() == i,
			m_behaviour.opened_index() == i,
			p_item->is_separator(), p_item->has_submenus()
		};
		RmDefaultControlPainter::draw_menu_item(*pctx, visual, style);
	}
	rm_widget::on_draw(pctx);
}

void rm_menu::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != DOWN)
		return;
	if (vk == RM_KEY_ESCAPE) {
		m_proot_menu->close_tree();
		return;
	}
	const bool previous = is_root_menu() ? vk == RM_KEY_LEFT : vk == RM_KEY_UP;
	const bool next = is_root_menu() ? vk == RM_KEY_RIGHT : vk == RM_KEY_DOWN;
	if (previous || next) {
		const size_t selected = find_selectable(m_behaviour.highlighted_index(), next ? 1 : -1);
		if (selected != RmMenuBehaviour::invalid_index) {
			m_behaviour.pointer_move(selected);
			if (m_behaviour.has_open_item())
				open_submenu(selected);
		}
		return;
	}
	const size_t highlighted = m_behaviour.highlighted_index();
	if (highlighted == RmMenuBehaviour::invalid_index)
		return;
	if (vk == RM_KEY_ENTER || vk == RM_KEY_SPACE ||
		(is_root_menu() && vk == RM_KEY_DOWN) || (!is_root_menu() && vk == RM_KEY_RIGHT))
		activate_item(highlighted);
	else if (!is_root_menu() && vk == RM_KEY_LEFT) {
		hide();
		m_behaviour.close();
	}
}

bool rm_menu::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;
	rebuild_item_layout();
	const rm_vec2 local_cursor = cursor_to_local(cursor_pos);
	const bool inside_self = local_cursor.x >= 0.f && local_cursor.x <= m_size.x &&
		local_cursor.y >= 0.f && local_cursor.y <= m_size.y;
	if (!inside_self) {
		if (contains_visible_popup(local_cursor))
			return true;
		if (is_root_menu() && event == RM_MOUSE_EVENT_CLICK && state == DOWN)
			close_tree();
		return true;
	}

	const size_t index = hit_test_item(local_cursor);
	if (event == RM_MOUSE_EVENT_MOVE) {
		m_behaviour.pointer_move(index);
		if (index < get_num_submenus()) {
			rm_menu* p_item = get_submenu(index);
			if (p_item->has_submenus() && (m_behaviour.has_open_item() || !is_root_menu()))
				open_submenu(index);
			else if (!is_root_menu())
				close_submenus();
		}
		return false;
	}
	if (event != RM_MOUSE_EVENT_CLICK)
		return false;
	if (state == DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled && index < get_num_submenus() &&
			get_submenu(index)->has_submenus())
			open_submenu(index);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (update.activated)
			activate_item(index);
		return !update.handled;
	}
	return true;
}

void rm_menu::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	for (size_t i = 0; i < get_num_submenus(); ++i)
		get_submenu(i)->set_theme(m_theme);
	update_popup_geometry();
}

std::map<const rm_widget*, std::vector<rm_radiobutton*>> rm_radiobutton::s_groups;

rm_radiobutton::rm_radiobutton(rm_widget* parent, int x, int y, int width, int height,
	const std::string& label, rm_radiobutton_cb cb, RmThemeRef theme) :
	rm_widget(x, y, width, height, parent, "ui_radiobutton"),
	m_label(label), m_behaviour(false),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme()) {
	set_callback(cb);
	auto& grp = s_groups[parent];
	grp.push_back(this);
	if (grp.size() == 1)
		m_behaviour.set_checked(true);
}

rm_radiobutton::~rm_radiobutton() {
	auto it = s_groups.find(m_pparent);
	if (it != s_groups.end()) {
		auto& grp = it->second;
		grp.erase(std::remove(grp.begin(), grp.end(), this), grp.end());
		if (grp.empty()) {
			s_groups.erase(it);
		}
	}
}

void rm_radiobutton::uncheck_siblings()
{
	auto it = s_groups.find(m_pparent);
	if (it == s_groups.end())
		return;
	for (rm_radiobutton* p_radio : it->second) {
		if (p_radio != this)
			p_radio->m_behaviour.set_checked(false);
	}
}

void rm_radiobutton::notify_activation()
{
	if (m_behaviour.is_checked())
		uncheck_siblings();
	if (is_valid_callback())
		get_callback()(this);
}

void rm_radiobutton::set_checked(bool checked)
{
	if (checked)
		uncheck_siblings();
	m_behaviour.set_checked(checked);
}

void rm_radiobutton::select_default(rm_widget* parent, int index) {
	auto it = s_groups.find(parent);

	if (it == s_groups.end() || index < 0 || static_cast<size_t>(index) >= it->second.size())
		return;

	auto& grp = it->second;
	for (size_t i = 0; i < grp.size(); ++i)
		grp[i]->m_behaviour.set_checked(i == static_cast<size_t>(index));
}

void rm_radiobutton::select_by_label(rm_widget* parent, const std::string& label) {
	auto it = s_groups.find(parent);

	if (it == s_groups.end())
		return;

	for (rm_radiobutton* p_radio : it->second) {
		if (p_radio->m_label == label) {
			p_radio->set_checked(true);
			return;
		}
	}
}

void rm_radiobutton::on_draw(NVGcontext* pctx) {
	RmDefaultControlPainter::draw_radiobutton(*pctx,
		{ m_size.x, m_size.y, get_font(), m_label.c_str(), is_enabled(),
		  m_behaviour.is_hovered(), m_behaviour.is_pressed(),
		  m_behaviour.is_checked() },
		m_theme->radiobutton);
	rm_widget::on_draw(pctx);
}

void rm_radiobutton::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmRadioButtonStyle& style = m_theme->radiobutton;
	const float size = std::min(style.indicator_size, m_size.y);
	const float radius = size * 0.5f;
	RmDefaultControlPainter::draw_circle_focus_ring(*pctx,
		style.horizontal_padding + radius, m_size.y * 0.5f,
		radius + style.focus_ring_width * 0.5f,
		style.focus_ring_width, style.focus_ring);
}

bool rm_radiobutton::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	const bool inside = m_bbox.inside(pos);
	if (event == RM_MOUSE_EVENT_MOVE)
		return !m_behaviour.pointer_move(inside).handled;
	if (event != RM_MOUSE_EVENT_CLICK || vk != RM_KEY_LMOUSE)
		return true;
	if (state == DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(inside);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(inside);
		if (update.activated)
			notify_activation();
		return !update.handled;
	}
	return true;
}

void rm_radiobutton::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	const bool activation_key = vk == RM_KEY_ENTER || vk == RM_KEY_SPACE;
	const RmBehaviourUpdate update = state == UP
		? m_behaviour.key_up(activation_key)
		: m_behaviour.key_down(activation_key);
	if (update.activated)
		notify_activation();
}

rm_switch::rm_switch(rm_widget* parent, int x, int y, int width,
	bool initial, rm_switch_cb cb, RmThemeRef theme) :
	rm_widget(x, y, width, static_cast<int>((theme ? theme : RmThemeSnapshot::default_theme())->switch_control.track_height),
		parent, "ui_switch"),
	m_behaviour(initial), m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_callback(cb);
}

void rm_switch::on_draw(NVGcontext* pctx)
{
	if (m_proot)
		m_behaviour.advance(m_proot->get_delta_time(), m_theme->switch_control.animation_duration);
	RmDefaultControlPainter::draw_switch(*pctx,
		{ m_size.x, m_size.y, m_behaviour.animation_progress(), is_enabled(),
		  m_elem_flags.is_hovered(), m_behaviour.is_pressed() },
		m_theme->switch_control);

	rm_widget::on_draw(pctx);
}

void rm_switch::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmSwitchStyle& style = m_theme->switch_control;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

bool rm_switch::on_mouse(RM_MOUSE_EVENT event, RM_KEY key, RM_KEY_STATE state, rm_vec2& pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	const bool inside = m_bbox.inside(pos);
	if (event == RM_MOUSE_EVENT_MOVE) {
		const auto update = m_behaviour.pointer_move(inside);
		return !update.handled;
	}
	if (event != RM_MOUSE_EVENT_CLICK || key != RM_KEY_LMOUSE)
		return true;
	if (state == DOWN) {
		const auto update = m_behaviour.pointer_down(inside);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == UP) {
		const auto update = m_behaviour.pointer_up(inside);
		if (update.activated && is_valid_callback())
			get_callback()(this);
		return !update.handled;
	}
	return true;
}

void rm_switch::on_keybd(int sc, RM_KEY key, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	const bool activation_key = key == RM_KEY_ENTER || key == RM_KEY_SPACE;
	const auto update = state == UP
		? m_behaviour.key_up(activation_key)
		: m_behaviour.key_down(activation_key);
	if (update.activated && is_valid_callback())
		get_callback()(this);
}

rm_listview::rm_listview(rm_widget* parent, int x, int y, int width, int height,
	rm_listview_cb cb, RmThemeRef theme) :
	rm_widget(x, y, width, height, parent, "ui_listview", RM_FLAG_DEFAULT),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
{
	set_callback(cb);
	m_behaviour.set_count(0);
}

void rm_listview::add_item(const std::string& text)
{
	m_items.push_back(text);
	m_behaviour.set_count(m_items.size());
}

bool rm_listview::remove_item(size_t index)
{
	if (index >= m_items.size())
		return false;
	const size_t selected = m_behaviour.selected_index();
	m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
	m_behaviour.set_count(m_items.size());
	if (!m_items.empty() && selected != RmListViewBehaviour::invalid_index) {
		const size_t replacement = selected > index
			? selected - 1 : std::min(selected, m_items.size() - 1);
		m_behaviour.select(replacement);
	}
	return true;
}

void rm_listview::clear_items()
{
	m_items.clear();
	m_behaviour.set_count(0);
}

bool rm_listview::set_selected_index(size_t index, bool notify)
{
	const RmBehaviourUpdate update = m_behaviour.select(index);
	if (!update.handled)
		return false;
	if (notify && update.state_changed)
		notify_selection();
	return true;
}

size_t rm_listview::hit_test_row(const rm_vec2& local_cursor) const
{
	const RmListViewStyle& style = m_theme->listview;
	if (local_cursor.x < 0.f || local_cursor.x > m_size.x ||
		local_cursor.y < style.vertical_padding || local_cursor.y > m_size.y ||
		style.row_height <= 0.f)
		return RmListViewBehaviour::invalid_index;
	const size_t index = static_cast<size_t>(
		(local_cursor.y - style.vertical_padding) / style.row_height);
	return index < m_items.size() ? index : RmListViewBehaviour::invalid_index;
}

void rm_listview::notify_selection()
{
	const size_t selected = m_behaviour.selected_index();
	if (selected < m_items.size() && is_valid_callback())
		get_callback()(this, selected);
}

void rm_listview::on_draw(NVGcontext* pctx)
{
	const RmListViewStyle& style = m_theme->listview;
	RmDefaultControlPainter::draw_listview_surface(*pctx,
		{ m_size.x, m_size.y, is_enabled() }, style);
	for (size_t i = 0; i < m_items.size(); ++i) {
		const float y = style.vertical_padding + style.row_height * static_cast<float>(i);
		if (y >= m_size.y)
			break;
		RmDefaultControlPainter::draw_listview_row(*pctx,
			{ style.vertical_padding, y,
			  std::max(0.f, m_size.x - style.vertical_padding * 2.f),
			  std::min(style.row_height, m_size.y - y), get_font(),
			  m_items[i].c_str(), is_enabled(),
			  m_behaviour.hovered_index() == i,
			  m_behaviour.pressed_index() == i,
			  m_behaviour.selected_index() == i }, style);
	}
	rm_widget::on_draw(pctx);
}

void rm_listview::on_draw_overlay(NVGcontext* pctx)
{
	if (!is_enabled() || !m_elem_flags.is_focused())
		return;
	const RmListViewStyle& style = m_theme->listview;
	RmDefaultControlPainter::draw_rect_focus_ring(*pctx,
		{ 0.0f, 0.0f, m_size.x, m_size.y }, style.corner_radius,
		style.focus_ring_width, style.focus_ring);
}

void rm_listview::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	RM_UNUSED(sc);
	if (state != DOWN || m_items.empty())
		return;
	RmBehaviourUpdate update;
	if (vk == RM_KEY_UP || vk == RM_KEY_DOWN) {
		update = m_behaviour.select_relative(vk == RM_KEY_DOWN ? 1 : -1);
	}
	else if (vk == RM_KEY_HOME || vk == RM_KEY_END) {
		update = m_behaviour.select(vk == RM_KEY_HOME ? 0 : m_items.size() - 1);
		update.activated = update.state_changed;
	}
	if (update.activated)
		notify_selection();
}

bool rm_listview::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
	RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	RM_UNUSED(delta);
	if (vk != RM_KEY_NONE && vk != RM_KEY_LMOUSE)
		return true;
	const size_t index = hit_test_row(cursor_to_local(cursor_pos));
	if (event == RM_MOUSE_EVENT_MOVE)
		return !m_behaviour.pointer_move(index).handled;
	if (event != RM_MOUSE_EVENT_CLICK)
		return true;
	if (state == DOWN) {
		const RmBehaviourUpdate update = m_behaviour.pointer_down(index);
		if (update.handled && get_root())
			get_root()->capture_pointer(this);
		return !update.handled;
	}
	if (state == UP) {
		const RmBehaviourUpdate update = m_behaviour.pointer_up(index);
		if (update.activated)
			notify_selection();
		return !update.handled;
	}
	return true;
}
