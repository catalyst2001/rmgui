//
// nvg_cmd.cpp — Evaluator for the data-driven NanoVG command buffer
//
// Dispatch is done via a static table of function pointers —
// one indirect call per command, zero branching in the hot loop.
//

#include "nvg_cmd.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────
//  Execution context passed to every handler
// ─────────────────────────────────────────────────────────────
struct NVGcmdExecCtx {
	NVGcontext*       ctx;
	const NVGcmdCell* args;     // pointer to first arg cell
	uint32_t          varmask;
	const void*       data;
	const NVGcmdLayout* layout;
};

// ─────────────────────────────────────────────────────────────
//  Variable resolvers (force-inlined on MSVC / GCC / Clang)
// ─────────────────────────────────────────────────────────────
#if defined(_MSC_VER)
# define NVG_FINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
# define NVG_FINLINE __attribute__((always_inline)) inline
#else
# define NVG_FINLINE inline
#endif

static NVG_FINLINE float resolveFloat(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return cell.f;

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return 0.0f;
	uint32_t idx = cell.u;
	if (idx >= layout->count) return 0.0f;

	const NVGcmdVar& v = (*layout)[idx];
	const uint8_t* base = (const uint8_t*)data;
	switch (v.type) {
		case NVG_VAR_FLOAT:  return *(const float*)(base + v.offset);
		case NVG_VAR_INT32:  return (float)*(const int32_t*)(base + v.offset);
		case NVG_VAR_UINT32: return (float)*(const uint32_t*)(base + v.offset);
		default:             return 0.0f;
	}
}

static NVG_FINLINE int32_t resolveInt(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return cell.i;

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return 0;
	uint32_t idx = cell.u;
	if (idx >= layout->count) return 0;

	const NVGcmdVar& v = (*layout)[idx];
	const uint8_t* base = (const uint8_t*)data;
	switch (v.type) {
		case NVG_VAR_FLOAT:  return (int32_t)*(const float*)(base + v.offset);
		case NVG_VAR_INT32:  return *(const int32_t*)(base + v.offset);
		case NVG_VAR_UINT32: return (int32_t)*(const uint32_t*)(base + v.offset);
		default:             return 0;
	}
}

static NVG_FINLINE const char* resolveString(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return "";

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return "";
	uint32_t idx = cell.u;
	if (idx >= layout->count) return "";

	const NVGcmdVar& v = (*layout)[idx];
	if (v.type != NVG_VAR_STRING) return "";
	const uint8_t* base = (const uint8_t*)data;
	const char* const* pp = (const char* const*)(base + v.offset);
	return *pp ? *pp : "";
}

static NVG_FINLINE NVGhandle resolveHandle(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return NVGhandle();

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return NVGhandle();
	uint32_t idx = cell.u;
	if (idx >= layout->count) return NVGhandle();

	const NVGcmdVar& v = (*layout)[idx];
	if (v.type != NVG_VAR_HANDLE) return NVGhandle();
	const uint8_t* base = (const uint8_t*)data;
	NVGhandle h;
	std::memcpy(&h, base + v.offset, sizeof(NVGhandle));
	return h;
}

// Shorthand macros used inside handler functions.
#define F(n) resolveFloat (e, n)
#define I(n) resolveInt   (e, n)
#define S(n) resolveString(e, n)
#define H(n) resolveHandle(e, n)

// ─────────────────────────────────────────────────────────────
//  Handler type
// ─────────────────────────────────────────────────────────────
typedef void (*NVGcmdHandler)(const NVGcmdExecCtx& e);

// ─────────────────────────────────────────────────────────────
//  Individual handlers — one per opcode
// ─────────────────────────────────────────────────────────────

// ── State ────────────────────────────────────────────────────
static void h_save   (const NVGcmdExecCtx& e) { e.ctx->save(); }
static void h_restore(const NVGcmdExecCtx& e) { e.ctx->restore(); }
static void h_reset  (const NVGcmdExecCtx& e) { e.ctx->reset(); }

