
#include <stdint.h>
#include <math.h>
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
	QuadItem* parent;
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

Terrain* CreateTerrain(int z)
{
	Terrain* t = (Terrain*)malloc(sizeof(Terrain));
	t->x = 0;
	t->y = 0;

	if (z >= 0)
	{
		t->level = 0;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->parent = 0;
		p->lo = 0; // (no neighbor)
		p->hi = z;
		p->flags = 0;

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;

		t->root = p;
	}
	else
	{
		t->level = -1;
		t->root = 0;
	}

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

Patch* GetTerrainPatch(Terrain* t, int x, int y)
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

void UpdateNodes(Terrain* t, int x, int y)
{
	// update limits for nodes containing {x,y} patch (patch is already updated)
	// ...
}

Patch* AddTerrainPatch(Terrain* t, int x, int y, int z)
{
	if (z < 0)
		return 0;

	if (!t->root)
	{
		t->x = -x;
		t->y = -y;
		t->level = 0;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->parent = 0;
		p->lo = 0; // no neighbor
		p->hi = z;
		p->flags = 0;

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;

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

		n->parent = 0;
		t->root->parent = n;
		t->root = n;
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

		n->parent = 0;
		t->root->parent = n;
		t->root = n;
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

		n->parent = 0;
		t->root->parent = n;
		t->root = n;
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

		n->parent = 0;
		t->root->parent = n;
		t->root = n;
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
				c->parent = n;
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
			p->parent = n;
			p->flags = 0;

			for (int e = 0; e < HEIGHT_CELLS; x++)
			{
				p->height[0][e] = z;
				p->height[e][HEIGHT_CELLS] = z;
				p->height[HEIGHT_CELLS][HEIGHT_CELLS-e] = z;
				p->height[HEIGHT_CELLS - e][0] = z;
			}

			int nx = x - t->x, ny = y - t->y;

			Patch* np[4] =
			{
				GetTerrainPatch(t, nx - 1, ny - 1),
				GetTerrainPatch(t, nx + 1, ny - 1),
				GetTerrainPatch(t, nx - 1, ny + 1),
				GetTerrainPatch(t, nx + 1, ny + 1)
			};

			if (np[0])
			{
				np[0]->flags |= 2;
				p->flags |= 1;

				if (np[0]->flags == 0xF)
					UpdateNodes(t, nx - 1, ny - 1);

				for (int y = 0; y <= HEIGHT_CELLS; y++)
					p->height[y][0] = np[0]->height[y][HEIGHT_CELLS];
			}
			if (np[1])
			{
				np[0]->flags |= 1;
				p->flags |= 2;

				if (np[1]->flags == 0xF)
					UpdateNodes(t, nx + 1, ny - 1);

				for (int y = 0; y <= HEIGHT_CELLS; y++)
					p->height[y][HEIGHT_CELLS] = np[0]->height[y][0];
			}
			if (np[2])
			{
				np[0]->flags |= 8;
				p->flags |= 4;

				if (np[2]->flags == 0xF)
					UpdateNodes(t, nx - 1, ny + 1);

				for (int x = 0; x <= HEIGHT_CELLS; x++)
					p->height[0][x] = np[0]->height[HEIGHT_CELLS][x];
			}
			if (np[3])
			{
				np[0]->flags |= 4;
				p->flags |= 8;

				if (np[3]->flags == 0xF)
					UpdateNodes(t, nx + 1, ny + 1);

				for (int x = 0; x <= HEIGHT_CELLS; x++)
					p->height[HEIGHT_CELLS][x] = np[0]->height[0][x];
			}

			for (int y = 1; y < HEIGHT_CELLS; y++)
			{
				for (int x = 1; x < HEIGHT_CELLS; y++)
				{
					double avr = 0;
					double nrm = 0;

					for (int e = 0; e < HEIGHT_CELLS; x++)
					{
						double w;

						w = 1.0 / ((x - e)*(x - e) + y * y);
						nrm += w;
						avr += p->height[0][e] * w;

						w = 1.0 / ((x - HEIGHT_CELLS) * (x - HEIGHT_CELLS) + (y - e)*(y - e));
						nrm += w;
						avr += p->height[e][HEIGHT_CELLS] * w;

						w = 1.0 / ((x - HEIGHT_CELLS + e)*(x - HEIGHT_CELLS + e) + (y - HEIGHT_CELLS)*(y - HEIGHT_CELLS));
						nrm += w;
						avr += p->height[HEIGHT_CELLS][HEIGHT_CELLS - e] * w;

						w = 1.0 / (x * x + (y - HEIGHT_CELLS + e)*(y - HEIGHT_CELLS + e));
						nrm += w;
						avr += p->height[HEIGHT_CELLS - e][0] * w;
					}

					p->height[y][x] = (int)round(avr / nrm);
				}
			}

			p->lo = 0xffff;
			p->hi = 0x0000;
			for (int y = 0; y <= HEIGHT_CELLS; y++)
			{
				for (int x = 0; x <= HEIGHT_CELLS; x++)
				{
					p->lo = p->height[y][x] < p->lo ? p->height[y][x] : p->lo;
					p->hi = p->height[y][x] > p->hi ? p->height[y][x] : p->hi;
				}
			}

			if ((p->flags & 0xf) != 0xF)
				p->lo = 0;

			UpdateNodes(t, nx, ny);
			return p;
		}
	}

	assert(0); // should never reach here
	return 0;
}
