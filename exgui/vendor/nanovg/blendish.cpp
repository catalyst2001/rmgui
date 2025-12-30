/*
Blendish - Blender 2.5 UI based theming functions for NanoVG

Copyright (c) 2014 Leonard Ritter <leonard.ritter@duangle.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "blendish.h"

#include <memory.h>
#include <math.h>

#ifdef _MSC_VER
    #pragma warning (disable: 4996) // Switch off security warnings
    #pragma warning (disable: 4100) // Switch off unreferenced formal parameter warnings
    #pragma warning (disable: 4244)
    #pragma warning (disable: 4305)
    #ifdef __cplusplus
    #define BND_INLINE inline
    #else
    #define BND_INLINE
    #endif

#include <float.h>

static float bnd_fminf ( float a, float b ) {
    return _isnan(a) ? b : ( _isnan(b) ? a : ((a < b) ? a : b));
}

static float bnd_fmaxf ( float a, float b ) {
    return _isnan(a) ? b : ( _isnan(b) ? a : ((a > b) ? a : b));
}

static double bnd_fmin ( double a, double b ) {
    return _isnan(a) ? b : ( _isnan(b) ? a : ((a < b) ? a : b));
}

static double bnd_fmax ( double a, double b ) {
    return _isnan(a) ? b : ( _isnan(b) ? a : ((a > b) ? a : b));
}

#else
    #define BND_INLINE static inline
    #define bnd_fminf(a, b) fminf(a, b)
    #define bnd_fmaxf(a, b) fmaxf(a, b)
    #define bnd_fmin(a, b) fmin(a, b)
    #define bnd_fmax(a, b) fmax(a, b)
#endif

////////////////////////////////////////////////////////////////////////////////

// default text size
#define BND_LABEL_FONT_SIZE 13

// default text padding in inner box
#define BND_PAD_LEFT 8
#define BND_PAD_RIGHT 8

// label: value separator string
#define BND_LABEL_SEPARATOR ": "

// alpha intensity of transparent items (0xa4)
#define BND_TRANSPARENT_ALPHA 0.9

// shade intensity of beveled panels
#define BND_BEVEL_SHADE 30
// shade intensity of beveled insets
#define BND_INSET_BEVEL_SHADE 30
// shade intensity of hovered inner boxes
#define BND_HOVER_SHADE 15
// shade intensity of splitter bevels
#define BND_SPLITTER_SHADE 100

// width of icon sheet
#define BND_ICON_SHEET_WIDTH 602
// height of icon sheet
#define BND_ICON_SHEET_HEIGHT 640
// gridsize of icon sheet in both dimensions
#define BND_ICON_SHEET_GRID 21
// offset of first icon tile relative to left border
#define BND_ICON_SHEET_OFFSET_X 5
// offset of first icon tile relative to top border
#define BND_ICON_SHEET_OFFSET_Y 10
// resolution of single icon
#define BND_ICON_SHEET_RES 16

// size of number field arrow
#define BND_NUMBER_ARROW_SIZE 4

// default text color
#define BND_COLOR_TEXT {{{ 0,0,0,1 }}}
// default highlighted text color
#define BND_COLOR_TEXT_SELECTED {{{ 1,1,1,1 }}}

// radius of tool button
#define BND_TOOL_RADIUS 4

// radius of option button
#define BND_OPTION_RADIUS 4
// width of option button checkbox
#define BND_OPTION_WIDTH 14
// height of option button checkbox
#define BND_OPTION_HEIGHT 15

// radius of text field
#define BND_TEXT_RADIUS 4

// radius of number button
#define BND_NUMBER_RADIUS 10

// radius of menu popup
#define BND_MENU_RADIUS 3
// feather of menu popup shadow
#define BND_SHADOW_FEATHER 12
// alpha of menu popup shadow
#define BND_SHADOW_ALPHA 0.5

// radius of scrollbar
#define BND_SCROLLBAR_RADIUS 7
// shade intensity of active scrollbar
#define BND_SCROLLBAR_ACTIVE_SHADE 15

// max glyphs for position testing
#define BND_MAX_GLYPHS 1024

// max rows for position testing
#define BND_MAX_ROWS 32

// text distance from bottom
#define BND_TEXT_PAD_DOWN 7

// stroke width of wire outline
#define BND_NODE_WIRE_OUTLINE_WIDTH 4
// stroke width of wire
#define BND_NODE_WIRE_WIDTH 2
// radius of node box
#define BND_NODE_RADIUS 8
// feather of node title text
#define BND_NODE_TITLE_FEATHER 1
// size of node title arrow
#define BND_NODE_ARROW_SIZE 9

////////////////////////////////////////////////////////////////////////////////

BND_INLINE float bnd_clamp(float v, float mn, float mx) {
    return (v > mx)?mx:(v < mn)?mn:v;
}

////////////////////////////////////////////////////////////////////////////////

// the initial theme
static BNDtheme bnd_theme = {
    // backgroundColor
    {{{ 0.447, 0.447, 0.447, 1.0 }}},
    // regularTheme
    {
        {{{ 0.098,0.098,0.098,1 }}}, // color_outline
        {{{ 0.098,0.098,0.098,1 }}}, // color_item
        {{{ 0.6,0.6,0.6,1 }}}, // color_inner
        {{{ 0.392,0.392,0.392,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        0, // shade_top
        0, // shade_down
    },
    // toolTheme
    {
        {{{ 0.098,0.098,0.098,1 }}}, // color_outline
        {{{ 0.098,0.098,0.098,1 }}}, // color_item
        {{{ 0.6,0.6,0.6,1 }}}, // color_inner
        {{{ 0.392,0.392,0.392,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        15, // shade_top
        -15, // shade_down
    },
    // radioTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 1,1,1,1 }}}, // color_item
        {{{ 0.275,0.275,0.275,1 }}}, // color_inner
        {{{ 0.337,0.502,0.761,1 }}}, // color_inner_selected
        BND_COLOR_TEXT_SELECTED, // color_text
        BND_COLOR_TEXT, // color_text_selected
        15, // shade_top
        -15, // shade_down
    },
    // textFieldTheme
    {
        {{{ 0.098,0.098,0.098,1 }}}, // color_outline
        {{{ 0.353, 0.353, 0.353,1 }}}, // color_item
        {{{ 0.6, 0.6, 0.6,1 }}}, // color_inner
        {{{ 0.6, 0.6, 0.6,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        0, // shade_top
        25, // shade_down
    },
    // optionTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 1,1,1,1 }}}, // color_item
        {{{ 0.275,0.275,0.275,1 }}}, // color_inner
        {{{ 0.275,0.275,0.275,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        15, // shade_top
        -15, // shade_down
    },
    // choiceTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 1,1,1,1 }}}, // color_item
        {{{ 0.275,0.275,0.275,1 }}}, // color_inner
        {{{ 0.275,0.275,0.275,1 }}}, // color_inner_selected
        BND_COLOR_TEXT_SELECTED, // color_text
        {{{ 0.8,0.8,0.8,1 }}}, // color_text_selected
        15, // shade_top
        -15, // shade_down
    },
    // numberFieldTheme
    {
        {{{ 0.098,0.098,0.098,1 }}}, // color_outline
        {{{ 0.353, 0.353, 0.353,1 }}}, // color_item
        {{{ 0.706, 0.706, 0.706,1 }}}, // color_inner
        {{{ 0.6, 0.6, 0.6,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        -20, // shade_top
        0, // shade_down
    },
    // sliderTheme
    {
        {{{ 0.098,0.098,0.098,1 }}}, // color_outline
        {{{ 0.502,0.502,0.502,1 }}}, // color_item
        {{{ 0.706, 0.706, 0.706,1 }}}, // color_inner
        {{{ 0.6, 0.6, 0.6,1 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        -20, // shade_top
        0, // shade_down
    },
    // scrollBarTheme
    {
        {{{ 0.196,0.196,0.196,1 }}}, // color_outline
        {{{ 0.502,0.502,0.502,1 }}}, // color_item
        {{{ 0.314, 0.314, 0.314,0.706 }}}, // color_inner
        {{{ 0.392, 0.392, 0.392,0.706 }}}, // color_inner_selected
        BND_COLOR_TEXT, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        5, // shade_top
        -5, // shade_down
    },
    // tooltipTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 0.392,0.392,0.392,1 }}}, // color_item
        {{{ 0.098, 0.098, 0.098, 0.902 }}}, // color_inner
        {{{ 0.176, 0.176, 0.176, 0.902 }}}, // color_inner_selected
        {{{ 0.627, 0.627, 0.627, 1 }}}, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        0, // shade_top
        0, // shade_down
    },
    // menuTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 0.392,0.392,0.392,1 }}}, // color_item
        {{{ 0.098, 0.098, 0.098, 0.902 }}}, // color_inner
        {{{ 0.176, 0.176, 0.176, 0.902 }}}, // color_inner_selected
        {{{ 0.627, 0.627, 0.627, 1 }}}, // color_text
        BND_COLOR_TEXT_SELECTED, // color_text_selected
        0, // shade_top
        0, // shade_down
    },
    // menuItemTheme
    {
        {{{ 0,0,0,1 }}}, // color_outline
        {{{ 0.675,0.675,0.675,0.502 }}}, // color_item
        {{{ 0,0,0,0 }}}, // color_inner
        {{{ 0.337,0.502,0.761,1 }}}, // color_inner_selected
        BND_COLOR_TEXT_SELECTED, // color_text
        BND_COLOR_TEXT, // color_text_selected
        38, // shade_top
        0, // shade_down
    },
    // nodeTheme
    {
        {{{ 0.945,0.345,0,1 }}}, // nodeSelectedColor
        {{{ 0,0,0,1 }}}, // wiresColor
        {{{ 0.498,0.439,0.439,1 }}}, // textSelectedColor
        {{{ 1,0.667,0.251,1 }}}, // activeNodeColor
        {{{ 1,1,1,1 }}}, // wireSelectColor
        {{{ 0.608,0.608,0.608,0.627 }}}, // nodeBackdropColor
        5, // noodleCurving
    },
};

////////////////////////////////////////////////////////////////////////////////

void bndSetTheme(BNDtheme theme) {
    bnd_theme = theme;
}

const BNDtheme *bndGetTheme() {
    return &bnd_theme;
}

// the handle to the image containing the icon sheet
static int bnd_icon_image = -1;

void bndSetIconImage(int image) {
    bnd_icon_image = image;
}

// the handle to the UI font
static int bnd_font = -1;

void bndSetFont(int font) {
    bnd_font = font;
}

////////////////////////////////////////////////////////////////////////////////

void bndLabel(NVGcontext *ctx,
    float x, float y, float w, float h, int iconid, const char *label) {
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bnd_theme.regularTheme.textColor, BND_LEFT,
        BND_LABEL_FONT_SIZE, label, NULL);
}

void bndToolButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_TOOL_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.toolTheme, state, 1);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.toolTheme.outlineColor));
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bndTextColor(&bnd_theme.toolTheme, state), BND_CENTER,
        BND_LABEL_FONT_SIZE, label, NULL);
}

void bndRadioButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_OPTION_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.radioTheme, state, 1);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.radioTheme.outlineColor));
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bndTextColor(&bnd_theme.radioTheme, state), BND_CENTER,
        BND_LABEL_FONT_SIZE, label, NULL);
}

int bndTextFieldTextPosition(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, const char *text, int px, int py) {
    return bndIconLabelTextPosition(ctx, x, y, w, h,
        iconid, BND_LABEL_FONT_SIZE, text, px, py);
}

void bndTextField(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *text, int cbegin, int cend) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_TEXT_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.textFieldTheme, state, 0);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.textFieldTheme.outlineColor));
    if (state != BND_ACTIVE) {
        cend = -1;
    }
    bndIconLabelCaret(ctx,x,y,w,h,iconid,
        bndTextColor(&bnd_theme.textFieldTheme, state), BND_LABEL_FONT_SIZE,
        text, bnd_theme.textFieldTheme.itemColor, cbegin, cend);
}

void bndOptionButton(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    const char *label) {
    float ox, oy;
    NVGcolor shade_top, shade_down;

    ox = x;
    oy = y+h-BND_OPTION_HEIGHT-3;

    bndBevelInset(ctx,ox,oy,
        BND_OPTION_WIDTH,BND_OPTION_HEIGHT,
        BND_OPTION_RADIUS,BND_OPTION_RADIUS);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.optionTheme, state, 1);
    bndInnerBox(ctx,ox,oy,
        BND_OPTION_WIDTH,BND_OPTION_HEIGHT,
        BND_OPTION_RADIUS,BND_OPTION_RADIUS,BND_OPTION_RADIUS,BND_OPTION_RADIUS,
        shade_top, shade_down);
    bndOutlineBox(ctx,ox,oy,
        BND_OPTION_WIDTH,BND_OPTION_HEIGHT,
        BND_OPTION_RADIUS,BND_OPTION_RADIUS,BND_OPTION_RADIUS,BND_OPTION_RADIUS,
        bndTransparent(bnd_theme.optionTheme.outlineColor));
    if (state == BND_ACTIVE) {
        bndCheck(ctx,ox,oy, bndTransparent(bnd_theme.optionTheme.itemColor));
    }
    bndIconLabelValue(ctx,x+12,y,w-12,h,-1,
        bndTextColor(&bnd_theme.optionTheme, state), BND_LEFT,
        BND_LABEL_FONT_SIZE, label, NULL);
}

void bndChoiceButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    int iconid, const char *label) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_OPTION_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.choiceTheme, state, 1);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.choiceTheme.outlineColor));
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bndTextColor(&bnd_theme.choiceTheme, state), BND_LEFT,
        BND_LABEL_FONT_SIZE, label, NULL);
    bndUpDownArrow(ctx,x+w-10,y+10,5,
        bndTransparent(bnd_theme.choiceTheme.itemColor));
}

void bndColorButton(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, NVGcolor color) {
    float cr[4];
    bndSelectCorners(cr, BND_TOOL_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], color, color);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.toolTheme.outlineColor));
}

void bndNumberField(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    const char *label, const char *value) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_NUMBER_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.numberFieldTheme, state, 0);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.numberFieldTheme.outlineColor));
    bndIconLabelValue(ctx,x,y,w,h,-1,
        bndTextColor(&bnd_theme.numberFieldTheme, state), BND_CENTER,
        BND_LABEL_FONT_SIZE, label, value);
    bndArrow(ctx,x+8,y+10,-BND_NUMBER_ARROW_SIZE,
        bndTransparent(bnd_theme.numberFieldTheme.itemColor));
    bndArrow(ctx,x+w-8,y+10,BND_NUMBER_ARROW_SIZE,
        bndTransparent(bnd_theme.numberFieldTheme.itemColor));
}

void bndSlider(NVGcontext *ctx,
    float x, float y, float w, float h, int flags, BNDwidgetState state,
    float progress, const char *label, const char *value) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_NUMBER_RADIUS, flags);
    bndBevelInset(ctx,x,y,w,h,cr[2],cr[3]);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.sliderTheme, state, 0);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);

    if (state == BND_ACTIVE) {
        shade_top = bndOffsetColor(
            bnd_theme.sliderTheme.itemColor, bnd_theme.sliderTheme.shadeTop);
        shade_down = bndOffsetColor(
            bnd_theme.sliderTheme.itemColor, bnd_theme.sliderTheme.shadeDown);
    } else {
        shade_top = bndOffsetColor(
            bnd_theme.sliderTheme.itemColor, bnd_theme.sliderTheme.shadeDown);
        shade_down = bndOffsetColor(
            bnd_theme.sliderTheme.itemColor, bnd_theme.sliderTheme.shadeTop);
    }
    ctx->scissor(x,y,8+(w-8)*bnd_clamp(progress,0,1),h);
    bndInnerBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    ctx->resetScissor();

    bndOutlineBox(ctx,x,y,w,h,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.sliderTheme.outlineColor));
    bndIconLabelValue(ctx,x,y,w,h,-1,
        bndTextColor(&bnd_theme.sliderTheme, state), BND_CENTER,
        BND_LABEL_FONT_SIZE, label, value);
}

void bndScrollBar(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    float offset, float size) {

    bndBevelInset(ctx,x,y,w,h,
        BND_SCROLLBAR_RADIUS, BND_SCROLLBAR_RADIUS);
    bndInnerBox(ctx,x,y,w,h,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        bndOffsetColor(
            bnd_theme.scrollBarTheme.innerColor, 3*bnd_theme.scrollBarTheme.shadeDown),
        bndOffsetColor(
            bnd_theme.scrollBarTheme.innerColor, 3*bnd_theme.scrollBarTheme.shadeTop));
    bndOutlineBox(ctx,x,y,w,h,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        bndTransparent(bnd_theme.scrollBarTheme.outlineColor));

    NVGcolor itemColor = bndOffsetColor(
        bnd_theme.scrollBarTheme.itemColor,
        (state == BND_ACTIVE)?BND_SCROLLBAR_ACTIVE_SHADE:0);

    bndScrollHandleRect(&x,&y,&w,&h,offset,size);

    bndInnerBox(ctx,x,y,w,h,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        bndOffsetColor(
            itemColor, 3*bnd_theme.scrollBarTheme.shadeTop),
        bndOffsetColor(
            itemColor, 3*bnd_theme.scrollBarTheme.shadeDown));
    bndOutlineBox(ctx,x,y,w,h,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        BND_SCROLLBAR_RADIUS,BND_SCROLLBAR_RADIUS,
        bndTransparent(bnd_theme.scrollBarTheme.outlineColor));
}

void bndMenuBackground(NVGcontext *ctx,
    float x, float y, float w, float h, int flags) {
    float cr[4];
    NVGcolor shade_top, shade_down;

    bndSelectCorners(cr, BND_MENU_RADIUS, flags);
    bndInnerColors(&shade_top, &shade_down, &bnd_theme.menuTheme,
        BND_DEFAULT, 0);
    bndInnerBox(ctx,x,y,w,h+1,cr[0],cr[1],cr[2],cr[3], shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h+1,cr[0],cr[1],cr[2],cr[3],
        bndTransparent(bnd_theme.menuTheme.outlineColor));
    bndDropShadow(ctx,x,y,w,h,BND_MENU_RADIUS,
        BND_SHADOW_FEATHER,BND_SHADOW_ALPHA);
}

void bndTooltipBackground(NVGcontext *ctx, float x, float y, float w, float h) {
    NVGcolor shade_top, shade_down;

    bndInnerColors(&shade_top, &shade_down, &bnd_theme.tooltipTheme,
        BND_DEFAULT, 0);
    bndInnerBox(ctx,x,y,w,h+1,
        BND_MENU_RADIUS,BND_MENU_RADIUS,BND_MENU_RADIUS,BND_MENU_RADIUS,
        shade_top, shade_down);
    bndOutlineBox(ctx,x,y,w,h+1,
        BND_MENU_RADIUS,BND_MENU_RADIUS,BND_MENU_RADIUS,BND_MENU_RADIUS,
        bndTransparent(bnd_theme.tooltipTheme.outlineColor));
    bndDropShadow(ctx,x,y,w,h,BND_MENU_RADIUS,
        BND_SHADOW_FEATHER,BND_SHADOW_ALPHA);
}

void bndMenuLabel(NVGcontext *ctx,
    float x, float y, float w, float h, int iconid, const char *label) {
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bnd_theme.menuTheme.textColor, BND_LEFT,
        BND_LABEL_FONT_SIZE, label, NULL);
}

void bndMenuItem(NVGcontext *ctx,
    float x, float y, float w, float h, BNDwidgetState state,
    int iconid, const char *label) {
    if (state != BND_DEFAULT) {
        bndInnerBox(ctx,x,y,w,h,0,0,0,0,
            bndOffsetColor(bnd_theme.menuItemTheme.innerSelectedColor,
                bnd_theme.menuItemTheme.shadeTop),
            bndOffsetColor(bnd_theme.menuItemTheme.innerSelectedColor,
                bnd_theme.menuItemTheme.shadeDown));
        state = BND_ACTIVE;
    }
    bndIconLabelValue(ctx,x,y,w,h,iconid,
        bndTextColor(&bnd_theme.menuItemTheme, state), BND_LEFT,
        BND_LABEL_FONT_SIZE, label, NULL);
}

void bndNodePort(NVGcontext *ctx, float x, float y, BNDwidgetState state,
    NVGcolor color) {
    ctx->beginPath();
    ctx->circle(x, y, BND_NODE_PORT_RADIUS);
    ctx->strokeColor(bnd_theme.nodeTheme.wiresColor);
    ctx->StrokeWidth(1.0f);
    ctx->stroke();
    ctx->fillColor((state != BND_DEFAULT)?
        bndOffsetColor(color, BND_HOVER_SHADE):color);
    ctx->fill();
}

void bndColoredNodeWire(NVGcontext *ctx, float x0, float y0, float x1, float y1,
    NVGcolor color0, NVGcolor color1) {
    float length = bnd_fmaxf(fabsf(x1 - x0),fabsf(y1 - y0));
    float delta = length*(float)bnd_theme.nodeTheme.noodleCurving/10.0f;

    ctx->beginPath();
    ctx->moveTo(x0, y0);
    ctx->bezierTo(x0 + delta, y0,
        x1 - delta, y1,
        x1, y1);
    NVGcolor colorw = bnd_theme.nodeTheme.wiresColor;
    colorw.a = (color0.a<color1.a)?color0.a:color1.a;
    ctx->strokeColor(colorw);
    ctx->StrokeWidth(BND_NODE_WIRE_OUTLINE_WIDTH);
    ctx->stroke();
    ctx->strokePaint(NVGpaint::linearGradient(
        x0, y0, x1, y1,
        color0,
        color1));
    ctx->StrokeWidth(BND_NODE_WIRE_WIDTH);
    ctx->stroke();
}

void bndNodeWire(NVGcontext *ctx, float x0, float y0, float x1, float y1,
    BNDwidgetState state0, BNDwidgetState state1) {
    bndColoredNodeWire(ctx, x0, y0, x1, y1,
        bndNodeWireColor(&bnd_theme.nodeTheme, state0),
        bndNodeWireColor(&bnd_theme.nodeTheme, state1));
}

void bndNodeBackground(NVGcontext *ctx, float x, float y, float w, float h,
    BNDwidgetState state, int iconid, const char *label, NVGcolor titleColor) {
    bndInnerBox(ctx,x,y,w,BND_NODE_TITLE_HEIGHT+2,
        BND_NODE_RADIUS,BND_NODE_RADIUS,0,0,
        bndTransparent(bndOffsetColor(titleColor, BND_BEVEL_SHADE)),
        bndTransparent(titleColor));
    bndInnerBox(ctx,x,y+BND_NODE_TITLE_HEIGHT-1,w,h+2-BND_NODE_TITLE_HEIGHT,
        0,0,BND_NODE_RADIUS,BND_NODE_RADIUS,
        bndTransparent(bnd_theme.nodeTheme.nodeBackdropColor),
        bndTransparent(bnd_theme.nodeTheme.nodeBackdropColor));
    bndNodeIconLabel(ctx,
        x+BND_NODE_ARROW_AREA_WIDTH,y,
        w-BND_NODE_ARROW_AREA_WIDTH-BND_NODE_MARGIN_SIDE,BND_NODE_TITLE_HEIGHT,
        iconid, bnd_theme.regularTheme.textColor,
        bndOffsetColor(titleColor, BND_BEVEL_SHADE),
        BND_LEFT, BND_LABEL_FONT_SIZE, label);
    //NVGcolor arrowColor;
    NVGcolor borderColor;
    switch(state) {
    default:
    case BND_DEFAULT: {
        borderColor = NVGcolor::RGBf(0,0,0);
        //arrowColor = bndOffsetColor(titleColor, -BND_BEVEL_SHADE);
    } break;
    case BND_HOVER: {
        borderColor = bnd_theme.nodeTheme.nodeSelectedColor;
        //arrowColor = bnd_theme.nodeTheme.nodeSelectedColor;
    } break;
    case BND_ACTIVE: {
        borderColor = bnd_theme.nodeTheme.activeNodeColor;
        //arrowColor = bnd_theme.nodeTheme.nodeSelectedColor;
    } break;
    }
    bndOutlineBox(ctx,x,y,w,h+1,
        BND_NODE_RADIUS,BND_NODE_RADIUS,BND_NODE_RADIUS,BND_NODE_RADIUS,
        bndTransparent(borderColor));
    /*
    bndNodeArrowDown(ctx,
        x + BND_NODE_MARGIN_SIDE, y + BND_NODE_TITLE_HEIGHT-4,
        BND_NODE_ARROW_SIZE, arrowColor);
    */
    bndDropShadow(ctx,x,y,w,h,BND_NODE_RADIUS,
        BND_SHADOW_FEATHER,BND_SHADOW_ALPHA);
}