// ── Transforms ───────────────────────────────────────────────
static void h_resetTransform(const NVGcmdExecCtx& e) { e.ctx->resetTransform(); }
static void h_transform (const NVGcmdExecCtx& e) { e.ctx->transform(F(0), F(1), F(2), F(3), F(4), F(5)); }
static void h_translate (const NVGcmdExecCtx& e) { e.ctx->translate(F(0), F(1)); }
static void h_rotate    (const NVGcmdExecCtx& e) { e.ctx->rotate(F(0)); }
static void h_skewX     (const NVGcmdExecCtx& e) { e.ctx->skewX(F(0)); }
static void h_skewY     (const NVGcmdExecCtx& e) { e.ctx->skewY(F(0)); }
static void h_scale     (const NVGcmdExecCtx& e) { e.ctx->scale(F(0), F(1)); }

// ── Render styles ────────────────────────────────────────────
static void h_fillColor     (const NVGcmdExecCtx& e) { e.ctx->fillColor(NVGcolor::RGBAf(F(0), F(1), F(2), F(3))); }
static void h_strokeColor   (const NVGcmdExecCtx& e) { e.ctx->strokeColor(NVGcolor::RGBAf(F(0), F(1), F(2), F(3))); }
static void h_strokeWidth   (const NVGcmdExecCtx& e) { e.ctx->StrokeWidth(F(0)); }
static void h_lineCap       (const NVGcmdExecCtx& e) { e.ctx->LineCap(I(0)); }
static void h_lineJoin      (const NVGcmdExecCtx& e) { e.ctx->LineJoin(I(0)); }
static void h_miterLimit    (const NVGcmdExecCtx& e) { e.ctx->MiterLimit(F(0)); }
static void h_globalAlpha   (const NVGcmdExecCtx& e) { e.ctx->GlobalAlpha(F(0)); }
static void h_shapeAntiAlias(const NVGcmdExecCtx& e) { e.ctx->shapeAntiAlias(I(0)); }
static void h_compositeOp   (const NVGcmdExecCtx& e) { e.ctx->globalCompositeOp(I(0)); }

// ── Scissor ──────────────────────────────────────────────────
static void h_scissor         (const NVGcmdExecCtx& e) { e.ctx->scissor(F(0), F(1), F(2), F(3)); }
static void h_intersectScissor(const NVGcmdExecCtx& e) { e.ctx->intersectScissor(F(0), F(1), F(2), F(3)); }
static void h_resetScissor    (const NVGcmdExecCtx& e) { e.ctx->resetScissor(); }

// ── Path building ────────────────────────────────────────────
static void h_beginPath  (const NVGcmdExecCtx& e) { e.ctx->beginPath(); }
static void h_moveTo     (const NVGcmdExecCtx& e) { e.ctx->moveTo(F(0), F(1)); }
static void h_lineTo     (const NVGcmdExecCtx& e) { e.ctx->lineTo(F(0), F(1)); }
static void h_bezierTo   (const NVGcmdExecCtx& e) { e.ctx->bezierTo(F(0), F(1), F(2), F(3), F(4), F(5)); }
static void h_quadTo     (const NVGcmdExecCtx& e) { e.ctx->quadTo(F(0), F(1), F(2), F(3)); }
static void h_arcTo      (const NVGcmdExecCtx& e) { e.ctx->arcTo(F(0), F(1), F(2), F(3), F(4)); }
static void h_closePath  (const NVGcmdExecCtx& e) { e.ctx->closePath(); }
static void h_pathWinding(const NVGcmdExecCtx& e) { e.ctx->pathWinding(I(0)); }

