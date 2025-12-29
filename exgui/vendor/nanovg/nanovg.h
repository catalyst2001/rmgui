//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
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

#ifndef NANOVG_H
#define NANOVG_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#define NVG_PI 3.14159265358979323846264338327f

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // nonstandard extension used : nameless struct/union
#endif

struct NVGcolor {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};

	static NVGcolor RGB(unsigned char r, unsigned char g, unsigned char b);
	static NVGcolor RGBf(float r, float g, float b);
	static NVGcolor RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	static NVGcolor RGBAf(float r, float g, float b, float a);
	static NVGcolor LerpRGBA(NVGcolor c0, NVGcolor c1, float u);
	static NVGcolor TransRGBA(NVGcolor c0, unsigned char a);
	static NVGcolor TransRGBAf(NVGcolor c0, float a);
	static NVGcolor HSL(float h, float s, float l);
	static NVGcolor HSLA(float h, float s, float l, unsigned char a);
};

struct NVGpaint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	NVGcolor innerColor;
	NVGcolor outerColor;
	int image;

	static NVGpaint LinearGradient(float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol);
	static NVGpaint BoxGradient(float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol);
	static NVGpaint RadialGradient(float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol);
	static NVGpaint ImagePattern(float ox, float oy, float ex, float ey, float angle, int image, float alpha);
};

enum NVGwinding {
	NVG_CCW = 1,
	NVG_CW = 2,
};

enum NVGsolidity {
	NVG_SOLID = 1,
	NVG_HOLE = 2,
};

enum NVGlineCap {
	NVG_BUTT,
	NVG_ROUND,
	NVG_SQUARE,
	NVG_BEVEL,
	NVG_MITER,
};

enum NVGalign {
	NVG_ALIGN_LEFT 		= 1<<0,
	NVG_ALIGN_CENTER 	= 1<<1,
	NVG_ALIGN_RIGHT 	= 1<<2,
	NVG_ALIGN_TOP 		= 1<<3,
	NVG_ALIGN_MIDDLE	= 1<<4,
	NVG_ALIGN_BOTTOM	= 1<<5,
	NVG_ALIGN_BASELINE	= 1<<6,
};

enum NVGblendFactor {
	NVG_ZERO = 1<<0,
	NVG_ONE = 1<<1,
	NVG_SRC_COLOR = 1<<2,
	NVG_ONE_MINUS_SRC_COLOR = 1<<3,
	NVG_DST_COLOR = 1<<4,
	NVG_ONE_MINUS_DST_COLOR = 1<<5,
	NVG_SRC_ALPHA = 1<<6,
	NVG_ONE_MINUS_SRC_ALPHA = 1<<7,
	NVG_DST_ALPHA = 1<<8,
	NVG_ONE_MINUS_DST_ALPHA = 1<<9,
	NVG_SRC_ALPHA_SATURATE = 1<<10,
};

enum NVGcompositeOperation {
	NVG_SOURCE_OVER,
	NVG_SOURCE_IN,
	NVG_SOURCE_OUT,
	NVG_ATOP,
	NVG_DESTINATION_OVER,
	NVG_DESTINATION_IN,
	NVG_DESTINATION_OUT,
	NVG_DESTINATION_ATOP,
	NVG_LIGHTER,
	NVG_COPY,
	NVG_XOR,
};

struct NVGcompositeOperationState {
	int srcRGB;
	int dstRGB;
	int srcAlpha;
	int dstAlpha;
};

typedef struct NVGcompositeOperationState NVGcompositeOperationState;

struct NVGglyphPosition {
	const char* str;
	float x;
	float minx, maxx;
};

typedef struct NVGglyphPosition NVGglyphPosition;

struct NVGtextRow {
	const char* start;
	const char* end;
	const char* next;
	float width;
	float minx, maxx;
};

typedef struct NVGtextRow NVGtextRow;

enum NVGimageFlags {
	NVG_IMAGE_GENERATE_MIPMAPS	= 1<<0,
	NVG_IMAGE_REPEATX			= 1<<1,
	NVG_IMAGE_REPEATY			= 1<<2,
	NVG_IMAGE_FLIPY				= 1<<3,
	NVG_IMAGE_PREMULTIPLIED		= 1<<4,
	NVG_IMAGE_NEAREST			= 1<<5,
};