void bndSplitterWidgets(NVGcontext *ctx, float x, float y, float w, float h) {
    NVGcolor insetLight = bndTransparent(
        bndOffsetColor(bnd_theme.backgroundColor, BND_SPLITTER_SHADE));
    NVGcolor insetDark = bndTransparent(
        bndOffsetColor(bnd_theme.backgroundColor, -BND_SPLITTER_SHADE));
    NVGcolor inset = bndTransparent(bnd_theme.backgroundColor);

    float x2 = x+w;
    float y2 = y+h;

    ctx->beginPath();
    ctx->moveTo(x, y2-13);
    ctx->lineTo(x+13, y2);
    ctx->moveTo(x, y2-9);
    ctx->lineTo(x+9, y2);
    ctx->moveTo(x, y2-5);
    ctx->lineTo(x+5, y2);

    ctx->moveTo(x2-11, y);
    ctx->lineTo(x2, y+11);
    ctx->moveTo(x2-7, y);
    ctx->lineTo(x2, y+7);
    ctx->moveTo(x2-3, y);
    ctx->lineTo(x2, y+3);

    ctx->strokeColor(insetDark);
    ctx->stroke();

    ctx->beginPath();
    ctx->moveTo(x, y2-11);
    ctx->lineTo(x+11, y2);
    ctx->moveTo(x, y2-7);
    ctx->lineTo(x+7, y2);
    ctx->moveTo(x, y2-3);
    ctx->lineTo(x+3, y2);

    ctx->moveTo(x2-13, y);
    ctx->lineTo(x2, y+13);
    ctx->moveTo(x2-9, y);
    ctx->lineTo(x2, y+9);
    ctx->moveTo(x2-5, y);
    ctx->lineTo(x2, y+5);

    ctx->strokeColor(insetLight);
    ctx->stroke();

    ctx->beginPath();
    ctx->moveTo(x, y2-12);
    ctx->lineTo(x+12, y2);
    ctx->moveTo(x, y2-8);
    ctx->lineTo(x+8, y2);
    ctx->moveTo(x, y2-4);
    ctx->lineTo(x+4, y2);

    ctx->moveTo(x2-12, y);
    ctx->lineTo(x2, y+12);
    ctx->moveTo(x2-8, y);
    ctx->lineTo(x2, y+8);
    ctx->moveTo(x2-4, y);
    ctx->lineTo(x2, y+4);

    ctx->strokeColor(inset);
    ctx->stroke();
}