// ── Shape helpers ────────────────────────────────────────────
static void h_arc     (const NVGcmdExecCtx& e) { e.ctx->arc(F(0), F(1), F(2), F(3), F(4), I(5)); }
static void h_rect    (const NVGcmdExecCtx& e) { e.ctx->rect(F(0), F(1), F(2), F(3)); }
static void h_rrect   (const NVGcmdExecCtx& e) { e.ctx->roundedRect(F(0), F(1), F(2), F(3), F(4)); }
static void h_rrectV  (const NVGcmdExecCtx& e) { e.ctx->roundedRectVarying(F(0), F(1), F(2), F(3), F(4), F(5), F(6), F(7)); }
static void h_ellipse (const NVGcmdExecCtx& e) { e.ctx->ellipse(F(0), F(1), F(2), F(3)); }
static void h_circle  (const NVGcmdExecCtx& e) { e.ctx->circle(F(0), F(1), F(2)); }

// ── Rendering ────────────────────────────────────────────────
static void h_fill  (const NVGcmdExecCtx& e) { e.ctx->fill(); }
static void h_stroke(const NVGcmdExecCtx& e) { e.ctx->stroke(); }

// ── Fill paint (gradients / patterns) ────────────────────────
static void h_fillLinGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(4),  F(5),  F(6),  F(7));
	NVGcolor oc = NVGcolor::RGBAf(F(8),  F(9),  F(10), F(11));
	e.ctx->fillPaint(NVGpaint::linearGradient(F(0), F(1), F(2), F(3), ic, oc));
}
static void h_fillBoxGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(6),  F(7),  F(8),  F(9));
	NVGcolor oc = NVGcolor::RGBAf(F(10), F(11), F(12), F(13));
	e.ctx->fillPaint(NVGpaint::boxGradient(F(0), F(1), F(2), F(3), F(4), F(5), ic, oc));
}
static void h_fillRadGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(4),  F(5),  F(6),  F(7));
	NVGcolor oc = NVGcolor::RGBAf(F(8),  F(9),  F(10), F(11));
	e.ctx->fillPaint(NVGpaint::radialGradient(F(0), F(1), F(2), F(3), ic, oc));
}
static void h_fillImgPat(const NVGcmdExecCtx& e) {
	e.ctx->fillPaint(NVGpaint::imagePattern(F(0), F(1), F(2), F(3), F(4), H(5), F(6)));
}

// ── Stroke paint ─────────────────────────────────────────────
static void h_strokeLinGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(4),  F(5),  F(6),  F(7));
	NVGcolor oc = NVGcolor::RGBAf(F(8),  F(9),  F(10), F(11));
	e.ctx->strokePaint(NVGpaint::linearGradient(F(0), F(1), F(2), F(3), ic, oc));
}
static void h_strokeBoxGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(6),  F(7),  F(8),  F(9));
	NVGcolor oc = NVGcolor::RGBAf(F(10), F(11), F(12), F(13));
	e.ctx->strokePaint(NVGpaint::boxGradient(F(0), F(1), F(2), F(3), F(4), F(5), ic, oc));
}
static void h_strokeRadGrad(const NVGcmdExecCtx& e) {
	NVGcolor ic = NVGcolor::RGBAf(F(4),  F(5),  F(6),  F(7));
	NVGcolor oc = NVGcolor::RGBAf(F(8),  F(9),  F(10), F(11));
	e.ctx->strokePaint(NVGpaint::radialGradient(F(0), F(1), F(2), F(3), ic, oc));
}
static void h_strokeImgPat(const NVGcmdExecCtx& e) {
	e.ctx->strokePaint(NVGpaint::imagePattern(F(0), F(1), F(2), F(3), F(4), H(5), F(6)));
}

// ── Text style ───────────────────────────────────────────────
static void h_fontSize        (const NVGcmdExecCtx& e) { e.ctx->setFontSize(F(0)); }
static void h_fontBlur        (const NVGcmdExecCtx& e) { e.ctx->setFontBlur(F(0)); }
static void h_textLetterSpc   (const NVGcmdExecCtx& e) { e.ctx->setTextLetterSpacing(F(0)); }
static void h_textLineHeight  (const NVGcmdExecCtx& e) { e.ctx->setTextLineHeight(F(0)); }
static void h_textAlign       (const NVGcmdExecCtx& e) { e.ctx->setTextAlign(I(0)); }
static void h_fontFaceId      (const NVGcmdExecCtx& e) { e.ctx->setFontFaceId(I(0)); }