struct NVGrenderTargetDesc {
	int width;
	int height;
	float devicePixelRatio;
	unsigned int colorFormat;
	unsigned int depthStencilFormat;
	int samples;
	unsigned int flags;
};

typedef struct NVGrenderTargetDesc NVGrenderTargetDesc;

enum NVGshaderStage {
	NVG_SHADER_STAGE_VERTEX = 0,
	NVG_SHADER_STAGE_FRAGMENT = 1,
	NVG_SHADER_STAGE_COMPUTE = 2,
};

enum NVGshaderCodeType {
	NVG_SHADER_CODE_TEXT = 0,
	NVG_SHADER_CODE_BINARY = 1,
};

struct NVGshaderDesc {
	const void* data;
	size_t size;
	unsigned int format;
	unsigned int type;
	NVGshaderStage stage;
	NVGshaderCodeType codeType;
};

typedef struct NVGshaderDesc NVGshaderDesc;

struct NVGblendState {
	int enabled;
	int srcRGB;
	int dstRGB;
	int srcAlpha;
	int dstAlpha;
};

typedef struct NVGblendState NVGblendState;

enum NVGpipelineFlags {
	NVG_PIPELINE_NONE = 0,
	NVG_PIPELINE_DEPTH_TEST = 1<<0,
	NVG_PIPELINE_DEPTH_WRITE = 1<<1,
	NVG_PIPELINE_CULL_BACK = 1<<2,
	NVG_PIPELINE_CULL_FRONT = 1<<3,
	NVG_PIPELINE_SCISSOR = 1<<4,
};

struct NVGpipelineDesc {
	int vertexShader;
	int fragmentShader;
	NVGblendState blend;
	unsigned int flags;
	const void* payload;
	size_t payloadSize;
	unsigned int payloadType;
};

typedef struct NVGpipelineDesc NVGpipelineDesc;

struct NVGcustomDraw {
	int pipeline;
	int image;
	const void* uniforms;
	size_t uniformSize;
	unsigned int uniformSlot;
};

typedef struct NVGcustomDraw NVGcustomDraw;

enum NVGtexture {
	NVG_TEXTURE_ALPHA = 0x01,
	NVG_TEXTURE_RGBA = 0x02,
};

struct NVGscissor {
	float xform[6];
	float extent[2];
};

typedef struct NVGscissor NVGscissor;

struct NVGvertex {
	float x,y,u,v;
};

typedef struct NVGvertex NVGvertex;

struct NVGpath {
	int first;
	int count;
	unsigned char closed;
	int nbevel;
	NVGvertex* fill;
	int nfill;
	NVGvertex* stroke;
	int nstroke;
	int winding;
	int convex;
};

typedef struct NVGpath NVGpath;

#ifndef NVG_MAX_STATES
#define NVG_MAX_STATES 32
#endif

#ifndef NVG_MAX_FONTIMAGES
#define NVG_MAX_FONTIMAGES 4
#endif

enum NVGcommands {
	NVG_MOVETO = 0,
	NVG_LINETO = 1,
	NVG_BEZIERTO = 2,
	NVG_CLOSE = 3,
	NVG_WINDING = 4,
};

enum NVGpointFlags {
	NVG_PT_CORNER = 0x01,
	NVG_PT_LEFT = 0x02,
	NVG_PT_BEVEL = 0x04,
	NVG_PR_INNERBEVEL = 0x08,
};

struct NVGstate {
	NVGcompositeOperationState compositeOperation;
	int shapeAntiAlias;
	NVGpaint fill;
	NVGpaint stroke;
	float strokeWidth;
	float miterLimit;
	int lineJoin;
	int lineCap;
	float alpha;
	float xform[6];
	NVGscissor scissor;
	float fontSize;
	float letterSpacing;
	float lineHeight;
	float fontBlur;
	int textAlign;
	int fontId;
};

typedef struct NVGstate NVGstate;

