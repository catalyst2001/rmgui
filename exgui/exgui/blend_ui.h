#pragma once
#include "rmgui.h"
#include "rmgui_controls.h"
#include "blendish.h"
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// Blendish-based UI Controls
// Standard GUI elements drawn using bnd* functions from blendish library
// ============================================================================

// ============================================================================
// bui_tooltip - Tooltip popup widget
// Uses: bndTooltipBackground, bndLabel
// ============================================================================
class bui_tooltip : public rm_widget {
    std::string m_text;
    int         m_icon;
    float       m_delay;       // seconds before showing
    float       m_timer;       // accumulated hover time
    bool        m_show;
    rm_widget*  m_target;      // widget to track

    virtual void on_draw(NVGcontext* pctx) override;
public:
    bui_tooltip(rm_widget* p_parent, const std::string& text,
        int iconid = -1, float delay = 0.5f);
    virtual ~bui_tooltip() = default;

    inline void set_text(const std::string& t) { m_text = t; }
    inline const std::string& get_text() const { return m_text; }
    inline void set_icon(int id) { m_icon = id; }
    inline void set_delay(float d) { m_delay = d; }
    void show_at(float x, float y);
    void dismiss();
};

// ============================================================================
// bui_toolbar - Toolbar container with grouped tool buttons
// Uses: bndToolButton, bndBackground
// ============================================================================
class bui_toolbar;
using bui_toolbar_cb = void(*)(bui_toolbar* ptoolbar, int button_id);

struct bui_tool_item {
    int         id;
    int         iconid;
    std::string label;
    bool        separator;     // true = separator, not a button
};

class bui_toolbar : public rm_widget, public rm_callback<bui_toolbar_cb> {
    std::vector<bui_tool_item> m_items;
    int                        m_hover_id;
    int                        m_active_id;
    bool                       m_vertical;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

    int hit_test_item(const rm_vec2& local) const;
public:
    bui_toolbar(rm_widget* p_parent, int x, int y, int width, int height,
        bool vertical = false, bui_toolbar_cb cb = nullptr);
    virtual ~bui_toolbar() = default;

    void add_button(int id, int iconid, const char* label = nullptr);
    void add_separator();
    inline int  get_active_id() const { return m_active_id; }
    inline void set_active_id(int id) { m_active_id = id; }
    inline bool is_vertical() const { return m_vertical; }
};

// ============================================================================
// bui_color_button - Color selection button
// Uses: bndColorButton
// ============================================================================
class bui_color_button;
using bui_color_button_cb = void(*)(bui_color_button* pbtn, NVGcolor color);

class bui_color_button : public rm_widget, public rm_callback<bui_color_button_cb> {
    NVGcolor m_color;
    int      m_corner_flags;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_color_button(rm_widget* p_parent, int x, int y, int w, int h,
        NVGcolor color, int corner_flags = BND_CORNER_NONE,
        bui_color_button_cb cb = nullptr);
    virtual ~bui_color_button() = default;

    inline void     set_color(NVGcolor c) { m_color = c; }
    inline NVGcolor get_color() const { return m_color; }
};

// ============================================================================
// bui_panel - Collapsible panel / group box
// Uses: bndBackground, bndBevel, bndLabel, bndIconLabelValue
// ============================================================================
class bui_panel;
using bui_panel_cb = void(*)(bui_panel* ppanel, bool collapsed);

class bui_panel : public rm_widget, public rm_callback<bui_panel_cb> {
    std::string m_title;
    int         m_icon;
    bool        m_collapsed;
    float       m_header_height;
    float       m_expanded_height; // remembered height when expanded

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_panel(rm_widget* p_parent, int x, int y, int w, int h,
        const char* title, int iconid = -1, bui_panel_cb cb = nullptr);
    virtual ~bui_panel() = default;

    inline bool is_collapsed() const { return m_collapsed; }
    void set_collapsed(bool c);
    void toggle_collapsed();
    inline void set_title(const char* t) { m_title = t; }
    inline const char* get_title() const { return m_title.c_str(); }

    // Content area starts below header
    rm_rect get_content_rect() const;
};

