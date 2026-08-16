#include "nanovg.h"
#include "nvg_cmd.h"
#include "../exgui/exgui/rmgui_theme.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <cstring>
#include <iterator>

namespace {

void require(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

class RecordingRenderer final : public NVGrenderer {
public:
  int viewport_calls = 0;
  int flush_calls = 0;
  int fill_calls = 0;
  int stroke_calls = 0;
  int triangle_calls = 0;
  int custom_draw_calls = 0;
  int last_z_index = 0;
  NVGcustomDraw last_draw{};
  std::vector<NVGvertex> last_vertices;
  std::vector<NVGvertex> last_fill_vertices;
  std::vector<NVGscissor> fill_scissors;

  NVGhandle createTexture(int, int, int, int, const unsigned char*) override
  {
    return NVGhandle(static_cast<NVGhandle::_handle_type>(1));
  }
  int deleteTexture(NVGhandle) override { return 1; }
  int updateTexture(NVGhandle, int, int, int, int, const unsigned char*) override { return 1; }
  int getTextureSize(NVGhandle, int* width, int* height) override
  {
    if (width) *width = 512;
    if (height) *height = 512;
    return 1;
  }
  void viewport(float, float, float) override { ++viewport_calls; }
  void cancel() override {}
  void flush() override { ++flush_calls; }
  void fill(const NVGpaint&, NVGcompositeOperationState,
    const NVGscissor& scissor, float,
    const float*, const NVGpath* paths, int path_count) override
  {
    ++fill_calls;
    fill_scissors.push_back(scissor);
    last_fill_vertices.clear();
    for (int i = 0; i < path_count; ++i) {
      if (paths[i].fill && paths[i].nfill > 0)
        last_fill_vertices.insert(last_fill_vertices.end(), paths[i].fill,
          paths[i].fill + paths[i].nfill);
    }
  }
  void stroke(const NVGpaint&, NVGcompositeOperationState, const NVGscissor&, float,
    float, const NVGpath*, int) override { ++stroke_calls; }
  void triangles(const NVGpaint&, NVGcompositeOperationState, const NVGscissor&,
    const NVGvertex*, int, float) override { ++triangle_calls; }
  NVGhandle createRenderTarget(const NVGrenderTargetDesc&) override { return {}; }
  bool deleteRenderTarget(NVGhandle) override { return false; }
  void setRenderTarget(NVGhandle) override {}
  NVGhandle getRenderTargetImage(NVGhandle) override { return {}; }
  NVGhandle createShader(const NVGshaderDesc&) override { return {}; }
  void deleteShader(NVGhandle) override {}
  NVGhandle getShader() override { return {}; }
  void setShader(NVGhandle) override {}
  NVGhandle createUniform(NVGhandle, const char*, NVGuniformDataType, uint32_t) override { return {}; }
  NVGhandle findUniform(NVGhandle, const char*) override { return {}; }
  void deleteUniform(NVGhandle) override {}
  void setUniformData(NVGhandle, const void*, size_t) override {}
  void setZIndex(int z) override { last_z_index = z; }

  void drawCustomTriangles(const NVGcustomDraw& draw,
    NVGcompositeOperationState, const NVGscissor&, const NVGvertex* vertices,
    int vertex_count, float) override
  {
    ++custom_draw_calls;
    last_draw = draw;
    last_vertices.assign(vertices, vertices + vertex_count);
  }
};

class InspectableContext final : public NVGcontext {
public:
  using NVGcontext::NVGcontext;

  size_t path_command_count() const { return m_pathCommands.getSize(); }
  size_t path_argument_count() const { return m_pathArguments.getSize(); }
  const NVGpathCommand* path_commands() const { return m_pathCommands.getData(); }
  const float* path_arguments() const { return m_pathArguments.getData(); }
};

class MemoryIo final : public NVGio {
public:
  std::vector<uint8_t> bytes;
  size_t cursor = 0;

  bool write(const void* data, size_t size) override
  {
    if (!data && size != 0) return false;
    if (size == 0) return true;
    const uint8_t* begin = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), begin, begin + size);
    return true;
  }

  size_t read(void* data, size_t size) override
  {
    const size_t available = cursor < bytes.size() ? bytes.size() - cursor : 0;
    const size_t count = std::min(size, available);
    if (count > 0) {
      std::memcpy(data, bytes.data() + cursor, count);
      cursor += count;
    }
    return count;
  }
};

