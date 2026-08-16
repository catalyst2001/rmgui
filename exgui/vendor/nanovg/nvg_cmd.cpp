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
#include <algorithm>
#include <unordered_set>
#include <utility>

// ─────────────────────────────────────────────────────────────
//  Execution context passed to every handler
// ─────────────────────────────────────────────────────────────
struct NVGcmdExecCtx {
	NVGcontext*         ctx;
	const NVGcmdCell*   args;     // pointer to first arg cell
	uint32_t            varmask;
	const void*         data;
	const NVGcmdLayout* layout;
	const NVGcmdBuf*    buf;      // needed for property resolution
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

template<class T>
static NVG_FINLINE T nvgCmdLoadValue(const void* data, uint32_t offset)
{
	T value{};
	if (data)
		std::memcpy(&value, static_cast<const uint8_t*>(data) + offset,
			sizeof(T));
	return value;
}

// ── Property-aware resolution helpers ────────────────────────

// Resolve the state index for a given property.
// Returns 0 if the property is constant or the state variable
// cannot be read (fallback to first value).
static NVG_FINLINE uint32_t resolvePropState(
	const NVGcmdExecCtx& e, const NVGcmdProperty& prop)
{
	if (prop.stateVarIndex == ~0u) return 0;   // constant

	uint32_t si = prop.stateVarIndex;
	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout || si >= layout->count) return 0;

	const NVGcmdVar& sv = (*layout)[si];
	uint32_t state;
	switch (sv.type) {
		case NVG_VAR_UINT32: state = nvgCmdLoadValue<uint32_t>(data, sv.offset); break;
		case NVG_VAR_INT32:  state = (uint32_t)nvgCmdLoadValue<int32_t>(data, sv.offset); break;
		case NVG_VAR_FLOAT:  state = (uint32_t)nvgCmdLoadValue<float>(data, sv.offset); break;
		default:             return 0;
	}
	if (state >= (uint32_t)prop.values.size()) return 0;
	return state;
}

static NVG_FINLINE float resolveFloat(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return cell.f;

	uint32_t idx = cell.u;

	// Property reference (high bit set)
	if (idx & NVG_CMD_PROP_BIT) {
		uint32_t pi = idx & ~NVG_CMD_PROP_BIT;
		if (!e.buf || pi >= (uint32_t)e.buf->props.size()) return 0.0f;
		const NVGcmdProperty& prop = e.buf->props[pi];
		return prop.values[resolvePropState(e, prop)].f;
	}

	// Data-bound variable
	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return 0.0f;
	if (idx >= layout->count) return 0.0f;

	const NVGcmdVar& v = (*layout)[idx];
	switch (v.type) {
		case NVG_VAR_FLOAT:  return nvgCmdLoadValue<float>(data, v.offset);
		case NVG_VAR_INT32:  return (float)nvgCmdLoadValue<int32_t>(data, v.offset);
		case NVG_VAR_UINT32: return (float)nvgCmdLoadValue<uint32_t>(data, v.offset);
		default:             return 0.0f;
	}
}

static NVG_FINLINE int32_t resolveInt(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return cell.i;

	uint32_t idx = cell.u;

	if (idx & NVG_CMD_PROP_BIT) {
		uint32_t pi = idx & ~NVG_CMD_PROP_BIT;
		if (!e.buf || pi >= (uint32_t)e.buf->props.size()) return 0;
		const NVGcmdProperty& prop = e.buf->props[pi];
		return prop.values[resolvePropState(e, prop)].i;
	}

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return 0;
	if (idx >= layout->count) return 0;

	const NVGcmdVar& v = (*layout)[idx];
	switch (v.type) {
		case NVG_VAR_FLOAT:  return (int32_t)nvgCmdLoadValue<float>(data, v.offset);
		case NVG_VAR_INT32:  return nvgCmdLoadValue<int32_t>(data, v.offset);
		case NVG_VAR_UINT32: return (int32_t)nvgCmdLoadValue<uint32_t>(data, v.offset);
		default:             return 0;
	}
}

