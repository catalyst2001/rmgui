//
// nvg_cmd.h — Data-driven command buffer for NanoVG-cpp
//
// Provides a serializable command stream with variable binding,
// enabling visual-editor–authored UI element schemas to be
// evaluated at runtime against live widget data.
//
// Format of a command in the buffer (all cells are 4 bytes):
//   0-arg commands:  [opcode]
//   N-arg commands:  [opcode] [varmask] [arg0] … [argN-1]
//
// When bit K of varmask is set, argK is a variable index resolved
// through the NVGcmdLayout at eval-time.
//

#ifndef NVG_CMD_H
#define NVG_CMD_H

#include "nanovg.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <initializer_list>
#include <cassert>
#include <cstring>
#include <string>

// ─────────────────────────────────────────────────────────────
//  I/O interface — abstract file access
// ─────────────────────────────────────────────────────────────
struct NVGio {
	virtual ~NVGio() = default;
	// Write exactly 'size' bytes. Returns true on success.
	virtual bool   write(const void* data, size_t size) = 0;
	// Read up to 'size' bytes. Returns number of bytes actually read (0 on EOF/error).
	virtual size_t read(void* data, size_t size) = 0;
};

// ─────────────────────────────────────────────────────────────
//  Opcodes
// ─────────────────────────────────────────────────────────────
enum NVGcmdOp : uint32_t {
	// ── State ────────────────────────────────────────────────
	NVG_CMD_SAVE,                    //  0  ()
	NVG_CMD_RESTORE,                 //  1  ()
	NVG_CMD_RESET,                   //  2  ()

	// ── Transforms ───────────────────────────────────────────
	NVG_CMD_RESET_TRANSFORM,         //  3  ()
	NVG_CMD_TRANSFORM,               //  4  (a, b, c, d, e, f)
	NVG_CMD_TRANSLATE,               //  5  (x, y)
	NVG_CMD_ROTATE,                  //  6  (angle)
	NVG_CMD_SKEW_X,                  //  7  (angle)
	NVG_CMD_SKEW_Y,                  //  8  (angle)
	NVG_CMD_SCALE,                   //  9  (sx, sy)

	// ── Render styles ────────────────────────────────────────
	NVG_CMD_FILL_COLOR,              // 10  (r, g, b, a)
	NVG_CMD_STROKE_COLOR,            // 11  (r, g, b, a)
	NVG_CMD_STROKE_WIDTH,            // 12  (width)
	NVG_CMD_LINE_CAP,                // 13  (cap)
	NVG_CMD_LINE_JOIN,               // 14  (join)
	NVG_CMD_MITER_LIMIT,             // 15  (limit)
	NVG_CMD_GLOBAL_ALPHA,            // 16  (alpha)
	NVG_CMD_SHAPE_ANTI_ALIAS,        // 17  (enabled)
	NVG_CMD_COMPOSITE_OP,            // 18  (op)

	// ── Scissor ──────────────────────────────────────────────
	NVG_CMD_SCISSOR,                 // 19  (x, y, w, h)
	NVG_CMD_INTERSECT_SCISSOR,       // 20  (x, y, w, h)
	NVG_CMD_RESET_SCISSOR,           // 21  ()

	// ── Path building ────────────────────────────────────────
	NVG_CMD_BEGIN_PATH,              // 22  ()
	NVG_CMD_MOVE_TO,                 // 23  (x, y)
	NVG_CMD_LINE_TO,                 // 24  (x, y)
	NVG_CMD_BEZIER_TO,              // 25  (c1x, c1y, c2x, c2y, x, y)
	NVG_CMD_QUAD_TO,                 // 26  (cx, cy, x, y)
	NVG_CMD_ARC_TO,                  // 27  (x1, y1, x2, y2, radius)
	NVG_CMD_CLOSE_PATH,              // 28  ()
	NVG_CMD_PATH_WINDING,            // 29  (dir)

	// ── Shape helpers ────────────────────────────────────────
	NVG_CMD_ARC,                     // 30  (cx, cy, r, a0, a1, dir)
	NVG_CMD_RECT,                    // 31  (x, y, w, h)
	NVG_CMD_ROUNDED_RECT,            // 32  (x, y, w, h, r)
	NVG_CMD_ROUNDED_RECT_VARYING,    // 33  (x, y, w, h, rtl, rtr, rbr, rbl)
	NVG_CMD_ELLIPSE,                 // 34  (cx, cy, rx, ry)
	NVG_CMD_CIRCLE,                  // 35  (cx, cy, r)

