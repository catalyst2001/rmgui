#pragma once
#include "rmgui.h"
#include <string>
#include <vector>
#include <functional>
#include <type_traits>

class rmgui_image {
public:
  int imageId;
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
};

class rm_button : public rm_widget {
  std::string m_text;
public:
  rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text);
  virtual ~rm_button();
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
};

class rm_label : public rm_widget {
  std::string m_text;
public:
  rm_label(rm_widget* p_parent, int x, int y, const std::string& text);
  virtual ~rm_label();
  virtual void on_draw(NVGcontext* pctx) override;
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
    m_text_color(nvgRGB(0, 0, 0)),
    m_active_bg_color(nvgRGB(255, 255, 255)),
    m_unactive_bg_color(nvgRGBA(220, 220, 220, 255)),
    m_border_color(nvgRGBA(255, 255, 255, 255)),
    m_blink_color(nvgRGB(0, 0, 0)),
    m_selection_color(nvgRGBA(51, 153, 255, 128)),
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
  virtual void on_keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state) override;
  virtual void on_text_input(int sym) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  // map local xy-coordinate to character index
  size_t hit_test_index(float px, float py = NAN) const;

  void ensure_visible(size_t idx);
};

/**
* CHECKBOX
*/
class rm_checkbox_style : public rm_corners_style {
  rm_vec2 m_text_offset;
  NVGcolor      m_text_color;
  NVGcolor      m_bg_color;
  NVGcolor      m_mark_color;
  NVGcolor      m_border_color;
  int           m_check_size;
  float         m_font_size;
  float         m_border_width;
public:
  rm_checkbox_style() :
    m_text_offset(5.f, 0.f),
    m_text_color(nvgRGB(255, 255, 255)),
    m_bg_color(nvgRGB(255, 255, 255)),
    m_mark_color(nvgRGB(0, 0, 0)),
    m_border_color(nvgRGB(0, 0, 0)),
    m_check_size(20),
    m_font_size(18.f), m_border_width(1.f){}

  /* selectors  */
  inline const rm_vec2& get_text_offsets() const { return m_text_offset; }
  inline const NVGcolor& get_text_color() const { return m_text_color; }
  inline const NVGcolor& get_background_color() const { return m_bg_color; }
  inline const NVGcolor& get_mark_color() const { return m_mark_color; }
  inline const NVGcolor& get_border_color() const { return m_border_color; }
  inline int             get_check_size() const { return m_check_size; }
  inline float           get_font_size() const { return m_font_size; }
  inline float           get_border_width() const { return m_border_width; }

  /* modifiers */
  inline void set_text_offsets(rm_vec2 offset) { m_text_offset = offset; }
  inline void set_text_color(NVGcolor clr) { m_text_color = clr; }
  inline void set_background_color(NVGcolor clr) { m_bg_color = clr; }
  inline void set_mark_color(NVGcolor clr) { m_mark_color = clr; }
  inline void set_border_color(NVGcolor clr) { m_border_color = clr; }
  inline void set_check_size(int newsize) { m_check_size = newsize; }
  inline void set_font_size(float fsize) { m_font_size = fsize; }
  inline void set_border_width(float bsize) { m_border_width = bsize; }
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
class rm_checkbox : public rm_widget, public rm_styled<rm_checkbox_style>, public rm_callback<rm_checkbox_cb> {
  bool           m_checked;
  std::string    m_label;
  rm_font        m_icon_font;

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
public:
  rm_checkbox(rm_widget* p_parent, int x, int y, int width, rm_checkbox_style *pstyle, const std::string& label, rm_checkbox_cb pcallback=nullptr);
  virtual ~rm_checkbox();
  inline bool        is_checked() const { return m_checked; }
  inline const char* get_label() const { return m_label.c_str(); }
  inline void        set_label(const char* plabeltext) { m_label.assign(plabeltext); }
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
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
  float   m_min;
  float   m_max;
  float   m_value;
  bool    m_dragging;
  float   m_last_value;
  rm_slider_callback m_pcallback;
  rm_rect m_inner_rect;
  float   m_thumb_size;
  void    compute_value(rm_vec2& cursor_pos);
  void    compute_inner_and_thumb();

  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos);

public:
  rm_slider(rm_widget* p_parent, int x, int y, int width, int height, float min, float max, float initial, rm_slider_callback pcallback=nullptr);
  virtual ~rm_slider();
  float get_value() const { return m_value; }
};