static NVG_FINLINE const char* resolveString(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return "";

	uint32_t idx = cell.u;

	// Properties cannot hold strings — fall through to data vars
	if (idx & NVG_CMD_PROP_BIT) return "";

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return "";
	if (idx >= layout->count) return "";

	const NVGcmdVar& v = (*layout)[idx];
	if (v.type != NVG_VAR_STRING) return "";
	const char* value = nvgCmdLoadValue<const char*>(data, v.offset);
	return value ? value : "";
}

static NVG_FINLINE NVGhandle resolveHandle(const NVGcmdExecCtx& e, uint32_t n)
{
	const NVGcmdCell& cell = e.args[n];
	if (!((e.varmask >> n) & 1)) return NVGhandle();

	uint32_t idx = cell.u;

	// Properties cannot hold handles
	if (idx & NVG_CMD_PROP_BIT) return NVGhandle();

	const void* data = e.data;
	const NVGcmdLayout* layout = e.layout;
	if (!data || !layout) return NVGhandle();
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

// ── Layer ordering ───────────────────────────────────────
static void h_setZIndex(const NVGcmdExecCtx& e) {
	e.ctx->setZIndex(I(0));
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
	/* 54 */ h_setZIndex,
};

namespace {

enum class NVGcmdExpectedType {
	number,
	integer,
	string,
	handle
};

NVGcmdExpectedType expectedType(uint32_t op, uint32_t argument)
{
	if ((op == NVG_CMD_TEXT && argument == 2) ||
		(op == NVG_CMD_TEXT_BOX && argument == 3))
		return NVGcmdExpectedType::string;
	if ((op == NVG_CMD_FILL_IMAGE_PATTERN ||
		op == NVG_CMD_STROKE_IMAGE_PATTERN) && argument == 5)
		return NVGcmdExpectedType::handle;
	return ((nvgCmdArgIntMask()[op] >> argument) & 1u)
		? NVGcmdExpectedType::integer : NVGcmdExpectedType::number;
}

bool compatibleType(NVGcmdExpectedType expected, NVGcmdVarType actual)
{
	switch (expected) {
	case NVGcmdExpectedType::string: return actual == NVG_VAR_STRING;
	case NVGcmdExpectedType::handle: return actual == NVG_VAR_HANDLE;
	case NVGcmdExpectedType::integer:
	case NVGcmdExpectedType::number:
		return actual == NVG_VAR_FLOAT || actual == NVG_VAR_INT32 ||
			actual == NVG_VAR_UINT32;
	default: return false;
	}
}

NVGcmdValidationResult validationError(NVGcmdValidationCode code,
	uint32_t cell, const char* message)
{
	NVGcmdValidationResult result;
	result.code = code;
	result.cell_index = cell;
	result.message = message;
	return result;
}

bool validVarType(NVGcmdVarType type)
{
	return type >= NVG_VAR_FLOAT && type <= NVG_VAR_HANDLE;
}

struct NVGcmdResolvedLocation {
	uint32_t op = NVG_CMD__COUNT;
	uint32_t maskCell = ~0u;
	uint32_t argumentCell = ~0u;
};

bool resolveLocation(const NVGcmdBuf& buf,
	const NVGcmdArgLocation& location,
	NVGcmdResolvedLocation& resolved) noexcept
{
	if (!location || location.streamRevision != buf.streamRevision)
		return false;
	const uint8_t* argument_counts = nvgCmdArgCount();
	uint32_t cursor = 0;
	while (cursor < buf.cells.size()) {
		const uint32_t command_cell = cursor;
		const uint32_t op = buf.cells[cursor++].u;
		if (op >= NVG_CMD__COUNT)
			return false;
		const uint32_t argument_count = argument_counts[op];
		uint32_t mask_cell = ~0u;
		if (argument_count > 0) {
			if (cursor >= buf.cells.size())
				return false;
			mask_cell = cursor++;
		}
		if (cursor + argument_count > buf.cells.size())
			return false;
		if (command_cell == location.commandCell) {
			if (location.argumentIndex >= argument_count)
				return false;
			resolved.op = op;
			resolved.maskCell = mask_cell;
			resolved.argumentCell = cursor + location.argumentIndex;
			return true;
		}
		if (command_cell > location.commandCell)
			return false;
		cursor += argument_count;
	}
	return false;
}

} // namespace

bool NVGcmdBuf::locateArgument(uint32_t commandIndex,
	uint32_t argumentIndex, NVGcmdArgLocation& location) const noexcept
{
	const uint8_t* argument_counts = nvgCmdArgCount();
	uint32_t cursor = 0;
	uint32_t command = 0;
	while (cursor < cells.size()) {
		const uint32_t command_cell = cursor;
		const uint32_t op = cells[cursor++].u;
		if (op >= NVG_CMD__COUNT)
			return false;
		const uint32_t argument_count = argument_counts[op];
		if (command == commandIndex)
			return locateArgumentAtCell(command_cell, argumentIndex, location);
		if (argument_count > 0)
			++cursor;
		if (cursor + argument_count > cells.size())
			return false;
		cursor += argument_count;
		++command;
	}
	return false;
}

bool NVGcmdBuf::locateArgumentAtCell(uint32_t commandCell,
	uint32_t argumentIndex, NVGcmdArgLocation& location) const noexcept
{
	NVGcmdArgLocation candidate;
	candidate.commandCell = commandCell;
	candidate.argumentIndex = argumentIndex;
	candidate.streamRevision = streamRevision;
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, candidate, resolved))
		return false;
	location = candidate;
	return true;
}

