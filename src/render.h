#pragma once

#include <stdint.h>
#include <assert.h>
#include <algorithm> // std::min/max

#include "terrain.h"
#include "world.h"

struct AnsiCell
{
	//		R	G	B	A
	uint8_t fg, bk, gl, spare; // for post pass
};

struct MatCell
{
	uint8_t fg[3];	// foreground color
	uint8_t gl;		// glyph code
	uint8_t bg[3];	// background color
	uint8_t flags;

	// transparency mask :
	// 0x1 - fg 
	// 0x2 - gl 
	// 0x4 - bg

	// blend modes 3x3 bits:
	// 0x03 2-bits fg blend mode (0:replace, 1:multiply, 2:screen, 3:transparent)
	// 0x04 glyph write mask (0:replace, 1:keep)
	// 0x18 2-bits bg blend mode (0:replace, 1:multiply, 2:screen, 3:transparent)
	// 3 bits left!

};

struct Material
{
	MatCell shade[4][16];
	int mode; // mode and flags
};

struct Renderer;
Renderer* CreateRenderer(uint64_t stamp);
void DeleteRenderer(Renderer* r);

// return null-terminated array of item pointers that are reachable by player
/*
Item** Render(Renderer* r, uint64_t stamp, Terrain* t, World* w, float water, 		// scene
			float zoom, float yaw, const float pos[3], const float lt[4],	// view
			int width, int height, AnsiCell* ptr,  // target
			Sprite* sprite, int anim, int frame, float dir, 	// player
			const int scene_shift[2]); // special fx
*/

void Render(Renderer* r, uint64_t stamp, Terrain* t, World* w, float water, 		// scene
	float zoom, float yaw, const float pos[3], const float lt[4],	// view
	int width, int height, AnsiCell* ptr,  // target
	Inst* player, // player
	const int scene_shift[2],
	bool perspective);

bool ProjectCoords(Renderer* r, const float pos[3], int view[3]); // like a sprite!
bool UnprojectCoords2D(Renderer* r, const int xy[2], float pos[3]); // reads height from buffer first!
bool UnprojectCoords3D(Renderer* r, const int xy[3], float pos[3]); // reads height from buffer first!

Item** GetNearbyItems(Renderer* r);
Inst** GetNearbyCharacters(Renderer* r);

extern int render_break_point[2];