	// ── Rendering ────────────────────────────────────────────
	NVG_CMD_FILL,                    // 36  ()
	NVG_CMD_STROKE,                  // 37  ()

	// ── Fill paint (gradients / patterns) ────────────────────
	NVG_CMD_FILL_LINEAR_GRADIENT,    // 38  (sx, sy, ex, ey,  ir, ig, ib, ia,  or, og, ob, oa)
	NVG_CMD_FILL_BOX_GRADIENT,       // 39  (x, y, w, h, r, f,  ir, ig, ib, ia,  or, og, ob, oa)
	NVG_CMD_FILL_RADIAL_GRADIENT,    // 40  (cx, cy, inr, outr,  ir, ig, ib, ia,  or, og, ob, oa)
	NVG_CMD_FILL_IMAGE_PATTERN,      // 41  (ox, oy, ex, ey, angle, image*, alpha)  *must be var

	// ── Stroke paint (gradients / patterns) ──────────────────
	NVG_CMD_STROKE_LINEAR_GRADIENT,  // 42
	NVG_CMD_STROKE_BOX_GRADIENT,     // 43
	NVG_CMD_STROKE_RADIAL_GRADIENT,  // 44
	NVG_CMD_STROKE_IMAGE_PATTERN,    // 45

	// ── Text style ───────────────────────────────────────────
	NVG_CMD_FONT_SIZE,               // 46  (size)
	NVG_CMD_FONT_BLUR,               // 47  (blur)
	NVG_CMD_TEXT_LETTER_SPACING,     // 48  (spacing)
	NVG_CMD_TEXT_LINE_HEIGHT,        // 49  (lineHeight)
	NVG_CMD_TEXT_ALIGN,              // 50  (align)
	NVG_CMD_FONT_FACE_ID,           // 51  (fontId)

	// ── Text rendering ───────────────────────────────────────
	NVG_CMD_TEXT,                    // 52  (x, y, stringVar)
	NVG_CMD_TEXT_BOX,                // 53  (x, y, breakWidth, stringVar)

	// ── Layer ordering ───────────────────────────────────────
	NVG_CMD_SET_ZINDEX,              // 54  (zIndex)

	NVG_CMD__COUNT
};

// Argument count per opcode (indexed by NVGcmdOp).
inline const uint8_t* nvgCmdArgCount() {
	static const uint8_t table[NVG_CMD__COUNT] = {
		/*  0 SAVE              */ 0,
		/*  1 RESTORE           */ 0,
		/*  2 RESET             */ 0,
		/*  3 RESET_TRANSFORM   */ 0,
		/*  4 TRANSFORM         */ 6,
		/*  5 TRANSLATE         */ 2,
		/*  6 ROTATE            */ 1,
		/*  7 SKEW_X            */ 1,
		/*  8 SKEW_Y            */ 1,
		/*  9 SCALE             */ 2,
		/* 10 FILL_COLOR        */ 4,
		/* 11 STROKE_COLOR      */ 4,
		/* 12 STROKE_WIDTH      */ 1,
		/* 13 LINE_CAP          */ 1,
		/* 14 LINE_JOIN         */ 1,
		/* 15 MITER_LIMIT       */ 1,
		/* 16 GLOBAL_ALPHA      */ 1,
		/* 17 SHAPE_ANTI_ALIAS  */ 1,
		/* 18 COMPOSITE_OP      */ 1,
		/* 19 SCISSOR           */ 4,
		/* 20 INTERSECT_SCISSOR */ 4,
		/* 21 RESET_SCISSOR     */ 0,
		/* 22 BEGIN_PATH        */ 0,
		/* 23 MOVE_TO           */ 2,
		/* 24 LINE_TO           */ 2,
		/* 25 BEZIER_TO         */ 6,
		/* 26 QUAD_TO           */ 4,
		/* 27 ARC_TO            */ 5,
		/* 28 CLOSE_PATH        */ 0,
		/* 29 PATH_WINDING      */ 1,
		/* 30 ARC               */ 6,
		/* 31 RECT              */ 4,
		/* 32 ROUNDED_RECT      */ 5,
		/* 33 ROUNDED_RECT_VARY */ 8,
		/* 34 ELLIPSE           */ 4,
		/* 35 CIRCLE            */ 3,
		/* 36 FILL              */ 0,
		/* 37 STROKE            */ 0,
		/* 38 FILL_LIN_GRAD     */ 12,
		/* 39 FILL_BOX_GRAD     */ 14,
		/* 40 FILL_RAD_GRAD     */ 12,
		/* 41 FILL_IMG_PAT      */ 7,
		/* 42 STROKE_LIN_GRAD   */ 12,
		/* 43 STROKE_BOX_GRAD   */ 14,
		/* 44 STROKE_RAD_GRAD   */ 12,
		/* 45 STROKE_IMG_PAT    */ 7,
		/* 46 FONT_SIZE         */ 1,
		/* 47 FONT_BLUR         */ 1,
		/* 48 TEXT_LETTER_SPC    */ 1,
		/* 49 TEXT_LINE_H       */ 1,
		/* 50 TEXT_ALIGN        */ 1,
		/* 51 FONT_FACE_ID      */ 1,
		/* 52 TEXT              */ 3,
		/* 53 TEXT_BOX          */ 4,
		/* 54 SET_ZINDEX        */ 1,
	};
	return table;
}

