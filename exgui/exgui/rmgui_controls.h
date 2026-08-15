#pragma once
#include "rmgui.h"
#include "rmgui_behaviour.h"
#include "rmgui_theme.h"
#include <string>
#include <vector>
#include <functional>
#include <type_traits>
#include <map>
#include <utility>

class rmgui_image {
public:
  NVGhandle imageId;
  int width;
  int height;
  int channels;

  rmgui_image();
  ~rmgui_image();

  bool load(const std::string& filename, NVGcontext* ctx);
};

class rm_image_button : public rm_widget {
  rmgui_image* m_image;
public:
  rm_image_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& imageFile);
  virtual ~rm_image_button();
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
};

class rm_button : public rm_widget {
  std::string m_text;
  RmButtonBehaviour m_behaviour;
  RmThemeRef m_theme;
  RmButtonVariant m_variant;
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
public:
  rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text,
    RmThemeRef theme = {}, RmButtonVariant variant = RmButtonVariant::primary);
  virtual ~rm_button();
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
  const RmButtonBehaviour& behaviour() const { return m_behaviour; }
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
  RmButtonVariant get_variant() const noexcept { return m_variant; }
  void set_variant(RmButtonVariant variant) noexcept { m_variant = variant; }
};

class rm_label : public rm_widget {
  std::string m_text;
  RmThemeRef m_theme;
public:
  rm_label(rm_widget* p_parent, int x, int y, const std::string& text, RmThemeRef theme = {});
  virtual ~rm_label();
  virtual void on_draw(NVGcontext* pctx) override;
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
};

/**
* TEXT INPUT
*/

enum RMGUI_TEXT_INPUT_FLAGS {
  RMGUI_TEXT_INPUT_SINGLELINE = 0,
  RMGUI_TEXT_INPUT_MULTILINE = 1 << 0
};

class rm_text_input_style : public rm_corners_style {
  NVGcolor      m_text_color;
  NVGcolor      m_active_bg_color;
  NVGcolor      m_unactive_bg_color;
  NVGcolor      m_border_color;
  NVGcolor      m_blink_color;
  NVGcolor      m_selection_color;
  float         m_font_size;
  float         m_border_width;
  float         m_text_offset;
  float         m_blink_width;
  bool          m_rounded_selection;
public:
  rm_text_input_style() :
    m_text_color(NVGcolor::RGB(0, 0, 0)),
    m_active_bg_color(NVGcolor::RGB(255, 255, 255)),
    m_unactive_bg_color(NVGcolor::RGBA(220, 220, 220, 255)),
    m_border_color(NVGcolor::RGBA(255, 255, 255, 255)),
    m_blink_color(NVGcolor::RGB(0, 0, 0)),
    m_selection_color(NVGcolor::RGBA(51, 153, 255, 128)),
    m_font_size(14.f), 
    m_border_width(1.f), 
    m_text_offset(1.f), m_blink_width(1.f), m_rounded_selection(false) {
  }

  inline const void     set_rounded_selection(bool enable) { m_rounded_selection = enable; }
  inline bool           has_rounded_selection() const { return m_rounded_selection; }

  /* selectors  */
  inline const NVGcolor& get_text_color() const { return m_text_color; }
  inline const NVGcolor& get_active_bgr_color() const { return m_active_bg_color; }
  inline const NVGcolor& get_unactive_bgr_color() const { return m_unactive_bg_color; }
  inline const NVGcolor& get_border_color() const { return m_border_color; }
  inline const NVGcolor& get_blink_color() const { return m_blink_color; }
  inline const NVGcolor& get_selection_color() const { return m_selection_color; }
  inline float           get_font_size() const { return m_font_size; }
  inline float           get_border_width() const { return m_border_width; }
  inline float           get_blink_width() const { return m_blink_width; }
  inline float           get_text_offset() const { return m_text_offset; }

  /* modifiers */
  inline void set_text_color(NVGcolor clr) { m_text_color = clr; }
  inline void set_active_bgr_color(NVGcolor clr) { m_active_bg_color = clr; }
  inline void set_unactive_bgr_color(NVGcolor clr) { m_unactive_bg_color = clr; }
  inline void set_border_color(NVGcolor clr) { m_border_color = clr; }
  inline void set_blink_color(NVGcolor clr) { m_blink_color = clr; }
  inline void set_selection_color(NVGcolor clr) { m_selection_color = clr; }
  inline void set_font_size(float fsize) { m_font_size = fsize; }
  inline void set_border_width(float bsize) { m_border_width = bsize; }
  inline void set_blink_width(float offset) { m_blink_width = offset; }
  inline void set_text_offsets(float offset) { m_text_offset = offset; }
};

