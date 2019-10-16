#pragma once

#include "asciiid_render.h"

struct Sprite
{
	struct Frame
	{
		int width;
		int height;
		int ref[3]; // on image x,y,z (x,y are int x2 units to allow half block refs)
		AnsiCell* cell; // cell[].spare encodes cell height relative to ref[2]
	};

	// from all frames and angles
	int proj_bbox[6];
	int refl_bbox[6];

	int anims;  // must be 0 for 'still' Sprite
	int frames; // must be 1 for 'still' Sprite
	int angles;
	Frame* atlas; // [frames][angles][2] (x2 because of projection/reflection)

	struct Anim
	{
		int speed;
		int length;
		int* frame_idx; // if frames need different timings, use higher fps ie 4x and dup frame references x3/x4/x5
	};

	Anim anim[1];
};

Sprite* LoadPlayer(const char* path);