// ============================================================================
// bui_number_field - Blender-style number field with drag editing
// Uses: bndNumberField
// ============================================================================
class bui_number_field;
using bui_number_field_cb = void(*)(bui_number_field* pfield, float value);

class bui_number_field : public rm_widget, public rm_callback<bui_number_field_cb> {
    std::string m_label;
    float       m_value;
    float       m_min;
    float       m_max;
    float       m_step;
    int         m_precision;   // decimal digits in display
    int         m_corner_flags;
    bool        m_dragging;
    float       m_drag_start_x;
    float       m_drag_start_value;
    char        m_value_buf[32];

    void format_value();
    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_number_field(rm_widget* p_parent, int x, int y, int w, int h,
        const char* label, float value = 0.f,
        float minval = 0.f, float maxval = 100.f, float step = 0.1f,
        int precision = 2, int corner_flags = BND_CORNER_NONE,
        bui_number_field_cb cb = nullptr);
    virtual ~bui_number_field() = default;

    inline float get_value() const { return m_value; }
    void  set_value(float v);
    inline void  set_label(const char* l) { m_label = l; }
    inline void  set_range(float mn, float mx) { m_min = mn; m_max = mx; }
    inline void  set_step(float s) { m_step = s; }
    inline void  set_precision(int p) { m_precision = p; }
};

// ============================================================================
// bui_option_button - Blender-style option button (checkbox with blendish look)
// Uses: bndOptionButton
// ============================================================================
class bui_option_button;
using bui_option_button_cb = void(*)(bui_option_button* pbtn, bool checked);

class bui_option_button : public rm_widget, public rm_callback<bui_option_button_cb> {
    std::string m_label;
    bool        m_checked;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_option_button(rm_widget* p_parent, int x, int y, int w, int h,
        const char* label, bool checked = false,
        bui_option_button_cb cb = nullptr);
    virtual ~bui_option_button() = default;

    inline bool is_checked() const { return m_checked; }
    inline void set_checked(bool c) { m_checked = c; }
    inline void set_label(const char* l) { m_label = l; }
};

// ============================================================================
// bui_choice_button - Blender-style choice/dropdown button
// Uses: bndChoiceButton, bndMenuBackground, bndMenuItem
// ============================================================================
class bui_choice_button;
using bui_choice_button_cb = void(*)(bui_choice_button* pbtn, int index);

struct bui_choice_item {
    int         id;
    int         iconid;
    std::string label;
};

class bui_choice_button : public rm_widget, public rm_callback<bui_choice_button_cb> {
    std::vector<bui_choice_item> m_items;
    int                          m_selected;
    int                          m_hover_idx;
    bool                         m_expanded;
    int                          m_icon;
    int                          m_corner_flags;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_choice_button(rm_widget* p_parent, int x, int y, int w, int h,
        int iconid = -1, int corner_flags = BND_CORNER_NONE,
        bui_choice_button_cb cb = nullptr);
    virtual ~bui_choice_button() = default;

    void add_item(const char* label, int id = -1, int iconid = -1);
    inline int  get_selected() const { return m_selected; }
    void set_selected(int idx);
    inline size_t get_num_items() const { return m_items.size(); }
    inline const bui_choice_item* get_item(size_t idx) const {
        return (idx < m_items.size()) ? &m_items[idx] : nullptr;
    }
    inline bool is_expanded() const { return m_expanded; }
};

// ============================================================================
// bui_splitter - Resizable split pane divider
// Uses: bndSplitterWidgets, bndBackground
// ============================================================================
class bui_splitter;
using bui_splitter_cb = void(*)(bui_splitter* psplitter, float ratio);

class bui_splitter : public rm_widget, public rm_callback<bui_splitter_cb> {
    float  m_ratio;           // 0..1, position of split
    bool   m_vertical;        // true = vertical divider (left|right)
    bool   m_dragging;
    float  m_thickness;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_splitter(rm_widget* p_parent, int x, int y, int w, int h,
        bool vertical = false, float initial_ratio = 0.5f,
        float thickness = 6.f, bui_splitter_cb cb = nullptr);
    virtual ~bui_splitter() = default;

