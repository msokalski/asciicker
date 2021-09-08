
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <float.h>

#ifdef EDITOR
#include "texheap.h"
#endif

#include "terrain.h"
#include "matrix.h"

#include "fast_rand.h"



inline int my_abs(int i)
{
    if (i<0)
	return -i;
    return i;
}


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



struct Node;

struct QuadItem
{
	Node* parent;
	uint16_t lo, hi;
	uint16_t flags; // 8 bits of neighbors, CCW, bit0 is on (-,-), bit7 is on (-,0)
};

struct Node : QuadItem
{
	QuadItem* quad[4]; // all 4 are same, either Nodes or Patches, at least 1 must not be NULL
};

struct Patch : QuadItem // 564 bytes (512x512 'raster' map would require 564KB)
{
#ifdef DARK_TERRAIN
	uint64_t dark; // (8x8)
#endif

	// visual contains:                grass, sand, rock,
	// 1bit elevation, 6bit material
	uint16_t visual[VISUAL_CELLS][VISUAL_CELLS];
	uint16_t height[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1];
	uint16_t diag; // (4x4)

#ifdef TEXHEAP
	TexAlloc* ta; // MUST BE AT THE TAIL OF STRUCT !!!
#endif
};

struct Terrain
{
	int x, y; // worldspace origin from tree origin
	int level; // 0 -> root is patch, -1 -> empty
	QuadItem* root;  // Node or Patch or NULL
	int nodes;
	int patches;

#ifdef TEXHEAP
	TexHeap th; // MUST BE AT THE TAIL OF STRUCT !!!
#endif
};

void GetTerrainBase(Terrain* t, int b[2])
{
	b[0] = t->x;
	b[1] = t->y;
}

void SetTerrainBase(Terrain* t, const int b[2])
{
	t->x = b[0];
	t->y = b[1];
}

Terrain* CreateTerrain(int z)
{
	Terrain* t = (Terrain*)malloc(sizeof(Terrain));
	t->x = 0;
	t->y = 0;
	t->nodes = 0;

#ifdef TEXHEAP

	int cap = TERRAIN_TEXHEAP_CAPACITY;

	TexDesc desc[2]=
	{
		{HEIGHT_CELLS + 1, HEIGHT_CELLS + 1, GL_R16UI},
		{VISUAL_CELLS, VISUAL_CELLS, GL_R16UI}
	};

	t->th.Create(cap,cap, 2, desc, sizeof(TexPageBuffer));
#endif

	if (z >= 0)
	{
		t->level = 0;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->parent = 0;
		p->lo = z;
		p->hi = z;
		p->flags = 0; // (no neighbor)

#ifdef DARK_TERRAIN
		p->dark = 0;
#endif

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;
		p->diag = 0;

		/*
		for (int y = 0; y < VISUAL_CELLS; y++)
			for (int x = 0; x < VISUAL_CELLS; x++)
				p->visual[y][x] = fast_rand();
		*/
		memset(p->visual,0x00,sizeof(uint16_t)*VISUAL_CELLS*VISUAL_CELLS);

		t->root = p;
		t->patches = 1;

#ifdef TEXHEAP
		TexData data[2]=
		{
			{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height},
			{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->visual},
		};
		p->ta = t->th.Alloc(data);
		p->ta->user = p;
#endif
	}
	else
	{
		t->level = -1;
		t->root = 0;
		t->patches = 0;
	}

	return t;
}

static void DeleteTerrain(Node* n, int lev)
{
	if (lev == 1)
	{
		for (int i = 0; i < 4; i++)
		{
			Patch* p = (Patch*)n->quad[i];
			if (p)
				free(p);
		}
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			Node* c = (Node*)n->quad[i];
			if (c)
				DeleteTerrain(c, lev - 1);
		}
	}

	free(n);
}

void DeleteTerrain(Terrain* t)
{
	if (!t)
		return;

#ifdef TEXHEAP
	t->th.Destroy();
#endif

	if (!t->root)
	{
		free(t);
		return;
	}

	if (t->level == 0)
	{
		Patch* p = (Patch*)t->root;
		free(p);
		free(t);
		return;
	}

	int lev = t->level;
	int xy = 0;
	Node* n = (Node*)t->root;
	free(t);

	DeleteTerrain(n, lev);

	/*

	while (true)
	{
	recurse:
		lev--;

		if (!lev)
		{
			for (int i = 0; i < 4; i++)
			{
				Patch* p = (Patch*)n->quad[i];
				if (p)
					free(p);
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				if (n->quad[i])
				{
					xy = (xy << 2) + i;
					n = (Node*)n->quad[i];
					goto recurse;
				}
			}
		}

		while (true)
		{
			Node* p = n->parent;
			free(n);

			if (!p)
				return;

			while ((xy & 3) < 3)
			{
				xy++;
				if (p->quad[xy & 3])
				{
					n = (Node*)p->quad[xy & 3];
					goto recurse;
				}
			}

			xy <<= 2;
			n = p;
			lev++;
		}
	}
	*/
}

struct Tap3x3
{
	Tap3x3(Patch* c)
	{
		assert(c);
		p[0][0] = GetTerrainNeighbor(c, -1, -1);
		p[0][1] = GetTerrainNeighbor(c, 0, -1);
		p[0][2] = GetTerrainNeighbor(c, +1, -1);
		p[1][0] = GetTerrainNeighbor(c, -1, 0);
		p[1][1] = c;
		p[1][2] = GetTerrainNeighbor(c, +1, 0);
		p[2][0] = GetTerrainNeighbor(c, -1, +1);
		p[2][1] = GetTerrainNeighbor(c, 0, +1);
		p[2][2] = GetTerrainNeighbor(c, +1, +1);
	}

	void SetDiag(int x, int y, bool d)
	{
		int px = 1, py = 1;

		if (x < 0)
		{
			x += HEIGHT_CELLS;
			px = 0;
		}
		else
		if (x >= HEIGHT_CELLS)
		{
			x -= HEIGHT_CELLS;
			px = 2;
		}

		if (y < 0)
		{
			y += HEIGHT_CELLS;
			py = 0;
		}
		else
		if (y >= HEIGHT_CELLS)
		{
			y -= HEIGHT_CELLS;
			py = 2;
		}

		if (p[py][px])
		{
			if (d)
				p[py][px]->diag |= 1 << (x + y * HEIGHT_CELLS);
			else
				p[py][px]->diag &= ~(1 << (x + y * HEIGHT_CELLS));
		}
		else
		{
			int a = 0;
		}
	}

	int Sample(int x, int y)
	{
		int px = 1, py = 1;

		if (x < 0)
		{
			x += HEIGHT_CELLS;
			px = 0;
		}
		else
		if (x /*>=*/ > HEIGHT_CELLS) // assuming '>' is fresher
		{
			x -= HEIGHT_CELLS;
			px = 2;
		}

		if (y < 0)
		{
			y += HEIGHT_CELLS;
			py = 0;
		}
		else
		if (y /*>=*/ > HEIGHT_CELLS) // assuming '>' is fresher
		{
			y -= HEIGHT_CELLS;
			py = 2;
		}

		if (!p[py][px])
		{
			if (px == 0)
				x = 0;
			else
			if (px == 2)
				x = HEIGHT_CELLS;

			if (py == 0)
				y = 0;
			else
			if (py == 2)
				y = HEIGHT_CELLS;

			px = 1;
			py = 1;
		}

		return p[py][px]->height[y][x];
	}