class rm_text_input : public rm_widget, public rm_styled<rm_text_input_style> {
  std::vector<float>              m_glyph_positions;
  rmgui_textbuffer                m_buffer;
  bool                            m_ctrl_pressed;
  bool                            m_active;
  bool                            m_dragging;
  rmgui_timer                     m_timer;
  bool                            m_blink_state;
  uint32_t                        m_flags;
  float                           m_scroll_offset;
  double                          m_last_click_time;
  rm_vec2                         m_last_click_pos;

  std::vector<size_t>             m_line_starts;
  std::vector<std::vector<float>> m_line_glyphs;
  float                           m_asc;
  float                           m_line_h;

  static constexpr double DOUBLE_CLICK_THRESHOLD = 0.35;
  static constexpr float  CLICK_MOVE_THRESHOLD = 4.f;
public:
  rm_text_input(rm_widget* p_parent, int x, int y, int width, int height, rm_text_input_style* pstyle, uint32_t flags/* = RMGUI_TEXT_INPUT_SINGLELINE*/, float blink_cursor_interval = 0.5f);
  virtual ~rm_text_input();
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual void on_text_input(int sym) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

  // map local xy-coordinate to character index
  size_t hit_test_index(float px, float py = NAN) const;

  void ensure_visible(size_t idx);
};

/**
* =============================================
* CheckBox
* 
* 
* =============================================
*/
class rm_checkbox;
using rm_checkbox_cb = bool (*)(rm_checkbox *pcheckbox);
class rm_checkbox : public rm_widget, public rm_callback<rm_checkbox_cb> {
  RmToggleBehaviour m_behaviour;
  std::string    m_label;
  RmThemeRef     m_theme;

  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_checkbox(rm_widget* p_parent, int x, int y, int width,
    const std::string& label, rm_checkbox_cb pcallback = nullptr, RmThemeRef theme = {});
  virtual ~rm_checkbox();
  inline bool        is_checked() const { return m_behaviour.is_checked(); }
  inline const char* get_label() const { return m_label.c_str(); }
  inline void        set_label(const char* plabeltext) { m_label.assign(plabeltext); }
  inline void        set_checked(bool val) { m_behaviour.set_checked(val); }
  const RmToggleBehaviour& behaviour() const { return m_behaviour; }
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
};

/**
* =============================================
* ComboBox
*
*
* =============================================
*/
class rm_combo_item {
  std::string name;
  void* pdata;
public:
  rm_combo_item() : pdata(nullptr) {}
  rm_combo_item(const char* pname, void* userptr = nullptr) : name(pname), pdata(userptr) {}

  inline const char* get_name() const { return name.c_str(); }
  inline void* get_userdata() const { return pdata; }
};

class rm_combobox;
using rm_combobox_cb = void(*)(rm_combobox *pcombo, rm_combo_item *pitem, size_t itemid);
class rm_combobox : public rm_widget, public rm_callback<rm_combobox_cb> {
  std::vector<rm_combo_item> m_items;
  int  m_selected;
  bool m_expanded;
  rm_vec2 m_cursor;

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  const size_t kinvalid_index = ((size_t)-1);
  rm_combobox(rm_widget* p_parent, int x, int y, int width, int height, rm_combobox_cb pcallback=nullptr);
  virtual ~rm_combobox();

  inline size_t get_num_items() const { return m_items.size(); }
  size_t add_item(const char *pitem, void *puserdata=nullptr);
  size_t find_item(const char* pitem);
  inline rm_combo_item* get_item(size_t idx) {
    assert(idx < m_items.size() && "item index out of bounds");
    return &m_items[idx];
  }
};

/**
* =============================================
* Slider
*
*
* =============================================
*/
class rm_slider;
using rm_slider_callback = void(*)(rm_slider *pslider);
class rm_slider : public rm_widget {
  RmSliderBehaviour m_behaviour;
  rm_slider_callback m_pcallback;
  RmThemeRef m_theme;
  void    update_value_from_pointer(const rm_vec2& local_cursor);
  void    on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void    on_pointer_capture_lost() override { m_behaviour.cancel(); }

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta);

