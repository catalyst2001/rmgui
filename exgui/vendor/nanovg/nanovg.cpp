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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#include "nanovg.h"
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#ifndef NVG_NO_STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // conditional expression is constant
#pragma warning(disable: 4204)  // nonstandard extension used : non-constant aggregate initializer
#pragma warning(disable: 4706)  // assignment within conditional expression
#endif

#define NVG_KAPPA90 0.5522847493f	// Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define NVG_COUNTOF(arr) (sizeof(arr) / sizeof(0[arr]))


static float nvg__sqrtf(float a) { return sqrtf(a); }
static float nvg__modf(float a, float b) { return fmodf(a, b); }
static float nvg__sinf(float a) { return sinf(a); }
static float nvg__cosf(float a) { return cosf(a); }
static float nvg__tanf(float a) { return tanf(a); }
static float nvg__atan2f(float a, float b) { return atan2f(a, b); }
static float nvg__acosf(float a) { return acosf(a); }
static float nvg__expf(float a) { return expf(a); }

//template<typename _type> _type nvg__mini(_type a, _type b) { return a < b ? a : b; }
//template<typename _type> _type nvg__maxi(_type a, _type b) { return a > b ? a : b; }

int nvg__mini(int a, int b)       { return a < b ? a : b; }
int nvg__maxi(int a, int b)       { return a > b ? a : b; }
float nvg__minf(float a, float b) { return a < b ? a : b; }
float nvg__maxf(float a, float b) { return a > b ? a : b; }

int nvg__clampi(int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
float nvg__absf(float a) { return a >= 0.0f ? a : -a; }
float nvg__signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
float nvg__clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
float nvg__cross(float dx0, float dy0, float dx1, float dy1) { return dx1 * dy0 - dx0 * dy1; }

static float nvg__normalize(float* x, float* y)
{
	float d = nvg__sqrtf((*x) * (*x) + (*y) * (*y));
	if (d > 1e-6f) {
		float id = 1.0f / d;
		*x *= id;
		*y *= id;
	}
	return d;
}


void NVGcontext::deletePathCache(NVGpathCache* c)
{
	if (c == nullptr)
		return;

	delete c;
	c = nullptr;
}

NVGpathCache* NVGcontext::allocPathCache(void)
{
	return new NVGpathCache;
}

void NVGcontext::setDevicePixelRatio(float ratio)
{
	m_tessTol = 0.25f / ratio;
	m_distTol = 0.01f / ratio;
	m_fringeWidth = 1.0f / ratio;
	m_devicePxRatio = ratio;
}

static NVGcompositeOperationState compositeOperationState(int op)
{
	static struct _factor {
		int sfactor;
		int dfactor;
	} factors[] = {
		{ NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA },	// NVG_SOURCE_OVER
		{ NVG_DST_ALPHA, NVG_ZERO },				// NVG_SOURCE_IN
		{ NVG_ONE_MINUS_DST_ALPHA, NVG_ZERO },	// NVG_SOURCE_OUT
		{ NVG_DST_ALPHA, NVG_ONE_MINUS_SRC_ALPHA }, // NVG_ATOP
		{ NVG_ONE_MINUS_DST_ALPHA, NVG_ONE },	// NVG_DESTINATION_OVER
		{ NVG_ZERO, NVG_SRC_ALPHA },				// NVG_DESTINATION_IN
		{ NVG_ZERO, NVG_ONE_MINUS_SRC_ALPHA },	// NVG_DESTINATION_OUT
		{ NVG_ONE_MINUS_DST_ALPHA, NVG_SRC_ALPHA }, // NVG_DESTINATION_ATOP
		{ NVG_ONE, NVG_ONE },					// NVG_LIGHTER
		{ NVG_ONE,  NVG_ZERO },					// NVG_COPY
		{ NVG_ONE_MINUS_DST_ALPHA, NVG_ONE_MINUS_SRC_ALPHA } // NVG_XOR
	};

	static constexpr int count = NVG_COUNTOF(factors);
	if (op < 0 || op >= count)
		op = NVG_COPY;

	_factor* pfactor = &factors[op];
	NVGcompositeOperationState state;
	state.srcRGB = pfactor->sfactor;
	state.dstRGB = pfactor->dfactor;
	state.srcAlpha = pfactor->sfactor;
	state.dstAlpha = pfactor->dfactor;
	return state;
}

NVGstate* NVGcontext::getState()
{
	return &m_states.top();
}

void NVGcontext::beginFrame(float windowWidth, float windowHeight, float devicePixelRatio)
{
	NVGcontext::beginFrame(NVGhandle(), windowWidth, windowHeight, devicePixelRatio);
}

void NVGcontext::beginFrame(NVGhandle renderTarget, float windowWidth, float windowHeight, float devicePixelRatio)
{
	m_renderer->setRenderTarget(renderTarget);
	m_boundRenderTarget = renderTarget;
	/*	printf("Tris: draws:%d  fill:%d  stroke:%d  text:%d  TOT:%d\n",
			drawCallCount, fillTriCount, strokeTriCount, textTriCount,
			fillTriCount+strokeTriCount+textTriCount);*/

	m_states.clear();
	save();
	reset();
	setDevicePixelRatio(devicePixelRatio);
	m_renderer->viewport(windowWidth, windowHeight, devicePixelRatio);
	m_viewWidth = windowWidth;
	m_viewHeight = windowHeight;
	m_drawCallCount = 0;
	m_fillTriCount = 0;
	m_strokeTriCount = 0;
	m_textTriCount = 0;
}

void NVGcontext::cancelFrame()
{
	m_renderer->cancel();
}

void NVGcontext::endFrame()
{
	m_renderer->flush();
	if (m_fontImageIdx != 0) {
		NVGhandle fontImage = m_fontImages[m_fontImageIdx];
		m_fontImages[m_fontImageIdx].invalidate();
		int i, j, iw, ih;
		// delete images that smaller than current one
		if (!fontImage.isValid())
			return;

		getImageSize(fontImage, &iw, &ih);
		for (i = j = 0; i < m_fontImageIdx; i++) {
			if (m_fontImages[i].isValid()) {
				int nw, nh;
				NVGhandle image = m_fontImages[i];
				m_fontImages[i].invalidate();
				getImageSize(image, &nw, &nh);
				if (nw < iw || nh < ih)
					deleteImage(image);
				else
					m_fontImages[j++] = image;
			}
		}
		// make current font image to first
		m_fontImages[j] = m_fontImages[0];
		m_fontImages[0] = fontImage;
		m_fontImageIdx = 0;
	}
}

NVGcolor NVGcolor::RGB(unsigned char r, unsigned char g, unsigned char b)
{
	return NVGcolor::RGBA(r, g, b, 255);
}

NVGcolor NVGcolor::RGBf(float r, float g, float b)
{
	return NVGcolor::RGBAf(r, g, b, 1.0f);
}

NVGcolor NVGcolor::RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	NVGcolor color;
	// Use longer initialization to suppress warning.
	color.r = r / 255.0f;
	color.g = g / 255.0f;
	color.b = b / 255.0f;
	color.a = a / 255.0f;
	return color;
}

NVGcolor NVGcolor::RGBAf(float r, float g, float b, float a)
{
	NVGcolor color;
	// Use longer initialization to suppress warning.
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}

NVGcolor NVGcolor::transRGBA(NVGcolor c, unsigned char a)
{
	c.a = a / 255.0f;
	return c;
}

NVGcolor NVGcolor::transRGBAf(NVGcolor c, float a)
{
	c.a = a;
	return c;
}

NVGcolor NVGcolor::lerpRGBA(NVGcolor c0, NVGcolor c1, float u)
{
	int i;
	float oneminu;
	NVGcolor cint = { {{0}} };

	u = nvg__clampf(u, 0.0f, 1.0f);
	oneminu = 1.0f - u;
	for (i = 0; i < 4; i++)
	{
		cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
	}

	return cint;
}

NVGcolor NVGcolor::HSL(float h, float s, float l)
{
	return NVGcolor::HSLA(h, s, l, 255);
}

static float nvg__hue(float h, float m1, float m2)
{
	if (h < 0) h += 1;
	if (h > 1) h -= 1;
	if (h < 1.0f / 6.0f)
		return m1 + (m2 - m1) * h * 6.0f;
	else if (h < 3.0f / 6.0f)
		return m2;
	else if (h < 4.0f / 6.0f)
		return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
	return m1;
}

