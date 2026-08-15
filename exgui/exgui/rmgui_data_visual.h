#pragma once

#include "rmgui.h"
#include "nvg_cmd.h"

#include <cstdint>
#include <string>

enum class RmDrawVisualState : uint32_t {
  normal = 0,
  hovered,
  pressed,
  disabled,
  selected,
  selected_hovered,
  selected_pressed,
  selected_disabled,
  count
};

/** Canonical geometry contract shared by runtime controls and the designer. */
struct RmDrawSurfaceData {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;
  float center_x = 0.0f;
  float center_y = 0.0f;
  float inner_x = 0.0f;
  float inner_y = 0.0f;
  float inner_width = 0.0f;
  float inner_height = 0.0f;
  float inner_right = 0.0f;
  float inner_bottom = 0.0f;
  uint32_t state = 0;

  static RmDrawSurfaceData from_bounds(float x, float y, float width,
    float height, RmDrawVisualState state) noexcept;
};

/** Immutable, validated drawing resource. Controls only keep its resource ID. */
class rm_draw_program final {
  friend class rm_surface;
  friend class RmDataDrivenPainter;

  rm_resource_id m_resource_id;
  std::string m_name;
  RmDrawProgramTarget m_target;
  NVGcmdBuf m_program;

  rm_draw_program(rm_resource_id resource_id, std::string name,
    RmDrawProgramTarget target, NVGcmdBuf program) noexcept;

public:
  rm_draw_program(const rm_draw_program&) = delete;
  rm_draw_program& operator=(const rm_draw_program&) = delete;

  rm_resource_id get_resource_id() const noexcept { return m_resource_id; }
  const std::string& get_name() const noexcept { return m_name; }
  RmDrawProgramTarget get_target() const noexcept { return m_target; }
  const char* get_element_name() const noexcept {
    return m_program.meta.elementName;
  }
  uint32_t get_format_version() const noexcept { return m_program.meta.version; }
  RmDrawProgramResourceSnapshot snapshot() const;
};

class RmDataDrivenPainter final {
public:
  static const NVGcmdLayout& surface_layout() noexcept;
  static RmDrawVisualState button_state(bool enabled, bool hovered,
    bool pressed) noexcept;
  static RmDrawVisualState tab_state(bool enabled, bool hovered,
    bool pressed, bool selected) noexcept;
  static bool draw_surface(NVGcontext& context,
    const rm_draw_program& program, RmDrawProgramTarget expected_target,
    const RmDrawSurfaceData& data) noexcept;
};
