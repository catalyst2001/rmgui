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
#define EXGUI_COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define RMGUI_UNUSED(x) (void)(x)

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

class rmgui_vector2
{
public:
  union {
    struct { float x, y; };
    float v[2];
  };
  rmgui_vector2() : x(0.f), y(0.f) {}
  rmgui_vector2(float xx, float yy) : x(xx), y(yy) {}
  rmgui_vector2(int xx, int yy) : x((float)xx), y((float)yy) {}
  ~rmgui_vector2() {}

  inline rmgui_vector2 operator=(rmgui_vector2& vec) { return *this = vec; }
  inline rmgui_vector2 operator+(rmgui_vector2& vec) { return rmgui_vector2(x + vec.x, y + vec.y); }
  inline rmgui_vector2 operator-(rmgui_vector2& vec) { return rmgui_vector2(x - vec.x, y - vec.y); }
  inline rmgui_vector2 operator*(rmgui_vector2& vec) { return rmgui_vector2(x * vec.x, y * vec.y); }
  inline rmgui_vector2 operator/(rmgui_vector2& vec) { return rmgui_vector2(x / vec.x, y / vec.y); }
  inline rmgui_vector2 operator+(float s) { return rmgui_vector2(x + s, y + s); }
  inline rmgui_vector2 operator-(float s) { return rmgui_vector2(x - s, y - s); }
  inline rmgui_vector2 operator*(float s) { return rmgui_vector2(x * s, y * s); }
  inline rmgui_vector2 operator/(float s) { return rmgui_vector2(x / s, y / s); }
  inline rmgui_vector2 operator+=(rmgui_vector2& vec) { x += vec.x; y += vec.y; return *this; }
  inline rmgui_vector2 operator-=(rmgui_vector2& vec) { x -= vec.x; y -= vec.y; return *this; }
  inline rmgui_vector2 operator*=(rmgui_vector2& vec) { x *= vec.x; y *= vec.y; return *this; }
  inline rmgui_vector2 operator/=(rmgui_vector2& vec) { x /= vec.x; y /= vec.y; return *this; }
  inline rmgui_vector2 operator*=(float s) { x *= s; y *= s; return *this; }
  inline rmgui_vector2 operator/=(float s) { x /= s; y /= s; return *this; }
  inline rmgui_vector2 operator+=(float s) { x += s; y += s; return *this; }
  inline rmgui_vector2 operator-=(float s) { x -= s; y -= s; return *this; }

  inline bool compare_strong(rmgui_vector2& vec) { return x == vec.x && y == vec.y; }
  inline bool operator==(rmgui_vector2& vec) { return fabsf(x - vec.x) < FLT_EPSILON && fabsf(y - vec.y) < FLT_EPSILON; }
  inline bool operator!=(rmgui_vector2& vec) { return fabsf(x - vec.x) >= FLT_EPSILON && fabsf(y - vec.y) >= FLT_EPSILON; }
  inline bool operator<(rmgui_vector2& vec) { return x < vec.x && y < vec.y; }
  inline bool operator<=(rmgui_vector2& vec) { return x <= vec.x && y <= vec.y; }
  inline bool operator>(rmgui_vector2& vec) { return x > vec.x && y > vec.y; }
  inline bool operator>=(rmgui_vector2& vec) { return x >= vec.x && y >= vec.y; }
  inline float operator[](int idx) { assert(idx < EXGUI_COUNTOF(v) && "index out of bounds"); return v[idx]; }
};

class rm_rect
{
public:
  union {
    struct { float x, y, width, height; };
    struct { float v[4]; };
  };
  rm_rect() {}
  rm_rect(rm_rect& rect) { *this = rect; }
  rm_rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
  rm_rect(int x, int y, int w, int h) : x((float)x), y((float)y), width((float)w), height((float)h) {}
  ~rm_rect() {}

  inline float operator[](int idx) { assert(idx < EXGUI_COUNTOF(v) && "index out of bounds"); return v[idx]; }
};

