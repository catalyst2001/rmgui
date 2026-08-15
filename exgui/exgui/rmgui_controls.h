#pragma once
#include "rmgui.h"
#include "rmgui_behaviour.h"
#include "rmgui_default_painter.h"
#include "rmgui_theme.h"
#include "smalldelegate.h"
#include <string>
#include <vector>
#include <type_traits>
#include <map>
#include <memory>
#include <string_view>
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

class rm_text_input : public rm_widget {
  RmTextInputBehaviour m_behaviour;
  RmThemeRef           m_theme;
  RmTextInputLayout    m_layout;
  bool                 m_ctrl_pressed;
  rmgui_timer          m_timer;
  bool                 m_blink_state;
  uint32_t             m_flags;
  float                m_scroll_offset;
  double               m_last_click_time;
  rm_vec2              m_last_click_pos;

  static constexpr double DOUBLE_CLICK_THRESHOLD = 0.35;
  static constexpr float  CLICK_MOVE_THRESHOLD = 4.f;

  void reset_caret(bool visible = true);
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override {
    m_behaviour.set_active(focused);
    if (!focused)
      m_ctrl_pressed = false;
  }
  void on_pointer_capture_lost() override { m_behaviour.cancel_pointer(); }
public:
  rm_text_input(rm_widget* p_parent, int x, int y, int width, int height,
    uint32_t flags = RMGUI_TEXT_INPUT_SINGLELINE, RmThemeRef theme = {},
    float blink_cursor_interval = 0.5f);
  virtual ~rm_text_input();
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual void on_text_input(int sym) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

  // map local xy-coordinate to character index
  size_t hit_test_index(float px, float py = NAN) const;

  void ensure_visible(size_t idx);
  const RmTextInputBehaviour& behaviour() const noexcept { return m_behaviour; }
  const std::string& get_text() const noexcept { return m_behaviour.text(); }
  void set_text(std::string text) { m_behaviour.set_text(std::move(text)); }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};

/**
* =============================================
* CheckBox
* 
* 
* =============================================
*/
class rm_checkbox;
using rm_checkbox_cb = Delegate<bool, rm_checkbox*>;
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
  std::string m_name;
  void* m_pdata;
public:
  rm_combo_item() : m_pdata(nullptr) {}
  rm_combo_item(const char* pname, void* p_userdata = nullptr) :
    m_name(pname ? pname : ""), m_pdata(p_userdata) {}

  inline const char* get_name() const { return m_name.c_str(); }
  inline void* get_userdata() const { return m_pdata; }
};

class rm_combobox;
using rm_combobox_cb = Delegate<void, rm_combobox*, rm_combo_item*, size_t>;
class rm_combobox : public rm_widget, public rm_callback<rm_combobox_cb> {
  static constexpr int base_zindex = 900;
  static constexpr int popup_zindex = 950;
  std::vector<rm_combo_item> m_items;
  RmComboBoxBehaviour m_behaviour;
  RmThemeRef m_theme;
  std::string m_placeholder;

  const RmComboBoxStyle& combo_style() const { return m_theme->combobox; }
  float popup_y() const;
  float popup_height() const;
  size_t hit_test_popup_item(const rm_vec2& local_cursor) const;
  void notify_selection();
  void sync_popup_layer();
  void on_enabled_changed(bool enabled) override;
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
    rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  static constexpr size_t kinvalid_index = RmComboBoxBehaviour::invalid_index;
  rm_combobox(rm_widget* p_parent, int x, int y, int width, int height,
    rm_combobox_cb pcallback = nullptr, RmThemeRef theme = {});
  virtual ~rm_combobox();