void bndJoinAreaOverlay(NVGcontext *ctx, float x, float y, float w, float h,
    int vertical, int mirror) {

    if (vertical) {
        float u = w;
        w = h; h = u;
    }

    float s = (w<h)?w:h;

    float x0,y0,x1,y1;
    if (mirror) {
        x0 = w;
        y0 = h;
        x1 = 0;
        y1 = 0;
        s = -s;
    } else {
        x0 = 0;
        y0 = 0;
        x1 = w;
        y1 = h;
    }

    float yc = (y0+y1)*0.5f;
    float s2 = s/2.0f;
    float s4 = s/4.0f;
    float s8 = s/8.0f;
    float x4 = x0+s4;

    float points[][2] = {
        { x0,y0 },
        { x1,y0 },
        { x1,y1 },
        { x0,y1 },
        { x0,yc+s8 },
        { x4,yc+s8 },
        { x4,yc+s4 },
        { x0+s2,yc },
        { x4,yc-s4 },
        { x4,yc-s8 },
        { x0,yc-s8 }
    };

    ctx->beginPath();
    int count = sizeof(points) / (sizeof(float)*2);
    ctx->moveTo(x+points[0][vertical&1],y+points[0][(vertical&1)^1]);
    for (int i = 1; i < count; ++i) {
        ctx->lineTo(x+points[i][vertical&1],y+points[i][(vertical&1)^1]);
    }

    ctx->fillColor(NVGcolor::RGBAf(0,0,0,0.3));
    ctx->fill();
}

