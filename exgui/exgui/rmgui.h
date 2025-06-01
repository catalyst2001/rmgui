//
// Copyright (c) 2024 Inradian Developments
// Retained Mode GUI (RmGUI) based on NanoVG
// 
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
// 
// Authors:
//  Mikko Mononen   memon@inside.org (NanoVG author)
//  Kirill Deryabin "catalyst" kd@allalg.ru
//  Daniil Runin "Daniluk2"
//  Igor Shubin "Okay+++"
//

#pragma once
#include "nanovg.h"
#include <vector>
#include <string>
#include <cassert>
#include "rmgui_resources.h"

/* utils */
#define RM_COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define RM_UNUSED(x) (void)(x)
#define RM_HANDLE_EXCEPTIONS(retval, expr) try { expr } catch (...) { return retval; }

/**
* object base class
*/
class rm_object {
  friend class rm_object_accrssor;
protected:
  int m_objid;
public:
  rm_object(int def_val) : m_objid(def_val) {}
  inline int get_handle() const { return m_objid; }
};

template<int k_invalid_value>
class rm_object_base : public rm_object {
public:
  rm_object_base() : rm_object(k_invalid_value) {}
  inline bool is_valid() const { return get_handle() != k_invalid_value; }
  operator int() const { return m_objid; }
  inline static int get_invalid() { return k_invalid_value; }
};

class rm_object_accrssor {
protected:
  inline static void set_handle_value(rm_object &handle, int val) { handle.m_objid = val; }
};

using rm_font = rm_object_base<-1>; //font handle
using rm_image = rm_object_base<0>; //image handle

class rm_vec2
{
public:
  union {
    struct { float x, y; };
    float v[2];
  };
  rm_vec2() : x(0.f), y(0.f) {}
  rm_vec2(float xx, float yy) : x(xx), y(yy) {}
  rm_vec2(int xx, int yy) : x((float)xx), y((float)yy) {}
  ~rm_vec2() {}

  inline void init(float xx, float yy) {
    x = xx;
    y = yy;
  }

  inline rm_vec2 operator=(const rm_vec2& src) {
    if (this != &src) {
      x = src.x;
      y = src.y;
    }
    return *this;
  }
  inline rm_vec2 operator=(rm_vec2& vec) { 
    x = vec.x;
    y = vec.y;
    return *this;
  }
  inline rm_vec2 operator+(rm_vec2& vec) { return rm_vec2(x + vec.x, y + vec.y); }
  inline rm_vec2 operator-(rm_vec2& vec) { return rm_vec2(x - vec.x, y - vec.y); }
  inline rm_vec2 operator*(rm_vec2& vec) { return rm_vec2(x * vec.x, y * vec.y); }
  inline rm_vec2 operator/(rm_vec2& vec) { return rm_vec2(x / vec.x, y / vec.y); }
  inline rm_vec2 operator+(float s) { return rm_vec2(x + s, y + s); }
  inline rm_vec2 operator-(float s) { return rm_vec2(x - s, y - s); }
  inline rm_vec2 operator*(float s) { return rm_vec2(x * s, y * s); }
  inline rm_vec2 operator/(float s) { return rm_vec2(x / s, y / s); }
  inline rm_vec2 operator+=(rm_vec2& vec) { x += vec.x; y += vec.y; return *this; }
  inline rm_vec2 operator-=(rm_vec2& vec) { x -= vec.x; y -= vec.y; return *this; }
  inline rm_vec2 operator*=(rm_vec2& vec) { x *= vec.x; y *= vec.y; return *this; }
  inline rm_vec2 operator/=(rm_vec2& vec) { x /= vec.x; y /= vec.y; return *this; }
  inline rm_vec2 operator*=(float s) { x *= s; y *= s; return *this; }
  inline rm_vec2 operator/=(float s) { x /= s; y /= s; return *this; }
  inline rm_vec2 operator+=(float s) { x += s; y += s; return *this; }
  inline rm_vec2 operator-=(float s) { x -= s; y -= s; return *this; }

  inline bool compare_strong(rm_vec2& vec) { return x == vec.x && y == vec.y; }
  inline bool operator==(rm_vec2& vec) { return fabsf(x - vec.x) < FLT_EPSILON && fabsf(y - vec.y) < FLT_EPSILON; }
  inline bool operator!=(rm_vec2& vec) { return fabsf(x - vec.x) >= FLT_EPSILON && fabsf(y - vec.y) >= FLT_EPSILON; }
  inline bool operator<(rm_vec2& vec) { return x < vec.x && y < vec.y; }
  inline bool operator<=(rm_vec2& vec) { return x <= vec.x && y <= vec.y; }
  inline bool operator>(rm_vec2& vec) { return x > vec.x && y > vec.y; }
  inline bool operator>=(rm_vec2& vec) { return x >= vec.x && y >= vec.y; }
  inline float operator[](int idx) { assert(idx < RM_COUNTOF(v) && "index out of bounds"); return v[idx]; }
  inline float lengthsq() { return x * x + y * y; }
  inline float length() { return sqrtf(lengthsq()); }
};

class rm_rect
{
public:
  union {
    struct { float x, y, width, height; };
    struct { float v[4]; };
  };
  rm_rect() : x(0.f), y(0.f), width(0.f), height(0.f) {}
  rm_rect(const rm_rect& rect) { *this = rect; }
  rm_rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
  rm_rect(float x, float y, const rm_vec2 &size) : x(x), y(y), width(size.x), height(size.y) {}
  rm_rect(const rm_vec2& pos, float w, float h) : x(pos.x), y(pos.y), width(w), height(h) {}
  rm_rect(int x, int y, int w, int h) : x((float)x), y((float)y), width((float)w), height((float)h) {}
  ~rm_rect() {}

  inline void init(float xx, float yy, float wwidth, float hheight) {
    x= xx;
    y= yy;
    width = wwidth;
    height = hheight;
  }

