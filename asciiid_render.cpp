
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
	uint16_t visual;
	uint16_t height;
	uint8_t spare;   // refl, patch xy parity etc...
	uint8_t diffuse; // just in case

	inline bool DepthTest(int z)
	{
		if (height >= z)
			return false;
		height = z;
		return true;
	}
};


struct SampleBuffer
{
	int w, h; // make 2x +2 bigger than terminal buffer
	Sample* ptr;
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

// we could easily make it template of <Sample,Shader>
void Renderer::RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/)
{
	struct Shader
	{
		void Fill(Sample* s, int bc[3]) const
		{
			if (s->height > water)
			{
				int u = (uv[0] * bc[0] + uv[2] * bc[1] + uv[4] * bc[2] + 0x4000) >> 15;
				int v = (uv[1] * bc[0] + uv[3] * bc[1] + uv[5] * bc[2] + 0x4000) >> 15;

				s->visual = map[v * VISUAL_CELLS + u];
				s->diffuse = diffuse;
				s->spare |= parity;
			}
			else
			{
				s->spare = 3;
			}
		}

		int* uv; // points to array of 6 ints (u0,v0,u1,v1,u2,v2) each is equal to 0 or VISUAL_CELLS
		uint16_t* map; // points to array of VISUAL_CELLS x VISUAL_CELLS ushorts
		uint16_t water;
		uint8_t diffuse; // shading experiment
		uint8_t parity;
	} shader;

	// 2 parity bits for drawing lines around patches
	// 0 - no patch rendered here
	// 1 - odd
	// 2 - even
	// 3 - under water
	shader.parity = ((x^y) & 1) + 1; 

	Renderer* r = (Renderer*)cookie;

	int w = r->sample_buffer.w;
	int h = r->sample_buffer.h;
	Sample* ptr = r->sample_buffer.ptr;

	uint16_t* hmap = GetTerrainHeightMap(p);

	// transform patch verts xy+dx+dy, together with hmap into this array
	int xyzf[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1][4];

	// uvs are same for all patches, could be set in the renderer once!
	// move to renderer:
	const static float uv[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1] = { 0 };

	for (int dy = 0; dy <= HEIGHT_CELLS; dy++)
	{
		for (int dx = 0; dx <= HEIGHT_CELLS; dx++)
		{
			// transform, round xyz 
			// ...
			xyzf[dy][dx][0] = 0;
			xyzf[dy][dx][1] = 0;
			xyzf[dy][dx][2] = 0; // height directly from hmap

			// check if / which screen edges cull it
			xyzf[dy][dx][2] = 0;

			// accum patch transformed xy bbox
			// ...
		}
	}

	uint16_t* vmap = GetTerrainVisualMap(p);
	uint16_t  diag = GetTerrainDiag(p);
	for (int dy = 0; dy < HEIGHT_CELLS; dy++)
	{
		for (int dx = 0; dx < HEIGHT_CELLS; dx++,diag>>=1)
		{
			if (diag & 1)
			{
				// .
				// |\
				// |_\
				// '  '
				// lower triangle
				static int lo_uv[] = {0,0, 0,0, 0,0};
				const int* lo[3] = { xyzf[dy][dx], xyzf[dy][dx + 1], xyzf[dy + 1][dx] };
				shader.uv = lo_uv;
				Rasterize(ptr, w, h, &shader, lo);

				// .__.
				//  \ |
				//   \|
				//    '
				// upper triangle
				static int up_uv[] = { 0,0, 0,0, 0,0 };
				const int* up[3] = { xyzf[dy + 1][dx + 1], xyzf[dy + 1][dx], xyzf[dy][dx + 1] };
				shader.uv = up_uv;
				Rasterize(ptr, w, h, &shader, up);
			}
			else
			{
				// lower triangle
				//    .
				//   /|
				//  /_|
				// '  '
				static int lo_uv[] = { 0,0, 0,0, 0,0 };
				const int* lo[3] = { xyzf[dy][dx + 1], xyzf[dy + 1][dx + 1], xyzf[dy][dx] };
				shader.uv = lo_uv;
				Rasterize(ptr, w, h, &shader, lo);

				// upper triangle
				// .__.
				// | / 
				// |/  
				// '
				static int up_uv[] = { 0,0, 0,0, 0,0 };
				const int* up[3] = { xyzf[dy + 1][dx], xyzf[dy][dx], xyzf[dy + 1][dx + 1] };
				shader.uv = up_uv;
				Rasterize(ptr, w, h, &shader, up);
			}
		}
	}
}


/*
Conversion from SampleBuffer to AnsiBuffer:
// sample buffer for terminal 160x90 has 324x184 samples = 476928 bytes!
// when converted to ansi buffer it is reduced to just 57600 bytes
*/

#if 0
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

	int w[4];
	w[3] = edgeFunction2(xyz[0], xyz[1], xyz[2]); // area (divisor)

	for (int y = bottom; y < top; y++)
	{
		for (int x = left; x < right; x++)
		{
			int p[2] = { x,y };

			w[0] = edgeFunction2(xyz[1], xyz[2], p);
			w[1] = edgeFunction2(xyz[2], xyz[0], p);
			w[2] = edgeFunction2(xyz[0], xyz[1], p);

			if (w[0] >= 0 && w[1] >= 0 && w[2] >= 0)
			{
				SampleCell* c = sample_buffer.ptr + (y>>1)*sample_buffer.w + (x>>1);
				Sample* s = &((*c)[((y&1)<<1)|(x&1)]);
				shader->Fill(s,w);
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
#endif