// Bitmask per opcode: bit K = 1 means arg K is integer (for text serialization).
inline const uint16_t* nvgCmdArgIntMask() {
	static const uint16_t table[NVG_CMD__COUNT] = {
		/*  0-9  */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 10-12 */ 0, 0, 0,
		/* 13 */    0x0001,  // LINE_CAP
		/* 14 */    0x0001,  // LINE_JOIN
		/* 15-16 */ 0, 0,
		/* 17 */    0x0001,  // SHAPE_ANTI_ALIAS
		/* 18 */    0x0001,  // COMPOSITE_OP
		/* 19-28 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* 29 */    0x0001,  // PATH_WINDING
		/* 30 */    0x0020,  // ARC (arg5=dir)
		/* 31-37 */ 0, 0, 0, 0, 0, 0, 0,
		/* 38-45 */ 0, 0, 0, 0, 0, 0, 0, 0,
		/* 46-49 */ 0, 0, 0, 0,
		/* 50 */    0x0001,  // TEXT_ALIGN
		/* 51 */    0x0001,  // FONT_FACE_ID
		/* 52-53 */ 0, 0,
		/* 54 */    0x0001,  // SET_ZINDEX
	};
	return table;
}

// Opcode name (lowercase with underscores) for text serialization.
const char* nvgCmdOpName(uint32_t op);
// Reverse lookup: returns NVG_CMD__COUNT if not found.
uint32_t    nvgCmdOpFromName(const char* name);

// ─────────────────────────────────────────────────────────────
//  Variable system
// ─────────────────────────────────────────────────────────────

// Types that a layout variable can hold.
enum NVGcmdVarType : uint16_t {
	NVG_VAR_FLOAT = 0,   // float    (4 bytes)
	NVG_VAR_INT32,       // int32_t  (4 bytes)
	NVG_VAR_UINT32,      // uint32_t (4 bytes)
	NVG_VAR_STRING,      // const char*  (pointer-sized)
	NVG_VAR_HANDLE,      // NVGhandle    (platform-sized)
};

static constexpr uint32_t NVG_CMD_MAX_NAME = 64;

inline void nvgCmdCopyName(char* destination, size_t capacity,
	const char* source) noexcept {
	if (!destination || capacity == 0) return;
	if (!source) source = "";
	size_t length = 0;
	while (length + 1 < capacity && source[length] != '\0') ++length;
	if (length > 0) std::memcpy(destination, source, length);
	destination[length] = '\0';
}

// Command cell (4 bytes, same width as a float / int32 / uint32).
union NVGcmdCell {
	float    f;
	int32_t  i;
	uint32_t u;
};
static_assert(sizeof(NVGcmdCell) == 4, "NVGcmdCell must be 4 bytes");

// Single entry in a variable layout — maps a variable index
// to a byte offset inside the element data block.
struct NVGcmdVar {
	uint32_t      offset;  // byte offset from the data pointer
	NVGcmdVarType type;    // how to interpret bytes at that offset
};