  inline void init(int xx, int yy, int wwidth, int hheight) {
    x = float(xx);
    y = float(yy);
    width = float(wwidth);
    height = float(hheight);
  }

  inline float operator[](int idx) { assert(idx < RM_COUNTOF(v) && "index out of bounds"); return v[idx]; }
};

/* undef min/max if defined macro */
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template<class _type>
_type rm_min(_type a, _type b)
{
  if (a < b)
    return a;
  return b;
}

template<class _type>
_type rm_max(_type a, _type b)
{
  if (a > b)
    return a;
  return b;
}

template<class _type>
_type rm_abs(_type a)
{
  //static_assert(std::is_integral<_type>() || std::is_floating_point<_type>(), "rm_abs have unsupported type!");
  if (a < (_type)0)
    return -a;

  return a;
}

template<class _type>
_type rm_clamp(_type v, _type minval, _type maxval)
{
  return rm_max(minval, rm_min(v, maxval));
}

template<class _type>
_type rm_sign(_type v)
{
  return (v < (_type)0) ? (_type)-1 : (_type)1;
}

class rm_bbox
{
public:
  union {
    struct { rm_vec2 min, max; };
    float array[4];
  };
  
  rm_bbox() : min(0.f, 0.f), max(0.f, 0.f) {}
  rm_bbox(rm_vec2 &_max) : max(_max) {}
  rm_bbox(rm_vec2 _min, rm_vec2 _max) : min(_min), max(_max) {}
  ~rm_bbox() {}

  inline bool  inside(rm_vec2& pt) { return min <= pt && pt <= max; }
  inline float get_width() const { return rm_abs(max.x-min.x); }
  inline float get_height() const { return rm_abs(max.y-min.y); }

  inline void init(rm_vec2& pos, rm_vec2& size) {
    min.x = pos.x;
    min.y = pos.y;
    max.x = min.x + size.x;
    max.y = min.y + size.y;
  }

  inline void from_rect(rm_rect& rect) {
    min.x = rect.x;
    min.y = rect.y;
    max.x = min.x + rect.width;
    max.y = min.y + rect.height;
  }
};

class rmgui_vector3
{
public:
  union {
    struct { float x, y, z; };
    float v[3];
  };
  rmgui_vector3() : x(0.f), y(0.f), z(0.f) {}
  rmgui_vector3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
  ~rmgui_vector3() {}

  rmgui_vector3 operator=(rmgui_vector3& vec) { return *this = vec; }
  rmgui_vector3 operator+(rmgui_vector3& vec) { return rmgui_vector3(x + vec.x, y + vec.y, z + vec.z); }
  rmgui_vector3 operator-(rmgui_vector3& vec) { return rmgui_vector3(x - vec.x, y - vec.y, z - vec.z); }
  rmgui_vector3 operator*(rmgui_vector3& vec) { return rmgui_vector3(x * vec.x, y * vec.y, z * vec.z); }
  rmgui_vector3 operator/(rmgui_vector3& vec) { return rmgui_vector3(x / vec.x, y / vec.y, z / vec.z); }
  rmgui_vector3 operator+(float s) { return rmgui_vector3(x + s, y + s, z + s); }
  rmgui_vector3 operator-(float s) { return rmgui_vector3(x - s, y - s, z - s); }
  rmgui_vector3 operator*(float s) { return rmgui_vector3(x * s, y * s, z * s); }
  rmgui_vector3 operator/(float s) { return rmgui_vector3(x / s, y / s, z / s); }
  rmgui_vector3 operator+=(rmgui_vector3& vec) { x += vec.x; y += vec.y; z += vec.z; return *this; }
  rmgui_vector3 operator-=(rmgui_vector3& vec) { x -= vec.x; y -= vec.y; z -= vec.z; return *this; }
  rmgui_vector3 operator*=(rmgui_vector3& vec) { x *= vec.x; y *= vec.y; z *= vec.z; return *this; }
  rmgui_vector3 operator/=(rmgui_vector3& vec) { x /= vec.x; y /= vec.y; z /= vec.z; return *this; }
  rmgui_vector3 operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
  rmgui_vector3 operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
  rmgui_vector3 operator+=(float s) { x += s; y += s; z += s; return *this; }
  rmgui_vector3 operator-=(float s) { x -= s; y -= s; z -= s; return *this; }
  float         operator[](int idx) { assert(idx < RM_COUNTOF(v) && "index out of bounds"); return v[idx]; }
};

/* virtual keys */
enum RM_KEY : uint32_t {
  RM_KEY_NONE = 0,
  RM_KEY_A,
  RM_KEY_B,
  RM_KEY_C,
  RM_KEY_D,
  RM_KEY_E,
  RM_KEY_F,
  RM_KEY_G,
  RM_KEY_H,
  RM_KEY_I,
  RM_KEY_J,
  RM_KEY_K,
  RM_KEY_L,
  RM_KEY_M,
  RM_KEY_N,
  RM_KEY_O,
  RM_KEY_P,
  RM_KEY_Q,
  RM_KEY_R,
  RM_KEY_S,
  RM_KEY_T,
  RM_KEY_U,
  RM_KEY_V,
  RM_KEY_W,
  RM_KEY_X,
  RM_KEY_Y,
  RM_KEY_Z,
  RM_KEY_0,
  RM_KEY_1,
  RM_KEY_2,
  RM_KEY_3,
  RM_KEY_4,
  RM_KEY_5,
  RM_KEY_6,
  RM_KEY_7,
  RM_KEY_8,
  RM_KEY_9,
  RM_KEY_ESCAPE,
  RM_KEY_ENTER,
  RM_KEY_TAB,
  RM_KEY_BACKSPACE,
  RM_KEY_INSERT,
  RM_KEY_DELETE,
  RM_KEY_RIGHT,
  RM_KEY_LEFT,
  RM_KEY_DOWN,
  RM_KEY_UP,
  RM_KEY_PAGE_UP,
  RM_KEY_PAGE_DOWN,
  RM_KEY_HOME,
  RM_KEY_END,
  RM_KEY_CAPS_LOCK,
  RM_KEY_SCROLL_LOCK,
  RM_KEY_NUM_LOCK,
  RM_KEY_PRINT_SCREEN,
  RM_KEY_PAUSE,
  RM_KEY_CONTROL,
  RM_KEY_LCTRL,
  RM_KEY_RCTRL,
  RM_KEY_F1,
  RM_KEY_F2,
  RM_KEY_F3,
  RM_KEY_F4,
  RM_KEY_F5,
  RM_KEY_F6,
  RM_KEY_F7,
  RM_KEY_F8,
  RM_KEY_F9,
  RM_KEY_F10,
  RM_KEY_F11,
  RM_KEY_F12,
  RM_KEY_LMOUSE,
  RM_KEY_MMOUSE,
  RM_KEY_RMOUSE,
  RM_KEY_XMOUSE1,
  RM_KEY_XMOUSE2,
  RM_KEY_XMOUSE3,
  RM_KEY_XMOUSE4,
  RM_KEY_XMOUSE5,
};

