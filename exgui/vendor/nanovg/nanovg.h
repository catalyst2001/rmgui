//
// Copyright (c) 2025 NanoVG-cpp
// 
// Mikko Mononen memon@inside.org
// Kirill Deryabin "catalyst" kd@allalg.ru
// Daniil Runin "Daniluk2"
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
// Original license:
//

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
#include <stdexcept>
#include <algorithm>
#include <cassert>

#ifdef RGB
#undef RGB
#endif

#define NVG_PI 3.14159265358979323846264338327f

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // nonstandard extension used : nameless struct/union
#endif

struct NVGcolor {
	union {
		float rgba[4];
		struct {
			float r, g, b, a;
		};
	};

	static NVGcolor RGB(unsigned char r, unsigned char g, unsigned char b);
	static NVGcolor RGBf(float r, float g, float b);
	static NVGcolor RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	static NVGcolor RGBAf(float r, float g, float b, float a);
	static NVGcolor lerpRGBA(NVGcolor c0, NVGcolor c1, float u);
	static NVGcolor transRGBA(NVGcolor c0, unsigned char a);
	static NVGcolor transRGBAf(NVGcolor c0, float a);
	static NVGcolor HSL(float h, float s, float l);
	static NVGcolor HSLA(float h, float s, float l, unsigned char a);
	static NVGcolor HSLAf(float h, float s, float l, float a);
};

struct NVGpaint {
	float xform[6];
	float extent[2];
	float radius;
	float feather;
	NVGcolor innerColor;
	NVGcolor outerColor;
	int image;

	static NVGpaint linearGradient(float sx, float sy, float ex, float ey, NVGcolor icol, NVGcolor ocol);
	static NVGpaint boxGradient(float x, float y, float w, float h, float r, float f, NVGcolor icol, NVGcolor ocol);
	static NVGpaint radialGradient(float cx, float cy, float inr, float outr, NVGcolor icol, NVGcolor ocol);
	static NVGpaint imagePattern(float ox, float oy, float ex, float ey, float angle, int image, float alpha);
};

enum NVGblurType {
	NVG_BLUR_GAUSSIAN = 0,
	NVG_BLUR_MOTION,
	NVG_BLUR_RADIAL,
	NVG_BLUR_LINEAR,
	NVG_BLUR_BOKEH,
	NVG_BLUR_LIQUID,
};

struct NVGblurStyle {
	float radius;
	float strength;
	int   steps;
	int   rings;
	NVGcolor color;
	NVGblurType type;
	float angle;
	float length;
	float jitter;
	int   blades;

	NVGblurStyle();
};

struct NVGglowStyle {
	float radius;
	float intensity;
	NVGcolor color;

	NVGglowStyle();
};

struct NVGglassStyle {
	float radius;
	float blur;
	int blurSamples;
	float highlight;
	float borderWidth;
	int backgroundImage;
	float backgroundAlpha;
	NVGcolor tint;
	NVGcolor highlightColor;
	NVGcolor shadowColor;
	NVGcolor borderColor;

	NVGglassStyle();
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
	NVG_ALIGN_LEFT = 1 << 0,
	NVG_ALIGN_CENTER = 1 << 1,
	NVG_ALIGN_RIGHT = 1 << 2,
	NVG_ALIGN_TOP = 1 << 3,
	NVG_ALIGN_MIDDLE = 1 << 4,
	NVG_ALIGN_BOTTOM = 1 << 5,
	NVG_ALIGN_BASELINE = 1 << 6,
};

enum NVGblendFactor {
	NVG_ZERO = 1 << 0,
	NVG_ONE = 1 << 1,
	NVG_SRC_COLOR = 1 << 2,
	NVG_ONE_MINUS_SRC_COLOR = 1 << 3,
	NVG_DST_COLOR = 1 << 4,
	NVG_ONE_MINUS_DST_COLOR = 1 << 5,
	NVG_SRC_ALPHA = 1 << 6,
	NVG_ONE_MINUS_SRC_ALPHA = 1 << 7,
	NVG_DST_ALPHA = 1 << 8,
	NVG_ONE_MINUS_DST_ALPHA = 1 << 9,
	NVG_SRC_ALPHA_SATURATE = 1 << 10,
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
	NVG_IMAGE_GENERATE_MIPMAPS = 1 << 0,
	NVG_IMAGE_REPEATX = 1 << 1,
	NVG_IMAGE_REPEATY = 1 << 2,
	NVG_IMAGE_FLIPY = 1 << 3,
	NVG_IMAGE_PREMULTIPLIED = 1 << 4,
	NVG_IMAGE_NEAREST = 1 << 5,
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
	NVG_PIPELINE_DEPTH_TEST = 1 << 0,
	NVG_PIPELINE_DEPTH_WRITE = 1 << 1,
	NVG_PIPELINE_CULL_BACK = 1 << 2,
	NVG_PIPELINE_CULL_FRONT = 1 << 3,
	NVG_PIPELINE_SCISSOR = 1 << 4,
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
	float x, y, u, v;
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
	float x, y;
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

