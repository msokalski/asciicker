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

	// from all frames angles anims and projections
	float proj_bbox[6];

	int anims;  // must be 0 for 'still' Sprite
	int frames; // must be 1 for 'still' Sprite
	int angles;
	Frame* atlas; // [frames][angles][2] (x2 because of projection/reflection)

	struct Anim
	{
		int length;
		int* frame_idx; // [angles * 2]
	};

	Anim anim[1];
};

Sprite* LoadPlayer(const char* path);
void FreeSprite(Sprite* spr);