/* key state */
enum RM_KEY_STATE : uint32_t {
  DOWN = 0,
  UP,
  REPEAT
};

/* mouse events */
enum RM_MOUSE_EVENT : uint32_t {
  RM_MOUSE_EVENT_MOVE = 0,
  RM_MOUSE_EVENT_CLICK,
  RM_MOUSE_EVENT_DOUBLE_CLICK
};

/* node event */
enum RM_EVENT : uint32_t {
  PARENT_CHANGED_EVENT = 0,
  PARENT_RESIZE_EVENT,
  PARENT_CHILD_ADDED_EVENT,
  ROOT_RESIZE_EVENT,
  CUSTOM_EVENT
};

/* Node notify and state flags */
#define RM_FLAG_NONE          (0)
#define RM_FLAG_VISIBLE       (1 << 0)
#define RM_FLAG_ACTIVE        (1 << 1)
#define RM_FLAG_NOTIFY_CHILDS (1 << 2)
#define RM_FLAG_HAS_SYM       (1 << 3)
#define RM_FLAG_HAS_KEYBD     (1 << 4)
#define RM_FLAG_HAS_MOUSE     (1 << 5)
#define RM_FLAG_HAS_CHILDS    (1 << 6)

/* special state flags */
#define RM_FLAG_HOVERED       (1 << 7)
#define RM_FLAG_FOCUSED       (1 << 8)
#define RM_FLAG_DRAGGED       (1 << 9)

#define RM_FLAG_GLOBAL        (1 << 10)
#define RM_FLAG_DISABLE_SCISSOR (1 << 11)
#define RM_FLAG_HIGHEST_PRIORITY      (1 << 12)

/* default flags for each widget */
#define RM_FLAG_DEFAULT       (RM_FLAG_VISIBLE|RM_FLAG_ACTIVE|RM_FLAG_NOTIFY_CHILDS|RM_FLAG_HAS_SYM|RM_FLAG_HAS_KEYBD|RM_FLAG_HAS_MOUSE|RM_FLAG_HAS_CHILDS)

/* flags base class */
class rmgui_flags
{
protected:
  uint32_t m_flags;
  inline void set_flags(uint32_t f) { m_flags = f; }

public:
  rmgui_flags(uint32_t flags) : m_flags(flags) {}
  ~rmgui_flags() {}

  inline uint32_t get() { return m_flags; }
  inline void     set(uint32_t f) { m_flags = f; }
  inline bool     is_set(uint32_t flag) { return (m_flags & flag) == flag; }
  inline void     set_bit(uint32_t bit) { m_flags |= bit; }
  inline void     inverse_bit(uint32_t bit) { m_flags &= ~bit; }
  inline void     toggle_bits(uint32_t bit) { m_flags ^= bit; }
  inline void     toggle_bits(uint32_t bits, bool state) { m_flags = state ? (m_flags | bits) : (m_flags & ~bits); }

  operator        uint32_t() { return m_flags; }
  //uint32_t        operator=(uint32_t f) { m_flags = f; }
  uint32_t        operator=(uint32_t f) { m_flags = f; return m_flags; }
};

/* for elements (access to modify only from class rmgui_root) */
class rmgui_flags_elem : public rmgui_flags
{
  /* allow rmgui_root class modify flags */
  friend class rm_surface;
public:
  rmgui_flags_elem() : rmgui_flags(RM_FLAG_DEFAULT) {}
  ~rmgui_flags_elem() {}

  inline bool has_visible() { return is_set(RM_FLAG_VISIBLE); }
  inline bool has_active() { return is_set(RM_FLAG_ACTIVE); }
  inline bool has_notify_childs() { return is_set(RM_FLAG_NOTIFY_CHILDS); }
  inline bool has_symbols_input() { return is_set(RM_FLAG_HAS_SYM); }
  inline bool has_keybd() { return is_set(RM_FLAG_HAS_KEYBD); }
  inline bool has_mouse() { return is_set(RM_FLAG_HAS_MOUSE); }
  inline bool has_childs() { return is_set(RM_FLAG_HAS_CHILDS); }

  /* state flags */
  inline bool is_hovered() { return is_set(RM_FLAG_HOVERED); }
  inline bool is_focused() { return is_set(RM_FLAG_FOCUSED); }

  uint32_t operator=(uint32_t f) {
    m_flags = f;
    return m_flags;
  }
};

/* clipboard data types */
enum RM_CB_DATA_TYPE : uint32_t {
  RM_CLIPBOARD_DATA_TYPE_NONE = 0,
  RM_CLIPBOARD_DATA_TYPE_BIN,
  RM_CLIPBOARD_DATA_TYPE_TEXT
};

