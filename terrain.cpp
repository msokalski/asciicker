
#include <stdint.h>
#include <malloc.h>
#include <assert.h>

#define HEIGHT_CELLS 4 // num of verts -1 along patch X and Y axis
#define VISUAL_CELLS (HEIGHT_CELLS*4)

#if 0
#define TERRAIN_MATERIALS 64
#define DETAIL_ANGLES 12

struct Material
{
	struct
	{
		struct
		{
			uint8_t f; // foreground (0-transparent only for elevations)
			uint8_t b; // background (0-transparent only for elevations)
			uint8_t c; // character
		} elev_upper, elev_lower, fill[16/*light*/][16/*shade*/];
	} optic[2/*optic_flag*/];
};

struct Foliage
{
	struct
	{
		uint8_t x,y; // center offs
		uint8_t width;
		uint8_t height;
		struct
		{
			uint8_t f; // foreground (0-transparent)
			uint8_t b; // background (0-transparent)
			uint8_t c; // character
			int8_t  d; // depth or height?
		}* sprite;
	} image[2/*norm & refl*/][DETAIL_ANGLES];
};
#endif

struct QuadItem
{
	uint16_t lo, hi;
};

struct Node : QuadItem
{
	QuadItem* quad[4]; // all 4 are same, either Nodes or Patches, at least 1 must not be NULL
};

struct Patch : QuadItem // 564 bytes (512x512 'raster' map would require 564KB)
{
	// visual contains:                grass, sand, rock,
	// 1bit elevation, 1bit optic_flag, 6bits material_idx, 4bits_shade, 4bits_light
	// uint16_t visual[VISUAL_CELLS][VISUAL_CELLS];
	uint16_t height[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1];
	uint16_t flags; // 0x1-has neighbor on -X, 0x2-has neighbor on +X, 0x4-has neighbor on -Y, 0x8-has neighbor on +Y
};

struct Terrain
{
	int x, y; // worldspace origin from tree origin
	int level; // 0 -> root is patch, -1 -> empty
	QuadItem* root;  // Node or Patch or NULL
};

Terrain* CreateTerrain(int water)
{
	Terrain* t = (Terrain*)malloc(sizeof(Terrain));
	t->x = 0;
	t->y = 0;
	t->level = 0;

	int flat = water + 1;

	Patch* p = (Patch*)malloc(sizeof(Patch));
	p->lo = 0; // (no neighbor)
	p->hi = flat;
	p->flags = 0;

	for (int y = 0; y < HEIGHT_CELLS + 1; y++)
		for (int x = 0; x < HEIGHT_CELLS + 1; x++)
			p->height[y][x] = flat;

	t->root = p;
	return t;
}

void DeleteTerrain(Terrain* t)
{
	if (!t->root)
		return;

	struct Recurse
	{
		static void Delete(QuadItem* q, int l)
		{
			if (l == 0)
			{
				Patch* p = (Patch*)q;
				free(p);
				return;
			}
			else
			{
				Node* n = (Node*)q;
				l--;

				for (int i=0; i<4; i++)
					if (n->quad[i])
						Delete(n->quad[i], l);
				free(n);
			}
		}
	};

	Recurse::Delete(t->root, t->level);
}

Patch* GetPatch(Terrain* t, int x, int y)
{
	if (!t->root)
		return 0;

	x += t->x;
	y += t->y;

	int range = 1 << t->level;
	if (x < 0 || y < 0 || x >= range || y >= range)
		return 0;

	if (t->level == 0)
		return (Patch*)t->root;

	int lev = t->level;

	Node* n = (Node*)t->root;
	while (n)
	{
		lev--;
		int i = ((x >> lev) & 1) | (((y >> lev) & 1) << 1);

		if (lev)
			n = (Node*)n->quad[i];
		else
			return (Patch*)n->quad[i];
	}

	return 0;
}


Patch* AddTerrainPatch(Terrain* t, int x, int y, int water)
{
	if (!t->root)
	{
		t->x = -x;
		t->y = -y;
		t->level = 0;

		int flat = water + 1;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->lo = 0; // no neighbor
		p->hi = flat;
		p->flags = 0;

		for (int y = 0; y < HEIGHT_CELLS + 1; y++)
			for (int x = 0; x < HEIGHT_CELLS + 1; x++)
				p->height[y][x] = flat;

		t->root = p;
		return p;
	}

	x += t->x;
	y += t->y;

	// create parents such root encloses x,y

	int range = 1 << t->level;

	while (x < 0)
	{
		Node* n = (Node*)malloc(sizeof(Node));

		if (2 * y < range)
		{
			n->quad[0] = 0;
			n->quad[1] = 0;
			n->quad[2] = 0;
			n->quad[3] = t->root;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->x += range;
			x += range;

			t->y += range;
			y += range;
		}
		else
		{
			n->quad[0] = 0;
			n->quad[1] = t->root;
			n->quad[2] = 0;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->x += range;
			x += range;
		}

		range *= 2;
	}

	while (y < 0)
	{
		Node* n = (Node*)malloc(sizeof(Node));

		if (2 * x < range)
		{
			n->quad[0] = 0;
			n->quad[1] = 0;
			n->quad[2] = 0;
			n->quad[3] = t->root;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->x += range;
			x += range;

			t->y += range;
			y += range;
		}
		else
		{
			n->quad[0] = 0;
			n->quad[1] = 0;
			n->quad[2] = t->root;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->y += range;
			y += range;
		}

		range *= 2;
	}

	while (x >= range)
	{
		Node* n = (Node*)malloc(sizeof(Node));

		if (2 * y > range)
		{
			n->quad[0] = t->root;
			n->quad[1] = 0;
			n->quad[2] = 0;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;
		}
		else
		{
			n->quad[0] = 0;
			n->quad[1] = 0;
			n->quad[2] = t->root;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->y += range;
			y += range;
		}

		range *= 2;
	}

	while (y >= range)
	{
		Node* n = (Node*)malloc(sizeof(Node));

		if (2 * x > range)
		{
			n->quad[0] = t->root;
			n->quad[1] = 0;
			n->quad[2] = 0;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;
		}
		else
		{
			n->quad[0] = 0;
			n->quad[1] = t->root;
			n->quad[2] = 0;
			n->quad[3] = 0;

			n->lo = t->root->lo;
			n->hi = t->root->hi;

			t->level++;

			t->x += range;
			x += range;
		}

		range *= 2;
	}

	// create children from root to x,y
	int lev = t->level;

	Node* n = (Node*)t->root;
	while (n)
	{
		lev--;
		int i = ((x >> lev) & 1) | (((y >> lev) & 1) << 1);

		if (lev)
		{
			if (!(Node*)n->quad[i])
			{
				Node* c = (Node*)malloc(sizeof(Node*));
				c->quad[0] = c->quad[1] = c->quad[2] = c->quad[3] = 0;
				n->quad[i] = c;
			}

			n = (Node*)n->quad[i];
		}
		else
		{
			if (n->quad[i])
				return (Patch*)n->quad[i];

			Patch* p = (Patch*)malloc(sizeof(Patch));

			// first of all check neighbors and adjust our and their flags!
			// ...

			// interpolate / extrapolate our coords with neighbours and accum lo,hi
			// ...

			// if we have any edge open, overwrite lo = 0
			// ...

			// at last, propagate our lo,hi to descendants down to the root
			// ...
		}
	}

	assert(0); // should never reach here
	return 0;
}
