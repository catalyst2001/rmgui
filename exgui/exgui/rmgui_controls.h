#pragma once
#include "rmgui.h"
#include <string>
#include <vector>
#include <functional>

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
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
};

class rm_button : public rm_widget {
  std::string m_text;
public:
  rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text);
  virtual ~rm_button();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;
};

class rm_label : public rm_widget {
  std::string m_text;
public:
  rm_label(rm_widget* p_parent, int x, int y, const std::string& text);
  virtual ~rm_label();
  virtual void on_draw(NVGcontext* p_ctx) override;
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
    m_unactive_bg_color(nvgRGBA(230, 230, 230, 255)),
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
  std::vector<float>  m_glyph_positions;
  std::string         m_text;
  rmgui_textbuffer    m_buffer;
  bool                m_ctrl_pressed;
  bool                m_active;
  bool                m_dragging;
  rmgui_timer         m_timer;
  bool                m_blink_state;
  uint32_t            m_flags;
public:
  rm_text_input(rm_widget* p_parent, int x, int y, int width, int height, rm_text_input_style* pstyle, uint32_t flags/* = RMGUI_TEXT_INPUT_SINGLELINE*/, float blink_cursor_interval = 0.5f);
  virtual ~rm_text_input();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual void on_keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state) override;
  virtual void on_text_input(int sym) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  // map local x-coordinate to character index
  size_t hit_test_index(float px) const {
    size_t n = m_glyph_positions.size();
    if (n == 0) return 0;
    if (px <= m_glyph_positions[0]) return 0;
    if (px >= m_glyph_positions[n - 1]) return n - 1;
    for (size_t i = 1; i < n; ++i) {
      float left = m_glyph_positions[i - 1];
      float right = m_glyph_positions[i];
      float mid = (left + right) * 0.5f;
      if (px < mid)
        return i - 1;
    }
    return n - 1;
  }
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

  virtual void on_draw(NVGcontext* p_ctx) override;
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

  virtual void on_draw(NVGcontext* p_ctx) override;
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

  virtual void on_draw(NVGcontext* p_ctx) override;
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
  void         set_percent(float p) { m_percent = rmgui_clamp(p, 0.f, 100.f); }
  inline float get_percent() const { return m_percent; }
  void         set_corner_round(float p) { m_round = p; }
  inline float get_corner_round() const { return m_round; }
  virtual void on_draw(NVGcontext* p_ctx) override;
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
  virtual void    on_draw(NVGcontext* p_ctx) override;
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
  void draw_scroll(NVGcontext* p_ctx, const rm_vec2&back, rm_scroll_style *pstyle, float pos);
  void draw_scroll(NVGcontext* p_ctx, rm_scroll_style* pstyle, rm_vec2 content_size, const rm_vec2& window_size, float thumb_height, float pos);
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

  virtual void on_draw(NVGcontext* p_ctx) override;
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
  virtual void on_draw(NVGcontext* p_ctx) override;
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
  NVGcolor      m_brd_stroke_color;
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
  virtual void on_draw(NVGcontext* p_ctx) override;
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
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rm_vec2& cursor_pos) override;

  // recursive draw helper
  float draw_node(NVGcontext* p_ctx, rm_tree_node* node, float x, float y);
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