/**
* system dependend operations
*/
class irm_sysdf
{
public:
  //virtual                   ~irmgui_sysdf() = 0;
  virtual void               get_cursor_pos(int* p_dst_x, int* p_dst_y) = 0;
  virtual void               set_cursor_pos(int x, int h) = 0;
  virtual uint32_t           num_monitors() = 0;
  virtual void               get_monitor_info(uint32_t monitor_idx, uint32_t *p_dst_DPI, uint32_t *p_w, uint32_t *p_h) = 0;
  virtual bool               get_clipboard_data_info(RM_CB_DATA_TYPE &dst_data, size_t &dst_size) = 0;
  virtual const uint8_t     *get_clipboard_data_ex(RM_CB_DATA_TYPE &dst, size_t &size) =0;
  virtual void               set_clipboard_data_ex(const uint8_t* p_src, size_t size) = 0;
  virtual float              get_time() = 0;
};

/**
* 
*/
class rm_color : public NVGcolor
{
public:
  rm_color(const NVGcolor& nvgcolor) {
    r = nvgcolor.r;
    g = nvgcolor.g;
    b = nvgcolor.b;
    a = nvgcolor.a;
  }
  rm_color(const rm_color& color) {
    r = color.r;
    g = color.g;
    b = color.b;
    a = color.a;
  }
  inline void from_RGB(uint8_t _r, uint8_t _g, uint8_t _b) { 
    *this = nvgRGB(_r, _g, _b); }//TODO: K.D. optimize stack costs!!
  inline void from_RGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) { *this = nvgRGBA(_r, _g, _b, _a); }//TODO: K.D. optimize stack costs!!
  inline void from_HSL(float h, float s, float l) { 
    *this = nvgHSL(h, s, l); }//TODO: K.D. optimize stack costs!!
  inline void from_HSLA(float h, float s, float l, float a) { 
    *this = nvgHSLA(h, s, l, a); }//TODO: K.D. optimize stack costs!!
  inline rm_color& lerp(rm_color &color, float u) {
    *this = nvgLerpRGBA(*this, color, u);//TODO: K.D. optimize stack costs!!
    return *this;
  }
  static inline NVGcolor lerp(const NVGcolor& from_color, const NVGcolor& to_color, float factor) { // NOTE: added by d2
    return NVGcolor{
      from_color.r + (to_color.r - from_color.r) * factor,
      from_color.g + (to_color.g - from_color.g) * factor,
      from_color.b + (to_color.b - from_color.b) * factor,
      from_color.a + (to_color.a - from_color.a) * factor
    };
  }
  inline rm_color& set_transp(uint8_t alpha) {
    *this = nvgTransRGBA(*this, alpha);//TODO: K.D. optimize stack costs!!
    return *this;
  }
  inline rm_color negative() const {
    return rm_color(1.f-r, 1.f-g, 1.f-b, 1.f);
  }
  rm_color() { r=0.f, g=0.f, b=0.f, a=1.f; }
  rm_color(uint8_t _r, uint8_t _g, uint8_t _b) { from_RGB(_r, _g, _b); }
  rm_color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) { *this = nvgRGBA(_r, _g, _b, _a); }
};

class rm_widget;

class rm_event_data
{
  uint32_t m_type;
protected:
  rm_event_data(uint32_t data_type) : m_type(data_type) {}
public:
  inline uint32_t get_type() const { return m_type; }
  template<class _dst_type>
  _dst_type as() { return reinterpret_cast<_dst_type>(this); }
};

/**
* GUI element interface
*/
class irmgui_widget
{
public:
  //virtual     ~irmgui_widget() = 0;
  virtual bool on_event(RM_EVENT event, rm_widget *p_from, rm_event_data *pevent_data) = 0;
  virtual void on_draw(NVGcontext* pctx) = 0;
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) = 0;

  /**
  * @brief Text input handler
  * @param sym - symbol key code
  * @return Nothing
  */
  virtual void on_text_input(int sym) = 0;

  /**
  * @brief Mouse event handler
  * @param event - received mouse event (RM_MOUSE_EVENT_MOVE or RM_MOUSE_EVENT_CLICK)
  * @param vk - received virtual key
  * @param state - received key state (DOWN, UP or REPEAT)
  * @param cursor_pos - received current cursor pos
  * @return To block further propagation of the event, return false. If ture is returned, the event is propagated to the following elements.
  */
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) = 0;
};

/**
* layout interface
*/
class irm_layout
{
public:
  /**
  * @brief performs measurements and pre-calculations
  * of positioning depending on the selected mode (defined by the class implementer)
  */
  virtual bool measure(rm_widget* pwidget) = 0;

  /**
  * @brief performs the arrangement of elements
  * taking into account previously taken measurements
  *
  */
  virtual bool perform(rm_widget* pwidget, rm_vec2 old_size) = 0;

  /**
  * @brief resets previously calculated positions
  */
  virtual bool reset(rm_widget* pwidget) = 0;
};

/**
* basic layout properties for any widget
*/
class rm_basic_layout_props
{
  rm_vec2 m_min_size;
  rm_vec2 m_max_size;
protected:
  rm_basic_layout_props() :
    m_min_size(0.f, 0.f), m_max_size(0.f, 0.f) {}

  inline void init(rm_vec2 size) {
    m_min_size = size;
    m_max_size = size;
  }
public:
  inline void set_min_size(rm_vec2 size) { m_min_size = size; }
  inline void set_max_size(rm_vec2 size) { m_max_size = size; }
  inline const rm_vec2& get_min_size() { return m_min_size; }
  inline const rm_vec2& get_max_size() { return m_max_size; }
};

/**
* flex box layout
*/
class rm_flexbox_layout : public irm_layout
{
public:
  bool measure(rm_widget* pwidget) override;
  bool perform(rm_widget* pwidget, rm_vec2 old_size) override;
  bool reset(rm_widget* pwidget) override;
};

/**
* grid layout
*/
class rm_grid_layout : public irm_layout
{
public:
  bool measure(rm_widget* pwidget) override;
  bool perform(rm_widget* pwidget, rm_vec2 old_size) override;
  bool reset(rm_widget* pwidget) override;
};