////////////////////////////////////////////////////////////////////////////////

float bndLabelWidth(NVGcontext *ctx, int iconid, const char *label) {
    int w = BND_PAD_LEFT + BND_PAD_RIGHT;
    if (iconid >= 0) {
        w += BND_ICON_SHEET_RES;
    }
    if (label && (bnd_font >= 0)) {
        ctx->setFontFaceId(bnd_font);
        ctx->setFontSize(BND_LABEL_FONT_SIZE);
        w += ctx->textBounds(1, 1, label, NULL, NULL);
    }
    return w;
}

float bndLabelHeight(NVGcontext *ctx, int iconid, const char *label, float width) {
    int h = BND_WIDGET_HEIGHT;
    width -= BND_TEXT_RADIUS*2;
    if (iconid >= 0) {
        width -= BND_ICON_SHEET_RES;
    }
    if (label && (bnd_font >= 0)) {
        ctx->setFontFaceId(bnd_font);
        ctx->setFontSize(BND_LABEL_FONT_SIZE);
        float bounds[4];
        ctx->textBoxBounds(1, 1, width, label, NULL, bounds);
        int bh = (int)(bounds[3] - bounds[1]) + BND_TEXT_PAD_DOWN;
        if (bh > h)
            h = bh;
    }
    return h;
}

