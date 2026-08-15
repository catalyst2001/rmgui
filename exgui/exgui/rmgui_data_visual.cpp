#include "rmgui_data_visual.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t RM_DRAW_PROGRAM_VERSION = 1;

const NVGcmdVar g_surface_vars[] = {
  NVG_VAR_ENTRY(RmDrawSurfaceData, x, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, y, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, width, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, height, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, right, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, bottom, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, center_x, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, center_y, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_x, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_y, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_width, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_height, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_right, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, inner_bottom, NVG_VAR_FLOAT),
  NVG_VAR_ENTRY(RmDrawSurfaceData, state, NVG_VAR_UINT32)
};

constexpr const char* g_surface_var_names[] = {
  "x", "y", "width", "height", "right", "bottom", "center_x",
  "center_y", "inner_x", "inner_y", "inner_width", "inner_height",
  "inner_right", "inner_bottom", "state"
};

constexpr NVGcmdVarType g_surface_var_types[] = {
  NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT,
  NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT,
  NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT, NVG_VAR_FLOAT,
  NVG_VAR_FLOAT, NVG_VAR_FLOAT,
  NVG_VAR_UINT32
};

const char* target_class_name(RmDrawProgramTarget target) noexcept
{
  switch (target) {
  case RmDrawProgramTarget::button_surface: return "rm_button_surface";
  case RmDrawProgramTarget::tab_surface: return "rm_tab_surface";
  default: return "";
  }
}

void set_error(std::string* p_error, const char* message)
{
  if (p_error)
    *p_error = message ? message : "";
}

bool has_safe_embedded_commands(const NVGcmdBuf& program,
  std::string* p_error)
{
  const uint8_t* argument_counts = nvgCmdArgCount();
  uint32_t cursor = 0;
  while (cursor < program.cells.size()) {
    const uint32_t op = program.cells[cursor++].u;
    if (op == NVG_CMD_RESET || op == NVG_CMD_RESET_TRANSFORM ||
      op == NVG_CMD_SCISSOR || op == NVG_CMD_RESET_SCISSOR ||
      op == NVG_CMD_SET_ZINDEX) {
      set_error(p_error,
        "draw program contains a command which can escape widget rendering state");
      return false;
    }
    const uint32_t argument_count = argument_counts[op];
    if (argument_count > 0)
      ++cursor;
    cursor += argument_count;
  }
  return true;
}

bool validate_surface_schema(const NVGcmdBuf& program,
  RmDrawProgramTarget target, std::string* p_error)
{
  if (program.meta.version != RM_DRAW_PROGRAM_VERSION) {
    set_error(p_error, "unsupported draw program format version");
    return false;
  }
  if (std::strcmp(program.meta.className, target_class_name(target)) != 0) {
    set_error(p_error, "draw program class does not match its control target");
    return false;
  }
  if (program.meta.elementName[0] == '\0') {
    set_error(p_error, "draw program has no element name");
    return false;
  }
  constexpr size_t expected_count = sizeof(g_surface_var_names) /
    sizeof(g_surface_var_names[0]);
  if (program.meta.vars.size() != expected_count) {
    set_error(p_error, "draw program does not use the canonical surface schema");
    return false;
  }
  for (size_t index = 0; index < expected_count; ++index) {
    if (std::strcmp(program.meta.vars[index].name,
        g_surface_var_names[index]) != 0 ||
      program.meta.vars[index].type != g_surface_var_types[index]) {
      set_error(p_error,
        "draw program variable order or type differs from the surface schema");
      return false;
    }
  }

  const NVGcmdValidationResult validation = nvgCmdValidate(program,
    &RmDataDrivenPainter::surface_layout());
  if (!validation) {
    if (p_error)
      *p_error = validation.message;
    return false;
  }
  return has_safe_embedded_commands(program, p_error);
}

class RmReadOnlyFileIo final : public NVGio {
  std::vector<uint8_t> m_bytes;
  size_t m_cursor = 0;

public:
  explicit RmReadOnlyFileIo(const char* pfilename)
  {
    std::ifstream stream(pfilename, std::ios::binary);
    if (stream)
      m_bytes.assign(std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>());
  }

  bool valid() const noexcept { return !m_bytes.empty(); }
  bool write(const void*, size_t) override { return false; }
  size_t read(void* data, size_t size) override
  {
    const size_t available = m_cursor < m_bytes.size()
      ? m_bytes.size() - m_cursor : 0;
    const size_t count = std::min(size, available);
    if (count > 0) {
      std::memcpy(data, m_bytes.data() + m_cursor, count);
      m_cursor += count;
    }
    return count;
  }
};

} // namespace

