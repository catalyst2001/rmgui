#include "blend_ui.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// Helper: map rm_widget hover/active state to BNDwidgetState
static BNDwidgetState bui_widget_state(const rm_widget* w, bool is_active = false) {
    // We can't call non-const on w, so cast away const for the flag check
    auto* mw = const_cast<rm_widget*>(w);
    if (is_active || mw->get_elem_flags().is_focused())
        return BND_ACTIVE;
    if (mw->get_elem_flags().is_hovered())
        return BND_HOVER;
    return BND_DEFAULT;
}

// ============================================================================
// bui_tooltip
// ============================================================================
bui_tooltip::bui_tooltip(rm_widget* p_parent, const std::string& text,
    int iconid, float delay)
    : rm_widget(0, 0, 200, (int)BND_WIDGET_HEIGHT, p_parent, "bui_tooltip",
        RM_FLAG_DEFAULT | RM_FLAG_DISABLE_SCISSOR | RM_FLAG_HIGHEST_PRIORITY)
    , m_text(text), m_icon(iconid), m_delay(delay)
    , m_timer(0.f), m_show(false), m_target(nullptr)
{
    hide();
}

void bui_tooltip::on_draw(NVGcontext* pctx) {
    if (!m_show || m_text.empty())
        return;

    float w = bndLabelWidth(pctx, m_icon, m_text.c_str()) + 16.f;
    float h = (float)BND_WIDGET_HEIGHT;

    bndTooltipBackground(pctx, 0, 0, w, h);
    bndLabel(pctx, 0, 0, w, h, m_icon, m_text.c_str());
}

void bui_tooltip::show_at(float x, float y) {
    m_show = true;
    show(true);
    move(rm_vec2(x, y));
}

void bui_tooltip::dismiss() {
    m_show = false;
    hide();
}

// ============================================================================
// bui_toolbar
// ============================================================================
bui_toolbar::bui_toolbar(rm_widget* p_parent, int x, int y, int width, int height,
    bool vertical, bui_toolbar_cb cb)
    : rm_widget(x, y, width, height, p_parent, "bui_toolbar")
    , m_hover_id(-1), m_active_id(-1), m_vertical(vertical)
{
    set_callback(cb);
}

void bui_toolbar::add_button(int id, int iconid, const char* label) {
    bui_tool_item item;
    item.id = id;
    item.iconid = iconid;
    item.label = label ? label : "";
    item.separator = false;
    m_items.push_back(item);
}

void bui_toolbar::add_separator() {
    bui_tool_item item;
    item.id = -1;
    item.iconid = -1;
    item.separator = true;
    m_items.push_back(item);
}

int bui_toolbar::hit_test_item(const rm_vec2& local) const {
    float pos = 0.f;
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].separator) {
            pos += 5.f;
            continue;
        }
        float btn_w = m_items[i].label.empty()
            ? (float)BND_TOOL_WIDTH
            : (float)BND_TOOL_WIDTH + 60.f;
        float btn_h = (float)BND_WIDGET_HEIGHT;

        if (m_vertical) {
            if (local.y >= pos && local.y < pos + btn_h &&
                local.x >= 0 && local.x < m_size.x)
                return m_items[i].id;
            pos += btn_h - 1.f;
        } else {
            if (local.x >= pos && local.x < pos + btn_w &&
                local.y >= 0 && local.y < m_size.y)
                return m_items[i].id;
            pos += btn_w - 1.f;
        }
    }
    return -1;
}

void bui_toolbar::on_draw(NVGcontext* pctx) {
    // Draw background
    bndBackground(pctx, 0, 0, m_size.x, m_size.y);

    float pos = 0.f;
    for (size_t i = 0; i < m_items.size(); i++) {
        const auto& item = m_items[i];
        if (item.separator) {
            pos += 5.f;
            continue;
        }

        float btn_w = item.label.empty()
            ? (float)BND_TOOL_WIDTH
            : (float)BND_TOOL_WIDTH + 60.f;
        float btn_h = (float)BND_WIDGET_HEIGHT;

        BNDwidgetState state = BND_DEFAULT;
        if (item.id == m_active_id)
            state = BND_ACTIVE;
        else if (item.id == m_hover_id)
            state = BND_HOVER;

        // Compute corner flags for grouped buttons
        int corners = BND_CORNER_NONE;
        bool has_prev = (i > 0 && !m_items[i - 1].separator);
        bool has_next = (i + 1 < m_items.size() && !m_items[i + 1].separator);

        if (m_vertical) {
            if (has_prev) corners |= BND_CORNER_TOP;
            if (has_next) corners |= BND_CORNER_DOWN;
            bndToolButton(pctx, 0, pos, m_size.x, btn_h, corners, state,
                item.iconid, item.label.empty() ? nullptr : item.label.c_str());
            pos += btn_h - 1.f;
        } else {
            if (has_prev) corners |= BND_CORNER_LEFT;
            if (has_next) corners |= BND_CORNER_RIGHT;
            bndToolButton(pctx, pos, 0, btn_w, btn_h, corners, state,
                item.iconid, item.label.empty() ? nullptr : item.label.c_str());
            pos += btn_w - 1.f;
        }
    }
    rm_widget::on_draw(pctx);
}

bool bui_toolbar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    m_hover_id = m_bbox.inside(cursor_pos) ? hit_test_item(local) : -1;

    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        if (m_hover_id >= 0) {
            m_active_id = m_hover_id;
            if (is_valid_callback())
                get_callback()(this, m_active_id);
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_color_button
// ============================================================================
bui_color_button::bui_color_button(rm_widget* p_parent, int x, int y, int w, int h,
    NVGcolor color, int corner_flags, bui_color_button_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_color_button")
    , m_color(color), m_corner_flags(corner_flags)
{
    set_callback(cb);
}

void bui_color_button::on_draw(NVGcontext* pctx) {
    bndColorButton(pctx, 0, 0, m_size.x, m_size.y, m_corner_flags, m_color);
    rm_widget::on_draw(pctx);
}

bool bui_color_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        if (m_bbox.inside(cursor_pos)) {
            if (is_valid_callback())
                get_callback()(this, m_color);
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_panel
// ============================================================================
bui_panel::bui_panel(rm_widget* p_parent, int x, int y, int w, int h,
    const char* title, int iconid, bui_panel_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_panel")
    , m_title(title ? title : ""), m_icon(iconid)
    , m_collapsed(false), m_header_height((float)BND_WIDGET_HEIGHT)
    , m_expanded_height((float)h)
{
    set_callback(cb);
}

void bui_panel::on_draw(NVGcontext* pctx) {
    float w = m_size.x;
    float h = m_size.y;

    // Background
    bndBackground(pctx, 0, 0, w, h);
    bndBevel(pctx, 0, 0, w, h);

    // Header - draw as a tool button
    BNDwidgetState hdr_state = get_elem_flags().is_hovered() ? BND_HOVER : BND_DEFAULT;
    int arrow_icon = m_collapsed ? BND_ICON_DISCLOSURE_TRI_RIGHT : BND_ICON_DISCLOSURE_TRI_DOWN;
    bndToolButton(pctx, 0, 0, w, m_header_height,
        BND_CORNER_DOWN, hdr_state, arrow_icon, m_title.c_str());

    // Draw children only if not collapsed
    if (!m_collapsed) {
        rm_widget::on_draw(pctx);
    }
}

bool bui_panel::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        rm_vec2 local = cursor_to_local(cursor_pos);
        // Click on header toggles collapse
        if (local.y >= 0 && local.y < m_header_height &&
            local.x >= 0 && local.x < m_size.x) {
            toggle_collapsed();
            return false;
        }
    }
    return true;
}

void bui_panel::set_collapsed(bool c) {
    if (c == m_collapsed) return;
    if (!c) {
        // Expanding: restore saved height
        m_collapsed = false;
        resize(m_size.x, m_expanded_height);
    } else {
        // Collapsing: save height, shrink to header
        m_expanded_height = m_size.y;
        m_collapsed = true;
        resize(m_size.x, m_header_height);
    }
    if (is_valid_callback())
        get_callback()(this, m_collapsed);
}

void bui_panel::toggle_collapsed() {
    set_collapsed(!m_collapsed);
}

rm_rect bui_panel::get_content_rect() const {
    return rm_rect(0.f, m_header_height, m_size.x, m_size.y - m_header_height);
}

// ============================================================================
// bui_number_field
// ============================================================================
bui_number_field::bui_number_field(rm_widget* p_parent, int x, int y, int w, int h,
    const char* label, float value, float minval, float maxval, float step,
    int precision, int corner_flags, bui_number_field_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_number_field", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_label(label ? label : ""), m_value(value)
    , m_min(minval), m_max(maxval), m_step(step)
    , m_precision(precision), m_corner_flags(corner_flags)
    , m_dragging(false), m_drag_start_x(0.f), m_drag_start_value(0.f)
{
    set_callback(cb);
    format_value();
}

void bui_number_field::format_value() {
    snprintf(m_value_buf, sizeof(m_value_buf), "%.*f", m_precision, (double)m_value);
}

void bui_number_field::set_value(float v) {
    m_value = rm_clamp(v, m_min, m_max);
    format_value();
}

void bui_number_field::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (m_dragging)
        state = BND_ACTIVE;
    else if (get_elem_flags().is_hovered())
        state = BND_HOVER;

    bndNumberField(pctx, 0, 0, m_size.x, m_size.y,
        m_corner_flags, state,
        m_label.empty() ? nullptr : m_label.c_str(), m_value_buf);
    rm_widget::on_draw(pctx);
}

bool bui_number_field::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            m_dragging = true;
            m_drag_start_x = cursor_pos.x;
            m_drag_start_value = m_value;
            return false;
        }
        if (state == UP) {
            if (m_dragging) {
                m_dragging = false;
                // If barely moved, treat as click on arrows
                float dx = cursor_pos.x - m_drag_start_x;
                if (fabsf(dx) < 3.f) {
                    rm_vec2 local = cursor_to_local(cursor_pos);
                    if (local.x < 14.f) {
                        // Left arrow: decrease
                        set_value(m_value - m_step);
                    } else if (local.x > m_size.x - 14.f) {
                        // Right arrow: increase
                        set_value(m_value + m_step);
                    }
                }
                if (is_valid_callback())
                    get_callback()(this, m_value);
                return false;
            }
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        float dx = cursor_pos.x - m_drag_start_x;
        float new_val = m_drag_start_value + dx * m_step;
        set_value(new_val);
        if (is_valid_callback())
            get_callback()(this, m_value);
        return false;
    }
    return true;
}