enum RM_CORNER : uint32_t {
  LEFT_TOP = 0,
  RIGHT_TOP,
  RIGHT_BOTTOM,
  LEFT_BOTTOM,

  RM_NUM_CORNERS
};

class rm_corners_style
{
protected:
  float m_corner_radius[RM_NUM_CORNERS];
public:
  rm_corners_style() {
    m_corner_radius[LEFT_TOP] = 0.f;
    m_corner_radius[RIGHT_TOP] = 0.f;
    m_corner_radius[RIGHT_BOTTOM] = 0.f;
    m_corner_radius[LEFT_BOTTOM] = 0.f;
  }
  ~rm_corners_style() {}

  inline float get_top_left() const { return m_corner_radius[LEFT_TOP]; }
  inline float get_top_right() const { return m_corner_radius[RIGHT_TOP]; }
  inline float get_bottom_right() const { return m_corner_radius[RIGHT_BOTTOM]; }
  inline float get_bottom_left() const { return m_corner_radius[LEFT_BOTTOM]; }

  inline void  set_all_corners_radius(float radius) {
    m_corner_radius[LEFT_TOP] = 
    m_corner_radius[RIGHT_TOP] = 
    m_corner_radius[RIGHT_BOTTOM] = 
    m_corner_radius[LEFT_BOTTOM] = radius;
  }
  inline void  set_corner_radius(RM_CORNER corner, float radius) { m_corner_radius[corner] = radius; }
  inline float get_corner_radius(RM_CORNER corner) { return m_corner_radius[corner]; }
  inline float get_avg_radius() const {
    float summ = m_corner_radius[0] + m_corner_radius[1] + m_corner_radius[2] + m_corner_radius[3];
    return summ / 4.f;
  }
};

/**
* rendering utilites
*/
class rm_utl
{
public:

  /**
  * draws border frame
  * modes:
  * RM_BFRM_MODE_RAISED
  * RM_BFRM_MODE_SUNKEN
  */
  enum {
    RM_BFRM_MODE_SUNKEN = 0,
    RM_BFRM_MODE_RAISED,
    RM_BFRM_MAX_COLORS //not use! reserved for colors array size!
  };

  /**
  * @brief Draws a frame with a lowered or raised edge
  * @param pctx - nanovg context
  * @param pos - position of frame in current transform
  * @param size - size of frame
  * @param mode - RM_BFRM_MODE_SUNKEN or RM_BFRM_MODE_RAISED
  * @param background - background color for filling
  * @param stroke - stroke color
  * @param stroke_width - stroke width
  * @param colors[] - colors of light (RM_BFRM_MODE_SUNKEN) and shadow (RM_BFRM_MODE_RAISED). Size of this array must be RM_BFRM_MAX_COLORS
  * @param pcstyle - defined corners radius style
  * @param shadow_offset - offset of shadow inside of frame
  * @returns nothing
  */
  static void draw_frame(NVGcontext* pctx,
    rm_vec2 &pos,
    rm_vec2 &size,
    uint32_t mode,
    const NVGcolor &background,
    const NVGcolor &stroke,
    float stroke_width, 
    const NVGcolor colors[],
    const rm_corners_style *pcstyle,
    float shadow_offset=1.f);

  static void draw_shadow(NVGcontext* pctx, rm_vec2 pos, rm_vec2 &size, rm_vec2 dir, float offset_scale,
    const NVGcolor &shadow_color, float shadow_size, float corner_radius, bool draw_shadow_center=false);

  static void draw_edge(NVGcontext* pctx, rm_vec2 pos, rm_vec2& size,
    const rm_corners_style *pcstyle, const rm_color& suncolor, const rm_color& shadowcolor);

  static inline const NVGcolor &get_transparent() {
    static const NVGcolor g_transparent_color = nvgRGBA(0, 0, 0, 0);
    return g_transparent_color;
  }
};

/**
* rmgui_widget
* 
* base class for all GUI elements
*/
class rm_surface;

class rm_widget : protected irmgui_widget, public rm_basic_layout_props
{
  /* allow rmgui_root class to call irmgui_element vmethods */
  friend class rm_surface;
  inline void set_root(rm_surface* p_root) { m_proot = p_root; }

protected:
  /* irmgui_element empty impls */
  virtual bool on_event(RM_EVENT event, rm_widget *p_from, rm_event_data* pevent_data) {
    RM_UNUSED(event);
    RM_UNUSED(p_from);
    RM_UNUSED(pevent_data);
    return true;
  }
  virtual void on_draw(NVGcontext* pctx) {
#ifdef RMGUI_DEBUG_DRAW
    static NVGcolor colors[] = {
      nvgRGB(255, 0, 0), nvgRGB(0, 255, 0)
    };
    nvgBeginPath(pctx);
    nvgRect(pctx, 0.f, 0.f, m_size.x, m_size.y);
    nvgStrokeColor(pctx, colors[get_elem_flags().is_hovered()]);
    nvgStrokeWidth(pctx, 2.f);
    nvgStroke(pctx);
#endif
  }
  virtual void on_keybd(int sc, RM_KEY vk, RM_KEY_STATE state) {
    RM_UNUSED(sc);
    RM_UNUSED(vk);
    RM_UNUSED(state);
  }
  virtual void on_text_input(int sym) {
    RM_UNUSED(sym);
  }
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta) {
    RM_UNUSED(event);
    RM_UNUSED(vk);
    RM_UNUSED(state);
    RM_UNUSED(cursor_pos);
    return true;
  }