/* undef min/max if defined macro */
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template<class _type>
_type rmgui_min(_type a, _type b)
{
  if (a < b)
    return a;
  return b;
}

template<class _type>
_type rmgui_max(_type a, _type b)
{
  if (a > b)
    return a;
  return b;
}

template<class _type>
_type rmgui_clamp(_type v, _type minval, _type maxval)
{
  return rmgui_max(minval, rmgui_min(v, maxval));
}

class rm_bbox
{
public:
  rmgui_vector2 min, max;
  rm_bbox() {}
  ~rm_bbox() {}

  inline bool inside(rmgui_vector2& pt) {
    return min <= pt && pt <= max;
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
  float         operator[](int idx) { assert(idx < EXGUI_COUNTOF(v) && "index out of bounds"); return v[idx]; }
};

/* virtual keys */
enum EXGUI_KEY : uint32_t {
  EXGUI_KEY_NONE = 0,
  EXGUI_KEY_A,
  EXGUI_KEY_B,
  EXGUI_KEY_C,
  EXGUI_KEY_D,
  EXGUI_KEY_E,
  EXGUI_KEY_F,
  EXGUI_KEY_G,
  EXGUI_KEY_H,
  EXGUI_KEY_I,
  EXGUI_KEY_J,
  EXGUI_KEY_K,
  EXGUI_KEY_L,
  EXGUI_KEY_M,
  EXGUI_KEY_N,
  EXGUI_KEY_O,
  EXGUI_KEY_P,
  EXGUI_KEY_Q,
  EXGUI_KEY_R,
  EXGUI_KEY_S,
  EXGUI_KEY_T,
  EXGUI_KEY_U,
  EXGUI_KEY_V,
  EXGUI_KEY_W,
  EXGUI_KEY_X,
  EXGUI_KEY_Y,
  EXGUI_KEY_Z,
  EXGUI_KEY_0,
  EXGUI_KEY_1,
  EXGUI_KEY_2,
  EXGUI_KEY_3,
  EXGUI_KEY_4,
  EXGUI_KEY_5,
  EXGUI_KEY_6,
  EXGUI_KEY_7,
  EXGUI_KEY_8,
  EXGUI_KEY_9,
  EXGUI_KEY_ESCAPE,
  EXGUI_KEY_ENTER,
  EXGUI_KEY_TAB,
  EXGUI_KEY_BACKSPACE,
  EXGUI_KEY_INSERT,
  EXGUI_KEY_DELETE,
  EXGUI_KEY_RIGHT,
  EXGUI_KEY_LEFT,
  EXGUI_KEY_DOWN,
  EXGUI_KEY_UP,
  EXGUI_KEY_PAGE_UP,
  EXGUI_KEY_PAGE_DOWN,
  EXGUI_KEY_HOME,
  EXGUI_KEY_END,
  EXGUI_KEY_CAPS_LOCK,
  EXGUI_KEY_SCROLL_LOCK,
  EXGUI_KEY_NUM_LOCK,
  EXGUI_KEY_PRINT_SCREEN,
  EXGUI_KEY_PAUSE,
  EXGUI_KEY_F1,
  EXGUI_KEY_F2,
  EXGUI_KEY_F3,
  EXGUI_KEY_F4,
  EXGUI_KEY_F5,
  EXGUI_KEY_F6,
  EXGUI_KEY_F7,
  EXGUI_KEY_F8,
  EXGUI_KEY_F9,
  EXGUI_KEY_F10,
  EXGUI_KEY_F11,
  EXGUI_KEY_F12
};

/* key state */
enum EXGUI_KEY_STATE : uint32_t {
  DOWN = 0,
  UP,
  REPEAT
};

/* mouse events */
enum EXGUI_MOUSE_EVENT : uint32_t {
  EXGUI_MOUSE_EVENT_MOVE = 0,
  EXGUI_MOUSE_EVENT_CLICK
};

/* node event */
enum EXGUI_EVENT : uint32_t {
  PARENT_CHANGED = 0,
  PARENT_RESIZE,
  PARENT_CHILD_ADDED,
  ROOT_RESIZE
};

/* Node notify and state flags */
#define EXGUI_FLAG_NONE          (0)
#define EXGUI_FLAG_VISIBLE       (1 << 0)
#define EXGUI_FLAG_ACTIVE        (1 << 1)
#define EXGUI_FLAG_NOTIFY_CHILDS (1 << 2)
#define EXGUI_FLAG_HAS_SYM       (1 << 3)
#define EXGUI_FLAG_HAS_KEYBD     (1 << 4)
#define EXGUI_FLAG_HAS_MOUSE     (1 << 5)
#define EXGUI_FLAG_HAS_CHILDS    (1 << 6)

/* special state flags */
#define EXGUI_FLAG_HOVERED       (1 << 7)
#define EXGUI_FLAG_FOCUSED       (1 << 8)
#define EXGUI_FLAG_DRAGGED       (1 << 9)

#define EXGUI_FLAG_GLOBAL        (1 << 10)
#define EXGUI_FLAG_DISABLE_SCISSOR (1 << 11)
#define EXGUI_FLAG_HIGHEST_PRIORITY      (1 << 12)

/* default flags for each widget */
#define EXGUI_FLAG_DEFAULT       (EXGUI_FLAG_VISIBLE|EXGUI_FLAG_ACTIVE|EXGUI_FLAG_NOTIFY_CHILDS|EXGUI_FLAG_HAS_SYM|EXGUI_FLAG_HAS_KEYBD|EXGUI_FLAG_HAS_MOUSE|EXGUI_FLAG_HAS_CHILDS)

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
  rmgui_flags_elem() : rmgui_flags(EXGUI_FLAG_DEFAULT) {}
  ~rmgui_flags_elem() {}

