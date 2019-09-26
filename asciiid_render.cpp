
// this is CPU scene renderer into ANSI framebuffer
#include "asciiid_render.h"

#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include "terrain.h"
#include "mesh.h"
#include "matrix.h"
#include "fast_rand.h"
// #include "sprite.h"

#define _USE_MATH_DEFINES
#include <math.h>

template <typename Sample, typename Shader>
inline void Rasterize(Sample* buf, int w, int h, Shader* s, const int* v[3])
{
	// each v[i] must point to 4 ints: {x,y,z,f} where f should indicate culling bits (can be 0)
	// shader must implement: bool Shader::Fill(Sample* s, int bc[3])
	// where bc contains 3 barycentric weights which are normalized to 0x8000 (use '>>15' after averaging)
	// Sample must implement bool DepthTest(int z, int divisor);
	// it must return true if z/divisor passes depth test on this sample
	// if test passes, it should write new z/d to sample's depth (if something like depth write mask is enabled)

	// produces samples between buffer cells 
	#define BC_A(a,b,c) (2*(((b)[0] - (a)[0]) * ((c)[1] - (a)[1]) - ((b)[1] - (a)[1]) * ((c)[0] - (a)[0])))

	// produces samples at centers of buffer cells 
	#define BC_P(a,b,c) (((b)[0] - (a)[0]) * (2*(c)[1]+1 - 2*(a)[1]) - ((b)[1] - (a)[1]) * (2*(c)[0]+1 - 2*(a)[0]))

	if ((v[0][3] & v[1][3] & v[2][3]) == 0)
	{
		int area = BC_A(v[0],v[1],v[2]);
		if (area > 0)
		{
			assert(area < 0x10000);
			float normalizer = (1.0f - FLT_EPSILON) / area;

			// canvas intersection with triangle bbox
			int left = std::max(0, std::min(v[0][0], std::min(v[1][0], v[2][0])));
			int right = std::min(w, std::max(v[0][0], std::max(v[1][0], v[2][0])));
			int bottom = std::max(0, std::min(v[0][1], std::min(v[1][1], v[2][1])));
			int top = std::min(h, std::max(v[0][1], std::max(v[1][1], v[2][1])));

			Sample* col = buf + bottom * w + left;
			for (int y = bottom; y < top; y++, col+=w)
			{
				Sample* row = col;
				for (int x = left; x < right; x++, row++)
				{
					int p[2] = { x,y };

					int bc[3] =
					{
						BC_P(v[1], v[2], p),
						BC_P(v[2], v[0], p),
						BC_P(v[0], v[1], p)
					};

					// outside
					if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
						continue;

					// edge pairing
					if (bc[0] == 0 && v[1][0] <= v[2][0] ||
						bc[1] == 0 && v[2][0] <= v[0][0] ||
						bc[2] == 0 && v[0][0] <= v[1][0])
					{
						continue;
					}

					assert(bc[0] + bc[1] + bc[2] == area);

					float nbc[3] =
					{
						bc[0] * normalizer,
						bc[1] * normalizer,
						bc[2] * normalizer
					};

					float z = nbc[0] * v[0][2] + nbc[1] * v[1][2] + nbc[2] * v[2][2];

					if (row->DepthTest(z))
						s->Fill(row, nbc);
				}
			}
		}
	}
	#undef BC
}



struct Sample
{
	uint16_t visual;
	uint8_t spare;   // refl, patch xy parity etc...
	uint8_t diffuse; // just in case
	float height;