protected:
  using _childs_vec = std::vector<rm_widget*>;
  _childs_vec      m_childs;
  rm_surface      *m_proot;
  rm_widget       *m_pparent;
  void            *m_puserptr;
  irm_sysdf       *m_psysdf;
  rmgui_flags_elem m_elem_flags;
  uint32_t         m_user_flags;
  rm_font          m_font;
  char             m_szclass[32];
  rm_bbox          m_bbox;
  rm_vec2          m_size; //width;height
  rm_vec2          m_pos_of_parent;
  rm_rect          m_content_area;
  int              m_zindex;

  ///* rmgui_root::rebuild_draw_cache accessor class */
  //class rmgui_root_update_acessor : public rmgui_root {
  //public:
  //  rmgui_root_update_acessor() {}
  //  ~rmgui_root_update_acessor() {}
  //  inline void rebuild_draw_cache() {
  //    rmgui_root::rebuild_draw_cache();
  //  }
  //};

  /* perform update root draw cache */
  inline void root_update() { /*((rmgui_surface *)m_proot)->rebuild_draw_cache();*/ }
  static void move_childs_relative(rm_widget *pwidget, rm_vec2 deltapos);
  static void move_to(rm_widget* proot_widget, rm_vec2 newpos);

  inline bool dispatch_event(RM_EVENT event, rm_widget* p_from, rm_event_data* pevent_data) {
    return on_event(event, p_from, pevent_data);
  }

public:
  void set_classname(const char* p_clsn) {
    strncpy(m_szclass, p_clsn, sizeof(m_szclass) - 1);
  }
  inline rm_surface* get_root() { return m_proot; }

  void grab_globals_from(rm_widget* p_parent) {
    /* get sysdf ifaec from parent */
    m_psysdf = m_pparent->get_sysdf();
    /* get root m_pparent from parent */
    m_proot = m_pparent->get_root();
  }

  rm_widget(int x, int y, int width, int height, rm_widget *p_parent, const char *p_classname,
    uint32_t flags = RM_FLAG_DEFAULT, uint32_t uflags = 0, void *p_userptr = nullptr) : m_proot(nullptr),
    m_pparent(p_parent), m_puserptr(p_userptr), m_psysdf(nullptr), m_zindex(0) {
    rm_vec2 parent_coord;
    if (m_pparent) {
      parent_coord = m_pparent->get_pos_of_parent();
      m_pparent->add_child(this);
      grab_globals_from(m_pparent);
    }
    m_elem_flags = flags;
    m_user_flags = uflags;
    m_pos_of_parent.init(x, y);
    m_size.init(width, height);
    m_bbox.init(m_pos_of_parent, m_size);
    m_content_area.init(0.f, 0.f, m_size.x, m_size.y);

    /* set font from root */
    if (m_proot)
      set_font(((rm_widget *)m_proot)->get_font());

    set_classname(p_classname);
  }
  ~rm_widget() {}

  inline const char *get_classname() { return m_szclass; }
  inline void       *get_userptr() { return m_puserptr; }
  inline bool        classname_is(const char* pname) { return !strcmp(m_szclass, pname); }

  template<class _dst_type>
  inline _dst_type  *get_userptr() { return reinterpret_cast<_dst_type*>(m_puserptr); }
  inline void        set_userptr(void* p) { m_puserptr = p; }

  /* rect && bbox */
  inline rm_bbox    &get_bbox() { return m_bbox; }
  inline rm_vec2    &get_pos_of_parent() { return m_pos_of_parent; }
  inline rm_vec2    &get_size() { return m_size; }
  inline rm_rect    &get_content_area() { return m_content_area; }

  /* visual */
  inline bool        is_visible() { return m_elem_flags.has_visible(); }
  inline void        show(bool b_show = true) { m_elem_flags.toggle_bits(RM_FLAG_VISIBLE, b_show); }
  inline void        hide() { show(false); }

  /* childs */
  inline size_t      get_num_childs() const { return m_childs.size(); }
  inline rm_widget*  get_child(size_t idx) { return m_childs[idx]; }
  inline rm_widget** get_all_childs() { return m_childs.data(); }
  bool               add_child(rm_widget* p_child);
  bool               remove_child(rm_widget* p_child);
  rm_widget         *find_child(const char* pclassname) const;

  /* parent */
  inline rm_widget  *get_parent() { return m_pparent; }
  void               set_parent(rm_widget* p_parent);

  /* flags */
  inline rmgui_flags_elem get_elem_flags() { return m_elem_flags; }
  inline uint32_t       get_user_flags() { return m_user_flags; }

  /* state active */
  inline void       set_enabled(bool enabled) {m_elem_flags.toggle_bits(RM_FLAG_ACTIVE, enabled);}
  inline const bool is_enabled() {return m_elem_flags.has_active();}

  /* system dependend functions interface */
  inline irm_sysdf  *get_sysdf() { return m_psysdf; }

  /* font */
  inline void        set_font(rm_font font) { m_font = font; }
  inline rm_font     get_font() { return m_font; }

  /* layers */
  inline void        set_zindex(int zidx) { m_zindex = zidx; }
  inline int         get_zindex() const { return m_zindex; }

  void resize(float width, float height);
  inline void resize(rm_vec2 newsize) { resize(newsize.x, newsize.y); }

  void move(int newx, int newy) { move_to(this, { newx, newy }); }
  void move(rm_vec2 newpos) { move_to(this, newpos); }
  void move_relative(rm_vec2& delta);

  rm_vec2 cursor_to_local(const rm_vec2& cursor_pos) {
    return rm_vec2(cursor_pos.x - m_pos_of_parent.x, cursor_pos.y - m_pos_of_parent.y);
  }
};

class rm_surface : public rm_widget, rm_object_accrssor
{
  NVGcontext *m_pctx;
  rm_widget  *m_pfocus;
  float       m_delta_time;
  float       m_device_pixel_ratio;
  rm_vec2     m_last_cursor;
  rm_vec2     m_delta_cursor;