public:
  rm_slider(rm_widget* p_parent, int x, int y, int width, int height,
    float min, float max, float initial, rm_slider_callback pcallback = nullptr, RmThemeRef theme = {});
  virtual ~rm_slider();
  float get_value() const { return m_behaviour.value(); }
  void set_value(float value) { m_behaviour.set_value(value); }
  const RmSliderBehaviour& behaviour() const { return m_behaviour; }
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
};

/**
* =============================================
* Default progress control
*
*
* =============================================
*/
class rm_progress : public rm_widget
{
protected:
  RmProgressBehaviour m_behaviour;
  RmThemeRef m_theme;
public:
  rm_progress(rm_widget* p_parent, int x, int y, int width, int height,
    float initial = 0.1f, RmThemeRef theme = {});
  virtual ~rm_progress();
  void         set_percent(float percent) { m_behaviour.set_percent(percent); }
  inline float get_percent() const { return m_behaviour.percent(); }
  const RmProgressBehaviour& behaviour() const { return m_behaviour; }
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
  virtual void on_draw(NVGcontext* pctx) override;
};

/**
* =============================================
* Progress Image
*
*
* =============================================
*/
class rm_progress_image : public rm_progress
{
  rm_image m_image;
  float    m_angle;
  float    m_alpha;
public:
  rm_progress_image(rm_widget* p_parent, int x, int y, int width, int height, 
    rm_image img, float pattern_angle = 0.f, float pattern_alpha = 1.f,
    float initial = 0.1f, RmThemeRef theme = {});
  ~rm_progress_image();
  inline void     set_image(rm_image img) { m_image = img; }
  inline rm_image get_image() const { return m_image; }
  virtual void    on_draw(NVGcontext* pctx) override;
};


/**
* internal scroll for inheritance
*/

/*
    horizontal scroll:
    <--------------------------------> (WIDTH)
    .--------------------------------.
    |[0000]                          |
    `--------------------------------`
    
    vertical scroll
     <---> (WIDTH)
     .---.
     |[0]|
     |[0]|
     |   |
     |   |
     |   |
     |   |
     |   |
     |   |
     `---`
*/
class rm_scroll_style
{
  NVGcolor  m_background_clr;
  NVGcolor  m_thumb_clr;
  NVGcolor  m_background_border_clr;
  NVGcolor  m_thumb_border_clr;
  float     m_corner_round;
  float     m_thumb_size;
  float     m_background_stroke_width;
  float     m_thumb_stroke_width;
public:
  void load_defaults() {
    m_background_clr = NVGcolor::RGB(30, 30, 30);
    m_thumb_clr = NVGcolor::RGB(80, 80, 80);
    m_background_border_clr = NVGcolor::RGB(100, 100, 100);
    m_thumb_border_clr = NVGcolor::RGB(100, 100, 100);
    m_corner_round = 0.f;
    m_thumb_size = 10.f;
    m_background_stroke_width = 1.f;
    m_thumb_stroke_width = 1.f;
  }
  rm_scroll_style() {
    load_defaults();
  }

  /* selectors */
  inline const NVGcolor &get_scroll_background_color() const { return m_background_clr; }
  inline const NVGcolor &get_scroll_thumb_color() const { return m_thumb_clr; }
  inline const NVGcolor &get_scroll_background_border_color() const { return m_background_border_clr; }
  inline const NVGcolor &get_scroll_thumb_border_color() const { return m_thumb_border_clr; }
  inline float           get_scroll_corner_radius() const { return m_corner_round; }
  inline float           get_thumb_size() const { return m_thumb_size; }
  inline float           get_background_stroke_width() const { return m_background_stroke_width; }
  inline float           get_thumb_stroke_width() const { return m_thumb_stroke_width; }

  /* modifiers */
  inline void set_scroll_background_color(NVGcolor color) { m_background_clr = color; }
  inline void set_scroll_thumb_color(NVGcolor color) { m_thumb_clr = color; }
  inline void set_scroll_background_border_color(NVGcolor color) { m_background_border_clr = color; }
  inline void set_scroll_thumb_border_color(NVGcolor color) { m_thumb_border_clr = color; }
  inline void set_scroll_corner_round(float radius) { m_corner_round = radius; }
  inline void set_thumb_size(float size) { m_thumb_size= size; }
  inline void set_background_stroke_width(float width) { m_background_stroke_width = width; }
  inline void set_thumb_stroke_width(float width) { m_thumb_stroke_width = width; }
};