	inline bool DepthTest(float z)
	{
		if (height > z)
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

	// transform
	double mul[6]; // 3x2 rot part
	int add[2]; // post rotated and rounded translation
	float water;
};

// we could easily make it template of <Sample,Shader>
void Renderer::RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/)
{
	struct Shader
	{
		void Fill(Sample* s, float bc[3]) const
		{
			if (s->height >= water)
			{
				int u = (int)floor(uv[0] * bc[0] + uv[2] * bc[1] + uv[4] * bc[2]);
				int v = (int)floor(uv[1] * bc[0] + uv[3] * bc[1] + uv[5] * bc[2]);

				if (u >= VISUAL_CELLS || v >= VISUAL_CELLS)
				{
					// detect overflow
					s->visual = 2;
				}
				else
				{
					s->visual = map[v * VISUAL_CELLS + u];
					s->diffuse = diffuse;
					s->spare |= parity;
				}
			}
			else
				s->spare = 3;
		}

		int* uv; // points to array of 6 ints (u0,v0,u1,v1,u2,v2) each is equal to 0 or VISUAL_CELLS
		uint16_t* map; // points to array of VISUAL_CELLS x VISUAL_CELLS ushorts
		float water;
		uint8_t diffuse; // shading experiment
		uint8_t parity;
	} shader;

	Renderer* r = (Renderer*)cookie;

	double* mul = r->mul;
	int* add = r->add;

	int w = r->sample_buffer.w;
	int h = r->sample_buffer.h;
	Sample* ptr = r->sample_buffer.ptr;

	uint16_t* hmap = GetTerrainHeightMap(p);
	
	uint16_t* hm = hmap;

	// transform patch verts xy+dx+dy, together with hmap into this array
	int xyzf[HEIGHT_CELLS + 1][HEIGHT_CELLS + 1][4];

	for (int dy = 0; dy <= HEIGHT_CELLS; dy++)
	{
		int vy = y * HEIGHT_CELLS + dy * VISUAL_CELLS;

		for (int dx = 0; dx <= HEIGHT_CELLS; dx++)
		{
			int vx = x * HEIGHT_CELLS + dx * VISUAL_CELLS;
			int vz = *(hm++);

			// transform 
			int tx = (int)floor(mul[0] * vx + mul[2] * vy + 0.5) + add[0];
			int ty = (int)floor(mul[1] * vx + mul[3] * vy + mul[5] * vz + 0.5) + add[1];

			xyzf[dy][dx][0] = tx;
			xyzf[dy][dx][1] = ty;
			xyzf[dy][dx][2] = vz;

			// todo: if patch is known to fully fit in screen, set f=0 
			// otherwise we need to check if / which screen edges cull each vertex
			xyzf[dy][dx][3] = (tx<0) | ((tx>w)<<1) | ((ty<0)<<2) | ((ty>h)<<3);
		}
	}

	uint16_t  diag = GetTerrainDiag(p);

	// 2 parity bits for drawing lines around patches
	// 0 - no patch rendered here
	// 1 - odd
	// 2 - even
	// 3 - under water
	shader.parity = (((x^y)/VISUAL_CELLS) & 1) + 1; 
	shader.water = r->water;
	shader.map = GetTerrainVisualMap(p);

//	if (shader.parity == 1)
//		return;

	hm = hmap;

	for (int dy = 0; dy < HEIGHT_CELLS; dy++, hm++)
	{
		for (int dx = 0; dx < HEIGHT_CELLS; dx++,diag>>=1, hm++)
		{
			if (diag & 1)
			{
				// .
				// |\
				// |_\
				// '  '
				// lower triangle

				// terrain should keep diffuse map with timestamp of light modification it was updated to
				// then if current light timestamp is different than in terrain we need to update diffuse (into terrain)
				// now we should simply use diffuse from terrain
				// note: if terrain is being modified, we should clear its timestamp or immediately update diffuse


				static int lo_uv[] = {0,0, VISUAL_CELLS,0, 0,VISUAL_CELLS};
				const int* lo[3] = { xyzf[dy][dx], xyzf[dy][dx + 1], xyzf[dy + 1][dx] };
				shader.uv = lo_uv;
				shader.diffuse = 0xff;
				Rasterize(ptr, w, h, &shader, lo);

				// .__.
				//  \ |
				//   \|
				//    '
				// upper triangle
				static int up_uv[] = { VISUAL_CELLS,VISUAL_CELLS, 0,VISUAL_CELLS, VISUAL_CELLS,0 };
				const int* up[3] = { xyzf[dy + 1][dx + 1], xyzf[dy + 1][dx], xyzf[dy][dx + 1] };
				shader.uv = up_uv;
				shader.diffuse = 0xff;
				Rasterize(ptr, w, h, &shader, up);
			}
			else
			{
				// lower triangle
				//    .
				//   /|
				//  /_|
				// '  '
				static int lo_uv[] = { VISUAL_CELLS,0, VISUAL_CELLS,VISUAL_CELLS, 0,0 };
				const int* lo[3] = { xyzf[dy][dx + 1], xyzf[dy + 1][dx + 1], xyzf[dy][dx] };
				shader.uv = lo_uv;
				shader.diffuse = 0xff;
				Rasterize(ptr, w, h, &shader, lo);

				// upper triangle
				// .__.
				// | / 
				// |/  
				// '
				static int up_uv[] = { 0,VISUAL_CELLS, 0,0, VISUAL_CELLS,VISUAL_CELLS };
				const int* up[3] = { xyzf[dy + 1][dx], xyzf[dy][dx], xyzf[dy + 1][dx + 1] };
				shader.uv = up_uv;
				shader.diffuse = 0xff;
				Rasterize(ptr, w, h, &shader, up);
			}
		}
	}
}