////////////////////////////////////////////////////////////////////////////////

void bndRoundedBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3) {
    float d;

    w = bnd_fmaxf(0, w);
    h = bnd_fmaxf(0, h);
    d = bnd_fminf(w, h);

    ctx->moveTo(x,y+h*0.5f);
    ctx->arcTo(x,y, x+w,y, bnd_fminf(cr0, d/2));
    ctx->arcTo(x+w,y, x+w,y+h, bnd_fminf(cr1, d/2));
    ctx->arcTo(x+w,y+h, x,y+h, bnd_fminf(cr2, d/2));
    ctx->arcTo(x,y+h, x,y, bnd_fminf(cr3, d/2));
    ctx->closePath();
}

NVGcolor bndTransparent(NVGcolor color) {
    color.a *= BND_TRANSPARENT_ALPHA;
    return color;
}

NVGcolor bndOffsetColor(NVGcolor color, int delta) {
    float offset = (float)delta / 255.0f;
    return delta?(
        NVGcolor::RGBAf(
            bnd_clamp(color.r+offset,0,1),
            bnd_clamp(color.g+offset,0,1),
            bnd_clamp(color.b+offset,0,1),
            color.a)
    ):color;
}

void bndBevel(NVGcontext *ctx, float x, float y, float w, float h) {
    ctx->StrokeWidth(1);

    x += 0.5f;
    y += 0.5f;
    w -= 1;
    h -= 1;

    ctx->beginPath();
    ctx->moveTo(x, y+h);
    ctx->lineTo(x+w, y+h);
    ctx->lineTo(x+w, y);
    ctx->strokeColor(bndTransparent(
        bndOffsetColor(bnd_theme.backgroundColor, -BND_BEVEL_SHADE)));
    ctx->stroke();

    ctx->beginPath();
    ctx->moveTo(x, y+h);
    ctx->lineTo(x, y);
    ctx->lineTo(x+w, y);
    ctx->strokeColor(bndTransparent(
        bndOffsetColor(bnd_theme.backgroundColor, BND_BEVEL_SHADE)));
    ctx->stroke();
}

