#include "rmgui_data_visual.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t RM_DRAW_PROGRAM_VERSION = 1;

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
  for (const NVGcmdInstruction& instruction : program.commands) {
    const uint32_t op = instruction.op;
    if (op == NVG_CMD_RESET || op == NVG_CMD_RESET_TRANSFORM ||
      op == NVG_CMD_SCISSOR || op == NVG_CMD_RESET_SCISSOR ||
      op == NVG_CMD_SET_ZINDEX) {
      set_error(p_error,
        "draw program contains a command which can escape widget rendering state");
      return false;
    }
  }
  return true;
}

bool validate_draw_program(const NVGcmdBuf& program,
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
  const NVGcmdValidationResult validation = nvgCmdValidate(program);
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

bool RmDrawVariableSchema::add_variable(std::string name,
  NVGcmdVarType type)
{
  const uint32_t size = nvgCmdVarTypeSize(type);
  if (name.empty() || size == 0 || find_variable(name) >= 0)
    return false;
  RmDrawVariableDesc variable;
  variable.name = std::move(name);
  variable.type = type;
  variable.offset = m_data_size;
  variable.size = size;
  m_variables.push_back(std::move(variable));
  m_data_size += size;
  ++m_revision;
  if (m_revision == 0) m_revision = 1;
  return true;
}

bool RmDrawVariableSchema::include(const RmDrawVariableSchema& schema)
{
  for (uint32_t index = 0; index < schema.get_num_variables(); ++index) {
    const RmDrawVariableDesc* variable = schema.get_variable(index);
    if (!variable || !add_variable(variable->name, variable->type))
      return false;
  }
  return true;
}

int RmDrawVariableSchema::find_variable(std::string_view name) const noexcept
{
  for (size_t index = 0; index < m_variables.size(); ++index)
    if (m_variables[index].name == name)
      return static_cast<int>(index);
  return -1;
}

const RmDrawVariableDesc* RmDrawVariableSchema::get_variable(
  uint32_t index) const noexcept
{
  return index < m_variables.size() ? &m_variables[index] : nullptr;
}

void RmDrawVariableBlock::reset(const RmDrawVariableSchema& schema)
{
  if (m_pschema == &schema && m_schema_revision == schema.get_revision())
    return;
  m_pschema = &schema;
  m_schema_revision = schema.get_revision();
  m_data.assign(schema.get_data_size(), uint8_t{});
}

bool RmDrawVariableBlock::set_value(uint32_t index, NVGcmdVarType type,
  const void* p_value, uint32_t size) noexcept
{
  if (!m_pschema || !p_value)
    return false;
  const RmDrawVariableDesc* variable = m_pschema->get_variable(index);
  if (!variable || variable->type != type || variable->size != size ||
    variable->offset > m_data.size() ||
    size > m_data.size() - variable->offset)
    return false;
  std::memcpy(m_data.data() + variable->offset, p_value, size);
  return true;
}

bool RmDrawVariableBlock::set_value(std::string_view name,
  NVGcmdVarType type, const void* p_value, uint32_t size) noexcept
{
  if (!m_pschema)
    return false;
  const int index = m_pschema->find_variable(name);
  return index >= 0 && set_value(static_cast<uint32_t>(index), type,
    p_value, size);
}

void RmDrawProgramBinding::reset() noexcept
{
  m_program_id = RM_INVALID_RESOURCE_ID;
  m_pschema = nullptr;
  m_schema_revision = 0;
  m_layout_variables.clear();
  m_arguments.clear();
  m_valid = false;
  m_error.clear();
}

bool RmDrawProgramBinding::matches(rm_resource_id program_id,
  const RmDrawVariableSchema& schema) const noexcept
{
  return m_program_id == program_id && m_pschema == &schema &&
    m_schema_revision == schema.get_revision();
}

bool RmDrawProgramBinding::compile(rm_resource_id program_id,
  const NVGcmdBuf& program, const RmDrawVariableSchema& schema,
  std::string* p_error)
{
  reset();
  m_program_id = program_id;
  m_pschema = &schema;
  m_schema_revision = schema.get_revision();
  const auto fail = [this, p_error](std::string message) {
    m_error = std::move(message);
    if (p_error) *p_error = m_error;
    return false;
  };
  std::vector<NVGcmdVar> variables;
  variables.reserve(program.meta.vars.size());
  for (const NVGcmdVarInfo& requested : program.meta.vars) {
    const int index = schema.find_variable(requested.name);
    if (index < 0)
      return fail(std::string("widget does not provide draw variable '") +
        requested.name + "'");
    const RmDrawVariableDesc* supplied = schema.get_variable(
      static_cast<uint32_t>(index));
    if (!supplied || supplied->type != requested.type)
      return fail(std::string("draw variable type mismatch for '") +
        requested.name + "'");
    variables.push_back({ supplied->offset, supplied->type, supplied->size });
  }

  const NVGcmdLayout layout{ variables.data(),
    static_cast<uint32_t>(variables.size()), schema.get_data_size() };
  const NVGcmdValidationResult validation = nvgCmdValidate(program, &layout);
  if (!validation)
    return fail(validation.message);
  m_layout_variables = std::move(variables);
  m_valid = true;
  m_error.clear();
  set_error(p_error, "");
  return true;
}

NVGcmdLayout RmDrawProgramBinding::get_layout() const noexcept
{
  return { m_layout_variables.data(),
    static_cast<uint32_t>(m_layout_variables.size()),
    m_pschema ? m_pschema->get_data_size() : 0 };
}

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

void RmDrawSurfaceData::write_to(RmDrawVariableBlock& variables) const noexcept
{
  variables.set_float("x", x);
  variables.set_float("y", y);
  variables.set_float("width", width);
  variables.set_float("height", height);
  variables.set_float("right", right);
  variables.set_float("bottom", bottom);
  variables.set_float("center_x", center_x);
  variables.set_float("center_y", center_y);
  variables.set_float("inner_x", inner_x);
  variables.set_float("inner_y", inner_y);
  variables.set_float("inner_width", inner_width);
  variables.set_float("inner_height", inner_height);
  variables.set_float("inner_right", inner_right);
  variables.set_float("inner_bottom", inner_bottom);
  variables.set_uint32("state", state);
}

const RmDrawVariableSchema& rm_widget::base_draw_variable_schema()
{
  static const RmDrawVariableSchema schema = [] {
    RmDrawVariableSchema result;
    const char* float_variables[] = {
      "x", "y", "width", "height", "right", "bottom", "center_x",
      "center_y", "inner_x", "inner_y", "inner_width", "inner_height",
      "inner_right", "inner_bottom", "widget_x", "widget_y",
      "content_width", "content_height", "viewport_width",
      "viewport_height", "content_offset_x", "content_offset_y"
    };
    for (const char* name : float_variables)
      result.add_variable(name, NVG_VAR_FLOAT);
    result.add_variable("state", NVG_VAR_UINT32);
    result.add_variable("enabled", NVG_VAR_UINT32);
    result.add_variable("focused", NVG_VAR_UINT32);
    result.add_variable("hovered", NVG_VAR_UINT32);
    result.add_variable("z_index", NVG_VAR_INT32);
    return result;
  }();
  return schema;
}

const RmDrawVariableSchema& rm_widget::get_draw_variable_schema() const
{
  return base_draw_variable_schema();
}

RmDrawVariableBlock& rm_widget::prepare_draw_variables(float x, float y,
  float width, float height, uint32_t state) const
{
  const RmDrawVariableSchema& schema = get_draw_variable_schema();
  m_draw_variable_block.reset(schema);
  const RmDrawSurfaceData surface = RmDrawSurfaceData::from_bounds(x, y,
    width, height, static_cast<RmDrawVisualState>(state));
  surface.write_to(m_draw_variable_block);
  m_draw_variable_block.set_float("widget_x", m_pos_of_parent.x);
  m_draw_variable_block.set_float("widget_y", m_pos_of_parent.y);
  m_draw_variable_block.set_float("content_width", m_content_extent.x);
  m_draw_variable_block.set_float("content_height", m_content_extent.y);
  m_draw_variable_block.set_float("viewport_width", m_content_area.width);
  m_draw_variable_block.set_float("viewport_height", m_content_area.height);
  m_draw_variable_block.set_float("content_offset_x", m_content_offset.x);
  m_draw_variable_block.set_float("content_offset_y", m_content_offset.y);
  m_draw_variable_block.set_uint32("enabled",
    m_elem_flags.has_active() ? 1u : 0u);
  m_draw_variable_block.set_uint32("focused",
    m_elem_flags.is_focused() ? 1u : 0u);
  m_draw_variable_block.set_uint32("hovered",
    m_elem_flags.is_hovered() ? 1u : 0u);
  m_draw_variable_block.set_int32("z_index", m_zindex);
  update_draw_variables(m_draw_variable_block);
  return m_draw_variable_block;
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
  const RmDrawSurfaceData& data, const RmDrawVariableBlock& variables,
  RmDrawProgramBinding& binding, std::string* p_error)
{
  if (program.m_target != expected_target || data.width <= 0.0f ||
    data.height <= 0.0f || !variables.get_schema())
    return false;

  if (!binding.matches(program.m_resource_id, *variables.get_schema()))
    binding.compile(program.m_resource_id, program.m_program,
      *variables.get_schema(), p_error);
  if (!binding.is_valid()) {
    if (p_error) *p_error = binding.get_error();
    return false;
  }

  context.save();
  context.intersectScissor(data.x, data.y, data.width, data.height);
  const NVGcmdLayout layout = binding.get_layout();
  if (!nvgCmdResolveArguments(binding.get_arguments(), program.m_program,
    variables.get_data(), &layout)) {
    context.restore();
    set_error(p_error, "draw program arguments could not be resolved");
    return false;
  }
  nvgEvalResolved(context, program.m_program, binding.get_arguments());
  context.restore();
  set_error(p_error, "");
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
  if (!validate_draw_program(program, target, p_error))
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
