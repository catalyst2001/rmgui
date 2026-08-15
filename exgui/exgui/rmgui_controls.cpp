#include "rmgui_controls.h"
#include "rmgui_default_painter.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
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
		m_behaviour.is_pressed(),
		m_elem_flags.is_focused()
	};
	RmDefaultControlPainter::draw_button(*pctx, visual, m_theme->buttons.resolve(m_variant));
	rm_widget::on_draw(pctx);
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
		m_elem_flags.is_focused(),
		m_behaviour.is_active() && m_blink_state
	};
	m_layout = RmDefaultControlPainter::layout_text_input(
		*pctx, visual, m_theme->text_input);
	m_scroll_offset = m_layout.scroll_offset;
	RmDefaultControlPainter::draw_text_input(
		*pctx, visual, m_layout, m_theme->text_input);
	rm_widget::on_draw(pctx);
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
		m_elem_flags.is_focused(),
		m_behaviour.is_checked()
	};
	RmDefaultControlPainter::draw_checkbox(*pctx, visual, m_theme->checkbox);
	rm_widget::on_draw(pctx);
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
		m_elem_flags.is_focused(), m_behaviour.is_expanded()
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
		m_behaviour.is_dragging(),
		m_elem_flags.is_focused()
	};
	RmDefaultControlPainter::draw_slider(*pctx, visual, m_theme->slider);

	rm_widget::on_draw(pctx);
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
	if (is_vertical()) {
		move({ std::max(0.f, parent_size.x - thickness), 0.f });
		resize(thickness, parent_size.y);
	}
	else {
		move({ 0.f, std::max(0.f, parent_size.y - thickness) });
		resize(parent_size.x, thickness);
	}
}

void rm_scrollbar::notify_position()
{
	if (is_valid_callback())
		get_callback()(this, m_behaviour.position());
}

void rm_scrollbar::on_draw(NVGcontext* pctx)
{
	const RmScrollbarStyle& style = m_theme->scrollbar;
	const float length = track_length();
	RmDefaultControlPainter::draw_scrollbar(*pctx,
		{ m_size.x, m_size.y,
		  m_behaviour.thumb_offset(length, style.minimum_thumb_length),
		  m_behaviour.thumb_length(length, style.minimum_thumb_length),
		  is_vertical(), is_enabled(), m_elem_flags.is_hovered(),
		  m_behaviour.is_dragging(), m_elem_flags.is_focused() }, style);
	rm_widget::on_draw(pctx);
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
	adjust_geometry();
}

rm_scrollbar::~rm_scrollbar()
{
}

void rm_scrollbar::set_position(float position, bool notify)
{
	const RmBehaviourUpdate update = m_behaviour.set_position(position);
	if (notify && update.state_changed)
		notify_position();
}

void rm_scrollbar::set_content_metrics(float content_extent, float viewport_extent)
{
	const float fraction = content_extent > FLT_EPSILON
		? viewport_extent / content_extent : 1.f;
	m_behaviour.set_viewport_fraction(fraction);
}

void rm_scrollbar::set_theme(RmThemeRef theme)
{
	m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
	adjust_geometry();
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
			m_elem_flags.is_focused(),
			m_tabs[i].is_closable() && !m_tabs[i].is_pinned(),
			m_close_hovered == i
		};
		RmDefaultControlPainter::draw_tab(*pctx, visual, style);
	}
	rm_widget::on_draw(pctx);
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

void rm_treeview::on_draw(NVGcontext* pctx) {
	//m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
	pctx->setFontSize(m_rowHeight * 0.8f);
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	// background
	pctx->beginPath();
	pctx->rect(0.f, 0.f, m_size.x, m_size.y);
	pctx->fillColor(NVGcolor::RGBA(245, 245, 245, 255));
	pctx->fill();

	float y = 0.f;
	for (auto root : m_roots) {
		y = draw_node(pctx, root, 0.f, y);
		if (y > m_size.y)
			break; // clip
	}
	rm_widget::on_draw(pctx);
}

float rm_treeview::draw_node(NVGcontext* pctx, rm_tree_node* node, float x, float y) {
	// background if selected
	if (node == m_selected) {
		pctx->beginPath();
		pctx->rect(x, y, m_size.x - x, m_rowHeight);
		pctx->fillColor(NVGcolor::RGBA(200, 230, 255, 255));
		pctx->fill();
	}
	// expand/collapse icon
	if (!node->children.empty()) {
		const float sz = m_rowHeight * 0.5f;
		float cx = x + (m_indent - sz) * 0.5f;
		float cy = y + (m_rowHeight - sz) * 0.5f;
		pctx->beginPath();

		//TODO: k.d replace by icons
		if (node->expanded) {
			// draw '-'
			pctx->moveTo(cx, cy + sz / 2);
			pctx->lineTo(cx + sz, cy + sz / 2);
		}
		else {
			// draw '+'
			pctx->moveTo(cx, cy + sz / 2);
			pctx->lineTo(cx + sz, cy + sz / 2);
			pctx->moveTo(cx + sz / 2, cy);
			pctx->lineTo(cx + sz / 2, cy + sz);
		}
		pctx->strokeColor(NVGcolor::RGBA(100, 100, 100, 255));
		pctx->stroke();
	}
	// draw text
	float tx = x + m_indent;
	float ty = y + m_rowHeight * 0.5f;
	pctx->fillColor(NVGcolor::RGBA(0, 0, 0, 255));
	pctx->text(tx, ty, node->name.c_str(), nullptr);

	y += m_rowHeight;
	// draw children
	if (node->expanded) {
		for (auto child : node->children) {
			y = draw_node(pctx, child, x + m_indent, y);
			if (y > m_size.y)
				break;
		}
	}
	return y;
}