// ── Text rendering ───────────────────────────────────────────
static void h_text(const NVGcmdExecCtx& e) {
	e.ctx->text(F(0), F(1), S(2), nullptr);
}
static void h_textBox(const NVGcmdExecCtx& e) {
	e.ctx->textBox(F(0), F(1), F(2), S(3), nullptr);
}

#undef F
#undef I
#undef S
#undef H

// ─────────────────────────────────────────────────────────────
//  Dispatch table — indexed by NVGcmdOp, one entry per opcode
// ─────────────────────────────────────────────────────────────
static const NVGcmdHandler g_cmdDispatch[NVG_CMD__COUNT] = {
	/*  0 */ h_save,
	/*  1 */ h_restore,
	/*  2 */ h_reset,
	/*  3 */ h_resetTransform,
	/*  4 */ h_transform,
	/*  5 */ h_translate,
	/*  6 */ h_rotate,
	/*  7 */ h_skewX,
	/*  8 */ h_skewY,
	/*  9 */ h_scale,
	/* 10 */ h_fillColor,
	/* 11 */ h_strokeColor,
	/* 12 */ h_strokeWidth,
	/* 13 */ h_lineCap,
	/* 14 */ h_lineJoin,
	/* 15 */ h_miterLimit,
	/* 16 */ h_globalAlpha,
	/* 17 */ h_shapeAntiAlias,
	/* 18 */ h_compositeOp,
	/* 19 */ h_scissor,
	/* 20 */ h_intersectScissor,
	/* 21 */ h_resetScissor,
	/* 22 */ h_beginPath,
	/* 23 */ h_moveTo,
	/* 24 */ h_lineTo,
	/* 25 */ h_bezierTo,
	/* 26 */ h_quadTo,
	/* 27 */ h_arcTo,
	/* 28 */ h_closePath,
	/* 29 */ h_pathWinding,
	/* 30 */ h_arc,
	/* 31 */ h_rect,
	/* 32 */ h_rrect,
	/* 33 */ h_rrectV,
	/* 34 */ h_ellipse,
	/* 35 */ h_circle,
	/* 36 */ h_fill,
	/* 37 */ h_stroke,
	/* 38 */ h_fillLinGrad,
	/* 39 */ h_fillBoxGrad,
	/* 40 */ h_fillRadGrad,
	/* 41 */ h_fillImgPat,
	/* 42 */ h_strokeLinGrad,
	/* 43 */ h_strokeBoxGrad,
	/* 44 */ h_strokeRadGrad,
	/* 45 */ h_strokeImgPat,
	/* 46 */ h_fontSize,
	/* 47 */ h_fontBlur,
	/* 48 */ h_textLetterSpc,
	/* 49 */ h_textLineHeight,
	/* 50 */ h_textAlign,
	/* 51 */ h_fontFaceId,
	/* 52 */ h_text,
	/* 53 */ h_textBox,
};

// ─────────────────────────────────────────────────────────────
//  Evaluator — hot loop: table lookup + indirect call, no branching
// ─────────────────────────────────────────────────────────────

void nvgEval(NVGcontext& ctx,
             const NVGcmdBuf& buf,
             const void* data,
             const NVGcmdLayout* layout)
{
	const NVGcmdCell* cells = buf.cells.data();
	const uint32_t total    = (uint32_t)buf.cells.size();
	const uint8_t* argTbl   = nvgCmdArgCount();
	uint32_t pc = 0;

	NVGcmdExecCtx e;
	e.ctx    = &ctx;
	e.data   = data;
	e.layout = layout;

	while (pc < total) {
		const uint32_t opVal = cells[pc].u;
		if (opVal >= NVG_CMD__COUNT) break;
		pc++;

		const uint8_t nargs = argTbl[opVal];
		if (nargs > 0) {
			if (pc >= total) break;
			e.varmask = cells[pc].u;
			pc++;
		} else {
			e.varmask = 0;
		}
		if (pc + nargs > total) break;

		e.args = cells + pc;
		g_cmdDispatch[opVal](e);   // ← one indirect call, no branches
		pc += nargs;
	}
}