// ============================================================================
// bui_option_button
// ============================================================================
bui_option_button::bui_option_button(rm_widget* p_parent, int x, int y, int w, int h,
    const char* label, bool checked, bui_option_button_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_option_button")
    , m_label(label ? label : ""), m_checked(checked)
{
    set_callback(cb);
}

void bui_option_button::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = m_checked ? BND_ACTIVE : BND_DEFAULT;
    if (get_elem_flags().is_hovered() && !m_checked)
        state = BND_HOVER;

    bndOptionButton(pctx, 0, 0, m_size.x, m_size.y, state, m_label.c_str());
    rm_widget::on_draw(pctx);
}

bool bui_option_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        if (m_bbox.inside(cursor_pos)) {
            m_checked = !m_checked;
            if (is_valid_callback())
                get_callback()(this, m_checked);
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_choice_button
// ============================================================================
bui_choice_button::bui_choice_button(rm_widget* p_parent, int x, int y, int w, int h,
    int iconid, int corner_flags, bui_choice_button_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_choice_button",
        RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_DISABLE_SCISSOR | RM_FLAG_HIGHEST_PRIORITY)
    , m_selected(-1), m_hover_idx(-1), m_expanded(false)
    , m_icon(iconid), m_corner_flags(corner_flags)
{
    set_callback(cb);
}

void bui_choice_button::add_item(const char* label, int id, int iconid) {
    bui_choice_item item;
    item.id = (id >= 0) ? id : (int)m_items.size();
    item.iconid = iconid;
    item.label = label ? label : "";
    m_items.push_back(item);
    if (m_selected < 0)
        m_selected = 0;
}

void bui_choice_button::set_selected(int idx) {
    if (idx >= 0 && idx < (int)m_items.size())
        m_selected = idx;
}

void bui_choice_button::on_draw(NVGcontext* pctx) {
    // Draw the main button
    BNDwidgetState btn_state = BND_DEFAULT;
    if (m_expanded)
        btn_state = BND_ACTIVE;
    else if (get_elem_flags().is_hovered())
        btn_state = BND_HOVER;

    const char* label = nullptr;
    int iconid = m_icon;
    if (m_selected >= 0 && m_selected < (int)m_items.size()) {
        label = m_items[m_selected].label.c_str();
        if (m_items[m_selected].iconid >= 0)
            iconid = m_items[m_selected].iconid;
    }

    bndChoiceButton(pctx, 0, 0, m_size.x, m_size.y,
        m_corner_flags, btn_state, iconid, label);

    // Draw dropdown menu when expanded
    if (m_expanded && !m_items.empty()) {
        float menu_y = m_size.y;
        float menu_h = (float)(m_items.size()) * (float)BND_WIDGET_HEIGHT;
        float menu_w = m_size.x;
        int last_zindex = pctx->getZIndex();
        pctx->setZIndex(1);
        bndMenuBackground(pctx, 0, menu_y, menu_w, menu_h, BND_CORNER_NONE);


        for (size_t i = 0; i < m_items.size(); i++) {
            float iy = menu_y + (float)i * (float)BND_WIDGET_HEIGHT;
            BNDwidgetState item_state = BND_DEFAULT;
            if ((int)i == m_hover_idx)
                item_state = BND_HOVER;
            if ((int)i == m_selected)
                item_state = BND_ACTIVE;

            bndMenuItem(pctx, 0, iy, menu_w, (float)BND_WIDGET_HEIGHT,
                item_state, m_items[i].iconid, m_items[i].label.c_str());
        }
        pctx->setZIndex(last_zindex);
    }

    rm_widget::on_draw(pctx);
}

bool bui_choice_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    if (m_expanded) {
        // Check hover on menu items
        float menu_y = m_size.y;
        m_hover_idx = -1;
        if (local.x >= 0 && local.x < m_size.x && local.y >= menu_y) {
            int idx = (int)((local.y - menu_y) / (float)BND_WIDGET_HEIGHT);
            if (idx >= 0 && idx < (int)m_items.size())
                m_hover_idx = idx;
        }

        if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
            if (m_hover_idx >= 0) {
                m_selected = m_hover_idx;
                if (is_valid_callback())
                    get_callback()(this, m_selected);
            }
            m_expanded = false;
            return false;
        }
    } else {
        if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
            if (m_bbox.inside(cursor_pos)) {
                m_expanded = true;
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
// bui_splitter
// ============================================================================
bui_splitter::bui_splitter(rm_widget* p_parent, int x, int y, int w, int h,
    bool vertical, float initial_ratio, float thickness, bui_splitter_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_splitter", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_ratio(rm_clamp(initial_ratio, 0.05f, 0.95f))
    , m_vertical(vertical), m_dragging(false), m_thickness(thickness)
{
    set_callback(cb);
}

void bui_splitter::on_draw(NVGcontext* pctx) {
    float w = m_size.x;
    float h = m_size.y;

    // Draw full background
    bndBackground(pctx, 0, 0, w, h);

    // Draw the splitter handle area
    if (m_vertical) {
        float sx = w * m_ratio - m_thickness * 0.5f;
        // Draw splitter bar
        bndBevel(pctx, sx, 0, m_thickness, h);
        // Draw splitter widgets (corner handles)
        bndSplitterWidgets(pctx, sx, 0, m_thickness, h);
    } else {
        float sy = h * m_ratio - m_thickness * 0.5f;
        bndBevel(pctx, 0, sy, w, m_thickness);
        bndSplitterWidgets(pctx, 0, sy, w, m_thickness);
    }

    rm_widget::on_draw(pctx);
}

bool bui_splitter::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN) {
            // Check if clicking on the splitter bar
            if (m_vertical) {
                float sx = m_size.x * m_ratio;
                if (fabsf(local.x - sx) < m_thickness) {
                    m_dragging = true;
                    return false;
                }
            } else {
                float sy = m_size.y * m_ratio;
                if (fabsf(local.y - sy) < m_thickness) {
                    m_dragging = true;
                    return false;
                }
            }
        }
        if (state == UP && m_dragging) {
            m_dragging = false;
            if (is_valid_callback())
                get_callback()(this, m_ratio);
            return false;
        }
    }

    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        if (m_vertical) {
            m_ratio = rm_clamp(local.x / m_size.x, 0.05f, 0.95f);
        } else {
            m_ratio = rm_clamp(local.y / m_size.y, 0.05f, 0.95f);
        }
        if (is_valid_callback())
            get_callback()(this, m_ratio);
        return false;
    }

    return true;
}

rm_rect bui_splitter::get_first_rect() const {
    rm_rect r;
    if (m_vertical) {
        float sx = m_size.x * m_ratio - m_thickness * 0.5f;
        r.init(0.f, 0.f, sx, m_size.y);
    } else {
        float sy = m_size.y * m_ratio - m_thickness * 0.5f;
        r.init(0.f, 0.f, m_size.x, sy);
    }
    return r;
}

rm_rect bui_splitter::get_second_rect() const {
    rm_rect r;
    if (m_vertical) {
        float sx = m_size.x * m_ratio + m_thickness * 0.5f;
        r.init(sx, 0.f, m_size.x - sx, m_size.y);
    } else {
        float sy = m_size.y * m_ratio + m_thickness * 0.5f;
        r.init(0.f, sy, m_size.x, m_size.y - sy);
    }
    return r;
}

// ============================================================================
// bui_separator
// ============================================================================
bui_separator::bui_separator(rm_widget* p_parent, int x, int y, int length, bool vertical)
    : rm_widget(x, y, vertical ? 2 : length, vertical ? length : 2, p_parent, "bui_separator")
    , m_vertical(vertical)
{
}

void bui_separator::on_draw(NVGcontext* pctx) {
    const BNDtheme* theme = bndGetTheme();
    NVGcolor shade = theme->backgroundColor;
    shade = bndOffsetColor(shade, -20);
    NVGcolor highlight = bndOffsetColor(theme->backgroundColor, 20);

    pctx->beginPath();
    if (m_vertical) {
        pctx->moveTo(0.5f, 0);
        pctx->lineTo(0.5f, m_size.y);
    } else {
        pctx->moveTo(0, 0.5f);
        pctx->lineTo(m_size.x, 0.5f);
    }
    pctx->StrokeWidth(1.0f);
    pctx->strokeColor(shade);
    pctx->stroke();

    pctx->beginPath();
    if (m_vertical) {
        pctx->moveTo(1.5f, 0);
        pctx->lineTo(1.5f, m_size.y);
    } else {
        pctx->moveTo(0, 1.5f);
        pctx->lineTo(m_size.x, 1.5f);
    }
    pctx->strokeColor(highlight);
    pctx->stroke();
}

