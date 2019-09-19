
// this is CPU scene renderer into ANSI framebuffer
#include "asciiid_render.h"

#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include "terrain.h"
#include "mesh.h"

#include <algorithm> // std::min/max

// #include "sprite.h"

struct Sample
{
	AnsiCell cell;
	float height;
};

typedef Sample SampleCell[4]; // {UL,UR,LL,LR}

struct SampleBuffer
{
	int w, h;
	SampleCell* ptr;
};

struct Renderer
{
	SampleBuffer sample_buffer; // render surface

	uint8_t* buffer;
	int buffer_size; // ansi_buffer allocation size in cells (minimize reallocs)

	// current angles & position!
	// ....

	// derived transform and clipping planes
	// ....

	void Clear();
	void PostFx();
	void Ansify();

	// it has its own Shaders for fill & stroke
	// but requires here some extra state
	// ....
	static void RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/);

	template <typename Shader>
	void LineStroke(Shader* shader, int xyz[2][3]);

	template <typename Shader>
	void TriangleFill(Shader* shader, int xyz[3][3], float uv[3][2]);
};

void Renderer::RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/)
{
	Renderer* r = (Renderer*)cookie;

	uint16_t* hmap = GetTerrainHeightMap(p);

	// transform patch verts xy+dx+dy, together with hmap into this array
	int xyz[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1][3];

	// uvs are same for all patches, could be set in the renderer once!
	// move to renderer:
	const static float uv[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1] = { 0 };


	// if every vertex would also have 4 bits indicating outside Nth edge
	// ANDing such 3 vertices flags together would clearly indicate no need to paint
	uint8_t flags[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1] = { 0 };

	for (int dy = 0; dy <= HEIGHT_CELLS; dy++)
	{
		for (int dx = 0; dx <= HEIGHT_CELLS; dx++)
		{
			// transform, round, check if/which screen edges culls
			// ...
			flags[dy][dx] = 0;
			xyz[dy][dx][0] = 0;
			xyz[dy][dx][1] = 0;
			xyz[dy][dx][2] = 0;
		}
	}

	uint16_t* vmap = GetTerrainVisualMap(p);
	uint16_t  diag = GetTerrainDiag(p);
	int mask = 1;
	for (int dy = 0; dy <= HEIGHT_CELLS; dy++)
	{
		for (int dx = 0; dx <= HEIGHT_CELLS; dx++,mask<<=1)
		{
			if (diag & mask)
			{
				// todo: add some helper indices table    __
				// {dx, dy} {dx + 1, dy} {dx, dy + 1} // |\ |
				// {dx+1, dy+1} {dx, dy+1} {dx+1, dy} // |_\|

			
				if ((flags[][] & flags[][] & flags[][]) == 0)
				{
					// .
					// |\
					// |_\
					// '  '
				}

				if ((flags[][] & flags[][] & flags[][]) == 0)
				{
					// .__.
					//  \ |
					//   \|
					//    '
				}
			}
			else
			{
				// todo: add some helper indices table        __
				// {dx + 1, dy} {dx + 1, dy + 1} {dx, dy} // | /|
				// {dx, dy + 1} {dx, dy} {dx + 1, dy + 1} // |/_|

				if ((flags[][] & flags[][] & flags[][]) == 0)
				{
					//    .
					//   /|
					//  /_|
					// '  '
				}

				if ((flags[][] & flags[][] & flags[][]) == 0)
				{
					// .__.
					// | / 
					// |/  
					// '
				}
			}
		}
	}
}


/*
Conversion from SampleBuffer to AnsiBuffer:
// sample buffer for terminal 160x90 has 324x184 samples = 476928 bytes!
// when converted to ansi buffer it is reduced to just 57600 bytes
*/

template <typename Shader>
void Renderer::PatchFill(Shader* shader, int xyz[HEIGHT_CELLS+1][3])
{
	// optimized such 
}

template <typename Shader>
void Renderer::TriangleFill(Shader* shader, int xyz[3][3], float uv[3][2])
{
	// samples are in cente
	#define edgeFunction2(a,b,c) ((c)[0]*2+1 - (a)[0]*2) * ((b)[1] - (a)[1]) - ((c)[1]*2+1 - (a)[1]*2) * ((b)[0] - (a)[0])
	//#define edgeFunction1(a,b,c) ((c)[0] - (a)[0]) * ((b)[1] - (a)[1]) - ((c)[1] - (a)[1]) * ((b)[0] - (a)[0])

	// triangle bbox
	int left   = xyz[0][0] < xyz[1][0] ? (xyz[0][0] < xyz[2][0] ? xyz[0][0] : xyz[2][0]) : (xyz[1][0] < xyz[2][0] ? xyz[1][0] : xyz[2][0]);
	int right  = xyz[0][0] > xyz[1][0] ? (xyz[0][0] > xyz[2][0] ? xyz[0][0] : xyz[2][0]) : (xyz[1][0] > xyz[2][0] ? xyz[1][0] : xyz[2][0]);
	int bottom = xyz[0][1] < xyz[1][1] ? (xyz[0][1] < xyz[2][1] ? xyz[0][1] : xyz[2][1]) : (xyz[1][1] < xyz[2][1] ? xyz[1][1] : xyz[2][1]);
	int top    = xyz[0][1] > xyz[1][1] ? (xyz[0][1] > xyz[2][1] ? xyz[0][1] : xyz[2][1]) : (xyz[1][1] > xyz[2][1] ? xyz[1][1] : xyz[2][1]);

	// intersect with render target
	left = std::max(0, left);
	right = std::min(sample_buffer.w, right);
	bottom = std::max(0, bottom);
	top = std::min(sample_buffer.h, top);

	int area = edgeFunction2(xyz[0], xyz[1], xyz[2]);

	for (int y = bottom; y < top; y++)
	{
		for (int x = left; x < right; x++)
		{
			int p[2] = { x,y };

			int w0 = edgeFunction2(xyz[1], xyz[2], p);
			int w1 = edgeFunction2(xyz[2], xyz[0], p);
			int w2 = edgeFunction2(xyz[0], xyz[1], p);

			if (w0 >= 0 && w1 >= 0 && w2 >= 0)
			{
				SampleCell* c = sample_buffer.ptr + (y>>1)*sample_buffer.w + (x>>1);
				Sample* s = &((*c)[((y&1)<<1)|(x&1)]);

				// interpolate 
				float uvz[3] =
				{
					 uv[0][0] * w0 +  uv[1][0] * w1 +  uv[2][0] * w2,
					 uv[0][1] * w0 +  uv[1][1] * w1 +  uv[2][1] * w2,
					(float)(xyz[0][2] * w0 + xyz[1][2] * w1 + xyz[2][2] * w2)
				};

				shader->Fill(s, uvz, area);
			}
		}
	}
}

bool Render(Terrain* t, World* w, float angle, int cx, int cy, int w, int h, int pitch, AnsiCell* ptr)
{

	Renderer r;

	double planes[4][4]; // 4 clip planes

	int view_flags = 0xAA; // should contain only bits that face viewing direction

	QueryTerrain(t, 4, planes, view_flags, r.RenderPatch, &r);

	
	QueryTerrain(t,)




		// all in this call!
		// -----------------
	/*
	struct Shader_A
	{
		void Fill(Sample* s, float uvz[3], int area)
		{
			printf(".");
		}

		// current patch visual map
		const uint16_t* map;
	} a;

	Patch* p = 0; // patch to be rendered
	a.map = GetTerrainVisualMap(p);
	*/
	int xyz[3][3];
	float uv[3][2];
	r.TriangleFill(&a, xyz, uv);

	return false;
}