class rm_scroll_base
{
  RM_ORIENT m_orientation;
protected:
  inline void      set_orient(RM_ORIENT orient) { m_orientation = orient; }
  inline RM_ORIENT get_orient() const { return m_orientation; }
  void orient_detect(const rm_rect &background);
  void draw_scroll(NVGcontext* pctx, const rm_vec2&back, rm_scroll_style *pstyle, float pos);
  void draw_scroll(NVGcontext* pctx, rm_scroll_style* pstyle, rm_vec2 content_size, const rm_vec2& window_size, float thumb_height, float pos);
};

class rm_scrollbar;
using rm_scrollbar_cb = void(*)(rm_scrollbar *pscrollbar, float value);
class rm_scrollbar : public rm_widget,
  rm_styled<rm_scroll_style>,
  rm_callback<rm_scrollbar_cb>,
  public rm_scroll_base
{
  float m_position;
  rm_widget* find_other_scrollbars();
  void       adjust_position();

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta);
public:
  rm_scrollbar(rm_widget* p_parent, RM_ORIENT orient, rm_scroll_style *p_style, float inital_pos=0.f);
  ~rm_scrollbar();

  inline void  set_position(float pos) { m_position = pos; }
  inline float get_position() const { return m_position; }
};

/**
* =============================================
* Animation
*
*
* =============================================
*/
class rm_animation : public rm_widget
{
  rm_image m_image;
  float    m_speed;
  float    m_angle;
  float    m_scale;
  virtual void on_draw(NVGcontext* pctx) override;
public:
  rm_animation(rm_widget* p_parent, int x, int y, int width, int height, rm_image img, float start_angle=0.f, float scale=1.f, float speed=1.f);
  ~rm_animation();
  inline void     set_image(rm_image img) { m_image = img; }
  inline rm_image get_image() const { return m_image; }
  inline void     set_angle(float angle) { m_angle = angle; }
  inline float    get_angle() const { return m_angle; }
  inline void     set_speed(float speed) { m_speed = speed; }
  inline float    get_speed() const { return m_speed; }
  inline void     set_scale(float scl) { m_scale = scl; }
  inline float    get_scale() const { return m_scale; }
};

/**
 * =============================================
 * Tab Control
 * =============================================
 */
class rm_tab_item {
  std::string m_name;
  uint32_t m_id;
  void* m_pdata;
  rm_widget* m_pwidget;
  bool m_closable;
  bool m_pinned;
public:
  rm_tab_item(const char* p_name, uint32_t id, rm_widget* p_widget,
    bool closable, bool pinned, void* p_userdata = nullptr) :
    m_name(p_name ? p_name : ""), m_id(id), m_pdata(p_userdata),
    m_pwidget(p_widget), m_closable(closable), m_pinned(pinned) {
  }

  const char* get_name() const noexcept { return m_name.c_str(); }
  uint32_t get_id() const noexcept { return m_id; }
  void* get_userdata() const noexcept { return m_pdata; }
  rm_widget* get_page() const noexcept { return m_pwidget; }
  bool is_closable() const noexcept { return m_closable; }
  bool is_pinned() const noexcept { return m_pinned; }
  void set_name(const char* p_name) { m_name = p_name ? p_name : ""; }
  void set_closable(bool closable) noexcept { m_closable = closable; }
  void set_pinned(bool pinned) noexcept { m_pinned = pinned; }
};

class rm_tabcontrol;
using rm_tabcontrol_cb = void(*)(rm_tabcontrol* ptabs, rm_tab_item* pitem, size_t tabid);
using rm_tabcontrol_close_cb = bool(*)(rm_tabcontrol* ptabs, rm_tab_item* pitem, size_t tabid);
class rm_tabcontrol : public rm_widget, public rm_callback<rm_tabcontrol_cb> {
  std::vector<rm_tab_item> m_tabs;
  std::vector<rm_rect> m_tab_bounds;
  RmTabBehaviour m_behaviour;
  RmThemeRef m_theme;
  RmTabVariant m_variant;
  RmTabPlacement m_placement;
  rm_tabcontrol_close_cb m_pclose_callback;
  size_t m_close_hovered;
  size_t m_close_pressed;