NVGcmdCell float_cell(float value)
{
  NVGcmdCell cell{};
  cell.f = value;
  return cell;
}

void test_command_buffer_runtime()
{
  struct DrawData {
    float width;
    float height;
    uint32_t state;
    int32_t z_index;
  } data{ 84.0f, 32.0f, 1u, 23 };

  const NVGcmdVar variables[] = {
    NVG_VAR_ENTRY(DrawData, width, NVG_VAR_FLOAT),
    NVG_VAR_ENTRY(DrawData, height, NVG_VAR_FLOAT),
    NVG_VAR_ENTRY(DrawData, state, NVG_VAR_UINT32),
    NVG_VAR_ENTRY(DrawData, z_index, NVG_VAR_INT32)
  };
  const NVGcmdLayout layout{ variables,
    static_cast<uint32_t>(std::size(variables)), sizeof(DrawData) };

  NVGcmdBuf commands;
  nvgCmdCopyName(commands.meta.className, NVG_CMD_MAX_NAME, "rm_button");
  nvgCmdCopyName(commands.meta.elementName, NVG_CMD_MAX_NAME, "strict_button");
  commands.meta.version = 1;
  commands.meta.addVar("width", NVG_VAR_FLOAT);
  commands.meta.addVar("height", NVG_VAR_FLOAT);
  commands.meta.addVar("state", NVG_VAR_UINT32);
  commands.meta.addVar("z_index", NVG_VAR_INT32);
  const uint32_t red = commands.addProperty("red", NVG_VAR_FLOAT, 2,
    { float_cell(0.2f), float_cell(0.8f) });
  const uint32_t green = commands.addProperty("green", NVG_VAR_FLOAT,
    float_cell(0.35f));

  commands.save();
  commands.setZIndex(V(3));
  commands.beginPath();
  commands.roundedRect(0.0f, 0.0f, V(0), V(1), 3.0f);
  commands.fillColor(P(red), P(green), 0.15f, 1.0f);
  commands.fill();
  commands.restore();

  require(nvgCmdValidate(commands, &layout).succeeded(),
    "a typed data-driven command buffer must validate");
  require(reinterpret_cast<const void*>(commands.data()) !=
    reinterpret_cast<const void*>(commands.argumentData()),
    "data-driven instructions and source arguments must be separate arrays");

  NVGcmdArgLocation width_location;
  require(commands.locateArgument(3, 2, width_location),
    "command arguments must be addressable without exposing raw cells");
  NVGcmdArgumentInfo width_info;
  require(commands.inspectArgument(width_location, width_info) &&
    width_info.op == NVG_CMD_ROUNDED_RECT &&
    width_info.source == NVGcmdArgSource::variable &&
    width_info.referenceIndex == 0,
    "argument inspection must preserve variable references");
  require(commands.setLiteralFloat(width_location, 96.0f) &&
    commands.inspectArgument(width_location, width_info) &&
    width_info.source == NVGcmdArgSource::literal &&
    width_info.value.f == 96.0f,
    "a coordinate variable must be replaceable with a checked literal");
  require(!commands.setLiteralInt(width_location, 96),
    "a float command argument must reject integer mutation");
  require(commands.bindVariable(width_location, "width") &&
    nvgCmdValidate(commands, &layout).succeeded(),
    "a mutated coordinate must bind back to a runtime variable");

  NVGcmdArgLocation color_location;
  NVGcmdArgumentInfo color_info;
  require(commands.locateArgument(4, 0, color_location) &&
    commands.inspectArgument(color_location, color_info) &&
    color_info.source == NVGcmdArgSource::property &&
    color_info.referenceIndex == red,
    "property-backed command arguments must be inspectable");
  require(commands.setLiteralFloat(color_location, 0.5f) &&
    commands.bindProperty(color_location, "red"),
    "a literal command argument must bind to a named property");
  require(!commands.locateArgument(99, 0, color_location),
    "an argument location must reject a missing command index");

  NVGcmdBuf structurally_changed = commands;
  NVGcmdArgLocation stale_location;
  require(structurally_changed.locateArgument(3, 2, stale_location),
    "a location must be obtainable before a structural edit");
  structurally_changed.beginPath();
  require(!structurally_changed.inspectArgument(stale_location, width_info),
    "structural edits must invalidate previously cached argument locations");

  const NVGcmdLayout undersized_layout{ variables,
    static_cast<uint32_t>(std::size(variables)), sizeof(float) };
  require(nvgCmdValidate(commands, &undersized_layout).code ==
    NVGcmdValidationCode::layout_mismatch,
    "runtime variable offsets must stay inside the declared data block");
  const NVGcmdLayout missing_table_layout{ nullptr,
    static_cast<uint32_t>(std::size(variables)), sizeof(DrawData) };
  require(nvgCmdValidate(commands, &missing_table_layout).code ==
    NVGcmdValidationCode::layout_mismatch,
    "a non-empty runtime layout must provide its variable table");

  MemoryIo text;
  require(commands.saveText(text), "command buffer must serialize to text");
  NVGcmdBuf text_roundtrip;
  require(text_roundtrip.loadText(text) &&
    nvgCmdValidate(text_roundtrip, &layout).succeeded(),
    "text command format must round-trip with variables and properties");

  MemoryIo binary;
  require(commands.saveBinary(binary), "command buffer must serialize to binary");
  NVGcmdBuf binary_roundtrip;
  require(binary_roundtrip.loadBinary(binary) &&
    nvgCmdValidate(binary_roundtrip, &layout).succeeded(),
    "binary command format must round-trip with validation");

  MemoryIo obsolete_binary = binary;
  obsolete_binary.cursor = 0;
  const uint32_t obsolete_version = 2;
  std::memcpy(obsolete_binary.bytes.data() + sizeof(uint32_t),
    &obsolete_version, sizeof(obsolete_version));
  NVGcmdBuf rejects_obsolete = commands;
  const uint32_t preserved_command_count = rejects_obsolete.size();
  require(!rejects_obsolete.loadBinary(obsolete_binary) &&
    rejects_obsolete.size() == preserved_command_count,
    "the modernized binary loader must reject the legacy interleaved format");

  auto renderer = std::make_unique<RecordingRenderer>();
  RecordingRenderer* recording = renderer.get();
  NVGcontext context(std::move(renderer), NVGcontextConfig{});
  NVGcmdArgBuffer first_arguments;
  context.beginFrame(200.0f, 100.0f, 1.0f);
  const NVGcmdEvalResult evaluated = nvgEvalChecked(
    context, binary_roundtrip, first_arguments, &data, &layout);
  context.endFrame();
  require(evaluated.succeeded() && evaluated.commands_executed == 7,
    "checked command execution must report every executed command");
  require(recording->fill_calls == 1 && recording->last_z_index == 23,
    "validated commands must reach NanoVG with bound runtime values");

  float first_max_x = 0.0f;
  for (const NVGvertex& vertex : recording->last_fill_vertices)
    first_max_x = std::max(first_max_x, vertex.x);
  DrawData resized_data = data;
  resized_data.width = 144.0f;
  NVGcmdArgBuffer resized_arguments;
  context.beginFrame(200.0f, 100.0f, 1.0f);
  const NVGcmdEvalResult resized_evaluation = nvgEvalChecked(
    context, binary_roundtrip, resized_arguments, &resized_data, &layout);
  context.endFrame();
  float resized_max_x = 0.0f;
  for (const NVGvertex& vertex : recording->last_fill_vertices)
    resized_max_x = std::max(resized_max_x, vertex.x);
  require(resized_evaluation.succeeded() && resized_max_x > first_max_x + 40.0f,
    "one immutable command buffer must respond to different widget sizes");
  require(first_arguments.size() == binary_roundtrip.argumentCount() &&
    resized_arguments.size() == binary_roundtrip.argumentCount() &&
    first_arguments.data() != resized_arguments.data(),
    "each widget instance must own an independent resolved argument buffer");
  const NVGcmdInstruction& rounded_rect = binary_roundtrip.commands[3];
  require(std::fabs(first_arguments.data()[rounded_rect.argumentOffset + 2].f -
    84.0f) < 1.0e-4f &&
    std::fabs(resized_arguments.data()[rounded_rect.argumentOffset + 2].f -
      144.0f) < 1.0e-4f,
    "instance argument buffers must resolve the same command against different data");
  require(binary_roundtrip.arguments[rounded_rect.argumentOffset + 2].u == 0,
    "runtime resolution must not mutate the shared variable reference");

  NVGcmdBuf edited_program = binary_roundtrip;
  NVGcmdArgBuffer edited_arguments;
  require(nvgCmdResolveArguments(edited_arguments, edited_program, &data,
    &layout), "an editable program must resolve before mutation");
  NVGcmdArgLocation edited_width;
  require(edited_program.locateArgument(3, 2, edited_width) &&
    edited_program.setLiteralFloat(edited_width, 101.0f) &&
    nvgEvalResolved(context, edited_program, edited_arguments) == 0,
    "source argument edits must invalidate previously resolved instance data");
  require(nvgCmdResolveArguments(edited_arguments, edited_program, &data,
    &layout) &&
    std::fabs(edited_arguments.data()[
      edited_program.commands[3].argumentOffset + 2].f - 101.0f) < 1.0e-4f,
    "an invalidated instance buffer must accept freshly resolved values");

  struct TextData { const char* text; } text_data{ "runtime text" };
  const NVGcmdVar text_variables[] = {
    NVG_VAR_ENTRY(TextData, text, NVG_VAR_STRING)
  };
  const NVGcmdLayout text_layout{ text_variables, 1, sizeof(TextData) };
  NVGcmdBuf text_program;
  text_program.meta.addVar("text", NVG_VAR_STRING);
  text_program.text(0.0f, 0.0f, V(0));
  NVGcmdArgBuffer text_arguments;
  require(nvgCmdValidate(text_program, &text_layout).succeeded() &&
    nvgCmdResolveArguments(text_arguments, text_program, &text_data,
      &text_layout) &&
    reinterpret_cast<const char*>(text_arguments.data()[2].pointer) ==
      text_data.text,
    "per-instance arguments must preserve pointer-sized values on 64-bit builds");

  NVGcmdBuf truncated;
  truncated.rect(0.0f, 0.0f, 10.0f, 10.0f);
  truncated.arguments.pop_back();
  require(nvgCmdValidate(truncated).code ==
    NVGcmdValidationCode::truncated_command,
    "validator must reject a truncated command stream");

  NVGcmdBuf unbalanced;
  unbalanced.save();
  require(nvgCmdValidate(unbalanced).code ==
    NVGcmdValidationCode::unbalanced_save_restore,
    "validator must reject unbalanced NanoVG state commands");

  NVGcmdBuf bad_reference;
  bad_reference.meta.addVar("width", NVG_VAR_FLOAT);
  bad_reference.beginPath();
  bad_reference.rect(0.0f, 0.0f, V(9), 10.0f);
  require(nvgCmdValidate(bad_reference).code ==
    NVGcmdValidationCode::variable_out_of_range,
    "validator must reject references outside the declared schema");

  NVGcmdBuf preserved = commands;
  const uint32_t preserved_size = preserved.size();
  MemoryIo malformed_text;
  const char malformed[] = "begin_path\nrect 0 0 10\nfill\n";
  malformed_text.bytes.assign(malformed, malformed + sizeof(malformed) - 1);
  require(!preserved.loadText(malformed_text) &&
    preserved.size() == preserved_size,
    "failed text loads must be transactional and preserve the previous program");
}