RmDrawSurfaceData RmDrawSurfaceData::from_bounds(float x, float y,
  float width, float height, RmDrawVisualState visual_state) noexcept
{
  RmDrawSurfaceData result;
  result.x = x;
  result.y = y;
  result.width = std::max(0.0f, width);
  result.height = std::max(0.0f, height);
  result.right = x + result.width;
  result.bottom = y + result.height;
  result.center_x = x + result.width * 0.5f;
  result.center_y = y + result.height * 0.5f;
  result.inner_x = x + 0.5f;
  result.inner_y = y + 0.5f;
  result.inner_width = std::max(0.0f, result.width - 1.0f);
  result.inner_height = std::max(0.0f, result.height - 1.0f);
  result.inner_right = result.inner_x + result.inner_width;
  result.inner_bottom = result.inner_y + result.inner_height;
  result.state = static_cast<uint32_t>(visual_state);
  return result;
}

rm_draw_program::rm_draw_program(rm_resource_id resource_id,
  std::string name, RmDrawProgramTarget target, NVGcmdBuf program) noexcept
  : m_resource_id(resource_id), m_name(std::move(name)), m_target(target),
  m_program(std::move(program))
{
}

RmDrawProgramResourceSnapshot rm_draw_program::snapshot() const
{
  return { m_resource_id, m_name, m_target, m_program.meta.elementName,
    m_program.meta.version };
}

const NVGcmdLayout& RmDataDrivenPainter::surface_layout() noexcept
{
  static const NVGcmdLayout layout{
    g_surface_vars,
    static_cast<uint32_t>(sizeof(g_surface_vars) / sizeof(g_surface_vars[0]))
  };
  return layout;
}

RmDrawVisualState RmDataDrivenPainter::button_state(bool enabled,
  bool hovered, bool pressed) noexcept
{
  if (!enabled) return RmDrawVisualState::disabled;
  if (pressed) return RmDrawVisualState::pressed;
  if (hovered) return RmDrawVisualState::hovered;
  return RmDrawVisualState::normal;
}

RmDrawVisualState RmDataDrivenPainter::tab_state(bool enabled, bool hovered,
  bool pressed, bool selected) noexcept
{
  if (selected) {
    if (!enabled) return RmDrawVisualState::selected_disabled;
    if (pressed) return RmDrawVisualState::selected_pressed;
    if (hovered) return RmDrawVisualState::selected_hovered;
    return RmDrawVisualState::selected;
  }
  return button_state(enabled, hovered, pressed);
}

bool RmDataDrivenPainter::draw_surface(NVGcontext& context,
  const rm_draw_program& program, RmDrawProgramTarget expected_target,
  const RmDrawSurfaceData& data) noexcept
{
  if (program.m_target != expected_target || data.width <= 0.0f ||
    data.height <= 0.0f)
    return false;

  context.save();
  context.intersectScissor(data.x, data.y, data.width, data.height);
  nvgEvalValidated(context, program.m_program, &data, &surface_layout());
  context.restore();
  return true;
}

rm_resource_id rm_surface::register_draw_program(const char* p_resource_name,
  RmDrawProgramTarget target, NVGcmdBuf program, std::string* p_error)
{
  if (!p_resource_name || !*p_resource_name) {
    set_error(p_error, "draw program resource name is empty");
    return RM_INVALID_RESOURCE_ID;
  }
  if (find_draw_program(p_resource_name) != RM_INVALID_RESOURCE_ID) {
    set_error(p_error, "draw program resource name is already registered");
    return RM_INVALID_RESOURCE_ID;
  }
  if (!validate_surface_schema(program, target, p_error))
    return RM_INVALID_RESOURCE_ID;

  const rm_resource_id resource_id =
    static_cast<rm_resource_id>(m_draw_programs.size() + 1);
  std::unique_ptr<rm_draw_program> resource(new rm_draw_program(resource_id,
    p_resource_name, target, std::move(program)));
  m_draw_programs.push_back(std::move(resource));
  m_draw_program_names.emplace(p_resource_name, resource_id);
  ++m_resource_revision;
  set_error(p_error, "");
  return resource_id;
}

rm_resource_id rm_surface::register_draw_program_text(
  const char* p_resource_name, RmDrawProgramTarget target,
  const char* pfilename, std::string* p_error)
{
  if (!pfilename || !*pfilename) {
    set_error(p_error, "draw program filename is empty");
    return RM_INVALID_RESOURCE_ID;
  }
  RmReadOnlyFileIo io(pfilename);
  if (!io.valid()) {
    set_error(p_error, "draw program file could not be read");
    return RM_INVALID_RESOURCE_ID;
  }
  NVGcmdBuf program;
  if (!program.loadText(io)) {
    set_error(p_error, "draw program text is malformed");
    return RM_INVALID_RESOURCE_ID;
  }
  return register_draw_program(p_resource_name, target, std::move(program),
    p_error);
}

const rm_draw_program* rm_surface::resolve_draw_program(
  rm_resource_id resource_id) const noexcept
{
  if (resource_id == RM_INVALID_RESOURCE_ID)
    return nullptr;
  const size_t index = static_cast<size_t>(resource_id - 1);
  return index < m_draw_programs.size() ? m_draw_programs[index].get() : nullptr;
}

rm_resource_id rm_surface::find_draw_program(
  std::string_view resource_name) const noexcept
{
  for (const auto& entry : m_draw_program_names)
    if (entry.first == resource_name)
      return entry.second;
  return RM_INVALID_RESOURCE_ID;
}
