#include "rmgui_controls.h"
#include "rmgui_default_painter.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>

#include "stb_image.h"

namespace {

template <typename Style>
RmStateColors same_color_for_all_states(Style color)
{
	return { color, color, color, color };
}

RmThemeRef theme_from_legacy_checkbox(const rm_checkbox_style* legacy)
{
	if (!legacy)
		return RmTheme::default_theme();

	auto theme = std::make_shared<RmTheme>(*RmTheme::default_theme());
	theme->checkbox.background = same_color_for_all_states(legacy->get_background_color());
	theme->checkbox.border = same_color_for_all_states(legacy->get_border_color());
	theme->checkbox.mark = same_color_for_all_states(legacy->get_mark_color());
	theme->checkbox.text = same_color_for_all_states(legacy->get_text_color());
	theme->checkbox.box_size = static_cast<float>(legacy->get_check_size());
	theme->checkbox.corner_radius = legacy->get_avg_radius();
	theme->checkbox.border_width = legacy->get_border_width();
	theme->checkbox.text_gap = legacy->get_text_offsets().x;
	theme->checkbox.font_size = legacy->get_font_size();
	return theme;
}

RmThemeRef theme_with_progress_radius(RmThemeRef source, float radius)
{
	auto theme = std::make_shared<RmTheme>(
		source ? *source : *RmTheme::default_theme());
	theme->progress.corner_radius = std::max(0.0f, radius);
	return theme;
}

RmThemeRef theme_from_legacy_switch(const rm_switch_style* legacy)
{
	if (!legacy)
		return RmTheme::default_theme();

	auto theme = std::make_shared<RmTheme>(*RmTheme::default_theme());
	theme->switch_control.track_on = same_color_for_all_states(legacy->get_track_on());
	theme->switch_control.track_off = same_color_for_all_states(legacy->get_track_off());
	theme->switch_control.knob = same_color_for_all_states(legacy->get_knob_color());
	theme->switch_control.track_height = legacy->get_track_height();
	theme->switch_control.padding = legacy->get_padding();
	theme->switch_control.animation_duration = legacy->get_anim_time();
	theme->switch_control.knob_radius = legacy->get_knob_radius();
	theme->switch_control.corner_radius = legacy->get_avg_radius();
	theme->switch_control.shadow_offset = legacy->get_shadow_offset();
	theme->switch_control.shadow_size = legacy->get_shadow_size();
	theme->switch_control.shadow = legacy->get_shadow_color();
	return theme;
}

RmThemeRef theme_from_legacy_slider(const rm_slider_style* legacy)
{
	if (!legacy)
		return RmTheme::default_theme();

	auto theme = std::make_shared<RmTheme>(*RmTheme::default_theme());
	theme->slider.track = same_color_for_all_states(legacy->get_track_bg());
	theme->slider.fill = same_color_for_all_states(legacy->get_track_fill());
	theme->slider.thumb = same_color_for_all_states(legacy->get_thumb_color());
	theme->slider.thumb_border = same_color_for_all_states(legacy->get_thumb_border_color());
	theme->slider.track_height = legacy->get_track_height();
	theme->slider.padding = legacy->get_padding();
	theme->slider.thumb_radius = legacy->get_thumb_radius();
	theme->slider.thumb_border_width = legacy->get_thumb_border_width();
	return theme;
}

} // namespace

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
	RmThemeRef theme)
	: rm_widget(x, y, width, height, p_parent, "ui_button"), m_text(text),
	m_theme(theme ? std::move(theme) : RmTheme::default_theme())
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
	RmDefaultControlPainter::draw_button(*pctx, visual, m_theme->button);
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
	m_theme(theme ? std::move(theme) : RmTheme::default_theme())
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
	rm_text_input_style* pstyle, uint32_t flags, float blink_cursor_interval)
	: rm_widget(x, y, width, height, p_parent, "ui_text_input", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL), m_active(false), m_ctrl_pressed(false), m_dragging(false),
	m_scroll_offset(0.f), m_last_click_time(0.0), m_last_click_pos({ 0,0 }), m_asc(0.f), m_line_h(0.f) {
	set_style(pstyle);
	m_blink_state = false;
	m_timer.set_interval(blink_cursor_interval);
	m_flags = flags;
}

rm_text_input::~rm_text_input() {}

void rm_text_input::on_draw(NVGcontext* pctx) {
	const std::string& txt = m_buffer.str();
	float desc;
	std::vector<std::string> lines;
	std::vector<size_t>      starts;

	pctx->setFontFaceId(((int)get_font().getValue()));
	pctx->setFontSize(m_pstyle->get_font_size());
	pctx->textMetrics(&m_asc, &desc, &m_line_h);

	// background & border
	pctx->beginPath();
	float stroke_width = m_pstyle->get_border_width();
	pctx->roundedRectVarying(stroke_width, stroke_width, m_size.x - stroke_width * 2, m_size.y - stroke_width * 2,
		m_pstyle->get_corner_radius(LEFT_TOP),
		m_pstyle->get_corner_radius(RIGHT_TOP),
		m_pstyle->get_corner_radius(RIGHT_BOTTOM),
		m_pstyle->get_corner_radius(LEFT_BOTTOM));
	pctx->fillColor(m_active
		? m_pstyle->get_active_bgr_color()
		: m_pstyle->get_unactive_bgr_color());
	pctx->fill();
	pctx->strokeColor(m_pstyle->get_border_color());
	pctx->StrokeWidth(m_pstyle->get_border_width());
	pctx->stroke();

	bool multiline = (m_flags & RMGUI_TEXT_INPUT_MULTILINE);
	float offset = 0.f;
	size_t ci = m_buffer.has_selection() ? m_buffer.sel_end : m_buffer.pos();
	if (!multiline) {
		// single-line glyph positions
		m_glyph_positions.clear();
		m_glyph_positions.push_back(0.f);
		for (size_t i = 1; i <= txt.size(); ++i) {
			float w = pctx->textBounds(0, 0, txt.substr(0, i).c_str(), nullptr, nullptr);
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
				float w = pctx->textBounds(0, 0,
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
		pctx->fillColor(m_pstyle->get_selection_color());
		if (!multiline) {
			size_t a = std::min(m_buffer.sel_start, m_buffer.sel_end);
			size_t b = std::max(m_buffer.sel_start, m_buffer.sel_end);
			float x0 = 5.f - offset + m_glyph_positions[a] + m_pstyle->get_text_offset();
			float x1 = 5.f - offset + m_glyph_positions[b] + m_pstyle->get_text_offset();
			float baseline = m_size.y * 0.5f + (m_asc + desc) * 0.5f;
			pctx->beginPath();
			pctx->rect(x0, baseline - m_asc, x1 - x0, m_asc - desc);
			pctx->fill();
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
					float x0 = 5.f - offset + pctx->textBounds(0, 0, pre.c_str(), nullptr, nullptr);
					float w = pctx->textBounds(0, 0, sel.c_str(), nullptr, nullptr);
					float y0 = top_pad + i * m_line_h - m_asc;
					pctx->beginPath();
					pctx->rect(x0 + m_pstyle->get_text_offset(), y0, w, m_line_h);
					pctx->fill();
				}
			}
		}
	}

	// draw text and cursor
	pctx->fillColor(m_pstyle->get_text_color());
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
	if (!multiline) {
		float baseline = m_size.y * 0.5f + (m_asc + desc) * 0.5f;
		pctx->text(5.f - offset + m_pstyle->get_text_offset(),
			baseline, txt.c_str(), nullptr);
		if (m_active && m_blink_state) {
			float cw = m_glyph_positions[ci];
			pctx->beginPath();
			pctx->moveTo(5.f - offset + cw + m_pstyle->get_text_offset() + 1, baseline - m_asc);
			pctx->lineTo(5.f - offset + cw + m_pstyle->get_text_offset() + 1, baseline - desc);
			pctx->StrokeWidth(m_pstyle->get_blink_width());
			pctx->strokeColor(m_pstyle->get_blink_color());
			pctx->stroke();
		}
	}
	else {
		float top_pad = 5.f + m_asc;
		for (size_t i = 0; i < lines.size(); ++i) {
			float y = top_pad + i * m_line_h;
			pctx->text(5.f - offset + m_pstyle->get_text_offset(), y, lines[i].c_str(), nullptr);
		}

		if (m_active && m_blink_state) {
			int cli = int(std::upper_bound(m_line_starts.begin(), m_line_starts.end(), ci) - m_line_starts.begin()) - 1;
			size_t off = ci - m_line_starts[cli];
			float cx = m_line_glyphs[cli][off];
			float cy = 5.f + m_asc + cli * m_line_h;
			pctx->beginPath();
			pctx->moveTo(5.f - offset + cx + m_pstyle->get_text_offset() + 1, cy - m_asc);
			pctx->lineTo(5.f - offset + cx + m_pstyle->get_text_offset() + 1, cy - m_asc + m_line_h);
			pctx->StrokeWidth(m_pstyle->get_blink_width());
			pctx->strokeColor(m_pstyle->get_blink_color());
			pctx->stroke();
		}
	}

	rm_widget::on_draw(pctx);
}