// Layout descriptor: an array of variable entries.
// Typically defined once per widget class as a static constant.
struct NVGcmdLayout {
	const NVGcmdVar* vars;
	uint32_t         count;

	inline const NVGcmdVar& operator[](uint32_t i) const {
		assert(i < count);
		return vars[i];
	}
};

// Helper macro for defining layout entries.
// Usage: NVG_VAR_ENTRY(MyWidget, x, NVG_VAR_FLOAT)
#define NVG_VAR_ENTRY(cls, member, vtype) \
	NVGcmdVar{ (uint32_t)offsetof(cls, member), vtype }

// Variable type name for text serialization.
const char*   nvgCmdVarTypeName(NVGcmdVarType type);
NVGcmdVarType nvgCmdVarTypeFromName(const char* name);

// ─────────────────────────────────────────────────────────────
//  Properties — schema-local values, optionally state-dependent
// ─────────────────────────────────────────────────────────────
//
// A property is a named value that lives in the schema itself
// (not in the widget data block).  It can be:
//   - constant:  one value, independent of state
//   - state-dep: N values, indexed by a data-bound variable
//     (typically the widget's "state" field).
//
// When a command arg is a property reference, the high bit of
// the variable index is set (| NVG_CMD_PROP_BIT).  The lower
// 31 bits are the prop index.
//

static constexpr uint32_t NVG_CMD_PROP_BIT = 0x80000000u;

struct NVGcmdProperty {
	char          name[NVG_CMD_MAX_NAME];
	NVGcmdVarType type;            // value type
	uint32_t      stateVarIndex;   // data-var that selects the state (~0u = constant)
	std::vector<NVGcmdCell> values; // 1 value if constant, N if state-dependent
};

// ─────────────────────────────────────────────────────────────
//  Metadata — widget schema descriptor
// ─────────────────────────────────────────────────────────────

struct NVGcmdVarInfo {
	char          name[NVG_CMD_MAX_NAME];
	NVGcmdVarType type;
};

struct NVGcmdMeta {
	char     className[NVG_CMD_MAX_NAME];
	char     elementName[NVG_CMD_MAX_NAME];
	uint32_t version;
	std::vector<NVGcmdVarInfo> vars;

	NVGcmdMeta() : version(0) {
		className[0] = '\0';
		elementName[0] = '\0';
	}
	void clear() {
		className[0] = '\0';
		elementName[0] = '\0';
		version = 0;
		vars.clear();
	}
	int findVar(const char* name) const {
		for (size_t i = 0; i < vars.size(); i++)
			if (std::strcmp(vars[i].name, name) == 0)
				return (int)i;
		return -1;
	}
	void addVar(const char* name, NVGcmdVarType type) {
		NVGcmdVarInfo vi{};
		nvgCmdCopyName(vi.name, NVG_CMD_MAX_NAME, name);
		vi.type = type;
		vars.push_back(vi);
	}
};

// ─────────────────────────────────────────────────────────────
//  Command argument — literal value or variable reference
// ─────────────────────────────────────────────────────────────
struct NVGcmdArg {
	NVGcmdCell cell;
	bool       isVar;

	// Implicit constructors for literal values.
	NVGcmdArg(float v)    : isVar(false) { cell.f = v; }
	NVGcmdArg(int32_t v)  : isVar(false) { cell.i = v; }
	NVGcmdArg(uint32_t v) : isVar(false) { cell.u = v; }

	// Named constructor for variable reference.
	static NVGcmdArg var(uint32_t varIndex) {
		NVGcmdArg a(0.0f);
		a.isVar  = true;
		a.cell.u = varIndex;
		return a;
	}
	// Named constructor for property reference.
	static NVGcmdArg prop(uint32_t propIndex) {
		NVGcmdArg a(0.0f);
		a.isVar  = true;
		a.cell.u = propIndex | NVG_CMD_PROP_BIT;
		return a;
	}
};

// Shorthand: V(idx) creates a variable reference argument.
inline NVGcmdArg V(uint32_t idx) { return NVGcmdArg::var(idx); }
// Shorthand: P(idx) creates a property reference argument.
inline NVGcmdArg P(uint32_t idx) { return NVGcmdArg::prop(idx); }