  inline size_t get_num_items() const { return m_items.size(); }
  size_t add_item(const char *pitem, void *puserdata = nullptr);
  bool remove_item(size_t index);
  void clear_items();
  size_t find_item(const char* pitem);
  inline rm_combo_item* get_item(size_t idx) {
    assert(idx < m_items.size() && "item index out of bounds");
    return &m_items[idx];
  }
  inline const rm_combo_item* get_item(size_t idx) const {
    assert(idx < m_items.size() && "item index out of bounds");
    return &m_items[idx];
  }
  size_t get_selected_index() const noexcept { return m_behaviour.selected_index(); }
  rm_combo_item* get_selected_item() {
    return get_selected_index() < m_items.size()
      ? &m_items[get_selected_index()] : nullptr;
  }
  bool set_selected_index(size_t index, bool notify = false);
  bool is_expanded() const noexcept { return m_behaviour.is_expanded(); }
  const RmComboBoxBehaviour& behaviour() const noexcept { return m_behaviour; }
  const char* get_placeholder() const noexcept { return m_placeholder.c_str(); }
  void set_placeholder(const char* p_text) { m_placeholder = p_text ? p_text : ""; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
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
using rm_slider_callback = Delegate<void, rm_slider*>;
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


class rm_scrollbar;
using rm_scrollbar_cb = Delegate<void, rm_scrollbar*, float>;
class rm_scrollbar : public rm_widget, public rm_callback<rm_scrollbar_cb>
{
  RM_ORIENT m_orientation;
  RmScrollbarBehaviour m_behaviour;
  RmThemeRef m_theme;

  bool is_vertical() const noexcept { return m_orientation == RM_ORIENT_VERT; }
  float track_length() const noexcept { return is_vertical() ? m_size.y : m_size.x; }
  float pointer_axis(const rm_vec2& local_cursor) const noexcept {
    return is_vertical() ? local_cursor.y : local_cursor.x;
  }
  void adjust_geometry();
  void notify_position();
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
    rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_scrollbar(rm_widget* p_parent, RM_ORIENT orientation,
    float initial_position = 0.f, rm_scrollbar_cb p_callback = nullptr,
    RmThemeRef theme = {});
  ~rm_scrollbar();

  void set_position(float position, bool notify = false);
  inline float get_position() const { return m_behaviour.position(); }
  void set_viewport_fraction(float fraction) { m_behaviour.set_viewport_fraction(fraction); }
  void set_content_metrics(float content_extent, float viewport_extent);
  float get_viewport_fraction() const { return m_behaviour.viewport_fraction(); }
  RM_ORIENT get_orientation() const noexcept { return m_orientation; }
  const RmScrollbarBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme);
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
using rm_tabcontrol_cb = Delegate<void, rm_tabcontrol*, rm_tab_item*, size_t>;
using rm_tabcontrol_close_cb = Delegate<bool, rm_tabcontrol*, rm_tab_item*, size_t>;
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
  std::string            tooltip;
  void* userdata;
  bool                   expanded;
  rm_image               collapsed_icon;
  rm_image               expanded_icon;
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

  rm_tree_node& set_tooltip(std::string text) {
    tooltip = std::move(text);
    return *this;
  }

  rm_tree_node& set_icons(rm_image collapsed, rm_image expanded) {
    collapsed_icon = collapsed;
    expanded_icon = expanded;
    return *this;
  }

  rm_image current_icon() const noexcept {
    if (expanded)
      return expanded_icon.isValid() ? expanded_icon : collapsed_icon;
    return collapsed_icon.isValid() ? collapsed_icon : expanded_icon;
  }
};

class rm_treeview;
using rm_treeview_cb = Delegate<void, rm_treeview*, rm_tree_node*>;

class rm_treeview : public rm_widget, public rm_callback<rm_treeview_cb> {
  struct VisibleRow {
    rm_tree_node* node = nullptr;
    size_t depth = 0;
  };

  std::vector<rm_tree_node*> m_roots;
  std::vector<VisibleRow>    m_visible_rows;
  rm_tree_node*              m_selected;
  RmTreeViewBehaviour        m_behaviour;
  RmThemeRef                 m_theme;
  size_t                     m_expander_pressed;