struct NVGpoint {
	float x,y;
	float dx, dy;
	float len;
	float dmx, dmy;
	unsigned char flags;
};

typedef struct NVGpoint NVGpoint;

struct NVGpathCache {
	NVGpoint* points;
	int npoints;
	int cpoints;
	NVGpath* paths;
	int npaths;
	int cpaths;
	NVGvertex* verts;
	int nverts;
	int cverts;
	float bounds[4];
};

typedef struct NVGpathCache NVGpathCache;

struct NVGcontextConfig {
	int edgeAntiAlias;

	NVGcontextConfig()
		: edgeAntiAlias(1) {
	}
};

class NVGrenderer {
public:
	virtual ~NVGrenderer() = default;

	virtual int Create() = 0;
	virtual int CreateTexture(int type, int w, int h, int imageFlags, const unsigned char* data) = 0;
	virtual int DeleteTexture(int image) = 0;
	virtual int UpdateTexture(int image, int x, int y, int w, int h, const unsigned char* data) = 0;
	virtual int GetTextureSize(int image, int* w, int* h) = 0;
	virtual void Viewport(float width, float height, float devicePixelRatio) = 0;
	virtual void Cancel() = 0;
	virtual void Flush() = 0;
	virtual void Fill(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths) = 0;
	virtual void Stroke(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths) = 0;
	virtual void Triangles(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, const NVGvertex* verts, int nverts, float fringe) = 0;
	virtual void Delete() = 0;

	virtual int CreateRenderTarget(const NVGrenderTargetDesc& desc) = 0;
	virtual void DeleteRenderTarget(int target) = 0;
	virtual void BindRenderTarget(int target) = 0;
	virtual int RenderTargetImage(int target) = 0;
	virtual int CreateShader(const NVGshaderDesc& desc) = 0;
	virtual void DeleteShader(int shader) = 0;
	virtual int CreatePipeline(const NVGpipelineDesc& desc) = 0;
	virtual void DeletePipeline(int pipeline) = 0;
	virtual void RenderCustomTriangles(const NVGcustomDraw& draw, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, const NVGvertex* verts, int nverts, float fringe) = 0;
};

struct FONScontext;

struct NVGcontext {
	static std::unique_ptr<NVGcontext> Create(std::unique_ptr<NVGrenderer> renderer, const NVGcontextConfig& config = NVGcontextConfig());
	~NVGcontext();

	NVGcontext(const NVGcontext&) = delete;
	NVGcontext& operator=(const NVGcontext&) = delete;

	// Frame.
	void BeginFrame(float windowWidth, float windowHeight, float devicePixelRatio);
	void BeginFrame(int renderTarget, float windowWidth, float windowHeight, float devicePixelRatio);
	void CancelFrame();
	void EndFrame();

	// Composite operation.
	void GlobalCompositeOperation(int op);
	void GlobalCompositeBlendFunc(int sfactor, int dfactor);
	void GlobalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

	// State handling.
	void Save();
	void Restore();
	void Reset();

	// Render styles.
	void ShapeAntiAlias(int enabled);
	void StrokeColor(NVGcolor color);
	void StrokePaint(NVGpaint paint);
	void FillColor(NVGcolor color);
	void FillPaint(NVGpaint paint);
	void MiterLimit(float limit);
	void StrokeWidth(float size);
	void LineCap(int cap);
	void LineJoin(int join);
	void GlobalAlpha(float alpha);

	// Transforms.
	void ResetTransform();
	void Transform(float a, float b, float c, float d, float e, float f);
	void Translate(float x, float y);
	void Rotate(float angle);
	void SkewX(float angle);
	void SkewY(float angle);
	void Scale(float x, float y);
	void CurrentTransform(float* xform);

	// Transform utils.
	static void TransformIdentity(float* dst);
	static void TransformTranslate(float* dst, float tx, float ty);
	static void TransformScale(float* dst, float sx, float sy);
	static void TransformRotate(float* dst, float a);
	static void TransformSkewX(float* dst, float a);
	static void TransformSkewY(float* dst, float a);
	static void TransformMultiply(float* dst, const float* src);
	static void TransformPremultiply(float* dst, const float* src);
	static int TransformInverse(float* dst, const float* src);
	static void TransformPoint(float* dstx, float* dsty, const float* xform, float srcx, float srcy);
	static float DegToRad(float deg);
	static float RadToDeg(float rad);