// ============================================================================
// bui_radio_button
// ============================================================================
bui_radio_button::bui_radio_button(rm_widget* p_parent, int x, int y, int w, int h,
    bool vertical, bui_radio_button_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_radio_button")
    , m_selected(0), m_hover_idx(-1), m_vertical(vertical)
{
    set_callback(cb);
}

void bui_radio_button::add_item(const char* label, int id, int iconid) {
    bui_radio_item item;
    item.id = (id >= 0) ? id : (int)m_items.size();
    item.iconid = iconid;
    item.label = label ? label : "";
    m_items.push_back(item);
}

void bui_radio_button::set_selected(int idx) {
    if (idx >= 0 && idx < (int)m_items.size())
        m_selected = idx;
}

int bui_radio_button::hit_test(const rm_vec2& local) const {
    if (m_items.empty()) return -1;
    if (m_vertical) {
        float btn_h = m_size.y / (float)m_items.size();
        int idx = (int)(local.y / btn_h);
        if (idx >= 0 && idx < (int)m_items.size() &&
            local.x >= 0 && local.x < m_size.x)
            return idx;
    } else {
        float btn_w = m_size.x / (float)m_items.size();
        int idx = (int)(local.x / btn_w);
        if (idx >= 0 && idx < (int)m_items.size() &&
            local.y >= 0 && local.y < m_size.y)
            return idx;
    }
    return -1;
}

void bui_radio_button::on_draw(NVGcontext* pctx) {
    if (m_items.empty()) return;

    for (size_t i = 0; i < m_items.size(); i++) {
        float bx, by, bw, bh;
        if (m_vertical) {
            bh = m_size.y / (float)m_items.size();
            bx = 0; by = (float)i * bh; bw = m_size.x;
        } else {
            bw = m_size.x / (float)m_items.size();
            bx = (float)i * bw; by = 0; bh = m_size.y;
        }

        BNDwidgetState state = BND_DEFAULT;
        if ((int)i == m_selected)
            state = BND_ACTIVE;
        else if ((int)i == m_hover_idx)
            state = BND_HOVER;

        // Corner flags for grouped appearance
        int corners = BND_CORNER_NONE;
        if (m_vertical) {
            if (i > 0) corners |= BND_CORNER_TOP;
            if (i + 1 < m_items.size()) corners |= BND_CORNER_DOWN;
        } else {
            if (i > 0) corners |= BND_CORNER_LEFT;
            if (i + 1 < m_items.size()) corners |= BND_CORNER_RIGHT;
        }

        bndRadioButton(pctx, bx, by, bw, bh, corners, state,
            m_items[i].iconid, m_items[i].label.c_str());
    }
    rm_widget::on_draw(pctx);
}

bool bui_radio_button::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    m_hover_idx = hit_test(local);

    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        if (m_hover_idx >= 0 && m_hover_idx != m_selected) {
            m_selected = m_hover_idx;
            if (is_valid_callback())
                get_callback()(this, m_selected);
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_text_field
// ============================================================================
bui_text_field::bui_text_field(rm_widget* p_parent, int x, int y, int w, int h,
    const char* text, int iconid, int corner_flags, bui_text_field_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_text_field", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_text(text ? text : ""), m_icon(iconid), m_corner_flags(corner_flags)
    , m_cursor((int)(text ? strlen(text) : 0)), m_sel_start(-1), m_editing(false)
{
    set_callback(cb);
}

bool bui_text_field::has_selection() const {
    return m_sel_start >= 0 && m_sel_start != m_cursor;
}

int bui_text_field::sel_min() const {
    return (m_sel_start < m_cursor) ? m_sel_start : m_cursor;
}

int bui_text_field::sel_max() const {
    return (m_sel_start > m_cursor) ? m_sel_start : m_cursor;
}

void bui_text_field::delete_selection() {
    if (!has_selection()) return;
    int lo = sel_min();
    int hi = sel_max();
    m_text.erase(lo, hi - lo);
    m_cursor = lo;
    m_sel_start = -1;
}

void bui_text_field::insert_char(int ch) {
    if (has_selection())
        delete_selection();
    if (ch >= 32) {
        // Encode UTF-8
        char buf[5] = {};
        if (ch < 0x80) {
            buf[0] = (char)ch;
        } else if (ch < 0x800) {
            buf[0] = (char)(0xC0 | (ch >> 6));
            buf[1] = (char)(0x80 | (ch & 0x3F));
        } else if (ch < 0x10000) {
            buf[0] = (char)(0xE0 | (ch >> 12));
            buf[1] = (char)(0x80 | ((ch >> 6) & 0x3F));
            buf[2] = (char)(0x80 | (ch & 0x3F));
        } else {
            buf[0] = (char)(0xF0 | (ch >> 18));
            buf[1] = (char)(0x80 | ((ch >> 12) & 0x3F));
            buf[2] = (char)(0x80 | ((ch >> 6) & 0x3F));
            buf[3] = (char)(0x80 | (ch & 0x3F));
        }
        m_text.insert(m_cursor, buf);
        m_cursor += (int)strlen(buf);
    }
}

void bui_text_field::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (m_editing)
        state = BND_ACTIVE;
    else if (get_elem_flags().is_hovered())
        state = BND_HOVER;

    int cbegin = m_editing ? m_cursor : 0;
    int cend = m_editing ? m_cursor : (int)m_text.size();
    if (has_selection()) {
        cbegin = sel_min();
        cend = sel_max();
    }

    bndTextField(pctx, 0, 0, m_size.x, m_size.y,
        m_corner_flags, state, m_icon, m_text.c_str(), cbegin, cend);
    rm_widget::on_draw(pctx);
}

bool bui_text_field::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
        if (m_bbox.inside(cursor_pos)) {
            m_editing = true;
            // Place cursor at end (simplified — no glyph hit testing)
            m_cursor = (int)m_text.size();
            m_sel_start = -1;
            return false;
        } else {
            if (m_editing) {
                m_editing = false;
                if (is_valid_callback())
                    get_callback()(this, m_text.c_str());
            }
        }
    }
    return true;
}

void bui_text_field::on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) {
    if (!m_editing || state == UP) return;

    switch (vk) {
    case RM_KEY_LEFT:
        if (m_cursor > 0) m_cursor--;
        m_sel_start = -1;
        break;
    case RM_KEY_RIGHT:
        if (m_cursor < (int)m_text.size()) m_cursor++;
        m_sel_start = -1;
        break;
    case RM_KEY_HOME:
        m_cursor = 0;
        m_sel_start = -1;
        break;
    case RM_KEY_END:
        m_cursor = (int)m_text.size();
        m_sel_start = -1;
        break;
    case RM_KEY_BACKSPACE:
        if (has_selection()) {
            delete_selection();
        } else if (m_cursor > 0) {
            m_text.erase(m_cursor - 1, 1);
            m_cursor--;
        }
        if (is_valid_callback())
            get_callback()(this, m_text.c_str());
        break;
    case RM_KEY_DELETE:
        if (has_selection()) {
            delete_selection();
        } else if (m_cursor < (int)m_text.size()) {
            m_text.erase(m_cursor, 1);
        }
        if (is_valid_callback())
            get_callback()(this, m_text.c_str());
        break;
    case RM_KEY_ENTER:
        m_editing = false;
        if (is_valid_callback())
            get_callback()(this, m_text.c_str());
        break;
    case RM_KEY_ESCAPE:
        m_editing = false;
        m_sel_start = -1;
        break;
    default:
        break;
    }
}

void bui_text_field::on_text_input(int sym) {
    if (!m_editing) return;
    insert_char(sym);
    if (is_valid_callback())
        get_callback()(this, m_text.c_str());
}

// ============================================================================
// bui_slider
// ============================================================================
bui_slider::bui_slider(rm_widget* p_parent, int x, int y, int w, int h,
    const char* label, float value, float minval, float maxval,
    int precision, int corner_flags, bui_slider_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_slider", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_label(label ? label : ""), m_value(value)
    , m_min(minval), m_max(maxval)
    , m_precision(precision), m_corner_flags(corner_flags)
    , m_dragging(false)
{
    set_callback(cb);
    format_value();
}

void bui_slider::format_value() {
    snprintf(m_value_buf, sizeof(m_value_buf), "%.*f", m_precision, (double)m_value);
}

void bui_slider::set_value(float v) {
    m_value = rm_clamp(v, m_min, m_max);
    format_value();
}

void bui_slider::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (m_dragging)
        state = BND_ACTIVE;
    else if (get_elem_flags().is_hovered())
        state = BND_HOVER;

    float range = m_max - m_min;
    float progress = (range > FLT_EPSILON) ? (m_value - m_min) / range : 0.f;
    progress = rm_clamp(progress, 0.f, 1.f);

    bndSlider(pctx, 0, 0, m_size.x, m_size.y,
        m_corner_flags, state, progress,
        m_label.empty() ? nullptr : m_label.c_str(), m_value_buf);
    rm_widget::on_draw(pctx);
}

bool bui_slider::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            m_dragging = true;
            float frac = rm_clamp(local.x / m_size.x, 0.f, 1.f);
            set_value(m_min + frac * (m_max - m_min));
            if (is_valid_callback())
                get_callback()(this, m_value);
            return false;
        }
        if (state == UP && m_dragging) {
            m_dragging = false;
            return false;
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        float frac = rm_clamp(local.x / m_size.x, 0.f, 1.f);
        set_value(m_min + frac * (m_max - m_min));
        if (is_valid_callback())
            get_callback()(this, m_value);
        return false;
    }
    return true;
}

// ============================================================================
// bui_scrollbar
// ============================================================================
bui_scrollbar::bui_scrollbar(rm_widget* p_parent, int x, int y, int w, int h,
    bool vertical, float handle_size, bui_scrollbar_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_scrollbar", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_offset(0.f), m_handle_size(rm_clamp(handle_size, 0.05f, 1.f))
    , m_vertical(vertical), m_dragging(false)
    , m_drag_start(0.f), m_drag_start_offset(0.f)
{
    set_callback(cb);
}

void bui_scrollbar::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (m_dragging)
        state = BND_ACTIVE;
    else if (get_elem_flags().is_hovered())
        state = BND_HOVER;

    if (m_vertical) {
        // bndScrollBar draws horizontal, so we rotate
        pctx->save();
        pctx->translate(m_size.x, 0);
        pctx->rotate(NVG_PI * 0.5f);
        bndScrollBar(pctx, 0, 0, m_size.y, m_size.x, state, m_offset, m_handle_size);
        pctx->restore();
    } else {
        bndScrollBar(pctx, 0, 0, m_size.x, m_size.y, state, m_offset, m_handle_size);
    }
    rm_widget::on_draw(pctx);
}

bool bui_scrollbar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            m_dragging = true;
            m_drag_start = m_vertical ? local.y : local.x;
            m_drag_start_offset = m_offset;
            return false;
        }
        if (state == UP && m_dragging) {
            m_dragging = false;
            return false;
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        float track_len = m_vertical ? m_size.y : m_size.x;
        float current = m_vertical ? local.y : local.x;
        float dx = current - m_drag_start;
        float usable = track_len * (1.f - m_handle_size);
        if (usable > 1.f)
            m_offset = rm_clamp(m_drag_start_offset + dx / usable, 0.f, 1.f);
        if (is_valid_callback())
            get_callback()(this, m_offset);
        return false;
    }
    return true;
}

// ============================================================================
// bui_node_port
// ============================================================================
bui_node_port::bui_node_port(rm_widget* p_parent, int x, int y,
    const char* label, NVGcolor color, bool is_input)
    : rm_widget(x, y, 10, 10, p_parent, "bui_node_port")
    , m_color(color), m_connected(false), m_input(is_input)
    , m_label(label ? label : "")
{
}

void bui_node_port::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (get_elem_flags().is_hovered())
        state = BND_HOVER;
    if (m_connected)
        state = BND_ACTIVE;
    bndNodePort(pctx, 5, 5, state, m_color);
}

rm_vec2 bui_node_port::get_center() const {
    return rm_vec2(m_pos_of_parent.x + 5.f, m_pos_of_parent.y + 5.f);
}

