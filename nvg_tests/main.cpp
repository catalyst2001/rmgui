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
  int custom_draw_calls = 0;
  int last_z_index = 0;
  NVGcustomDraw last_draw{};
  std::vector<NVGvertex> last_vertices;

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
  void fill(const NVGpaint&, NVGcompositeOperationState, const NVGscissor&, float,
    const float*, const NVGpath*, int) override {}
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

  const RmThemeCompileResult result = RmThemeCompiler::compile(document);
  require(result.succeeded(), "a valid theme document must compile");
  require(result.theme->name == "Test theme", "theme display name must be preserved");
  require(same_color(result.theme->button.background.normal,
    document.tokens.colors.accent), "accent token must drive the button recipe");
  require(result.theme->button.font_size == 17.0f,
    "typography token must drive control font size");
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
  require(sanitized.theme->button.font_size == 1.0f,
    "invalid font size must be clamped");
  require(sanitized.theme->checkbox.box_size == 1.0f,
    "invalid component size must be clamped");
  require(sanitized.theme->button.background.normal.r == 1.0f &&
    sanitized.theme->button.background.normal.g == 0.0f &&
    sanitized.theme->button.background.normal.a == 1.0f,
    "invalid color components must be normalized");
}

} // namespace

int main()
{
  test_custom_triangle_forwarding();
  test_theme_document_compilation();
  std::cout << "All NanoVG and theme tests passed\n";
  return 0;
}