  inline bool has_visible() { return is_set(EXGUI_FLAG_VISIBLE); }
  inline bool has_active() { return is_set(EXGUI_FLAG_ACTIVE); }
  inline bool has_notify_childs() { return is_set(EXGUI_FLAG_NOTIFY_CHILDS); }
  inline bool has_symbols_input() { return is_set(EXGUI_FLAG_HAS_SYM); }
  inline bool has_keybd() { return is_set(EXGUI_FLAG_HAS_KEYBD); }
  inline bool has_mouse() { return is_set(EXGUI_FLAG_HAS_MOUSE); }
  inline bool has_childs() { return is_set(EXGUI_FLAG_HAS_CHILDS); }

  /* state flags */
  inline bool is_hovered() { return is_set(EXGUI_FLAG_HOVERED); }
  inline bool is_focused() { return is_set(EXGUI_FLAG_FOCUSED); }

  uint32_t operator=(uint32_t f) {
    m_flags = f;
    return m_flags;
  }
};

/* clipboard data types */
enum EXGUI_CB_DATA_TYPE : uint32_t {
  EXGUI_CLIPBOARD_DATA_TYPE_NONE = 0,
  EXGUI_CLIPBOARD_DATA_TYPE_BIN,
  EXGUI_CLIPBOARD_DATA_TYPE_TEXT
};

/**
* system dependend operations
*/
class irmgui_sysdf
{
public:
  //virtual                   ~irmgui_sysdf() = 0;
  virtual void               get_cursor_pos(int* p_dst_x, int* p_dst_y) = 0;
  virtual void               set_cursor_pos(int x, int h) = 0;
  virtual uint32_t           num_monitors() = 0;
  virtual void               get_monitor_info(uint32_t monitor_idx, uint32_t *p_dst_DPI, uint32_t *p_w, uint32_t *p_h) = 0;
  virtual bool               get_clipboard_data_info(EXGUI_CB_DATA_TYPE &dst_data, size_t &dst_size) = 0;
  virtual const uint8_t     *get_clipboard_data_ex(EXGUI_CB_DATA_TYPE &dst, size_t &size) =0;
  virtual void               set_clipboard_data_ex(const uint8_t* p_src, size_t size) = 0;
  virtual float              get_time() = 0;
};

