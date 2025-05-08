#pragma once
#include "rmgui.h"
#include <string>
#include <vector>

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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
};

class rm_button : public rm_widget {
  std::string m_text;
public:
  rm_button(rm_widget* p_parent, int x, int y, int width, int height, const std::string& text);
  virtual ~rm_button();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
};

class rm_label : public rm_widget {
  std::string m_text;
public:
  rm_label(rm_widget* p_parent, int x, int y, const std::string& text);
  virtual ~rm_label();
  virtual void on_draw(NVGcontext* p_ctx) override;
};

enum RMGUI_TEXT_INPUT_FLAGS {
  RMGUI_TEXT_INPUT_SINGLELINE = 0,
  RMGUI_TEXT_INPUT_MULTILINE = 1 << 0
};

class rm_text_input : public rm_widget {
  std::string m_text;
  bool m_active;
  rmgui_timer m_timer;
  bool        m_blink_state;
  float       m_text_offset;
  uint32_t    m_flags;
public:
  rm_text_input(rm_widget* p_parent, int x, int y, int width, int height, uint32_t flags/* = RMGUI_TEXT_INPUT_SINGLELINE*/, float blink_cursor_interval = 0.5f);
  virtual ~rm_text_input();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual void on_text_input(int sym) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
};

/**
* CHECKBOX
*/
class rm_checkbox_style : public rm_corners_style {
  rmgui_vector2 m_text_offset;
  NVGcolor      m_text_color;
  NVGcolor      m_bkg_color;
  NVGcolor      m_mark_color;
  NVGcolor      m_border_color;
  int           m_check_size;
  float         m_font_size;
public:
  rm_checkbox_style() :
    m_text_offset(5.f, 0.f),
    m_text_color(nvgRGB(255, 255, 255)),
    m_bkg_color(nvgRGB(255, 255, 255)),
    m_mark_color(nvgRGB(0, 0, 0)),
    m_border_color(nvgRGB(0, 0, 0)),
    m_check_size(20),
    m_font_size(18.f) {}

  /* selectors  */
  inline const rmgui_vector2& get_text_offsets() const { return m_text_offset; }
  inline const NVGcolor& get_text_color() const { return m_text_color; }
  inline const NVGcolor& get_background_color() const { return m_bkg_color; }
  inline const NVGcolor& get_mark_color() const { return m_mark_color; }
  inline const NVGcolor& get_border_color() const { return m_border_color; }
  inline int             get_check_size() const { return m_check_size; }
  inline float           get_font_size() const { return m_font_size; }

  /* modifiers */
  inline void set_text_offsets(rmgui_vector2 offset) { m_text_offset = offset; }
  inline void set_text_color(NVGcolor clr) { m_text_color = clr; }
  inline void set_background_color(NVGcolor clr) { m_bkg_color = clr; }
  inline void set_mark_color(NVGcolor clr) { m_mark_color = clr; }
  inline void set_border_color(NVGcolor clr) { m_border_color = clr; }
  inline void set_check_size(int newsize) { m_check_size = newsize; }
  inline void set_font_size(float fsize) { m_font_size = fsize; }
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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
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

  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
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
  void    compute_value(rmgui_vector2& cursor_pos);
  void    compute_inner_and_thumb();
public:
  rm_slider(rm_widget* p_parent, int x, int y, int width, int height, float min, float max, float initial, rm_slider_callback pcallback=nullptr);
  virtual ~rm_slider();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos);
  float get_value() const { return m_value; }
};

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
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos);
};

/**
* progress widget
* with image pattern
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
class rm_scroll_base
{
  NVGcolor  m_background_clr;
  NVGcolor  m_thumb_clr;
  float     m_corner_round;
  float     m_thumb_size;
  RM_ORIENT m_orientation;
protected:
  void orient_detect(const rm_rect &background);
  void draw(NVGcontext* p_ctx, const rm_rect &back, float pos);
};