NVGcolor NVGcolor::HSLA(float h, float s, float l, unsigned char a)
{
	float m1, m2;
	NVGcolor col;
	h = nvg__modf(h, 1.0f);
	if (h < 0.0f) h += 1.0f;
	s = nvg__clampf(s, 0.0f, 1.0f);
	l = nvg__clampf(l, 0.0f, 1.0f);
	m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
	m1 = 2 * l - m2;
	col.r = nvg__clampf(nvg__hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	col.g = nvg__clampf(nvg__hue(h, m1, m2), 0.0f, 1.0f);
	col.b = nvg__clampf(nvg__hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	col.a = a / 255.0f;
	return col;
}

NVGcolor NVGcolor::HSLAf(float h, float s, float l, float a)
{
	float m1, m2;
	NVGcolor col;
	h = nvg__modf(h, 1.0f);
	if (h < 0.0f) h += 1.0f;
	s = nvg__clampf(s, 0.0f, 1.0f);
	l = nvg__clampf(l, 0.0f, 1.0f);
	m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
	m1 = 2 * l - m2;
	col.r = nvg__clampf(nvg__hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	col.g = nvg__clampf(nvg__hue(h, m1, m2), 0.0f, 1.0f);
	col.b = nvg__clampf(nvg__hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
	col.a = nvg__clampf(a, 0.0f, 1.0f);
	return col;
}

void NVGcontext::TransformIdentity(float* t)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void NVGcontext::TransformTranslate(float* t, float tx, float ty)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = tx; t[5] = ty;
}

void NVGcontext::TransformScale(float* t, float sx, float sy)
{
	t[0] = sx; t[1] = 0.0f;
	t[2] = 0.0f; t[3] = sy;
	t[4] = 0.0f; t[5] = 0.0f;
}

void NVGcontext::TransformRotate(float* t, float a)
{
	float cs = nvg__cosf(a), sn = nvg__sinf(a);
	t[0] = cs; t[1] = sn;
	t[2] = -sn; t[3] = cs;
	t[4] = 0.0f; t[5] = 0.0f;
}

void NVGcontext::TransformSkewX(float* t, float a)
{
	t[0] = 1.0f; t[1] = 0.0f;
	t[2] = nvg__tanf(a); t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void NVGcontext::TransformSkewY(float* t, float a)
{
	t[0] = 1.0f; t[1] = nvg__tanf(a);
	t[2] = 0.0f; t[3] = 1.0f;
	t[4] = 0.0f; t[5] = 0.0f;
}

void NVGcontext::TransformMultiply(float* t, const float* s)
{
	float t0 = t[0] * s[0] + t[1] * s[2];
	float t2 = t[2] * s[0] + t[3] * s[2];
	float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
	t[1] = t[0] * s[1] + t[1] * s[3];
	t[3] = t[2] * s[1] + t[3] * s[3];
	t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
	t[0] = t0;
	t[2] = t2;
	t[4] = t4;
}

void NVGcontext::TransformPremultiply(float* t, const float* s)
{
	float s2[6];
	memcpy(s2, s, sizeof(float) * 6);
	NVGcontext::TransformMultiply(s2, t);
	memcpy(t, s2, sizeof(float) * 6);
}

int NVGcontext::TransformInverse(float* inv, const float* t)
{
	double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
	if (det > -1e-6 && det < 1e-6) {
		NVGcontext::TransformIdentity(inv);
		return 0;
	}
	invdet = 1.0 / det;
	inv[0] = (float)(t[3] * invdet);
	inv[2] = (float)(-t[2] * invdet);
	inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
	inv[1] = (float)(-t[1] * invdet);
	inv[3] = (float)(t[0] * invdet);
	inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
	return 1;
}

void NVGcontext::TransformPoint(float* dx, float* dy, const float* t, float sx, float sy)
{
	*dx = sx * t[0] + sy * t[2] + t[4];
	*dy = sx * t[1] + sy * t[3] + t[5];
}

float NVGcontext::DegToRad(float deg)
{
	return deg / 180.0f * NVG_PI;
}

float NVGcontext::RadToDeg(float rad)
{
	return rad / NVG_PI * 180.0f;
}

static void nvg__setPaintColor(NVGpaint* p, NVGcolor color)
{
	memset(p, 0, sizeof(*p));
	p->image.invalidate();
	p->shader.invalidate();
	NVGcontext::TransformIdentity(p->xform);
	p->radius = 0.0f;
	p->feather = 1.0f;
	p->innerColor = color;
	p->outerColor = color;
}


// State handling
void NVGcontext::save()
{
	/* push copy of last state */
	if (m_states.getSize() > 0) {
		if (!m_states.push(m_states.top())) {
			//printf("NVGcontext::save(): state stack overflowed!\n");
		}
		return;
	}

	/* push uninitialized state */
	m_states.push();
}

void NVGcontext::restore()
{
	if (!m_states.pop()) {
		//printf("NVGcontext::restore(): state stack underflowed!\n");
	}
}

void NVGcontext::reset()
{
	NVGstate* state = getState();
	memset(state, 0, sizeof(*state));

	nvg__setPaintColor(&state->fill, NVGcolor::RGBA(255, 255, 255, 255));
	nvg__setPaintColor(&state->stroke, NVGcolor::RGBA(0, 0, 0, 255));
	state->compositeOperation = compositeOperationState(NVG_SOURCE_OVER);
	state->shapeAntiAlias = 1;
	state->strokeWidth = 1.0f;
	state->miterLimit = 10.0f;
	state->lineCap = NVG_BUTT;
	state->lineJoin = NVG_MITER;
	state->alpha = 1.0f;
	NVGcontext::TransformIdentity(state->xform);

	state->scissor.extent[0] = -1.0f;
	state->scissor.extent[1] = -1.0f;

	state->fontSize = 16.0f;
	state->letterSpacing = 0.0f;
	state->lineHeight = 1.0f;
	state->fontBlur = 0.0f;
	state->textAlign = NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE;
	state->fontId = 0;
}

// State setting
void NVGcontext::shapeAntiAlias(int enabled)
{
	NVGstate* state = getState();
	state->shapeAntiAlias = enabled;
}

void NVGcontext::StrokeWidth(float width)
{
	NVGstate* state = getState();
	state->strokeWidth = width;
}

void NVGcontext::MiterLimit(float limit)
{
	NVGstate* state = getState();
	state->miterLimit = limit;
}

void NVGcontext::LineCap(int cap)
{
	NVGstate* state = getState();
	state->lineCap = cap;
}

void NVGcontext::LineJoin(int join)
{
	NVGstate* state = getState();
	state->lineJoin = join;
}

void NVGcontext::GlobalAlpha(float alpha)
{
	NVGstate* state = getState();
	state->alpha = alpha;
}

void NVGcontext::setZIndex(int z)
{
	NVGstate* state = getState();
	state->zIndex = z;
	m_renderer->setZIndex(z);
}

int NVGcontext::getZIndex() const
{
	return const_cast<NVGcontext*>(this)->getState()->zIndex;
}

void NVGcontext::transform(float a, float b, float c, float d, float e, float f)
{
	NVGstate* state = getState();
	float t[6] = { a, b, c, d, e, f };
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::resetTransform()
{
	NVGstate* state = getState();
	NVGcontext::TransformIdentity(state->xform);
}

void NVGcontext::translate(float x, float y)
{
	float t[6];
	NVGstate* state = getState();
	NVGcontext::TransformTranslate(t, x, y);
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::rotate(float angle)
{
	float t[6];
	NVGstate* state = getState();
	NVGcontext::TransformRotate(t, angle);
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::skewX(float angle)
{
	float t[6];
	NVGstate* state = getState();
	NVGcontext::TransformSkewX(t, angle);
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::skewY(float angle)
{
	float t[6];
	NVGstate* state = getState();
	NVGcontext::TransformSkewY(t, angle);
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::scale(float x, float y)
{
	float t[6];
	NVGstate* state = getState();
	NVGcontext::TransformScale(t, x, y);
	NVGcontext::TransformPremultiply(state->xform, t);
}

void NVGcontext::getCurrentTransform(float* xform)
{
	if (!xform)
		return;
	NVGstate* state = getState();
	memcpy(xform, state->xform, sizeof(float) * 6);
}

void NVGcontext::strokeColor(NVGcolor color)
{
	NVGstate* state = getState();
	nvg__setPaintColor(&state->stroke, color);
}

void NVGcontext::strokePaint(NVGpaint paint)
{
	NVGstate* state = getState();
	state->stroke = paint;
	NVGcontext::TransformMultiply(state->stroke.xform, state->xform);
}

void NVGcontext::fillColor(NVGcolor color)
{
	NVGstate* state = getState();
	nvg__setPaintColor(&state->fill, color);
}

void NVGcontext::fillPaint(NVGpaint paint)
{

	NVGstate* state = getState();
	state->fill = paint;
	NVGcontext::TransformMultiply(state->fill.xform, state->xform);
}

#ifndef NVG_NO_STB
NVGhandle NVGcontext::createImage(const char* filename, int imageFlags)
{
	int w, h, n;
	NVGhandle image;
	unsigned char* img;
	stbi_set_unpremultiply_on_load(1);
	stbi_convert_iphone_png_to_rgb(1);
	img = stbi_load(filename, &w, &h, &n, 4);
	if (img == NULL) {
		//		printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
		return NVGhandle();
	}
	image = createImageRGBA(w, h, imageFlags, img);
	stbi_image_free(img);
	return image;
}

NVGhandle NVGcontext::createImageMem(int imageFlags, unsigned char* data, int ndata)
{
	int w, h, n;
	NVGhandle image;
	unsigned char* img = stbi_load_from_memory(data, ndata, &w, &h, &n, 4);
	if (img == NULL) {
		//		printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
		return NVGhandle();
	}
	image = createImageRGBA(w, h, imageFlags, img);
	stbi_image_free(img);
	return image;
}
#endif

NVGhandle NVGcontext::createImageRGBA(int w, int h, int imageFlags, const unsigned char* data)
{
	return m_renderer->createTexture(NVG_TEXTURE_RGBA, w, h, imageFlags, data);
}

void NVGcontext::updateImage(NVGhandle image, const unsigned char* data)
{
	int w, h;
	m_renderer->getTextureSize(image, &w, &h);
	m_renderer->updateTexture(image, 0, 0, w, h, data);
}

void NVGcontext::getImageSize(NVGhandle image, int* w, int* h)
{
	m_renderer->getTextureSize(image, w, h);
}

void NVGcontext::deleteImage(NVGhandle image)
{
	m_renderer->deleteTexture(image);
}

NVGpaint NVGpaint::linearGradient(float sx, float sy, float ex, float ey,
	NVGcolor icol, NVGcolor ocol)
{
	NVGpaint p;
	float dx, dy, d;
	const float large = 1e5;
	memset(&p, 0, sizeof(p));
	p.image.invalidate();
	p.shader.invalidate();

	// Calculate transform aligned to the line
	dx = ex - sx;
	dy = ey - sy;
	d = sqrtf(dx * dx + dy * dy);
	if (d > 0.0001f) {
		dx /= d;
		dy /= d;
	}
	else {
		dx = 0;
		dy = 1;
	}

	p.xform[0] = dy; p.xform[1] = -dx;
	p.xform[2] = dx; p.xform[3] = dy;
	p.xform[4] = sx - dx * large; p.xform[5] = sy - dy * large;

	p.extent[0] = large;
	p.extent[1] = large + d * 0.5f;

	p.radius = 0.0f;

	p.feather = nvg__maxf(1.0f, d);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}

NVGpaint NVGpaint::radialGradient(float cx, float cy, float inr, float outr,
	NVGcolor icol, NVGcolor ocol)
{
	NVGpaint p;
	float r = (inr + outr) * 0.5f;
	float f = (outr - inr);
	memset(&p, 0, sizeof(p));
	p.image.invalidate();
	p.shader.invalidate();

	NVGcontext::TransformIdentity(p.xform);
	p.xform[4] = cx;
	p.xform[5] = cy;

	p.extent[0] = r;
	p.extent[1] = r;

	p.radius = r;

	p.feather = nvg__maxf(1.0f, f);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}

NVGpaint NVGpaint::boxGradient(float x, float y, float w, float h, float r, float f,
	NVGcolor icol, NVGcolor ocol)
{
	NVGpaint p;
	memset(&p, 0, sizeof(p));
	p.image.invalidate();
	p.shader.invalidate();

	NVGcontext::TransformIdentity(p.xform);
	p.xform[4] = x + w * 0.5f;
	p.xform[5] = y + h * 0.5f;

	p.extent[0] = w * 0.5f;
	p.extent[1] = h * 0.5f;

	p.radius = r;

	p.feather = nvg__maxf(1.0f, f);

	p.innerColor = icol;
	p.outerColor = ocol;

	return p;
}


NVGpaint NVGpaint::imagePattern(float cx, float cy, float w, float h, float angle,
	NVGhandle image, float alpha)
{
	NVGpaint p;
	memset(&p, 0, sizeof(p));
	p.image.invalidate();
	p.shader.invalidate();

	NVGcontext::TransformRotate(p.xform, angle);
	p.xform[4] = cx;
	p.xform[5] = cy;

	p.extent[0] = w;
	p.extent[1] = h;

	p.image = image;

	p.innerColor = p.outerColor = NVGcolor::RGBAf(1, 1, 1, alpha);

	return p;
}

NVGpaint NVGpaint::glass(float panelX, float panelY, float panelW, float panelH,
	float bgW, float bgH, NVGhandle blurredBackdrop, NVGhandle glassShader,
	NVGcolor tint, float alpha)
{
	NVGpaint p;
	memset(&p, 0, sizeof(p));
	p.image.invalidate();
	p.shader.invalidate();

	// xform/extent encode the panel rect (per-draw, in frag UBO)
	NVGcontext::TransformIdentity(p.xform);
	p.xform[4] = panelX;
	p.xform[5] = panelY;

	p.extent[0] = panelW;
	p.extent[1] = panelH;

	// radius/feather store panel origin in widget-local coords (for globalUV reconstruction)
	p.radius = panelX;
	p.feather = panelY;

	p.image = blurredBackdrop;
	p.shader = glassShader;

	// innerCol = tint color (applied to the backdrop)
	p.innerColor = tint;
	p.innerColor.a *= alpha;
	// outerCol = highlight/fresnel edge color (white by default)
	p.outerColor = NVGcolor::RGBAf(1.0f, 1.0f, 1.0f, 0.4f * alpha);

	return p;
}

NVGblurStyle::NVGblurStyle()
	: radius(4.0f),
	strength(0.6f),
	steps(8),
	rings(2),
	color(NVGcolor::RGBA(0, 0, 0, 255)),
	type(NVG_BLUR_GAUSSIAN),
	angle(0.0f),
	length(1.0f),
	jitter(0.0f),
	blades(6)
{
}

NVGglowStyle::NVGglowStyle()
	: radius(12.0f),
	intensity(0.7f),
	color(NVGcolor::RGBA(255, 255, 255, 255))
{
}

// Scissoring
void NVGcontext::scissor(float x, float y, float w, float h)
{
	NVGstate* state = getState();
	w = nvg__maxf(0.0f, w);
	h = nvg__maxf(0.0f, h);

	NVGcontext::TransformIdentity(state->scissor.xform);
	state->scissor.xform[4] = x + w * 0.5f;
	state->scissor.xform[5] = y + h * 0.5f;
	NVGcontext::TransformMultiply(state->scissor.xform, state->xform);

	state->scissor.extent[0] = w * 0.5f;
	state->scissor.extent[1] = h * 0.5f;
}

static void _intersectRects(float* dst,
	float ax, float ay, float aw, float ah,
	float bx, float by, float bw, float bh)
{
	float minx = nvg__maxf(ax, bx);
	float miny = nvg__maxf(ay, by);
	float maxx = nvg__minf(ax + aw, bx + bw);
	float maxy = nvg__minf(ay + ah, by + bh);
	dst[0] = minx;
	dst[1] = miny;
	dst[2] = nvg__maxf(0.0f, maxx - minx);
	dst[3] = nvg__maxf(0.0f, maxy - miny);
}

void NVGcontext::intersectScissor(float x, float y, float w, float h)
{
	NVGstate* state = getState();
	float pxform[6], invxorm[6];
	float rect[4];
	float ex, ey, tex, tey;

	// If no previous scissor has been set, set the scissor as current scissor.
	if (state->scissor.extent[0] < 0) {
		scissor(x, y, w, h);
		return;
	}

	// Transform the current scissor rect into current transform space.
	// If there is difference in rotation, this will be approximation.
	memcpy(pxform, state->scissor.xform, sizeof(float) * 6);
	ex = state->scissor.extent[0];
	ey = state->scissor.extent[1];
	NVGcontext::TransformInverse(invxorm, state->xform);
	NVGcontext::TransformMultiply(pxform, invxorm);
	tex = ex * nvg__absf(pxform[0]) + ey * nvg__absf(pxform[2]);
	tey = ex * nvg__absf(pxform[1]) + ey * nvg__absf(pxform[3]);

	// Intersect rects.
	_intersectRects(rect, pxform[4] - tex, pxform[5] - tey, tex * 2, tey * 2, x, y, w, h);
	scissor(rect[0], rect[1], rect[2], rect[3]);
}

void NVGcontext::resetScissor()
{
	NVGstate* state = getState();
	memset(state->scissor.xform, 0, sizeof(state->scissor.xform));
	state->scissor.extent[0] = -1.0f;
	state->scissor.extent[1] = -1.0f;
}

static bool nvg__isScissorEmpty(const NVGscissor& scissor)
{
	// Negative extents mean that scissoring is disabled. A non-negative
	// scissor with either zero extent is the empty intersection and must not
	// reach an antialiased renderer: its soft mask would otherwise produce a
	// half-covered one-pixel line at the collapsed edge.
	return scissor.extent[0] >= 0.0f &&
		(scissor.extent[0] <= 0.0f || scissor.extent[1] <= 0.0f);
}

// Global composite operation.
void NVGcontext::globalCompositeOp(int op)
{
	NVGstate* state = getState();
	state->compositeOperation = compositeOperationState(op);
}

void NVGcontext::globalCompositeBlendFunc(int sfactor, int dfactor)
{
	globalCompositeBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void NVGcontext::globalCompositeBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha)
{
	NVGcompositeOperationState op;
	op.srcRGB = srcRGB;
	op.dstRGB = dstRGB;
	op.srcAlpha = srcAlpha;
	op.dstAlpha = dstAlpha;

	NVGstate* state = getState();
	state->compositeOperation = op;
}

static int _nvgPtEquals(float x1, float y1, float x2, float y2, float tol)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return dx * dx + dy * dy < tol * tol;
}

static float nvg__distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
	float pqx, pqy, dx, dy, d, t;
	pqx = qx - px;
	pqy = qy - py;
	dx = x - px;
	dy = y - py;
	d = pqx * pqx + pqy * pqy;
	t = pqx * dx + pqy * dy;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	dx = px + t * pqx - x;
	dy = py + t * pqy - y;
	return dx * dx + dy * dy;
}

void NVGcontext::appendCommands(float* vals, int nvals)
{
	NVGstate* state = getState();
	int i;
	if ((int)vals[0] != NVG_CLOSE && (int)vals[0] != NVG_WINDING) {
		m_commandx = vals[nvals - 2];
		m_commandy = vals[nvals - 1];
	}

	// transform commands
	i = 0;
	while (i < nvals) {
		int cmd = (int)vals[i];
		switch (cmd) {
		case NVG_MOVETO:
			NVGcontext::TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
			i += 3;
			break;
		case NVG_LINETO:
			NVGcontext::TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
			i += 3;
			break;
		case NVG_BEZIERTO:
			NVGcontext::TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
			NVGcontext::TransformPoint(&vals[i + 3], &vals[i + 4], state->xform, vals[i + 3], vals[i + 4]);
			NVGcontext::TransformPoint(&vals[i + 5], &vals[i + 6], state->xform, vals[i + 5], vals[i + 6]);
			i += 7;
			break;
		case NVG_CLOSE:
			i++;
			break;
		case NVG_WINDING:
			i += 2;
			break;
		default:
			i++;
		}
	}
	m_commands.appendBack(vals, nvals);
}

static float nvg__getAverageScale(float* t)
{
	float sx = sqrtf(t[0] * t[0] + t[2] * t[2]);
	float sy = sqrtf(t[1] * t[1] + t[3] * t[3]);
	return (sx + sy) * 0.5f;
}

static float nvg__triarea2(float ax, float ay, float bx, float by, float cx, float cy)
{
	float abx = bx - ax;
	float aby = by - ay;
	float acx = cx - ax;
	float acy = cy - ay;
	return acx * aby - abx * acy;
}

static float nvg__polyArea(NVGpoint* pts, int npts)
{
	int i;
	float area = 0;
	for (i = 2; i < npts; i++) {
		NVGpoint* a = &pts[0];
		NVGpoint* b = &pts[i - 1];
		NVGpoint* c = &pts[i];
		area += nvg__triarea2(a->x, a->y, b->x, b->y, c->x, c->y);
	}
	return area * 0.5f;
}

static void nvg__polyReverse(NVGpoint* pts, int npts)
{
	NVGpoint tmp;
	int i = 0, j = npts - 1;
	while (i < j) {
		tmp = pts[i];
		pts[i] = pts[j];
		pts[j] = tmp;
		i++;
		j--;
	}
}

static void nvg__vset(NVGvertex* vtx, float x, float y, float u, float v)
{
	vtx->x = x;
	vtx->y = y;
	vtx->u = u;
	vtx->v = v;
}

void NVGcontext::flattenPaths()
{
	NVGpoint* last;
	size_t i;
	float* cp1;
	float* cp2;
	float* p;

	if (m_pathCache->getNumPaths() > 0)
		return;

	// Flatten
	i = 0;
	while (i < m_commands.getSize()) {
		int cmd = (int)m_commands[i];
		switch (cmd) {
		case NVG_MOVETO:
			m_pathCache->addPath();
			p = &m_commands[i + 1];
			m_pathCache->addPoint(p[0], p[1], NVG_PT_CORNER, m_distTol);
			i += 3;
			break;
		case NVG_LINETO:
			p = &m_commands[i + 1];
			m_pathCache->addPoint(p[0], p[1], NVG_PT_CORNER, m_distTol);
			i += 3;
			break;
		case NVG_BEZIERTO:
			last = m_pathCache->getLastPoint();
			if (last != NULL) {
				cp1 = &m_commands[i + 1];
				cp2 = &m_commands[i + 3];
				p = &m_commands[i + 5];
				m_pathCache->tesselateBezier(last->x, last->y,
					cp1[0], cp1[1],
					cp2[0], cp2[1],
					p[0], p[1],
					0, NVG_PT_CORNER, m_tessTol);
			}
			i += 7;
			break;
		case NVG_CLOSE:
			m_pathCache->closePath();
			i++;
			break;
		case NVG_WINDING:
			m_pathCache->pathWinding((int)m_commands[i + 1]);
			i += 2;
			break;
		default:
			i++;
		}
	}
	m_pathCache->updateCache(m_distTol);
}

static int nvg__curveDivs(float r, float arc, float tol)
{
	float da = acosf(r / (r + tol)) * 2.0f;
	return nvg__maxi(2, (int)ceilf(arc / da));
}

static void nvg__chooseBevel(int bevel, NVGpoint* p0, NVGpoint* p1, float w,
	float* x0, float* y0, float* x1, float* y1)
{
	if (bevel) {
		*x0 = p1->x + p0->dy * w;
		*y0 = p1->y - p0->dx * w;
		*x1 = p1->x + p1->dy * w;
		*y1 = p1->y - p1->dx * w;
	}
	else {
		*x0 = p1->x + p1->dmx * w;
		*y0 = p1->y + p1->dmy * w;
		*x1 = p1->x + p1->dmx * w;
		*y1 = p1->y + p1->dmy * w;
	}
}

static NVGvertex* nvg__roundJoin(NVGvertex* dst, NVGpoint* p0, NVGpoint* p1,
	float lw, float rw, float lu, float ru, int ncap,
	float fringe)
{
	int i, n;
	float dlx0 = p0->dy;
	float dly0 = -p0->dx;
	float dlx1 = p1->dy;
	float dly1 = -p1->dx;
	NVG_NOTUSED(fringe);

	if (p1->flags & NVG_PT_LEFT) {
		float lx0, ly0, lx1, ly1, a0, a1;
		nvg__chooseBevel(p1->flags & NVG_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);
		a0 = atan2f(-dly0, -dlx0);
		a1 = atan2f(-dly1, -dlx1);
		if (a1 > a0) a1 -= NVG_PI * 2;

		nvg__vset(dst, lx0, ly0, lu, 1); dst++;
		nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1); dst++;

		n = nvg__clampi((int)ceilf(((a0 - a1) / NVG_PI) * ncap), 2, ncap);
		for (i = 0; i < n; i++) {
			float u = i / (float)(n - 1);
			float a = a0 + u * (a1 - a0);
			float rx = p1->x + cosf(a) * rw;
			float ry = p1->y + sinf(a) * rw;
			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;
			nvg__vset(dst, rx, ry, ru, 1); dst++;
		}

		nvg__vset(dst, lx1, ly1, lu, 1); dst++;
		nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1); dst++;

	}
	else {
		float rx0, ry0, rx1, ry1, a0, a1;
		nvg__chooseBevel(p1->flags & NVG_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);
		a0 = atan2f(dly0, dlx0);
		a1 = atan2f(dly1, dlx1);
		if (a1 < a0) a1 += NVG_PI * 2;

		nvg__vset(dst, p1->x + dlx0 * rw, p1->y + dly0 * rw, lu, 1); dst++;
		nvg__vset(dst, rx0, ry0, ru, 1); dst++;

		n = nvg__clampi((int)ceilf(((a1 - a0) / NVG_PI) * ncap), 2, ncap);
		for (i = 0; i < n; i++) {
			float u = i / (float)(n - 1);
			float a = a0 + u * (a1 - a0);
			float lx = p1->x + cosf(a) * lw;
			float ly = p1->y + sinf(a) * lw;
			nvg__vset(dst, lx, ly, lu, 1); dst++;
			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;
		}

		nvg__vset(dst, p1->x + dlx1 * rw, p1->y + dly1 * rw, lu, 1); dst++;
		nvg__vset(dst, rx1, ry1, ru, 1); dst++;

	}
	return dst;
}

static NVGvertex* nvg__bevelJoin(NVGvertex* dst, NVGpoint* p0, NVGpoint* p1,
	float lw, float rw, float lu, float ru, float fringe)
{
	float rx0, ry0, rx1, ry1;
	float lx0, ly0, lx1, ly1;
	float dlx0 = p0->dy;
	float dly0 = -p0->dx;
	float dlx1 = p1->dy;
	float dly1 = -p1->dx;
	NVG_NOTUSED(fringe);

	if (p1->flags & NVG_PT_LEFT) {
		nvg__chooseBevel(p1->flags & NVG_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);

		nvg__vset(dst, lx0, ly0, lu, 1); dst++;
		nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1); dst++;

		if (p1->flags & NVG_PT_BEVEL) {
			nvg__vset(dst, lx0, ly0, lu, 1); dst++;
			nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1); dst++;

			nvg__vset(dst, lx1, ly1, lu, 1); dst++;
			nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1); dst++;
		}
		else {
			rx0 = p1->x - p1->dmx * rw;
			ry0 = p1->y - p1->dmy * rw;

			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;
			nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1); dst++;

			nvg__vset(dst, rx0, ry0, ru, 1); dst++;
			nvg__vset(dst, rx0, ry0, ru, 1); dst++;

			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;
			nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1); dst++;
		}

		nvg__vset(dst, lx1, ly1, lu, 1); dst++;
		nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1); dst++;

	}
	else {
		nvg__chooseBevel(p1->flags & NVG_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);

		nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1); dst++;
		nvg__vset(dst, rx0, ry0, ru, 1); dst++;

		if (p1->flags & NVG_PT_BEVEL) {
			nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1); dst++;
			nvg__vset(dst, rx0, ry0, ru, 1); dst++;

			nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1); dst++;
			nvg__vset(dst, rx1, ry1, ru, 1); dst++;
		}
		else {
			lx0 = p1->x + p1->dmx * lw;
			ly0 = p1->y + p1->dmy * lw;

			nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1); dst++;
			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;

			nvg__vset(dst, lx0, ly0, lu, 1); dst++;
			nvg__vset(dst, lx0, ly0, lu, 1); dst++;

			nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1); dst++;
			nvg__vset(dst, p1->x, p1->y, 0.5f, 1); dst++;
		}

		nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1); dst++;
		nvg__vset(dst, rx1, ry1, ru, 1); dst++;
	}

	return dst;
}