void bndBevelInset(NVGcontext *ctx, float x, float y, float w, float h,
    float cr2, float cr3) {
    float d;

    y -= 0.5f;
    d = bnd_fminf(w, h);
    cr2 = bnd_fminf(cr2, d/2);
    cr3 = bnd_fminf(cr3, d/2);

    ctx->beginPath();
    ctx->moveTo(x+w,y+h-cr2);
    ctx->arcTo(x+w,y+h, x,y+h, cr2);
    ctx->arcTo(x,y+h, x,y, cr3);

    NVGcolor bevelColor = bndOffsetColor(bnd_theme.backgroundColor,
        BND_INSET_BEVEL_SHADE);

    ctx->StrokeWidth(1);
    ctx->strokePaint(NVGpaint::linearGradient(
            x,y+h-bnd_fmaxf(cr2,cr3)-1,
            x,y+h-1,
        NVGcolor::RGBAf(bevelColor.r, bevelColor.g, bevelColor.b, 0),
        bevelColor));
    ctx->stroke();
}

void bndBackground(NVGcontext *ctx, float x, float y, float w, float h) {
    ctx->beginPath();
    ctx->rect(x, y, w, h);
    ctx->fillColor(bnd_theme.backgroundColor);
    ctx->fill();
}

void bndIcon(NVGcontext *ctx, float x, float y, int iconid) {
    int ix, iy, u, v;
    if (bnd_icon_image < 0) return; // no icons loaded

    ix = iconid & 0xff;
    iy = (iconid>>8) & 0xff;
    u = BND_ICON_SHEET_OFFSET_X + ix*BND_ICON_SHEET_GRID;
    v = BND_ICON_SHEET_OFFSET_Y + iy*BND_ICON_SHEET_GRID;

    ctx->beginPath();
    ctx->rect(x,y,BND_ICON_SHEET_RES,BND_ICON_SHEET_RES);
    ctx->fillPaint(NVGpaint::imagePattern(x-u,y-v,
        BND_ICON_SHEET_WIDTH,
        BND_ICON_SHEET_HEIGHT,
        0,bnd_icon_image,1));
    ctx->fill();
}

void bndDropShadow(NVGcontext *ctx, float x, float y, float w, float h,
    float r, float feather, float alpha) {

    ctx->beginPath();
    y += feather;
    h -= feather;

    ctx->moveTo(x-feather, y-feather);
    ctx->lineTo(x, y-feather);
    ctx->lineTo(x, y+h-feather);
    ctx->arcTo(x,y+h,x+r,y+h,r);
    ctx->arcTo(x+w,y+h,x+w,y+h-r,r);
    ctx->lineTo(x+w, y-feather);
    ctx->lineTo(x+w+feather, y-feather);
    ctx->lineTo(x+w+feather, y+h+feather);
    ctx->lineTo(x-feather, y+h+feather);
    ctx->closePath();

    ctx->fillPaint(NVGpaint::boxGradient(
        x - feather*0.5f,y - feather*0.5f,
        w + feather,h+feather,
        r+feather*0.5f,
        feather,
        NVGcolor::RGBAf(0,0,0,alpha*alpha),
        NVGcolor::RGBAf(0,0,0,0)));
    ctx->fill();
}

void bndInnerBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3,
    NVGcolor shade_top, NVGcolor shade_down) {
    ctx->beginPath();
    bndRoundedBox(ctx,x+1,y+1,w-2,h-3,bnd_fmaxf(0,cr0-1),
        bnd_fmaxf(0,cr1-1),bnd_fmaxf(0,cr2-1),bnd_fmaxf(0,cr3-1));
    ctx->fillPaint(((h-2)>w)?
        NVGpaint::linearGradient(x,y,x+w,y,shade_top,shade_down):
        NVGpaint::linearGradient(x,y,x,y+h,shade_top,shade_down));
    ctx->fill();
}

void bndOutlineBox(NVGcontext *ctx, float x, float y, float w, float h,
    float cr0, float cr1, float cr2, float cr3, NVGcolor color) {
    ctx->beginPath();
    bndRoundedBox(ctx,x+0.5f,y+0.5f,w-1,h-2,cr0,cr1,cr2,cr3);
    ctx->strokeColor(color);
    ctx->StrokeWidth(1);
    ctx->stroke();
}