	virtual int create() = 0;
	virtual int createTexture(int type, int w, int h, int imageFlags, const unsigned char* data) = 0;
	virtual int deleteTexture(int image) = 0;
	virtual int updateTexture(int image, int x, int y, int w, int h, const unsigned char* data) = 0;
	virtual int getTextureSize(int image, int* w, int* h) = 0;
	virtual void viewport(float width, float height, float devicePixelRatio) = 0;
	virtual void cancel() = 0;
	virtual void flush() = 0;
	virtual void fill(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths) = 0;
	virtual void stroke(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths) = 0;
	virtual void triangles(const NVGpaint& paint, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, const NVGvertex* verts, int nverts, float fringe) = 0;
	virtual void Delete() = 0;

	virtual int createRenderTarget(const NVGrenderTargetDesc& desc) = 0;
	virtual void deleteRenderTarget(int target) = 0;
	virtual void setRenderTarget(int target) = 0;
	virtual int getRenderTargetImage(int target) = 0;
	virtual int createShader(const NVGshaderDesc& desc) = 0;
	virtual void deleteShader(int shader) = 0;
	virtual int createPipeline(const NVGpipelineDesc& desc) = 0;
	virtual void deletePipeline(int pipeline) = 0;
	virtual void drawCustomTriangles(const NVGcustomDraw& draw, NVGcompositeOperationState compositeOperation, const NVGscissor& scissor, const NVGvertex* verts, int nverts, float fringe) = 0;
};

struct FONScontext;

/**
* @brief transform matrix
*/
struct NVGtransform {
	union {
		struct { float mat[3][2]; };
		struct { float v[3 * 2]; };
	};
};

/**
* memory allocation overrides for debug and tracking
*/
#ifdef _DEBUG
void* nvg__alloc(size_t size, const char* file, int line);
void  nvg__free(void* ptr, const char* file, int line);

#define NVG_OVERRIDE_ALLOC() \
    static void* operator new(size_t size) { return nvg__alloc(size, "<not provided>", 0); } \
    static void  operator delete(void* ptr) noexcept { nvg__free(ptr, "<not provided>", 0); } \
    static void* operator new[](size_t size) { return nvg__alloc(size, "<not provided>", 0); } \
    static void  operator delete[](void* ptr) noexcept { nvg__free(ptr, "<not provided>", 0); } \
    static void  operator delete(void* ptr, size_t) noexcept { nvg__free(ptr, "<not provided>", 0); } \
    static void  operator delete[](void* ptr, size_t) noexcept { nvg__free(ptr, "<not provided>", 0); } \
    static void* operator new(size_t size, const char* file, int line) { return nvg__alloc(size, file, line); } \
    static void  operator delete(void* ptr, const char* file, int line) noexcept { nvg__free(ptr, file, line); } \
    static void* operator new[](size_t size, const char* file, int line) { return nvg__alloc(size, file, line); } \
    static void  operator delete[](void* ptr, const char* file, int line) noexcept { nvg__free(ptr, file, line); }
#else
void* nvg__alloc(size_t size);
void  nvg__free(void* ptr);

#define NVG_OVERRIDE_ALLOC() \
    static void* operator new(size_t size) { return nvg__alloc(size); } \
    static void  operator delete(void* ptr) noexcept { nvg__free(ptr); } \
    static void* operator new[](size_t size) { return nvg__alloc(size); } \
    static void  operator delete[](void* ptr) noexcept { nvg__free(ptr); } \
    static void  operator delete(void* ptr, size_t) noexcept { nvg__free(ptr); } \
    static void  operator delete[](void* ptr, size_t) noexcept { nvg__free(ptr); }
#endif

#include <algorithm>
#include <cstddef>

/**
* @brief A simple dynamic buffer for NanoVG commands and data.
*/
template<typename _type, size_t _limit = 0>
class NVGbuffer {
public:
	NVG_OVERRIDE_ALLOC();
private:
	size_t m_capacity = 0;
	size_t m_size = 0;
	_type* m_pdata = nullptr;
public:
	NVGbuffer() = default;

