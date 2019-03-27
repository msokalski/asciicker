
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "texheap.h"
#include "terrain.h"
#include "matrix.h"

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
	uint16_t flags; // 8 bits of neighbors, bit0 is on (-,-), bit7 is on (-,0)
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

Terrain* CreateTerrain(int z)
{
	Terrain* t = (Terrain*)malloc(sizeof(Terrain));
	t->x = 0;
	t->y = 0;
	t->nodes = 0;

#ifdef TEXHEAP
	int cap = 1024 / (HEIGHT_CELLS + 1);
	t->th.Create(cap,cap, HEIGHT_CELLS+1, HEIGHT_CELLS+1, GL_R16UI, sizeof(TexPageBuffer));
#endif

	if (z >= 0)
	{
		t->level = 0;

		Patch* p = (Patch*)malloc(sizeof(Patch));
		p->parent = 0;
		p->lo = z;
		p->hi = z;
		p->flags = 0; // (no neighbor)

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;

		t->root = p;
		t->patches = 1;

#ifdef TEXHEAP
		p->ta = t->th.Alloc(GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height);
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
	if (z < 0)
		return 0;

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

		for (int y = 0; y <= HEIGHT_CELLS; y++)
			for (int x = 0; x <= HEIGHT_CELLS; x++)
				p->height[y][x] = z;

		t->root = p;
		t->patches = 1;

#ifdef TEXHEAP
		p->ta = t->th.Alloc(GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height);
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

			UpdateNodes(p);

#ifdef TEXHEAP
			p->ta = t->th.Alloc(GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height);
#endif

			return p;
		}
	}

	assert(0); // should never reach here
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

void GetTerrainLimits(Patch* p, uint16_t* lo, uint16_t* hi)
{
	if (lo)
		*lo = p->lo;
	if (hi)
		*hi = p->hi;
}


void UpdateTerrainPatch(Patch* p)
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

#ifdef TEXHEAP
	p->ta->Update(GL_RED_INTEGER, GL_UNSIGNED_SHORT, p->height); 
#endif

	UpdateNodes(p);
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

static __forceinline void QueryTerrain(QuadItem* q, int x, int y, int range, int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
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

static void __forceinline QueryTerrain(QuadItem* q, int x, int y, int range, int planes, double* plane[], int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie)
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
		double* pp[4] = { plane[0],plane[1],plane[2],plane[3] };
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
static inline bool RayIntersectsTriangle(double ray[6], double v0[3], double v1[3], double v2[3], double ret[3])
{
	triangle_intersections++;

	const double EPSILON = 0.0000001;

	double edge1[3] = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
	double edge2[3] = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };

	double h[3] = // rayVector.crossProduct(edge2);
	{
		ray[4] * edge2[2] - ray[5] * edge2[1],
		ray[5] * edge2[0] - ray[3] * edge2[2],
		ray[3] * edge2[1] - ray[4] * edge2[0]
	};
	
	double a = // edge1.dotProduct(h);
		edge1[0] * h[0] + edge1[1] * h[1] + edge1[2] * h[2];

	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this vangle.

	double f = 1.0 / a;
	double s[3] = // rayOrigin - vertex0;
	{
		ray[6] - v0[0],
		ray[7] - v0[1],
		ray[8] - v0[2],
	};

	double u = // f * (s.dotProduct(h));
		f * (s[0] * h[0] + s[1] * h[1] + s[2] * h[2]);

	if (u < 0.0 || u > 1.0)
		return false;

	double q[3] = //s.crossProduct(edge1);
	{
		s[1] * edge1[2] - s[2] * edge1[1],
		s[2] * edge1[0] - s[0] * edge1[2],
		s[0] * edge1[1] - s[1] * edge1[0]
	};

	double v = // f * rayVector.dotProduct(q);
		f * (ray[3] * q[0] + ray[4] * q[1] + ray[5] * q[2]);

	if (v < 0.0 || u + v > 1.0)
		return false;

	// At this stage we can compute t to find out where the intersection point is on the line.

	double t = //f * edge2.dotProduct(q);
		f * (edge2[0] * q[0] + edge2[1] * q[1] + edge2[2] * q[2]);

	// POSITIVE DIR OF RAY ONLY?
	// if (t < EPSILON)
	//	  return false;

	ret[0] = ray[6] + ray[3] * t;
	ret[1] = ray[7] + ray[4] * t;
	ret[2] = ray[8] + ray[5] * t;
	return true;
}

int hit_patch_tests = 0;
bool HitPatch(Patch* p, int x, int y, double ray[6], double ret[3])
{
	hit_patch_tests ++;
	static const double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;
	bool hit = false;

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

			double r[3];

			if (RayIntersectsTriangle(ray, v[2], v[0], v[1], r) &&  r[2] > ret[2])
			{
				hit |= 1;
				ret[0] = r[0];
				ret[1] = r[1];
				ret[2] = r[2];
			}

			if (RayIntersectsTriangle(ray, v[2], v[1], v[3], r) && r[2] > ret[2])
			{
				hit |= 1;
				ret[0] = r[0];
				ret[1] = r[1];
				ret[2] = r[2];
			}
		}
	}

	return hit;
}