  /* event notifier functions */
  static void keybd_dispatcher(rm_widget *p_elem, int sc, 
    RM_KEY vk, RM_KEY_STATE state);
#if 0
  static void text_input_dispatcher(rmgui_widget *p_elem, int sym);
#endif
  bool mouse_dispatcher(rm_widget *p_elem,
    RM_MOUSE_EVENT event, RM_KEY vk, 
    RM_KEY_STATE state, rm_vec2 &cursor_pos);
  void draw_recursive(rm_widget* p_elem, float dt);

public:
  rm_surface(NVGcontext *pctx, int width, int height, irm_sysdf *p_sysdf);
  ~rm_surface();

  /* main events */
  void draw(float dt);
  void keybd(int sc, RM_KEY vk, RM_KEY_STATE state);
  void textinput(int sym);
  void mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, int x, int y);

  NVGcontext* get_context() { return m_pctx; }

  /* delta time */
  inline float get_delta_time() const { return m_delta_time; }

  /* images */
  rm_image load_image_from_memory(const void *psrc, size_t srclen, int flags);
  rm_image load_image(const char *pfilename, int flags);
  void     free_image(rm_image& image);

  /* fonts */
  rm_font  load_font_from_memory(const void* psrc_ttf_mem, size_t srclen, const char* pfontname);
  rm_font  load_font(const char* pfilename, const char *pfontname);
  rm_font  find_font(const char* pfontname);
  void     free_font(rm_font& font);

  /* pixel ratio */
  void     set_device_pixel_ratio(float ratio);
  float    get_device_pixel_ratio();

  /* utility */
  void get_text_bounds(rm_bbox& dst,
    const char* ptext,
    rm_font hfont,
    rm_vec2 start = rm_vec2(0.f, 0.f));
  float get_text_width(const char* ptext,
    rm_font hfont,
    rm_vec2 start = rm_vec2(0.f, 0.f));
  float get_text_height(const char* ptext,
    rm_font hfont,
    rm_vec2 start = rm_vec2(0.f, 0.f));
};

class rmgui_timer
{
  float m_curr_time;
  float m_next_time;
  float m_interval;
public:
  rmgui_timer() : m_curr_time(0.f), m_next_time(0.f), m_interval(1.f) {}
  rmgui_timer(float current_time, float interval) {
    m_interval = interval;
    m_curr_time = current_time;
    m_next_time = m_curr_time + m_interval;
  }

  void  set_interval(float f) { m_interval = f; }
  float get_interval() { return m_interval; }

  bool has_elapsed(irm_sysdf* p_sysdf);

  void reset(irm_sysdf* p_sysdf) { m_curr_time = p_sysdf->get_time(); m_next_time = m_curr_time + m_interval; }
};

/**
* callback for element
*/
template<class _callback_type>
class rm_callback
{
protected:
  _callback_type m_pcallback;
public:
  rm_callback() : m_pcallback(nullptr) {}
  ~rm_callback() {}

  inline bool             is_valid_callback() const { return m_pcallback != nullptr; }
  inline _callback_type   get_callback() const { return m_pcallback; }
  inline void             set_callback(_callback_type pcb) { m_pcallback = pcb; }
};

/**
* text input handler
*/

/*
===========================================================================================
 DEFAULT WIDGETS HERE
===========================================================================================
*/

/**
* styled widget base class
*/
template<class _dst_style_type>
class rm_styled
{
protected:
  _dst_style_type* m_pstyle;
public:
  rm_styled() : m_pstyle(nullptr) {}
  ~rm_styled() {}

  inline _dst_style_type* get_style() { return m_pstyle; }
  inline void             set_style(_dst_style_type* p_style) { m_pstyle = p_style; }
};

/**
* Window
*/
class rm_window_style : public rm_corners_style
{
protected:
  float    m_title_font_size;
  float    m_title_font_blur_factor;
  NVGcolor m_title_font_color;
  NVGcolor m_title_font_shadow_color;
  union {
    struct {
      NVGcolor m_active_background_color;
      NVGcolor m_inactive_background_color;
    };
    NVGcolor m_background_colors[2];
  };

  NVGcolor m_titlebar_background_color;
  NVGcolor m_titlebar_shadow_color;
  NVGcolor m_titlebar_shadow_alpha_color;
  float    m_titlebar_height;
  NVGcolor m_window_top_gradient;
  NVGcolor m_window_bottom_gradient;
public:
  rm_window_style() {
    apply_defaults();
  }

  inline float     get_font_size() { return m_title_font_size; }
  inline float     get_font_blur_factor() { return m_title_font_blur_factor; }
  inline NVGcolor &get_font_color() { return m_title_font_color; }
  inline NVGcolor &get_font_shadow_color() { return m_title_font_shadow_color; }
  inline NVGcolor &get_active_background_color() { return m_active_background_color; }
  inline NVGcolor &get_inactive_background_color() { return m_inactive_background_color; }
  inline NVGcolor &get_background_color(int idx) { 
    assert(idx < RM_COUNTOF(m_background_colors) && "index out of bounds");
    return m_background_colors[idx];
  }
  inline NVGcolor &get_titlebar_background_color() { return m_titlebar_background_color; }
  inline NVGcolor &get_titlebar_shadow_color() { return m_titlebar_shadow_color; }
  inline NVGcolor &get_titlebar_shadow_alpha_color() { return m_titlebar_shadow_alpha_color; }
  inline float     get_titlebar_height() { return m_titlebar_height; }
  inline NVGcolor &get_top_gradient_color() { return m_window_top_gradient; }
  inline NVGcolor &get_bottom_gradient_color() { return m_window_bottom_gradient; }