  const RmTabStyle& tab_style() const;
  bool is_horizontal() const noexcept;
  rm_rect get_bar_bounds() const;
  rm_rect get_page_bounds() const;
  void rebuild_tab_layout(NVGcontext* pctx);
  void update_pages_geometry();
  void update_children_active();
  size_t hit_test_tab(const rm_vec2& local_cursor) const;
  bool hit_test_close(size_t index, const rm_vec2& local_cursor) const;
  void notify_selection_changed();

  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override {
    m_behaviour.cancel();
    m_close_pressed = RmTabBehaviour::invalid_index;
  }
protected:
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_tabcontrol(rm_widget* p_parent, int x, int y, int width, int height,
    rm_tabcontrol_cb p_callback = nullptr, RmThemeRef theme = {},
    RmTabVariant variant = RmTabVariant::document,
    RmTabPlacement placement = RmTabPlacement::top);
  virtual ~rm_tabcontrol() {}

  size_t get_num_tabs() const noexcept { return m_tabs.size(); }
  rm_widget* add_tab(const char* p_name,
    uint32_t id = std::numeric_limits<uint32_t>::max(),
    bool closable = false, bool pinned = false, void* p_userdata = nullptr);
  rm_widget* add_tab_widget(const char* p_name, uint32_t id, rm_widget* p_page,
    bool closable = false, bool pinned = false, void* p_userdata = nullptr);
  bool remove_tab(size_t index);
  rm_widget* find_tab(const char* p_name);
  rm_widget* find_tab(uint32_t id);
  rm_tab_item* get_tab(size_t index) {
    return index < m_tabs.size() ? &m_tabs[index] : nullptr;
  }
  const rm_tab_item* get_tab(size_t index) const {
    return index < m_tabs.size() ? &m_tabs[index] : nullptr;
  }
  bool set_selected_index(size_t index);
  size_t get_selected_index() const noexcept { return m_behaviour.selected_index(); }
  rm_tab_item* get_selected_tab() { return get_tab(get_selected_index()); }
  const RmTabBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_close_callback(rm_tabcontrol_close_cb p_callback) noexcept {
    m_pclose_callback = p_callback;
  }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
    update_pages_geometry();
  }
  void set_variant(RmTabVariant variant) { m_variant = variant; update_pages_geometry(); }
  RmTabVariant get_variant() const noexcept { return m_variant; }
  void set_placement(RmTabPlacement placement) { m_placement = placement; update_pages_geometry(); }
  RmTabPlacement get_placement() const noexcept { return m_placement; }
  void resize(float width, float height) override;
};

/**
 * =============================================
 * Tree View Control
 * =============================================
 */
class rm_tree_node {
public:
  std::string            name;
  void* userdata;
  bool                   expanded;
  std::vector<rm_tree_node*> children;
  rm_tree_node* parent;

  rm_tree_node(const char* pname, void* puserdata = nullptr)
    : name(pname), userdata(puserdata), expanded(false), parent(nullptr) {
  }

  ~rm_tree_node() {
    for (auto child : children) delete child;
  }

  // add child node
  rm_tree_node* add_child(const char* pname, void* puserdata = nullptr) {
    rm_tree_node* node = new rm_tree_node(pname, puserdata);
    node->parent = this;
    children.push_back(node);
    return node;
  }
};

class rm_treeview;
using rm_treeview_cb = std::function<void(rm_treeview*, rm_tree_node*)>;

class rm_treeview : public rm_widget, public rm_callback<rm_treeview_cb> {
  std::vector<rm_tree_node*> m_roots;
  rm_tree_node* m_selected;
  float                      m_rowHeight;
  float                      m_indent;
protected:
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

  // recursive draw helper
  float draw_node(NVGcontext* pctx, rm_tree_node* node, float x, float y);
  // recursive hit-test helper
  bool hit_test(const rm_vec2& pos, rm_tree_node* node, float x, float& y, rm_tree_node*& out);

public:
  rm_treeview(int x, int y, int width, int height, rm_widget* p_parent, rm_treeview_cb cb = nullptr)
    : rm_widget(x, y, width, height, p_parent, "ui_treeview", RM_FLAG_DEFAULT | RM_FLAG_GLOBAL, 0, nullptr),
    m_selected(nullptr), m_rowHeight(20.0f), m_indent(16.0f)
  {
    set_callback(cb);
    //set_zindex(997);
  }

  virtual ~rm_treeview() {
    for (auto root : m_roots) delete root;
  }

  // add a root-level node
  rm_tree_node* add_root(const char* pname, void* puserdata = nullptr) {
    rm_tree_node* node = new rm_tree_node(pname, puserdata);
    m_roots.push_back(node);
    return node;
  }
  // clear all nodes
  void clear() {
    for (auto root : m_roots) delete root;
    m_roots.clear();
    m_selected = nullptr;
  }