// ─────────────────────────────────────────────────────────────
//  Command buffer — serialisable list of drawing commands
// ─────────────────────────────────────────────────────────────
struct NVGcmdBuf {
	NVGcmdMeta              meta;
	std::vector<NVGcmdCell> cells;
	std::vector<NVGcmdProperty> props;   // schema-local properties

	void clear() {
		meta.clear();
		cells.clear();
		props.clear();
	}
	uint32_t size()  const         { return (uint32_t)cells.size(); }
	const NVGcmdCell* data() const { return cells.data(); }

	// ── Property helpers ─────────────────────────────────────

	// Add a constant property. Returns property index.
	uint32_t addProperty(const char* name, NVGcmdVarType type,
	                     NVGcmdCell value) {
		NVGcmdProperty p{};
		nvgCmdCopyName(p.name, NVG_CMD_MAX_NAME, name);
		p.type = type;
		p.stateVarIndex = ~0u;
		p.values.push_back(value);
		uint32_t idx = (uint32_t)props.size();
		props.push_back(std::move(p));
		return idx;
	}

	// Add a state-dependent property. Returns property index.
	// stateVar is the data-variable index whose uint32 value selects
	// which entry from 'values' to use.
	uint32_t addProperty(const char* name, NVGcmdVarType type,
	                     uint32_t stateVarIndex,
	                     std::initializer_list<NVGcmdCell> values) {
		assert(values.size() > 0);
		NVGcmdProperty p{};
		nvgCmdCopyName(p.name, NVG_CMD_MAX_NAME, name);
		p.type = type;
		p.stateVarIndex = stateVarIndex;
		p.values.assign(values);
		uint32_t idx = (uint32_t)props.size();
		props.push_back(std::move(p));
		return idx;
	}

	int findProperty(const char* name) const {
		for (size_t i = 0; i < props.size(); i++)
			if (std::strcmp(props[i].name, name) == 0)
				return (int)i;
		return -1;
	}

	// ── Serialization ────────────────────────────────────────
	bool saveBinary(NVGio& io) const;
	bool loadBinary(NVGio& io);
	bool saveText(NVGio& io) const;
	bool loadText(NVGio& io);

	// ── Generic emitter ──────────────────────────────────────
	bool emit(NVGcmdOp op, std::initializer_list<NVGcmdArg> args) {
		if ((uint32_t)op >= NVG_CMD__COUNT)
			return false;
		const uint8_t expected = nvgCmdArgCount()[(uint32_t)op];
		assert(args.size() == expected && "arg count mismatch");
		if (args.size() != expected)
			return false;

		cells.reserve(cells.size() + 1 + (args.size() > 0 ? 1 + args.size() : 0));

		NVGcmdCell c;
		c.u = (uint32_t)op;
		cells.push_back(c);

		if (args.size() > 0) {
			uint32_t mask = 0;
			uint32_t bit  = 0;
			for (auto& a : args) {
				if (a.isVar) mask |= (1u << bit);
				++bit;
			}
			c.u = mask;
			cells.push_back(c);
			for (auto& a : args)
				cells.push_back(a.cell);
		}
		return true;
	}

	// ── State ────────────────────────────────────────────────
	void save()    { emit(NVG_CMD_SAVE,    {}); }
	void restore() { emit(NVG_CMD_RESTORE, {}); }
	void reset()   { emit(NVG_CMD_RESET,   {}); }

	// ── Transforms ───────────────────────────────────────────
	void resetTransform() { emit(NVG_CMD_RESET_TRANSFORM, {}); }

	void transform(NVGcmdArg a, NVGcmdArg b, NVGcmdArg c,
	               NVGcmdArg d, NVGcmdArg e, NVGcmdArg f) {
		emit(NVG_CMD_TRANSFORM, {a, b, c, d, e, f});
	}
	void translate(NVGcmdArg x, NVGcmdArg y) {
		emit(NVG_CMD_TRANSLATE, {x, y});
	}
	void rotate(NVGcmdArg angle) {
		emit(NVG_CMD_ROTATE, {angle});
	}
	void skewX(NVGcmdArg angle) {
		emit(NVG_CMD_SKEW_X, {angle});
	}
	void skewY(NVGcmdArg angle) {
		emit(NVG_CMD_SKEW_Y, {angle});
	}
	void scale(NVGcmdArg sx, NVGcmdArg sy) {
		emit(NVG_CMD_SCALE, {sx, sy});
	}