// ─────────────────────────────────────────────────────────────
//  NVGcontext::eval  (forwarding helper)
// ─────────────────────────────────────────────────────────────
void NVGcontext::eval(const NVGcmdBuf& buf,
                      const void* data,
                      const NVGcmdLayout* layout)
{
	nvgEval(*this, buf, data, layout);
}

// ─────────────────────────────────────────────────────────────
//  Opcode / variable-type name tables
// ─────────────────────────────────────────────────────────────

static const char* const g_opNames[NVG_CMD__COUNT] = {
	"save",                    //  0
	"restore",                 //  1
	"reset",                   //  2
	"reset_transform",         //  3
	"transform",               //  4
	"translate",               //  5
	"rotate",                  //  6
	"skew_x",                  //  7
	"skew_y",                  //  8
	"scale",                   //  9
	"fill_color",              // 10
	"stroke_color",            // 11
	"stroke_width",            // 12
	"line_cap",                // 13
	"line_join",               // 14
	"miter_limit",             // 15
	"global_alpha",            // 16
	"shape_anti_alias",        // 17
	"composite_op",            // 18
	"scissor",                 // 19
	"intersect_scissor",       // 20
	"reset_scissor",           // 21
	"begin_path",              // 22
	"move_to",                 // 23
	"line_to",                 // 24
	"bezier_to",               // 25
	"quad_to",                 // 26
	"arc_to",                  // 27
	"close_path",              // 28
	"path_winding",            // 29
	"arc",                     // 30
	"rect",                    // 31
	"rounded_rect",            // 32
	"rounded_rect_varying",    // 33
	"ellipse",                 // 34
	"circle",                  // 35
	"fill",                    // 36
	"stroke",                  // 37
	"fill_linear_gradient",    // 38
	"fill_box_gradient",       // 39
	"fill_radial_gradient",    // 40
	"fill_image_pattern",      // 41
	"stroke_linear_gradient",  // 42
	"stroke_box_gradient",     // 43
	"stroke_radial_gradient",  // 44
	"stroke_image_pattern",    // 45
	"font_size",               // 46
	"font_blur",               // 47
	"text_letter_spacing",     // 48
	"text_line_height",        // 49
	"text_align",              // 50
	"font_face_id",            // 51
	"text",                    // 52
	"text_box",                // 53
};

const char* nvgCmdOpName(uint32_t op)
{
	if (op >= NVG_CMD__COUNT) return "unknown";
	return g_opNames[op];
}

uint32_t nvgCmdOpFromName(const char* name)
{
	for (uint32_t i = 0; i < NVG_CMD__COUNT; i++)
		if (std::strcmp(g_opNames[i], name) == 0) return i;
	return NVG_CMD__COUNT;
}

const char* nvgCmdVarTypeName(NVGcmdVarType type)
{
	switch (type) {
	case NVG_VAR_FLOAT:  return "float";
	case NVG_VAR_INT32:  return "int";
	case NVG_VAR_UINT32: return "uint";
	case NVG_VAR_STRING: return "string";
	case NVG_VAR_HANDLE: return "handle";
	default:             return "float";
	}
}

NVGcmdVarType nvgCmdVarTypeFromName(const char* name)
{
	if (std::strcmp(name, "float")  == 0) return NVG_VAR_FLOAT;
	if (std::strcmp(name, "int")    == 0) return NVG_VAR_INT32;
	if (std::strcmp(name, "uint")   == 0) return NVG_VAR_UINT32;
	if (std::strcmp(name, "string") == 0) return NVG_VAR_STRING;
	if (std::strcmp(name, "handle") == 0) return NVG_VAR_HANDLE;
	return NVG_VAR_FLOAT;
}