    inline float get_ratio() const { return m_ratio; }
    inline void  set_ratio(float r) { m_ratio = rm_clamp(r, 0.05f, 0.95f); }
    inline bool  is_vertical() const { return m_vertical; }

    // Get rects for the two split areas
    rm_rect get_first_rect() const;
    rm_rect get_second_rect() const;
};

// ============================================================================
// bui_separator - Horizontal or vertical divider line
// Uses: bndBevel
// ============================================================================
class bui_separator : public rm_widget {
    bool m_vertical;
    virtual void on_draw(NVGcontext* pctx) override;
public:
    bui_separator(rm_widget* p_parent, int x, int y, int length,
        bool vertical = false);
    virtual ~bui_separator() = default;
};

// ============================================================================
// bui_radio_button - Blendish-style radio button group
// Uses: bndRadioButton
// ============================================================================
class bui_radio_button;
using bui_radio_button_cb = void(*)(bui_radio_button* pbtn, int selected);

struct bui_radio_item {
    int         id;
    int         iconid;
    std::string label;
};

class bui_radio_button : public rm_widget, public rm_callback<bui_radio_button_cb> {
    std::vector<bui_radio_item> m_items;
    int  m_selected;
    int  m_hover_idx;
    bool m_vertical;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
    int hit_test(const rm_vec2& local) const;
public:
    bui_radio_button(rm_widget* p_parent, int x, int y, int w, int h,
        bool vertical = false, bui_radio_button_cb cb = nullptr);
    virtual ~bui_radio_button() = default;

    void add_item(const char* label, int id = -1, int iconid = -1);
    inline int get_selected() const { return m_selected; }
    void set_selected(int idx);
    inline size_t get_num_items() const { return m_items.size(); }
};

// ============================================================================
// bui_text_field - Blendish-style editable text field
// Uses: bndTextField
// ============================================================================
class bui_text_field;
using bui_text_field_cb = void(*)(bui_text_field* pfield, const char* text);

class bui_text_field : public rm_widget, public rm_callback<bui_text_field_cb> {
    std::string m_text;
    int         m_icon;
    int         m_corner_flags;
    int         m_cursor;        // cursor position (character index)
    int         m_sel_start;     // selection start (-1 = no selection)
    bool        m_editing;       // currently focused for editing

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
    virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
    virtual void on_text_input(int sym) override;

    void insert_char(int ch);
    void delete_selection();
    bool has_selection() const;
    int  sel_min() const;
    int  sel_max() const;
public:
    bui_text_field(rm_widget* p_parent, int x, int y, int w, int h,
        const char* text = "", int iconid = -1,
        int corner_flags = BND_CORNER_NONE,
        bui_text_field_cb cb = nullptr);
    virtual ~bui_text_field() = default;

    inline void set_text(const char* t) { m_text = t ? t : ""; m_cursor = (int)m_text.size(); m_sel_start = -1; }
    inline const char* get_text() const { return m_text.c_str(); }
    inline void set_icon(int id) { m_icon = id; }
    inline bool is_editing() const { return m_editing; }
};

// ============================================================================
// bui_slider - Blendish-style slider with label and value display
// Uses: bndSlider
// ============================================================================
class bui_slider;
using bui_slider_cb = void(*)(bui_slider* pslider, float value);

class bui_slider : public rm_widget, public rm_callback<bui_slider_cb> {
    std::string m_label;
    float       m_value;
    float       m_min;
    float       m_max;
    int         m_precision;
    int         m_corner_flags;
    bool        m_dragging;
    char        m_value_buf[32];

    void format_value();
    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_slider(rm_widget* p_parent, int x, int y, int w, int h,
        const char* label, float value = 0.f,
        float minval = 0.f, float maxval = 1.f,
        int precision = 2, int corner_flags = BND_CORNER_NONE,
        bui_slider_cb cb = nullptr);
    virtual ~bui_slider() = default;

    inline float get_value() const { return m_value; }
    void set_value(float v);
    inline void set_label(const char* l) { m_label = l; }
    inline void set_range(float mn, float mx) { m_min = mn; m_max = mx; }
};