	// ── Render styles ────────────────────────────────────────
	void fillColor(NVGcmdArg r, NVGcmdArg g, NVGcmdArg b, NVGcmdArg a) {
		emit(NVG_CMD_FILL_COLOR, {r, g, b, a});
	}
	void strokeColor(NVGcmdArg r, NVGcmdArg g, NVGcmdArg b, NVGcmdArg a) {
		emit(NVG_CMD_STROKE_COLOR, {r, g, b, a});
	}
	void strokeWidth(NVGcmdArg w) {
		emit(NVG_CMD_STROKE_WIDTH, {w});
	}
	void lineCap(NVGcmdArg cap) {
		emit(NVG_CMD_LINE_CAP, {cap});
	}
	void lineJoin(NVGcmdArg join) {
		emit(NVG_CMD_LINE_JOIN, {join});
	}
	void miterLimit(NVGcmdArg limit) {
		emit(NVG_CMD_MITER_LIMIT, {limit});
	}
	void globalAlpha(NVGcmdArg alpha) {
		emit(NVG_CMD_GLOBAL_ALPHA, {alpha});
	}
	void shapeAntiAlias(NVGcmdArg enabled) {
		emit(NVG_CMD_SHAPE_ANTI_ALIAS, {enabled});
	}
	void compositeOp(NVGcmdArg op) {
		emit(NVG_CMD_COMPOSITE_OP, {op});
	}

	// ── Scissor ──────────────────────────────────────────────
	void scissor(NVGcmdArg x, NVGcmdArg y, NVGcmdArg w, NVGcmdArg h) {
		emit(NVG_CMD_SCISSOR, {x, y, w, h});
	}
	void intersectScissor(NVGcmdArg x, NVGcmdArg y, NVGcmdArg w, NVGcmdArg h) {
		emit(NVG_CMD_INTERSECT_SCISSOR, {x, y, w, h});
	}
	void resetScissor() {
		emit(NVG_CMD_RESET_SCISSOR, {});
	}

	// ── Path building ────────────────────────────────────────
	void beginPath() { emit(NVG_CMD_BEGIN_PATH, {}); }

	void moveTo(NVGcmdArg x, NVGcmdArg y) {
		emit(NVG_CMD_MOVE_TO, {x, y});
	}
	void lineTo(NVGcmdArg x, NVGcmdArg y) {
		emit(NVG_CMD_LINE_TO, {x, y});
	}
	void bezierTo(NVGcmdArg c1x, NVGcmdArg c1y,
	              NVGcmdArg c2x, NVGcmdArg c2y,
	              NVGcmdArg x,   NVGcmdArg y) {
		emit(NVG_CMD_BEZIER_TO, {c1x, c1y, c2x, c2y, x, y});
	}
	void quadTo(NVGcmdArg cx, NVGcmdArg cy,
	            NVGcmdArg x,  NVGcmdArg y) {
		emit(NVG_CMD_QUAD_TO, {cx, cy, x, y});
	}
	void arcTo(NVGcmdArg x1, NVGcmdArg y1,
	           NVGcmdArg x2, NVGcmdArg y2,
	           NVGcmdArg radius) {
		emit(NVG_CMD_ARC_TO, {x1, y1, x2, y2, radius});
	}
	void closePath()  { emit(NVG_CMD_CLOSE_PATH,  {}); }
	void pathWinding(NVGcmdArg dir) {
		emit(NVG_CMD_PATH_WINDING, {dir});
	}