// ─────────────────────────────────────────────────────────────
//  Binary format helpers
// ─────────────────────────────────────────────────────────────
// Binary layout (all values little-endian):
//   [4] magic 'NVCB'    [4] format version (1)
//   [4] schema version  [4+pad] className
//   [4+pad] elementName [4] numVars
//   per var:  [4+pad] name  [4] type
//   [4] numCells        [numCells*4] cell data

static const uint32_t NVG_BINARY_MAGIC   = ('N') | ('V' << 8) | ('C' << 16) | ('B' << 24);
static const uint32_t NVG_BINARY_VERSION = 1;

static bool ioWriteU32(NVGio& io, uint32_t v) {
	return io.write(&v, 4);
}

static bool ioReadU32(NVGio& io, uint32_t& v) {
	return io.read(&v, 4) == 4;
}

static bool ioWriteStr(NVGio& io, const char* str) {
	uint32_t len = (uint32_t)std::strlen(str);
	if (!ioWriteU32(io, len)) return false;
	if (len > 0 && !io.write(str, len)) return false;
	uint32_t pad = ((len + 3u) & ~3u) - len;
	if (pad > 0) {
		static const char zeros[4] = {};
		if (!io.write(zeros, pad)) return false;
	}
	return true;
}

static bool ioReadStr(NVGio& io, char* buf, size_t bufSize) {
	uint32_t len;
	if (!ioReadU32(io, len)) return false;
	if ((size_t)len >= bufSize) return false;
	uint32_t paddedLen = (len + 3u) & ~3u;
	if (len > 0 && io.read(buf, len) != len) return false;
	buf[len] = '\0';
	uint32_t pad = paddedLen - len;
	if (pad > 0) {
		char discard[4];
		if (io.read(discard, pad) != pad) return false;
	}
	return true;
}

// ─────────────────────────────────────────────────────────────
//  Binary serialization
// ─────────────────────────────────────────────────────────────

bool NVGcmdBuf::saveBinary(NVGio& io) const
{
	if (!ioWriteU32(io, NVG_BINARY_MAGIC))   return false;
	if (!ioWriteU32(io, NVG_BINARY_VERSION)) return false;

	// Meta
	if (!ioWriteU32(io, meta.version))        return false;
	if (!ioWriteStr(io, meta.className))      return false;
	if (!ioWriteStr(io, meta.elementName))    return false;

	// Variable info
	uint32_t nv = (uint32_t)meta.vars.size();
	if (!ioWriteU32(io, nv)) return false;
	for (uint32_t i = 0; i < nv; i++) {
		if (!ioWriteStr(io, meta.vars[i].name))           return false;
		if (!ioWriteU32(io, (uint32_t)meta.vars[i].type)) return false;
	}

	// Command cells
	uint32_t nc = (uint32_t)cells.size();
	if (!ioWriteU32(io, nc)) return false;
	if (nc > 0 && !io.write(cells.data(), nc * sizeof(NVGcmdCell)))
		return false;

	return true;
}

bool NVGcmdBuf::loadBinary(NVGio& io)
{
	clear();

	// Header
	uint32_t magic, fmtVer;
	if (!ioReadU32(io, magic)  || magic  != NVG_BINARY_MAGIC)   return false;
	if (!ioReadU32(io, fmtVer) || fmtVer != NVG_BINARY_VERSION) return false;

	// Meta
	if (!ioReadU32(io, meta.version))                           return false;
	if (!ioReadStr(io, meta.className,   NVG_CMD_MAX_NAME))    return false;
	if (!ioReadStr(io, meta.elementName, NVG_CMD_MAX_NAME))    return false;

	// Variable info
	uint32_t nv;
	if (!ioReadU32(io, nv))  return false;
	if (nv > 4096)           return false;  // sanity limit
	meta.vars.resize(nv);
	for (uint32_t i = 0; i < nv; i++) {
		if (!ioReadStr(io, meta.vars[i].name, NVG_CMD_MAX_NAME)) return false;
		uint32_t t;
		if (!ioReadU32(io, t)) return false;
		meta.vars[i].type = (NVGcmdVarType)t;
	}

	// Command cells
	uint32_t nc;
	if (!ioReadU32(io, nc)) return false;
	if (nc > 0x1000000u)    return false;  // 16M cells max (~64 MB)
	cells.resize(nc);
	if (nc > 0 && io.read(cells.data(), nc * sizeof(NVGcmdCell)) != nc * sizeof(NVGcmdCell))
		return false;

	return true;
}