	// Images.
	int CreateImage(const char* filename, int imageFlags);
	int CreateImageMem(int imageFlags, unsigned char* data, int ndata);
	int CreateImageRGBA(int w, int h, int imageFlags, const unsigned char* data);
	void UpdateImage(int image, const unsigned char* data);
	void ImageSize(int image, int* w, int* h);
	void DeleteImage(int image);

	// Scissoring.
	void Scissor(float x, float y, float w, float h);
	void IntersectScissor(float x, float y, float w, float h);
	void ResetScissor();

	// Paths.
	void BeginPath();
	void MoveTo(float x, float y);
	void LineTo(float x, float y);
	void BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
	void QuadTo(float cx, float cy, float x, float y);
	void ArcTo(float x1, float y1, float x2, float y2, float radius);
	void ClosePath();
	void PathWinding(int dir);
	void Arc(float cx, float cy, float r, float a0, float a1, int dir);
	void Rect(float x, float y, float w, float h);
	void RoundedRect(float x, float y, float w, float h, float r);
	void RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
	void Ellipse(float cx, float cy, float rx, float ry);
	void Circle(float cx, float cy, float r);
	void Fill();
	void Stroke();

	// Text.
	int CreateFont(const char* name, const char* filename);
	int CreateFontAtIndex(const char* name, const char* filename, const int fontIndex);
	int CreateFontMem(const char* name, unsigned char* data, int ndata, int freeData);
	int CreateFontMemAtIndex(const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);
	int FindFont(const char* name);
	int AddFallbackFontId(int baseFont, int fallbackFont);
	int AddFallbackFont(const char* baseFont, const char* fallbackFont);
	void ResetFallbackFontsId(int baseFont);
	void ResetFallbackFonts(const char* baseFont);
	void FontSize(float size);
	void FontBlur(float blur);
	void TextLetterSpacing(float spacing);
	void TextLineHeight(float lineHeight);
	void TextAlign(int align);
	void FontFaceId(int font);
	void FontFace(const char* font);
	float Text(float x, float y, const char* string, const char* end);
	void TextBox(float x, float y, float breakRowWidth, const char* string, const char* end);
	float TextBounds(float x, float y, const char* string, const char* end, float* bounds);
	void TextBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);
	int TextGlyphPositions(float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions);
	void TextMetrics(float* ascender, float* descender, float* lineh);
	int TextBreakLines(const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows);

	// Custom pipeline.
	int CreateRenderTarget(const NVGrenderTargetDesc& desc);
	void DeleteRenderTarget(int target);
	void BindRenderTarget(int target);
	int RenderTargetImage(int target);
	int CreateShader(const NVGshaderDesc& desc);
	void DeleteShader(int shader);
	int CreatePipeline(const NVGpipelineDesc& desc);
	void DeletePipeline(int pipeline);
	void DrawTriangles(const NVGcustomDraw& draw, const NVGvertex* verts, int nverts);

	// Debug.
	void DebugDumpPathCache();

	std::unique_ptr<NVGrenderer> renderer;
	int rendererCreated;
	NVGcontextConfig config;
	float* commands;
	int ccommands;
	int ncommands;
	float commandx, commandy;
	NVGstate states[NVG_MAX_STATES];
	int nstates;
	NVGpathCache* cache;
	float tessTol;
	float distTol;
	float fringeWidth;
	float devicePxRatio;
	FONScontext* fs;
	int fontImages[NVG_MAX_FONTIMAGES];
	int fontImageIdx;
	int drawCallCount;
	int fillTriCount;
	int strokeTriCount;
	int textTriCount;

private:
	NVGcontext();
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define NVG_NOTUSED(v) for (;;) { (void)(1 ? (void)0 : ( (void)(v) ) ); break; }

#endif // NANOVG_H