void rm_text_input::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state)
{
	if (!m_active) {
		return;
	}

	if (vk == RM_KEY_LCTRL || vk == RM_KEY_RCTRL) {
		m_ctrl_pressed = (state != RM_KEY_STATE::UP);
		return;
	}

	if ((state == RM_KEY_STATE::DOWN || state == RM_KEY_STATE::REPEAT)) {
		if (!m_ctrl_pressed) {
			bool handled = true;
			switch (vk) {
			case RM_KEY_BACKSPACE:
				m_buffer.backspace();
				break;
			case RM_KEY_DELETE:
				m_buffer.delete_forward();
				break;
			case RM_KEY_LEFT:
				m_buffer.move_cursor_left();
				break;
			case RM_KEY_RIGHT:
				m_buffer.move_cursor_right();
				break;
			case RM_KEY_UP:
				m_buffer.move_cursor_up();
				break;
			case RM_KEY_DOWN:
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
			case RM_KEY_Z:
				m_buffer.undo();
				break;
			case RM_KEY_Y:
				m_buffer.redo();
				break;
			}
		}
	}

	if (state != RM_KEY_STATE::DOWN)
		return;

	if (m_ctrl_pressed && vk == RM_KEY_A) {
		m_buffer.select_all();
		m_timer.reset(m_psysdf);
		m_blink_state = false;
		return;
	}

	if (m_ctrl_pressed) {
		switch (vk) {
		case RM_KEY_C:
			m_buffer.copy_all(m_psysdf);
			break;

		case RM_KEY_V:
			m_buffer.paste(m_psysdf);
			break;

		case RM_KEY_X:
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

	if (vk == RM_KEY_ENTER && (m_flags & RMGUI_TEXT_INPUT_MULTILINE)) {
		m_buffer.insert_cp('\n');
		m_timer.reset(m_psysdf);
		m_blink_state = false;
		return;
	}
}

bool rm_text_input::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	bool inside = m_bbox.inside(cursor_pos);
	float offset = m_pstyle->get_text_offset();
	float text_draw_x = m_pos_of_parent.x + 5.f + offset;
	float local_x = cursor_pos.x - text_draw_x;
	float local_y = cursor_pos.y - m_pos_of_parent.y;
	const double now = m_psysdf->get_time();
	bool multiline = (m_flags & RMGUI_TEXT_INPUT_MULTILINE);

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
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

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::DOWN) {
		m_buffer.select_all();
		m_timer.reset(m_psysdf);
		m_blink_state = false;
		return true;
	}

	if (event == RM_MOUSE_EVENT_MOVE && state == RM_KEY_STATE::DOWN && m_dragging) {
		size_t idx = multiline ? hit_test_index(local_x + m_scroll_offset, local_y) : hit_test_index(local_x);
		m_buffer.sel_end = idx;
		if (!multiline) {
			ensure_visible(idx);
		}
		return true;
	}

	if (event == RM_MOUSE_EVENT_CLICK && state == RM_KEY_STATE::UP) {
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
	size_t end = (static_cast<size_t>(li + 1) < m_line_starts.size())
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

rm_checkbox::rm_checkbox(rm_widget* p_parent, int x, int y, int width,
	const std::string& label, rm_checkbox_cb pcallback, RmThemeRef theme)
	: rm_widget(x, y, width,
		static_cast<int>((theme ? theme : RmTheme::default_theme())->checkbox.box_size),
		p_parent, "ui_checkbox"),
	m_behaviour(false), m_label(label),
	m_theme(theme ? std::move(theme) : RmTheme::default_theme())
{
	set_callback(pcallback);
}

rm_checkbox::rm_checkbox(rm_widget* p_parent, int x, int y, int width,
	rm_checkbox_style* pstyle, const std::string& label, rm_checkbox_cb pcallback)
	: rm_checkbox(p_parent, x, y, width, label, pcallback, theme_from_legacy_checkbox(pstyle)) {}

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

rm_combobox::rm_combobox(rm_widget* p_parent, int x, int y, int width, int height, rm_combobox_cb pcallback)
	: rm_widget(x, y, width, height, p_parent, "ui_combobox", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_DISABLE_SCISSOR | RM_FLAG_HIGHEST_PRIORITY), m_selected(0), m_expanded(false)
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
		[pitem](rm_combo_item& item) {
			return !strcmp(pitem, item.get_name());
		}
	);

	if (it != m_items.end())
		return it - m_items.begin();

	return rm_combobox::kinvalid_index;
}

void rm_combobox::on_draw(NVGcontext* pctx) {
	//m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!

	//bgr
	pctx->beginPath();
	pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, 4.0f);
	pctx->fillColor(NVGcolor::RGBA(180, 180, 180, 255));
	pctx->fill();
	pctx->strokeColor(NVGcolor::RGBA(0, 0, 0, 255));
	pctx->stroke();

	pctx->setFontSize(18.0f);
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	pctx->fillColor(NVGcolor::RGBA(0, 0, 0, 255));
	if (!m_items.empty() && m_selected >= 0 && m_selected < (int)m_items.size())
		pctx->text(5.f, m_size.y / 2.0f, m_items[m_selected].get_name(), nullptr);

	// arrow
	float x = 0, y = 0, w = m_size.x, h = m_size.y;
	float r = 4.0f;
	float ax = x + w - h * 0.5f, ay = y + h * 0.5f, sz = 5.f;
	pctx->beginPath();
	if (!m_expanded) {
		pctx->moveTo(ax - sz, ay - sz * 0.5f);
		pctx->lineTo(ax + sz, ay - sz * 0.5f);
		pctx->lineTo(ax, ay + sz * 0.5f);
	}
	else {
		pctx->moveTo(ax - sz, ay + sz * 0.5f);
		pctx->lineTo(ax + sz, ay + sz * 0.5f);
		pctx->lineTo(ax, ay - sz * 0.5f);
	}
	pctx->closePath();
	pctx->fillColor(NVGcolor::RGBA(50, 50, 50, 255));
	pctx->fill();

	if (m_expanded) {
		for (size_t i = 0; i < m_items.size(); i++) {
			rm_combo_item& item = m_items[i];
			float itemY = m_size.y * (1.f + i);
			rm_bbox bbox;
			rm_vec2 item_pos(m_pos_of_parent.x, m_pos_of_parent.y + itemY);
			bbox.init(item_pos, m_size);
			pctx->beginPath();
			pctx->rect(0.f, itemY, m_size.x, m_size.y);
			pctx->fillColor(bbox.inside(m_cursor) ? NVGcolor::RGBA(80, 80, 80, 255) : NVGcolor::RGBA(200, 200, 200, 255));
			pctx->fill();
			pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			pctx->fillColor(NVGcolor::RGBA(0, 0, 0, 255));
			pctx->text(5.f, itemY + m_size.y / 2.0f, item.get_name(), nullptr);
		}
	}
	rm_widget::on_draw(pctx);
}