  inline rm_tree_node* get_selected() const { return m_selected; }
  inline void set_row_height(float h) { m_rowHeight = h; }
  inline void set_indent(float i) { m_indent = i; }
};

class rm_output_text : public rm_widget
{
  rm_line_ring_buffer m_linesbuf;
  std::string         m_textbuf;
  float               m_line_height;
protected:
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_output_text(rm_widget* p_parent, int x, int y, int width, int height, float line_height=16.f, size_t num_lines=16);
  rm_output_text(rm_widget* p_parent, float x, float y, float width, float height, float line_height=16.f, size_t num_lines=16);
  virtual ~rm_output_text() {}

  inline void print(const std::string& text) { m_linesbuf.append_text(text); }
  inline void print(const char* ptext) { m_linesbuf.append_text(ptext); }
  void printf(const char* pformat, ...);
};

/**
* @brief Numbers input widget
*/
class rm_number_input : public rm_widget
{
public:
  enum input_type : uint32_t {
    type_int = 0,
    type_float
  };
protected:
  input_type       m_type;
  float            m_value;
  float            m_minval;
  float            m_maxval;
  float            m_step;
  rmgui_textbuffer m_buffer;

  void draw_buttons(NVGcontext* pctx);
  void on_draw(NVGcontext* pctx) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_number_input(rm_widget* p_parent, int x, int y, int width, int height, 
    input_type type = type_float, float value = 0.f, float step = 0.1f, float minval = 0.f, float maxval = 100.f);

  inline input_type           get_type() const { return m_type; }
  template<class _type> _type get_value() const { return static_cast<_type>(m_value); }
  template<class _type> _type get_min() const { return static_cast<_type>(m_minval); }
  template<class _type> _type get_max() const { return static_cast<_type>(m_maxval); }
  template<class _type> _type get_step() const { return static_cast<_type>(m_step); }
};


/**
* rm_menu
*/
class rm_menu;
using rm_menu_fn = void (*)(rm_menu *pmenu, uint32_t menuid, uint32_t id);

class rm_menu : public rm_widget, public rm_callback<rm_menu_fn>
{
  RmMenuBehaviour m_behaviour;
  RmThemeRef m_theme;
  std::vector<rm_rect> m_item_bounds;
  uint32_t m_itemid;
  uint32_t m_menuid;
  uint32_t m_level;
  std::string m_text;
  float m_text_width;
  rm_menu* m_proot_menu;
  bool m_separator;

  rm_menu(rm_menu* p_parent, const char* p_name, uint32_t menuid,
    uint32_t itemid, bool separator);
  const RmMenuStyle& menu_style() const { return m_theme->menu; }
  bool is_root_menu() const noexcept { return m_level == 0; }
  bool has_submenus() const noexcept { return rm_widget::get_num_childs() != 0; }
  void rebuild_item_layout();
  void update_popup_geometry();
  size_t hit_test_item(const rm_vec2& local_cursor) const;
  size_t find_selectable(size_t start, int direction) const;
  void open_submenu(size_t index);
  void close_submenus();
  void close_tree();
  bool contains_visible_popup(const rm_vec2& local_cursor) const;
  void activate_item(size_t index);
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
private:
  void     on_draw(NVGcontext* pctx) override;
  void     on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool     on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_menu(rm_widget* p_parent, rm_menu_fn p_callback = nullptr,
    RmThemeRef theme = {});
  /* delete methods */
  inline size_t get_num_childs() = delete;
  inline rm_widget* get_child(size_t idx) = delete;
  inline rm_widget** get_all_childs() = delete;
  bool add_child(rm_widget* p_child) = delete;
  bool remove_child(rm_widget* p_child) = delete;
  rm_widget* find_child_by_classname(const char* pclassname) const = delete;

  /* main methods */
  uint32_t get_menu_level() const noexcept { return m_level; }
  uint32_t get_menu_id() const noexcept { return m_menuid; }
  uint32_t get_item_id() const noexcept { return m_itemid; }
  bool is_separator() const noexcept { return m_separator; }
  rm_menu* create_submenu(const char *pname,
    uint32_t menuid,
    uint32_t itemid);
  size_t   get_num_submenus() const;
  rm_menu* get_submenu(size_t idx);