// ============================================================================
// bui_node
// ============================================================================
bui_node::bui_node(rm_widget* p_parent, int x, int y, int w,
    const char* title, int iconid, NVGcolor title_color, bui_node_cb cb)
    : rm_widget(x, y, w, (int)BND_WIDGET_HEIGHT, p_parent, "bui_node",
        RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_title(title ? title : ""), m_icon(iconid)
    , m_title_color(title_color), m_dragging(false), m_drag_offset(0, 0)
{
    set_callback(cb);
    /* allow node to grow vertically when ports are added */
    set_max_size(rm_vec2(0.f, 0.f));
}

bui_node_port* bui_node::add_input(const char* label, NVGcolor color) {
    float y = (float)BND_WIDGET_HEIGHT + (float)m_inputs.size() * (float)BND_WIDGET_HEIGHT;
    auto* port = new bui_node_port(this, 0, (int)y, label, color, true);
    m_inputs.push_back(port);
    m_input_labels.push_back(label ? label : "");
    // Resize node height
    float total = (float)BND_WIDGET_HEIGHT +
        rm_max(m_inputs.size(), m_outputs.size()) * (float)BND_WIDGET_HEIGHT;
    resize(m_size.x, total);
    return port;
}

bui_node_port* bui_node::add_output(const char* label, NVGcolor color) {
    float y = (float)BND_WIDGET_HEIGHT + (float)m_outputs.size() * (float)BND_WIDGET_HEIGHT;
    auto* port = new bui_node_port(this, (int)m_size.x - 10, (int)y, label, color, false);
    m_outputs.push_back(port);
    m_output_labels.push_back(label ? label : "");
    float total = (float)BND_WIDGET_HEIGHT +
        rm_max(m_inputs.size(), m_outputs.size()) * (float)BND_WIDGET_HEIGHT;
    resize(m_size.x, total);
    return port;
}

void bui_node::on_draw(NVGcontext* pctx) {
    BNDwidgetState state = BND_DEFAULT;
    if (get_elem_flags().is_hovered() || m_dragging)
        state = BND_HOVER;
    if (get_elem_flags().is_focused())
        state = BND_ACTIVE;

    bndNodeBackground(pctx, 0, 0, m_size.x, m_size.y,
        state, m_icon, m_title.c_str(), m_title_color);

    // Draw port labels
    pctx->setFontSize(11.0f);
    pctx->fillColor(NVGcolor::RGB(200, 200, 200));
    pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    for (size_t i = 0; i < m_inputs.size(); i++) {
        float iy = (float)BND_WIDGET_HEIGHT + (float)i * (float)BND_WIDGET_HEIGHT + (float)BND_WIDGET_HEIGHT * 0.5f;
        if (i < m_input_labels.size() && !m_input_labels[i].empty())
            pctx->text(14.f, iy, m_input_labels[i].c_str(), nullptr);
    }
    pctx->setTextAlign(NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    for (size_t i = 0; i < m_outputs.size(); i++) {
        float iy = (float)BND_WIDGET_HEIGHT + (float)i * (float)BND_WIDGET_HEIGHT + (float)BND_WIDGET_HEIGHT * 0.5f;
        if (i < m_output_labels.size() && !m_output_labels[i].empty())
            pctx->text(m_size.x - 14.f, iy, m_output_labels[i].c_str(), nullptr);
    }

    rm_widget::on_draw(pctx);
}

bool bui_node::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            // Only drag if clicking header area
            if (local.y < (float)BND_WIDGET_HEIGHT) {
                m_dragging = true;
                m_drag_offset = local;
                return false;
            }
        }
        if (state == UP && m_dragging) {
            m_dragging = false;
            if (is_valid_callback())
                get_callback()(this);
            return false;
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        // delta is provided by the framework
        rm_vec2 new_pos(
            m_pos_of_parent.x + delta.x,
            m_pos_of_parent.y + delta.y
        );
        move(new_pos);
        return false;
    }
    return true;
}

// ============================================================================
// bui_node_wires
// ============================================================================
bui_node_wires::bui_node_wires(rm_widget* p_parent)
    : rm_widget(0, 0, 0, 0, p_parent, "bui_node_wires",
        RM_FLAG_VISIBLE | RM_FLAG_DISABLE_SCISSOR)
{
    /* zero-size so bbox never matches cursor → no mouse interaction.
       DISABLE_SCISSOR lets us draw wires across the full parent area. */
    set_min_size(rm_vec2(0.f, 0.f));
    set_max_size(rm_vec2(0.f, 0.f));
}

void bui_node_wires::add_connection(bui_node_port* from, bui_node_port* to) {
    if (!from || !to) return;
    from->set_connected(true);
    to->set_connected(true);
    m_connections.push_back({ from, to });
}

void bui_node_wires::on_draw(NVGcontext* pctx) {
    for (auto& conn : m_connections) {
        if (!conn.from || !conn.to) continue;
        rm_widget* from_node = conn.from->get_parent();
        rm_widget* to_node = conn.to->get_parent();
        if (!from_node || !to_node) continue;

        rm_vec2 fc = conn.from->get_center();
        rm_vec2 tc = conn.to->get_center();
        float x0 = from_node->get_pos_of_parent().x + fc.x;
        float y0 = from_node->get_pos_of_parent().y + fc.y;
        float x1 = to_node->get_pos_of_parent().x + tc.x;
        float y1 = to_node->get_pos_of_parent().y + tc.y;

        bndColoredNodeWire(pctx, x0, y0, x1, y1,
            conn.from->get_color(), conn.to->get_color());
    }
}

// ============================================================================
// bui_node_wire_renderer
// ============================================================================
void bui_node_wire_renderer::draw_wire(NVGcontext* pctx,
    rm_vec2 from, rm_vec2 to,
    BNDwidgetState state_from, BNDwidgetState state_to) {
    bndNodeWire(pctx, from.x, from.y, to.x, to.y, state_from, state_to);
}

void bui_node_wire_renderer::draw_colored_wire(NVGcontext* pctx,
    rm_vec2 from, NVGcolor color_from,
    rm_vec2 to, NVGcolor color_to) {
    bndColoredNodeWire(pctx, from.x, from.y, to.x, to.y, color_from, color_to);
}

// ============================================================================
// bui_menu_label
// ============================================================================
bui_menu_label::bui_menu_label(rm_widget* p_parent, int x, int y, int w,
    const char* text, int iconid)
    : rm_widget(x, y, w, (int)BND_WIDGET_HEIGHT, p_parent, "bui_menu_label")
    , m_text(text ? text : ""), m_icon(iconid)
{
}

void bui_menu_label::on_draw(NVGcontext* pctx) {
    bndMenuLabel(pctx, 0, 0, m_size.x, m_size.y, m_icon, m_text.c_str());
}

// ============================================================================
// bui_color_picker
// ============================================================================
bui_color_picker::bui_color_picker(rm_widget* p_parent, int x, int y, int size,
    NVGcolor initial, bui_color_picker_cb cb)
    : rm_widget(x, y, size, size, p_parent, "bui_color_picker", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_hue(0.f), m_sat(1.f), m_val(1.f), m_alpha(1.f)
    , m_dragging_wheel(false), m_dragging_triangle(false)
{
    set_callback(cb);
    float r2 = size * 0.5f;
    m_wheel_outer_r = r2 - 2.f;
    m_wheel_inner_r = m_wheel_outer_r - r2 * 0.18f;
    set_color(initial);
}

void bui_color_picker::hsv_to_rgb(float h, float s, float v, float& r, float& g, float& b) const {
    int i = (int)(h * 6.f);
    float f = h * 6.f - (float)i;
    float p = v * (1.f - s);
    float q = v * (1.f - f * s);
    float t = v * (1.f - (1.f - f) * s);
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    default: r = g = b = 0; break;
    }
}

NVGcolor bui_color_picker::get_color() const {
    float r, g, b;
    const_cast<bui_color_picker*>(this)->hsv_to_rgb(m_hue, m_sat, m_val, r, g, b);
    return NVGcolor::RGBAf(r, g, b, m_alpha);
}

void bui_color_picker::set_color(NVGcolor c) {
    float cmax = rm_max(c.r, rm_max(c.g, c.b));
    float cmin = rm_min(c.r, rm_min(c.g, c.b));
    float delta = cmax - cmin;
    m_val = cmax;
    m_sat = (cmax > FLT_EPSILON) ? delta / cmax : 0.f;
    if (delta < FLT_EPSILON) {
        m_hue = 0.f;
    } else if (cmax == c.r) {
        m_hue = fmodf((c.g - c.b) / delta, 6.f) / 6.f;
    } else if (cmax == c.g) {
        m_hue = ((c.b - c.r) / delta + 2.f) / 6.f;
    } else {
        m_hue = ((c.r - c.g) / delta + 4.f) / 6.f;
    }
    if (m_hue < 0.f) m_hue += 1.f;
    m_alpha = c.a;
}