bool rm_combobox::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	m_cursor = cursor_pos;
	if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
		if (m_bbox.inside(cursor_pos)) {
			if (m_expanded) {
				m_expanded = false;
				return false;
			}
			m_expanded = true;
			return false;
		}

		if (m_expanded) {
			float itemYStart = m_pos_of_parent.y + m_size.y;
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
	m_theme(theme ? std::move(theme) : RmTheme::default_theme()) {
}

rm_slider::rm_slider(rm_widget* p_parent, int x, int y, int width, int height,
	rm_slider_style* pstyle, float min, float max, float initial, rm_slider_callback pcallback)
	: rm_slider(p_parent, x, y, width, height, min, max, initial, pcallback,
		theme_from_legacy_slider(pstyle)) {}

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

rm_progress_base::rm_progress_base(rm_widget* p_parent, int x, int y, int width, int height,
	float initial, float corner_round, RmThemeRef theme) :
	rm_widget(x, y, width, height, p_parent, "ui_progress_base"),
	m_behaviour(initial), m_theme(theme_with_progress_radius(std::move(theme), corner_round))
{}

rm_progress_base::~rm_progress_base()
{
}

void rm_progress_base::on_draw(NVGcontext* pctx)
{
	RmDefaultControlPainter::draw_progress(*pctx,
		{ m_size.x, m_size.y, m_behaviour.fraction(), is_enabled() },
		m_theme->progress);
	rm_widget::on_draw(pctx);
}

void rm_progress_base::set_corner_round(float radius)
{
	m_theme = theme_with_progress_radius(m_theme, radius);
}

rm_progress_image::rm_progress_image(rm_widget* p_parent, int x, int y, int width, int height,
	rm_image img, float pattern_angle, float pattern_alpha, float initial, float corner_round) :
	rm_progress_base(p_parent, x, y, width, height, initial, corner_round)
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
	pctx->roundedRect(0.f, 0.f, m_size.x, m_size.y, get_corner_round());
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
			percent_rect.width, percent_rect.height, get_corner_round());
		pctx->fillPaint(paint);
		pctx->fill();

		drawSprite(pctx, m_image, m_alpha, 0.f, 0.f, 13.f, 15.f,
			percent_rect.x, percent_rect.y, percent_rect.width, percent_rect.height,
			get_corner_round(), get_corner_round(), get_corner_round(), get_corner_round());
	}
	rm_widget::on_draw(pctx);
}

void rm_scroll_base::orient_detect(const rm_rect& background)
{
	m_orientation = (background.width > background.height) ? RM_ORIENT_HORZ : RM_ORIENT_VERT;
}

void rm_scroll_base::draw_scroll(NVGcontext* pctx, const rm_vec2& back, rm_scroll_style* pstyle, float pos)
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
	pctx->beginPath();
	pctx->fillColor(pstyle->get_scroll_background_color());
	pctx->roundedRect(0, 0, back.x, back.y, pstyle->get_scroll_corner_radius());
	pctx->fill();
	pctx->StrokeWidth(pstyle->get_background_stroke_width());
	pctx->strokeColor(pstyle->get_scroll_background_border_color());
	pctx->stroke();

	/* paint thumb */
	pctx->beginPath();
	pctx->fillColor(pstyle->get_scroll_thumb_color());
	pctx->roundedRect(thumb_rect.x, thumb_rect.y, thumb_rect.width, thumb_rect.height, pstyle->get_scroll_corner_radius());
	pctx->fill();
	pctx->StrokeWidth(pstyle->get_thumb_stroke_width());
	pctx->strokeColor(pstyle->get_scroll_thumb_border_color());
	pctx->stroke();
}

void rm_scroll_base::draw_scroll(NVGcontext* pctx, rm_scroll_style* pstyle, rm_vec2 content,
	const rm_vec2& window, float thumb_thickness, float pos)
{
	rm_rect thumb_rect;
	float   thumb_size;
	float   background_round;
	float   thumb_round;

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
		background_round = (window.y / 2.f) * pstyle->get_scroll_corner_radius();
		thumb_round = (thumb_rect.height / 2.f) * pstyle->get_scroll_corner_radius();
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
		background_round = (window.x / 2.f) * pstyle->get_scroll_corner_radius();
		thumb_round = (thumb_rect.width / 2.f) * pstyle->get_scroll_corner_radius();
	}

	// paint background
	pctx->beginPath();
	pctx->fillColor(pstyle->get_scroll_background_color());
	pctx->roundedRect(0, 0, window.x, window.y, background_round);
	pctx->fill();
	pctx->StrokeWidth(pstyle->get_background_stroke_width());
	pctx->strokeColor(pstyle->get_scroll_background_border_color());
	pctx->stroke();

	// paint thumb
	pctx->beginPath();
	pctx->fillColor(pstyle->get_scroll_thumb_color());
	pctx->roundedRect(thumb_rect.x, thumb_rect.y, thumb_rect.width, thumb_rect.height, thumb_round);
	pctx->fill();
	pctx->StrokeWidth(pstyle->get_thumb_stroke_width());
	pctx->strokeColor(pstyle->get_scroll_thumb_border_color());
	pctx->stroke();
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
		m_pos_of_parent.init(m_pos_of_parent.x, m_pos_of_parent.y + parent_size.y - m_pstyle->get_thumb_size());
	}
	else {
		m_size.init(m_pstyle->get_thumb_size(), parent_size.y);
		m_pos_of_parent.init(m_pos_of_parent.x + parent_size.x - m_pstyle->get_thumb_size(), m_pos_of_parent.y);
	}
}

void rm_scrollbar::on_draw(NVGcontext* pctx)
{
	rm_vec2 content_rect = m_pparent ? m_pparent->get_size() : m_size;
	draw_scroll(pctx, m_pstyle, content_rect, m_size, m_pstyle->get_thumb_size(), m_position);
	rm_widget::on_draw(pctx);
}

bool rm_scrollbar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	return true;
}

rm_scrollbar::rm_scrollbar(rm_widget* p_parent, RM_ORIENT orient, rm_scroll_style* p_style, float inital_pos) :
	rm_widget(0, 0, 0, 0, p_parent, "ui_scrollbar", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL), m_position(inital_pos)
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
		RM_FLAG_DEFAULT, 0, nullptr),
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
		RM_FLAG_DEFAULT,
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