	// ── Shape helpers ────────────────────────────────────────
	void arc(NVGcmdArg cx, NVGcmdArg cy, NVGcmdArg r,
	         NVGcmdArg a0, NVGcmdArg a1, NVGcmdArg dir) {
		emit(NVG_CMD_ARC, {cx, cy, r, a0, a1, dir});
	}
	void rect(NVGcmdArg x, NVGcmdArg y,
	          NVGcmdArg w, NVGcmdArg h) {
		emit(NVG_CMD_RECT, {x, y, w, h});
	}
	void roundedRect(NVGcmdArg x, NVGcmdArg y,
	                 NVGcmdArg w, NVGcmdArg h,
	                 NVGcmdArg r) {
		emit(NVG_CMD_ROUNDED_RECT, {x, y, w, h, r});
	}
	void roundedRectVarying(NVGcmdArg x,   NVGcmdArg y,
	                        NVGcmdArg w,   NVGcmdArg h,
	                        NVGcmdArg rtl, NVGcmdArg rtr,
	                        NVGcmdArg rbr, NVGcmdArg rbl) {
		emit(NVG_CMD_ROUNDED_RECT_VARYING, {x, y, w, h, rtl, rtr, rbr, rbl});
	}
	void ellipse(NVGcmdArg cx, NVGcmdArg cy,
	             NVGcmdArg rx, NVGcmdArg ry) {
		emit(NVG_CMD_ELLIPSE, {cx, cy, rx, ry});
	}
	void circle(NVGcmdArg cx, NVGcmdArg cy, NVGcmdArg r) {
		emit(NVG_CMD_CIRCLE, {cx, cy, r});
	}

	// ── Rendering ────────────────────────────────────────────
	void fill()   { emit(NVG_CMD_FILL,   {}); }
	void stroke() { emit(NVG_CMD_STROKE, {}); }