bool NVGcmdBuf::inspectArgument(const NVGcmdArgLocation& location,
	NVGcmdArgumentInfo& info) const noexcept
{
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, location, resolved))
		return false;
	const uint32_t mask = cells[resolved.maskCell].u;
	const bool is_reference = ((mask >> location.argumentIndex) & 1u) != 0;
	const NVGcmdCell& cell = cells[resolved.argumentCell];
	info = {};
	info.op = static_cast<NVGcmdOp>(resolved.op);
	info.argumentIndex = location.argumentIndex;
	info.value = cell;
	const NVGcmdExpectedType expected = expectedType(resolved.op,
		location.argumentIndex);
	info.type = expected == NVGcmdExpectedType::integer
		? NVG_VAR_INT32 : expected == NVGcmdExpectedType::string
		? NVG_VAR_STRING : expected == NVGcmdExpectedType::handle
		? NVG_VAR_HANDLE : NVG_VAR_FLOAT;
	if (!is_reference)
		return true;
	if ((cell.u & NVG_CMD_PROP_BIT) != 0) {
		info.source = NVGcmdArgSource::property;
		info.referenceIndex = cell.u & ~NVG_CMD_PROP_BIT;
		if (info.referenceIndex < props.size())
			info.type = props[info.referenceIndex].type;
	}
	else {
		info.source = NVGcmdArgSource::variable;
		info.referenceIndex = cell.u;
		if (info.referenceIndex < meta.vars.size())
			info.type = meta.vars[info.referenceIndex].type;
	}
	return true;
}

bool NVGcmdBuf::setLiteralFloat(const NVGcmdArgLocation& location,
	float value) noexcept
{
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, location, resolved) ||
		expectedType(resolved.op, location.argumentIndex) !=
			NVGcmdExpectedType::number)
		return false;
	cells[resolved.maskCell].u &= ~(1u << location.argumentIndex);
	cells[resolved.argumentCell].f = value;
	return true;
}

bool NVGcmdBuf::setLiteralInt(const NVGcmdArgLocation& location,
	int32_t value) noexcept
{
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, location, resolved) ||
		expectedType(resolved.op, location.argumentIndex) !=
			NVGcmdExpectedType::integer)
		return false;
	cells[resolved.maskCell].u &= ~(1u << location.argumentIndex);
	cells[resolved.argumentCell].i = value;
	return true;
}

