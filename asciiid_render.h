#pragma once

#include <stdint.h>
#include <assert.h>
#include <algorithm> // std::min/max

#include "terrain.h"
#include "mesh.h"

struct AnsiCell
{
	uint8_t gl; // CP437 only
	uint8_t fg, bk; // palettized!
	uint8_t spare; // for post pass
};

bool Render(Terrain* t, World* w, int water, 					// scene
			float zoom, float yaw, float pos[3], 				// view
			int width, int height, int pitch, AnsiCell* ptr);	// target