	explicit NVGbuffer(size_t cap) : m_capacity(cap), m_size(0) {
		if (m_capacity < _limit) m_capacity = _limit;
		m_pdata = (m_capacity ? new _type[m_capacity] : nullptr);
	}

	~NVGbuffer() {
		if (m_pdata) {
			delete[] m_pdata;
			m_pdata = nullptr;
		}
	}

	NVGbuffer(const NVGbuffer&) = delete;
	NVGbuffer& operator=(const NVGbuffer&) = delete;

	NVGbuffer(NVGbuffer&& other) noexcept
		: m_capacity(other.m_capacity), m_size(other.m_size), m_pdata(other.m_pdata) {
		other.m_capacity = 0;
		other.m_size = 0;
		other.m_pdata = nullptr;
	}

	NVGbuffer& operator=(NVGbuffer&& other) noexcept {
		if (this == &other) return *this;
		delete[] m_pdata;
		m_capacity = other.m_capacity;
		m_size = other.m_size;
		m_pdata = other.m_pdata;
		other.m_capacity = 0;
		other.m_size = 0;
		other.m_pdata = nullptr;
		return *this;
	}

	inline _type* getData() { return m_pdata; }
	inline size_t getSize() const { return m_size; }
	inline size_t getCapacity() const { return m_capacity; }

	bool availCapacity(size_t needed) {
		// overflow guard
		const size_t required = m_size + needed;
		if (required < m_size)
			return false;

		// already enough + meets minimal limit (if any)
		const size_t minCap = (_limit > 0 ? _limit : 0);
		if (required <= m_capacity && m_capacity >= minCap)
			return true;

		// target is at least required and at least _limit
		size_t target = std::max(required, minCap);

		// grow-by-1.5 from previous capacity (or just target if capacity==0)
		size_t grown = (m_capacity > 0) ? (m_capacity + m_capacity / 2) : target;

		// ensure we don't end up smaller than target
		size_t newCap = std::max(target, grown);
		_type* newData = new _type[newCap];
		for (size_t i = 0; i < m_size; ++i)
			newData[i] = m_pdata[i];

		delete[] m_pdata;
		m_pdata = newData;
		m_capacity = newCap;
		return true;
	}

	void clear() {
		m_size = 0;
	}

	void shrinkToFit() {
		if (m_size == m_capacity)
			return;

		_type* newData = (m_size ? new _type[m_size] : nullptr);
		for (size_t i = 0; i < m_size; ++i)
			newData[i] = m_pdata[i];

		delete[] m_pdata;
		m_pdata = newData;
		m_capacity = m_size;
	}

	bool appendBack(const _type* pdata, size_t count) {
		if (!availCapacity(count))
			return false;

		for (size_t i = 0; i < count; ++i)
			m_pdata[m_size + i] = pdata[i];

		m_size += count;
		return true;
	}

	inline _type &operator[](size_t index) {
#ifdef _DEBUG
		if(index >= m_size)
			throw std::out_of_range("NVGbuffer index out of range");
#endif
		return m_pdata[index];
	}

	inline bool isEmpty() const {
		return (m_size == 0);
	}
};

/**
* @brief A simple fixed-capacity stack for NanoVG state handling.
*/
template<typename _type, size_t _capacity>
class NVGstackFixed {
	static_assert(_capacity > 0, "_capacity is 0!");
	size_t m_pos;
	_type  m_data[_capacity]{};
public:
	NVGstackFixed() : m_pos(0) {}

	/**
	* @brief Clears the stack.
	*/
	void clear() { m_pos = 0; }

	/**
	* @brief Pushes a value to top of the stack.
	* @param val Value to push.
	* @return true if succeeded, false if stack is full.
	*/
	bool push(const _type& val) {
		if (m_pos < _capacity) {
			m_data[m_pos++] = val;
			return true;
		}
		return false;
	}

	/**
	* @brief Reserve value to top of the stack. Clear value if it needed.
	* @return true if succeeded, false if stack is full.
	*/
	bool push() {
		if (m_pos < _capacity) {
			m_pos++;
			return true;
		}
		return false;
	}

	/**
	* @brief Retreives the top value of the stack.
	* @return Reference to the top value.
	*/
	_type &top() {
		assert(m_pos > 0 && "NVGstackFixed::top() called on empty stack!");
		return m_data[m_pos - 1];
	}