// ─────────────────────────────────────────────────────────────
//  Text serialization
// ─────────────────────────────────────────────────────────────
// Text format example:
//   # NVG Widget Schema
//   @class  ButtonWidget
//   @name   primaryButton
//   @version 1
//   @var x float
//   @var y float
//
//   begin_path
//   rounded_rect $x $y $w $h 5
//   fill_color 0.2 0.3 0.8 1
//   fill

bool NVGcmdBuf::saveText(NVGio& io) const
{
	std::vector<char> out;
	out.reserve(cells.size() * 16);

	auto putStr  = [&](const char* s) { size_t n = std::strlen(s); out.insert(out.end(), s, s + n); };
	auto putChar = [&](char ch)       { out.push_back(ch); };
	auto putLine = [&](const char* s) { putStr(s); putChar('\n'); };

	char tmp[256];

	// Header
	putLine("# NVG Widget Schema (text format v1)");

	std::snprintf(tmp, sizeof(tmp), "@class %s", meta.className);
	putLine(tmp);
	std::snprintf(tmp, sizeof(tmp), "@name %s", meta.elementName);
	putLine(tmp);
	std::snprintf(tmp, sizeof(tmp), "@version %u", meta.version);
	putLine(tmp);

	// Variable declarations
	for (size_t i = 0; i < meta.vars.size(); i++) {
		std::snprintf(tmp, sizeof(tmp), "@var %s %s",
		              meta.vars[i].name,
		              nvgCmdVarTypeName(meta.vars[i].type));
		putLine(tmp);
	}
	putChar('\n');

	// Command stream
	const uint8_t*  argTbl  = nvgCmdArgCount();
	const uint16_t* intMask = nvgCmdArgIntMask();
	const NVGcmdCell* c = cells.data();
	uint32_t total = (uint32_t)cells.size();
	uint32_t pc = 0;

	while (pc < total) {
		uint32_t opVal = c[pc].u;
		if (opVal >= NVG_CMD__COUNT) break;
		pc++;

		putStr(nvgCmdOpName(opVal));

		uint8_t nargs = argTbl[opVal];
		if (nargs > 0) {
			uint32_t varmask = c[pc].u;
			pc++;
			uint16_t imask = intMask[opVal];

			for (uint8_t k = 0; k < nargs; k++) {
				putChar(' ');
				if ((varmask >> k) & 1) {
					// Variable reference
					uint32_t vi = c[pc + k].u;
					putChar('$');
					if (vi < meta.vars.size())
						putStr(meta.vars[vi].name);
					else {
						std::snprintf(tmp, sizeof(tmp), "%u", vi);
						putStr(tmp);
					}
				} else if ((imask >> k) & 1) {
					// Integer literal
					std::snprintf(tmp, sizeof(tmp), "%d", c[pc + k].i);
					putStr(tmp);
				} else {
					// Float literal
					std::snprintf(tmp, sizeof(tmp), "%.7g", (double)c[pc + k].f);
					putStr(tmp);
				}
			}
			pc += nargs;
		}
		putChar('\n');
	}

	return io.write(out.data(), out.size());
}