void rm_tabcontrol::on_draw(NVGcontext* pctx) {
	//m_bbox.from_rect(m_absolute); //NOTE: K.D. commented
	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!

	// background
	//pctx->BeginPath();
	//pctx->RoundedRect( 0.f, 0.f, m_relative.width, m_relative.height, 4.0f);
	//pctx->FillColor( m_pstyle->get_background_color());
	//pctx->Fill();
	//pctx->StrokeColor( m_pstyle->get_border_color());
	//pctx->Stroke();
	bool is_horizontal = m_pstyle->is_horizontal();
	size_t num_tabs = m_tabs.size();
	if (!num_tabs)
		return;

	rm_vec2 tab_size;
	get_one_tab_size(tab_size);

	pctx->setFontSize(m_pstyle->get_font_size());
	pctx->setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

	for (size_t i = 0; i < num_tabs; ++i) {
		bool is_first = i == 0;
		bool is_last = i == num_tabs - 1;
		float x = (is_horizontal ? i * tab_size.x : 0.f);
		float y = (is_horizontal ? 0.f : i * tab_size.y);

		pctx->beginPath();
		pctx->fillColor((int(i) == m_selected) ? m_pstyle->get_selected_color() : m_pstyle->get_unselected_color());
		pctx->roundedRectVarying(x, y, tab_size.x, tab_size.y,
			is_first ? m_pstyle->get_corner_radius(LEFT_TOP) : 0.f,
			(is_horizontal ? is_last : is_first) ? m_pstyle->get_corner_radius(RIGHT_TOP) : 0.f,
			is_last ? m_pstyle->get_corner_radius(RIGHT_BOTTOM) : 0.f,
			(is_horizontal ? is_first : is_last) ? m_pstyle->get_corner_radius(LEFT_BOTTOM) : 0.f);
		pctx->fill();
		pctx->strokeColor(m_pstyle->get_border_color());
		pctx->stroke();
		pctx->fillColor(m_pstyle->get_text_color());
		pctx->text((x + tab_size.x * 0.5f) + m_pstyle->get_text_offsets().x,
			(y + tab_size.y * 0.5f) + m_pstyle->get_text_offsets().y,
			m_tabs[i].get_name(),
			NULL);
		rm_widget::on_draw(pctx);
	}
}

bool rm_tabcontrol::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
	int     idx;
	rm_vec2 tabcontrol_size;
	get_tabcontrol_size(tabcontrol_size);
	m_bbox.init(m_pos_of_parent, tabcontrol_size);
	rm_vec2 local_mouse_pos = cursor_to_local(cursor_pos);
	size_t num_tabs = m_tabs.size();
	rm_vec2 sz;
	get_one_tab_size(sz);
	float axis = m_pstyle->is_horizontal() ? local_mouse_pos.x : local_mouse_pos.y;
	if (num_tabs && event == RM_MOUSE_EVENT_CLICK && state == DOWN && m_bbox.inside(cursor_pos)) {
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

void rm_number_input::draw_buttons(NVGcontext* pctx)
{
	//pctx->BeginPath();

}

void rm_number_input::on_draw(NVGcontext* pctx)
{
	//pctx->BeginPath();

}

bool rm_number_input::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	return false;
}

rm_number_input::rm_number_input(rm_widget* p_parent, int x, int y, int width, int height,
	input_type type, float value, float step, float minval, float maxval) :
	rm_widget(x, y, width, height, p_parent, "ui_number_input", RM_FLAG_DEFAULT),
	m_type(type), m_value(value), m_step(step), m_minval(minval), m_maxval(maxval)
{
}

void rm_tabcontrol_ex::on_draw(NVGcontext* pctx)
{
	rm_vec2 pos(0.f, 0.f), size;
	float tab_height = m_pstyle->get_tab_height();
	rm_vec2 tab_up_offsets = m_pstyle->get_tab_up_offsets();
	bool b_is_selected = false;
	/* draw tab rows */
	for (size_t rowi = 0; rowi < get_num_rows(); rowi++) {
		auto& row = get_tab_row(rowi);
		pos.x = 0.f;
		pos.y = float(rowi * tab_height);
		for (size_t tabi = 0; tabi < row.get_num_tabs(); tabi++) {
			tab* ptab = row.get_tab(tabi);
			size.x = ptab->get_width();
			size.y = tab_height;
			b_is_selected = m_active_row == rowi && m_active_tab == tabi;
			/* draw tab background */
			tab_path(pctx, pos.x, pos.y, size.x, size.y,
				m_pstyle->get_tab_up_offsets(), m_pstyle->get_tab_corners_radius());
			//if m_active_row==rowi && m_active_tab==tabi - it is selected tab
			pctx->fillColor(m_pstyle->get_state_color(b_is_selected)); //set state color and set to drawings tab
			pctx->fill();

			/* draw tab edge */
			rm_color shadow_clr(42, 42, 42);
			rm_color sun_clr(100, 100, 100);
			draw_tab_edge(pctx, pos, size, m_pstyle->get_tab_up_offsets(),
				m_pstyle->get_tab_corners_radius(), m_pstyle, sun_clr, shadow_clr);

			/* draw tab buttons */
			float button_size = m_pstyle->get_tab_buttons_size();
			for (size_t j = 0; j < ptab->m_buttons.size(); j++) {
				rm_vec2 pos(ptab->m_textsize.x + m_pstyle->get_text_offsets().x, tab_height / 2.f);
				rm_tab_button& btn = ptab->m_buttons[j];
				btn.draw(pctx, pos, button_size, m_pstyle);
			}

			/* draw tab name */
			pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
			pctx->fillColor(m_pstyle->get_text_color());
			pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			float xoffset = m_pstyle->get_text_offsets().x;
			if (xoffset < tab_up_offsets.x)
				xoffset = tab_up_offsets.x;

			pctx->text(pos.x + xoffset, pos.y + size.y / 2.f, ptab->get_name().c_str(), nullptr);
			//if (b_is_selected)
			//  pctx->IntersectScissor( pos.x, pos.y + size.y, size.x, size.y);

			pos.x += size.x + ktab_spacing;
		}
	}
	rm_widget::on_draw(pctx);
}

bool rm_tabcontrol_ex::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	tab* ptab;
	size_t row, tab;
	rm_vec2 mouse = cursor_to_local(cursor_pos);
	//printf("mousepos: %f %f\n", mouse.x, mouse.y);
	ptab = get_tab_at_cursor(row, tab, mouse);
	if (ptab) {
		if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
			select_tab(row, tab);
			return false;
		}

		//printf("TAB FOUND!\n");
	}
	return true;
}

void rm_tabcontrol_ex::hide_all_except(size_t row, size_t tabidx)
{
	tab* ptab;
	rm_widget* ppage;
	bool       page_state;
	for (size_t rowi = 0; rowi < get_num_rows(); rowi++) {
		tab_row& rowref = get_tab_row(rowi);
		for (size_t tabi = 0; tabi < rowref.get_num_tabs(); tabi++) {
			ptab = rowref.get_tab(tabi);
			ppage = ptab->get_page_widget();
			assert(ppage && "ppage was nullptr");
			page_state = rowi == row && tabi == tabidx;
			ppage->set_enabled(page_state);
			ppage->show(page_state);
		}
	}
}