void bui_color_picker::on_draw(NVGcontext* pctx) {
    float cx = m_size.x * 0.5f;
    float cy = m_size.y * 0.5f;

    // Draw hue wheel
    for (int i = 0; i < 6; i++) {
        float a0 = (float)i / 6.f * NVG_PI * 2.f - NVG_PI * 0.5f;
        float a1 = (float)(i + 1) / 6.f * NVG_PI * 2.f - NVG_PI * 0.5f;
        float r0, g0, b0, r1, g1, b1;
        hsv_to_rgb((float)i / 6.f, 1.f, 1.f, r0, g0, b0);
        hsv_to_rgb((float)(i + 1) / 6.f, 1.f, 1.f, r1, g1, b1);

        NVGpaint paint = NVGpaint::linearGradient(
            cx + cosf(a0) * m_wheel_inner_r, cy + sinf(a0) * m_wheel_inner_r,
            cx + cosf(a1) * m_wheel_inner_r, cy + sinf(a1) * m_wheel_inner_r,
            NVGcolor::RGBAf(r0, g0, b0, 1.f), NVGcolor::RGBAf(r1, g1, b1, 1.f));

        pctx->beginPath();
        pctx->arc(cx, cy, m_wheel_inner_r, a0, a1, NVG_CW);
        pctx->arc(cx, cy, m_wheel_outer_r, a1, a0, NVG_CCW);
        pctx->closePath();
        pctx->fillPaint(paint);
        pctx->fill();
    }

    // Draw hue indicator on wheel
    float hue_angle = m_hue * NVG_PI * 2.f - NVG_PI * 0.5f;
    float mid_r = (m_wheel_inner_r + m_wheel_outer_r) * 0.5f;
    float hx = cx + cosf(hue_angle) * mid_r;
    float hy = cy + sinf(hue_angle) * mid_r;
    pctx->beginPath();
    pctx->circle(hx, hy, 4.f);
    pctx->StrokeWidth(2.f);
    pctx->strokeColor(NVGcolor::RGB(255, 255, 255));
    pctx->stroke();

    // Draw SV triangle inside wheel
    float tri_r = m_wheel_inner_r - 6.f;
    float a = hue_angle;
    float ax = cx + cosf(a) * tri_r;
    float ay = cy + sinf(a) * tri_r;
    float bx = cx + cosf(a + NVG_PI * 2.f / 3.f) * tri_r;
    float by = cy + sinf(a + NVG_PI * 2.f / 3.f) * tri_r;
    float ccx = cx + cosf(a + NVG_PI * 4.f / 3.f) * tri_r;
    float ccy = cy + sinf(a + NVG_PI * 4.f / 3.f) * tri_r;

    // White-to-hue gradient
    float hr, hg, hb;
    hsv_to_rgb(m_hue, 1.f, 1.f, hr, hg, hb);
    NVGpaint hue_paint = NVGpaint::linearGradient(bx, by, ax, ay,
        NVGcolor::RGB(255, 255, 255), NVGcolor::RGBAf(hr, hg, hb, 1.f));
    pctx->beginPath();
    pctx->moveTo(ax, ay);
    pctx->lineTo(bx, by);
    pctx->lineTo(ccx, ccy);
    pctx->closePath();
    pctx->fillPaint(hue_paint);
    pctx->fill();

    // Black overlay gradient
    NVGpaint black_paint = NVGpaint::linearGradient(
        (ax + bx) * 0.5f, (ay + by) * 0.5f, ccx, ccy,
        NVGcolor::RGBA(0, 0, 0, 0), NVGcolor::RGBA(0, 0, 0, 255));
    pctx->beginPath();
    pctx->moveTo(ax, ay);
    pctx->lineTo(bx, by);
    pctx->lineTo(ccx, ccy);
    pctx->closePath();
    pctx->fillPaint(black_paint);
    pctx->fill();

    // SV cursor
    float sx = ax * m_sat + bx * (1.f - m_sat);
    float sy = ay * m_sat + by * (1.f - m_sat);
    float svx = sx * m_val + ccx * (1.f - m_val);
    float svy = sy * m_val + ccy * (1.f - m_val);
    pctx->beginPath();
    pctx->circle(svx, svy, 5.f);
    pctx->StrokeWidth(2.f);
    pctx->strokeColor(NVGcolor::RGB(255, 255, 255));
    pctx->stroke();
    pctx->fillColor(get_color());
    pctx->fill();
}

bool bui_color_picker::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    float cx = m_size.x * 0.5f;
    float cy = m_size.y * 0.5f;
    float dx = local.x - cx;
    float dy = local.y - cy;
    float dist = sqrtf(dx * dx + dy * dy);

    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            if (dist >= m_wheel_inner_r && dist <= m_wheel_outer_r) {
                m_dragging_wheel = true;
                float angle = atan2f(dy, dx) + NVG_PI * 0.5f;
                if (angle < 0.f) angle += NVG_PI * 2.f;
                m_hue = angle / (NVG_PI * 2.f);
                if (m_hue > 1.f) m_hue -= 1.f;
                if (is_valid_callback()) get_callback()(this, get_color());
                return false;
            }
            if (dist < m_wheel_inner_r) {
                m_dragging_triangle = true;
                return false;
            }
        }
        if (state == UP && (m_dragging_wheel || m_dragging_triangle)) {
            m_dragging_wheel = false;
            m_dragging_triangle = false;
            if (is_valid_callback()) get_callback()(this, get_color());
            return false;
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE) {
        if (m_dragging_wheel) {
            float angle = atan2f(dy, dx) + NVG_PI * 0.5f;
            if (angle < 0.f) angle += NVG_PI * 2.f;
            m_hue = angle / (NVG_PI * 2.f);
            if (m_hue > 1.f) m_hue -= 1.f;
            if (is_valid_callback()) get_callback()(this, get_color());
            return false;
        }
        if (m_dragging_triangle) {
            // Simplified: map position to S/V
            float tri_r = m_wheel_inner_r - 6.f;
            float hue_angle = m_hue * NVG_PI * 2.f - NVG_PI * 0.5f;
            float ax = cx + cosf(hue_angle) * tri_r;
            float ay = cy + sinf(hue_angle) * tri_r;
            float bxx = cx + cosf(hue_angle + NVG_PI * 2.f / 3.f) * tri_r;
            float byy = cy + sinf(hue_angle + NVG_PI * 2.f / 3.f) * tri_r;
            float ccx = cx + cosf(hue_angle + NVG_PI * 4.f / 3.f) * tri_r;
            float ccy = cy + sinf(hue_angle + NVG_PI * 4.f / 3.f) * tri_r;

            // Barycentric coordinates
            float d00 = (bxx - ccx) * (bxx - ccx) + (byy - ccy) * (byy - ccy);
            float d01 = (bxx - ccx) * (ax - ccx) + (byy - ccy) * (ay - ccy);
            float d11 = (ax - ccx) * (ax - ccx) + (ay - ccy) * (ay - ccy);
            float d20 = (bxx - ccx) * (local.x - ccx) + (byy - ccy) * (local.y - ccy);
            float d21 = (ax - ccx) * (local.x - ccx) + (ay - ccy) * (local.y - ccy);
            float denom = d00 * d11 - d01 * d01;
            if (fabsf(denom) > FLT_EPSILON) {
                float u = (d11 * d20 - d01 * d21) / denom;
                float v = (d00 * d21 - d01 * d20) / denom;
                m_sat = rm_clamp(u + v, 0.f, 1.f);
                m_val = rm_clamp(v / rm_max(m_sat, FLT_EPSILON), 0.f, 1.f);
            }
            if (is_valid_callback()) get_callback()(this, get_color());
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_knob
// ============================================================================
bui_knob::bui_knob(rm_widget* p_parent, int x, int y, int diameter,
    const char* label, float value, float minval, float maxval,
    bui_knob_cb cb)
    : rm_widget(x, y, diameter, diameter + 16, p_parent, "bui_knob", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL)
    , m_label(label ? label : ""), m_value(value)
    , m_min(minval), m_max(maxval)
    , m_dragging(false), m_drag_start_y(0.f), m_drag_start_value(0.f)
{
    set_callback(cb);
}

void bui_knob::set_value(float v) {
    m_value = rm_clamp(v, m_min, m_max);
}

void bui_knob::on_draw(NVGcontext* pctx) {
    float diameter = rm_min(m_size.x, m_size.y - 16.f);
    float cx = m_size.x * 0.5f;
    float cy = diameter * 0.5f;
    float r = diameter * 0.5f - 2.f;

    float range = m_max - m_min;
    float frac = (range > FLT_EPSILON) ? (m_value - m_min) / range : 0.f;
    frac = rm_clamp(frac, 0.f, 1.f);

    // Arc range: 135° to 405° (270° sweep)
    float start_angle = NVG_PI * 0.75f;
    float end_angle = NVG_PI * 2.25f;
    float value_angle = start_angle + frac * (end_angle - start_angle);

    // Track background arc
    pctx->beginPath();
    pctx->arc(cx, cy, r, start_angle, end_angle, NVG_CW);
    pctx->StrokeWidth(3.f);
    pctx->strokeColor(NVGcolor::RGBA(60, 60, 60, 200));
    pctx->stroke();

    // Value arc
    if (frac > 0.001f) {
        const BNDtheme* theme = bndGetTheme();
        NVGcolor fill_color = theme->sliderTheme.itemColor;
        pctx->beginPath();
        pctx->arc(cx, cy, r, start_angle, value_angle, NVG_CW);
        pctx->StrokeWidth(3.f);
        pctx->strokeColor(fill_color);
        pctx->stroke();
    }

    // Knob body
    NVGpaint knob_bg = NVGpaint::radialGradient(cx, cy - r * 0.1f, r * 0.1f, r * 0.8f,
        NVGcolor::RGBA(120, 120, 120, 255), NVGcolor::RGBA(60, 60, 60, 255));
    pctx->beginPath();
    pctx->circle(cx, cy, r * 0.7f);
    pctx->fillPaint(knob_bg);
    pctx->fill();

    // Pointer line
    float px = cx + cosf(value_angle) * r * 0.5f;
    float py = cy + sinf(value_angle) * r * 0.5f;
    pctx->beginPath();
    pctx->moveTo(cx, cy);
    pctx->lineTo(px, py);
    pctx->StrokeWidth(2.f);
    pctx->strokeColor(NVGcolor::RGB(255, 255, 255));
    pctx->stroke();

    // Border
    pctx->beginPath();
    pctx->circle(cx, cy, r * 0.7f);
    pctx->StrokeWidth(1.f);
    pctx->strokeColor(NVGcolor::RGBA(0, 0, 0, 100));
    pctx->stroke();

    // Label below
    if (!m_label.empty()) {
        pctx->setFontSize(11.f);
        pctx->setTextAlign(NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        pctx->fillColor(NVGcolor::RGB(200, 200, 200));
        pctx->text(cx, diameter + 2.f, m_label.c_str(), nullptr);
    }
}

bool bui_knob::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    if (event == RM_MOUSE_EVENT_CLICK) {
        if (state == DOWN && m_bbox.inside(cursor_pos)) {
            m_dragging = true;
            m_drag_start_y = cursor_pos.y;
            m_drag_start_value = m_value;
            return false;
        }
        if (state == UP && m_dragging) {
            m_dragging = false;
            return false;
        }
    }
    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        float dy = m_drag_start_y - cursor_pos.y; // up = increase
        float sensitivity = (m_max - m_min) / 150.f;
        set_value(m_drag_start_value + dy * sensitivity);
        if (is_valid_callback())
            get_callback()(this, m_value);
        return false;
    }
    return true;
}

// ============================================================================
// bui_radio_toolbar
// ============================================================================
bui_radio_toolbar::bui_radio_toolbar(rm_widget* p_parent, int x, int y, int w, int h,
    bool vertical, bui_radio_toolbar_cb cb)
    : rm_widget(x, y, w, h, p_parent, "bui_radio_toolbar")
    , m_hover_id(-1), m_active_id(-1), m_vertical(vertical)
{
    set_callback(cb);
}

void bui_radio_toolbar::add_button(int id, int iconid, const char* label) {
    bui_radio_tool_item item;
    item.id = id;
    item.iconid = iconid;
    item.label = label ? label : "";
    item.separator = false;
    m_items.push_back(item);
}

void bui_radio_toolbar::add_separator() {
    bui_radio_tool_item item;
    item.id = -1;
    item.iconid = -1;
    item.separator = true;
    m_items.push_back(item);
}

int bui_radio_toolbar::hit_test_item(const rm_vec2& local) const {
    float pos = 0.f;
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].separator) {
            pos += 5.f;
            continue;
        }
        float btn_w = m_items[i].label.empty()
            ? (float)BND_TOOL_WIDTH
            : (float)BND_TOOL_WIDTH + 60.f;
        float btn_h = (float)BND_WIDGET_HEIGHT;

        if (m_vertical) {
            if (local.y >= pos && local.y < pos + btn_h &&
                local.x >= 0 && local.x < m_size.x)
                return m_items[i].id;
            pos += btn_h - 1.f;
        } else {
            if (local.x >= pos && local.x < pos + btn_w &&
                local.y >= 0 && local.y < m_size.y)
                return m_items[i].id;
            pos += btn_w - 1.f;
        }
    }
    return -1;
}