  void append_visible(rm_tree_node* p_node, size_t depth);
  void rebuild_visible_rows();
  size_t hit_test_row(const rm_vec2& cursor_pos) const;
  bool hit_test_expander(const rm_vec2& cursor_pos, size_t index) const;
  void select_index(size_t index, bool notify);
  void toggle_index(size_t index);
  bool contains_node(const rm_tree_node* p_node) const;
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override {
    m_behaviour.cancel();
    m_expander_pressed = RmTreeViewBehaviour::invalid_index;
  }
protected:
  virtual void on_draw(NVGcontext* pctx) override;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;

public:
  rm_treeview(int x, int y, int width, int height, rm_widget* p_parent,
    rm_treeview_cb cb = nullptr, RmThemeRef theme = {});

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
    m_visible_rows.clear();
    m_selected = nullptr;
    m_behaviour.set_count(0);
  }

  inline rm_tree_node* get_selected() const { return m_selected; }
  bool set_expanded(rm_tree_node* p_node, bool expanded);
  bool expand(rm_tree_node* p_node) { return set_expanded(p_node, true); }
  bool collapse(rm_tree_node* p_node) { return set_expanded(p_node, false); }
  bool toggle(rm_tree_node* p_node) {
    return p_node ? set_expanded(p_node, !p_node->expanded) : false;
  }
  const RmTreeViewBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};

enum class RmPropertyType {
  text,
  integer,
  real,
  boolean,
  choice
};

class rm_property_group;

class rm_property {
  friend class rm_propertyview;

  uint32_t m_id = 0;
  std::string m_name;
  std::string m_value;
  std::string m_error;
  RmPropertyType m_type = RmPropertyType::text;
  std::vector<std::string> m_choices;
  rm_property_group* m_pgroup = nullptr;
  void* m_puserdata = nullptr;

public:
  rm_property(uint32_t id, std::string name, std::string value,
    RmPropertyType type, rm_property_group* p_group, void* p_userdata)
    : m_id(id), m_name(std::move(name)), m_value(std::move(value)),
    m_type(type), m_pgroup(p_group), m_puserdata(p_userdata) {}

  uint32_t get_id() const noexcept { return m_id; }
  const std::string& get_name() const noexcept { return m_name; }
  const std::string& get_value() const noexcept { return m_value; }
  const std::string& get_error() const noexcept { return m_error; }
  RmPropertyType get_type() const noexcept { return m_type; }
  const std::vector<std::string>& get_choices() const noexcept { return m_choices; }
  rm_property_group* get_group() const noexcept { return m_pgroup; }
  void* get_userdata() const noexcept { return m_puserdata; }
  bool is_valid() const noexcept { return m_error.empty(); }

  rm_property& set_choices(std::vector<std::string> choices) {
    m_choices = std::move(choices);
    m_type = RmPropertyType::choice;
    return *this;
  }
};

class rm_property_group {
  friend class rm_propertyview;

  uint32_t m_id = 0;
  std::string m_name;
  bool m_expanded = true;
  std::vector<std::unique_ptr<rm_property>> m_properties;

public:
  rm_property_group(uint32_t id, std::string name)
    : m_id(id), m_name(std::move(name)) {}

  uint32_t get_id() const noexcept { return m_id; }
  const std::string& get_name() const noexcept { return m_name; }
  bool is_expanded() const noexcept { return m_expanded; }
  size_t get_num_properties() const noexcept { return m_properties.size(); }
  rm_property* get_property(size_t index) const noexcept {
    return index < m_properties.size() ? m_properties[index].get() : nullptr;
  }
};

class rm_propertyview;
using rm_property_changed_cb = Delegate<void, rm_propertyview*, rm_property*>;
using rm_property_validation_cb = Delegate<std::string, rm_propertyview*,
  const rm_property*, const char*>;