void rm_tabcontrol_ex::get_text_bounds(rm_vec2& dstsize, const char* ptabname)
{
	rm_bbox text_bounds;
	assert(m_pstyle && "m_pstyle was nullptr!");
	assert(m_proot && "m_proot was nullptr!");
	/* get text size */
	NVGcontext* pctx = m_proot->get_context();
	assert(pctx && "pctx was nullptr!");
	pctx->save();
	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
	pctx->textBounds(0.f, 0.f, ptabname, nullptr, text_bounds.array);
	pctx->restore();

	/* clamp height */
	dstsize.x = text_bounds.get_width();
	dstsize.y = rm_min(text_bounds.get_height(), m_pstyle->get_tab_height());
}

size_t rm_tabcontrol_ex::find_free_row_or_create(const char* ptabname)
{
	rm_vec2 size;
	get_text_bounds(size, ptabname);

	/* find tab with available space */
	//constexpr float minimum_size_to_downwidth = 10.f; /*< this constant defines the minimum allowable deviation of the remaining size to reduce the width of the tab and fit it in this line  */
	for (size_t rowi = 0; rowi < m_tab_rows.size(); rowi++) {
		float remaining_row_width = m_size.x;
		tab_row& row = m_tab_rows[rowi];
		for (auto ptab : row.m_tabs) {
			assert(ptab && "ptab was nullptr! this is abnormal!");
			remaining_row_width -= ptab->get_width();
		}
		remaining_row_width -= size.x;
		if (remaining_row_width >= 0.f) {
			return rowi;
		}
	}

	/* create new row */
	RM_HANDLE_EXCEPTIONS(invalid_index::ROW,
		m_tab_rows.push_back(tab_row());
	)
		return m_tab_rows.size() - 1;
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::get_tab_at_cursor(size_t& dstrow,
	size_t& dsttab, rm_vec2& local_cursor)
{
	assert(m_pstyle && "m_pstyle was nullptr!");
	rm_vec2 pos, size;
	float   tab_height;
	//float   rows_height;
	tab_height = m_pstyle->get_tab_height();
	/* for horizontal tabs */
	if (is_horizontal()) {
		for (size_t i = 0; i < get_num_rows(); i++) {
			tab_row& row = get_tab_row(i);
			pos.init(0.f, float(i * tab_height));
			size.init(m_size.x, tab_height);
			/* check for cursor inside in each row */
			if (rm_bbox(pos, size).inside(local_cursor)) {
				/* find tab */
				rm_vec2 tabpos(0.f, pos.y);
				for (size_t j = 0; j < row.get_num_tabs(); j++) {
					tab* ptab = row.get_tab(j);
					assert(ptab && "ptab was nullptr!");
					if (rm_bbox(tabpos, { tabpos.x + ptab->get_width(), tabpos.y + tab_height }).inside(local_cursor)) {
						dstrow = i;
						dsttab = j;
						/* tab found */
						return ptab;
					}
					tabpos.x += ptab->get_width() + ktab_spacing;
				}
			}
		}
	}

	/* for vertical tabs */
	return nullptr; //TODO: K.D. IMPL THIS
}

rm_tabcontrol_ex::rm_tabcontrol_ex(rm_widget* p_parent,
	int x, int y, int width, int height,
	uint32_t tab_flags, uint32_t tab_type, rm_tabcontrol_ex_style* pstyle, size_t num_rows) :
	rm_widget(x, y, width, height, p_parent, "ui_kdtabcontrol", RM_FLAG_DEFAULT, tab_flags),
	m_active_row(invalid_index::ROW), m_active_tab(invalid_index::TAB)
{
	assert(is_valid_type(tab_type) && "tab type is invalid!");
	set_style(pstyle);
	set_num_rows(num_rows);
	m_type = tab_type;
}

bool rm_tabcontrol_ex::set_num_rows(size_t newsize)
{
	RM_HANDLE_EXCEPTIONS(false,
		m_tab_rows.resize(newsize);
		)
		return true;
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::find_tab_in_row(size_t rowidx, const char* pname)
{
	tab_row& row = get_tab_row(rowidx);
	auto it = std::find_if(row.m_tabs.begin(), row.m_tabs.end(), [pname](const tab* ptab) {
		return !strcmp(ptab->get_name().c_str(), pname);
		});
	if (it != row.m_tabs.end())
		return *it;

	return nullptr;
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::find_tab_in_row(size_t rowidx, uint32_t tabid)
{
	tab_row& row = get_tab_row(rowidx);
	auto it = std::find_if(row.m_tabs.begin(), row.m_tabs.end(), [tabid](const tab* ptab) {
		return ptab->get_id() == tabid;
		});
	if (it != row.m_tabs.end())
		return *it;

	return nullptr;
}

size_t rm_tabcontrol_ex::find_tab_idx_in_row(size_t rowidx, const char* pname)
{
	tab_row& row = get_tab_row(rowidx);
	for (size_t i = 0; i < row.m_tabs.size(); i++) {
		auto ptab = row.m_tabs[i];
		if (!strcmp(ptab->get_name().c_str(), pname)) {
			return i;
		}
	}
	return invalid_index::TAB;
}

size_t rm_tabcontrol_ex::find_tab_idx_in_row(size_t rowidx, uint32_t tabid)
{
	tab_row& row = get_tab_row(rowidx);
	for (size_t i = 0; i < row.m_tabs.size(); i++) {
		auto ptab = row.m_tabs[i];
		if (ptab->get_id() == tabid) {
			return i;
		}
	}
	return invalid_index::TAB;
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::add_tab(const char* pname,
	uint32_t tabid,
	float width,
	void* puserptr,
	size_t insert_after,
	size_t row_index)
{
	page* ppage = new (std::nothrow)page(this, { 0.f, 0.f }, { 0.f, 0.f });
	if (!ppage)
		return nullptr;

	return add_tab_widget(pname, tabid, ppage, width, puserptr, insert_after, row_index);
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::add_tab_widget(const char* pname,
	uint32_t tabid,
	rm_widget* pwidget,
	float width,
	void* puserptr,
	size_t insert_after,
	size_t row_index)
{
	tab* ptab;
	if (row_index == invalid_index::ROW)
		row_index = find_free_row_or_create(pname);

	tab_row& row = get_tab_row(row_index);
	ptab = row.new_tab(this, pname, tabid, width, nullptr, insert_after, puserptr, tab::FNONE);
	if (!ptab)
		return nullptr;

	float rows_height = get_rows_total_height();
	pwidget->move({ 0.f, rows_height });
	pwidget->resize({ m_size.x, m_size.y - rows_height });
	ptab->m_pwidget = pwidget;
	select_tab(0, 0);
	return ptab;
}

bool rm_tabcontrol_ex::select_tab(size_t rowidx, size_t tabidx)
{
	size_t temp_row_select;
	if (rowidx >= get_num_rows())
		return false;

	temp_row_select = rowidx;
	tab_row& row = get_tab_row(temp_row_select);
	if (tabidx >= row.get_num_tabs())
		return false;

	m_active_row = temp_row_select;
	m_active_tab = tabidx;
	hide_all_except(m_active_row, m_active_tab);
	return true;
}

void rm_tabcontrol_ex::rm_tab_button::draw(NVGcontext* pctx, rm_vec2& pos,
	float size, rm_tabcontrol_ex_style* pstyle)
{
	const rm_color& bg_color = pstyle->get_background_color();
	const rm_color& border_color = pstyle->get_border_color();
	rm_color background, border;
	if (is_hovered()) {
		background = bg_color.negative();
		border = bg_color.negative();
	}
	else {
		background = bg_color;
		border = bg_color;
	}
	pctx->beginPath();
	pctx->setFontFaceId(((int)m_font.getValue())); //FIXME: wait fontstash refactoring!
	pctx->fillColor(background);
	pctx->strokeColor(border);
	pctx->rect(pos.x, pos.y, size, size);
	pctx->fill();
	pctx->stroke();
	pctx->fillColor(pstyle->get_text_color());
	pctx->text(pos.x, pos.y + size / 2.f, m_icon_sym, nullptr);
}

void rm_tabcontrol_ex::page::on_draw(NVGcontext* pctx)
{
	const rm_tabcontrol_ex_style* pcurrstyle = get_style();
	pctx->beginPath();
	pctx->fillColor(pcurrstyle->get_background_color());
	pctx->strokeColor(pcurrstyle->get_border_color());
	pctx->roundedRectVarying(
		0.f, 0.f, m_size.x, m_size.y,
		pcurrstyle->get_top_left(),
		pcurrstyle->get_top_right(),
		pcurrstyle->get_bottom_right(),
		pcurrstyle->get_bottom_left());
	pctx->fill();
	pctx->stroke();

	rm_color shadow_clr(42, 42, 42);
	rm_color sun_clr(100, 100, 100);
	rm_utl::draw_edge(pctx, { 0.f, 0.f }, m_size, pcurrstyle, sun_clr, shadow_clr);
	rm_widget::on_draw(pctx);
}

rm_tabcontrol_ex::tab* rm_tabcontrol_ex::tab_row::new_tab(
	rm_tabcontrol_ex* pcontrol,
	const char* pname,
	uint32_t tabid,
	float width,
	rm_widget* ppage_widget,
	size_t insert_after,
	void* puserptr, uint32_t flags)
{
	tab* ptab = new (std::nothrow)tab(pcontrol, width,
		pname, tabid, ppage_widget, puserptr, flags);
	if (!ptab)
		return nullptr;

	/* insert tab to container */
	if (insert_after == invalid_index::TAB || insert_after >= m_tabs.size()) {
		/* insert back by default if tab index not set */
		m_tabs.push_back(ptab);
	}
	else {
		/* try to insert at 'insert_after' */
		try {
			m_tabs.emplace(m_tabs.begin() + insert_after, ptab);
		}
		catch (...) {
			assert("std::vector<_Ty>::emplace(...) raised exception");
			delete ptab;
			return nullptr;
		}
	}
	return ptab;
}

void rm_tab_drawer::tab_path(NVGcontext* pctx,
	float x, float y,
	float w, float h,
	rm_vec2 up_offsets, float r)
{
	rm_vec2 pos(x, y);
	rm_vec2 size(w, h);
	float xBL = pos.x;
	float yBL = pos.y + size.y;
	float xBR = pos.x + size.x;
	float yBR = pos.y + size.y;
	float xTR = pos.x + size.x + up_offsets.y;
	float yTR = pos.y;
	float xTL = pos.x + up_offsets.x;
	float yTL = pos.y;

	pctx->beginPath();
	pctx->moveTo(xBL, yBL);
	pctx->lineTo(xBR, yBR);
	pctx->arcTo(
		xTR, yTR,
		xTL, yTL,
		r);
	pctx->arcTo(
		xTL, yTL,
		xBL, yBL,
		r);
	pctx->closePath();
}

void rm_tab_drawer::draw_tab_edge(NVGcontext* pctx, rm_vec2 pos, rm_vec2& size, rm_vec2 up_offsets,
	float r, const rm_corners_style* pcstyle, const rm_color& suncolor, const rm_color& shadowcolor)
{
	/* light */
	tab_path(pctx, pos.x, pos.y + 1.f, size.x, size.y, up_offsets, r);
	pctx->strokeColor(suncolor);
	pctx->StrokeWidth(1.f);
	pctx->stroke();

	/* light */
	tab_path(pctx, pos.x, pos.y, size.x, size.y, up_offsets, r);
	pctx->strokeColor(shadowcolor);
	pctx->stroke();
}

void rm_tabcontrol_ex::tab::width_recompute()
{
	float buttons_total_width;
	float text_region_width;
	float total_width;
	float tab_up_offsets;
	rm_tabcontrol_ex_style* pstyle;
	assert(m_powner && "m_powner was nullptr");
	assert(m_powner->get_style() && "m_powner->get_style() returned nullptr");
	/* get text bounds */
	pstyle = m_powner->get_style();
	m_powner->get_text_bounds(m_textsize, m_name.c_str());
	buttons_total_width = pstyle->get_tab_buttons_spacing() +
		(pstyle->get_tab_buttons_size() + pstyle->get_tab_buttons_spacing()) * float(m_buttons.size());
	text_region_width = pstyle->get_text_offsets().x * 2.f + m_textsize.x; //NOTE: K.D. "txtoffs * 2.f" - for left and right text spacing
	tab_up_offsets = rm_abs(pstyle->get_tab_up_offsets().x) + rm_abs(pstyle->get_tab_up_offsets().y);
	total_width = text_region_width + buttons_total_width + tab_up_offsets;
	/* update m_width only for greater sizes */
	if (total_width > m_width)
		m_width = total_width;
}

rm_tabcontrol_ex::rm_tab_button* rm_tabcontrol_ex::tab::add_button(rm_font font, uint32_t id, const char* putf8str)
{
	RM_HANDLE_EXCEPTIONS(nullptr,
		m_buttons.push_back({ font, id , putf8str });
		)
		width_recompute();
	return &m_buttons[m_buttons.size() - 1];
}

void rm_tabcontrol_ex::tab::set_name(const char* pname)
{
	assert(pname && "pname was nullptr!");
	m_name.assign(pname);
	width_recompute();
}

rm_menu* rm_menu::create_submenu(const char* pname,
	uint32_t menuid,
	uint32_t itemid,
	uint32_t flags)
{
	rm_menu* pmenu = new (std::nothrow)rm_menu(this, int(this->m_size.y), pname);
	if (!pmenu)
		return nullptr;

	pmenu->m_flags = MF_NONE;
	pmenu->m_menuid = menuid;
	pmenu->m_itemid = itemid;

	/* noname == menu separator */
	if (!pname)
		pmenu->m_flags |= MF_SEPARATOR;

	if (m_pparent->classname_is("ui_menu")) {
		m_max_text_width = recompute_text_width();
		resize(m_max_text_width, m_size.y);
	}
	m_proot_menu->hide_all_submenus_except(nullptr);
	return pmenu;
}

size_t rm_menu::get_num_submenus()
{
	return rm_widget::get_num_childs();
}

rm_menu* rm_menu::get_submenu(size_t idx)
{
	if (idx >= get_num_submenus()) {
		return nullptr;
	}

	rm_widget* child = rm_widget::get_child(idx);
	if (!child || !child->classname_is("ui_menu")) {
		return nullptr;
	}

	return static_cast<rm_menu*>(child);
}

/**
 detect_my_level()
 @brief Determines the level of the current menu, hierarchically relative to the parent widget.
 There are only two menu levels.
 The zero-level menu is the menu that is drawn across the entire top border of the window, allowing for the addition of submenus.
 All higher levels are drawn in the same way.
 These menus can have separators, child submenus, and other things that will not be structurally different higher up in the hierarchy.
 @param pparent - address of parent widget
 @return Level of hierarchy depth
*/
uint32_t rm_menu::detect_my_level(rm_widget* pparent)
{
	uint32_t   depth = 0;
	rm_widget* pwidget = pparent;
	assert(pparent && "pparent was nullptr!");
	static constexpr const char* MENU_CLASS = "ui_menu";
	if (!pparent->classname_is(MENU_CLASS)) {
		assert(pparent->find_child(MENU_CLASS) == this && "ui_menu already exists in pparent! check your code or assign hierarchy");
		return 0;
	}

	while (pwidget && pwidget->classname_is(MENU_CLASS)) {
		depth++;
		pwidget = pwidget->get_parent();
	}
	return depth;
}

//TODO: K.D. add this to styles
constexpr float menu_text_pad = 5.f;

void rm_menu::on_draw(NVGcontext* pctx)
{
	rm_vec2  pos;
	float    item_width;
	float    half_height = m_size.y / 2.f;
	size_t   num_submenus = get_num_submenus();
	rm_menu* pmenu_item;
	/* draw menu in top of window */
	pos.init(0.f, 0.f);
	if (!m_level) {
		static rm_color menu_background(70, 70, 70);
		static rm_color menu_hover(100, 100, 100);

		/* draw background */
		pctx->beginPath();
		pctx->rect(0.f, 0.f, m_size.x, m_size.y);
		pctx->fillColor(menu_background);
		pctx->fill();
		pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
		pctx->setTextAlign(NVG_ALIGN_MIDDLE);
		for (size_t i = 0; i < num_submenus; i++) {
			pmenu_item = get_submenu(i);
			assert(pmenu_item && "pmenu_item was nullptr!");
			item_width = menu_text_pad + pmenu_item->m_text_width + menu_text_pad;
			if (pmenu_item->is_navigated()) {
				pctx->beginPath();
				pctx->fillColor(rm_color(180, 180, 180));
				pctx->rect(pos.x, pos.y, item_width, m_size.y);
				pctx->fill();
			}

			pctx->fillColor(rm_color(255, 255, 255));
			pctx->text(
				menu_text_pad + pos.x,
				pos.y + half_height,
				pmenu_item->m_text.c_str(),
				nullptr);
			pos.x += item_width;
		}
	}
	else {
		/* draw popup menu */
		pctx->beginPath();
		pctx->fillColor(rm_color(120, 120, 120));
		pctx->strokeColor(rm_color(150, 150, 150));
		pctx->rect(0.f, 0.f, m_size.x, m_size.y);
		pctx->fill();
		pctx->stroke();
		pos.init(0.f, 0.f);
		for (size_t i = 0; i < num_submenus; i++) {
			pmenu_item = get_submenu(i);
			assert(pmenu_item && "pmenu_item was nullptr!");
			if (pmenu_item->is_navigated()) {
				pctx->beginPath();
				pctx->fillColor(rm_color(150, 150, 150));
				pctx->rect(pos.x, pos.y, m_size.x, m_size.y);
				pctx->fill();
			}

			pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
			pctx->setTextAlign(NVG_ALIGN_MIDDLE);
			pctx->fillColor(rm_color(255, 255, 255));
			pctx->text(pos.x, pos.y + half_height, pmenu_item->m_text.c_str(), nullptr);
			pos.y += 25.f;
		}
	}
	rm_widget::on_draw(pctx);
}

bool rm_menu::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	/* handle root */
	rm_vec2 pos;
	float   item_width;
	rm_vec2 local = cursor_to_local(cursor_pos);
	rm_menu* psubmenu;
	if (m_bbox.inside(cursor_pos)) {
		if (state == DOWN) {
			if (!m_level) {
				/* handle root menu */
				for (size_t i = 0; i < get_num_submenus(); i++) {
					psubmenu = get_submenu(i);
					item_width = menu_text_pad + psubmenu->m_text_width + menu_text_pad;
					bool over_header = (local.y >= 0 && local.y < m_size.y &&
						local.x >= 0.f && local.x < 0.f + item_width);
					bool in_submenu = pos.x <= local.x && local.x < pos.x + item_width;
					if (event == RM_MOUSE_EVENT_MOVE) {
						if (!over_header)// нужно что бы для каждого так работало, щас ток для первого File (короче это должно быть не скрытие, а открытие след меню при условии если меню какое либо уже открыто посмотри у визуалки как)
							psubmenu->hide();

						if (psubmenu->is_visible() && !over_header) {
							psubmenu->show();
						}

						psubmenu->set_navigated(in_submenu);
					}
					else if (event == RM_MOUSE_EVENT_CLICK && in_submenu) {
						if (!psubmenu->is_visible()) {
							psubmenu->move({ pos.x, m_size.y });
							hide_all_submenus_except(psubmenu);
						}
						else {
							psubmenu->hide();
						}
					}

					pos.x += item_width;
				}
				return false;
			}
			else {
				/* handle child menu */

				return false;
			}
		}
	}
	else {
		for (size_t i = 0; i < get_num_submenus(); i++) {
			psubmenu = get_submenu(i);
			psubmenu->set_navigated(false);

			if (event == RM_MOUSE_EVENT_CLICK && state == UP)
				psubmenu->hide();
		}
	}

	return true;
}

bool rm_menu::add_submenu(rm_menu* pmenu)
{
	m_max_text_width = rm_max(m_max_text_width, pmenu->m_text_width);
	return rm_widget::add_child(pmenu);
}

float rm_menu::recompute_text_width()
{
	float width = 0.f;
	for (size_t i = 0; i < get_num_submenus(); i++) {
		rm_menu* pmenu = get_submenu(i);
		width = rm_max(width, pmenu->m_text_width);
	}
	return width;
}

void rm_menu::hide_all_submenus_except(rm_menu* psubmenu)
{
	rm_menu* pmenu;
	for (size_t i = 0; i < get_num_submenus(); i++) {
		pmenu = get_submenu(i);
		assert(pmenu && "pmenu was nullptr");
		pmenu->show(pmenu == psubmenu);
	}
}

rm_menu::rm_menu(rm_widget* p_parent, int height, const char* pname) :
	rm_widget(0, 0, 0, height, p_parent, "ui_menu", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL),
	m_text(pname ? pname : ""), m_max_text_width(0.f), m_proot_menu(nullptr)
{
	/* detect menu level from parent */
	assert(p_parent && "p_parent was nullptr!");
	assert(m_proot && "m_proot was nullptr!");
	m_level = detect_my_level(p_parent);
	m_text_width = m_proot->get_text_width(m_text.c_str(), get_font());
	if (!m_level) {
		m_elem_flags.set_bit(RM_FLAG_DISABLE_SCISSOR);
		m_proot_menu = this;
		move({ 0.f, 0.f });
		resize({ p_parent->get_size().x, float(height) });
		return;
	}
	else {
		/* */
		//m_elem_flags.set_bit(RM_FLAG_HIGHEST_PRIORITY);
		rm_menu* parent_menu = static_cast<rm_menu*>(p_parent);
		m_proot_menu = parent_menu->m_proot_menu;
	}
}

std::map<const rm_widget*, std::vector<rm_radiobutton*>> rm_radiobutton::s_groups; // NOTE: d2 mb need refactoring this..

rm_radiobutton::rm_radiobutton(rm_widget* parent, int x, int y, int width, int height,
	rm_radiobutton_style* pstyle, const std::string& label, rm_radiobutton_cb cb) :
	rm_widget(x, y, width, height, parent, "ui_radiobutton"),
	m_label(label), m_checked(false), m_allow_uncheck(false) {
	set_style(pstyle);
	set_callback(cb);
	assert(pstyle && "pstyle must not be null");
	auto& grp = s_groups[parent];
	grp.push_back(this);
	if (grp.size() == 1)
		m_checked = true;
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

void rm_radiobutton::select_default(rm_widget* parent, int index) {
	auto it = s_groups.find(parent);

	if (it == s_groups.end())
		return;

	auto& grp = it->second;
	for (size_t i = 0; i < grp.size(); ++i)
		grp[i]->m_checked = (i == (size_t)index);
}

void rm_radiobutton::select_by_label(rm_widget* parent, const std::string& label) {
	auto it = s_groups.find(parent);

	if (it == s_groups.end())
		return;

	for (auto* rb : it->second)
		rb->m_checked = (rb->m_label == label);
}

void rm_radiobutton::on_draw(NVGcontext* pctx) {
	rm_radiobutton_style& style = *get_style();
	float w = (float)m_size.x;
	float h = (float)m_size.y;
	float circle_radius = style.get_circle_radius();

	/* draw shadow */
	rm_utl::draw_shadow(pctx, rm_vec2(0.f, 0.f), m_size, rm_vec2(0.f, 1.0f), style.get_shadow_offset(), style.get_shadow_color(), style.get_shadow_size(), style.get_avg_radius());

	/* draw bg */
	pctx->beginPath();
	pctx->roundedRectVarying(
		0.5f, 0.5f, w - 1.0f, h - 1.0f,
		style.get_corner_radius(LEFT_TOP),
		style.get_corner_radius(RIGHT_TOP),
		style.get_corner_radius(RIGHT_BOTTOM),
		style.get_corner_radius(LEFT_BOTTOM));
	pctx->fillColor(style.get_bg_inner());
	pctx->fill();

	float cy = h * 0.5f;
	float cx = circle_radius + 5.f;

	if (m_checked) {
		float w_o = style.get_border_width_outer();
		float w_i = style.get_border_width_inner();

		/* draw active outer border */
		float r_o = circle_radius - w_o * 0.5f;
		pctx->beginPath();
		pctx->circle(cx, cy, r_o);
		pctx->StrokeWidth(w_o);
		pctx->strokeColor(style.get_border_active_outer());
		pctx->stroke();

		/* draw active inner border */
		float r_i = r_o - w_o * 0.5f - w_i * 0.5f;
		pctx->beginPath();
		pctx->circle(cx, cy, r_i);
		pctx->StrokeWidth(w_i);
		pctx->strokeColor(style.get_border_active_inner());
		pctx->stroke();

		/* draw active mark */
		float r_fill = r_i - w_i * 0.5f;
		pctx->beginPath();
		pctx->circle(cx, cy, r_fill);
		pctx->fillColor(style.get_mark_color());
		pctx->fill();
	}
	else {
		/* draw inactive border */
		float w_n = style.get_border_width_inactive();
		float r_n = circle_radius - w_n * 0.5f;
		pctx->beginPath();
		pctx->circle(cx, cy, r_n);
		pctx->StrokeWidth(w_n);
		pctx->strokeColor(style.get_border_inactive());
		pctx->stroke();
	}
	float circle_right = cx + circle_radius + style.get_border_width_outer();
	float bounds[4];

	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
	pctx->setFontSize(style.get_font_size());
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	pctx->textBounds(0, 0, m_label.c_str(), nullptr, bounds);

	float textWidth = bounds[2] - bounds[0];
	float avail = w - circle_right;
	float tx = circle_right + (avail - textWidth) / 2.0f;
	rm_vec2 offs = style.get_text_offset();
	tx += offs.x;
	float ty = cy + offs.y;

	pctx->fillColor(style.get_text_color());
	pctx->text(tx, ty, m_label.c_str(), nullptr);

	rm_widget::on_draw(pctx);
}

bool rm_radiobutton::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& pos, rm_vec2 delta)
{
	if (event == RM_MOUSE_EVENT_CLICK && state == UP && m_bbox.inside(pos)) {
		if (m_checked) {
			if (m_allow_uncheck) {
				m_checked = false;
			}
			else {
				return false;
			}
		}
		else {
			auto& grp = s_groups[m_pparent];
			for (auto* rb : grp)
				rb->m_checked = false;
			m_checked = true;
		}
		if (is_valid_callback())
			get_callback()(this);
		return false;
	}
	return true;
}

rm_switch::rm_switch(rm_widget* parent, int x, int y, int width,
	bool initial, rm_switch_cb cb, RmThemeRef theme) :
	rm_widget(x, y, width, static_cast<int>((theme ? theme : RmTheme::default_theme())->switch_control.track_height),
		parent, "ui_switch"),
	m_behaviour(initial), m_theme(theme ? std::move(theme) : RmTheme::default_theme())
{
	set_callback(cb);
}

rm_switch::rm_switch(rm_widget* parent, int x, int y, int width,
	rm_switch_style* pstyle, bool initial, rm_switch_cb cb) :
	rm_switch(parent, x, y, width, initial, cb, theme_from_legacy_switch(pstyle))
{}

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

rm_listview::rm_listview(rm_widget* parent, int x, int y, int width, int height, rm_listview_style* pstyle, rm_listview_cb cb) :
	rm_widget(x, y, width, height, parent, "ui_listview", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL),
	m_hover_index((size_t)-1), m_selected_index((size_t)-1)
{
	set_style(pstyle);
	set_callback(cb);
	assert(pstyle && "style must not be null");
}

void rm_listview::add_item(const std::string& text)
{
	m_items.push_back(text);
	float h = m_items.size() * get_style()->get_row_height();
	resize({ m_size.x, h });
}

void rm_listview::clear_items()
{
	m_items.clear();
	m_hover_index = m_selected_index = (size_t)-1;
	resize({ m_size.x, 0.f });
}

void rm_listview::on_draw(NVGcontext* pctx)
{
	rm_listview_style& style = *get_style();
	/* draw background */
	pctx->beginPath();
	pctx->rect(0, 0, m_size.x, m_size.y);
	pctx->fillColor(style.get_background_color());
	pctx->fill();

	/* draw items */
	pctx->setFontFaceId(((int)get_font().getValue())); //FIXME: wait fontstash refactoring!
	pctx->setFontSize(style.get_font_size());
	pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

	float row_h = style.get_row_height();
	float pad = style.get_text_padding();
	for (size_t i = 0; i < m_items.size(); ++i) {
		float y0 = i * row_h;
		/* background for hover/selected */
		if (i == m_selected_index) {
			pctx->beginPath();
			pctx->rect(0, y0, m_size.x, row_h);
			pctx->fillColor(style.get_selected_color());
			pctx->fill();
		}
		else if (i == m_hover_index) {
			pctx->beginPath();
			pctx->rect(0, y0, m_size.x, row_h);
			pctx->fillColor(style.get_hover_color());
			pctx->fill();
		}
		pctx->fillColor(style.get_text_color());
		pctx->text(pad, y0 + row_h * 0.5f,
			m_items[i].c_str(), nullptr);
	}

	rm_widget::on_draw(pctx);
}

bool rm_listview::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta)
{
	rm_listview_style& style = *get_style();
	rm_vec2 local = cursor_to_local(cursor_pos);
	float row_h = style.get_row_height();
	size_t idx = size_t(local.y / row_h);
	if (local.x < 0 || local.x > m_size.x || idx >= m_items.size()) {
		m_hover_index = (size_t)-1;
	}
	else {
		m_hover_index = idx;
	}

	if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
		if (m_bbox.inside(cursor_pos)) {
			if (m_hover_index < m_items.size()) {
				m_selected_index = m_hover_index;
				if (is_valid_callback())
					get_callback()(this, m_selected_index);
			}
		}
		else {
			m_selected_index = (size_t)-1;
		}
		return false;
	}
	return true;
}