	void Update()
	{
		for (int y = -1; y <= HEIGHT_CELLS; y++)
		{
			for (int x = -1; x <= HEIGHT_CELLS; x++)
			{
				int c0 =
					+ Sample(x + 2, y + 2)
					+ Sample(x + 1, y + 2)
					+ Sample(x + 2, y + 1)
					+ Sample(x - 1, y - 1)
					+ Sample(x - 1, y)
					+ Sample(x, y - 1)
					- Sample(x, y)
					- Sample(x + 1, y + 1)
					- Sample(x + 1, y) * 2
					- Sample(x, y + 1) * 2;

				int c1 =
					+ Sample(x - 1, y + 2)
					+ Sample(x - 1, y + 1)
					+ Sample(x, y + 2)
					+ Sample(x + 2, y - 1)
					+ Sample(x + 1, y - 1)
					+ Sample(x + 2, y)
					- Sample(x, y + 1)
					- Sample(x + 1, y)
					- Sample(x, y) * 2
					- Sample(x + 1, y + 1) * 2;

				SetDiag(x, y, my_abs(c0) > my_abs(c1));
			}
		}
	}

	Patch* p[3][3];
};

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

void GetTerrainPatch(Terrain* t, Patch* p, int* x, int* y)
{
	int px = 0, py = 0;

	int lev = 0;
	QuadItem* q = p;
	Node* n = p->parent;
	while (n)
	{
		for (int i=0; i<4; i++)
			if (n->quad[i] == q)
			{
				px = px | ((i & 1) << lev);
				py = py | (((i>>1) & 1) << lev);
				break;
			}

		q = n;
		n = n->parent;
		lev++;
	}

	if (x)
		*x = px - t->x;
	if (x)
		*y = py - t->y;
}

static void UpdateNodes(Patch* p)
{
	QuadItem* q = p;
	Node* n = p->parent;

	while (n)
	{
		int lo = 0xffff;
		int hi = 0x0000;
		int fl = 0xFF;

		for (int i = 0; i < 4; i++)
		{
			if (n->quad[i])
			{
				lo = n->quad[i]->lo < lo ? n->quad[i]->lo : lo;
				hi = n->quad[i]->hi > hi ? n->quad[i]->hi : hi;
				fl = fl & n->quad[i]->flags;
			}
		}

		n->lo = lo;
		n->hi = hi;
		n->flags = fl;

		n = n->parent;
	}
}

bool DelTerrainPatch(Terrain* t, int x, int y)
{
	Patch* p = GetTerrainPatch(t, x, y);
	if (!p)
		return false;

	int flags = p->flags;
	Node* n = p->parent;

#ifdef TEXHEAP
	TexAlloc* last = p->ta->Free();
	if (last)
	{
		Patch* l = (Patch*)last->user;
		UpdateTerrainVisualMap(l);
		UpdateTerrainHeightMap(l);
	}
#endif
	free(p);

	t->patches--;

	if (!n)
	{
		t->level = -1;
		t->root = 0;
		return true;
	}

	// leaf trim

	QuadItem* q = p;

	while (true)
	{
		int c = 0;
		for (int i = 0; i < 4; i++)
		{
			if (n->quad[i] == q)
				n->quad[i] = 0;
			else
			if (n->quad[i])
				c++;
		}

		if (!c)
		{
			q = n;
			n = n->parent;
			free((Node*)q);
			t->nodes--;
		}
		else
			break;
	}

	// root trim

	n = (Node*)t->root;
	while (true)
	{
		int c = 0;
		int j = -1;
		for (int i = 0; i < 4; i++)
		{
			if (n->quad[i])
			{
				j = i;
				c++;
			}
		}

		// lost ...
		assert(j >= 0);

		if (c > 1)
			break;

		t->level--;

		if (j & 1)
			t->x -= 1 << t->level;
		if (j & 2)
			t->y -= 1 << t->level;

		t->root = n->quad[j];
		t->root->parent = 0;
		free(n);
		t->nodes--;

		if (t->level)
			n = (Node*)n->quad[j];
		else
			break;
	}

	Patch* np[8] =
	{
		flags & 0x01 ? GetTerrainPatch(t, x - 1, y - 1) : 0,
		flags & 0x02 ? GetTerrainPatch(t, x, y - 1) : 0,
		flags & 0x04 ? GetTerrainPatch(t, x + 1, y - 1) : 0,
		flags & 0x08 ? GetTerrainPatch(t, x + 1, y) : 0,
		flags & 0x10 ? GetTerrainPatch(t, x + 1, y + 1) : 0,
		flags & 0x20 ? GetTerrainPatch(t, x, y + 1) : 0,
		flags & 0x40 ? GetTerrainPatch(t, x - 1, y + 1) : 0,
		flags & 0x80 ? GetTerrainPatch(t, x - 1, y) : 0,
	};

	for (int i = 0; i < 8; i++)
	{
		if (np[i])
		{
			int j = (i + 4) & 7;
			np[i]->flags &= ~(1 << j);
		}
	}

	return true;
}