  inline bool add_item(const char* pitemname, uint32_t id) {
    return create_submenu(pitemname, get_menu_id(), id) != nullptr;
  }
  inline bool add_separator() {
    return create_submenu(nullptr, get_menu_id(), 0) != nullptr;
  }
  void set_theme(RmThemeRef theme);
  const RmMenuBehaviour& behaviour() const noexcept { return m_behaviour; }
  void close() { m_proot_menu->close_tree(); }
};

/**
 * RADIOBUTTON STYLE
 */
class rm_radiobutton_style : public rm_corners_style {
  rm_vec2   m_text_offset;
  NVGcolor  m_bg_inner;
  NVGcolor  m_text_color;
  NVGcolor  m_border_active_outer;
  NVGcolor  m_border_active_inner;
  float     m_border_width_outer;
  float     m_border_width_inner;
  NVGcolor  m_border_inactive;
  float     m_border_width_inactive;
  NVGcolor  m_mark_color;
  float     m_circle_radius;
  float     m_font_size;
  float     m_shadow_offset;
  float     m_shadow_size;
  NVGcolor  m_shadow_color;

public:
  rm_radiobutton_style() : m_text_offset(0.f, 0.f), 
    m_bg_inner(NVGcolor::RGBA(0, 0, 0, 60)), m_text_color(NVGcolor::RGB(255, 255, 255)),
    m_border_active_outer(NVGcolor::RGB(0, 122, 255)),
    m_border_active_inner(NVGcolor::RGB(102, 204, 255)), m_border_width_outer(2.f),
    m_border_width_inner(1.f),
    m_border_inactive(NVGcolor::RGBA(255, 255, 255, 192)),
    m_border_width_inactive(2.f),
    m_mark_color(NVGcolor::RGB(255, 255, 255)), m_circle_radius(8.0f), m_font_size(18.f),
    m_shadow_offset(5.f), m_shadow_size(6.f),
    m_shadow_color(NVGcolor::RGBAf(0.0f, 0.0f, 0.0f, 0.25f)) {}

  /* selectors */
  inline const rm_vec2& get_text_offset()       const { return m_text_offset; }
  inline NVGcolor       get_bg_inner()          const { return m_bg_inner; }
  inline NVGcolor       get_text_color()        const { return m_text_color; }
  inline NVGcolor       get_border_active_outer() const { return m_border_active_outer; }
  inline NVGcolor       get_border_active_inner() const { return m_border_active_inner; }
  inline float          get_border_width_outer()  const { return m_border_width_outer; }
  inline float          get_border_width_inner()  const { return m_border_width_inner; }
  inline NVGcolor       get_border_inactive()   const { return m_border_inactive; }
  inline float          get_border_width_inactive() const { return m_border_width_inactive; }
  inline NVGcolor       get_mark_color()        const { return m_mark_color; }
  inline float          get_circle_radius()     const { return m_circle_radius; }
  inline float          get_font_size()         const { return m_font_size; }
  inline float          get_shadow_offset() const { return m_shadow_offset; }
  inline float          get_shadow_size()   const { return m_shadow_size; }
  inline NVGcolor       get_shadow_color()  const { return m_shadow_color; }

  /* modificators */
  inline void set_text_offset(const rm_vec2& v) { m_text_offset = v; }
  inline void set_bg_inner(NVGcolor c) { m_bg_inner = c; }
  inline void set_text_color(NVGcolor c) { m_text_color = c; }
  inline void set_border_active_outer(NVGcolor c) { m_border_active_outer = c; }
  inline void set_border_active_inner(NVGcolor c) { m_border_active_inner = c; }
  inline void set_border_width_outer(float w) { m_border_width_outer = w; }
  inline void set_border_width_inner(float w) { m_border_width_inner = w; }
  inline void set_border_inactive(NVGcolor c) { m_border_inactive = c; }
  inline void set_border_width_inactive(float w) { m_border_width_inactive = w; }
  inline void set_mark_color(NVGcolor c) { m_mark_color = c; }
  inline void set_circle_radius(float r) { m_circle_radius = r; }
  inline void set_font_size(float s) { m_font_size = s; }
  inline void set_shadow_offset(float offset) { m_shadow_offset = offset; }
  inline void set_shadow_size(float size) { m_shadow_size = size; }
  inline void set_shadow_color(NVGcolor c) { m_shadow_color = c; }
};

class rm_radiobutton;
using rm_radiobutton_cb = bool(*)(rm_radiobutton*);
/**
 * RADIOBUTTON
 */