class rm_widget;

/**
* GUI element abstract class
*/
class irmgui_widget
{
public:
  //virtual     ~irmgui_widget() = 0;
  virtual bool on_event(EXGUI_EVENT event, rm_widget *p_from) = 0;
  virtual void on_draw(NVGcontext* p_ctx) = 0;
  virtual void on_keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state) = 0;

  /**
  * @brief Text input handler
  * @param sym - symbol key code
  * @return Nothing
  */
  virtual void on_text_input(int sym) = 0;

  /**
  * @brief Mouse event handler
  * @param event - received mouse event (EXGUI_MOUSE_EVENT_MOVE or EXGUI_MOUSE_EVENT_CLICK)
  * @param vk - received virtual key
  * @param state - received key state (DOWN, UP or REPEAT)
  * @param cursor_pos - received current cursor pos
  * @return To block further propagation of the event, return false. If ture is returned, the event is propagated to the following elements.
  */
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) = 0;
};

/**
* rendering utilites
*/
class rmgui_utl
{
protected:

  /**
  * draw_border_frame
  * 
  * draws border frame
  * 
  * modes:
  * RM_BFRM_MODE_RAISED
  * RM_BFRM_MODE_SUNKEN
  * 
  * 
  * 
  */
  enum {
    RM_BFRM_MODE_SUNKEN = 0,
    RM_BFRM_MODE_RAISED
  };
  void draw_border_frame(NVGcontext* ctx, rm_rect& rect, uint32_t mode, const NVGcolor colors[]);
};

/**
* rmgui_widget
* 
* base class for all GUI elements
*/
class rm_surface;

class rm_widget : protected irmgui_widget
{
  /* allow rmgui_root class to call irmgui_element vmethods */
  friend class rm_surface;
  inline void set_root(rm_surface* p_root) { m_proot = p_root; }

protected:
  /* irmgui_element empty impls */
  virtual bool on_event(EXGUI_EVENT event, rm_widget *p_from) {
    RMGUI_UNUSED(event);
    RMGUI_UNUSED(p_from);
    return true;
  }
  virtual void on_draw(NVGcontext* p_ctx) {
    RMGUI_UNUSED(p_ctx);
  }
  virtual void on_keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state) {
    RMGUI_UNUSED(sc);
    RMGUI_UNUSED(vk);
    RMGUI_UNUSED(state);
  }
  virtual void on_text_input(int sym) {
    RMGUI_UNUSED(sym);
  }
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos) {
    RMGUI_UNUSED(event);
    RMGUI_UNUSED(vk);
    RMGUI_UNUSED(state);
    RMGUI_UNUSED(cursor_pos);
    return true;
  }

protected:
  using _childs_vec = std::vector<rm_widget*>;
  _childs_vec      m_childs;
  rm_surface      *m_proot;
  rm_widget       *m_pparent;
  void            *m_puserptr;
  irmgui_sysdf    *m_psysdf;
  rmgui_flags_elem m_elem_flags;
  uint32_t         m_user_flags;
  rm_font          m_font;
  char             m_szclass[32];
  rm_bbox          m_bbox;
  rm_rect          m_relative;
  rm_rect          m_absolute;
  int              m_zindex;

  inline rm_surface* get_root() { return m_proot; }

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

  static void move_recursive(rm_widget *pwidget, float newx, float newy);