bool NVGcmdBuf::loadText(NVGio& io)
{
	clear();

	// Read entire stream into memory
	std::vector<char> buf;
	{
		char chunk[4096];
		size_t n;
		while ((n = io.read(chunk, sizeof(chunk))) > 0)
			buf.insert(buf.end(), chunk, chunk + n);
	}
	if (buf.empty()) return false;

	const char* p   = buf.data();
	const char* end = p + buf.size();

	auto skipWS  = [&]() { while (p < end && (*p == ' ' || *p == '\t')) p++; };
	auto atEOL   = [&]() -> bool { return p >= end || *p == '\n' || *p == '\r'; };
	auto skipEOL = [&]() {
		if (p < end && *p == '\r') p++;
		if (p < end && *p == '\n') p++;
	};
	auto readTok = [&](char* tok, size_t maxLen) -> bool {
		skipWS();
		if (atEOL()) return false;
		size_t i = 0;
		while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
			if (i < maxLen - 1) tok[i++] = *p;
			p++;
		}
		tok[i] = '\0';
		return i > 0;
	};
	auto skipLine = [&]() {
		while (p < end && *p != '\n' && *p != '\r') p++;
		skipEOL();
	};

	char token[256];
	const uint8_t*  argTbl     = nvgCmdArgCount();
	const uint16_t* intMaskTbl = nvgCmdArgIntMask();

	while (p < end) {
		skipWS();
		if (atEOL()) { skipEOL(); continue; }
		if (*p == '#') { skipLine(); continue; }

		if (!readTok(token, sizeof(token))) { skipLine(); continue; }

		// ── Meta directives ─────────────────────────────────────
		if (token[0] == '@') {
			if (std::strcmp(token, "@class") == 0) {
				if (readTok(token, sizeof(token))) {
					std::strncpy(meta.className, token, NVG_CMD_MAX_NAME - 1);
					meta.className[NVG_CMD_MAX_NAME - 1] = '\0';
				}
			} else if (std::strcmp(token, "@name") == 0) {
				if (readTok(token, sizeof(token))) {
					std::strncpy(meta.elementName, token, NVG_CMD_MAX_NAME - 1);
					meta.elementName[NVG_CMD_MAX_NAME - 1] = '\0';
				}
			} else if (std::strcmp(token, "@version") == 0) {
				if (readTok(token, sizeof(token)))
					meta.version = (uint32_t)std::strtoul(token, nullptr, 10);
			} else if (std::strcmp(token, "@var") == 0) {
				char varName[NVG_CMD_MAX_NAME] = {};
				char varType[64] = {};
				if (readTok(varName, sizeof(varName)) &&
				    readTok(varType, sizeof(varType)))
					meta.addVar(varName, nvgCmdVarTypeFromName(varType));
			}
			skipLine();
			continue;
		}

		// ── Command ─────────────────────────────────────────────
		uint32_t op = nvgCmdOpFromName(token);
		if (op >= NVG_CMD__COUNT) { skipLine(); continue; }

		NVGcmdCell c;
		c.u = op;
		cells.push_back(c);

		uint8_t nargs = argTbl[op];
		if (nargs > 0) {
			uint16_t imask = intMaskTbl[op];
			uint32_t varmask = 0;
			std::vector<NVGcmdCell> argCells(nargs);

			for (uint8_t k = 0; k < nargs; k++) {
				char argTok[256] = {};
				if (!readTok(argTok, sizeof(argTok))) break;

				if (argTok[0] == '$') {
					varmask |= (1u << k);
					int vi = meta.findVar(argTok + 1);
					argCells[k].u = (vi >= 0) ? (uint32_t)vi : 0;
				} else if ((imask >> k) & 1) {
					argCells[k].i = (int32_t)std::strtol(argTok, nullptr, 10);
				} else {
					argCells[k].f = std::strtof(argTok, nullptr);
				}
			}

			c.u = varmask;
			cells.push_back(c);
			for (uint8_t k = 0; k < nargs; k++)
				cells.push_back(argCells[k]);
		}

		skipLine();
	}

	return !cells.empty();
}
