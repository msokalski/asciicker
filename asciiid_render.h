#pragma once

#include <stdint.h>

struct AnsiCell
{
	uint8_t glyph;
	uint8_t fg, bk;
	uint8_t spare;
};

bool Render(float angle, int cx, int cy, int w, int h, int pitch, AnsiCell* ptr);

