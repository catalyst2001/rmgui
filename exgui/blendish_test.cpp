#include "blendish_test.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BX_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

void drawBlendish(NVGcontext* _vg, float _x, float _y, float _w, float _h, float _t)
{
	float x = _x;
	float y = _y;

	bndBackground(_vg, _x - 10.0f, _y - 10.0f, _w, _h);

	bndToolButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, BND_ICONID(6, 3), "Default");
	y += 25.0f;
	bndToolButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, BND_ICONID(6, 3), "Hovered");
	y += 25.0f;
	bndToolButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, BND_ICONID(6, 3), "Active");

	y += 40.0f;
	bndRadioButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, -1, "Default");
	y += 25.0f;
	bndRadioButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, -1, "Hovered");
	y += 25.0f;
	bndRadioButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, -1, "Active");

	y += 25.0f;
	bndLabel(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, -1, "Label:");
	y += float(BND_WIDGET_HEIGHT);
	bndChoiceButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, -1, "Default");
	y += 25.0f;
	bndChoiceButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, -1, "Hovered");
	y += 25.0f;
	bndChoiceButton(_vg, x, y, 80.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, -1, "Active");

	y += 25.0f;
	float ry = y;
	float rx = x;

	y = _y;
	x += 130.0f;
	bndOptionButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_DEFAULT, "Default");
	y += 25.0f;
	bndOptionButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_HOVER, "Hovered");
	y += 25.0f;
	bndOptionButton(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_ACTIVE, "Active");

	y += 40.0f;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_DOWN, BND_DEFAULT, "Top", "100");
	y += float(BND_WIDGET_HEIGHT) - 2.0f;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, "Center", "100");
	y += float(BND_WIDGET_HEIGHT) - 2.0f;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_TOP, BND_DEFAULT, "Bottom", "100");

	float mx = x - 30.0f;
	float my = y - 12.0f;
	float mw = 120.0f;
	bndMenuBackground(_vg, mx, my, mw, 120.0f, BND_CORNER_TOP);
	bndMenuLabel(_vg, mx, my, mw, BND_WIDGET_HEIGHT, -1, "Menu Title");
	my += float(BND_WIDGET_HEIGHT) - 2.0f;
	bndMenuItem(_vg, mx, my, mw, BND_WIDGET_HEIGHT, BND_DEFAULT, BND_ICONID(17, 3), "Default");
	my += float(BND_WIDGET_HEIGHT) - 2.0f;
	bndMenuItem(_vg, mx, my, mw, BND_WIDGET_HEIGHT, BND_HOVER, BND_ICONID(18, 3), "Hovered");
	my += float(BND_WIDGET_HEIGHT) - 2.0f;
	bndMenuItem(_vg, mx, my, mw, BND_WIDGET_HEIGHT, BND_ACTIVE, BND_ICONID(19, 3), "Active");

	y = _y;
	x += 130.0f;
	float ox = x;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, "Default", "100");
	y += 25.0f;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, "Hovered", "100");
	y += 25.0f;
	bndNumberField(_vg, x, y, 120.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, "Active", "100");

	y += 40.0f;
	bndRadioButton(_vg, x, y, 60.0f, BND_WIDGET_HEIGHT, BND_CORNER_RIGHT, BND_DEFAULT, -1, "One");
	x += 60.0f - 1.0f;
	bndRadioButton(_vg, x, y, 60.0f, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, -1, "Two");
	x += 60.0f - 1.0f;
	bndRadioButton(_vg, x, y, 60.0f, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, -1, "Three");
	x += 60.0f - 1.0f;
	bndRadioButton(_vg, x, y, 60.0f, BND_WIDGET_HEIGHT, BND_CORNER_LEFT, BND_ACTIVE, -1, "Butts");

	x = ox;
	y += 40.0f;
	float progress_value = fmodf(_t / 10.0f, 1.0f);
	char progress_label[32];
	snprintf(progress_label, BX_COUNTOF(progress_label), "%d%%", int(progress_value * 100 + 0.5f));
	bndSlider(_vg, x, y, 240, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, progress_value, "Default", progress_label);
	y += 25.0f;
	bndSlider(_vg, x, y, 240, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, progress_value, "Hovered", progress_label);
	y += 25.0f;
	bndSlider(_vg, x, y, 240, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, progress_value, "Active", progress_label);

	float rw = x + 240.0f - rx;
	float s_offset = sinf(_t / 2.0f) * 0.5f + 0.5f;
	float s_size = cosf(_t / 3.11f) * 0.5f + 0.5f;

	bndScrollBar(_vg, rx, ry, rw, BND_SCROLLBAR_HEIGHT, BND_DEFAULT, s_offset, s_size);
	ry += 20.0f;
	bndScrollBar(_vg, rx, ry, rw, BND_SCROLLBAR_HEIGHT, BND_HOVER, s_offset, s_size);
	ry += 20.0f;
	bndScrollBar(_vg, rx, ry, rw, BND_SCROLLBAR_HEIGHT, BND_ACTIVE, s_offset, s_size);

	const char edit_text[] = "The quick brown fox";
	int textlen = int(strlen(edit_text) + 1);
	int t = int(_t * 2);
	int idx1 = (t / textlen) % textlen;
	int idx2 = idx1 + (t % (textlen - idx1));

	ry += 25.0f;
	bndTextField(_vg, rx, ry, 240.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_DEFAULT, -1, edit_text, idx1, idx2);
	ry += 25.0f;
	bndTextField(_vg, rx, ry, 240.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_HOVER, -1, edit_text, idx1, idx2);
	ry += 25.0f;
	bndTextField(_vg, rx, ry, 240.0f, BND_WIDGET_HEIGHT, BND_CORNER_NONE, BND_ACTIVE, -1, edit_text, idx1, idx2);

	rx += rw + 20.0f;
	ry = _y;
	bndScrollBar(_vg, rx, ry, BND_SCROLLBAR_WIDTH, 240.0f, BND_DEFAULT, s_offset, s_size);
	rx += 20.0f;
	bndScrollBar(_vg, rx, ry, BND_SCROLLBAR_WIDTH, 240.0f, BND_HOVER, s_offset, s_size);
	rx += 20.0f;
	bndScrollBar(_vg, rx, ry, BND_SCROLLBAR_WIDTH, 240.0f, BND_ACTIVE, s_offset, s_size);

	x = ox;
	y += 40.0f;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_RIGHT, BND_DEFAULT, BND_ICONID(0, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(1, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(2, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(3, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(4, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndToolButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_LEFT, BND_DEFAULT, BND_ICONID(5, 10), NULL);
	x += BND_TOOL_WIDTH - 1;
	x += 5.0f;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_RIGHT, BND_DEFAULT, BND_ICONID(0, 11), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(1, 11), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(2, 11), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_DEFAULT, BND_ICONID(3, 11), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_ALL, BND_ACTIVE, BND_ICONID(4, 11), NULL);
	x += BND_TOOL_WIDTH - 1;
	bndRadioButton(_vg, x, y, BND_TOOL_WIDTH, BND_WIDGET_HEIGHT, BND_CORNER_LEFT, BND_DEFAULT, BND_ICONID(5, 11), NULL);
}