Patch* AddTerrainPatch(Terrain* t, int x, int y, int z)
{
	if (!t->root)
	{
		t->x = -x;
		t->y = -y;
		t->level = 0;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->parent = 0;
		p->lo = z;
		p->hi = z;
		p->flags = 0; // no neighbor

#ifdef DARK_TERRAIN
		p->dark = 0;
#endif

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;
		p->diag = 0;

		/*
		for (int y = 0; y < VISUAL_CELLS; y++)
			for (int x = 0; x < VISUAL_CELLS; x++)
				p->visual[y][x] = fast_rand();
		*/
	
		memset(p->visual,0x00,sizeof(uint16_t)*VISUAL_CELLS*VISUAL_CELLS);

		t->root = p;
		t->patches = 1;

#ifdef TEXHEAP
		TexData data[2] =
		{
			{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height},
			{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->visual},
		};
		p->ta = t->th.Alloc(data);
		p->ta->user = p;
#endif

		return p;
	}

	x += t->x;
	y += t->y;

	// create parents such root encloses x,y

	int range = 1 << t->level;

	while (x < 0)
	{
		Node* n = (Node*)malloc(sizeof(Node));
		t->nodes++;

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
		t->nodes++;

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
		t->nodes++;

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
		t->nodes++;

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

	int lev = t->level;

	if (lev == 0)
	{
		// already exist
		return (Patch*)t->root;
	}

	// create children from root to x,y

	Node* n = (Node*)t->root;
	while (n)
	{
		lev--;
		int i = ((x >> lev) & 1) | (((y >> lev) & 1) << 1);

		if (lev)
		{
			if (!(Node*)n->quad[i])
			{
				Node* c = (Node*)malloc(sizeof(Node));
				t->nodes++;

				c->parent = n;
				c->quad[0] = c->quad[1] = c->quad[2] = c->quad[3] = 0;
				n->quad[i] = c;
			}

			n = (Node*)n->quad[i];
		}
		else
		{
			if (n->quad[i])
			{
				// already exist
				return (Patch*)n->quad[i];
			}

			Patch* p = (Patch*)malloc(sizeof(Patch));
			t->patches++;

			n->quad[i] = p;
			p->parent = n;
			p->flags = 0;

#ifdef DARK_TERRAIN
			p->dark = 0;
#endif

			p->diag = 0;

			int nx = x - t->x, ny = y - t->y;

			Patch* np[8] =
			{
				GetTerrainPatch(t, nx - 1, ny - 1),
				GetTerrainPatch(t, nx, ny - 1),
				GetTerrainPatch(t, nx + 1, ny - 1),
				GetTerrainPatch(t, nx + 1, ny),
				GetTerrainPatch(t, nx + 1, ny + 1),
				GetTerrainPatch(t, nx, ny + 1),
				GetTerrainPatch(t, nx - 1, ny + 1),
				GetTerrainPatch(t, nx - 1, ny),
			};

			for (int i = 0; i < 8; i++)
			{
				if (np[i])
				{
					int f = np[i]->flags;
					int j = (i + 4) & 7;
					np[i]->flags |= 1 << j;
					p->flags |= 1 << i;

					if (f!=np[i]->flags)
						UpdateNodes(np[i]);

					// fill shared vertices

					switch (i)
					{
						case 0:
							p->height[0][0] = np[i]->height[HEIGHT_CELLS][HEIGHT_CELLS];
							break;

						case 1:
							for (int x=0; x<= HEIGHT_CELLS; x++)
								p->height[0][x] = np[i]->height[HEIGHT_CELLS][x];
							break;

						case 2:
							p->height[0][HEIGHT_CELLS] = np[i]->height[HEIGHT_CELLS][0];
							break;

						case 3:
							for (int y = 0; y <= HEIGHT_CELLS; y++)
								p->height[y][HEIGHT_CELLS] = np[i]->height[y][0];
							break;

						case 4:
							p->height[HEIGHT_CELLS][HEIGHT_CELLS] = np[i]->height[0][0];
							break;

						case 5:
							for (int x = 0; x <= HEIGHT_CELLS; x++)
								p->height[HEIGHT_CELLS][x] = np[i]->height[0][x];
							break;

						case 6:
							p->height[HEIGHT_CELLS][0] = np[i]->height[0][HEIGHT_CELLS];
							break;

						case 7:
							for (int y = 0; y <= HEIGHT_CELLS; y++)
								p->height[y][0] = np[i]->height[y][HEIGHT_CELLS];
							break;
					}
				}
			}

			// set free corners

			if (!(p->flags & 0x83))
				p->height[0][0] = z;

			if (!(p->flags & 0x0E))
				p->height[0][HEIGHT_CELLS] = z;

			if (!(p->flags & 0x38))
				p->height[HEIGHT_CELLS][HEIGHT_CELLS] = z;

			if (!(p->flags & 0xE0))
				p->height[HEIGHT_CELLS][0] = z;

			// interpolate free edges

			if (!(p->flags & 0x02))
			{
				// bottom
				int y = 0;
				int h0 = p->height[y][0];
				int h1 = p->height[y][HEIGHT_CELLS];
				for (int x = 1; x < HEIGHT_CELLS; x++)
					p->height[y][x] = (h0 * (HEIGHT_CELLS - x) + h1 * x + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
			}

			if (!(p->flags & 0x08))
			{
				// right
				int x = HEIGHT_CELLS;
				int h0 = p->height[0][x];
				int h1 = p->height[HEIGHT_CELLS][x];
				for (int y = 1; y < HEIGHT_CELLS; y++)
					p->height[y][x] = (h0 * (HEIGHT_CELLS - y) + h1 * y + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
			}

			if (!(p->flags & 0x20))
			{
				// top
				int y = HEIGHT_CELLS;
				int h0 = p->height[y][0];
				int h1 = p->height[y][HEIGHT_CELLS];
				for (int x = 1; x < HEIGHT_CELLS; x++)
					p->height[y][x] = (h0 * (HEIGHT_CELLS - x) + h1 * x + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
			}

			if (!(p->flags & 0x80))
			{
				// left
				int x = 0;
				int h0 = p->height[0][x];
				int h1 = p->height[HEIGHT_CELLS][x];
				for (int y = 1; y < HEIGHT_CELLS; y++)
					p->height[y][x] = (h0 * (HEIGHT_CELLS - y) + h1 * y + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
			}

			// interpolate inter-patch vertices

			for (int y = 1; y < HEIGHT_CELLS; y++)
			{
				for (int x = 1; x < HEIGHT_CELLS; x++)
				{
					double avr = 0;
					double nrm = 0;

					for (int e = 0; e < HEIGHT_CELLS; e++)
					{
						double w;

						w = 1.0 / sqrt((x - e)*(x - e) + y * y);
						nrm += w;
						avr += p->height[0][e] * w;

						w = 1.0 / sqrt((x - HEIGHT_CELLS) * (x - HEIGHT_CELLS) + (y - e)*(y - e));
						nrm += w;
						avr += p->height[e][HEIGHT_CELLS] * w;

						w = 1.0 / sqrt((x - HEIGHT_CELLS + e)*(x - HEIGHT_CELLS + e) + (y - HEIGHT_CELLS)*(y - HEIGHT_CELLS));
						nrm += w;
						avr += p->height[HEIGHT_CELLS][HEIGHT_CELLS - e] * w;

						w = 1.0 / sqrt(x * x + (y - HEIGHT_CELLS + e)*(y - HEIGHT_CELLS + e));
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

			/*
			for (int y = 0; y < VISUAL_CELLS; y++)
				for (int x = 0; x < VISUAL_CELLS; x++)
					p->visual[y][x] = fast_rand();
			*/
			memset(p->visual,0x00,sizeof(uint16_t)*VISUAL_CELLS*VISUAL_CELLS);

			// update diag flags
			Tap3x3 tap(p);
			tap.Update();

			UpdateNodes(p);

#ifdef TEXHEAP
			TexData data[2] =
			{
				{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height},
				{GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->visual},
			};
			p->ta = t->th.Alloc(data);
			p->ta->user = p;
#endif

			return p;
		}
	}

	assert(0); // should never reach here
	return 0;
}

Patch* GetTerrainNeighbor(Patch* p, int dx, int dy)
{
	if (dx == 0 && dy == 0)
		return p;

	int r = 1;

	QuadItem* q = p;
	Node* n = q->parent;

	while (n)
	{
		if (n->quad[1] == q)
			dx += r;
		else
		if (n->quad[2] == q)
			dy += r;
		else
		if (n->quad[3] == q)
		{
			dx += r;
			dy += r;
		}
		else
			assert(n->quad[0] == q);

		r <<= 1;

		if (dx >= 0 && dx < r && dy >= 0 && dy < r)
			break; // in range!

		q = n;
		n = n->parent;
	}

	while (n)
	{
		int hr = r >> 1;

		int i = 0;
		if (dx >= hr)
		{
			i |= 1;
			dx -= hr;
		}

		if (dy >= hr)
		{
			i |= 2;
			dy -= hr;
		}

		if (hr == 1)
			return (Patch*)n->quad[i];

		r = hr;
		n = (Node*)n->quad[i];
	}
	   
	return 0;
}


int GetTerrainPatches(Terrain* t)
{
	return t->patches;
}

size_t GetTerrainBytes(Terrain* t)
{
	return t->patches * sizeof(Patch) + t->nodes * sizeof(Node);
}


uint16_t* GetTerrainHeightMap(Patch* p)
{
	return (uint16_t*)p->height;
}

uint16_t* GetTerrainVisualMap(Patch* p)
{
	return (uint16_t*)p->visual;
}

Patch* CalcTerrainGhost(Terrain* t, int x, int y, int z, uint16_t ghost[4 * HEIGHT_CELLS])
{
	Patch* p = GetTerrainPatch(t, x, y);
	if (p)
	{
		int i = 0;
		for (int x = 0; x < HEIGHT_CELLS; x++)
			ghost[i++] = p->height[0][x];
		for (int y = 0; y < HEIGHT_CELLS; y++)
			ghost[i++] = p->height[y][HEIGHT_CELLS];
		for (int x = HEIGHT_CELLS; x > 0; x--)
			ghost[i++] = p->height[HEIGHT_CELLS][x];
		for (int y = HEIGHT_CELLS; y > 0; y--)
			ghost[i++] = p->height[y][0];

		return p;
	}

	int nx = x, ny = y;

	Patch* np[8] =
	{
		GetTerrainPatch(t, nx - 1, ny - 1),
		GetTerrainPatch(t, nx, ny - 1),
		GetTerrainPatch(t, nx + 1, ny - 1),
		GetTerrainPatch(t, nx + 1, ny),
		GetTerrainPatch(t, nx + 1, ny + 1),
		GetTerrainPatch(t, nx, ny + 1),
		GetTerrainPatch(t, nx - 1, ny + 1),
		GetTerrainPatch(t, nx - 1, ny),
	};

	int flags = 0;

	for (int i = 0; i < 8; i++)
	{
		if (np[i])
		{
			flags |= 1 << i;

			// fill shared vertices

			switch (i)
			{
			case 0:
				ghost[0] = np[i]->height[HEIGHT_CELLS][HEIGHT_CELLS];
				break;

			case 1:
				for (int x = 0; x <= HEIGHT_CELLS; x++)
					ghost[x] = np[i]->height[HEIGHT_CELLS][x];
				break;

			case 2:
				ghost[HEIGHT_CELLS] = np[i]->height[HEIGHT_CELLS][0];
				break;

			case 3:
				for (int y = 0; y <= HEIGHT_CELLS; y++)
					ghost[HEIGHT_CELLS+y] = np[i]->height[y][0];
				break;

			case 4:
				ghost[2*HEIGHT_CELLS] = np[i]->height[0][0];
				break;

			case 5:
				for (int x = 0; x <= HEIGHT_CELLS; x++)
					ghost[3*HEIGHT_CELLS-x] = np[i]->height[0][x];
				break;

			case 6:
				ghost[3*HEIGHT_CELLS] = np[i]->height[0][HEIGHT_CELLS];
				break;

			case 7:
				ghost[0] = np[i]->height[0][HEIGHT_CELLS];
				for (int y = 1; y <= HEIGHT_CELLS; y++)
					ghost[4*HEIGHT_CELLS-y] = np[i]->height[y][HEIGHT_CELLS];
				break;
			}
		}
	}

	// set free corners

	if (!(flags & 0x83))
		ghost[0] = z;

	if (!(flags & 0x0E))
		ghost[HEIGHT_CELLS] = z;

	if (!(flags & 0x38))
		ghost[2*HEIGHT_CELLS] = z;

	if (!(flags & 0xE0))
		ghost[3*HEIGHT_CELLS] = z;

	// interpolate free edges

	if (!(flags & 0x02))
	{
		// bottom
		int y = 0;
		int h0 = ghost[0];
		int h1 = ghost[HEIGHT_CELLS];
		for (int x = 1; x < HEIGHT_CELLS; x++)
			ghost[x] = (h0 * (HEIGHT_CELLS - x) + h1 * x + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
	}

	if (!(flags & 0x08))
	{
		// right
		int x = HEIGHT_CELLS;
		int h0 = ghost[HEIGHT_CELLS];
		int h1 = ghost[2*HEIGHT_CELLS];;
		for (int y = 1; y < HEIGHT_CELLS; y++)
			ghost[HEIGHT_CELLS+y] = (h0 * (HEIGHT_CELLS - y) + h1 * y + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
	}

	if (!(flags & 0x20))
	{
		// top
		int y = HEIGHT_CELLS;
		int h0 = ghost[3*HEIGHT_CELLS];
		int h1 = ghost[2*HEIGHT_CELLS];
		for (int x = 1; x < HEIGHT_CELLS; x++)
			ghost[3*HEIGHT_CELLS-x] = (h0 * (HEIGHT_CELLS - x) + h1 * x + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
	}

	if (!(flags & 0x80))
	{
		// left
		int x = 0;
		int h0 = ghost[0];
		int h1 = ghost[3*HEIGHT_CELLS];
		for (int y = 1; y < HEIGHT_CELLS; y++)
			ghost[4*HEIGHT_CELLS-y] = (h0 * (HEIGHT_CELLS - y) + h1 * y + HEIGHT_CELLS / 2) / HEIGHT_CELLS;
	}

	return 0;
}


void GetTerrainLimits(Patch* p, uint16_t* lo, uint16_t* hi)
{
	if (lo)
		*lo = p->lo;
	if (hi)
		*hi = p->hi;
}


void UpdateTerrainHeightMap(Patch* p)
{
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

	Tap3x3 tap(p);
	tap.Update();

#ifdef TEXHEAP
	TexData data = { GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height };
	p->ta->Update(0, 1, &data); // ONLY HEIGHT !!!
#endif

	UpdateNodes(p);
}

void UpdateTerrainVisualMap(Patch* p)
{
#ifdef TEXHEAP
	TexData data = { GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->visual };
	p->ta->Update(1, 1, &data); // ONLY VISUAL !!!
#endif
}

#ifdef TEXHEAP
TexHeap* GetTerrainTexHeap(Terrain* t)
{
	return &t->th;
}

TexAlloc* GetTerrainTexAlloc(Patch* p)
{
	return p->ta;
}
#endif

uint16_t GetTerrainHi(Patch* p, uint16_t* lo)
{
	if (lo)
		*lo = p->lo;
	return p->hi;
}

uint16_t GetTerrainDiag(Patch* p)
{
	return p->diag;
}

void SetTerrainDiag(Patch* p, uint16_t diag)
{
	p->diag = diag;
}

#ifdef DARK_TERRAIN
uint64_t GetTerrainDark(Patch* p)
{
	return p->dark;
}

void SetTerrainDark(Patch* p, uint64_t dark)
{
	p->dark = dark;
}
#endif

void QueryTerrainSample(Patch* p, int x, int y, void(*cb)(Patch* p, int u, int v, double coords[3], void* cookie), void* cookie)
{
	static const double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;

	for (int v = 0; v < VISUAL_CELLS; v++)
	{
		double fv = (2 * v + 1) * HEIGHT_CELLS / (2.0 * VISUAL_CELLS);
		int hy = (int)floor(fv);
		fv -= hy;

		for (int u = 0; u < VISUAL_CELLS; u++)
		{
			// get the geometry triangle for visual uv

			double fu = (2 * u + 1) * HEIGHT_CELLS / (2.0 * VISUAL_CELLS);
			int hx = (int)floor(fu);
			fu -= hx;

			double h;

			bool rot = p->diag & (1 << (hx + hy * HEIGHT_CELLS));

			if (rot)
			{
				if (u < VISUAL_CELLS - v)
				{
					// v[2], v[0], v[1]
					h = p->height[hy][hx] +
						fu * (p->height[hy][hx + 1] - p->height[hy][hx]) +
						fv * (p->height[hy + 1][hx] - p->height[hy][hx]);
				}
				else
				{
					// v[2], v[1], v[3]
					h = p->height[hy + 1][hx + 1] +
						(1 - fu) * (p->height[hy + 1][hx] - p->height[hy + 1][hx + 1]) +
						(1 - fv) * (p->height[hy][hx + 1] - p->height[hy + 1][hx + 1]);
				}
			}
			else
			{
				if (u < y)
				{
					// v[0], v[3], v[2]
					h = p->height[hy][hx] +
						fu * (p->height[hy + 1][hx + 1] - p->height[hy + 1][hx]) +
						fv * (p->height[hy + 1][hx] - p->height[hy][hx]);
				}
				else
				{
					// v[0], v[1], v[3]
					h = p->height[hy][hx] +
						fu * (p->height[hy][hx + 1] - p->height[hy][hx]) +
						fv * (p->height[hy + 1][hx + 1] - p->height[hy][hx + 1]);
				}
			}

			double pnt[] = { x + u + 0.5, y + v + 0.5, h};
			cb(p, u,v, pnt, cookie);
		}
	}
}

void QueryTerrainSample(QuadItem* q, int x, int y, int range, void(*cb)(Patch* p, int u, int v, double coords[3], void* cookie), void* cookie)
{
	if (range > VISUAL_CELLS)
	{
		range >>= 1;
		Node* n = (Node*)q;
		if (n->quad[0])
			QueryTerrainSample(n->quad[0], x, y, range, cb, cookie);
		if (n->quad[1])
			QueryTerrainSample(n->quad[1], x + range, y, range, cb, cookie);
		if (n->quad[2])
			QueryTerrainSample(n->quad[2], x, y + range, range, cb, cookie);
		if (n->quad[3])
			QueryTerrainSample(n->quad[3], x + range, y + range, range, cb, cookie);
	}
	else
		QueryTerrainSample((Patch*)q, x, y, cb, cookie);
}

#ifdef DARK_TERRAIN
void UpdateTerrainDark(Terrain* t, World* w, float lightpos[3], bool editor)
{
	struct Updater
	{
		static void cb(Patch* p, int u, int v, double coords[3], void* cookie)
		{
			Updater* updater = (Updater*)cookie;

			double hit[3] = { coords[0], coords[1], coords[2] };
			Patch* q = HitTerrain(updater->t, coords, updater->lightdir, hit, 0);

			uint64_t mask = ((uint64_t)1) << (u + VISUAL_CELLS * v);

			if (q)
			{
				if (/*q != p || */ hit[2] > coords[2] + HEIGHT_SCALE/4)
				{
					p->dark |= mask;
					return;
				}
			}

			Inst* i = HitWorld(updater->w, coords, updater->lightdir, hit, 0, false, updater->editor);

			if (i)
			{
				if (hit[2] > coords[2])
				{
					p->dark |= mask;
					return;
				}
			}

			p->dark &= ~mask;
		}

		bool editor;
		Terrain* t;
		World* w;
		double lightdir[3];
	};

	Updater updater = { editor, t, w, {-lightpos[0], -lightpos[1], -lightpos[2] * HEIGHT_SCALE} };

//	double n = 1.0 / sqrt(lightpos[0]* lightpos[0]+ lightpos[1]* lightpos[1]+ lightpos[2]* lightpos[2]);
//	updater.lightdir[0] *= n;
//	updater.lightdir[1] *= n;
//	updater.lightdir[2] *= n;

	if (t->root)
		QueryTerrainSample(t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, Updater::cb, &updater);
}
#endif

static inline /*__forceinline*/ void QueryTerrain(QuadItem* q, int x, int y, int range, int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
{
	if (range == VISUAL_CELLS)
	{
		cb((Patch*)q, x, y, view_flags & ~q->flags, cookie);
	}
	else
	{
		Node* n = (Node*)q;

		range >>= 1;

		if (n->quad[0])
			QueryTerrain(n->quad[0], x, y, range, view_flags, cb, cookie);
		if (n->quad[1])
			QueryTerrain(n->quad[1], x + range, y, range, view_flags, cb, cookie);
		if (n->quad[2])
			QueryTerrain(n->quad[2], x, y + range, range, view_flags, cb, cookie);
		if (n->quad[3])
			QueryTerrain(n->quad[3], x + range, y + range, range, view_flags, cb, cookie);
	}
}

static void inline /*__forceinline*/ QueryTerrain(QuadItem* q, int x, int y, int range, int planes, double* plane[], int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
{
	int hi = q->hi;
	int lo = q->lo;
	int fl = view_flags & ~q->flags;

	if (fl)
		lo = 0;

	int c[4] = { x, y, lo, 1 }; // 0,0,0

	for (int i = 0; i < planes; i++)
	{
		int neg_pos[2] = { 0,0 };

		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[0] += range; // 1,0,0
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[1] += range; // 1,1,0
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[0] -= range; // 0,1,0
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[2] = hi; // 0,1,1
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[0] += range; // 1,1,1
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[1] -= range; // 1,0,1
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[0] -= range; // 0,0,1
		neg_pos[PositiveProduct(plane[i], c)] ++;

		c[2] = lo; // 0,0,0

		if (neg_pos[0] == 8)
			return;

		if (neg_pos[1] == 8)
		{
			planes--;
			if (i < planes)
			{
				double* swap = plane[i];
				plane[i] = plane[planes];
				plane[planes] = swap;
			}
			i--;
		}
	}

	if (range == VISUAL_CELLS)
	{
		cb((Patch*)q, x, y, fl, cookie);
	}
	else
	{
		Node* n = (Node*)q;
		
		range >>= 1;

		if (!planes)
		{
			if (n->quad[0])
				QueryTerrain(n->quad[0], x, y, range, view_flags, cb, cookie);
			if (n->quad[1])
				QueryTerrain(n->quad[1], x + range, y, range, view_flags, cb, cookie);
			if (n->quad[2])
				QueryTerrain(n->quad[2], x, y + range, range, view_flags, cb, cookie);
			if (n->quad[3])
				QueryTerrain(n->quad[3], x + range, y + range, range, view_flags, cb, cookie);
		}
		else
		{
			if (n->quad[0])
				QueryTerrain(n->quad[0], x, y, range, planes, plane, view_flags, cb, cookie);
			if (n->quad[1])
				QueryTerrain(n->quad[1], x + range, y, range, planes, plane, view_flags, cb, cookie);
			if (n->quad[2])
				QueryTerrain(n->quad[2], x, y + range, range, planes, plane, view_flags, cb, cookie);
			if (n->quad[3])
				QueryTerrain(n->quad[3], x + range, y + range, range, planes, plane, view_flags, cb, cookie);
		}
	}
}

void QueryTerrain(Terrain* t, int planes, double plane[][4], int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
{
	if (!t || !t->root)
		return;

	if (planes<=0)
		QueryTerrain(t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, view_flags & 0xAA, cb, cookie);
	else
	{
		double* pp[6] = { plane[0],plane[1],plane[2],plane[3],plane[4],plane[5] };
		QueryTerrain(t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, planes, pp, view_flags & 0xAA, cb, cookie);
	}
}


void QueryTerrain(QuadItem* q, int x, int y, int range, const double xyr[3], int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
{
	int hit = 0;

	double rr = xyr[2] * xyr[2];
	double dx0 = xyr[0] - x; dx0 *= dx0;
	double dy0 = xyr[1] - y; dy0 *= dy0;
	double dx1 = xyr[0] - x - range; dx1 *= dx1;
	double dy1 = xyr[1] - y - range; dy1 *= dy1;

	if (dx0 + dy0 < rr) 
		hit++;

	if (dx1 + dy0 < rr)
		hit++;

	if (dx0 + dy1 < rr)
		hit++;
	
	if (dx1 + dy1 < rr)
		hit++;

	if (!hit)
	{
		bool fit_x = xyr[0] >= x && xyr[0] <= x + range;
		bool fit_y = xyr[1] >= y && xyr[1] <= y + range;

		if (fit_y && xyr[0] >= x - xyr[2] && xyr[0] <= x + range + xyr[2] ||
			fit_x && xyr[1] >= y - xyr[2] && xyr[1] <= y + range + xyr[2])
		{
			hit = 1;
		}
		else
			return;
	}

	if (range == VISUAL_CELLS)
	{
		cb((Patch*)q, x, y, view_flags & ~q->flags, cookie);
	}
	else
	{
		Node* n = (Node*)q;
		range >>= 1;

		if (hit == 4)
		{
			if (n->quad[0])
				QueryTerrain(n->quad[0], x, y, range, view_flags, cb, cookie);
			if (n->quad[1])
				QueryTerrain(n->quad[1], x + range, y, range, view_flags, cb, cookie);
			if (n->quad[2])
				QueryTerrain(n->quad[2], x, y + range, range, view_flags, cb, cookie);
			if (n->quad[3])
				QueryTerrain(n->quad[3], x + range, y + range, range, view_flags, cb, cookie);
		}
		else
		{
			if (n->quad[0])
				QueryTerrain(n->quad[0], x, y, range, xyr, view_flags, cb, cookie);
			if (n->quad[1])
				QueryTerrain(n->quad[1], x + range, y, range, xyr, view_flags, cb, cookie);
			if (n->quad[2])
				QueryTerrain(n->quad[2], x, y + range, range, xyr, view_flags, cb, cookie);
			if (n->quad[3])
				QueryTerrain(n->quad[3], x + range, y + range, range, xyr, view_flags, cb, cookie);
		}
	}
}

void QueryTerrain(Terrain* t, double x, double y, double r, int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
{
	if (!t || !t->root)
		return;

	if (r <= 0)
		return;

	double xyr[3] = { x,y,r };

	QueryTerrain(t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, xyr, view_flags & 0xAA, cb, cookie);
}


int triangle_intersections = 0; 
int hit_patch_tests = 0;
bool HitPatch(Patch* p, int x, int y, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	hit_patch_tests ++;
	static const double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;
	bool hit = false;

	int rot = p->diag;

	for (int hy = 0; hy < HEIGHT_CELLS; hy++)
	{
		for (int hx = 0; hx < HEIGHT_CELLS; hx++)
		{
			double x0 = x + hx * sxy, x1 = x0 + sxy;
			double y0 = y + hy * sxy, y1 = y0 + sxy;

			double v[4][3] =
			{
				{x0,y0,(double)p->height[hy][hx]},
				{x1,y0,(double)p->height[hy][hx+1]},
				{x0,y1,(double)p->height[hy+1][hx]},
				{x1,y1,(double)p->height[hy+1][hx+1]},
			};

			if (rot & 1)
			{
				triangle_intersections++;
				if (RayIntersectsTriangle(ray, v[2], v[0], v[1], ret, positive_only))
				{
					hit |= 1;

					if (nrm)
					{
						double e1[3] = {v[0][0]-v[2][0],v[0][1]-v[2][1],v[0][2]-v[2][2]};
						double e2[3] = {v[1][0]-v[2][0],v[1][1]-v[2][1],v[1][2]-v[2][2]};
						CrossProduct(e1,e2,nrm);
					}
				}

				triangle_intersections++;
				if (RayIntersectsTriangle(ray, v[2], v[1], v[3], ret, positive_only))
				{
					hit |= 1;

					if (nrm)
					{
						double e1[3] = {v[1][0]-v[2][0],v[1][1]-v[2][1],v[1][2]-v[2][2]};
						double e2[3] = {v[3][0]-v[2][0],v[3][1]-v[2][1],v[3][2]-v[2][2]};
						CrossProduct(e1,e2,nrm);
					}
				}
			}
			else
			{
				triangle_intersections++;
				if (RayIntersectsTriangle(ray, v[0], v[3], v[2], ret, positive_only))
				{
					hit |= 1;

					if (nrm)
					{
						double e1[3] = {v[3][0]-v[0][0],v[3][1]-v[0][1],v[3][2]-v[0][2]};
						double e2[3] = {v[2][0]-v[0][0],v[2][1]-v[0][1],v[2][2]-v[0][2]};
						CrossProduct(e1,e2,nrm);
					}
				}

				triangle_intersections++;
				if (RayIntersectsTriangle(ray, v[0], v[1], v[3], ret, positive_only))
				{
					hit |= 1;

					if (nrm)
					{
						double e1[3] = {v[1][0]-v[0][0],v[1][1]-v[0][1],v[1][2]-v[0][2]};
						double e2[3] = {v[3][0]-v[0][0],v[3][1]-v[0][1],v[3][2]-v[0][2]};
						CrossProduct(e1,e2,nrm);
					}
				}
			}

			rot >>= 1;
		}
	}

	return hit;
}

Patch* HitTerrain0(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[1] - qlo * ray[3] + ray[5] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qlo * ray[4] > 0 ||
		ray[2] - ray[4] * x + ray[3] * (y + range) > 0 ||
		qhi * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[0] + qhi * ray[4] - ray[5] * y > 0 ||
		ray[4] * (x + range) - ray[3] * y - ray[2] > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain0(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain0(n->quad[1], x+range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain0(n->quad[2], x, y+range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain0(n->quad[3], x+range, y+range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain1(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[5] * (y + range) - ray[0] - qlo * ray[4] > 0 ||
		qlo * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[2] - ray[4] * x + ray[3] * y > 0 ||
		ray[0] + qhi * ray[4] - ray[5] * y > 0 ||
		ray[1] - qhi * ray[3] + ray[5] * (x + range) > 0 ||
		ray[4] * (x + range) - ray[3] * (y + range) - ray[2] > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain1(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain1(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain1(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain1(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain2(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[0] + qlo * ray[4] - ray[5] * y > 0 ||
		ray[1] - qlo * ray[3] + ray[5] * (x + range) > 0 ||
		ray[2] + ray[3] * (y + range) - ray[4] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qhi * ray[4] > 0 ||
		qhi * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[4] * x - ray[3] * y - ray[2] > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain2(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain2(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain2(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain2(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain3(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (qlo * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[0] + qlo * ray[4] - ray[5] * y > 0 ||
		ray[2] - ray[4] * (x + range) + ray[3] * y > 0 ||
		ray[1] - qhi * ray[3] + ray[5] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qhi * ray[4] > 0 ||
		ray[4] * x - ray[3] * (y + range) - ray[2] > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain3(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain3(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain3(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain3(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain4(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[0] + qhi * ray[4] - ray[5] * (y + range) > 0 ||
		-ray[1] + qhi * ray[3] - ray[5] * (x + range) > 0 ||
		-ray[2] + ray[4] * (x + range) - ray[3] * y > 0 ||
		-ray[0] - qlo * ray[4] + ray[5] * y > 0 ||
		ray[1] - qlo * ray[3] + ray[5] * x > 0 ||
		ray[2] - ray[4] * x + ray[3] * (y + range) > 0)
		return 0;


	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain4(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain4(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain4(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain4(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain5(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[1] - qhi * ray[3] + ray[5] * x > 0 ||
		ray[0] + qhi * ray[4] - ray[5] * (y + range) > 0 ||
		-ray[2] + ray[4] * (x + range) - ray[3] * (y + range) > 0 ||
		-ray[1] + qlo * ray[3] - ray[5] * (x + range) > 0 ||
		-ray[0] - qlo * ray[4] + ray[5] * y > 0 ||
		ray[2] - ray[4] * x + ray[3] * y > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain5(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain5(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain5(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain5(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain6(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (-ray[1] + qhi * ray[3] - ray[5] * (x + range) > 0 ||
		-ray[0] - qhi * ray[4] + ray[5] * y > 0 ||
		-ray[2] + ray[4] * x - ray[3] * y > 0 ||
		ray[1] - qlo * ray[3] + ray[5] * x > 0 ||
		ray[0] + qlo * ray[4] - ray[5] * (y + range) > 0 ||
		ray[2] - ray[4] * (x + range) + ray[3] * (y + range) > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain6(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain6(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain6(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain6(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain7(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only)
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (-ray[0] - qhi * ray[4] + ray[5] * y > 0 ||
		ray[1] - qhi * ray[3] + ray[5] * x > 0 ||
		-ray[2] + ray[4] * x - ray[3] * (y + range) > 0 ||
		ray[0] + qlo * ray[4] - ray[5] * (y + range) > 0 ||
		-ray[1] + qlo * ray[3] - ray[5] * (x + range) > 0 ||
		ray[2] - ray[4] * (x + range) + ray[3] * y > 0)
		return 0;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret, nrm, positive_only))
			return p;
		else
			return 0;
	}

	// recurse
	range >>= 1;
	Node* n = (Node*)q;
	Patch* p = 0;
	if (n->quad[0])
	{
		Patch* h = HitTerrain7(n->quad[0], x, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain7(n->quad[1], x + range, y, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain7(n->quad[2], x, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain7(n->quad[3], x + range, y + range, range, ray, ret, nrm, positive_only);
		if (h)
			p = h;
	}

	return p;
}

double HitTerrain(Patch* p, double u, double v)
{
	if (u<0 || u>1 || v<0 || v>1 || !p)
		return -1;

	int u0 = (int)floor(u*HEIGHT_CELLS), u1;
	if (u0==HEIGHT_CELLS)
	{
		u0 = HEIGHT_CELLS-1;
		u1 = HEIGHT_CELLS;
		u = 1.0;
	}
	else
	{
		u1 = u0+1;
		u = u * HEIGHT_CELLS - u0;
	}

	int v0 = (int)floor(v*HEIGHT_CELLS), v1;
	if (v0==HEIGHT_CELLS)
	{
		v0 = HEIGHT_CELLS-1;
		v1 = HEIGHT_CELLS;
		v = 1.0;
	}
	else
	{
		v1 = v0+1;
		v = v * HEIGHT_CELLS - v0;
	}

	if (p->diag & (1<<(u0+v0*HEIGHT_CELLS)))
	{
		// diagonal is u0,v0 - u1,v1

		if (u+v<1)
		{
			// interp. u0,v0 u1,v0 u0,v1
			int h00 = p->height[v0][u0];
			int h10 = p->height[v1][u0];
			int h01 = p->height[v0][u1];
			return h00 + u*(h01 - h00) + v*(h10 - h00);
		}
		else
		{
			// interp. u1,v1 u0,v1 u1,v0
			int h11 = p->height[v1][u1];
			int h10 = p->height[v1][u0];
			int h01 = p->height[v0][u1];
			return h11 + (1-u)*(h10 - h11) + (1-v)*(h01 - h11);
		}
	}
	else
	{
		// diagonal is u0,v1 - u1,v0

		if (u-v>0)
		{
			// interp. u1,v0 u1,v1 u0,v0
			int h01 = p->height[v0][u1];
			int h11 = p->height[v1][u1];
			int h00 = p->height[v0][u0];
			return h01 + (1-u)*(h00 - h01) + v*(h11 - h01);
		}
		else
		{
			// interp. u0,v1 u0,v0 u1,v1
			int h10 = p->height[v1][u0];
			int h00 = p->height[v0][u0];
			int h11 = p->height[v1][u1];
			return h10 + u*(h11 - h10) + (1-v)*(h00 - h10);
		}
	}
	
	return -1;
}

Patch* HitTerrain(Terrain* t, double p[3], double v[3], double ret[3], double nrm[3], bool positive_only)
{
	if (!t || !t->root)
		return 0;

	// p should be projected to the BOTTOM plane!
	double ray[] =
	{
		p[1] * v[2] - p[2] * v[1],
		p[2] * v[0] - p[0] * v[2],
		p[0] * v[1] - p[1] * v[0],
		v[0], v[1], v[2],
		p[0], p[1], p[2], // used by triangle-ray intersection
		FLT_MAX
	};

	int sign_case = 0;

	if (v[0] >= 0)
		sign_case |= 1;
	if (v[1] >= 0)
		sign_case |= 2;
	if (v[2] >= 0)
		sign_case |= 4;

	// assert((sign_case & 4) == 0); // watching from the bottom? -> raytraced reflections?

	static Patch* (* const func_vect[])(QuadItem* q, int x, int y, int range, double ray[10], double ret[3], double nrm[3], bool positive_only) =
	{
		HitTerrain0, 
		HitTerrain1, 
		HitTerrain2, 
		HitTerrain3,
		HitTerrain4,
		HitTerrain5,
		HitTerrain6,
		HitTerrain7
	};

	hit_patch_tests = 0;
	triangle_intersections = 0;
	Patch* patch = func_vect[sign_case](t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, ray, ret, nrm, positive_only);
	return patch;
}

size_t TerrainDetach(Terrain* t, Patch* p, int* px, int* py)
{
	int x, y;
	GetTerrainPatch(t, p, &x, &y);

	if (px)
		*px = x;
	if (py)
		*py = y;

	int flags = p->flags;
	Node* n = p->parent;

	/////////////
	// free(p);
	t->patches--;
	p->parent = 0;
	/////////////

	if (!n)
	{
		t->level = -1;
		t->root = 0;
		return sizeof(Patch);
	}

	// leaf trim

	QuadItem* q = p;

	while (true)
	{
		int c = 0;
		for (int i = 0; i < 4; i++)
		{
			if (n->quad[i] == q)
				n->quad[i] = 0;
			else
				if (n->quad[i])
					c++;
		}

		if (!c)
		{
			q = n;
			n = n->parent;
			free((Node*)q);
			t->nodes--;
		}
		else
			break;
	}

	// root trim

	n = (Node*)t->root;
	while (true)
	{
		int c = 0;
		int j = -1;
		for (int i = 0; i < 4; i++)
		{
			if (n->quad[i])
			{
				j = i;
				c++;
			}
		}

		// lost ...
		assert(j >= 0);

		if (c > 1)
			break;

		t->level--;

		if (j & 1)
			t->x -= 1 << t->level;
		if (j & 2)
			t->y -= 1 << t->level;

		t->root = n->quad[j];
		t->root->parent = 0;
		free(n);
		t->nodes--;

		if (t->level)
			n = (Node*)t->root;
		else
			break;
	}

	Patch* np[8] =
	{
		flags & 0x01 ? GetTerrainPatch(t, x - 1, y - 1) : 0,
		flags & 0x02 ? GetTerrainPatch(t, x, y - 1) : 0,
		flags & 0x04 ? GetTerrainPatch(t, x + 1, y - 1) : 0,
		flags & 0x08 ? GetTerrainPatch(t, x + 1, y) : 0,
		flags & 0x10 ? GetTerrainPatch(t, x + 1, y + 1) : 0,
		flags & 0x20 ? GetTerrainPatch(t, x, y + 1) : 0,
		flags & 0x40 ? GetTerrainPatch(t, x - 1, y + 1) : 0,
		flags & 0x80 ? GetTerrainPatch(t, x - 1, y) : 0,
	};

	for (int i = 0; i < 8; i++)
	{
		if (np[i])
		{
			int j = (i + 4) & 7;
			np[i]->flags &= ~(1 << j);
		}
	}

	return sizeof(Patch);
}

size_t TerrainAttach(Terrain* t, Patch* p, int x, int y)
{
	if (!t->root)
	{
		t->x = -x;
		t->y = -y;
		t->level = 0;

		p->parent = 0;

		t->root = p;
		t->patches = 1;

		return sizeof(Patch);
	}

	x += t->x;
	y += t->y;

	// create parents such root encloses x,y

	int range = 1 << t->level;

	while (x < 0)
	{
		Node* n = (Node*)malloc(sizeof(Node));
		t->nodes++;

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
		t->nodes++;

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
		t->nodes++;

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
		t->nodes++;

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

	int lev = t->level;

	if (lev == 0)
	{
		// already exist
		return 0;
	}

	// create children from root to x,y

	Node* n = (Node*)t->root;
	while (n)
	{
		lev--;
		int i = ((x >> lev) & 1) | (((y >> lev) & 1) << 1);

		if (lev)
		{
			if (!(Node*)n->quad[i])
			{
				Node* c = (Node*)malloc(sizeof(Node));
				t->nodes++;

				c->parent = n;
				c->quad[0] = c->quad[1] = c->quad[2] = c->quad[3] = 0;
				n->quad[i] = c;
			}

			n = (Node*)n->quad[i];
		}
		else
		{
			if (n->quad[i])
			{
				// already exist
				return 0;
			}

			t->patches++;

			n->quad[i] = p;
			p->parent = n;

			UpdateNodes(p);

			return sizeof(Patch);
		}
	}

	assert(0); // should never reach here
	return 0;
}

size_t TerrainDispose(Patch* p)
{
	// MUST BE DETACHED!

#ifdef TEXHEAP
	TexAlloc* last = p->ta->Free();
	if (last)
	{
		Patch* l = (Patch*)last->user;
		UpdateTerrainVisualMap(l);
		UpdateTerrainHeightMap(l);
	}
#endif

	free(p);

	return sizeof(Patch);
}



struct FileHeader
{
	uint32_t file_sign;
	uint32_t header_size;
	uint32_t num_patches;
	uint32_t reserved;
};

struct FilePatch
{
	int32_t x,y; // 4
	uint16_t visual[VISUAL_CELLS][VISUAL_CELLS]; // 2x64
	uint16_t height[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1]; //2x25
	uint16_t diag; // 2
};

void SaveTree(FILE* f, int x, int y, int lev, const QuadItem* item)
{
	if (!lev)
	{
		const Patch* p = (const Patch*)item;
		fwrite(&x,1,sizeof(int32_t),f);
		fwrite(&y,1,sizeof(int32_t),f);
		fwrite(p->visual,VISUAL_CELLS*VISUAL_CELLS,sizeof(uint16_t),f);
		fwrite(p->height,(HEIGHT_CELLS+1)*(HEIGHT_CELLS+1),sizeof(uint16_t),f);
		fwrite(&p->diag,1,sizeof(int16_t),f);
		return;
	}

	const Node* n = (const Node*)item;
	lev--;

	int r = 1<<lev;

	if (n->quad[0])
		SaveTree(f,x,y,lev,n->quad[0]);
	if (n->quad[1])
		SaveTree(f,x+r,y,lev,n->quad[1]);
	if (n->quad[2])
		SaveTree(f,x,y+r,lev,n->quad[2]);
	if (n->quad[3])
		SaveTree(f,x+r,y+r,lev,n->quad[3]);
}

bool SaveTerrain(const Terrain* t, FILE* f)
{
	if (!t || !f)
		return false;

	FileHeader hdr =
	{
		*(uint32_t*)"AS3D",
		(uint32_t)sizeof(FileHeader),
		(uint32_t)t->patches,
		(uint32_t)0
	};

	fwrite(&hdr,1,sizeof(FileHeader),f);

	if (t->root)
		SaveTree(f, -t->x, -t->y, t->level,t->root);

	return true;
}

Terrain* LoadTerrain(FILE* f)
{
	if (!f)
		return 0;

	FileHeader hdr;
	if (fread(&hdr,1,sizeof(FileHeader),f)!=sizeof(FileHeader))
	{
		return 0;
	}

	if (hdr.file_sign != *(uint32_t*)"AS3D" ||
		hdr.header_size != sizeof(FileHeader))
	{
		return 0;
	}

	Terrain* t = CreateTerrain();
	for (unsigned i = 0; i < hdr.num_patches; i++)
	{
		FilePatch pch;
		if (fread(&pch,1,sizeof(FilePatch),f)!=sizeof(FilePatch))
		{
			DeleteTerrain(t);
			return 0;
		}

		Patch* p = AddTerrainPatch(t, pch.x, pch.y, 0);

		memcpy(p->visual,pch.visual,sizeof(uint16_t)*VISUAL_CELLS*VISUAL_CELLS);
		memcpy(p->height,pch.height,sizeof(uint16_t)*(HEIGHT_CELLS+1)*(HEIGHT_CELLS+1));

		UpdateTerrainVisualMap(p);
		UpdateTerrainHeightMap(p);

		p->diag = pch.diag;
	}

	return t;
}