Patch* HitTerrain0(QuadItem* q, int x, int y, int range, double ray[6], double ret[3])
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[1] - qlo * ray[3] + ray[5] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qlo * ray[4] > 0 ||
		ray[2] - ray[4] * x + ray[3] * (y + range) > 0 ||
		qhi * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[0] + qhi * ray[4] - ray[5] * y > 0 ||
		ray[4] * (x + range) - ray[3] * y - ray[2] > 0)
		return false;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret))
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
		Patch* h = HitTerrain0(n->quad[0], x, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain0(n->quad[1], x+range, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain0(n->quad[2], x, y+range, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain0(n->quad[3], x+range, y+range, range, ray, ret);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain1(QuadItem* q, int x, int y, int range, double ray[6], double ret[3])
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[5] * (y + range) - ray[0] - qlo * ray[4] > 0 ||
		qlo * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[2] - ray[4] * x + ray[3] * y > 0 ||
		ray[0] + qhi * ray[4] - ray[5] * y > 0 ||
		ray[1] - qhi * ray[3] + ray[5] * (x + range) > 0 ||
		ray[4] * (x + range) - ray[3] * (y + range) - ray[2] > 0)
		return false;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret))
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
		Patch* h = HitTerrain1(n->quad[0], x, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain1(n->quad[1], x + range, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain1(n->quad[2], x, y + range, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain1(n->quad[3], x + range, y + range, range, ray, ret);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain2(QuadItem* q, int x, int y, int range, double ray[6], double ret[3])
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (ray[0] + qlo * ray[4] - ray[5] * y > 0 ||
		ray[1] - qlo * ray[3] + ray[5] * (x + range) > 0 ||
		ray[2] + ray[3] * (y + range) - ray[4] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qhi * ray[4] > 0 ||
		qhi * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[4] * x - ray[3] * y - ray[2] > 0)
		return false;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret))
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
		Patch* h = HitTerrain2(n->quad[0], x, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain2(n->quad[1], x + range, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain2(n->quad[2], x, y + range, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain2(n->quad[3], x + range, y + range, range, ray, ret);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain3(QuadItem* q, int x, int y, int range, double ray[6], double ret[3])
{
	int qlo = q->lo;
	int qhi = q->hi;

	if (qlo * ray[3] - ray[5] * x - ray[1] > 0 ||
		ray[0] + qlo * ray[4] - ray[5] * y > 0 ||
		ray[2] - ray[4] * (x + range) + ray[3] * y > 0 ||
		ray[1] - qhi * ray[3] + ray[5] * (x + range) > 0 ||
		ray[5] * (y + range) - ray[0] - qhi * ray[4] > 0 ||
		ray[4] * x - ray[3] * (y + range) - ray[2] > 0)
		return false;

	if (range == VISUAL_CELLS)
	{
		Patch* p = (Patch*)q;
		if (HitPatch(p, x, y, ray, ret))
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
		Patch* h = HitTerrain3(n->quad[0], x, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[1])
	{
		Patch* h = HitTerrain3(n->quad[1], x + range, y, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[2])
	{
		Patch* h = HitTerrain3(n->quad[2], x, y + range, range, ray, ret);
		if (h)
			p = h;
	}
	if (n->quad[3])
	{
		Patch* h = HitTerrain3(n->quad[3], x + range, y + range, range, ray, ret);
		if (h)
			p = h;
	}

	return p;
}

Patch* HitTerrain(Terrain* t, double p[3], double v[3], double ret[3])
{
	// p should be projected to the BOTTOM plane!
	double ray[] =
	{
		p[1] * v[2] - p[2] * v[1],
		p[2] * v[0] - p[0] * v[2],
		p[0] * v[1] - p[1] * v[0],
		v[0], v[1], v[2],
		p[0], p[1], p[2] // used by triangle-ray intersection
	};

	int sign_case = 0;

	if (v[0] >= 0)
		sign_case |= 1;
	if (v[1] >= 0)
		sign_case |= 2;
	if (v[2] >= 0)
		sign_case |= 4;

	assert((sign_case & 4) == 0); // watching from the bottom? -> raytraced reflections?

	static Patch* (* const func_vect[])(QuadItem* q, int x, int y, int range, double ray[6], double ret[3]) =
	{
		HitTerrain0, 
		HitTerrain1, 
		HitTerrain2, 
		HitTerrain3
	};

	ret[0] = p[0];
	ret[1] = p[1];
	ret[2] = p[2];

	hit_patch_tests = 0;
	triangle_intersections = 0;
	Patch* patch = func_vect[sign_case](t->root, -t->x*VISUAL_CELLS, -t->y*VISUAL_CELLS, VISUAL_CELLS << t->level, ray, ret);
	return patch;
}