void bndSelectCorners(float *radiuses, float r, int flags) {
    radiuses[0] = (flags & BND_CORNER_TOP_LEFT)?0:r;
    radiuses[1] = (flags & BND_CORNER_TOP_RIGHT)?0:r;
    radiuses[2] = (flags & BND_CORNER_DOWN_RIGHT)?0:r;
    radiuses[3] = (flags & BND_CORNER_DOWN_LEFT)?0:r;
}

void bndInnerColors(
    NVGcolor *shade_top, NVGcolor *shade_down,
    const BNDwidgetTheme *theme, BNDwidgetState state, int flipActive) {

    switch(state) {
    default:
    case BND_DEFAULT: {
        *shade_top = bndOffsetColor(theme->innerColor, theme->shadeTop);
        *shade_down = bndOffsetColor(theme->innerColor, theme->shadeDown);
    } break;
    case BND_HOVER: {
        NVGcolor color = bndOffsetColor(theme->innerColor, BND_HOVER_SHADE);
        *shade_top = bndOffsetColor(color, theme->shadeTop);
        *shade_down = bndOffsetColor(color, theme->shadeDown);
    } break;
    case BND_ACTIVE: {
        *shade_top = bndOffsetColor(theme->innerSelectedColor,
            flipActive?theme->shadeDown:theme->shadeTop);
        *shade_down = bndOffsetColor(theme->innerSelectedColor,
            flipActive?theme->shadeTop:theme->shadeDown);
    } break;
    }
}

NVGcolor bndTextColor(const BNDwidgetTheme *theme, BNDwidgetState state) {
    return (state == BND_ACTIVE)?theme->textSelectedColor:theme->textColor;
}

void bndIconLabelValue(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, int align, float fontsize, const char *label,
    const char *value) {
    (void)h; // unused
    float pleft = BND_PAD_LEFT;
    if (label) {
        if (iconid >= 0) {
            bndIcon(ctx,x+4,y+2,iconid);
            pleft += BND_ICON_SHEET_RES;
        }

        if (bnd_font < 0) return;
        ctx->setFontFaceId(bnd_font);
        ctx->setFontSize(fontsize);
        ctx->beginPath();
        ctx->fillColor(color);
        if (value) {
            float label_width = ctx->textBounds(1, 1, label, NULL, NULL);
            float sep_width = ctx->textBounds(1, 1,
                BND_LABEL_SEPARATOR, NULL, NULL);

            ctx->setTextAlign(NVG_ALIGN_LEFT|NVG_ALIGN_BASELINE);
            x += pleft;
            if (align == BND_CENTER) {
                float width = label_width + sep_width
                    + ctx->textBounds(1, 1, value, NULL, NULL);
                x += ((w-BND_PAD_RIGHT-pleft)-width)*0.5f;
            }
            y += BND_WIDGET_HEIGHT-BND_TEXT_PAD_DOWN;
            ctx->text(x, y, label, NULL);
            x += label_width;
            ctx->text(x, y, BND_LABEL_SEPARATOR, NULL);
            x += sep_width;
            ctx->text(x, y, value, NULL);
        } else {
            ctx->setTextAlign((align==BND_LEFT)?(NVG_ALIGN_LEFT|NVG_ALIGN_BASELINE):
                (NVG_ALIGN_CENTER|NVG_ALIGN_BASELINE));
            ctx->textBox(x+pleft,y+BND_WIDGET_HEIGHT-BND_TEXT_PAD_DOWN,
                w-BND_PAD_RIGHT-pleft,label, NULL);
        }
    } else if (iconid >= 0) {
        bndIcon(ctx,x+2,y+2,iconid);
    }
}

void bndNodeIconLabel(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, NVGcolor shadowColor,
    int align, float fontsize, const char *label) {
    (void)align; // unused
    if (label && (bnd_font >= 0)) {
        ctx->setFontFaceId(bnd_font);
        ctx->setFontSize(fontsize);
        ctx->beginPath();
        ctx->setTextAlign(NVG_ALIGN_LEFT|NVG_ALIGN_BASELINE);
        ctx->fillColor(shadowColor);
        ctx->setFontBlur(BND_NODE_TITLE_FEATHER);
        ctx->textBox(x+1,y+h+3-BND_TEXT_PAD_DOWN,
            w,label, NULL);
        ctx->fillColor(color);
        ctx->setFontBlur(0);
        ctx->textBox(x,y+h+2-BND_TEXT_PAD_DOWN,
            w,label, NULL);
    }
    if (iconid >= 0) {
        bndIcon(ctx,x+w-BND_ICON_SHEET_RES,y+3,iconid);
    }
}