bool NVGcmdBuf::bindVariable(const NVGcmdArgLocation& location,
	uint32_t variableIndex) noexcept
{
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, location, resolved) ||
		variableIndex >= meta.vars.size() ||
		!compatibleType(expectedType(resolved.op, location.argumentIndex),
			meta.vars[variableIndex].type))
		return false;
	cells[resolved.maskCell].u |= 1u << location.argumentIndex;
	cells[resolved.argumentCell].u = variableIndex;
	return true;
}

bool NVGcmdBuf::bindVariable(const NVGcmdArgLocation& location,
	const char* variableName) noexcept
{
	const int index = meta.findVar(variableName);
	return index >= 0 && bindVariable(location, static_cast<uint32_t>(index));
}

bool NVGcmdBuf::bindProperty(const NVGcmdArgLocation& location,
	uint32_t propertyIndex) noexcept
{
	NVGcmdResolvedLocation resolved;
	if (!resolveLocation(*this, location, resolved) ||
		propertyIndex >= props.size() ||
		!compatibleType(expectedType(resolved.op, location.argumentIndex),
			props[propertyIndex].type))
		return false;
	cells[resolved.maskCell].u |= 1u << location.argumentIndex;
	cells[resolved.argumentCell].u = propertyIndex | NVG_CMD_PROP_BIT;
	return true;
}

bool NVGcmdBuf::bindProperty(const NVGcmdArgLocation& location,
	const char* propertyName) noexcept
{
	const int index = findProperty(propertyName);
	return index >= 0 && bindProperty(location, static_cast<uint32_t>(index));
}