class rm_propertyview : public rm_widget,
  public rm_callback<rm_property_changed_cb> {
  struct VisibleRow {
    rm_property_group* group = nullptr;
    rm_property* property = nullptr;
    float y = 0.0f;
    float height = 0.0f;
  };

  std::vector<std::unique_ptr<rm_property_group>> m_groups;
  std::vector<std::unique_ptr<rm_property>> m_properties;
  std::vector<VisibleRow> m_visible_rows;
  RmPropertyViewBehaviour m_behaviour;
  RmThemeRef m_theme;
  rm_property_validation_cb m_validation_callback;
  rm_property* m_pediting = nullptr;
  std::string m_edit_buffer;
  bool m_choice_open = false;
  size_t m_choice_hovered = RmPropertyViewBehaviour::invalid_index;
  uint32_t m_next_item_id = 0;

  void rebuild_visible_rows();
  size_t hit_test_row(const rm_vec2& cursor_pos) const;
  size_t hit_test_choice(const rm_vec2& cursor_pos) const;
  float value_column_x() const;
  void begin_edit(rm_property* p_property);
  bool commit_edit();
  void cancel_edit();
  void choose_value(size_t index);
  std::string validate_builtin(const rm_property& property,
    std::string_view value) const;
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override {
    if (!focused) {
      m_behaviour.cancel();
      if (m_pediting)
        commit_edit();
    }
  }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }

protected:
  virtual std::string on_validate_property(const rm_property& property,
    const std::string& value) const;
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  void on_text_input(int sym) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
    rm_vec2& cursor_pos, rm_vec2 delta) override;

public:
  rm_propertyview(rm_widget* p_parent, int x, int y, int width, int height,
    rm_property_changed_cb changed = nullptr, RmThemeRef theme = {});

  rm_property_group* add_group(const char* p_name,
    uint32_t id = std::numeric_limits<uint32_t>::max());
  rm_property* add_property(const char* p_name, const char* p_value,
    RmPropertyType type = RmPropertyType::text,
    rm_property_group* p_group = nullptr,
    uint32_t id = std::numeric_limits<uint32_t>::max(),
    void* p_userdata = nullptr);
  rm_property* add_choice_property(const char* p_name, const char* p_value,
    std::vector<std::string> choices,
    rm_property_group* p_group = nullptr,
    uint32_t id = std::numeric_limits<uint32_t>::max(),
    void* p_userdata = nullptr);
  bool set_group_expanded(rm_property_group* p_group, bool expanded);
  bool set_property_value(rm_property* p_property, std::string value,
    bool notify = false);
  rm_property* get_selected_property() const noexcept;
  const RmPropertyViewBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_validation_callback(rm_property_validation_cb callback) {
    m_validation_callback = callback;
  }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};

class rm_output_text : public rm_widget
{
  RmOutputTextBehaviour m_behaviour;
  RmThemeRef            m_theme;
protected:
  virtual void on_draw(NVGcontext* pctx) override;
public:
  rm_output_text(rm_widget* p_parent, float x, float y, float width, float height,
    size_t num_lines = 16, RmThemeRef theme = {});
  virtual ~rm_output_text() {}

  inline void print(const std::string& text) { m_behaviour.append_text(text); }
  inline void print(const char* ptext) { if (ptext) m_behaviour.append_text(ptext); }
  void printf(const char* pformat, ...);
  void clear() { m_behaviour.clear(); }
  const RmOutputTextBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};

/**
* @brief Numbers input widget
*/
class rm_number_input : public rm_widget
{
protected:
  RmNumberInputBehaviour m_behaviour;
  RmThemeRef              m_theme;

  RmNumberInputPart hit_test_part(const rm_vec2& cursor_pos) const;
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_number_input(rm_widget* p_parent, int x, int y, int width, int height, 
    RmNumberInputType type = RmNumberInputType::floating_point,
    float value = 0.f, float step = 0.1f, float minval = 0.f,
    float maxval = 100.f, RmThemeRef theme = {});