void test_custom_triangle_forwarding()
{
  auto renderer = std::make_unique<RecordingRenderer>();
  RecordingRenderer* recording = renderer.get();
  NVGcontext context(std::move(renderer), NVGcontextConfig{});

  context.beginFrame(640.0f, 480.0f, 1.5f);
  context.setZIndex(17);

  const NVGcustomDraw draw{
    NVGhandle(static_cast<NVGhandle::_handle_type>(10)),
    NVGhandle(static_cast<NVGhandle::_handle_type>(11))
  };
  const NVGvertex vertices[] = {
    { 1.0f, 2.0f, 0.0f, 0.0f },
    { 3.0f, 4.0f, 1.0f, 0.0f },
    { 5.0f, 6.0f, 0.5f, 1.0f }
  };
  context.drawTriangles(draw, vertices, 3);
  context.drawTriangles(NVGcustomDraw{}, vertices, 3);
  context.drawTriangles(draw, nullptr, 3);
  context.drawTriangles(draw, vertices, 0);
  context.endFrame();

  require(recording->viewport_calls == 1, "beginFrame must configure the renderer viewport");
  require(recording->custom_draw_calls == 1, "custom triangles must reach the renderer");
  require(recording->last_draw.shader == draw.shader, "custom shader handle must be preserved");
  require(recording->last_draw.image == draw.image, "custom image handle must be preserved");
  require(recording->last_vertices.size() == 3, "all custom vertices must be preserved");
  require(recording->last_vertices[2].x == 5.0f && recording->last_vertices[2].v == 1.0f,
    "custom vertex data must be preserved");
  require(recording->last_z_index == 17, "custom draw must use the current z-index");
  require(recording->flush_calls == 1, "endFrame must flush deferred rendering");
}