public:
  void set_classname(const char* p_clsn) {
    strncpy(m_szclass, p_clsn, sizeof(m_szclass) - 1);
  }

  void grab_globals_from(rm_widget* p_parent) {
    /* get sysdf ifaec from parent */
    m_psysdf = m_pparent->get_sysdf();
    /* get root m_pparent from parent */
    m_proot = m_pparent->get_root();
  }

  rm_widget(int x, int y, int width, int height, rm_widget *p_parent, const char *p_classname,
    uint32_t flags = EXGUI_FLAG_DEFAULT, 
    uint32_t uflags = 0, void *p_userptr = nullptr) : m_proot(nullptr),
    m_pparent(p_parent), m_puserptr(p_userptr), m_psysdf(nullptr), m_zindex(0) {
    m_elem_flags = flags;
    m_user_flags = uflags;
    m_absolute = rm_rect(x, y, width, height);
    m_relative = rm_rect(0, 0, width, height);
    m_bbox.from_rect(m_absolute);

    if (m_pparent) {
      m_pparent->add_child(this);
      grab_globals_from(m_pparent);
    }

    /* set font from root */
    if (m_proot)
      set_font(((rm_widget *)m_proot)->get_font());

    set_classname(p_classname);
  }
  ~rm_widget() {}

  inline const char    *get_classname() { return m_szclass; }
  inline void          *get_userptr() { return m_puserptr; }
  inline void           set_userptr(void* p) { m_puserptr = p; }

  /* rect && bbox */
  inline rm_bbox       &get_bbox() { return m_bbox; }
  inline rm_rect       &get_relative() { return m_relative; }
  inline rm_rect       &get_absolute() { return m_absolute; }

  /* visual */
  inline bool           is_visible() { return m_elem_flags.has_visible(); }
  inline void           show(bool b_show = true) { m_elem_flags.toggle_bits(EXGUI_FLAG_VISIBLE, b_show); }
  inline void           hide() { show(false); }

  /* childs */
  inline size_t         get_num_childs() { return m_childs.size(); }
  inline rm_widget*  get_child(size_t idx) { return m_childs[idx]; }
  inline rm_widget** get_all_childs() { return m_childs.data(); }
  bool                  add_child(rm_widget* p_child);
  bool                  remove_child(rm_widget* p_child);

  /* parent */
  inline rm_widget  *get_parent() { return m_pparent; }
  void                  set_parent(rm_widget* p_parent);

  /* flags */
  inline rmgui_flags_elem get_elem_flags() { return m_elem_flags; }
  inline uint32_t       get_user_flags() { return m_user_flags; }

  /* system dependend functions interface */
  inline irmgui_sysdf  *get_sysdf() { return m_psysdf; }

  /* font */
  inline void           set_font(rm_font font) { m_font = font; }
  inline rm_font        get_font() { return m_font; }

  /* layers */
  inline void           set_zindex(int zidx) { m_zindex = zidx; }
  inline int            get_zindex() const { return m_zindex; }

  void move(int newx, int newy)  {
    move_recursive(this, newx, newy);
  }
};

struct layer_draw_cache {
  int zindex;
  std::vector<rm_widget*> m_cache;
  layer_draw_cache(int zidx) : zindex(zidx) {}

  operator int() const { return zindex; }
  inline int get_zindex() const { return zindex; }

  void add_widget(rm_widget* pwidget) {
    m_cache.push_back(pwidget);
  }

  void clear() {
    m_cache.clear();
  }

  inline size_t size() const { return m_cache.size(); }
  inline rm_widget *get_widget(size_t idx) {
    assert(idx < m_cache.size() && "idx out of bounds");
    return m_cache[idx];
  }
};

class rm_surface : public rm_widget, rm_object_accrssor
{
  NVGcontext                   *m_pctx;
  rm_widget                    *m_pfocus;
  std::vector<layer_draw_cache *> m_layers;

  void build_draw_cache_recursive(rm_widget *p_elem);

  /* event notifier functions */
  static void event_dispatcher(rm_widget *p_elem);
  static void keybd_dispatcher(rm_widget *p_elem, int sc, 
    EXGUI_KEY vk, EXGUI_KEY_STATE state);
#if 0
  static void text_input_dispatcher(rmgui_widget *p_elem, int sym);
#endif
  bool mouse_dispatcher(rm_widget *p_elem,
    EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, 
    EXGUI_KEY_STATE state, rmgui_vector2 &cursor_pos);