class rm_radiobutton : public rm_widget, public rm_styled<rm_radiobutton_style>, public rm_callback<rm_radiobutton_cb>
{
  std::string m_label;
  bool        m_checked;
  bool        m_allow_uncheck;
  static std::map<const rm_widget*, std::vector<rm_radiobutton*>> s_groups;

public:
  rm_radiobutton(rm_widget* parent, int x, int y, int width, int height,
    rm_radiobutton_style* style,
    const std::string& label,
    rm_radiobutton_cb cb = nullptr);

  virtual ~rm_radiobutton();

  inline bool is_checked() const { return m_checked; }
  inline void set_checked(bool v) { m_checked = v; }
  inline void set_allow_uncheck(bool v) { m_allow_uncheck = v; }

  static void select_default(rm_widget* parent, int index);
  static void select_by_label(rm_widget* parent, const std::string& label);

  inline const char* get_label() const { return m_label.c_str(); }
  inline void        set_label(const char* s) { m_label = s; }

  inline static std::map<const rm_widget*, std::vector<rm_radiobutton*>> get_groups() { return s_groups; }

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& pos, rm_vec2 delta) override;
};

class rm_switch;
using rm_switch_cb = void(*)(rm_switch*);
/**
 * SWITCH
*/
class rm_switch : public rm_widget, public rm_callback<rm_switch_cb>{
  RmSwitchBehaviour m_behaviour;
  RmThemeRef m_theme;

  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }

public:
  rm_switch(rm_widget* parent, int x, int y, int width,
    bool initial = false, rm_switch_cb cb = nullptr, RmThemeRef theme = {});

  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY key, RM_KEY_STATE state) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY key, RM_KEY_STATE state, rm_vec2& pos, rm_vec2 delta) override;
  
  inline void set_on(bool state, bool animation) {
    m_behaviour.set_on(state, animation);
  }
  
  inline bool is_on() const { return m_behaviour.is_on(); }
  const RmSwitchBehaviour& behaviour() const { return m_behaviour; }
  void set_theme(RmThemeRef theme) { m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme(); }
};

/**
 * LISTVIEW STYLE
*/
class rm_listview_style : public rm_corners_style {
  float     m_padding;
  float     m_row_height;
  NVGcolor  m_bg_color;
  NVGcolor  m_text_color;
  NVGcolor  m_hover_color;
  NVGcolor  m_selected_color;
  float     m_font_size;
public:
  rm_listview_style(): m_padding(8.f), m_row_height(24.f), 
    m_bg_color(NVGcolor::RGB(255, 255, 255)), 
    m_text_color(NVGcolor::RGB(0, 0, 0)), m_hover_color(NVGcolor::RGB(240, 240, 240)), 
    m_selected_color(NVGcolor::RGB(200, 200, 255)), m_font_size(16.f){
  }

  /* selectors */
  inline float            get_text_padding()        const { return m_padding; }
  inline float            get_row_height()          const { return m_row_height; }
  inline const NVGcolor&  get_background_color()    const { return m_bg_color; }
  inline const NVGcolor&  get_text_color()          const { return m_text_color; }
  inline const NVGcolor&  get_hover_color()         const { return m_hover_color; }
  inline const NVGcolor&  get_selected_color()      const { return m_selected_color; }
  inline float            get_font_size()           const { return m_font_size; }

  /* modifiers */
  inline void set_text_padding(float p) { m_padding = p; }
  inline void set_row_height(float h) { m_row_height = h; }
  inline void set_background_color(NVGcolor c) { m_bg_color = c; }
  inline void set_text_color(NVGcolor c) { m_text_color = c; }
  inline void set_hover_color(NVGcolor c) { m_hover_color = c; }
  inline void set_selected_color(NVGcolor c) { m_selected_color = c; }
  inline void set_font_size(float f) { m_font_size = f; }
};

/**
 * LISTVIEW
*/
class rm_listview;
using rm_listview_cb = void(*)(rm_listview* lv, size_t index);
class rm_listview : public rm_widget, public rm_styled<rm_listview_style>, public rm_callback<rm_listview_cb>
{
  std::vector<std::string> m_items;
  size_t m_hover_index;
  size_t m_selected_index;

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_listview(rm_widget* parent, int x, int y, int width, int height, rm_listview_style* pstyle, rm_listview_cb cb = nullptr);
  ~rm_listview() {};

  void add_item(const std::string& text);
  void clear_items();

  float get_item_height() const { return m_pstyle->get_row_height(); }
  size_t get_selected_index() const { return m_selected_index; }
  std::vector<std::string> get_items() const { return m_items; }
};