void test_nested_scissor_intersection()
{
  auto renderer = std::make_unique<RecordingRenderer>();
  RecordingRenderer* recording = renderer.get();
  NVGcontext context(std::move(renderer), NVGcontextConfig{});

  context.beginFrame(200.0f, 200.0f, 1.0f);
  context.scissor(10.0f, 10.0f, 100.0f, 100.0f);
  context.save();
  context.translate(50.0f, 50.0f);
  context.intersectScissor(0.0f, 0.0f, 100.0f, 100.0f);
  context.beginPath();
  context.rect(0.0f, 0.0f, 100.0f, 100.0f);
  context.fillColor(NVGcolor::RGB(255, 255, 255));
  context.fill();
  context.restore();

  context.beginPath();
  context.rect(0.0f, 0.0f, 20.0f, 20.0f);
  context.fillColor(NVGcolor::RGB(255, 255, 255));
  context.fill();

  context.save();
  context.intersectScissor(150.0f, 150.0f, 10.0f, 10.0f);
  context.beginPath();
  context.rect(150.0f, 150.0f, 10.0f, 10.0f);
  context.fillColor(NVGcolor::RGB(255, 255, 255));
  context.fill();
  context.StrokeWidth(1.0f);
  context.strokeColor(NVGcolor::RGB(255, 255, 255));
  context.stroke();
  const NVGvertex hidden_vertices[] = {
    { 150.0f, 150.0f, 0.0f, 0.0f },
    { 160.0f, 150.0f, 1.0f, 0.0f },
    { 150.0f, 160.0f, 0.0f, 1.0f }
  };
  context.drawTriangles({
    NVGhandle(static_cast<NVGhandle::_handle_type>(21)),
    NVGhandle(static_cast<NVGhandle::_handle_type>(22))
  }, hidden_vertices, 3);
  context.restore();
  context.endFrame();

  require(recording->fill_scissors.size() == 2,
    "nested clipping test must submit both fills");
  require(recording->stroke_calls == 0 && recording->custom_draw_calls == 0,
    "an empty scissor must suppress fill, stroke, and custom geometry calls");
  const NVGscissor& nested = recording->fill_scissors[0];
  require(std::fabs(nested.xform[4] - 80.0f) < 1.0e-4f &&
    std::fabs(nested.xform[5] - 80.0f) < 1.0e-4f &&
    std::fabs(nested.extent[0] - 30.0f) < 1.0e-4f &&
    std::fabs(nested.extent[1] - 30.0f) < 1.0e-4f,
    "a child scissor must remain intersected with its translated parent viewport");
  const NVGscissor& restored = recording->fill_scissors[1];
  require(std::fabs(restored.xform[4] - 60.0f) < 1.0e-4f &&
    std::fabs(restored.xform[5] - 60.0f) < 1.0e-4f &&
    std::fabs(restored.extent[0] - 50.0f) < 1.0e-4f &&
    std::fabs(restored.extent[1] - 50.0f) < 1.0e-4f,
    "restoring a child draw must recover the parent scissor unchanged");
}