void bui_radio_toolbar::on_draw(NVGcontext* pctx) {
    float pos = 0.f;
    for (size_t i = 0; i < m_items.size(); i++) {
        const auto& item = m_items[i];
        if (item.separator) {
            pos += 5.f;
            continue;
        }

        float btn_w = item.label.empty()
            ? (float)BND_TOOL_WIDTH
            : (float)BND_TOOL_WIDTH + 60.f;
        float btn_h = (float)BND_WIDGET_HEIGHT;

        BNDwidgetState state = BND_DEFAULT;
        if (item.id == m_active_id)
            state = BND_ACTIVE;
        else if (item.id == m_hover_id)
            state = BND_HOVER;

        // Corner flags for grouped appearance
        int corners = BND_CORNER_NONE;
        bool has_prev = (i > 0 && !m_items[i - 1].separator);
        bool has_next = (i + 1 < m_items.size() && !m_items[i + 1].separator);

        if (m_vertical) {
            if (has_prev) corners |= BND_CORNER_TOP;
            if (has_next) corners |= BND_CORNER_DOWN;
            bndRadioButton(pctx, 0, pos, m_size.x, btn_h, corners, state,
                item.iconid, item.label.empty() ? nullptr : item.label.c_str());
            pos += btn_h - 1.f;
        } else {
            if (has_prev) corners |= BND_CORNER_LEFT;
            if (has_next) corners |= BND_CORNER_RIGHT;
            bndRadioButton(pctx, pos, 0, btn_w, btn_h, corners, state,
                item.iconid, item.label.empty() ? nullptr : item.label.c_str());
            pos += btn_w - 1.f;
        }
    }
    rm_widget::on_draw(pctx);
}

bool bui_radio_toolbar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    m_hover_id = m_bbox.inside(cursor_pos) ? hit_test_item(local) : -1;

    if (event == RM_MOUSE_EVENT_CLICK && state == UP) {
        if (m_hover_id >= 0) {
            m_active_id = m_hover_id;
            if (is_valid_callback())
                get_callback()(this, m_active_id);
            return false;
        }
    }
    return true;
}

// ============================================================================
// bui_menubar
// ============================================================================
bui_menubar::bui_menubar(rm_widget* p_parent, int x, int y, int w,
    bui_menubar_cb cb)
    : rm_widget(x, y, w, (int)BND_WIDGET_HEIGHT, p_parent, "bui_menubar",
        RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_DISABLE_SCISSOR)
    , m_open_submenu(-1), m_hover_header(-1), m_hover_item(-1)
    , m_item_padding(8.f), m_widths_dirty(true)
    , m_corner_tl(0.f), m_corner_tr(0.f)
{
    set_callback(cb);
    set_zindex(BUI_ZINDEX_DROPDOWN);

    // Auto-detect bui_window parent and sync corner radius
    if (p_parent && p_parent->classname_is("bui_window")) {
        bui_window* pwnd = static_cast<bui_window*>(p_parent);
        float cr = pwnd->get_corner_radius();
        m_corner_tl = cr;
        m_corner_tr = cr;
    }

    // Push parent's content area down so all siblings appear below the menu
    if (p_parent) {
        float menu_h = m_size.y;
        rm_rect& ca = p_parent->get_content_area();
        m_pos_of_parent.y = -menu_h;
        m_bbox.init(m_pos_of_parent, m_size);
        ca.y += menu_h;
    }
}

int bui_menubar::add_submenu(const char* label, int id, int iconid) {
    bui_submenu_def sub;
    sub.id = (id >= 0) ? id : (int)m_submenus.size();
    sub.label = label ? label : "";
    sub.iconid = iconid;
    sub.cached_header_w = 0.f;
    m_submenus.push_back(std::move(sub));
    m_widths_dirty = true;
    return (int)m_submenus.size() - 1;
}

void bui_menubar::add_item(int submenu_idx, const char* label, int id, int iconid) {
    if (submenu_idx < 0 || submenu_idx >= (int)m_submenus.size()) return;
    bui_menu_item_def item;
    item.id = id;
    item.iconid = iconid;
    item.label = label ? label : "";
    item.separator = false;
    m_submenus[submenu_idx].items.push_back(std::move(item));
    m_widths_dirty = true;
}

void bui_menubar::add_separator(int submenu_idx) {
    if (submenu_idx < 0 || submenu_idx >= (int)m_submenus.size()) return;
    bui_menu_item_def item;
    item.id = -1;
    item.iconid = -1;
    item.separator = true;
    m_submenus[submenu_idx].items.push_back(std::move(item));
}

float bui_menubar::get_header_x(int idx) const {
    float x = 0.f;
    for (int i = 0; i < idx && i < (int)m_submenus.size(); i++) {
        x += m_submenus[i].cached_header_w;
    }
    return x;
}

float bui_menubar::get_header_width(int idx) const {
    if (idx < 0 || idx >= (int)m_submenus.size()) return 0.f;
    return m_submenus[idx].cached_header_w;
}

float bui_menubar::get_dropdown_width(int idx) const {
    if (idx < 0 || idx >= (int)m_submenus.size()) return 0.f;
    const auto& sub = m_submenus[idx];
    // Use header width as minimum, then check cached_header_w of items
    float max_w = rm_max(100.f, sub.cached_header_w);
    for (size_t i = 0; i < sub.items.size(); i++) {
        if (sub.items[i].separator) continue;
        // Approximate: icon + label + padding
        float w = 30.f;
        if (sub.items[i].iconid >= 0) w += 20.f;
        w += (float)sub.items[i].label.size() * 7.f; // rough estimate when no ctx
        if (w > max_w) max_w = w;
    }
    return max_w;
}

void bui_menubar::recompute_widths(NVGcontext* pctx) {
    for (auto& sub : m_submenus) {
        sub.cached_header_w = bndLabelWidth(pctx, sub.iconid,
            sub.label.c_str()) + m_item_padding * 2.f;
    }
    m_widths_dirty = false;
}

float bui_menubar::get_dropdown_x(int idx) const {
    return get_header_x(idx);
}

float bui_menubar::get_dropdown_y() const {
    return m_size.y;
}

float bui_menubar::get_dropdown_height(int idx) const {
    if (idx < 0 || idx >= (int)m_submenus.size()) return 0.f;
    const auto& items = m_submenus[idx].items;
    float h = 0.f;
    for (size_t i = 0; i < items.size(); i++) {
        h += items[i].separator ? 8.f : (float)BND_WIDGET_HEIGHT;
    }
    return h;
}

int bui_menubar::header_hit_test(const rm_vec2& local) const {
    if (local.y < 0 || local.y > m_size.y) return -1;
    float x = 0.f;
    for (int i = 0; i < (int)m_submenus.size(); i++) {
        float w = get_header_width(i);
        if (local.x >= x && local.x < x + w)
            return i;
        x += w;
    }
    return -1;
}

int bui_menubar::item_hit_test(const rm_vec2& local) const {
    if (m_open_submenu < 0 || m_open_submenu >= (int)m_submenus.size())
        return -1;
    const auto& items = m_submenus[m_open_submenu].items;
    float dx = get_dropdown_x(m_open_submenu);
    float dy = get_dropdown_y();
    float dw = get_dropdown_width(m_open_submenu);
    float iy = dy;
    for (int i = 0; i < (int)items.size(); i++) {
        float ih = items[i].separator ? 8.f : (float)BND_WIDGET_HEIGHT;
        if (!items[i].separator &&
            local.x >= dx && local.x < dx + dw &&
            local.y >= iy && local.y < iy + ih) {
            return i;
        }
        iy += ih;
    }
    return -1;
}

void bui_menubar::on_draw(NVGcontext* pctx) {
    if (m_widths_dirty)
        recompute_widths(pctx);

    // Draw menu bar background (flat, no drop shadow)
    const BNDtheme* theme = bndGetTheme();
    {
        NVGcolor shade_top, shade_down;
        bndInnerColors(&shade_top, &shade_down, &theme->menuTheme, BND_DEFAULT, 0);
        bndInnerBox(pctx, 0, 0, m_size.x, m_size.y,
            m_corner_tl, m_corner_tr, 0, 0, shade_top, shade_down);
    }

    // Draw header items
    float hx = 0.f;
    for (int i = 0; i < (int)m_submenus.size(); i++) {
        float hw = m_submenus[i].cached_header_w;
        bool is_active = (i == m_open_submenu);
        bool is_hover = (i == m_hover_header && !is_active);
        float rr = 4.f; // corner radius

        if (is_active) {
            pctx->beginPath();
            bndRoundedBox(pctx, hx + 1, 1, hw - 2, m_size.y - 2, rr, rr, rr, rr);
            pctx->fillColor(NVGcolor::RGBA(0, 0, 0, 50));
            pctx->fill();
        } else if (is_hover) {
            pctx->beginPath();
            bndRoundedBox(pctx, hx + 1, 1, hw - 2, m_size.y - 2, rr, rr, rr, rr);
            pctx->fillColor(NVGcolor::RGBA(255, 255, 255, 24));
            pctx->fill();
        }

        // Draw icon + centered label
        // bndLabelWidth sets up the blendish font on the context as a side effect
        bndLabelWidth(pctx, -1, m_submenus[i].label.c_str());
        float tw = pctx->textBounds(0, 0, m_submenus[i].label.c_str(), nullptr, nullptr);
        bool has_icon = (m_submenus[i].iconid >= 0);
        float icon_w = has_icon ? 16.f : 0.f;
        float icon_gap = has_icon ? 4.f : 0.f;
        float total_w = icon_w + icon_gap + tw;
        float cx = hx + (hw - total_w) * 0.5f;
        float ty = m_size.y * 0.5f + 4.f; // baseline offset

        if (has_icon) {
            bndIcon(pctx, cx, (m_size.y - 16.f) * 0.5f, m_submenus[i].iconid);
            cx += icon_w + icon_gap;
        }

        pctx->beginPath();
        pctx->fillColor(theme->menuItemTheme.textColor);
        pctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
        pctx->text(cx, ty, m_submenus[i].label.c_str(), nullptr);
        hx += hw;
    }

    // Draw open dropdown
    if (m_open_submenu >= 0 && m_open_submenu < (int)m_submenus.size()) {
        const auto& items = m_submenus[m_open_submenu].items;
        float dx = get_dropdown_x(m_open_submenu);
        float dy = get_dropdown_y();

        // Compute dropdown width using context
        float dw = rm_max(100.f, m_submenus[m_open_submenu].cached_header_w);
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i].separator) continue;
            float w = bndLabelWidth(pctx, items[i].iconid, items[i].label.c_str()) + 30.f;
            if (w > dw) dw = w;
        }
        float dh = get_dropdown_height(m_open_submenu);

        int last_zindex = pctx->getZIndex();
        pctx->setZIndex(BUI_ZINDEX_DROPDOWN + 1);

        bndMenuBackground(pctx, dx, dy, dw, dh, BND_CORNER_NONE);

        float iy = dy;
        for (int i = 0; i < (int)items.size(); i++) {
            if (items[i].separator) {
                // Draw separator line
                const BNDtheme* theme = bndGetTheme();
                NVGcolor shade = bndOffsetColor(theme->backgroundColor, -20);
                pctx->beginPath();
                pctx->moveTo(dx + 4.f, iy + 4.f);
                pctx->lineTo(dx + dw - 4.f, iy + 4.f);
                pctx->StrokeWidth(1.0f);
                pctx->strokeColor(shade);
                pctx->stroke();
                iy += 8.f;
            } else {
                BNDwidgetState item_state = BND_DEFAULT;
                if (i == m_hover_item)
                    item_state = BND_HOVER;
                bndMenuItem(pctx, dx, iy, dw, (float)BND_WIDGET_HEIGHT,
                    item_state, items[i].iconid, items[i].label.c_str());
                iy += (float)BND_WIDGET_HEIGHT;
            }
        }
        pctx->setZIndex(last_zindex);
    }
}