	/**
	* @brief Pops the top value off the stack.
	* @return true if succeeded, false if stack is empty.
	*/
	bool pop() { 
		if (m_pos > 0) {
			--m_pos;
			return true;
		}
		return false;
	}

	/**
	* @brief Gets current size of the stack.
	* @return Current size of the stack.
	*/
	inline size_t getSize() const { return m_pos; }

	/**
	* @brief Gets maximum capacity of the stack.
	* @return Maximum capacity of the stack.
	*/
	inline size_t getCapacity() const { return _capacity; }

	/**
	* @brief Checks whether the stack is empty.
	* @return true if the stack is empty
	*/
	inline bool isEmpty() const { return (m_pos == 0); }
};

/**
* @brief NanoVG context.
*/
class NVGcontext {
public:
	NVG_OVERRIDE_ALLOC();
protected:
	std::unique_ptr<NVGrenderer> m_renderer;
	int              m_rendererCreated;
	NVGcontextConfig m_config;
	NVGbuffer<float> m_commandsBuffer;
	float  m_commandx, m_commandy;
	NVGstackFixed<NVGstate, NVG_MAX_STATES> m_states;
	NVGpathCache* m_pcache;
	float m_tessTol;
	float m_distTol;
	float m_fringeWidth;
	float m_devicePxRatio;
	float m_viewWidth;
	float m_viewHeight;

	int   m_boundRenderTarget;
	int   m_glassRenderTarget;
	int   m_glassRenderTargetW;
	int   m_glassRenderTargetH;
	float m_glassRenderTargetRatio;

	FONScontext* m_fs;
	int m_fontImages[NVG_MAX_FONTIMAGES];
	int m_fontImageIdx;
	int m_drawCallCount;
	int m_fillTriCount;
	int m_strokeTriCount;
	int m_textTriCount;

private:
	void nvg__deletePathCache(NVGpathCache* c);
	NVGpathCache* allocPathCache(void);
	void setDevicePixelRatio(float ratio);
	NVGstate* getState(NVGcontext* ctx);
	void nvg__appendCommands(NVGcontext* ctx, float* vals, int nvals);
	void nvg__clearPathCache(NVGcontext* ctx);
	NVGpath* nvg__lastPath(NVGcontext* ctx);
	void nvg__addPath(NVGcontext* ctx);
	NVGpoint* nvg__lastPoint(NVGcontext* ctx);
	void nvg__addPoint(NVGcontext* ctx, float x, float y, int flags);
	void nvg__closePath(NVGcontext* ctx);
	void nvg__pathWinding(NVGcontext* ctx, int winding);
	NVGvertex* nvg__allocTempVerts(NVGcontext* ctx, int nverts);
	void nvg__tesselateBezier(NVGcontext* ctx,
		float x1, float y1, float x2, float y2,
		float x3, float y3, float x4, float y4,
		int level, int type);
	void nvg__flattenPaths(NVGcontext* ctx);
	void nvg__calculateJoins(NVGcontext* ctx, float w, int lineJoin, float miterLimit);
	int  nvg__expandStroke(NVGcontext* ctx, float w, float fringe, int lineCap, int lineJoin, float miterLimit);
	int  nvg__expandFill(NVGcontext* ctx, float w, int lineJoin, float miterLimit);
	void nvg__flushTextTexture(NVGcontext* ctx);
	int  nvg__allocTextAtlas(NVGcontext* ctx);
	void nvg__renderText(NVGcontext* ctx, NVGvertex* verts, int nverts);
public:
	NVGcontext(std::unique_ptr<NVGrenderer> m_renderer, const NVGcontextConfig& m_config);
	~NVGcontext();

	NVGcontext(const NVGcontext&) = delete;
	NVGcontext& operator=(const NVGcontext&) = delete;

	// Frame.
	void beginFrame(float windowWidth, float windowHeight, float devicePixelRatio);
	void beginFrame(int renderTarget, float windowWidth,
		float windowHeight, float devicePixelRatio);
	void cancelFrame();
	void endFrame();