bool Render(Terrain* t, World* w, float water, float zoom, float yaw, float pos[3], int width, int height, AnsiCell* ptr)
{
	Renderer r;

	float scale = 3.0;

	zoom *= scale;

	/*
	int dw = 4+2*width;
	int dh = 4+2*height;
	*/
	int dw = 1 + width + 1;
	int dh = 1 + height + 1;

	float ds = 2*zoom / VISUAL_CELLS;

	r.sample_buffer.w = dw;
	r.sample_buffer.h = dh;
	r.sample_buffer.ptr = (Sample*)malloc(dw*dh*sizeof(Sample));
	r.water = water;

	// clear (at least depth!)
	memset(r.sample_buffer.ptr, 0x00, dw*dh * sizeof(Sample));

	static const double sin30 = sin(M_PI*30.0/180.0); 
	static const double cos30 = cos(M_PI*30.0/180.0);

	double a = yaw * M_PI / 180.0;
	double sinyaw = sin(a);
	double cosyaw = cos(a);

	double tm[16];
	tm[0] = +cosyaw *ds;
	tm[1] = -sinyaw * sin30*ds;
	tm[2] = 0;
	tm[3] = 0;
	tm[4] = +sinyaw * ds;
	tm[5] = +cosyaw * sin30*ds;
	tm[6] = 0;
	tm[7] = 0;
	tm[8] = 0;
	tm[9] = +cos30/HEIGHT_SCALE*ds*HEIGHT_CELLS;
	tm[10] = 1.0; //+2./0xffff;
	tm[11] = 0;
	tm[12] = dw*0.5 - (pos[0] * tm[0] + pos[1] * tm[4] + pos[2] * tm[8]) * HEIGHT_CELLS;
	tm[13] = dh*0.5 - (pos[0] * tm[1] + pos[1] * tm[5] + pos[2] * tm[9]) * HEIGHT_CELLS;
	tm[14] = 0.0; //-1.0;
	tm[15] = 1.0;

	r.mul[0] = tm[0];
	r.mul[1] = tm[1];
	r.mul[2] = tm[4];
	r.mul[3] = tm[5];
	r.mul[4] = 0;
	r.mul[5] = tm[9];
	r.add[0] = (int)floor(tm[12] + 0.5);
	r.add[1] = (int)floor(tm[13] + 0.5);

	int planes = 4;
	int view_flags = 0xAA; // should contain only bits that face viewing direction

	double clip_world[4][4];

	double clip_left[4] =   { 1, 0, 0, .9 };
	double clip_right[4] =  {-1, 0, 0, .9 };
	double clip_bottom[4] = { 0, 1, 0, .9 };
	double clip_top[4] =    { 0,-1, 0, .9 };

	// easier to use another transform for clipping
	{
		// somehow it works
		double clip_tm[16];
		clip_tm[0] = +cosyaw / (0.5 * dw) * ds * HEIGHT_CELLS;
		clip_tm[1] = -sinyaw*sin30 / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[2] = 0;
		clip_tm[3] = 0;
		clip_tm[4] = +sinyaw / (0.5 * dw) * ds * HEIGHT_CELLS;
		clip_tm[5] = +cosyaw*sin30 / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[6] = 0;
		clip_tm[7] = 0;
		clip_tm[8] = 0;
		clip_tm[9] = +cos30 / HEIGHT_SCALE / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[10] = +2. / 0xffff;
		clip_tm[11] = 0;
		clip_tm[12] = -(pos[0] * clip_tm[0] + pos[1] * clip_tm[4] + pos[2] * clip_tm[8]);
		clip_tm[13] = -(pos[0] * clip_tm[1] + pos[1] * clip_tm[5] + pos[2] * clip_tm[9]);
		clip_tm[14] = -1.0;
		clip_tm[15] = 1.0;

		TransposeProduct(clip_tm, clip_left, clip_world[0]);
		TransposeProduct(clip_tm, clip_right, clip_world[1]);
		TransposeProduct(clip_tm, clip_bottom, clip_world[2]);
		TransposeProduct(clip_tm, clip_top, clip_world[3]);
	}

	QueryTerrain(t, planes, clip_world, view_flags, Renderer::RenderPatch, &r);

	void* GetMaterialArr();
	Material* matlib = (Material*)GetMaterialArr();

	Sample* src = r.sample_buffer.ptr + 1 + dw;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++, ptr++, src++)
		{
			int mat = src[0].visual & 0x00FF;
			int shd = 0; // (src[0].visual >> 8) & 0x007F;
			int elv = 0; // (src[0].visual >> 15) & 0x0001;

			// fill from material
			const MatCell* cell = &(matlib[mat].shade[1][shd]);
			const uint8_t* bg = matlib[mat].shade[1][shd].bg;
			const uint8_t* fg = matlib[mat].shade[1][shd].fg;

			ptr->gl = cell->gl;
			ptr->bk = 16 + (((bg[0]+25) / 51) + (((bg[1]+25) / 51) * 6) + (((bg[2]+25) / 51) * 36));
			ptr->fg = 16 + (((fg[0]+25) / 51) + (((fg[1]+25) / 51) * 6) + (((fg[2]+25) / 51) * 36));
			ptr->spare = 0xFF;
		}

		src += 2;
	}

	/*
	Sample* src = r.sample_buffer.ptr+2*dw+2;
	for (int y=0; y<height; y++)
	{
		for (int x = 0; x < width; x++, ptr++, src += 2)
		{
			ptr->fg = 0x00;
			ptr->bk = 0xF0;
			ptr->gl = src[0].visual & 0xFF;
			//ptr->gl ^= src[1].visual & 0xFF;
			//ptr->gl ^= src[dw].visual & 0xFF;
			//ptr->gl ^= src[dw+1].visual & 0xFF;
			ptr->spare = 0xFF; // alpha?
		}
		src += 2 + dw + 2;
	}
	*/

	free(r.sample_buffer.ptr);
	return true;
}