bool bui_menubar::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);

    // Check if cursor is in header bar
    int hdr = header_hit_test(local);
    m_hover_header = hdr;

    // Check if cursor is in open dropdown
    int itm = item_hit_test(local);
    m_hover_item = itm;

    bool in_bar = (local.y >= 0 && local.y < m_size.y && local.x >= 0 && local.x < m_size.x);
    bool in_dropdown = false;
    if (m_open_submenu >= 0) {
        float dx = get_dropdown_x(m_open_submenu);
        float dy = get_dropdown_y();
        float dw = get_dropdown_width(m_open_submenu);
        float dh = get_dropdown_height(m_open_submenu);
        in_dropdown = (local.x >= dx && local.x < dx + dw &&
                       local.y >= dy && local.y < dy + dh);
    }

    if (event == RM_MOUSE_EVENT_CLICK && state == DOWN) {
        if (in_bar && hdr >= 0) {
            // Toggle submenu open/close
            if (m_open_submenu == hdr)
                close();
            else {
                m_open_submenu = hdr;
                m_hover_item = -1;
            }
            return false;
        }
        if (in_dropdown && itm >= 0) {
            // Item clicked
            const auto& items = m_submenus[m_open_submenu].items;
            if (!items[itm].separator) {
                int sub_id = m_submenus[m_open_submenu].id;
                int item_id = items[itm].id;
                close();
                if (is_valid_callback())
                    get_callback()(this, sub_id, item_id);
            }
            return false;
        }
        // Clicked outside — close menu
        if (m_open_submenu >= 0) {
            close();
            return false;
        }
    }

    if (event == RM_MOUSE_EVENT_MOVE) {
        // If menu is open and hovering over a different header, switch submenu
        if (m_open_submenu >= 0 && in_bar && hdr >= 0 && hdr != m_open_submenu) {
            m_open_submenu = hdr;
            m_hover_item = -1;
        }
    }

    // Consume events when menu is open and cursor is in interactive area
    if (m_open_submenu >= 0 && (in_bar || in_dropdown))
        return false;

    return true;
}

// ============================================================================
// bui_window
// ============================================================================
bui_window::bui_window(rm_widget* p_parent, int x, int y, int w, int h,
    bui_window_type type, const char* title, int iconid)
    : rm_widget(x, y, w, h, p_parent, "bui_window", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL | RM_FLAG_OPAQUE)
    , m_wtype(type), m_title(title ? title : ""), m_icon(iconid)
    , m_title_height(type == BUI_WINDOW_POPUP ? 0.f : 25.f)
    , m_corner_radius(4.f)
    , m_dragging(false)
{
    set_zindex(type == BUI_WINDOW_POPUP ? BUI_ZINDEX_POPUP : BUI_ZINDEX_OVERLAPPED);

    // Content area starts below title bar
    m_content_area.y = m_title_height;
    m_content_area.height = m_size.y - m_title_height;

    // Allow resize freely
    set_min_size(rm_vec2(80.f, m_title_height + 20.f));
    set_max_size(rm_vec2(0.f, 0.f));
}

void bui_window::on_draw(NVGcontext* pctx) {
    float cr = m_corner_radius;

    // Compensate for content_area.y offset applied by draw_recursive
    float offy = -m_content_area.y;

    // Drop shadow
    bndDropShadow(pctx, 0, offy, m_size.x, m_size.y, cr,
        2.f, 20.f);

    if (m_wtype == BUI_WINDOW_OVERLAPPED) {
        // Background
        bndBackground(pctx, 0, offy, m_size.x, m_size.y);
        bndBevel(pctx, 0, offy, m_size.x, m_size.y);

        // Title bar (drawn as tool button, like bui_panel "Properties")
        BNDwidgetState hdr_state = (m_dragging || get_elem_flags().is_focused())
            ? BND_ACTIVE : (get_elem_flags().is_hovered() ? BND_HOVER : BND_DEFAULT);
        bndToolButton(pctx, 0, offy, m_size.x, m_title_height,
            BND_CORNER_DOWN, hdr_state, m_icon, m_title.c_str());
    } else {
        // Popup: single rounded box
        const BNDtheme* theme = bndGetTheme();
        NVGcolor bg = bndOffsetColor(theme->backgroundColor, -5);
        bndInnerBox(pctx, 0, offy, m_size.x, m_size.y, cr, cr, cr, cr, bg, bg);

        // Outline
        bndOutlineBox(pctx, 0.5f, offy + 0.5f, m_size.x - 1.f, m_size.y - 1.f,
            cr, cr, cr, cr,
            NVGcolor::RGBA(0, 0, 0, 80));
    }
}

bool bui_window::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    bool inside = m_bbox.inside(cursor_pos);

    if (event == RM_MOUSE_EVENT_CLICK && state == DOWN && inside) {
        // Drag from title bar (overlapped) or anywhere (popup)
        bool in_title = (m_wtype == BUI_WINDOW_POPUP)
            || (local.y >= 0.f && local.y <= m_title_height);
        if (in_title) {
            m_dragging = true;
            m_drag_start_pos = m_pos_of_parent;
            m_drag_start_mouse = cursor_pos;
            return false;
        }
    }

    if (event == RM_MOUSE_EVENT_CLICK && state == UP && m_dragging) {
        m_dragging = false;
        return false;
    }

    if (event == RM_MOUSE_EVENT_MOVE && m_dragging) {
        rm_vec2 dm = cursor_pos - m_drag_start_mouse;
        m_pos_of_parent = rm_vec2(
            m_drag_start_pos.x + dm.x,
            m_drag_start_pos.y + dm.y
        );
        m_bbox.init(m_pos_of_parent, m_size);
        return false;
    }

    return true;
}

// ============================================================================
// bui_toolbox
// ============================================================================
bui_toolbox::bui_toolbox(rm_widget* p_parent, int x, int y, int size,
    bui_toolbox_anchor anchor, bui_toolbox_cb cb)
    : rm_widget(x, y,
        (anchor == BUI_ANCHOR_LEFT || anchor == BUI_ANCHOR_RIGHT) ? size : size,
        (anchor == BUI_ANCHOR_LEFT || anchor == BUI_ANCHOR_RIGHT) ? size : size,
        p_parent, "bui_toolbox",
        RM_FLAG_DEFAULT | RM_FLAG_DISABLE_SCISSOR)
    , m_anchor(anchor)
    , m_hover_id(-1), m_active_id(-1), m_active_subtool_id(-1)
    , m_lmb_down(false), m_lmb_down_time(0.f), m_lmb_down_tool_id(-1)
    , m_hold_threshold(0.5f)
    , m_popup_open(false), m_popup_tool_idx(-1), m_popup_hover_idx(-1)
    , m_resizing(false), m_resize_start_pos(0.f), m_resize_start_size(0.f)
    , m_button_size((float)size), m_is_expanded(false)
{
    set_callback(cb);
    set_max_size(rm_vec2(0.f, 0.f));
}

bool bui_toolbox::is_vertical() const {
    return m_anchor == BUI_ANCHOR_LEFT || m_anchor == BUI_ANCHOR_RIGHT;
}

int bui_toolbox::find_item_index(int tool_id) const {
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (!m_items[i].separator && m_items[i].id == tool_id)
            return i;
    }
    return -1;
}

float bui_toolbox::get_tool_offset(int idx) const {
    float off = 0.f;
    for (int i = 0; i < idx && i < (int)m_items.size(); i++) {
        off += m_items[i].separator ? 5.f : m_button_size;
    }
    return off;
}

int bui_toolbox::add_tool(int id, int iconid, const char* label) {
    bui_toolbox_item item;
    item.id = id;
    item.iconid = iconid;
    item.label = label ? label : "";
    item.separator = false;
    m_items.push_back(std::move(item));

    // Resize widget to fit all items
    float total = get_tool_offset((int)m_items.size());
    if (is_vertical()) {
        resize_nolayout(m_size.x, total);
    } else {
        resize_nolayout(total, m_size.y);
    }
    return id;
}

void bui_toolbox::add_multitool(int tool_id, int subtool_id, int iconid, const char* label) {
    int idx = find_item_index(tool_id);
    if (idx < 0) return;
    bui_multitool_item mt;
    mt.id = subtool_id;
    mt.iconid = iconid;
    mt.label = label ? label : "";
    m_items[idx].multitools.push_back(std::move(mt));
}

void bui_toolbox::add_separator() {
    bui_toolbox_item item;
    item.id = -1;
    item.iconid = -1;
    item.separator = true;
    m_items.push_back(std::move(item));

    float total = get_tool_offset((int)m_items.size());
    if (is_vertical()) {
        resize_nolayout(m_size.x, total);
    } else {
        resize_nolayout(total, m_size.y);
    }
}