// ============================================================================
// bui_scrollbar - Blendish-style scrollbar
// Uses: bndScrollBar
// ============================================================================
class bui_scrollbar;
using bui_scrollbar_cb = void(*)(bui_scrollbar* pbar, float offset);

class bui_scrollbar : public rm_widget, public rm_callback<bui_scrollbar_cb> {
    float m_offset;       // 0..1
    float m_handle_size;  // 0..1 (visible portion)
    bool  m_vertical;
    bool  m_dragging;
    float m_drag_start;
    float m_drag_start_offset;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_scrollbar(rm_widget* p_parent, int x, int y, int w, int h,
        bool vertical = false, float handle_size = 0.2f,
        bui_scrollbar_cb cb = nullptr);
    virtual ~bui_scrollbar() = default;

    inline float get_offset() const { return m_offset; }
    inline void  set_offset(float o) { m_offset = rm_clamp(o, 0.f, 1.f); }
    inline void  set_handle_size(float s) { m_handle_size = rm_clamp(s, 0.05f, 1.f); }
    inline float get_handle_size() const { return m_handle_size; }
};

// ============================================================================
// bui_node_port - Node editor port/pin
// Uses: bndNodePort
// ============================================================================
class bui_node_port : public rm_widget {
    NVGcolor    m_color;
    bool        m_connected;
    bool        m_input;     // true=input port, false=output
    std::string m_label;

    virtual void on_draw(NVGcontext* pctx) override;
public:
    bui_node_port(rm_widget* p_parent, int x, int y,
        const char* label = "",
        NVGcolor color = NVGcolor::RGB(200, 200, 200),
        bool is_input = true);
    virtual ~bui_node_port() = default;

    inline void set_color(NVGcolor c) { m_color = c; }
    inline NVGcolor get_color() const { return m_color; }
    inline void set_connected(bool c) { m_connected = c; }
    inline bool is_connected() const { return m_connected; }
    inline bool is_input() const { return m_input; }
    inline const char* get_label() const { return m_label.c_str(); }
    rm_vec2 get_center() const;
};

// ============================================================================
// bui_node - Node editor node with ports
// Uses: bndNodeBackground, bndLabel
// ============================================================================
class bui_node;
using bui_node_cb = void(*)(bui_node* pnode);

class bui_node : public rm_widget, public rm_callback<bui_node_cb> {
    std::string m_title;
    int         m_icon;
    NVGcolor    m_title_color;
    bool        m_dragging;
    rm_vec2     m_drag_offset;
    std::vector<bui_node_port*>  m_inputs;
    std::vector<bui_node_port*>  m_outputs;
    std::vector<std::string>     m_input_labels;
    std::vector<std::string>     m_output_labels;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_node(rm_widget* p_parent, int x, int y, int w,
        const char* title, int iconid = -1,
        NVGcolor title_color = NVGcolor::RGBA(100, 100, 100, 255),
        bui_node_cb cb = nullptr);
    virtual ~bui_node() = default;

    bui_node_port* add_input(const char* label, NVGcolor color = NVGcolor::RGB(200, 200, 200));
    bui_node_port* add_output(const char* label, NVGcolor color = NVGcolor::RGB(200, 200, 200));
    inline const char* get_title() const { return m_title.c_str(); }
    inline void set_title(const char* t) { m_title = t; }
    size_t get_num_inputs() const { return m_inputs.size(); }
    size_t get_num_outputs() const { return m_outputs.size(); }
    bui_node_port* get_input(size_t idx) { return idx < m_inputs.size() ? m_inputs[idx] : nullptr; }
    bui_node_port* get_output(size_t idx) { return idx < m_outputs.size() ? m_outputs[idx] : nullptr; }
};

// ============================================================================
// bui_node_wire_renderer - Draws Bezier wires between node ports
// Uses: bndNodeWire, bndColoredNodeWire
// Not a widget, utility class
// ============================================================================
struct bui_wire_connection {
    bui_node_port* from;
    bui_node_port* to;
};