void test_narrow_rounded_rectangle_geometry()
{
  auto renderer = std::make_unique<RecordingRenderer>();
  RecordingRenderer* recording = renderer.get();
  NVGcontext context(std::move(renderer), NVGcontextConfig{});

  context.beginFrame(200.0f, 200.0f, 1.0f);
  context.beginPath();
  context.roundedRect(10.0f, 10.0f, 12.0f, 100.0f, 40.0f);
  context.fillColor(NVGcolor::RGB(255, 255, 255));
  context.fill();
  context.endFrame();

  require(recording->fill_calls == 1 && !recording->last_fill_vertices.empty(),
    "narrow rounded rectangle must produce fill geometry");
  float min_x = 1000.0f;
  for (const NVGvertex& vertex : recording->last_fill_vertices)
    min_x = std::min(min_x, vertex.x);

  float min_side_y = 1000.0f;
  float max_side_y = -1000.0f;
  for (const NVGvertex& vertex : recording->last_fill_vertices) {
    if (std::fabs(vertex.x - min_x) < 1.0e-3f) {
      min_side_y = std::min(min_side_y, vertex.y);
      max_side_y = std::max(max_side_y, vertex.y);
    }
  }
  require(max_side_y - min_side_y > 80.0f,
    "oversized radius must preserve the long straight side of a narrow rectangle");
}