NVGcmdValidationResult nvgCmdValidate(const NVGcmdBuf& buf,
	const NVGcmdLayout* layout)
{
	if (layout && layout->count > 0 && !layout->vars)
		return validationError(NVGcmdValidationCode::layout_mismatch, 0,
			"runtime layout has no variable table");
	std::unordered_set<std::string> names;
	for (size_t index = 0; index < buf.meta.vars.size(); ++index) {
		const NVGcmdVarInfo& variable = buf.meta.vars[index];
		if (variable.name[0] == '\0')
			return validationError(NVGcmdValidationCode::duplicate_name, 0,
				"metadata contains an unnamed variable");
		if (!validVarType(variable.type))
			return validationError(NVGcmdValidationCode::invalid_variable_type,
				0, "metadata contains an invalid variable type");
		if (!names.emplace(variable.name).second)
			return validationError(NVGcmdValidationCode::duplicate_name, 0,
				"metadata contains duplicate variable names");
		if (layout) {
			if (index >= layout->count)
				return validationError(NVGcmdValidationCode::layout_mismatch, 0,
					"runtime layout has fewer variables than the schema");
			const NVGcmdVar& runtime_variable =
				(*layout)[static_cast<uint32_t>(index)];
			const uint32_t expected_size = nvgCmdVarTypeSize(variable.type);
			if (runtime_variable.type != variable.type)
				return validationError(NVGcmdValidationCode::layout_mismatch, 0,
					"runtime layout variable type differs from the schema");
			if (runtime_variable.size != expected_size)
				return validationError(NVGcmdValidationCode::layout_mismatch, 0,
					"runtime layout variable size differs from its declared type");
			if (runtime_variable.offset > layout->dataSize ||
				runtime_variable.size > layout->dataSize - runtime_variable.offset)
				return validationError(NVGcmdValidationCode::layout_mismatch, 0,
					"runtime layout variable exceeds the supplied data block");
		}
	}

	names.clear();
	for (const NVGcmdProperty& property : buf.props) {
		if (property.name[0] == '\0')
			return validationError(NVGcmdValidationCode::duplicate_name, 0,
				"schema contains an unnamed property");
		if (!names.emplace(property.name).second)
			return validationError(NVGcmdValidationCode::duplicate_name, 0,
				"schema contains duplicate property names");
		if (!validVarType(property.type) || property.type == NVG_VAR_STRING ||
			property.type == NVG_VAR_HANDLE || property.values.empty())
			return validationError(NVGcmdValidationCode::invalid_property, 0,
				"properties must contain at least one numeric value");
		if (property.stateVarIndex != ~0u) {
			if (property.stateVarIndex >= buf.meta.vars.size() ||
				!compatibleType(NVGcmdExpectedType::integer,
					buf.meta.vars[property.stateVarIndex].type))
				return validationError(NVGcmdValidationCode::invalid_property, 0,
					"property state selector is not a numeric schema variable");
		}
	}

	const uint8_t* arg_count = nvgCmdArgCount();
	uint32_t pc = 0;
	int32_t save_depth = 0;
	while (pc < buf.cells.size()) {
		const uint32_t command_cell = pc;
		const uint32_t op = buf.cells[pc++].u;
		if (op >= NVG_CMD__COUNT)
			return validationError(NVGcmdValidationCode::invalid_opcode,
				command_cell, "command stream contains an invalid opcode");
		if (op == NVG_CMD_SAVE)
			++save_depth;
		else if (op == NVG_CMD_RESTORE && --save_depth < 0)
			return validationError(NVGcmdValidationCode::restore_without_save,
				command_cell, "restore command has no matching save");

		const uint32_t count = arg_count[op];
		uint32_t mask = 0;
		if (count > 0) {
			if (pc >= buf.cells.size())
				return validationError(NVGcmdValidationCode::truncated_command,
					command_cell, "command is missing its variable mask");
			mask = buf.cells[pc++].u;
			const uint32_t allowed = count == 32 ? ~0u : ((1u << count) - 1u);
			if ((mask & ~allowed) != 0)
				return validationError(NVGcmdValidationCode::invalid_variable_mask,
					command_cell, "variable mask references a missing argument");
		}
		if (pc + count > buf.cells.size())
			return validationError(NVGcmdValidationCode::truncated_command,
				command_cell, "command argument list is truncated");

		for (uint32_t argument = 0; argument < count; ++argument) {
			const NVGcmdExpectedType expected = expectedType(op, argument);
			if (((mask >> argument) & 1u) == 0) {
				if (expected == NVGcmdExpectedType::string ||
					expected == NVGcmdExpectedType::handle)
					return validationError(NVGcmdValidationCode::invalid_variable_type,
						pc + argument,
						"string and handle command arguments must be variable references");
				continue;
			}
			const uint32_t reference = buf.cells[pc + argument].u;
			if ((reference & NVG_CMD_PROP_BIT) != 0) {
				const uint32_t property_index = reference & ~NVG_CMD_PROP_BIT;
				if (property_index >= buf.props.size())
					return validationError(NVGcmdValidationCode::property_out_of_range,
						pc + argument, "property reference is outside the schema");
				if (!compatibleType(expected, buf.props[property_index].type))
					return validationError(NVGcmdValidationCode::invalid_variable_type,
						pc + argument, "property type is incompatible with the command argument");
			}
			else {
				if (reference >= buf.meta.vars.size())
					return validationError(NVGcmdValidationCode::variable_out_of_range,
						pc + argument, "variable reference is outside the schema");
				if (!compatibleType(expected, buf.meta.vars[reference].type))
					return validationError(NVGcmdValidationCode::invalid_variable_type,
						pc + argument, "variable type is incompatible with the command argument");
			}
		}
		pc += count;
	}
	if (save_depth != 0)
		return validationError(NVGcmdValidationCode::unbalanced_save_restore,
			static_cast<uint32_t>(buf.cells.size()),
			"command stream leaves NanoVG save/restore state unbalanced");
	return {};
}

// ─────────────────────────────────────────────────────────────
//  Evaluator — hot loop: table lookup + indirect call, no branching
// ─────────────────────────────────────────────────────────────