class bui_node_wire_renderer {
public:
    static void draw_wire(NVGcontext* pctx,
        rm_vec2 from, rm_vec2 to,
        BNDwidgetState state_from = BND_DEFAULT,
        BNDwidgetState state_to = BND_DEFAULT);
    static void draw_colored_wire(NVGcontext* pctx,
        rm_vec2 from, NVGcolor color_from,
        rm_vec2 to, NVGcolor color_to);
};

// ============================================================================
// bui_menu_label - Menu section header label
// Uses: bndMenuLabel
// ============================================================================
class bui_menu_label : public rm_widget {
    std::string m_text;
    int         m_icon;

    virtual void on_draw(NVGcontext* pctx) override;
public:
    bui_menu_label(rm_widget* p_parent, int x, int y, int w,
        const char* text, int iconid = -1);
    virtual ~bui_menu_label() = default;
    inline void set_text(const char* t) { m_text = t ? t : ""; }
};

// ============================================================================
// bui_color_picker - HSV color picker with preview
// ============================================================================
class bui_color_picker;
using bui_color_picker_cb = void(*)(bui_color_picker* ppicker, NVGcolor color);

class bui_color_picker : public rm_widget, public rm_callback<bui_color_picker_cb> {
    float    m_hue;        // 0..1
    float    m_sat;        // 0..1
    float    m_val;        // 0..1
    float    m_alpha;      // 0..1
    bool     m_dragging_wheel;
    bool     m_dragging_triangle;
    float    m_wheel_inner_r;
    float    m_wheel_outer_r;

    void hsv_to_rgb(float h, float s, float v, float& r, float& g, float& b) const;
    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_color_picker(rm_widget* p_parent, int x, int y, int size,
        NVGcolor initial = NVGcolor::RGB(255, 0, 0),
        bui_color_picker_cb cb = nullptr);
    virtual ~bui_color_picker() = default;

    NVGcolor get_color() const;
    void set_color(NVGcolor c);
    inline float get_hue() const { return m_hue; }
    inline float get_saturation() const { return m_sat; }
    inline float get_value() const { return m_val; }
};

// ============================================================================
// bui_knob - Rotary dial control
// ============================================================================
class bui_knob;
using bui_knob_cb = void(*)(bui_knob* pknob, float value);

class bui_knob : public rm_widget, public rm_callback<bui_knob_cb> {
    std::string m_label;
    float       m_value;
    float       m_min;
    float       m_max;
    bool        m_dragging;
    float       m_drag_start_y;
    float       m_drag_start_value;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
    bui_knob(rm_widget* p_parent, int x, int y, int diameter,
        const char* label, float value = 0.f,
        float minval = 0.f, float maxval = 1.f,
        bui_knob_cb cb = nullptr);
    virtual ~bui_knob() = default;

    inline float get_value() const { return m_value; }
    void set_value(float v);
    inline void set_label(const char* l) { m_label = l; }
    inline void set_range(float mn, float mx) { m_min = mn; m_max = mx; }
};

// ============================================================================
// bui_radio_toolbar - Toolbar with radio-button visual style
// Uses: bndRadioButton
// Supports vertical/horizontal orientation, separators, icon+label items
// ============================================================================
class bui_radio_toolbar;
using bui_radio_toolbar_cb = void(*)(bui_radio_toolbar* ptoolbar, int button_id);

struct bui_radio_tool_item {
    int         id;
    int         iconid;
    std::string label;
    bool        separator;
};

class bui_radio_toolbar : public rm_widget, public rm_callback<bui_radio_toolbar_cb> {
    std::vector<bui_radio_tool_item> m_items;
    int    m_hover_id;
    int    m_active_id;
    bool   m_vertical;

    virtual void on_draw(NVGcontext* pctx) override;
    virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk,
        RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

    int hit_test_item(const rm_vec2& local) const;
public:
    bui_radio_toolbar(rm_widget* p_parent, int x, int y, int w, int h,
        bool vertical = false, bui_radio_toolbar_cb cb = nullptr);
    virtual ~bui_radio_toolbar() = default;

    void add_button(int id, int iconid, const char* label = nullptr);
    void add_separator();
    inline int  get_active_id() const { return m_active_id; }
    inline void set_active_id(int id) { m_active_id = id; }
    inline bool is_vertical() const { return m_vertical; }
};