/**
* =============================================
* Progress Base class
*
*
* =============================================
*/
class rm_progress_base : public rm_widget
{
protected:
  float m_round;
  float m_percent;
public:
  rm_progress_base(rm_widget* p_parent, int x, int y, int width, int height, float inital=0.1f, float corner_round=0.5f);
  virtual ~rm_progress_base();
  void         set_percent(float p) { m_percent = rm_clamp(p, 0.f, 100.f); }
  inline float get_percent() const { return m_percent; }
  void         set_corner_round(float p) { m_round = p; }
  inline float get_corner_round() const { return m_round; }
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos);
};

/**
* =============================================
* Progress Image
*
*
* =============================================
*/
class rm_progress_image : public rm_progress_base
{
  rm_image m_image;
  float    m_angle;
  float    m_alpha;
public:
  rm_progress_image(rm_widget* p_parent, int x, int y, int width, int height, 
    rm_image img, float patangle=0.f, float patalpha=1.f, float inital = 0.1f, float corner_round = 0.5f);
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
    m_background_clr = nvgRGB(30, 30, 30);
    m_thumb_clr = nvgRGB(80, 80, 80);
    m_background_border_clr = nvgRGB(100, 100, 100);
    m_thumb_border_clr = nvgRGB(100, 100, 100);
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos);
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
 * Tab Control Style
 * =============================================
 */
class rm_tabcontrol_style : public rm_corners_style {
  rm_vec2 m_text_offset;
  NVGcolor      m_text_color;
  NVGcolor      m_bg_color;
  NVGcolor      m_border_color;
  NVGcolor      m_selected_color;
  NVGcolor      m_unselected_color;
  float         m_font_size;
  bool          m_is_horizontal;
  float         m_tab_height;
public:
  rm_tabcontrol_style() :
    m_text_offset(0.f, 0.f),
    m_text_color(nvgRGB(0, 0, 0)),
    m_bg_color(nvgRGB(255, 255, 255)),
    m_border_color(nvgRGB(0, 0, 0)),
    m_selected_color(nvgRGB(240, 240, 240)),
    m_unselected_color(nvgRGB(200, 200, 200)),
    m_font_size(15.f),
    m_is_horizontal(true), m_tab_height(30.f){
  }
  inline bool            is_horizontal() const { return m_is_horizontal; }
  inline void            set_horizontal(bool enabled) { m_is_horizontal = enabled; }

  /* selectors  */
  inline const rm_vec2& get_text_offsets() const { return m_text_offset; }
  inline const float    get_tab_height() const { return m_tab_height; }
  inline const NVGcolor& get_text_color() const { return m_text_color; }
  inline const NVGcolor& get_background_color() const { return m_bg_color; }
  inline const NVGcolor& get_border_color() const { return m_border_color; }
  inline const NVGcolor& get_selected_color() const { return m_selected_color; }
  inline const NVGcolor& get_unselected_color() const { return m_unselected_color; }
  inline float           get_font_size() const { return m_font_size; }

  /* modifiers */
  inline void set_text_offsets(rm_vec2 offset) { m_text_offset = offset; }
  inline void set_tab_height(float t) { m_tab_height = t; }
  inline void set_text_color(NVGcolor clr) { m_text_color = clr; }
  inline void set_background_color(NVGcolor clr) { m_bg_color = clr; }
  inline void set_border_color(NVGcolor clr) { m_border_color = clr; }
  inline void set_selected_color(NVGcolor clr) { m_selected_color = clr; }
  inline void set_unselected_color(NVGcolor clr) { m_unselected_color = clr; }
  inline void set_font_size(float fsize) { m_font_size = fsize; }
};

/**
 * =============================================
 * Tab Control
 * =============================================
 */
class rm_tab_item {
  std::string name;
  void       *pdata;
  rm_widget  *pwidget;
public:
  rm_tab_item() : pdata(nullptr), pwidget(nullptr) {}
  rm_tab_item(const char* pname, rm_widget* pw, void* userptr = nullptr)
    : name(pname), pdata(userptr), pwidget(pw) {
  }

  inline const char* get_name() const { return name.c_str(); }
  inline void* get_userdata() const { return pdata; }
  inline rm_widget* get_page() const { return pwidget; }
};

