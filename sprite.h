#pragma once

#include "render.h"

struct Sprite
{
	int refs;

	struct Frame
	{
		int width;
		int height;
		int ref[3]; // on image x,y,z (x,y are int x2 units to allow half block refs)
		int meta_xy[2]; // some special position, ie crossbow's arrow tip (in half cells)
		AnsiCell* cell; // cell[].spare encodes cell height relative to ref[2]
	};

	// from all frames angles anims and projections
	float proj_bbox[6];

	int projs;
	int anims;  // must be 0 for 'still' Sprite
	int frames; // must be 1 for 'still' Sprite
	int angles;
	Frame* atlas; // [frames][angles][2] (x2 because of projection/reflection)

	struct Anim
	{
		int length;
		int* frame_idx; // [angles * 2]
	};

	Sprite* next;
	Sprite* prev;
	char* name;
	void* cookie;

	Anim anim[1];
};

Sprite* LoadSprite(const char* path, const char* name, /*bool has_refl = true,*/ const uint8_t* recolor = 0, bool detached = false);
Sprite* GetFirstSprite();
Sprite* GetPrevSprite(Sprite* s);
Sprite* GetNextSprite(Sprite* s);
int GetSpriteName(Sprite* s, char* buf, int size);

void SetSpriteCookie(Sprite* s, void* cookie);
void* GetSpriteCookie(Sprite* s);

Sprite* LoadPlayer(const char* path);
void FreeSprite(Sprite* spr);

void BlitSprite(AnsiCell* ptr, int width, int height, const Sprite::Frame* sf, int x, int y, const int clip[4]=0, bool src_clip=true, AnsiCell* bk=0);
void PaintFrame(AnsiCell* ptr, int width, int height, int x, int y, int w, int h, const int dst_clip[4] = 0, uint8_t fg=0, uint8_t bk=255, bool dbl=true, bool combine=true);
void FillRect(AnsiCell* ptr, int width, int height, int x, int y, int w, int h, AnsiCell ac);

int AverageGlyph(const AnsiCell* ptr, int mask);
int DarkenGlyph(const AnsiCell* ptr);

int LightenColor(int c);