  /* access is open for inheritance (rmgui_root::rebuild_draw_cache accessor class ) */
//protected:
public:
  void rebuild_draw_cache();
  layer_draw_cache* get_layer_by_zid(int zid);

public:
  rm_surface(NVGcontext *p_ctx, int width, int height, irmgui_sysdf *p_sysdf);
  ~rm_surface();

  /* main events */
  void draw();
  void keybd(int sc, EXGUI_KEY vk, EXGUI_KEY_STATE state);
  void textinput(int sym);
  void mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, int x, int y);
  void resize(int width, int height);

  NVGcontext* get_context() { return m_pctx; }

  /* images */
  rm_image load_image_from_memory(const void *psrc, size_t srclen, int flags);
  rm_image load_image(const char *pfilename, int flags);
  void     free_image(rm_image& image);

  /* fonts */
  rm_font  load_font_from_memory(const void* psrc_ttf_mem, size_t srclen, const char* pfontname);
  rm_font  load_font(const char* pfilename, const char *pfontname);
  rm_font  find_font(const char* pfontname);
  void     free_font(rm_font& font);
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

  bool has_elapsed(irmgui_sysdf* p_sysdf);
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
* style base class
*/
class rm_style_base
{
public:
  rm_style_base() {}
  ~rm_style_base() {}
};

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

enum EXGUI_CORNER : uint32_t {
  LEFT_TOP = 0,
  RIGHT_TOP,
  RIGHT_BOTTOM,
  LEFT_BOTTOM,

  EXGUI_NUM_CORNERS
};

class rm_corners_style
{
protected:
  float m_corner_radius[EXGUI_NUM_CORNERS];
public:
  rm_corners_style() {
    m_corner_radius[LEFT_TOP] = 0.f;
    m_corner_radius[RIGHT_TOP] = 0.f;
    m_corner_radius[RIGHT_BOTTOM] = 0.f;
    m_corner_radius[LEFT_BOTTOM] = 0.f;
  }
  ~rm_corners_style() {}

  inline void  set_corner_radius(EXGUI_CORNER corner, float radius) { m_corner_radius[corner] = radius; }
  inline float get_corner_radius(EXGUI_CORNER corner) { return m_corner_radius[corner]; }
};


/**
* Window
*/
class rm_window_style : public rm_style_base, public rm_corners_style
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
  rm_window_style() {}
  ~rm_window_style() {}

  inline float     get_font_size() { return m_title_font_size; }
  inline float     get_font_blur_factor() { return m_title_font_blur_factor; }
  inline NVGcolor &get_font_color() { return m_title_font_color; }
  inline NVGcolor &get_font_shadow_color() { return m_title_font_shadow_color; }
  inline NVGcolor &get_active_background_color() { return m_active_background_color; }
  inline NVGcolor &get_inactive_background_color() { return m_inactive_background_color; }
  inline NVGcolor &get_background_color(int idx) { 
    assert(idx < EXGUI_COUNTOF(m_background_colors) && "index out of bounds");
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

class rm_window : public rm_widget, public rm_styled<rm_window_style>
{
  rm_window_style        *m_pstyle;
  std::vector<rm_widget*> m_top_widgets;

  /* paint window background */
  virtual void on_draw(NVGcontext* p_ctx);
  virtual bool on_mouse(EXGUI_MOUSE_EVENT event, EXGUI_KEY vk, EXGUI_KEY_STATE state, rmgui_vector2& cursor_pos);

public:
  rm_window(rm_widget* p_parent, int x, int y, int width, int height, uint32_t flags = EXGUI_FLAG_DEFAULT, uint32_t uflags = 0, void* p_userptr = nullptr);
  ~rm_window();
};