class rm_tabcontrol;
using rm_tabcontrol_cb = void(*)(rm_tabcontrol* ptabs, rm_tab_item* pitem, size_t tabid);
class rm_tabcontrol : public rm_widget, public rm_styled<rm_tabcontrol_style>, public rm_callback<rm_tabcontrol_cb> {
  std::vector<rm_tab_item>             m_tabs;
  int                                  m_selected;
protected:
  virtual void on_draw(NVGcontext* pctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  void update_children_active();

  void get_widget_size(rm_vec2 &dst_pos, rm_vec2 &dst_size);
  void get_one_tab_size(rm_vec2 &dst_size);
public:

  rm_tabcontrol(rm_widget* p_parent, int x, int y, int width, int height, rm_tabcontrol_cb cb = nullptr);
  virtual ~rm_tabcontrol() {}

  inline size_t get_num_tabs() const { return m_tabs.size(); }

  rm_widget* add_tab(const char* pname, void* puserdata = nullptr);
  rm_widget* find_tab(const char* pname);
  inline rm_tab_item* get_tab(size_t idx) {
    assert(idx < m_tabs.size());
    return &m_tabs[idx];
  }

  void set_selected_index(int idx);
  inline int get_selected_index() const { return m_selected; }
  inline rm_tab_item* get_selected_tab() {
    if (m_selected < 0 || m_selected >= static_cast<int>(m_tabs.size()))
      return nullptr;
    return &m_tabs[m_selected];
  }

  void get_tabcontrol_size(rm_vec2& dst);
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  // recursive draw helper
  float draw_node(NVGcontext* pctx, rm_tree_node* node, float x, float y);
  // recursive hit-test helper
  bool hit_test(const rm_vec2& pos, rm_tree_node* node, float x, float& y, rm_tree_node*& out);

public:
  rm_treeview(int x, int y, int width, int height, rm_widget* p_parent, rm_treeview_cb cb = nullptr)
    : rm_widget(x, y, width, height, p_parent, "ui_treeview", EXGUI_FLAG_DEFAULT | EXGUI_FLAG_GLOBAL, 0, nullptr),
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
public:
  rm_output_text(rm_widget* p_parent, int x, int y, int width, int height, float line_height=16.f, size_t num_lines=16);
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
  bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
public:
  rm_number_input(rm_widget* p_parent, int x, int y, int width, int height, 
    input_type type = type_float, float value = 0.f, float step = 0.1f, float minval = 0.f, float maxval = 100.f);
  rm_number_input(rm_widget* p_parent, int x, int y, int width, int height,
    input_type type, int value = 0, int step = 1, int minval = 0, int maxval = 100);

  inline input_type           get_type() const { return m_type; }
  template<class _type> _type get_value() const { return static_cast<_type>(m_value); }
  template<class _type> _type get_min() const { return static_cast<_type>(m_minval); }
  template<class _type> _type get_max() const { return static_cast<_type>(m_maxval); }
  template<class _type> _type get_step() const { return static_cast<_type>(m_step); }
};

/**
* tabcontrol extended
*/
enum rm_tabcontrol_flags {
  TCF_NONE = 0,
  TCF_AUTOSIZE=1<<0,
  TCF_SCROLL_OVERFLOW=1<<1,
  TCF_ROWS_OVERFLOW=1<<2
};

class rm_tabcontrol_ex_style : public rm_corners_style {
  rm_vec2  m_text_offset;
  rm_color m_text_color;
  union {
    struct {
      rm_color m_bg_color;
      rm_color m_selected_color;
    };
    rm_color m_hover_colors[2];
  };
  rm_color m_border_color;
  rm_vec2  m_tab_up_offsets;
  float    m_font_size;
  bool     m_is_horizontal;
  float    m_tab_height;
  float    m_tab_corners_radius;
  float    m_tab_buttons_size;
  float    m_tab_buttons_spacing;
public:
  rm_tabcontrol_ex_style() :
    m_text_offset(0.f, 0.f),
    m_text_color(0, 0, 0),
    m_bg_color(255, 255, 255),
    m_border_color(0, 0, 0),
    m_selected_color(240, 240, 240),
    m_font_size(15.f),
    m_is_horizontal(true),
    m_tab_height(30.f),
    m_tab_corners_radius(4.f),
    m_tab_buttons_size(10.f),
    m_tab_buttons_spacing(5.f)
  {
  }
  inline bool            is_horizontal() const { return m_is_horizontal; }
  inline void            set_horizontal(bool enabled) { m_is_horizontal = enabled; }

