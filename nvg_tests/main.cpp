#include "nanovg.h"
#include "../exgui/exgui/rmgui_theme.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

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
    float, const NVGpath*, int) override {}
  void triangles(const NVGpaint&, NVGcompositeOperationState, const NVGscissor&,
    const NVGvertex*, int, float) override {}
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
  context.endFrame();

  require(recording->fill_scissors.size() == 2,
    "nested clipping test must submit both fills");
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
  test_theme_document_compilation();
  std::cout << "All NanoVG and theme tests passed\n";
  return 0;
}
