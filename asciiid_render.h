#pragma once

#include <stdint.h>
#include <assert.h>
#include <algorithm> // std::min/max

#include "terrain.h"
#include "mesh.h"

struct AnsiCell
{
	//		R	G	B	A
	uint8_t fg, bk, gl, spare; // for post pass
};

bool Render(Terrain* t, World* w, int water, 		// scene
			float zoom, float yaw, float pos[3], 	// view
			int width, int height, AnsiCell* ptr);	// target