bool rm_treeview::on_mouse(RM_MOUSE_EVENT event,
	RM_KEY vk,
	RM_KEY_STATE state,
	rm_vec2& cursor_pos, rm_vec2 delta)
{
	if (event == RM_MOUSE_EVENT_CLICK && state == DOWN)
	{
		rm_tree_node* hitNode = nullptr;
		float y = m_pos_of_parent.y;
		if (hit_test(cursor_pos,
			nullptr,
			m_pos_of_parent.x,
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

void rm_output_text::on_draw(NVGcontext* pctx)
{
	rm_vec2 textpos(5.f, 0.f);
	//pctx->BeginPath();
	//pctx->RoundedRect( 1.f, 1.f, m_size.x - 1.f, m_size.y - 1.f, 4.f);
	//pctx->FillColor( NVGcolor::RGB(255, 255, 255));
	////pctx->StrokeColor( NVGcolor::RGB(0, 0, 0));
	//pctx->Fill();
	////pctx->Stroke();

	rm_vec2 pos(1.f, 1.f);
	rm_vec2 size(m_size.x - 1.f, m_size.y - 1.f);
	NVGcolor background = NVGcolor::RGB(255, 255, 255);
	NVGcolor stroke = NVGcolor::RGB(0, 0, 0);
	NVGcolor colors[rm_utl::RM_BFRM_MAX_COLORS] = { NVGcolor::RGB(128, 128, 128), NVGcolor::RGB(60, 60, 60) };
	rm_corners_style cstyle;
	cstyle.set_all_corners_radius(8.f);
	rm_utl::draw_frame(pctx, pos, size, rm_utl::RM_BFRM_MODE_SUNKEN, background, stroke,
		1.f, colors, &cstyle);

	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
	pctx->fillColor(NVGcolor::RGBA(0, 0, 0, 255));
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	for (size_t i = 0; i < m_linesbuf.get_num_output_lines(); i++) {
		const rm_line_ring_buffer::rm_rb_line* pline = m_linesbuf.get_output_line(i);
		pctx->text(textpos.x, textpos.y, pline->get_cstr(), nullptr);
		textpos.y += m_line_height;
	}
}

bool rm_output_text::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	return true;
}

rm_output_text::rm_output_text(rm_widget* p_parent, int x, int y, int width, int height, float line_height, size_t num_lines) :
	rm_widget(x, y, width, height, p_parent, "ui_outputtext", RM_FLAG_DEFAULT), m_linesbuf(num_lines, 512, num_lines), m_line_height(line_height)
{
	m_textbuf.resize(8096);
}

rm_output_text::rm_output_text(rm_widget* p_parent, float x, float y, float width, float height, float line_height, size_t num_lines) :
	rm_widget(x, y, width, height, p_parent, "ui_outputtext", RM_FLAG_DEFAULT), m_linesbuf(num_lines, 512, num_lines), m_line_height(line_height)
{
	m_textbuf.resize(8096);
}

void rm_output_text::printf(const char* pformat, ...)
{
	va_list argptr;
	va_start(argptr, pformat);
	vsnprintf(&m_textbuf[0], m_textbuf.size(), pformat, argptr);
	va_end(argptr);
	m_linesbuf.append_text(m_textbuf);
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
		is_enabled(),
		m_elem_flags.is_hovered(),
		m_elem_flags.is_focused(),
		hovered == RmNumberInputPart::increment,
		pressed == RmNumberInputPart::increment,
		hovered == RmNumberInputPart::decrement,
		pressed == RmNumberInputPart::decrement
	};
	RmDefaultControlPainter::draw_number_input(
		*pctx, visual, m_theme->number_input);
	rm_widget::on_draw(pctx);
}

RmNumberInputPart rm_number_input::hit_test_part(const rm_vec2& cursor_pos) const
{
	if (!m_bbox.inside(cursor_pos))
		return RmNumberInputPart::none;
	const float local_x = cursor_pos.x - m_pos_of_parent.x;
	const float local_y = cursor_pos.y - m_pos_of_parent.y;
	const float button_x = m_size.x -
		std::min(m_theme->number_input.button_width, m_size.x);
	if (local_x < button_x)
		return RmNumberInputPart::field;
	return local_y < m_size.y * 0.5f
		? RmNumberInputPart::increment : RmNumberInputPart::decrement;
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
	RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "ui_number_input",
		RM_FLAG_DEFAULT | RM_FLAG_OPAQUE),
	m_behaviour(type, value, step, minval, maxval),
	m_theme(theme ? std::move(theme) : RmThemeSnapshot::default_theme())
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
		  m_elem_flags.is_focused(), m_behaviour.is_checked() },
		m_theme->radiobutton);
	rm_widget::on_draw(pctx);
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
		  m_elem_flags.is_hovered(), m_behaviour.is_pressed(), m_elem_flags.is_focused() },
		m_theme->switch_control);

	rm_widget::on_draw(pctx);
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
		{ m_size.x, m_size.y, m_elem_flags.is_focused(), is_enabled() }, style);
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