void test_separate_path_command_and_argument_buffers()
{
  auto renderer = std::make_unique<RecordingRenderer>();
  RecordingRenderer* recording = renderer.get();
  InspectableContext context(std::move(renderer), NVGcontextConfig{});

  context.beginFrame(240.0f, 160.0f, 1.0f);
  context.translate(5.0f, 7.0f);
  context.beginPath();
  context.roundedRectVarying(10.0f, 20.0f, 100.0f, 40.0f,
    3.0f, 4.0f, 5.0f, 6.0f);

  constexpr NVGcommands expected_commands[] = {
    NVG_MOVETO, NVG_LINETO, NVG_BEZIERTO, NVG_LINETO, NVG_BEZIERTO,
    NVG_LINETO, NVG_BEZIERTO, NVG_LINETO, NVG_BEZIERTO, NVG_CLOSE
  };
  constexpr uint32_t expected_offsets[] = {
    0, 2, 4, 10, 12, 18, 20, 26, 28, 34
  };
  require(context.path_command_count() == std::size(expected_commands),
    "roundedRectVarying must emit ten compact path descriptors");
  require(context.path_argument_count() == 34,
    "roundedRectVarying must store only its coordinate arguments as floats");
  require(reinterpret_cast<const void*>(context.path_commands()) !=
    reinterpret_cast<const void*>(context.path_arguments()),
    "path opcodes and path arguments must use independent allocations");

  for (size_t i = 0; i < std::size(expected_commands); ++i) {
    require(context.path_commands()[i].getType() == expected_commands[i],
      "a path descriptor must preserve its opcode");
    require(context.path_commands()[i].getArgumentOffset() == expected_offsets[i],
      "a path descriptor must point at its first argument");
  }
  require(std::fabs(context.path_arguments()[0] - 15.0f) < 1.0e-4f &&
    std::fabs(context.path_arguments()[1] - 30.0f) < 1.0e-4f,
    "path arguments must retain NanoVG's current transform semantics");

  context.fillColor(NVGcolor::RGB(255, 255, 255));
  context.fill();
  require(recording->fill_calls == 1 && !recording->last_fill_vertices.empty(),
    "the separated buffers must flatten into renderable rounded geometry");

  context.beginPath();
  require(context.path_command_count() == 0 &&
    context.path_argument_count() == 0,
    "beginPath must reset both path buffers together");

  context.moveTo(1.0f, 2.0f);
  context.lineTo(3.0f, 4.0f);
  context.bezierTo(4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
  context.quadTo(10.0f, 11.0f, 12.0f, 13.0f);
  context.closePath();
  context.pathWinding(NVG_HOLE);
  constexpr NVGcommands basic_commands[] = {
    NVG_MOVETO, NVG_LINETO, NVG_BEZIERTO,
    NVG_BEZIERTO, NVG_CLOSE, NVG_WINDING
  };
  constexpr uint32_t basic_offsets[] = { 0, 2, 4, 10, 16, 16 };
  require(context.path_command_count() == std::size(basic_commands) &&
    context.path_argument_count() == 17,
    "basic path APIs must write descriptors and arguments independently");
  for (size_t i = 0; i < std::size(basic_commands); ++i) {
    require(context.path_commands()[i].getType() == basic_commands[i] &&
      context.path_commands()[i].getArgumentOffset() == basic_offsets[i],
      "every basic path API must emit a valid compact descriptor");
  }
  require(std::fabs(context.path_arguments()[10] - (5.0f + 28.0f / 3.0f)) <
    1.0e-4f,
    "quadTo must derive its first control point from the previous endpoint");
  context.endFrame();
}

bool same_color(const NVGcolor& lhs, const NVGcolor& rhs)
{
  return std::fabs(lhs.r - rhs.r) < 1.0e-6f &&
    std::fabs(lhs.g - rhs.g) < 1.0e-6f &&
    std::fabs(lhs.b - rhs.b) < 1.0e-6f &&
    std::fabs(lhs.a - rhs.a) < 1.0e-6f;
}

void test_theme_document_compilation()
{
  RmThemeDocument document = RmThemeDocument::dark_theme();
  document.name = "Test theme";
  document.tokens.colors.accent = NVGcolor::RGBA(12, 34, 210, 255);
  document.tokens.typography.control = 17.0f;
  document.tokens.controls.switch_track_height = 30.0f;
  document.tokens.controls.text_input_horizontal_padding = 13.0f;
  document.tokens.controls.number_input_button_width = 31.0f;
  document.tokens.controls.button_icon_size = 18.0f;
  document.tokens.controls.tooltip_show_delay = 0.25f;
  document.tokens.controls.treeview_indent = 23.0f;
  document.tokens.controls.treeview_selection_horizontal_padding = 7.0f;
  document.tokens.controls.treeview_draw_background = false;
  document.tokens.controls.propertyview_name_column_ratio = 0.42f;
  document.tokens.controls.output_text_line_height = 19.0f;

  const RmThemeCompileResult result = RmThemeCompiler::compile(document);
  require(result.succeeded(), "a valid theme document must compile");
  require(result.theme->name == "Test theme", "theme display name must be preserved");
  require(same_color(result.theme->buttons.primary.background.normal,
    document.tokens.colors.accent), "accent token must drive the button recipe");
  require(result.theme->buttons.primary.font_size == 17.0f,
    "typography token must drive control font size");
  require(same_color(result.theme->buttons.secondary.background.normal,
    document.tokens.colors.control), "secondary buttons must use control tokens");
  require(result.theme->buttons.subtle.border_width == 0.0f &&
    result.theme->buttons.subtle.background.normal.a == 0.0f,
    "subtle buttons must compile as borderless transparent controls");
  require(same_color(result.theme->buttons.destructive.background.normal,
    document.tokens.colors.danger), "destructive buttons must use danger tokens");
  require(result.theme->text_input.horizontal_padding == 13.0f,
    "text input recipes must use editable padding metrics");
  require(same_color(result.theme->text_input.selection,
    NVGcolor::RGBAf(document.tokens.colors.accent.r,
      document.tokens.colors.accent.g, document.tokens.colors.accent.b, 0.55f)),
    "text input selection must derive from the accent token");
  require(result.theme->number_input.button_width == 31.0f,
    "number input recipes must use editable spinner metrics");
  require(result.theme->buttons.primary.icon_size == 18.0f,
    "button recipes must expose editable icon metrics");
  require(result.theme->tooltip.show_delay == 0.25f &&
    same_color(result.theme->tooltip.background,
      document.tokens.colors.control),
    "surface tooltips must compile from shared theme tokens");
  require(result.theme->treeview.indent == 23.0f,
    "tree view recipes must use editable hierarchy metrics");
  require(result.theme->treeview.selection_horizontal_padding == 7.0f,
    "tree view recipes must use editable selection metrics");
  require(!result.theme->treeview.draw_background,
    "tree view recipes must support a transparent surface");
  require(same_color(result.theme->treeview.selection_border,
    document.tokens.colors.focus_ring) &&
    result.theme->treeview.selection_border_width == 1.0f,
    "tree view selection must use a compact one-pixel focus border");
  require(result.theme->propertyview.name_column_ratio == 0.42f,
    "property view recipes must use editable table metrics");
  require(result.theme->buttons.primary.focus_ring_width == 1.0f &&
    result.theme->buttons.primary.corner_radius <= 5.0f,
    "default controls must use strict one-pixel focus and compact corners");
  require(result.theme->output_text.line_height == 19.0f,
    "output text recipes must use editable line metrics");
  require(same_color(result.theme->output_text.background,
    document.tokens.colors.surface_elevated),
    "output text must use the elevated surface token");
  require(result.theme->tabs.document.tab_height ==
    document.tokens.controls.tab_height,
    "tab recipes must use shared control metrics");
  require(result.theme->tabs.segmented.fill_available_width,
    "segmented tabs must distribute the available width");
  require(result.theme->tabs.underline.show_indicator,
    "underline tabs must compile an active indicator");
  require(result.theme->menu.item_height == document.tokens.controls.menu_item_height,
    "menu recipes must use shared control metrics");
  require(same_color(result.theme->menu.popup_background,
    document.tokens.colors.surface_elevated),
    "popup menus must use the elevated surface token");
  require(result.theme->combobox.item_height ==
    document.tokens.controls.combobox_item_height,
    "combobox recipes must use editable item metrics");
  require(same_color(result.theme->combobox.popup_background,
    document.tokens.colors.surface_elevated),
    "combobox popup must use the elevated surface token");
  require(same_color(result.theme->combobox.selected_mark,
    document.tokens.colors.accent),
    "combobox selected mark must use the accent token");
  require(result.theme->radiobutton.indicator_size ==
    document.tokens.controls.radiobutton_indicator_size,
    "radio button recipes must use editable indicator metrics");
  require(same_color(result.theme->radiobutton.mark.normal,
    document.tokens.colors.accent),
    "radio button marks must use the accent token");
  require(result.theme->listview.row_height ==
    document.tokens.controls.listview_row_height,
    "list view recipes must use editable row metrics");
  require(same_color(result.theme->listview.selected_background.normal,
    document.tokens.colors.accent),
    "list view selection must use the accent token");
  require(result.theme->scrollbar.thickness ==
    document.tokens.controls.scrollbar_thickness,
    "scrollbar recipes must use editable thickness metrics");
  require(same_color(result.theme->scrollbar.thumb.pressed,
    document.tokens.colors.accent_pressed),
    "dragged scrollbar thumb must use the pressed accent token");
  require(result.theme->toolbar.button_extent ==
    document.tokens.controls.toolstrip_button_extent &&
    result.theme->toolbox.selected_border_width == 1.0f,
    "toolbar and toolbox recipes must use editable compact metrics");
  require(result.theme->rebar.gripper_extent ==
    document.tokens.controls.rebar_gripper_extent,
    "rebar recipes must expose band gripper geometry");
  require(result.theme->splitter.thickness ==
    document.tokens.controls.splitter_thickness,
    "splitter recipes must use editable divider thickness");
  require(result.theme->switch_control.track_height == 30.0f,
    "component metric token must drive switch geometry");
  require(result.theme->switch_control.corner_radius == 15.0f,
    "compiled switch recipe must derive its pill radius");

  RmThemeDocument invalid = RmThemeDocument::light_theme();
  invalid.name.clear();
  invalid.tokens.colors.accent = NVGcolor::RGBAf(2.0f, -1.0f, 0.5f, 3.0f);
  invalid.tokens.typography.control = -20.0f;
  invalid.tokens.controls.checkbox_size = 0.0f;
  const RmThemeCompileResult sanitized = RmThemeCompiler::compile(invalid);
  require(sanitized.succeeded(), "recoverable authoring errors must produce a snapshot");
  require(sanitized.diagnostics.size() >= 4,
    "theme compiler must report every corrected authoring value");
  require(sanitized.theme->buttons.primary.font_size == 1.0f,
    "invalid font size must be clamped");
  require(sanitized.theme->checkbox.box_size == 1.0f,
    "invalid component size must be clamped");
  require(sanitized.theme->buttons.primary.background.normal.r == 1.0f &&
    sanitized.theme->buttons.primary.background.normal.g == 0.0f &&
    sanitized.theme->buttons.primary.background.normal.a == 1.0f,
    "invalid color components must be normalized");
}

} // namespace

int main()
{
  test_custom_triangle_forwarding();
  test_nested_scissor_intersection();
  test_narrow_rounded_rectangle_geometry();
  test_separate_path_command_and_argument_buffers();
  test_command_buffer_runtime();
  test_theme_document_compilation();
  std::cout << "All NanoVG, command, and theme tests passed\n";
  return 0;
}