NVGcmdEvalResult nvgEvalChecked(NVGcontext& ctx,
	const NVGcmdBuf& buf, const void* data, const NVGcmdLayout* layout)
{
	NVGcmdEvalResult result;
	result.validation = nvgCmdValidate(buf, layout);
	if (!result.validation)
		return result;
	result.commands_executed = nvgEvalValidated(ctx, buf, data, layout);
	return result;
}

uint32_t nvgEvalValidated(NVGcontext& ctx,
	const NVGcmdBuf& buf, const void* data, const NVGcmdLayout* layout)
{
	const NVGcmdCell* cells = buf.cells.data();
	const uint32_t total    = (uint32_t)buf.cells.size();
	const uint8_t* argTbl   = nvgCmdArgCount();
	uint32_t pc = 0;

	NVGcmdExecCtx e;
	e.ctx    = &ctx;
	e.data   = data;
	e.layout = layout;
	e.buf    = &buf;
	uint32_t commands_executed = 0;

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
		++commands_executed;
	}
	return commands_executed;
}

void nvgEval(NVGcontext& ctx,
	const NVGcmdBuf& buf, const void* data, const NVGcmdLayout* layout)
{
	(void)nvgEvalChecked(ctx, buf, data, layout);
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
	"set_zindex",              // 54
};

const char* nvgCmdOpName(uint32_t op)
{
	if (op >= NVG_CMD__COUNT) return "unknown";
	return g_opNames[op];
}

uint32_t nvgCmdOpFromName(const char* name)
{
	if (!name) return NVG_CMD__COUNT;
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
	if (!name) return NVG_VAR_FLOAT;
	if (std::strcmp(name, "float")  == 0) return NVG_VAR_FLOAT;
	if (std::strcmp(name, "int")    == 0) return NVG_VAR_INT32;
	if (std::strcmp(name, "uint")   == 0) return NVG_VAR_UINT32;
	if (std::strcmp(name, "string") == 0) return NVG_VAR_STRING;
	if (std::strcmp(name, "handle") == 0) return NVG_VAR_HANDLE;
	return NVG_VAR_FLOAT;
}