int bndIconLabelTextPosition(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, float fontsize, const char *label, int px, int py) {
    (void)h; // unused
    float bounds[4];
    float pleft = BND_TEXT_RADIUS;
    if (!label) return -1;
    if (iconid >= 0)
        pleft += BND_ICON_SHEET_RES;

    if (bnd_font < 0) return -1;

    x += pleft;
    y += BND_WIDGET_HEIGHT - BND_TEXT_PAD_DOWN;

    ctx->setFontFaceId(bnd_font);
    ctx->setFontSize(fontsize);
    ctx->setTextAlign(NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    w -= BND_TEXT_RADIUS + pleft;

    float asc, desc, lh;
    static NVGtextRow rows[BND_MAX_ROWS];
    int nrows = ctx->textBreakLines(label, NULL, w, rows, BND_MAX_ROWS);
    if (nrows == 0) return 0;
    ctx->textBoxBounds(x, y, w, label, NULL, bounds);
    ctx->textMetrics(&asc, &desc, &lh);

    // calculate vertical position
    int row = bnd_clamp((int)((float)(py - bounds[1]) / lh), 0, nrows - 1);
    // search horizontal position
    static NVGglyphPosition glyphs[BND_MAX_GLYPHS];
    int nglyphs = ctx->textGlyphPositions(x, y, rows[row].start, rows[row].end + 1, glyphs, BND_MAX_GLYPHS);
    int col, p = 0;
    for (col = 0; col < nglyphs && glyphs[col].x < px; ++col)
        p = glyphs[col].str - label;
    // see if we should move one character further
    if (col > 0 && col < nglyphs && glyphs[col].x - px < px - glyphs[col - 1].x)
        p = glyphs[col].str - label;
    return p;
}

static void bndCaretPosition(NVGcontext *ctx, float x, float y,
    float desc, float lineHeight, const char *caret, NVGtextRow *rows,int nrows,
    int *cr, float *cx, float *cy) {
    static NVGglyphPosition glyphs[BND_MAX_GLYPHS];
    int r,nglyphs;
    for (r=0; r < nrows-1 && rows[r].end < caret; ++r);
    *cr = r;
    *cx = x;
    *cy = y-lineHeight-desc + r*lineHeight;
    if (nrows == 0) return;
    *cx = rows[r].minx;
    nglyphs = ctx->textGlyphPositions(x, y, rows[r].start, rows[r].end+1, glyphs, BND_MAX_GLYPHS);
    for (int i=0; i < nglyphs; ++i) {
        *cx=glyphs[i].x;
        if (glyphs[i].str == caret) break;
    }
}

void bndIconLabelCaret(NVGcontext *ctx, float x, float y, float w, float h,
    int iconid, NVGcolor color, float fontsize, const char *label,
    NVGcolor caretcolor, int cbegin, int cend) {
    (void)h; // unused
    float pleft = BND_TEXT_RADIUS;
    if (!label) return;
    if (iconid >= 0) {
        bndIcon(ctx,x+4,y+2,iconid);
        pleft += BND_ICON_SHEET_RES;
    }

    if (bnd_font < 0) return;

    x+=pleft;
    y+=BND_WIDGET_HEIGHT-BND_TEXT_PAD_DOWN;

    ctx->setFontFaceId(bnd_font);
    ctx->setFontSize(fontsize);
    ctx->setTextAlign(NVG_ALIGN_LEFT|NVG_ALIGN_BASELINE);

    w -= BND_TEXT_RADIUS+pleft;

    if (cend >= cbegin) {
        int c0r,c1r;
        float c0x,c0y,c1x,c1y;
        float desc,lh;
        static NVGtextRow rows[BND_MAX_ROWS];
        int nrows = ctx->textBreakLines(label, label+cend+1, w, rows, BND_MAX_ROWS);
        ctx->textMetrics(NULL, &desc, &lh);

        bndCaretPosition(ctx, x, y, desc, lh, label+cbegin,
            rows, nrows, &c0r, &c0x, &c0y);
        bndCaretPosition(ctx, x, y, desc, lh, label+cend,
            rows, nrows, &c1r, &c1x, &c1y);

        ctx->beginPath();
        if (cbegin == cend) {
            ctx->fillColor(NVGcolor::RGBf(0.337,0.502,0.761));
            ctx->rect(c0x-1, c0y, 2, lh+1);
        } else {
            ctx->fillColor(caretcolor);
            if (c0r == c1r) {
                ctx->rect(c0x-1, c0y, c1x-c0x+1, lh+1);
            } else {
                int blk=c1r-c0r-1;
                ctx->rect(c0x-1, c0y, x+w-c0x+1, lh+1);
                ctx->rect(x, c1y, c1x-x+1, lh+1);

                if (blk)
                    ctx->rect(x, c0y+lh, w, blk*lh+1);
            }
        }
        ctx->fill();
    }

    ctx->beginPath();
    ctx->fillColor(color);
    ctx->textBox(x,y,w,label, NULL);
}

void bndCheck(NVGcontext *ctx, float ox, float oy, NVGcolor color) {
    ctx->beginPath();
    ctx->StrokeWidth(2);
    ctx->strokeColor(color);
    ctx->LineCap(NVG_BUTT);
    ctx->LineJoin(NVG_MITER);
    ctx->moveTo(ox+4,oy+5);
    ctx->lineTo(ox+7,oy+8);
    ctx->lineTo(ox+14,oy+1);
    ctx->stroke();
}

void bndArrow(NVGcontext *ctx, float x, float y, float s, NVGcolor color) {
    ctx->beginPath();
    ctx->moveTo(x,y);
    ctx->lineTo(x-s,y+s);
    ctx->lineTo(x-s,y-s);
    ctx->closePath();
    ctx->fillColor(color);
    ctx->fill();
}

void bndUpDownArrow(NVGcontext *ctx, float x, float y, float s, NVGcolor color) {
    float w;

    ctx->beginPath();
    w = 1.1f*s;
    ctx->moveTo(x,y-1);
    ctx->lineTo(x+0.5*w,y-s-1);
    ctx->lineTo(x+w,y-1);
    ctx->closePath();
    ctx->moveTo(x,y+1);
    ctx->lineTo(x+0.5*w,y+s+1);
    ctx->lineTo(x+w,y+1);
    ctx->closePath();
    ctx->fillColor(color);
    ctx->fill();
}

void bndNodeArrowDown(NVGcontext *ctx, float x, float y, float s, NVGcolor color) {
    float w;
    ctx->beginPath();
    w = 1.0f*s;
    ctx->moveTo(x,y);
    ctx->lineTo(x+0.5*w,y-s);
    ctx->lineTo(x-0.5*w,y-s);
    ctx->closePath();
    ctx->fillColor(color);
    ctx->fill();
}

void bndScrollHandleRect(float *x, float *y, float *w, float *h,
    float offset, float size) {
    size = bnd_clamp(size,0,1);
    offset = bnd_clamp(offset,0,1);
    if ((*h) > (*w)) {
        float hs = bnd_fmaxf(size*(*h), (*w)+1);
        *y = (*y) + ((*h)-hs)*offset;
        *h = hs;
    } else {
        float ws = bnd_fmaxf(size*(*w), (*h)-1);
        *x = (*x) + ((*w)-ws)*offset;
        *w = ws;
    }
}

NVGcolor bndNodeWireColor(const BNDnodeTheme *theme, BNDwidgetState state) {
    switch(state) {
        default:
        case BND_DEFAULT: return NVGcolor::RGBf(0.5f,0.5f,0.5f);
        case BND_HOVER: return theme->wireSelectColor;
        case BND_ACTIVE: return theme->activeNodeColor;
    }
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BND_INLINE
#undef BND_INLINE
#endif