  RmNumberInputType get_type() const noexcept { return m_behaviour.type(); }
  template<class _type> _type get_value() const { return static_cast<_type>(m_behaviour.value()); }
  template<class _type> _type get_min() const { return static_cast<_type>(m_behaviour.minimum()); }
  template<class _type> _type get_max() const { return static_cast<_type>(m_behaviour.maximum()); }
  template<class _type> _type get_step() const { return static_cast<_type>(m_behaviour.step()); }
  void set_value(float value) { m_behaviour.set_value(value); }
  void set_range(float minimum, float maximum) { m_behaviour.set_range(minimum, maximum); }
  void set_step(float step) { m_behaviour.set_step(step); }
  const RmNumberInputBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};


/**
* rm_menu
*/
class rm_menu;
using rm_menu_fn = Delegate<void, rm_menu*, uint32_t, uint32_t>;

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
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel_press(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel_press(); }
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

class rm_radiobutton;
using rm_radiobutton_cb = Delegate<bool, rm_radiobutton*>;
/**
 * RADIOBUTTON
 */
class rm_radiobutton : public rm_widget, public rm_callback<rm_radiobutton_cb>
{
  std::string m_label;
  RmRadioButtonBehaviour m_behaviour;
  RmThemeRef m_theme;
  static std::map<const rm_widget*, std::vector<rm_radiobutton*>> s_groups;

  void uncheck_siblings();
  void notify_activation();
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
    rm_vec2& pos, rm_vec2 delta) override;

public:
  rm_radiobutton(rm_widget* parent, int x, int y, int width, int height,
    const std::string& label, rm_radiobutton_cb cb = nullptr,
    RmThemeRef theme = {});

  virtual ~rm_radiobutton();

  inline bool is_checked() const { return m_behaviour.is_checked(); }
  void set_checked(bool checked);
  inline void set_allow_uncheck(bool allow) { m_behaviour.set_allow_uncheck(allow); }

  static void select_default(rm_widget* parent, int index);
  static void select_by_label(rm_widget* parent, const std::string& label);

  inline const char* get_label() const { return m_label.c_str(); }
  inline void set_label(const char* p_text) { m_label = p_text ? p_text : ""; }
  const RmRadioButtonBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};

class rm_switch;
using rm_switch_cb = Delegate<void, rm_switch*>;
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
 * LISTVIEW
*/
class rm_listview;
using rm_listview_cb = Delegate<void, rm_listview*, size_t>;
class rm_listview : public rm_widget, public rm_callback<rm_listview_cb>
{
  std::vector<std::string> m_items;
  RmListViewBehaviour m_behaviour;
  RmThemeRef m_theme;

  size_t hit_test_row(const rm_vec2& local_cursor) const;
  void notify_selection();
  void on_enabled_changed(bool enabled) override { m_behaviour.set_enabled(enabled); }
  void on_focus_changed(bool focused) override { if (!focused) m_behaviour.cancel(); }
  void on_pointer_capture_lost() override { m_behaviour.cancel(); }
  void on_draw(NVGcontext* pctx) override;
  void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) override;
  bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state,
    rm_vec2& cursor_pos, rm_vec2 delta) override;
public:
  rm_listview(rm_widget* parent, int x, int y, int width, int height,
    rm_listview_cb cb = nullptr, RmThemeRef theme = {});
  ~rm_listview() {};

  void add_item(const std::string& text);
  bool remove_item(size_t index);
  void clear_items();

  float get_item_height() const { return m_theme->listview.row_height; }
  size_t get_selected_index() const { return m_behaviour.selected_index(); }
  bool set_selected_index(size_t index, bool notify = false);
  const std::vector<std::string>& get_items() const { return m_items; }
  const RmListViewBehaviour& behaviour() const noexcept { return m_behaviour; }
  void set_theme(RmThemeRef theme) {
    m_theme = theme ? std::move(theme) : RmThemeSnapshot::default_theme();
  }
};