	// ── Fill paint (gradients / image patterns) ──────────────
	// Colors are passed as individual r,g,b,a floats so that each
	// component can independently be a literal or a variable.
	void fillLinearGradient(NVGcmdArg sx,  NVGcmdArg sy,
	                        NVGcmdArg ex,  NVGcmdArg ey,
	                        NVGcmdArg icr, NVGcmdArg icg,
	                        NVGcmdArg icb, NVGcmdArg ica,
	                        NVGcmdArg ocr, NVGcmdArg ocg,
	                        NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_FILL_LINEAR_GRADIENT,
		     {sx, sy, ex, ey, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	void fillBoxGradient(NVGcmdArg x,   NVGcmdArg y,
	                     NVGcmdArg w,   NVGcmdArg h,
	                     NVGcmdArg r,   NVGcmdArg f,
	                     NVGcmdArg icr, NVGcmdArg icg,
	                     NVGcmdArg icb, NVGcmdArg ica,
	                     NVGcmdArg ocr, NVGcmdArg ocg,
	                     NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_FILL_BOX_GRADIENT,
		     {x, y, w, h, r, f, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	void fillRadialGradient(NVGcmdArg cx,  NVGcmdArg cy,
	                        NVGcmdArg inr, NVGcmdArg outr,
	                        NVGcmdArg icr, NVGcmdArg icg,
	                        NVGcmdArg icb, NVGcmdArg ica,
	                        NVGcmdArg ocr, NVGcmdArg ocg,
	                        NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_FILL_RADIAL_GRADIENT,
		     {cx, cy, inr, outr, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	// arg[5] (image handle) must be a variable reference.
	void fillImagePattern(NVGcmdArg ox,    NVGcmdArg oy,
	                      NVGcmdArg ex,    NVGcmdArg ey,
	                      NVGcmdArg angle, NVGcmdArg image,
	                      NVGcmdArg alpha) {
		emit(NVG_CMD_FILL_IMAGE_PATTERN,
		     {ox, oy, ex, ey, angle, image, alpha});
	}

	// ── Stroke paint (same signatures) ───────────────────────
	void strokeLinearGradient(NVGcmdArg sx,  NVGcmdArg sy,
	                          NVGcmdArg ex,  NVGcmdArg ey,
	                          NVGcmdArg icr, NVGcmdArg icg,
	                          NVGcmdArg icb, NVGcmdArg ica,
	                          NVGcmdArg ocr, NVGcmdArg ocg,
	                          NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_STROKE_LINEAR_GRADIENT,
		     {sx, sy, ex, ey, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	void strokeBoxGradient(NVGcmdArg x,   NVGcmdArg y,
	                       NVGcmdArg w,   NVGcmdArg h,
	                       NVGcmdArg r,   NVGcmdArg f,
	                       NVGcmdArg icr, NVGcmdArg icg,
	                       NVGcmdArg icb, NVGcmdArg ica,
	                       NVGcmdArg ocr, NVGcmdArg ocg,
	                       NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_STROKE_BOX_GRADIENT,
		     {x, y, w, h, r, f, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	void strokeRadialGradient(NVGcmdArg cx,  NVGcmdArg cy,
	                          NVGcmdArg inr, NVGcmdArg outr,
	                          NVGcmdArg icr, NVGcmdArg icg,
	                          NVGcmdArg icb, NVGcmdArg ica,
	                          NVGcmdArg ocr, NVGcmdArg ocg,
	                          NVGcmdArg ocb, NVGcmdArg oca) {
		emit(NVG_CMD_STROKE_RADIAL_GRADIENT,
		     {cx, cy, inr, outr, icr, icg, icb, ica, ocr, ocg, ocb, oca});
	}
	void strokeImagePattern(NVGcmdArg ox,    NVGcmdArg oy,
	                        NVGcmdArg ex,    NVGcmdArg ey,
	                        NVGcmdArg angle, NVGcmdArg image,
	                        NVGcmdArg alpha) {
		emit(NVG_CMD_STROKE_IMAGE_PATTERN,
		     {ox, oy, ex, ey, angle, image, alpha});
	}

	// ── Text style ───────────────────────────────────────────
	void fontSize(NVGcmdArg size)          { emit(NVG_CMD_FONT_SIZE,          {size}); }
	void fontBlur(NVGcmdArg blur)          { emit(NVG_CMD_FONT_BLUR,          {blur}); }
	void textLetterSpacing(NVGcmdArg s)    { emit(NVG_CMD_TEXT_LETTER_SPACING, {s}); }
	void textLineHeight(NVGcmdArg h)       { emit(NVG_CMD_TEXT_LINE_HEIGHT,    {h}); }
	void textAlign(NVGcmdArg align)        { emit(NVG_CMD_TEXT_ALIGN,          {align}); }
	void fontFaceId(NVGcmdArg id)          { emit(NVG_CMD_FONT_FACE_ID,       {id}); }

	// ── Text rendering ───────────────────────────────────────
	// The last argument (stringVar) must be a variable reference
	// pointing to a const char* in the element data.
	void text(NVGcmdArg x, NVGcmdArg y, NVGcmdArg stringVar) {
		emit(NVG_CMD_TEXT, {x, y, stringVar});
	}
	void textBox(NVGcmdArg x, NVGcmdArg y,
	             NVGcmdArg breakWidth, NVGcmdArg stringVar) {
		emit(NVG_CMD_TEXT_BOX, {x, y, breakWidth, stringVar});
	}

	// ── Layer ordering ───────────────────────────────────────
	void setZIndex(NVGcmdArg z) {
		emit(NVG_CMD_SET_ZINDEX, {z});
	}
};

enum class NVGcmdValidationCode : uint32_t {
	ok = 0,
	invalid_opcode,
	truncated_command,
	invalid_variable_mask,
	variable_out_of_range,
	property_out_of_range,
	invalid_variable_type,
	invalid_property,
	duplicate_name,
	restore_without_save,
	unbalanced_save_restore,
	layout_mismatch
};

struct NVGcmdValidationResult {
	NVGcmdValidationCode code = NVGcmdValidationCode::ok;
	uint32_t cell_index = 0;
	std::string message;

	bool succeeded() const noexcept { return code == NVGcmdValidationCode::ok; }
	explicit operator bool() const noexcept { return succeeded(); }
};

struct NVGcmdEvalResult {
	NVGcmdValidationResult validation;
	uint32_t commands_executed = 0;

	bool succeeded() const noexcept { return validation.succeeded(); }
	explicit operator bool() const noexcept { return succeeded(); }
};

NVGcmdValidationResult nvgCmdValidate(const NVGcmdBuf& buf,
	const NVGcmdLayout* layout = nullptr);

// ─────────────────────────────────────────────────────────────
//  Evaluator — execute a command buffer against an NVGcontext
// ─────────────────────────────────────────────────────────────
// data   — pointer to the element's data block (may be nullptr
//          if no variable references are used).
// layout — variable layout for this element class.
void nvgEval(NVGcontext& ctx,
             const NVGcmdBuf& buf,
             const void* data        = nullptr,
             const NVGcmdLayout* layout = nullptr);

NVGcmdEvalResult nvgEvalChecked(NVGcontext& ctx,
	const NVGcmdBuf& buf, const void* data = nullptr,
	const NVGcmdLayout* layout = nullptr);

#endif // NVG_CMD_H