  /* setters */
  void apply_defaults() {
    set_corner_radius(LEFT_TOP, 5.f);
    set_corner_radius(RIGHT_TOP, 5.f);
    set_corner_radius(RIGHT_BOTTOM, 5.f);
    set_corner_radius(LEFT_BOTTOM, 5.f);

    m_title_font_size = 1.f;
    m_title_font_blur_factor = 0.f;
    m_title_font_color = nvgRGBA(255, 255, 255, 255);
    m_title_font_shadow_color = nvgRGBA(128, 128, 128, 128);
    m_active_background_color = nvgRGBA(40, 40, 40, 128);
    m_inactive_background_color = nvgRGBA(20, 20, 20, 128);

    m_titlebar_height=10.f;
    m_titlebar_background_color = nvgRGBA(111, 111, 255, 128);
    m_titlebar_shadow_color = nvgRGBA(20, 20, 20, 128);
    m_titlebar_shadow_alpha_color = nvgRGBA(20, 20, 20, 128);
    m_window_top_gradient = nvgRGBA(40, 40, 40, 255);
    m_window_bottom_gradient = nvgRGBA(45, 45, 45, 255);
  }
};

/**
* orientation for scroll/progress/slider widgets
*/
enum RM_ORIENT : uint32_t {
  RM_ORIENT_AUTO=0,
  RM_ORIENT_HORZ,
  RM_ORIENT_VERT
};

enum rm_window_flags : uint32_t {
  WCF_NONE = 0,
  WCF_VRESIZE=1<<0,
  WCF_HRESIZE=1<<1,
};

class rm_window : public rm_widget, public rm_styled<rm_window_style>
{
  enum {
    FL_NONE=0,
    FL_DRAG=1<<0,
    FL_RESIZE=1<<1
  };
  uint32_t                m_flags;
  std::vector<rm_widget*> m_top_widgets;
  rm_window_style        *m_pstyle;

  /* paint window background */
  virtual void on_draw(NVGcontext* pctx);
  virtual bool on_mouse(RM_MOUSE_EVENT event, RM_KEY vk, RM_KEY_STATE state, rm_vec2& cursor_pos, rm_vec2 delta);

public:
  rm_window(rm_widget* p_parent, int x, int y, int width, int height, uint32_t flags = RM_FLAG_DEFAULT|RM_FLAG_GLOBAL, uint32_t uflags = 0, void* p_userptr = nullptr);
  ~rm_window();
};

class rmgui_textbuffer {
private:
  struct state { 
    std::string text; 
    size_t cur, sel_start, sel_end; 
  };

  std::string         text;
  size_t              cursor;
  std::vector<state>  undos;
  std::vector<state>  redos;
public:
  rmgui_textbuffer() : cursor(0), sel_start(0), sel_end(0) {}
  void set_cursor(size_t pos) {cursor = pos; clear_selection();}
  void insert_cp(uint32_t cp);
  void backspace();

  //void cut_all();
  void cut_selection(irm_sysdf* psysdf);
  void copy_all(irm_sysdf* psysdf);
  void paste(irm_sysdf* psysdf);
  void select_all() { sel_start = 0; sel_end = text.size(); cursor = sel_end; }

  void undo();
  void redo();

  void move_cursor_left();
  void move_cursor_right();
  void move_cursor_up();
  void move_cursor_down();

  void delete_forward();

  void clear_selection() { sel_start = sel_end = cursor; }
  bool has_selection() const { return sel_start != sel_end; }

  const std::string& str() const { return text; }
  size_t pos() const { return cursor; }
  size_t sel_start, sel_end;

private:
  void save_undo();
  void clear_redo() { redos.clear(); }
  void delete_selection();
};

/**
* class utils
*/
template<class _class, typename ..._args>
void rm_construct(_class& obj, _args... args) {
  new (obj) _class(args...);
}

template<class _class>
void rm_destruct(_class& obj) {
  _class::operator delete(&obj);
}

class rm_string_tokenizer
{
  char         m_delim;
  bool         m_next_token_available;
  size_t       m_last_pos;
  size_t       m_cur_pos;
  const std::string& m_str;
public:
  rm_string_tokenizer(const std::string& target, char delim);
  bool get_token(std::string& dst);
  inline bool next_token() { return m_next_token_available; }
};

class rm_line_ring_buffer
{
public:
  /**
  * ring buffer one line
  */
  class rm_rb_line {
    using _width_vec = std::vector<float>;
    size_t      m_sel_begin;
    size_t      m_sel_end;
    size_t      m_cursor;
    _width_vec  m_syms_width;
    std::string m_line;
  public:
    rm_rb_line();
    bool                      set_string(const char* pstr);
    bool                      insert_from_cursor(const char* pstr);
    void                      sym_widths_recompute(NVGcontext* pctx);
    bool                      get_substring_from_selection(std::string& dst);
    inline const std::string& get_string() const { return m_line; }
    inline void               clear() { m_line.clear(); }
    inline const char* get_cstr() const { return m_line.c_str(); }
    inline const std::vector<float>& get_line_widths() const { return m_syms_width; }
    inline bool               is_valid_cusor() const { return m_cursor < m_line.size(); }
  };
private:
  size_t m_start_line;
  size_t m_num_output_lines;
  size_t m_sel_line_begin;
  size_t m_sel_line_end;
  std::vector<rm_rb_line> m_lines_buf;

  rm_rb_line* get_line_for_write();
  const char* format_string(std::string& dst, const char* pformat, va_list argptr);
public:
  rm_line_ring_buffer(size_t ringbuf_size = 32768, size_t line_limit = 512, size_t num_output_lines = 16);
  void set_selection(size_t beginline, size_t endline);
  void get_selection(size_t& beginline, size_t& endline);
  bool get_selection_text_size(size_t& dstlen);
  bool copy_selection(std::string& dst);
  bool clipboard_copy(irm_sysdf* psdf);
  void set_num_output_lines(size_t numlines);

  /* work with lines */
  inline size_t      get_num_lines() const { return m_lines_buf.size(); }
  inline rm_rb_line* get_line(size_t idx) {
    assert(idx < m_lines_buf.size() && "line index out of bounds!");
    return &m_lines_buf[idx];
  }

  /* get lines for output (drawing text) */
  inline size_t     get_num_output_lines() const { return m_num_output_lines; }
  const rm_rb_line* get_output_line(size_t idx);

  bool append_text(const std::string& content);
  //bool append_text(const char* pformat, ...);
};