static NVGvertex* nvg__buttCapStart(NVGvertex* dst, NVGpoint* p,
	float dx, float dy, float w, float d,
	float aa, float u0, float u1)
{
	float px = p->x - dx * d;
	float py = p->y - dy * d;
	float dlx = dy;
	float dly = -dx;
	nvg__vset(dst, px + dlx * w - dx * aa, py + dly * w - dy * aa, u0, 0); dst++;
	nvg__vset(dst, px - dlx * w - dx * aa, py - dly * w - dy * aa, u1, 0); dst++;
	nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1); dst++;
	nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1); dst++;
	return dst;
}

static NVGvertex* nvg__buttCapEnd(NVGvertex* dst, NVGpoint* p,
	float dx, float dy, float w, float d,
	float aa, float u0, float u1)
{
	float px = p->x + dx * d;
	float py = p->y + dy * d;
	float dlx = dy;
	float dly = -dx;
	nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1); dst++;
	nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1); dst++;
	nvg__vset(dst, px + dlx * w + dx * aa, py + dly * w + dy * aa, u0, 0); dst++;
	nvg__vset(dst, px - dlx * w + dx * aa, py - dly * w + dy * aa, u1, 0); dst++;
	return dst;
}


static NVGvertex* nvg__roundCapStart(NVGvertex* dst, NVGpoint* p,
	float dx, float dy, float w, int ncap,
	float aa, float u0, float u1)
{
	int i;
	float px = p->x;
	float py = p->y;
	float dlx = dy;
	float dly = -dx;
	NVG_NOTUSED(aa);
	for (i = 0; i < ncap; i++) {
		float a = i / (float)(ncap - 1) * NVG_PI;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		nvg__vset(dst, px - dlx * ax - dx * ay, py - dly * ax - dy * ay, u0, 1); dst++;
		nvg__vset(dst, px, py, 0.5f, 1); dst++;
	}
	nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1); dst++;
	nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1); dst++;
	return dst;
}