	// Composite operation.
	void globalCompositeOp(int op);
	void globalCompositeBlendFunc(int sfactor, int dfactor);
	void globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha);

	// State handling.
	void save();
	void restore();
	void reset();

	// Render styles.
	void shapeAntiAlias(int enabled);
	void strokeColor(NVGcolor color);
	void strokePaint(NVGpaint paint);
	void fillColor(NVGcolor color);
	void fillPaint(NVGpaint paint);
	void MiterLimit(float limit);
	void StrokeWidth(float size);
	void LineCap(int cap);
	void LineJoin(int join);
	void GlobalAlpha(float alpha);

	// Transforms.
	void resetTransform();
	void transform(float a, float b, float c, float d, float e, float f);
	void translate(float x, float y);
	void rotate(float angle);
	void skewX(float angle);
	void skewY(float angle);
	void scale(float x, float y);
	void getCurrentTransform(float* xform);

	// Transform utils.
	static void TransformIdentity(float* dst);
	static void TransformTranslate(float* dst, float tx, float ty);
	static void TransformScale(float* dst, float sx, float sy);
	static void TransformRotate(float* dst, float a);
	static void TransformSkewX(float* dst, float a);
	static void TransformSkewY(float* dst, float a);
	static void TransformMultiply(float* dst, const float* src);
	static void TransformPremultiply(float* dst, const float* src);
	static int  TransformInverse(float* dst, const float* src);
	static void TransformPoint(float* dstx, float* dsty, const float* xform, float srcx, float srcy);
	static float DegToRad(float deg);
	static float RadToDeg(float rad);

	// Images.
	int  createImage(const char* filename, int imageFlags);
	int  createImageMem(int imageFlags, unsigned char* data, int ndata);
	int  createImageRGBA(int w, int h, int imageFlags, const unsigned char* data);
	void updateImage(int image, const unsigned char* data);
	void getImageSize(int image, int* w, int* h);
	void deleteImage(int image);

	// Scissoring.
	void scissor(float x, float y, float w, float h);
	void intersectScissor(float x, float y, float w, float h);
	void resetScissor();

	// Paths.
	void beginPath();
	void moveTo(float x, float y);
	void lineTo(float x, float y);
	void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
	void quadTo(float cx, float cy, float x, float y);
	void arcTo(float x1, float y1, float x2, float y2, float radius);
	void closePath();
	void pathWinding(int dir);
	void arc(float cx, float cy, float r, float a0, float a1, int dir);
	void rect(float x, float y, float w, float h);
	void roundedRect(float x, float y, float w, float h, float r);
	void roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);
	void ellipse(float cx, float cy, float rx, float ry);
	void circle(float cx, float cy, float r);
	void fill();
	void stroke();

	// Text.
	int  createFont(const char* name, const char* filename);
	int  createFontAtIndex(const char* name, const char* filename, const int fontIndex);
	int  createFontMem(const char* name, unsigned char* data, int ndata, int freeData);
	int  createFontMemAtIndex(const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex);
	int  findFont(const char* name);
	int  addFallbackFontId(int baseFont, int fallbackFont);
	int  addFallbackFont(const char* baseFont, const char* fallbackFont);
	int  deleteFont(int font);
	void resetFallbackFontsId(int baseFont);
	void resetFallbackFonts(const char* baseFont);
	void setFontSize(float size);
	void setFontBlur(float blur);
	void setTextLetterSpacing(float spacing);
	void setTextLineHeight(float lineHeight);
	void setTextAlign(int align);
	void setFontFaceId(int font);
	void setFontFace(const char* font);
	float text(float x, float y, const char* string, const char* end);
	void textBox(float x, float y, float breakRowWidth, const char* string, const char* end);
	float textBounds(float x, float y, const char* string, const char* end, float* bounds);
	void textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds);
	int textGlyphPositions(float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions);
	void textMetrics(float* ascender, float* descender, float* lineh);
	int textBreakLines(const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows);

	// Effects.
	void textBlur(float x, float y, const char* string, const char* end, const NVGblurStyle& style);
	void glowRect(float x, float y, float w, float h, float r, const NVGglowStyle& style);
	void glassRect(float x, float y, float w, float h, const NVGglassStyle& style);

	// Custom pipeline.
	int  createRenderTarget(const NVGrenderTargetDesc& desc);
	void deleteRenderTarget(int target);
	void setRenderTarget(int target);
	int  getRenderTargetImage(int target);
	int  createShader(const NVGshaderDesc& desc);
	void deleteShader(int shader);
	int  createPipeline(const NVGpipelineDesc& desc);
	void deletePipeline(int pipeline);
	void drawTriangles(const NVGcustomDraw& draw, const NVGvertex* verts, int nverts);

	// Debug.
	void debugDumpPathCache();

	inline NVGrenderer* getRenderer() { return m_renderer.get(); }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define NVG_NOTUSED(v) for (;;) { (void)(1 ? (void)0 : ( (void)(v) ) ); break; }

#endif // NANOVG_H
