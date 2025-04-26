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

class rm_checkbox : public rm_widget {
  bool m_checked;
  std::string m_label;
public:
  rm_checkbox(rm_widget* p_parent, int x, int y, int size, const std::string& label);
  virtual ~rm_checkbox();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
};

class rm_combobox : public rm_widget {
  std::vector<std::string> m_items;
  int m_selected;
  bool m_expanded;
public:
  rm_combobox(rm_widget* p_parent, int x, int y, int width, int height, const std::vector<std::string>& items);
  virtual ~rm_combobox();
  virtual void on_draw(NVGcontext* p_ctx) override;
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) override;
};

class rm_slider : public rm_widget {
  float m_min;
  float m_max;
  float m_value;
  bool m_dragging;
public:
  rm_slider(rm_widget* p_parent, int x, int y, int width, int height, float min, float max, float initial);
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