int bui_toolbox::hit_test_tool(const rm_vec2& local) const {
    float off = 0.f;
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (m_items[i].separator) { off += 5.f; continue; }
        float bs = m_button_size;
        if (is_vertical()) {
            if (local.y >= off && local.y < off + bs &&
                local.x >= 0 && local.x < m_size.x)
                return m_items[i].id;
        } else {
            if (local.x >= off && local.x < off + bs &&
                local.y >= 0 && local.y < m_size.y)
                return m_items[i].id;
        }
        off += bs;
    }
    return -1;
}

int bui_toolbox::hit_test_popup(const rm_vec2& local) const {
    if (m_popup_tool_idx < 0 || m_popup_tool_idx >= (int)m_items.size())
        return -1;
    const auto& mts = m_items[m_popup_tool_idx].multitools;
    float tool_off = get_tool_offset(m_popup_tool_idx);
    float bs = m_button_size;

    // Popup position: adjacent to the button
    float px, py;
    if (is_vertical()) {
        // Popup to the right/left of the button
        px = (m_anchor == BUI_ANCHOR_LEFT) ? m_size.x : -120.f;
        py = tool_off;
    } else {
        px = tool_off;
        py = (m_anchor == BUI_ANCHOR_TOP) ? m_size.y : -((float)mts.size() * BND_WIDGET_HEIGHT);
    }
    float pw = 120.f;

    for (int i = 0; i < (int)mts.size(); i++) {
        float iy = py + i * (float)BND_WIDGET_HEIGHT;
        if (local.x >= px && local.x < px + pw &&
            local.y >= iy && local.y < iy + (float)BND_WIDGET_HEIGHT)
            return i;
    }
    return -1;
}

void bui_toolbox::draw_multitool_triangle(NVGcontext* pctx, float x, float y, float sz) const {
    pctx->beginPath();
    pctx->moveTo(x + sz, y + sz);
    pctx->lineTo(x, y + sz);
    pctx->lineTo(x + sz, y);
    pctx->closePath();
    pctx->fillColor(NVGcolor::RGBA(200, 200, 200, 180));
    pctx->fill();
}

void bui_toolbox::on_draw(NVGcontext* pctx) {
    // Check long-press for multitool popup
    if (m_lmb_down && !m_popup_open && m_psysdf) {
        float now = m_psysdf->get_time();
        int idx = find_item_index(m_lmb_down_tool_id);
        if (idx >= 0 && !m_items[idx].multitools.empty() &&
            (now - m_lmb_down_time) >= m_hold_threshold) {
            m_popup_open = true;
            m_popup_tool_idx = idx;
            m_popup_hover_idx = -1;
        }
    }

    const BNDtheme* theme = bndGetTheme();

    // Background
    {
        NVGcolor bg = bndOffsetColor(theme->backgroundColor, -10);
        bndInnerBox(pctx, 0, 0, m_size.x, m_size.y, 0, 0, 0, 0, bg, bg);
    }

    // Draw tool buttons
    float off = 0.f;
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (m_items[i].separator) {
            // Separator line
            NVGcolor sep = bndOffsetColor(theme->backgroundColor, -30);
            pctx->beginPath();
            if (is_vertical()) {
                pctx->moveTo(4.f, off + 2.5f);
                pctx->lineTo(m_size.x - 4.f, off + 2.5f);
            } else {
                pctx->moveTo(off + 2.5f, 4.f);
                pctx->lineTo(off + 2.5f, m_size.y - 4.f);
            }
            pctx->StrokeWidth(1.f);
            pctx->strokeColor(sep);
            pctx->stroke();
            off += 5.f;
            continue;
        }

        float bx = is_vertical() ? 0.f : off;
        float by = is_vertical() ? off : 0.f;
        float bs = m_button_size;

        bool is_active = (m_items[i].id == m_active_id);
        bool is_hover = (m_items[i].id == m_hover_id && !is_active);
        BNDwidgetState bstate = BND_DEFAULT;
        if (is_active) bstate = BND_ACTIVE;
        else if (is_hover) bstate = BND_HOVER;

        // Draw button using bndToolButton
        if (m_is_expanded && !m_items[i].label.empty()) {
            bndToolButton(pctx, bx, by,
                is_vertical() ? m_size.x : bs,
                is_vertical() ? bs : m_size.y,
                BND_CORNER_NONE, bstate, m_items[i].iconid,
                m_items[i].label.c_str());
        } else {
            bndToolButton(pctx, bx, by, bs, bs,
                BND_CORNER_NONE, bstate, m_items[i].iconid, nullptr);
        }

        // Multitool triangle indicator
        if (!m_items[i].multitools.empty()) {
            float tri_sz = 5.f;
            float tri_x = bx + bs - tri_sz - 2.f;
            float tri_y = by + bs - tri_sz - 2.f;
            draw_multitool_triangle(pctx, tri_x, tri_y, tri_sz);
        }

        off += bs;
    }

    // Draw multitool popup
    if (m_popup_open && m_popup_tool_idx >= 0 &&
        m_popup_tool_idx < (int)m_items.size()) {
        const auto& mts = m_items[m_popup_tool_idx].multitools;
        if (!mts.empty()) {
            float tool_off = get_tool_offset(m_popup_tool_idx);
            float px, py;
            if (is_vertical()) {
                px = (m_anchor == BUI_ANCHOR_LEFT) ? m_size.x : -120.f;
                py = tool_off;
            } else {
                px = tool_off;
                py = (m_anchor == BUI_ANCHOR_TOP) ? m_size.y
                    : -((float)mts.size() * BND_WIDGET_HEIGHT);
            }
            float pw = 120.f;
            float ph = (float)mts.size() * (float)BND_WIDGET_HEIGHT;

            int last_z = pctx->getZIndex();
            pctx->setZIndex(BUI_ZINDEX_DROPDOWN + 1);

            bndMenuBackground(pctx, px, py, pw, ph, BND_CORNER_NONE);
            for (int i = 0; i < (int)mts.size(); i++) {
                BNDwidgetState istate = (i == m_popup_hover_idx) ? BND_HOVER : BND_DEFAULT;
                bndMenuItem(pctx, px, py + i * (float)BND_WIDGET_HEIGHT,
                    pw, (float)BND_WIDGET_HEIGHT,
                    istate, mts[i].iconid, mts[i].label.c_str());
            }

            pctx->setZIndex(last_z);
        }
    }
}

bool bui_toolbox::on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
    RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    rm_vec2 local = cursor_to_local(cursor_pos);
    bool in_bbox = m_bbox.inside(cursor_pos);

    int tool_hit = in_bbox ? hit_test_tool(local) : -1;
    int popup_hit = m_popup_open ? hit_test_popup(local) : -1;
    m_hover_id = tool_hit;
    m_popup_hover_idx = popup_hit;

    if (event == RM_MOUSE_EVENT_CLICK && vk == RM_KEY_LMOUSE) {
        if (state == DOWN) {
            if (tool_hit >= 0) {
                m_lmb_down = true;
                m_lmb_down_time = m_psysdf ? m_psysdf->get_time() : 0.f;
                m_lmb_down_tool_id = tool_hit;
                return false;
            }
            if (m_popup_open && popup_hit < 0) {
                // Clicked outside popup — close it
                m_popup_open = false;
                m_lmb_down = false;
                return false;
            }
        } else if (state == UP) {
            if (m_popup_open) {
                // Select from popup
                if (popup_hit >= 0 && m_popup_tool_idx >= 0 &&
                    m_popup_tool_idx < (int)m_items.size()) {
                    const auto& mts = m_items[m_popup_tool_idx].multitools;
                    if (popup_hit < (int)mts.size()) {
                        m_active_id = m_items[m_popup_tool_idx].id;
                        m_active_subtool_id = mts[popup_hit].id;
                        if (is_valid_callback())
                            get_callback()(this, m_active_id, m_active_subtool_id, false);
                    }
                }
                m_popup_open = false;
                m_lmb_down = false;
                return false;
            }
            if (m_lmb_down) {
                m_lmb_down = false;
                // Short press — activate default
                if (tool_hit >= 0) {
                    m_active_id = tool_hit;
                    m_active_subtool_id = -1;
                    if (is_valid_callback())
                        get_callback()(this, m_active_id, -1, true);
                    return false;
                }
            }
        }
    }

    if (event == RM_MOUSE_EVENT_MOVE && m_popup_open) {
        m_popup_hover_idx = hit_test_popup(local);
    }

    // Resize by dragging edge
    if (event == RM_MOUSE_EVENT_CLICK && vk == RM_KEY_LMOUSE) {
        if (state == DOWN && in_bbox && !m_resizing) {
            float edge_tol = 5.f;
            bool on_edge = false;
            if (is_vertical()) {
                if (m_anchor == BUI_ANCHOR_LEFT && local.x >= m_size.x - edge_tol)
                    on_edge = true;
                else if (m_anchor == BUI_ANCHOR_RIGHT && local.x <= edge_tol)
                    on_edge = true;
            } else {
                if (m_anchor == BUI_ANCHOR_TOP && local.y >= m_size.y - edge_tol)
                    on_edge = true;
                else if (m_anchor == BUI_ANCHOR_BOTTOM && local.y <= edge_tol)
                    on_edge = true;
            }
            if (on_edge) {
                m_resizing = true;
                m_resize_start_pos = is_vertical() ? cursor_pos.x : cursor_pos.y;
                m_resize_start_size = is_vertical() ? m_size.x : m_size.y;
                return false;
            }
        } else if (state == UP && m_resizing) {
            m_resizing = false;
            // Snap: if past half of button_size, expand; otherwise collapse
            float cross = is_vertical() ? m_size.x : m_size.y;
            m_is_expanded = (cross > m_button_size * 1.5f);
            return false;
        }
    }

    if (event == RM_MOUSE_EVENT_MOVE && m_resizing) {
        float cur = is_vertical() ? cursor_pos.x : cursor_pos.y;
        float diff = cur - m_resize_start_pos;
        if (m_anchor == BUI_ANCHOR_RIGHT || m_anchor == BUI_ANCHOR_BOTTOM)
            diff = -diff;
        float new_cross = rm_max(m_button_size, m_resize_start_size + diff);
        if (is_vertical())
            resize_nolayout(new_cross, m_size.y);
        else
            resize_nolayout(m_size.x, new_cross);
        return false;
    }

    if (m_popup_open || m_lmb_down)
        return false;

    return true;
}