static bool nvgCmdParseVarType(const char* name, NVGcmdVarType& type)
{
	if (!name) return false;
	if (std::strcmp(name, "float") == 0) type = NVG_VAR_FLOAT;
	else if (std::strcmp(name, "int") == 0) type = NVG_VAR_INT32;
	else if (std::strcmp(name, "uint") == 0) type = NVG_VAR_UINT32;
	else if (std::strcmp(name, "string") == 0) type = NVG_VAR_STRING;
	else if (std::strcmp(name, "handle") == 0) type = NVG_VAR_HANDLE;
	else return false;
	return true;
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
static const uint32_t NVG_BINARY_VERSION = 2;

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

struct NVGcmdLoadGuard {
	NVGcmdBuf& target;
	NVGcmdBuf previous;
	bool committed = false;

	explicit NVGcmdLoadGuard(NVGcmdBuf& value)
		: target(value), previous(std::move(value)) {
		target.clear();
	}
	~NVGcmdLoadGuard() {
		if (!committed)
			target = std::move(previous);
	}
};

// ─────────────────────────────────────────────────────────────
//  Binary serialization
// ─────────────────────────────────────────────────────────────

bool NVGcmdBuf::saveBinary(NVGio& io) const
{
	if (!nvgCmdValidate(*this)) return false;
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

	// Properties (v2)
	uint32_t np = (uint32_t)props.size();
	if (!ioWriteU32(io, np)) return false;
	for (uint32_t i = 0; i < np; i++) {
		const NVGcmdProperty& pr = props[i];
		if (!ioWriteStr(io, pr.name))              return false;
		if (!ioWriteU32(io, (uint32_t)pr.type))    return false;
		if (!ioWriteU32(io, pr.stateVarIndex))     return false;
		uint32_t nv = (uint32_t)pr.values.size();
		if (!ioWriteU32(io, nv))                   return false;
		if (nv > 0 && !io.write(pr.values.data(), nv * sizeof(NVGcmdCell)))
			return false;
	}

	return true;
}

bool NVGcmdBuf::loadBinary(NVGio& io)
{
	NVGcmdLoadGuard transaction(*this);

	// Header
	uint32_t magic, fmtVer;
	if (!ioReadU32(io, magic)  || magic  != NVG_BINARY_MAGIC)   return false;
	if (!ioReadU32(io, fmtVer) || fmtVer == 0 ||
		fmtVer > NVG_BINARY_VERSION) return false;

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

	// Properties (v2+)
	if (fmtVer >= 2) {
		uint32_t np;
		if (!ioReadU32(io, np)) return false;
		if (np > 4096) return false;
		props.resize(np);
		for (uint32_t i = 0; i < np; i++) {
			NVGcmdProperty& pr = props[i];
			if (!ioReadStr(io, pr.name, NVG_CMD_MAX_NAME)) return false;
			uint32_t t;
			if (!ioReadU32(io, t)) return false;
			pr.type = (NVGcmdVarType)t;
			if (!ioReadU32(io, pr.stateVarIndex)) return false;
			uint32_t nv;
			if (!ioReadU32(io, nv)) return false;
			if (nv > 4096) return false;
			pr.values.resize(nv);
			if (nv > 0 && io.read(pr.values.data(), nv * sizeof(NVGcmdCell)) != nv * sizeof(NVGcmdCell))
				return false;
		}
	}

	if (!nvgCmdValidate(*this)) return false;
	transaction.committed = true;
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
	if (!nvgCmdValidate(*this)) return false;
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

	// Property declarations
	for (size_t i = 0; i < props.size(); i++) {
		const NVGcmdProperty& pr = props[i];
		putStr("@prop ");
		putStr(pr.name);
		putChar(' ');
		putStr(nvgCmdVarTypeName(pr.type));

		if (pr.stateVarIndex != ~0u && pr.stateVarIndex < meta.vars.size()) {
			putStr(" $");
			putStr(meta.vars[pr.stateVarIndex].name);
		}

		for (size_t j = 0; j < pr.values.size(); j++) {
			putChar(' ');
			if (pr.type == NVG_VAR_INT32) {
				std::snprintf(tmp, sizeof(tmp), "%d", pr.values[j].i);
			} else if (pr.type == NVG_VAR_UINT32) {
				std::snprintf(tmp, sizeof(tmp), "%u", pr.values[j].u);
			} else {
				std::snprintf(tmp, sizeof(tmp), "%.7g", (double)pr.values[j].f);
			}
			putStr(tmp);
		}
		putChar('\n');
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
					// Variable or property reference
					uint32_t vi = c[pc + k].u;
					if (vi & NVG_CMD_PROP_BIT) {
						// Property reference
						uint32_t pi = vi & ~NVG_CMD_PROP_BIT;
						putChar('%');
						if (pi < props.size())
							putStr(props[pi].name);
						else {
							std::snprintf(tmp, sizeof(tmp), "%u", pi);
							putStr(tmp);
						}
					} else {
						// Data variable reference
						putChar('$');
						if (vi < meta.vars.size())
							putStr(meta.vars[vi].name);
						else {
							std::snprintf(tmp, sizeof(tmp), "%u", vi);
							putStr(tmp);
						}
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
	NVGcmdLoadGuard transaction(*this);

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
	bool parse_error = false;

	while (p < end) {
		skipWS();
		if (atEOL()) { skipEOL(); continue; }
		if (*p == '#') { skipLine(); continue; }

		if (!readTok(token, sizeof(token))) { skipLine(); continue; }

		// ── Meta directives ─────────────────────────────────────
		if (token[0] == '@') {
			if (std::strcmp(token, "@class") == 0) {
				if (readTok(token, sizeof(token))) {
					nvgCmdCopyName(meta.className, NVG_CMD_MAX_NAME, token);
				}
				else parse_error = true;
			} else if (std::strcmp(token, "@name") == 0) {
				if (readTok(token, sizeof(token))) {
					nvgCmdCopyName(meta.elementName, NVG_CMD_MAX_NAME, token);
				}
				else parse_error = true;
			} else if (std::strcmp(token, "@version") == 0) {
				if (readTok(token, sizeof(token)))
					meta.version = (uint32_t)std::strtoul(token, nullptr, 10);
				else parse_error = true;
			} else if (std::strcmp(token, "@var") == 0) {
				char varName[NVG_CMD_MAX_NAME] = {};
				char varType[64] = {};
				NVGcmdVarType parsed_type = NVG_VAR_FLOAT;
				if (readTok(varName, sizeof(varName)) &&
					readTok(varType, sizeof(varType)) &&
					nvgCmdParseVarType(varType, parsed_type))
					meta.addVar(varName, parsed_type);
				else parse_error = true;
			} else if (std::strcmp(token, "@prop") == 0) {
				// @prop name type [$stateVar] val0 [val1 val2 ...]
				char propName[NVG_CMD_MAX_NAME] = {};
				char propType[64] = {};
				if (!readTok(propName, sizeof(propName)) ||
				    !readTok(propType, sizeof(propType))) {
					parse_error = true; skipLine(); continue;
				}
				NVGcmdProperty pr{};
				nvgCmdCopyName(pr.name, NVG_CMD_MAX_NAME, propName);
				if (!nvgCmdParseVarType(propType, pr.type))
					parse_error = true;
				pr.stateVarIndex = ~0u;

				// Remaining tokens: optional $stateVar + values
				std::vector<char*> valTokens;
				std::vector<std::vector<char>> valBufs;
				while (!atEOL()) {
					char vt[256] = {};
					if (!readTok(vt, sizeof(vt))) break;
					valBufs.emplace_back(vt, vt + std::strlen(vt) + 1);
				}

				size_t startIdx = 0;
				if (!valBufs.empty() && valBufs[0][0] == '$') {
					int si = meta.findVar(valBufs[0].data() + 1);
					pr.stateVarIndex = (si >= 0) ? (uint32_t)si : ~0u;
					if (si < 0) parse_error = true;
					startIdx = 1;
				}

				for (size_t vi = startIdx; vi < valBufs.size(); vi++) {
					NVGcmdCell vc{};
					const char* vs = valBufs[vi].data();
					if (pr.type == NVG_VAR_INT32)
						vc.i = (int32_t)std::strtol(vs, nullptr, 10);
					else if (pr.type == NVG_VAR_UINT32)
						vc.u = (uint32_t)std::strtoul(vs, nullptr, 10);
					else
						vc.f = std::strtof(vs, nullptr);
					pr.values.push_back(vc);
				}
				if (pr.values.empty()) {
					NVGcmdCell vc{}; vc.f = 0.0f;
					pr.values.push_back(vc);
				}
				props.push_back(std::move(pr));
			}
			else {
				parse_error = true;
			}
			skipLine();
			continue;
		}

		// ── Command ─────────────────────────────────────────────
		uint32_t op = nvgCmdOpFromName(token);
		if (op >= NVG_CMD__COUNT) {
			parse_error = true; skipLine(); continue;
		}

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
				if (!readTok(argTok, sizeof(argTok))) {
					parse_error = true; break;
				}

				if (argTok[0] == '$') {
					varmask |= (1u << k);
					int vi = meta.findVar(argTok + 1);
					argCells[k].u = (vi >= 0) ? (uint32_t)vi : 0;
					if (vi < 0) parse_error = true;
				} else if (argTok[0] == '%') {
					varmask |= (1u << k);
					int pi = findProperty(argTok + 1);
					argCells[k].u = ((pi >= 0) ? (uint32_t)pi : 0) | NVG_CMD_PROP_BIT;
					if (pi < 0) parse_error = true;
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
		skipWS();
		if (!atEOL()) parse_error = true;

		skipLine();
	}

	if (parse_error || cells.empty() || !nvgCmdValidate(*this)) return false;
	transaction.committed = true;
	return true;
}