  /* selectors  */
  inline const rm_vec2&  get_text_offsets() const { return m_text_offset; }
  inline const rm_vec2&  get_tab_up_offsets() const { return m_tab_up_offsets; }
  inline const float     get_tab_height() const { return m_tab_height; }
  inline const rm_color& get_text_color() const { return m_text_color; }
  inline const rm_color& get_background_color() const { return m_bg_color; }
  inline const rm_color& get_border_color() const { return m_border_color; }
  inline const rm_color& get_selected_color() const { return m_selected_color; }
  inline float           get_font_size() const { return m_font_size; }
  inline float           get_tab_corners_radius() const { return m_tab_corners_radius; }
  inline rm_color& get_state_color(bool bhovered) { return m_hover_colors[(int)bhovered]; }
  inline float           get_tab_buttons_size() const { return m_tab_buttons_size; }
  inline float           get_tab_buttons_spacing() const { return m_tab_buttons_spacing; }

  /* modifiers */
  inline void set_text_offsets(rm_vec2 offset) { m_text_offset = offset; }
  inline void set_tab_up_offsets(rm_vec2 offset) { m_tab_up_offsets = offset; }
  inline void set_tab_height(float t) { m_tab_height = t; }
  inline void set_text_color(rm_color clr) { m_text_color = clr; }
  inline void set_background_color(rm_color clr) { m_bg_color = clr; }
  inline void set_border_color(rm_color clr) { m_border_color = clr; }
  inline void set_selected_color(rm_color clr) { m_selected_color = clr; }
  inline void set_font_size(float fsize) { m_font_size = fsize; }
  inline void set_tab_corners_radius(float radius) { m_tab_corners_radius = radius; }
  inline void set_tab_buttons_size(float size) { m_tab_buttons_size = size; }
  inline void set_tab_buttons_spacing(float spacing) { m_tab_buttons_spacing = spacing; }
};

class rm_tab_drawer
{
protected:
  static void draw_tab_path(NVGcontext* pctx,
    float x, float y,
    float w, float h,
    rm_vec2 up_offsets,
    float r);
  static void draw_tab_edge(NVGcontext* pctx,
    rm_vec2 pos,
    rm_vec2& size,
    rm_vec2 up_offsets,
    float r,
    const rm_corners_style* pcstyle,
    const rm_color& suncolor,
    const rm_color& shadowcolor);
};

class rm_tabcontrol_ex : public rm_widget,
  public rm_styled<rm_tabcontrol_ex_style>, rm_tab_drawer
{
public:
  enum invalid_index : size_t {
    TAB = (size_t)-1, /*< invalid tab index value */
    ROW = (size_t)-1 /*< invalid tab row index value */
  };

  class rm_tab_button {
    rm_font  m_font;
    uint32_t m_id;
    bool     m_hovered;
    char     m_icon_sym[8]{};
  public:
    rm_tab_button() : m_id(0), m_hovered(false) {}
    rm_tab_button(rm_font font, uint32_t id, const char* putf8str) {
      set_icon_symbol(putf8str);
      m_font=font;
      m_id = id;
    }
    inline bool is_hovered(rm_vec2 &pos, float size, rm_vec2& cursor) {
      m_hovered= rm_bbox(pos, pos + size).inside(cursor);
      return m_hovered;
    }
    inline bool is_hovered() { return m_hovered; }
    inline void set_icon_font(rm_font font) { m_font = font; }
    inline void set_icon_symbol(const char* putf8str) { strncpy(m_icon_sym, putf8str, sizeof(m_icon_sym) - 1); }
    inline void set_id(uint32_t id) { m_id = id; }
    void draw(NVGcontext *pctx, rm_vec2 &pos, float size, rm_tabcontrol_ex_style *pstyle);
  };

  class page : public rm_widget, public rm_styled<rm_tabcontrol_ex_style> {
    friend class tab;
    friend class rm_tabcontrol_ex;
    void on_draw(NVGcontext* pctx) override;
  protected:
    page(rm_tabcontrol_ex *ptabcontrol, rm_vec2 pos, rm_vec2 size) :
      rm_widget(pos.x, pos.y, size.x, size.y, ptabcontrol, "ui_tabcontrolex_page") {
      set_style(ptabcontrol->get_style());
    }
  };

  class tab {
    friend class rm_tabcontrol_ex;
    uint32_t          m_tabid; /*< unique tab id */
    uint32_t          m_flags; /*< tab flags */
    float             m_width;
    rm_vec2           m_textsize;
    std::string       m_name;
    void*             m_puserptr;
    rm_widget*        m_pwidget; /*< widget associated with tab */
    rm_tabcontrol_ex* m_powner; /*< owner tabcontrol */
    std::vector<rm_tab_button> m_buttons;

    void width_recompute();
  public:
    enum {
      FNONE = 0, /*< no flags */
      FOTHER_WIDGET = 1 << 0 /*< m_pwidget have other controlled widget */
    };

    tab() : m_tabid(0),
      m_flags(FNONE),
      m_width(10.f),
      m_puserptr(nullptr),
      m_pwidget(nullptr),
      m_powner(nullptr) {
    }
    tab(rm_tabcontrol_ex *ptabcontrol,
      float width,
      const char *pname,
      uint32_t tabid,
      rm_widget *pwidget = nullptr,
      void *puserptr = nullptr,
      uint32_t flags = FNONE) : m_tabid(tabid), m_flags(flags), m_width(width),
      m_puserptr(puserptr), m_pwidget(pwidget), m_powner(ptabcontrol) {
      set_name(pname);
    }
    rm_tab_button* add_button(rm_font font, uint32_t id, const char* putf8str);
    inline uint32_t get_flags() const { return m_flags; }
    inline float get_width() { return m_width; }
    inline uint32_t get_id() const { return m_tabid; }
    inline const std::string& get_name() const { return m_name; }
    inline void* get_userptr() { return m_puserptr; }
    inline rm_widget* get_page_widget() { return m_pwidget; }
    inline rm_tabcontrol_ex* get_owner() { return m_powner; }
    inline bool is_other_widget() const { return m_flags & FOTHER_WIDGET; }
    void   set_name(const char* pname);
  };

  class tab_row {
    friend class rm_tabcontrol_ex;
    std::vector<tab*> m_tabs; /*< tabs ptrs on this row */

    tab* new_tab(rm_tabcontrol_ex* pcontrol,
      const char* pname,
      uint32_t tabid,
      float width,
      rm_widget* ppage_widget,
      size_t insert_after,
      void* puserptr,
      uint32_t flags);

  public:
    tab_row() {}
    inline size_t get_num_tabs() const { return m_tabs.size(); }
    inline tab* get_tab(size_t tabidx) {
      assert(tabidx < get_num_tabs() && "tab index out of bounds");
      return m_tabs[tabidx];
    }
  };
private:
  size_t m_active_row; /*< active row index (INVALID_ROW if tab not selected) */
  size_t m_active_tab; /*< active tab index (INVALID_TAB if tab not selected) */
  std::vector<tab_row> m_tab_rows; /*< tab rows */

  void on_draw(NVGcontext* pctx) override;
  bool on_mouse(EXGUI_MOUSE_EVENT event, 
    EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  void   hide_all_except(size_t row, size_t tabidx);
  void   get_text_bounds(rm_vec2 &dstsize, const char * ptabname);
  size_t find_free_row_or_create(const char *ptabname);
  inline float get_rows_total_height() {
    assert(m_pstyle && "m_pstyle was nullptr!");
    return static_cast<float>(m_pstyle->get_tab_height() * get_num_rows());
  }
public:
  rm_tabcontrol_ex(rm_widget* p_parent, int x, int y, int width, int height, uint32_t tab_flags, rm_tabcontrol_ex_style *pstyle, size_t num_rows=1);
  static inline bool is_valid_tab(size_t tabid) { return tabid != invalid_index::TAB; }
  inline size_t get_active_tab() const { return m_active_tab; }
  inline size_t get_active_row() const { return m_active_row; }
  inline size_t get_num_rows() const { return m_tab_rows.size(); }
  inline tab_row& get_tab_row(size_t idx) {
    assert(idx < m_tab_rows.size() && "row index out of bounds");
    return m_tab_rows[idx];
  }
  bool   set_num_rows(size_t newsize);
  tab*   find_tab_in_row(size_t rowidx, const char *pname);
  tab*   find_tab_in_row(size_t rowidx, uint32_t tabid);
  size_t find_tab_idx_in_row(size_t rowidx, const char* pname);
  size_t find_tab_idx_in_row(size_t rowidx, uint32_t tabid);

  bool add_child(rm_widget* p_child) = delete;

  tab *add_tab(const char *pname,
    uint32_t tabid,
    float width=1.f,
    void *puserptr = nullptr,
    size_t insert_after= invalid_index::TAB,
    size_t row_index = invalid_index::ROW);

  tab *add_tab_widget(const char *pname,
    uint32_t tabid,
    rm_widget* pwidget,
    float width = 1.f,
    void* puserptr = nullptr,
    size_t insert_after = invalid_index::TAB,
    size_t row_index = invalid_index::ROW);

  bool remove_tab(page *ppage);
  bool remove_tab(size_t tabidx);
  bool select_tab(size_t rowidx, size_t tabidx);
};