static NVGvertex* nvg__roundCapEnd(NVGvertex* dst, NVGpoint* p,
	float dx, float dy, float w, int ncap,
	float aa, float u0, float u1)
{
	int i;
	float px = p->x;
	float py = p->y;
	float dlx = dy;
	float dly = -dx;
	NVG_NOTUSED(aa);
	nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1); dst++;
	nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1); dst++;
	for (i = 0; i < ncap; i++) {
		float a = i / (float)(ncap - 1) * NVG_PI;
		float ax = cosf(a) * w, ay = sinf(a) * w;
		nvg__vset(dst, px, py, 0.5f, 1); dst++;
		nvg__vset(dst, px - dlx * ax + dx * ay, py - dly * ax + dy * ay, u0, 1); dst++;
	}
	return dst;
}

// Draw
void NVGcontext::beginPath()
{
	m_commands.clear();
	m_pathCache->clearPathCache();
}

void NVGcontext::moveTo(float x, float y)
{
	float vals[] = { NVG_MOVETO, x, y };
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::lineTo(float x, float y)
{
	float vals[] = { NVG_LINETO, x, y };
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
	float vals[] = { NVG_BEZIERTO, c1x, c1y, c2x, c2y, x, y };
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::quadTo(float cx, float cy, float x, float y)
{
	float x0 = m_commandx;
	float y0 = m_commandy;
	float vals[] = {
		NVG_BEZIERTO,
			x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0),
			x + 2.0f / 3.0f * (cx - x), y + 2.0f / 3.0f * (cy - y),
			x, y
	};
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::arcTo(float x1, float y1, float x2, float y2, float radius)
{
	float x0 = m_commandx;
	float y0 = m_commandy;
	float dx0, dy0, dx1, dy1, a, d, cx, cy, a0, a1;
	int dir;

	if (m_commands.isEmpty())
		return;

	// Handle degenerate cases.
	if (_nvgPtEquals(x0, y0, x1, y1, m_distTol) ||
		_nvgPtEquals(x1, y1, x2, y2, m_distTol) ||
		nvg__distPtSeg(x1, y1, x0, y0, x2, y2) < m_distTol * m_distTol ||
		radius < m_distTol) {
		lineTo(x1, y1);
		return;
	}

	// Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
	dx0 = x0 - x1;
	dy0 = y0 - y1;
	dx1 = x2 - x1;
	dy1 = y2 - y1;
	nvg__normalize(&dx0, &dy0);
	nvg__normalize(&dx1, &dy1);
	a = nvg__acosf(dx0 * dx1 + dy0 * dy1);
	d = radius / nvg__tanf(a / 2.0f);

	//	printf("a=%fT- d=%f\n", a/NVG_PI*180.0f, d);
	if (d > 10000.0f) {
		lineTo(x1, y1);
		return;
	}

	if (nvg__cross(dx0, dy0, dx1, dy1) > 0.0f) {
		cx = x1 + dx0 * d + dy0 * radius;
		cy = y1 + dy0 * d + -dx0 * radius;
		a0 = nvg__atan2f(dx0, -dy0);
		a1 = nvg__atan2f(-dx1, dy1);
		dir = NVG_CW;
		//		printf("CW c=(%f, %f) a0=%fT- a1=%fT-\n", cx, cy, a0/NVG_PI*180.0f, a1/NVG_PI*180.0f);
	}
	else {
		cx = x1 + dx0 * d + -dy0 * radius;
		cy = y1 + dy0 * d + dx0 * radius;
		a0 = nvg__atan2f(-dx0, dy0);
		a1 = nvg__atan2f(dx1, -dy1);
		dir = NVG_CCW;
		//		printf("CCW c=(%f, %f) a0=%fT- a1=%fT-\n", cx, cy, a0/NVG_PI*180.0f, a1/NVG_PI*180.0f);
	}
	arc(cx, cy, radius, a0, a1, dir);
}

void NVGcontext::closePath()
{
	float vals[] = { NVG_CLOSE };
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::pathWinding(int dir)
{
	float vals[] = { NVG_WINDING, (float)dir };
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::arc(float cx, float cy, float r, float a0, float a1, int dir)
{
	float a = 0, da = 0, hda = 0, kappa = 0;
	float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
	float px = 0, py = 0, ptanx = 0, ptany = 0;
	float vals[3 + 5 * 7 + 100];
	int i, ndivs, nvals;
	int move = m_commands.getSize() > 0 ? NVG_LINETO : NVG_MOVETO;

	// Clamp angles
	da = a1 - a0;
	if (dir == NVG_CW) {
		if (nvg__absf(da) >= NVG_PI * 2) {
			da = NVG_PI * 2;
		}
		else {
			while (da < 0.0f) da += NVG_PI * 2;
		}
	}
	else {
		if (nvg__absf(da) >= NVG_PI * 2) {
			da = -NVG_PI * 2;
		}
		else {
			while (da > 0.0f) da -= NVG_PI * 2;
		}
	}

	// Split arc into max 90 degree segments.
	ndivs = nvg__maxi(1, nvg__mini((int)(nvg__absf(da) / (NVG_PI * 0.5f) + 0.5f), 5));
	hda = (da / (float)ndivs) / 2.0f;
	kappa = nvg__absf(4.0f / 3.0f * (1.0f - nvg__cosf(hda)) / nvg__sinf(hda));

	if (dir == NVG_CCW)
		kappa = -kappa;

	nvals = 0;
	for (i = 0; i <= ndivs; i++) {
		a = a0 + da * (i / (float)ndivs);
		dx = nvg__cosf(a);
		dy = nvg__sinf(a);
		x = cx + dx * r;
		y = cy + dy * r;
		tanx = -dy * r * kappa;
		tany = dx * r * kappa;

		if (i == 0) {
			vals[nvals++] = (float)move;
			vals[nvals++] = x;
			vals[nvals++] = y;
		}
		else {
			vals[nvals++] = NVG_BEZIERTO;
			vals[nvals++] = px + ptanx;
			vals[nvals++] = py + ptany;
			vals[nvals++] = x - tanx;
			vals[nvals++] = y - tany;
			vals[nvals++] = x;
			vals[nvals++] = y;
		}
		px = x;
		py = y;
		ptanx = tanx;
		ptany = tany;
	}
	appendCommands(vals, nvals);
}

void NVGcontext::rect(float x, float y, float w, float h)
{
	float vals[] = {
		NVG_MOVETO, x,y,
		NVG_LINETO, x,y + h,
		NVG_LINETO, x + w,y + h,
		NVG_LINETO, x + w,y,
		NVG_CLOSE
	};
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::roundedRect(float x, float y, float w, float h, float r)
{
	roundedRectVarying(x, y, w, h, r, r, r, r);
}

void NVGcontext::roundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
{
	if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f) {
		rect(x, y, w, h);
		return;
	}
	else {
		float halfw = nvg__absf(w) * 0.5f;
		float halfh = nvg__absf(h) * 0.5f;
		// A roundedRect radius describes a circular corner. Clamping X and Y
		// independently turns oversized corners on narrow rectangles into ellipses
		// and can consume the entire straight edge. Keep a single clamped radius
		// for both axes so the resulting geometry remains a rounded rectangle.
		float maxRadius = nvg__minf(halfw, halfh);
		float rBL = nvg__minf(nvg__maxf(radBottomLeft, 0.0f), maxRadius);
		float rBR = nvg__minf(nvg__maxf(radBottomRight, 0.0f), maxRadius);
		float rTR = nvg__minf(nvg__maxf(radTopRight, 0.0f), maxRadius);
		float rTL = nvg__minf(nvg__maxf(radTopLeft, 0.0f), maxRadius);
		float rxBL = rBL * nvg__signf(w), ryBL = rBL * nvg__signf(h);
		float rxBR = rBR * nvg__signf(w), ryBR = rBR * nvg__signf(h);
		float rxTR = rTR * nvg__signf(w), ryTR = rTR * nvg__signf(h);
		float rxTL = rTL * nvg__signf(w), ryTL = rTL * nvg__signf(h);
		float vals[] = {
			NVG_MOVETO, x, y + ryTL,
			NVG_LINETO, x, y + h - ryBL,
			NVG_BEZIERTO, x, y + h - ryBL * (1 - NVG_KAPPA90), x + rxBL * (1 - NVG_KAPPA90), y + h, x + rxBL, y + h,
			NVG_LINETO, x + w - rxBR, y + h,
			NVG_BEZIERTO, x + w - rxBR * (1 - NVG_KAPPA90), y + h, x + w, y + h - ryBR * (1 - NVG_KAPPA90), x + w, y + h - ryBR,
			NVG_LINETO, x + w, y + ryTR,
			NVG_BEZIERTO, x + w, y + ryTR * (1 - NVG_KAPPA90), x + w - rxTR * (1 - NVG_KAPPA90), y, x + w - rxTR, y,
			NVG_LINETO, x + rxTL, y,
			NVG_BEZIERTO, x + rxTL * (1 - NVG_KAPPA90), y, x, y + ryTL * (1 - NVG_KAPPA90), x, y + ryTL,
			NVG_CLOSE
		};
		appendCommands(vals, NVG_COUNTOF(vals));
	}
}

void NVGcontext::ellipse(float cx, float cy, float rx, float ry)
{
	float vals[] = {
		NVG_MOVETO, cx - rx, cy,
		NVG_BEZIERTO, cx - rx, cy + ry * NVG_KAPPA90, cx - rx * NVG_KAPPA90, cy + ry, cx, cy + ry,
		NVG_BEZIERTO, cx + rx * NVG_KAPPA90, cy + ry, cx + rx, cy + ry * NVG_KAPPA90, cx + rx, cy,
		NVG_BEZIERTO, cx + rx, cy - ry * NVG_KAPPA90, cx + rx * NVG_KAPPA90, cy - ry, cx, cy - ry,
		NVG_BEZIERTO, cx - rx * NVG_KAPPA90, cy - ry, cx - rx, cy - ry * NVG_KAPPA90, cx - rx, cy,
		NVG_CLOSE
	};
	appendCommands(vals, NVG_COUNTOF(vals));
}

void NVGcontext::circle(float cx, float cy, float r)
{
	ellipse(cx, cy, r, r);
}

void NVGcontext::debugDumpPathCache()
{
	int j;
	const int pathCount = static_cast<int>(m_pathCache->getNumPaths());
	printf("Dumping %d cached paths\n", pathCount);
	for (int i = 0; i < pathCount; i++) {
		const NVGpath* path = &m_pathCache->getPath(i);
		printf(" - Path %d\n", i);
		if (path->nfill) {
			printf("   - fill: %d\n", path->nfill);
			for (j = 0; j < path->nfill; j++)
				printf("%f\t%f\n", path->fill[j].x, path->fill[j].y);
		}
		if (path->nstroke) {
			printf("   - stroke: %d\n", path->nstroke);
			for (j = 0; j < path->nstroke; j++)
				printf("%f\t%f\n", path->stroke[j].x, path->stroke[j].y);
		}
	}
}

void NVGcontext::fill()
{
	NVGstate* state = getState();
	if (nvg__isScissorEmpty(state->scissor))
		return;
	NVGpaint fillPaint = state->fill;
	flattenPaths();
	const int pathCount = static_cast<int>(m_pathCache->getNumPaths());
	if (m_config.edgeAntiAlias && state->shapeAntiAlias)
		m_pathCache->expandFill(m_fringeWidth, NVG_MITER, 2.4f, m_fringeWidth);
	else
		m_pathCache->expandFill(0.0f, NVG_MITER, 2.4f, m_fringeWidth);

	// Apply global alpha
	fillPaint.innerColor.a *= state->alpha;
	fillPaint.outerColor.a *= state->alpha;

	m_renderer->setZIndex(state->zIndex);
	m_renderer->fill(fillPaint, state->compositeOperation, state->scissor, m_fringeWidth,
		m_pathCache->bounds, m_pathCache->getPaths(), pathCount);

	// Count triangles
	for (int i = 0; i < pathCount; i++) {
		NVGpath &path = m_pathCache->getPath(i);
		m_fillTriCount += path.nfill - 2;
		m_fillTriCount += path.nstroke - 2;
		m_drawCallCount += 2;
	}
}

void NVGcontext::stroke()
{
	NVGstate* state = getState();
	if (nvg__isScissorEmpty(state->scissor))
		return;
	float scale = nvg__getAverageScale(state->xform);
	float strokeWidth = nvg__clampf(state->strokeWidth * scale, 0.0f, 200.0f);
	NVGpaint strokePaint = state->stroke;
	if (strokeWidth < m_fringeWidth) {
		// If the stroke width is less than pixel size, use alpha to emulate coverage.
		// Since coverage is area, scale by alpha*alpha.
		float alpha = nvg__clampf(strokeWidth / m_fringeWidth, 0.0f, 1.0f);
		strokePaint.innerColor.a *= alpha * alpha;
		strokePaint.outerColor.a *= alpha * alpha;
		strokeWidth = m_fringeWidth;
	}

	// Apply global alpha
	strokePaint.innerColor.a *= state->alpha;
	strokePaint.outerColor.a *= state->alpha;

	flattenPaths();
	const int pathCount = static_cast<int>(m_pathCache->getNumPaths());

	if (m_config.edgeAntiAlias && state->shapeAntiAlias)
		m_pathCache->expandStroke(strokeWidth * 0.5f, m_fringeWidth, state->lineCap, state->lineJoin, state->miterLimit, m_tessTol);
	else
		m_pathCache->expandStroke(strokeWidth * 0.5f, 0.0f, state->lineCap, state->lineJoin, state->miterLimit, m_tessTol);

	m_renderer->setZIndex(state->zIndex);
	m_renderer->stroke(strokePaint, state->compositeOperation, state->scissor, m_fringeWidth,
		strokeWidth, m_pathCache->getPaths(), pathCount);

	// Count triangles
	for (int i = 0; i < pathCount; i++) {
		NVGpath& path = m_pathCache->getPath(i);
		m_strokeTriCount += path.nstroke - 2;
		m_drawCallCount++;
	}
}

// Add fonts
int NVGcontext::createFont(const char* name, const char* filename)
{
	return fonsAddFont(m_fs, name, filename, 0);
}

int NVGcontext::createFontAtIndex(const char* name, const char* filename, const int fontIndex)
{
	return fonsAddFont(m_fs, name, filename, fontIndex);
}

int NVGcontext::createFontMem(const char* name, unsigned char* data, int ndata, int freeData)
{
	return fonsAddFontMem(m_fs, name, data, ndata, freeData, 0);
}

int NVGcontext::createFontMemAtIndex(const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex)
{
	return fonsAddFontMem(m_fs, name, data, ndata, freeData, fontIndex);
}

int NVGcontext::findFont(const char* name)
{
	if (!name)
		return -1;

	return fonsGetFontByName(m_fs, name);
}

int NVGcontext::addFallbackFontId(int baseFont, int fallbackFont)
{
	if (baseFont == -1 || fallbackFont == -1)
		return 0;

	return fonsAddFallbackFont(m_fs, baseFont, fallbackFont);
}

int NVGcontext::addFallbackFont(const char* baseFont, const char* fallbackFont)
{
	return addFallbackFontId(findFont(baseFont), findFont(fallbackFont));
}

int NVGcontext::deleteFont(int font)
{
	//TODO KD: implement font deletion in fons
	return 1;
}

void NVGcontext::resetFallbackFontsId(int baseFont)
{
	fonsResetFallbackFont(m_fs, baseFont);
}

void NVGcontext::resetFallbackFonts(const char* baseFont)
{
	resetFallbackFontsId(findFont(baseFont));
}

// State setting
void NVGcontext::setFontSize(float size)
{
	NVGstate* state = getState();
	state->fontSize = size;
}

void NVGcontext::setFontBlur(float blur)
{
	NVGstate* state = getState();
	state->fontBlur = blur;
}

void NVGcontext::setTextLetterSpacing(float spacing)
{

	NVGstate* state = getState();
	state->letterSpacing = spacing;
}

void NVGcontext::setTextLineHeight(float lineHeight)
{
	NVGstate* state = getState();
	state->lineHeight = lineHeight;
}

void NVGcontext::setTextAlign(int align)
{
	NVGstate* state = getState();
	state->textAlign = align;
}

void NVGcontext::setFontFaceId(int font)
{
	NVGstate* state = getState();
	state->fontId = font;
}

void NVGcontext::setFontFace(const char* font)
{
	NVGstate* state = getState();
	state->fontId = fonsGetFontByName(m_fs, font);
}

static float nvg__quantize(float a, float d)
{
	return ((int)(a / d + 0.5f)) * d;
}

static float nvg__getFontScale(NVGstate* state)
{
	return nvg__minf(nvg__quantize(nvg__getAverageScale(state->xform), 0.01f), 4.0f);
}

static int nvg__isTransformFlipped(const float* xform)
{
	float det = xform[0] * xform[3] - xform[2] * xform[1];
	return(det < 0);
}

void NVGcontext::nvg__flushTextTexture()
{
	int dirty[4];
	if (fonsValidateTexture(m_fs, dirty)) {
		NVGhandle fontImage = m_fontImages[m_fontImageIdx];
		// Update texture
		if (fontImage.isValid()) {
			int iw, ih;
			const unsigned char* data = fonsGetTextureData(m_fs, &iw, &ih);
			int x = dirty[0];
			int y = dirty[1];
			int w = dirty[2] - dirty[0];
			int h = dirty[3] - dirty[1];
			m_renderer->updateTexture(fontImage, x, y, w, h, data);
		}
	}
}

int NVGcontext::nvg__allocTextAtlas()
{
	int iw, ih;
	nvg__flushTextTexture();
	if (m_fontImageIdx >= NVG_MAX_FONTIMAGES - 1)
		return 0;

	// if next fontImage already have a texture
	if (m_fontImages[m_fontImageIdx + 1].isValid())
		getImageSize(m_fontImages[m_fontImageIdx + 1], &iw, &ih);
	else { // calculate the new font image size and create it.
		getImageSize(m_fontImages[m_fontImageIdx], &iw, &ih);
		if (iw > ih)
			ih *= 2;
		else
			iw *= 2;
		if (iw > NVG_MAX_FONTIMAGE_SIZE || ih > NVG_MAX_FONTIMAGE_SIZE)
			iw = ih = NVG_MAX_FONTIMAGE_SIZE;
		m_fontImages[m_fontImageIdx + 1] = m_renderer->createTexture(NVG_TEXTURE_ALPHA, iw, ih, 0, NULL);
	}
	++m_fontImageIdx;
	fonsResetAtlas(m_fs, iw, ih);
	return 1;
}

void NVGcontext::nvg__renderText(NVGvertex* verts, int nverts)
{
	NVGstate* state = getState();
	if (nvg__isScissorEmpty(state->scissor))
		return;
	NVGpaint paint = state->fill;

	// Render triangles.
	paint.image = m_fontImages[m_fontImageIdx];

	// Apply global alpha
	paint.innerColor.a *= state->alpha;
	paint.outerColor.a *= state->alpha;

	m_renderer->setZIndex(state->zIndex);
	m_renderer->triangles(paint, state->compositeOperation, state->scissor, verts, nverts, m_fringeWidth);

	m_drawCallCount++;
	m_textTriCount += nverts / 3;
}

float NVGcontext::text(float x, float y, const char* string, const char* end)
{

	NVGstate* state = getState();
	FONStextIter iter, prevIter;
	FONSquad q;
	NVGvertex* verts;
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;
	int cverts = 0;
	int nverts = 0;
	int isFlipped = nvg__isTransformFlipped(state->xform);

	if (end == NULL)
		end = string + strlen(string);

	if (state->fontId == FONS_INVALID) return x;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);

	cverts = nvg__maxi(2, (int)(end - string)) * 6; // conservative estimate.
	verts = m_pathCache->allocTempVerts(cverts);
	if (verts == NULL)
		return x;

	fonsTextIterInit(m_fs, &iter, x * scale, y * scale, string, end, FONS_GLYPH_BITMAP_REQUIRED);
	prevIter = iter;
	while (fonsTextIterNext(m_fs, &iter, &q)) {
		float c[4 * 2];
		if (iter.prevGlyphIndex == -1) { // can not retrieve glyph?
			if (nverts != 0) {
				nvg__renderText(verts, nverts);
				nverts = 0;
			}
			if (!nvg__allocTextAtlas())
				break; // no memory :(

			iter = prevIter;
			fonsTextIterNext(m_fs, &iter, &q); // try again
			if (iter.prevGlyphIndex == -1) // still can not find glyph?
				break;
		}
		prevIter = iter;
		if (isFlipped) {
			float tmp;

			tmp = q.y0; q.y0 = q.y1; q.y1 = tmp;
			tmp = q.t0; q.t0 = q.t1; q.t1 = tmp;
		}
		// Transform corners.
		NVGcontext::TransformPoint(&c[0], &c[1], state->xform, q.x0 * invscale, q.y0 * invscale);
		NVGcontext::TransformPoint(&c[2], &c[3], state->xform, q.x1 * invscale, q.y0 * invscale);
		NVGcontext::TransformPoint(&c[4], &c[5], state->xform, q.x1 * invscale, q.y1 * invscale);
		NVGcontext::TransformPoint(&c[6], &c[7], state->xform, q.x0 * invscale, q.y1 * invscale);
		// Create triangles
		if (nverts + 6 <= cverts) {
			nvg__vset(&verts[nverts], c[0], c[1], q.s0, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[4], c[5], q.s1, q.t1); nverts++;
			nvg__vset(&verts[nverts], c[2], c[3], q.s1, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[0], c[1], q.s0, q.t0); nverts++;
			nvg__vset(&verts[nverts], c[6], c[7], q.s0, q.t1); nverts++;
			nvg__vset(&verts[nverts], c[4], c[5], q.s1, q.t1); nverts++;
		}
	}

	// TODO: add back-end bit to do this just once per frame.
	nvg__flushTextTexture();

	nvg__renderText(verts, nverts);
	return iter.nextx / scale;
}

void NVGcontext::textBox(float x, float y, float breakRowWidth, const char* string, const char* end)
{

	NVGstate* state = getState();
	NVGtextRow rows[2];
	int nrows = 0, i;
	int oldAlign = state->textAlign;
	int halign = state->textAlign & (NVG_ALIGN_LEFT | NVG_ALIGN_CENTER | NVG_ALIGN_RIGHT);
	int valign = state->textAlign & (NVG_ALIGN_TOP | NVG_ALIGN_MIDDLE | NVG_ALIGN_BOTTOM | NVG_ALIGN_BASELINE);
	float lineh = 0;

	if (state->fontId == FONS_INVALID) return;

	textMetrics(NULL, NULL, &lineh);

	state->textAlign = NVG_ALIGN_LEFT | valign;

	while ((nrows = textBreakLines(string, end, breakRowWidth, rows, 2))) {
		for (i = 0; i < nrows; i++) {
			NVGtextRow* row = &rows[i];
			if (halign & NVG_ALIGN_LEFT)
				text(x, y, row->start, row->end);
			else if (halign & NVG_ALIGN_CENTER)
				text(x + breakRowWidth * 0.5f - row->width * 0.5f, y, row->start, row->end);
			else if (halign & NVG_ALIGN_RIGHT)
				text(x + breakRowWidth - row->width, y, row->start, row->end);
			y += lineh * state->lineHeight;
		}

		//FIXME KD: potential buffer overflow 
		string = rows[nrows - 1].next;
	}

	state->textAlign = oldAlign;
}

int NVGcontext::textGlyphPositions(float x, float y, const char* string, const char* end, NVGglyphPosition* positions, int maxPositions)
{

	NVGstate* state = getState();
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;
	FONStextIter iter, prevIter;
	FONSquad q;
	int npos = 0;

	if (state->fontId == FONS_INVALID) return 0;

	if (end == NULL)
		end = string + strlen(string);

	if (string == end)
		return 0;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);

	fonsTextIterInit(m_fs, &iter, x * scale, y * scale, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
	prevIter = iter;
	while (fonsTextIterNext(m_fs, &iter, &q)) {
		if (iter.prevGlyphIndex < 0 && nvg__allocTextAtlas()) { // can not retrieve glyph?
			iter = prevIter;
			fonsTextIterNext(m_fs, &iter, &q); // try again
		}
		prevIter = iter;
		positions[npos].str = iter.str;
		positions[npos].x = iter.x * invscale;
		positions[npos].minx = nvg__minf(iter.x, q.x0) * invscale;
		positions[npos].maxx = nvg__maxf(iter.nextx, q.x1) * invscale;
		npos++;
		if (npos >= maxPositions)
			break;
	}

	return npos;
}

enum NVGcodepointType {
	NVG_SPACE,
	NVG_NEWLINE,
	NVG_CHAR,
	NVG_CJK_CHAR,
};

int NVGcontext::textBreakLines(const char* string, const char* end, float breakRowWidth, NVGtextRow* rows, int maxRows)
{

	NVGstate* state = getState();
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;
	FONStextIter iter, prevIter;
	FONSquad q;
	int nrows = 0;
	float rowStartX = 0;
	float rowWidth = 0;
	float rowMinX = 0;
	float rowMaxX = 0;
	const char* rowStart = NULL;
	const char* rowEnd = NULL;
	const char* wordStart = NULL;
	float wordStartX = 0;
	float wordMinX = 0;
	const char* breakEnd = NULL;
	float breakWidth = 0;
	float breakMaxX = 0;
	int type = NVG_SPACE, ptype = NVG_SPACE;
	unsigned int pcodepoint = 0;

	if (maxRows == 0) return 0;
	if (state->fontId == FONS_INVALID) return 0;

	if (end == NULL)
		end = string + strlen(string);

	if (string == end) return 0;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);

	breakRowWidth *= scale;

	fonsTextIterInit(m_fs, &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
	prevIter = iter;
	while (fonsTextIterNext(m_fs, &iter, &q)) {
		if (iter.prevGlyphIndex < 0 && nvg__allocTextAtlas()) { // can not retrieve glyph?
			iter = prevIter;
			fonsTextIterNext(m_fs, &iter, &q); // try again
		}
		prevIter = iter;
		switch (iter.codepoint) {
		case 9:			// \t
		case 11:		// \v
		case 12:		// \f
		case 32:		// space
		case 0x00a0:	// NBSP
			type = NVG_SPACE;
			break;
		case 10:		// \n
			type = pcodepoint == 13 ? NVG_SPACE : NVG_NEWLINE;
			break;
		case 13:		// \r
			type = pcodepoint == 10 ? NVG_SPACE : NVG_NEWLINE;
			break;
		case 0x0085:	// NEL
			type = NVG_NEWLINE;
			break;
		default:
			if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) ||
				(iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) ||
				(iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
				(iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) ||
				(iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) ||
				(iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
				type = NVG_CJK_CHAR;
			else
				type = NVG_CHAR;
			break;
		}

		if (type == NVG_NEWLINE) {
			// Always handle new lines.
			rows[nrows].start = rowStart != NULL ? rowStart : iter.str;
			rows[nrows].end = rowEnd != NULL ? rowEnd : iter.str;
			rows[nrows].width = rowWidth * invscale;
			rows[nrows].minx = rowMinX * invscale;
			rows[nrows].maxx = rowMaxX * invscale;
			rows[nrows].next = iter.next;
			nrows++;
			if (nrows >= maxRows)
				return nrows;
			// Set null break point
			breakEnd = rowStart;
			breakWidth = 0.0;
			breakMaxX = 0.0;
			// Indicate to skip the white space at the beginning of the row.
			rowStart = NULL;
			rowEnd = NULL;
			rowWidth = 0;
			rowMinX = rowMaxX = 0;
		}
		else {
			if (rowStart == NULL) {
				// Skip white space until the beginning of the line
				if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
					// The current char is the row so far
					rowStartX = iter.x;
					rowStart = iter.str;
					rowEnd = iter.next;
					rowWidth = iter.nextx - rowStartX;
					rowMinX = q.x0 - rowStartX;
					rowMaxX = q.x1 - rowStartX;
					wordStart = iter.str;
					wordStartX = iter.x;
					wordMinX = q.x0 - rowStartX;
					// Set null break point
					breakEnd = rowStart;
					breakWidth = 0.0;
					breakMaxX = 0.0;
				}
			}
			else {
				float nextWidth = iter.nextx - rowStartX;

				// track last non-white space character
				if (type == NVG_CHAR || type == NVG_CJK_CHAR) {
					rowEnd = iter.next;
					rowWidth = iter.nextx - rowStartX;
					rowMaxX = q.x1 - rowStartX;
				}
				// track last end of a word
				if (((ptype == NVG_CHAR || ptype == NVG_CJK_CHAR) && type == NVG_SPACE) || type == NVG_CJK_CHAR) {
					breakEnd = iter.str;
					breakWidth = rowWidth;
					breakMaxX = rowMaxX;
				}
				// track last beginning of a word
				if ((ptype == NVG_SPACE && (type == NVG_CHAR || type == NVG_CJK_CHAR)) || type == NVG_CJK_CHAR) {
					wordStart = iter.str;
					wordStartX = iter.x;
					wordMinX = q.x0;
				}

				// Break to new line when a character is beyond break width.
				if ((type == NVG_CHAR || type == NVG_CJK_CHAR) && nextWidth > breakRowWidth) {
					// The run length is too long, need to break to new line.
					if (breakEnd == rowStart) {
						// The current word is longer than the row length, just break it from here.
						rows[nrows].start = rowStart;
						rows[nrows].end = iter.str;
						rows[nrows].width = rowWidth * invscale;
						rows[nrows].minx = rowMinX * invscale;
						rows[nrows].maxx = rowMaxX * invscale;
						rows[nrows].next = iter.str;
						nrows++;
						if (nrows >= maxRows)
							return nrows;
						rowStartX = iter.x;
						rowStart = iter.str;
						rowEnd = iter.next;
						rowWidth = iter.nextx - rowStartX;
						rowMinX = q.x0 - rowStartX;
						rowMaxX = q.x1 - rowStartX;
						wordStart = iter.str;
						wordStartX = iter.x;
						wordMinX = q.x0 - rowStartX;
					}
					else {
						// Break the line from the end of the last word, and start new line from the beginning of the new.
						rows[nrows].start = rowStart;
						rows[nrows].end = breakEnd;
						rows[nrows].width = breakWidth * invscale;
						rows[nrows].minx = rowMinX * invscale;
						rows[nrows].maxx = breakMaxX * invscale;
						rows[nrows].next = wordStart;
						nrows++;
						if (nrows >= maxRows)
							return nrows;
						// Update row
						rowStartX = wordStartX;
						rowStart = wordStart;
						rowEnd = iter.next;
						rowWidth = iter.nextx - rowStartX;
						rowMinX = wordMinX - rowStartX;
						rowMaxX = q.x1 - rowStartX;
					}
					// Set null break point
					breakEnd = rowStart;
					breakWidth = 0.0;
					breakMaxX = 0.0;
				}
			}
		}

		pcodepoint = iter.codepoint;
		ptype = type;
	}

	// Break the line from the end of the last word, and start new line from the beginning of the new.
	if (rowStart != NULL) {
		rows[nrows].start = rowStart;
		rows[nrows].end = rowEnd;
		rows[nrows].width = rowWidth * invscale;
		rows[nrows].minx = rowMinX * invscale;
		rows[nrows].maxx = rowMaxX * invscale;
		rows[nrows].next = end;
		nrows++;
	}

	return nrows;
}

float NVGcontext::textBounds(float x, float y, const char* string, const char* end, float* bounds)
{

	NVGstate* state = getState();
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;
	float width;

	if (state->fontId == FONS_INVALID) return 0;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);

	width = fonsTextBounds(m_fs, x * scale, y * scale, string, end, bounds);
	if (bounds != NULL) {
		// Use line bounds for height.
		fonsLineBounds(m_fs, y * scale, &bounds[1], &bounds[3]);
		bounds[0] *= invscale;
		bounds[1] *= invscale;
		bounds[2] *= invscale;
		bounds[3] *= invscale;
	}
	return width * invscale;
}

void NVGcontext::textBoxBounds(float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds)
{

	NVGstate* state = getState();
	NVGtextRow rows[2];
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;
	int nrows = 0, i;
	int oldAlign = state->textAlign;
	int halign = state->textAlign & (NVG_ALIGN_LEFT | NVG_ALIGN_CENTER | NVG_ALIGN_RIGHT);
	int valign = state->textAlign & (NVG_ALIGN_TOP | NVG_ALIGN_MIDDLE | NVG_ALIGN_BOTTOM | NVG_ALIGN_BASELINE);
	float lineh = 0, rminy = 0, rmaxy = 0;
	float minx, miny, maxx, maxy;

	if (state->fontId == FONS_INVALID) {
		if (bounds != NULL)
			bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		return;
	}

	textMetrics(NULL, NULL, &lineh);

	state->textAlign = NVG_ALIGN_LEFT | valign;

	minx = maxx = x;
	miny = maxy = y;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);
	fonsLineBounds(m_fs, 0, &rminy, &rmaxy);
	rminy *= invscale;
	rmaxy *= invscale;

	while ((nrows = textBreakLines(string, end, breakRowWidth, rows, 2))) {
		for (i = 0; i < nrows; i++) {
			NVGtextRow* row = &rows[i];
			float rminx, rmaxx, dx = 0;
			// Horizontal bounds
			if (halign & NVG_ALIGN_LEFT)
				dx = 0;
			else if (halign & NVG_ALIGN_CENTER)
				dx = breakRowWidth * 0.5f - row->width * 0.5f;
			else if (halign & NVG_ALIGN_RIGHT)
				dx = breakRowWidth - row->width;
			rminx = x + row->minx + dx;
			rmaxx = x + row->maxx + dx;
			minx = nvg__minf(minx, rminx);
			maxx = nvg__maxf(maxx, rmaxx);
			// Vertical bounds.
			miny = nvg__minf(miny, y + rminy);
			maxy = nvg__maxf(maxy, y + rmaxy);

			y += lineh * state->lineHeight;
		}
		//TODO KD: potential buffer overflow
		string = rows[nrows - 1].next;
	}

	state->textAlign = oldAlign;

	if (bounds != NULL) {
		bounds[0] = minx;
		bounds[1] = miny;
		bounds[2] = maxx;
		bounds[3] = maxy;
	}
}

void NVGcontext::textMetrics(float* ascender, float* descender, float* lineh)
{

	NVGstate* state = getState();
	float scale = nvg__getFontScale(state) * m_devicePxRatio;
	float invscale = 1.0f / scale;

	if (state->fontId == FONS_INVALID) return;

	fonsSetSize(m_fs, state->fontSize * scale);
	fonsSetSpacing(m_fs, state->letterSpacing * scale);
	fonsSetBlur(m_fs, state->fontBlur * scale);
	fonsSetAlign(m_fs, state->textAlign);
	fonsSetFont(m_fs, state->fontId);

	fonsVertMetrics(m_fs, ascender, descender, lineh);
	if (ascender != NULL)
		*ascender *= invscale;
	if (descender != NULL)
		*descender *= invscale;
	if (lineh != NULL)
		*lineh *= invscale;
}

static NVGcolor nvg__mulAlpha(NVGcolor color, float alpha)
{
	color.a *= alpha;
	return color;
}

void NVGcontext::textBlur(float x, float y, const char* string, const char* end, const NVGblurStyle& style)
{
	NVG_NOTUSED(x);
	NVG_NOTUSED(y);
	NVG_NOTUSED(string);
	NVG_NOTUSED(end);
	NVG_NOTUSED(style);
}

void NVGcontext::glowRect(float x, float y, float w, float h, float r, const NVGglowStyle& style)
{
	if (w <= 0.0f || h <= 0.0f || style.radius <= 0.0f || style.color.a <= 0.0f) {
		return;
	}

	save();

	const float spread = nvg__maxf(style.radius, 0.0f);
	const float radius = nvg__minf(nvg__maxf(r, 0.0f), nvg__minf(w, h) * 0.5f);
	const float outerRadius = radius + spread;
	const float intensity = nvg__clampf(style.intensity, 0.0f, 1.0f);
	NVGcolor inner = nvg__mulAlpha(style.color, intensity);
	NVGcolor outer = inner;
	outer.a = 0.0f;

	NVGpaint glow = NVGpaint::boxGradient(
		x - spread, y - spread,
		w + spread * 2.0f, h + spread * 2.0f,
		outerRadius, spread,
		inner, outer);

	beginPath();
	roundedRect(x - spread, y - spread, w + spread * 2.0f, h + spread * 2.0f, outerRadius);
	roundedRect(x, y, w, h, radius);
	pathWinding(NVG_HOLE);
	fillPaint(glow);
	fill();

	restore();
}

NVGhandle NVGcontext::createRenderTarget(const NVGrenderTargetDesc& desc)
{
	return m_renderer ? m_renderer->createRenderTarget(desc) : NVGhandle();
}

void NVGcontext::deleteRenderTarget(NVGhandle target)
{
	if (m_renderer)
		m_renderer->deleteRenderTarget(target);
}

void NVGcontext::setRenderTarget(NVGhandle target)
{
	if (m_renderer) {
		m_renderer->setRenderTarget(target);
		m_boundRenderTarget = target;
	}
}

NVGhandle NVGcontext::getRenderTargetImage(NVGhandle target)
{
	return m_renderer ? m_renderer->getRenderTargetImage(target) : NVGhandle();
}

NVGhandle NVGcontext::createShader(const NVGshaderDesc& desc)
{
	return m_renderer ? m_renderer->createShader(desc) : NVGhandle();
}

void NVGcontext::deleteShader(NVGhandle shader)
{
	if (m_renderer)
		m_renderer->deleteShader(shader);
}

NVGhandle NVGcontext::createUniform(NVGhandle shader, const char* name, NVGuniformDataType type, uint32_t count)
{
	return m_renderer ? m_renderer->createUniform(shader, name, type, count) : NVGhandle();
}

NVGhandle NVGcontext::findUniform(NVGhandle shader, const char* name)
{
	return m_renderer ? m_renderer->findUniform(shader, name) : NVGhandle();
}

void NVGcontext::setUniformData(NVGhandle uniform, const void* data, size_t size)
{
	if (m_renderer)
		m_renderer->setUniformData(uniform, data, size);
}

void NVGcontext::deleteUniform(NVGhandle uniform)
{
	if (m_renderer)
		m_renderer->deleteUniform(uniform);
}

NVGhandle NVGcontext::getBuiltinBlurShader()
{
	return m_renderer ? m_renderer->getBuiltinBlurShader() : NVGhandle();
}

NVGhandle NVGcontext::getBuiltinGlassShader()
{
	return m_renderer ? m_renderer->getBuiltinGlassShader() : NVGhandle();
}

void NVGcontext::drawTriangles(const NVGcustomDraw& draw, const NVGvertex* verts, int nverts)
{
	NVGstate* state = getState();
	if (m_renderer && !nvg__isScissorEmpty(state->scissor) &&
		draw.shader.isValid() && verts && nverts > 0) {
		m_renderer->setZIndex(state->zIndex);
		m_renderer->drawCustomTriangles(draw, state->compositeOperation, state->scissor, verts, nverts, m_fringeWidth);
	}
}

NVGcontext::NVGcontext(std::unique_ptr<NVGrenderer> renderer, const NVGcontextConfig& config)
	: m_renderer()
	, m_rendererCreated(0)
	, m_config()
	, m_commandx(0.0f)
	, m_commandy(0.0f)
	, m_pathCache(NULL)
	, m_tessTol(0.0f)
	, m_distTol(0.0f)
	, m_fringeWidth(0.0f)
	, m_devicePxRatio(0.0f)
	, m_fs(NULL)
	, m_fontImageIdx(0)
	, m_drawCallCount(0)
	, m_fillTriCount(0)
	, m_strokeTriCount(0)
	, m_textTriCount(0)
	, m_commands(NVG_INIT_COMMANDS_SIZE)
{
	m_renderer = std::move(renderer);
	m_rendererCreated = 1;
	m_config = config;

	//TODO KD: remove this cycle later (constructors already provide 'invalid' state)
	//for (int i = 0; i < NVG_MAX_FONTIMAGES; ++i)
	//	m_fontImages[i].invalidate();

	FONSparams fontParams;
	if (!m_renderer)
		throw std::invalid_argument("renderer is null");

	m_pathCache = allocPathCache();
	if (!m_pathCache)
		throw std::bad_alloc();

	save();
	reset();
	setDevicePixelRatio(1.0f);

	// Init font rendering
	memset(&fontParams, 0, sizeof(fontParams));
	fontParams.width = NVG_INIT_FONTIMAGE_SIZE;
	fontParams.height = NVG_INIT_FONTIMAGE_SIZE;
	fontParams.flags = FONS_ZERO_TOPLEFT;
	fontParams.renderCreate = NULL;
	fontParams.renderUpdate = NULL;
	fontParams.renderDraw = NULL;
	fontParams.renderDelete = NULL;
	fontParams.userPtr = NULL;
	m_fs = fonsCreateInternal(&fontParams);
	if (!m_fs)
		throw std::bad_alloc();

	// Create font texture
	m_fontImages[0] = m_renderer->createTexture(NVG_TEXTURE_ALPHA, fontParams.width, fontParams.height, 0, NULL);
	if (!m_fontImages[0].isValid())
		throw std::bad_alloc();

	m_fontImageIdx = 0;
}

NVGcontext::~NVGcontext()
{
	int i;
	if (m_pathCache != NULL)
		deletePathCache(m_pathCache);

	if (m_fs)
		fonsDeleteInternal(m_fs);

	for (i = 0; i < NVG_MAX_FONTIMAGES; i++) {
		if (m_fontImages[i].isValid()) {
			deleteImage(m_fontImages[i]);
			m_fontImages[i].invalidate();
		}
	}
}

void* nvg__alloc(size_t size, const char* pfile, int line)
{
	printf("nvg__alloc(): %s:%d, size=%zu\n", pfile, line, size);
	return std::malloc(size);
}

void nvg__free(void* ptr, const char* pfile, int line)
{
	printf("nvg__free(): %s:%d, ptr=0x%p\n", pfile, line, ptr);
	std::free(ptr);
}

void* nvg__alloc(size_t size)
{
	printf("nvg__alloc(): size=%zu\n", size);
	return std::malloc(size);
}

void nvg__free(void* ptr)
{
	printf("nvg__free(): ptr=0x%p\n", ptr);
	std::free(ptr);
}

NVGpath* NVGpathCache::getLastPath()
{
	if (m_paths.getSize() > 0)
		return &m_paths[m_paths.getSize() - 1];

	return nullptr;
}

NVGpoint* NVGpathCache::getLastPoint()
{
	if (m_points.getSize() > 0)
		return &m_points[m_points.getSize() - 1];

	return nullptr;
}

bool NVGpathCache::addPath()
{
	NVGpath* path = m_paths.appendBack();
	if (path) {
		memset(path, 0, sizeof(*path));
		path->first = (int)m_points.getSize(); //TODO KD: use one type without narrowing conversion
		path->winding = NVG_CCW;
		return true;
	}
	return false;
}

bool NVGpathCache::addPoint(float x, float y, int flags, float distTol)
{
	NVGpoint* pt;
	NVGpath* path = getLastPath();
	if (!path)
		return false;

	if (path->count > 0 && m_points.getSize() > 0) {
		pt = getLastPoint();
		if (_nvgPtEquals(pt->x, pt->y, x, y, distTol)) {
			pt->flags |= flags;
			return true;
		}
	}

	pt = m_points.appendBack();
	if (!pt)
		return false;

	memset(pt, 0, sizeof(*pt));
	pt->x = x;
	pt->y = y;
	pt->flags = (unsigned char)flags;
	path->count++;
	return true;
}

NVGvertex* NVGpathCache::allocTempVerts(int nverts)
{
	if(m_verts.availCapacity(nverts))
		return m_verts.getData();

	return nullptr;
}

void NVGpathCache::updateCache(float distanceTolerance)
{
	NVGpath* path;
	NVGpoint* pts, *p0, *p1;
	float area;

	/* reset bounds */
	bounds[0] = bounds[1] = 1e6f;
	bounds[2] = bounds[3] = -1e6f;

	// Calculate the direction and length of line segments.
	for (size_t j = 0; j < m_paths.getSize(); j++) {
		path = &m_paths[j];
		pts = &m_points[path->first];

		// If the first and last points are the same, remove the last, mark as closed path.
		p0 = &pts[path->count - 1];
		p1 = &pts[0];
		if (_nvgPtEquals(p0->x, p0->y, p1->x, p1->y, distanceTolerance)) {
			path->count--;
			p0 = &pts[path->count - 1];
			path->closed = 1;
		}

		// Enforce winding.
		if (path->count > 2) {
			area = nvg__polyArea(pts, path->count);
			if (path->winding == NVG_CCW && area < 0.0f)
				nvg__polyReverse(pts, path->count);
			if (path->winding == NVG_CW && area > 0.0f)
				nvg__polyReverse(pts, path->count);
		}

		for (int i = 0; i < path->count; i++) {
			// Calculate segment direction and length
			p0->dx = p1->x - p0->x;
			p0->dy = p1->y - p0->y;
			p0->len = nvg__normalize(&p0->dx, &p0->dy);
			// Update bounds
			bounds[0] = nvg__minf(bounds[0], p0->x);
			bounds[1] = nvg__minf(bounds[1], p0->y);
			bounds[2] = nvg__maxf(bounds[2], p0->x);
			bounds[3] = nvg__maxf(bounds[3], p0->y);
			// Advance
			p0 = p1++;
		}
	}
}

void NVGpathCache::calculateJoins(float w, int lineJoin, float miterLimit)
{
	int i, j;
	const int pathCount = static_cast<int>(getNumPaths());
	float iw = 0.0f;

	if (w > 0.0f)
		iw = 1.0f / w;

	// Calculate which joins needs extra vertices to append, and gather vertex count.
	for (i = 0; i < pathCount; i++) {
		NVGpath* path = &m_paths[i];
		NVGpoint* pts = &m_points[path->first];
		NVGpoint* p0 = &pts[path->count - 1];
		NVGpoint* p1 = &pts[0];
		int nleft = 0;

		path->nbevel = 0;

		for (j = 0; j < path->count; j++) {
			float dlx0, dly0, dlx1, dly1, dmr2, cross, limit;
			dlx0 = p0->dy;
			dly0 = -p0->dx;
			dlx1 = p1->dy;
			dly1 = -p1->dx;
			// Calculate extrusions
			p1->dmx = (dlx0 + dlx1) * 0.5f;
			p1->dmy = (dly0 + dly1) * 0.5f;
			dmr2 = p1->dmx * p1->dmx + p1->dmy * p1->dmy;
			if (dmr2 > 0.000001f) {
				float scale = 1.0f / dmr2;
				if (scale > 600.0f) {
					scale = 600.0f;
				}
				p1->dmx *= scale;
				p1->dmy *= scale;
			}

			// Clear flags, but keep the corner.
			p1->flags = (p1->flags & NVG_PT_CORNER) ? NVG_PT_CORNER : 0;

			// Keep track of left turns.
			cross = p1->dx * p0->dy - p0->dx * p1->dy;
			if (cross > 0.0f) {
				nleft++;
				p1->flags |= NVG_PT_LEFT;
			}

			// Calculate if we should use bevel or miter for inner join.
			limit = nvg__maxf(1.01f, nvg__minf(p0->len, p1->len) * iw);
			if ((dmr2 * limit * limit) < 1.0f)
				p1->flags |= NVG_PR_INNERBEVEL;

			// Check to see if the corner needs to be beveled.
			if (p1->flags & NVG_PT_CORNER) {
				if ((dmr2 * miterLimit * miterLimit) < 1.0f || lineJoin == NVG_BEVEL || lineJoin == NVG_ROUND) {
					p1->flags |= NVG_PT_BEVEL;
				}
			}

			if ((p1->flags & (NVG_PT_BEVEL | NVG_PR_INNERBEVEL)) != 0)
				path->nbevel++;

			p0 = p1++;
		}

		path->convex = (nleft == path->count) ? 1 : 0;
	}
}

bool NVGpathCache::expandFill(float w, int lineJoin, float miterLimit, float fringeWidth)
{
	NVGvertex* verts;
	NVGvertex* dst;
	int cverts, convex, i, j;
	const int pathCount = static_cast<int>(getNumPaths());
	float aa = fringeWidth;
	int fringe = w > 0.0f;

	calculateJoins(w, lineJoin, miterLimit);

	// Calculate max vertex usage.
	cverts = 0;
	for (i = 0; i < pathCount; i++) {
		NVGpath* path = &m_paths[i];
		cverts += path->count + path->nbevel + 1;
		if (fringe)
			cverts += (path->count + path->nbevel * 5 + 1) * 2; // plus one for loop
	}

	verts = allocTempVerts(cverts);
	if (verts == NULL)
		return false;

	convex = (getNumPaths() == 1 && m_paths[0].convex);

	for (i = 0; i < pathCount; i++) {
		NVGpath* path = &m_paths[i];
		NVGpoint* pts = &m_points[path->first];
		NVGpoint* p0;
		NVGpoint* p1;
		float rw, lw, woff;
		float ru, lu;

		// Calculate shape vertices.
		woff = 0.5f * aa;
		dst = verts;
		path->fill = dst;

		if (fringe) {
			// Looping
			p0 = &pts[path->count - 1];
			p1 = &pts[0];
			for (j = 0; j < path->count; ++j) {
				if (p1->flags & NVG_PT_BEVEL) {
					float dlx0 = p0->dy;
					float dly0 = -p0->dx;
					float dlx1 = p1->dy;
					float dly1 = -p1->dx;
					if (p1->flags & NVG_PT_LEFT) {
						float lx = p1->x + p1->dmx * woff;
						float ly = p1->y + p1->dmy * woff;
						nvg__vset(dst, lx, ly, 0.5f, 1); dst++;
					}
					else {
						float lx0 = p1->x + dlx0 * woff;
						float ly0 = p1->y + dly0 * woff;
						float lx1 = p1->x + dlx1 * woff;
						float ly1 = p1->y + dly1 * woff;
						nvg__vset(dst, lx0, ly0, 0.5f, 1); dst++;
						nvg__vset(dst, lx1, ly1, 0.5f, 1); dst++;
					}
				}
				else {
					nvg__vset(dst, p1->x + (p1->dmx * woff), p1->y + (p1->dmy * woff), 0.5f, 1); dst++;
				}
				p0 = p1++;
			}
		}
		else {
			for (j = 0; j < path->count; ++j) {
				nvg__vset(dst, pts[j].x, pts[j].y, 0.5f, 1);
				dst++;
			}
		}

		path->nfill = (int)(dst - verts);
		verts = dst;

		// Calculate fringe
		if (fringe) {
			lw = w + woff;
			rw = w - woff;
			lu = 0;
			ru = 1;
			dst = verts;
			path->stroke = dst;

			// Create only half a fringe for convex shapes so that
			// the shape can be rendered without stenciling.
			if (convex) {
				lw = woff;	// This should generate the same vertex as fill inset above.
				lu = 0.5f;	// Set outline fade at middle.
			}

			// Looping
			p0 = &pts[path->count - 1];
			p1 = &pts[0];

			for (j = 0; j < path->count; ++j) {
				if ((p1->flags & (NVG_PT_BEVEL | NVG_PR_INNERBEVEL)) != 0) {
					dst = nvg__bevelJoin(dst, p0, p1, lw, rw, lu, ru, fringeWidth);
				}
				else {
					nvg__vset(dst, p1->x + (p1->dmx * lw), p1->y + (p1->dmy * lw), lu, 1); dst++;
					nvg__vset(dst, p1->x - (p1->dmx * rw), p1->y - (p1->dmy * rw), ru, 1); dst++;
				}
				p0 = p1++;
			}

			// Loop it
			nvg__vset(dst, verts[0].x, verts[0].y, lu, 1); dst++;
			nvg__vset(dst, verts[1].x, verts[1].y, ru, 1); dst++;

			path->nstroke = (int)(dst - verts);
			verts = dst;
		}
		else {
			path->stroke = NULL;
			path->nstroke = 0;
		}
	}
	return true;
}

bool NVGpathCache::expandStroke(float w, float fringe, int lineCap, int lineJoin, float miterLimit, float tessTolerance)
{
	NVGvertex* verts;
	NVGvertex* dst;
	int cverts, i, j;
	const int pathCount = static_cast<int>(getNumPaths());
	float aa = fringe;//fringeWidth;
	float u0 = 0.0f, u1 = 1.0f;
	int ncap = nvg__curveDivs(w, NVG_PI, tessTolerance);	// Calculate divisions per half circle.

	w += aa * 0.5f;

	// Disable the gradient used for antialiasing when antialiasing is not used.
	if (aa == 0.0f) {
		u0 = 0.5f;
		u1 = 0.5f;
	}

	calculateJoins(w, lineJoin, miterLimit);

	// Calculate max vertex usage.
	cverts = 0;
	for (i = 0; i < pathCount; i++) {
		NVGpath* path = &m_paths[i];
		int loop = (path->closed == 0) ? 0 : 1;
		if (lineJoin == NVG_ROUND)
			cverts += (path->count + path->nbevel * (ncap + 2) + 1) * 2; // plus one for loop
		else
			cverts += (path->count + path->nbevel * 5 + 1) * 2; // plus one for loop
		if (loop == 0) {
			// space for caps
			if (lineCap == NVG_ROUND) {
				cverts += (ncap * 2 + 2) * 2;
			}
			else {
				cverts += (3 + 3) * 2;
			}
		}
	}

	verts = allocTempVerts(cverts);
	if (!verts)
		return false;

	for (i = 0; i < pathCount; i++) {
		NVGpath* path = &m_paths[i];
		NVGpoint* pts = &m_points[path->first];
		NVGpoint* p0;
		NVGpoint* p1;
		int s, e, loop;
		float dx, dy;

		path->fill = 0;
		path->nfill = 0;

		// Calculate fringe or stroke
		loop = (path->closed == 0) ? 0 : 1;
		dst = verts;
		path->stroke = dst;

		if (loop) {
			// Looping
			p0 = &pts[path->count - 1];
			p1 = &pts[0];
			s = 0;
			e = path->count;
		}
		else {
			// Add cap
			p0 = &pts[0];
			p1 = &pts[1];
			s = 1;
			e = path->count - 1;
		}

		if (loop == 0) {
			// Add cap
			dx = p1->x - p0->x;
			dy = p1->y - p0->y;
			nvg__normalize(&dx, &dy);
			if (lineCap == NVG_BUTT)
				dst = nvg__buttCapStart(dst, p0, dx, dy, w, -aa * 0.5f, aa, u0, u1);
			else if (lineCap == NVG_BUTT || lineCap == NVG_SQUARE)
				dst = nvg__buttCapStart(dst, p0, dx, dy, w, w - aa, aa, u0, u1);
			else if (lineCap == NVG_ROUND)
				dst = nvg__roundCapStart(dst, p0, dx, dy, w, ncap, aa, u0, u1);
		}

		for (j = s; j < e; ++j) {
			if ((p1->flags & (NVG_PT_BEVEL | NVG_PR_INNERBEVEL)) != 0) {
				if (lineJoin == NVG_ROUND) {
					dst = nvg__roundJoin(dst, p0, p1, w, w, u0, u1, ncap, aa);
				}
				else {
					dst = nvg__bevelJoin(dst, p0, p1, w, w, u0, u1, aa);
				}
			}
			else {
				nvg__vset(dst, p1->x + (p1->dmx * w), p1->y + (p1->dmy * w), u0, 1); dst++;
				nvg__vset(dst, p1->x - (p1->dmx * w), p1->y - (p1->dmy * w), u1, 1); dst++;
			}
			p0 = p1++;
		}

		if (loop) {
			// Loop it
			nvg__vset(dst, verts[0].x, verts[0].y, u0, 1); dst++;
			nvg__vset(dst, verts[1].x, verts[1].y, u1, 1); dst++;
		}
		else {
			// Add cap
			dx = p1->x - p0->x;
			dy = p1->y - p0->y;
			nvg__normalize(&dx, &dy);
			if (lineCap == NVG_BUTT)
				dst = nvg__buttCapEnd(dst, p1, dx, dy, w, -aa * 0.5f, aa, u0, u1);
			else if (lineCap == NVG_BUTT || lineCap == NVG_SQUARE)
				dst = nvg__buttCapEnd(dst, p1, dx, dy, w, w - aa, aa, u0, u1);
			else if (lineCap == NVG_ROUND)
				dst = nvg__roundCapEnd(dst, p1, dx, dy, w, ncap, aa, u0, u1);
		}

		path->nstroke = (int)(dst - verts);

		verts = dst;
	}
	return true;
}

void NVGpathCache::closePath()
{
	NVGpath* path = getLastPath();
	if (!path)
		return;
	path->closed = 1;
}

void NVGpathCache::pathWinding(int winding)
{
	NVGpath* path = getLastPath();
	if (!path)
		return;
	path->winding = winding;
}

void NVGpathCache::tesselateBezier(float x1, float y1, float x2, float y2,
	float x3, float y3, float x4, float y4, int level, int type, float tessTolerance)
{
	float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
	float dx, dy, d2, d3;

	if (level > 10)
		return;

	x12 = (x1 + x2) * 0.5f;
	y12 = (y1 + y2) * 0.5f;
	x23 = (x2 + x3) * 0.5f;
	y23 = (y2 + y3) * 0.5f;
	x34 = (x3 + x4) * 0.5f;
	y34 = (y3 + y4) * 0.5f;
	x123 = (x12 + x23) * 0.5f;
	y123 = (y12 + y23) * 0.5f;

	dx = x4 - x1;
	dy = y4 - y1;
	d2 = nvg__absf(((x2 - x4) * dy - (y2 - y4) * dx));
	d3 = nvg__absf(((x3 - x4) * dy - (y3 - y4) * dx));

	if ((d2 + d3) * (d2 + d3) < tessTolerance * (dx * dx + dy * dy)) {
		addPoint(x4, y4, type, tessTolerance);
		return;
	}

	/*	if (nvg__absf(x1+x3-x2-x2) + nvg__absf(y1+y3-y2-y2) + nvg__absf(x2+x4-x3-x3) + nvg__absf(y2+y4-y3-y3) < tessTol) {
			addPoint(x4, y4, type, tessTolerance);
			return;
		}*/

	x234 = (x23 + x34) * 0.5f;
	y234 = (y23 + y34) * 0.5f;
	x1234 = (x123 + x234) * 0.5f;
	y1234 = (y123 + y234) * 0.5f;

	tesselateBezier(x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0, tessTolerance);
	tesselateBezier(x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type, tessTolerance);
}
// vim: ft=cpp nu noet ts=4
