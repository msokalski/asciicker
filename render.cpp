
// this is CPU scene renderer into ANSI framebuffer
#include "render.h"
#include "sprite.h"

#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include "terrain.h"
#include "world.h"
#include "matrix.h"
#include "fast_rand.h"
// #include "sprite.h"

#include "inventory.h"

#include "PerlinNoise.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <string.h>

#define DBL

static bool global_refl_mode = false;
extern Sprite* player_sprite;
extern Sprite* attack_sprite;
extern Sprite* inventory_sprite;

template <typename Sample>
inline void Bresenham(Sample* buf, int w, int h, int from[3], int to[3])
{
	int sx = to[0] - from[0];
	int sy = to[1] - from[1];

	if (sx == 0 && sy==0)
		return;

	int sz = to[2] - from[2];

	int ax = sx >= 0 ? sx : -sx;
	int ay = sy >= 0 ? sy : -sy;

	if (ax >= ay)
	{
		float n = +1.0f / sx;
		// horizontal domain

		if (from[0] > to[0])
		{
			int* swap = from;
			from = to;
			to = swap;
		}

		int	x0 = (std::max(0, from[0]) + 1) & ~1; // round up start x, so we won't produce out of domain samples
		int	x1 = std::min(w, to[0]);

		for (int x = x0; x < x1; x+=2)
		{
			float a = x - from[0] + 0.5f;
			int y = (int)floor((a * sy)*n + from[1] + 0.5f);
			if (y >= 0 && y < h)
			{
				float z = (a * sz) * n + from[2];
				Sample* ptr = buf + w * y + x;
				if (ptr->DepthTest_RO(z))
					ptr->spare |= 4;
				ptr++;
				if (ptr->DepthTest_RO(z))
					ptr->spare |= 4;
			}
		}
	}
	else
	{
		float n = 1.0f / sy;
		// vertical domain

		if (from[1] > to[1])
		{
			int* swap = from;
			from = to;
			to = swap;
		}

		int y0 = std::max(0, from[1]);
		int y1 = std::min(h, to[1]);

		for (int y = y0; y < y1; y++)
		{
			int a = y - from[1];
			int x = (int)floor((a * sx) * n + from[0] + 0.5f);
			if (x >= 0 && x < w)
			{
				float z = (a * sz)*n + from[2];
				Sample* ptr = buf + w * y + x;
				if (ptr->DepthTest_RO(z))
					ptr->spare |= 4;
			}
		}
	}
}

// todo: lets add "int varyings" template arg
// and add "const float varying[3][varyings]" function param (values at verts)
// so we can calc here values in lower-left corner and x,y gradients
// and provide all varyings interpolated into shader->Blend() call
// we should do it also for 'z' coord

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

		// todo:
		// calc all varyings at 0,0 screen coord
		// and their dx,dy gradients

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
					//if ((bc[0] | bc[1] | bc[2]) < 0)
						continue;

					/*
					if (area > 0)
					{
						if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0)
							continue;
					}
					else
					{
						if (bc[0] > 0 || bc[1] > 0 || bc[2] > 0)
							continue;
					}
					*/

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

					// todo: calc 'z' as z @ 0,0 + x*dzdx + y*dzdy
					// ...
					
					// todo: separate depth+water test from fill
					// make test here, if it passes calc all other varyings similarly to 'z'
					// ...

					float z = nbc[0] * v[0][2] + nbc[1] * v[1][2] + nbc[2] * v[2][2];
					//float z = (bc[0] * v[0][2] + bc[1] * v[1][2] + bc[2] * v[2][2]) * normalizer;

					// currently we have 3 muls (normalization) and 3 muls, 2 adds per varying
					// that for {z,r,g,b} results in: 15 muls and 8 adds

					// if we use gradients, we'd have: 2 muls and 2 adds per varying
					// resulting in: 8 muls and 8 adds

					s->Blend(row,z,nbc);
					/*
					if (row->DepthTest_RW(z))
					{
						if (global_refl_mode)
							s->Refl(row, nbc);
						else
							s->Fill(row, nbc);
					}
					*/
				}
			}
		}
	}
	#undef BC
}



struct Sample
{
	uint16_t visual;
	uint8_t diffuse;
	uint8_t spare;   // refl, patch xy parity etc..., direct color bit (meshes): visual has just 565 color?
	float height;

	/*
	inline bool DepthTest_RW(float z)
	{
		if (height > z)
			return false;
		spare &= ~0x4; // clear lines
		height = z;
		return true;
	}
	*/

	inline bool DepthTest_RO(float z)
	{
		if (height > z)
		{
			int a = 0;
		}
		return height <= z;
	}
};

struct SampleBuffer
{
	int w, h; // make 2x +2 bigger than terminal buffer
	Sample* ptr;
};

struct SpriteRenderBuf
{
	Sprite* sprite;
	int s_pos[3];
	int angle;
	int anim;
	int frame;
	int reps[4];
	bool refl;
};

struct Renderer
{
	void Init()
	{
		memset(this, 0, sizeof(Renderer));
		pn.reseed(std::default_random_engine::default_seed);
	}

	void Free()
	{
		if (sample_buffer.ptr)
			free(sample_buffer.ptr);
		if (sprites_alloc)
			free(sprites_alloc);
	}

	uint64_t stamp;
	siv::PerlinNoise pn;
	double pn_time;

	SampleBuffer sample_buffer; // render surface

	int sprites_alloc_size;
	int sprites;
	SpriteRenderBuf* sprites_alloc;

	uint8_t* buffer;
	int buffer_size; // ansi_buffer allocation size in cells (minimize reallocs)

	static void RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/);
	static void RenderSprite(Sprite* s, float pos[3], float yaw, int anim, int frame, int reps[4], void* cookie /*Renderer*/);
	static void RenderMesh(Mesh* m, double* tm, void* cookie /*Renderer*/);
	static void RenderFace(float coords[9], uint8_t colors[12], uint32_t visual, void* cookie /*Renderer*/);
	
	// unstatic -> needs R/W access to sample_buffer.ptr[].height for depth testing!
	void RenderSprite(AnsiCell* ptr, int width, int height, Sprite* s, bool refl, int anim, int frame, int angle, int pos[3]);

	// transform
	double mul[6]; // 3x2 rot part
	double add[3]; // post rotated and rounded translation
	float yaw,pos[3];
	float water;
	float light[4];
	bool int_flag;

	double viewinst_tm[16];
	const double* inst_tm;

	int patch_uv[HEIGHT_CELLS][2]; // constant
};

static int create_auto_mat(uint8_t mat[]);
static uint8_t auto_mat[/*b*/32/*g*/ * 32/*r*/ * 32/*bg,fg,gl*/ * 3];
int auto_mat_result = create_auto_mat(auto_mat);
static int create_auto_mat(uint8_t mat[])
{
	/*
	#define FLO(x) ((int)floor(5 * x / 31.0f))
	#define REM(x) (5*x-31*flo[x])
	*/

	#define MCV 5
	#define MCV_TO_5(mcv) (((mcv) * 5 + MCV/2) / MCV)
	#define FLO(x) ((int)floor(MCV * x / 31.0f))
	#define REM(x) (MCV*x-31*flo[x])

	static const int flo[32] =
	{
		FLO(0),  FLO(1),  FLO(2),  FLO(3),
		FLO(4),  FLO(5),  FLO(6),  FLO(7),
		FLO(8),  FLO(9),  FLO(10), FLO(11),
		FLO(12), FLO(13), FLO(14), FLO(15),
		FLO(16), FLO(17), FLO(18), FLO(19),
		FLO(20), FLO(21), FLO(22), FLO(23),
		FLO(24), FLO(25), FLO(26), FLO(27),
		FLO(28), FLO(29), FLO(30), FLO(31),
	};

	static const int rem[32]=
	{
		REM(0),  REM(1),  REM(2),  REM(3),
		REM(4),  REM(5),  REM(6),  REM(7),
		REM(8),  REM(9),  REM(10), REM(11),
		REM(12), REM(13), REM(14), REM(15),
		REM(16), REM(17), REM(18), REM(19),
		REM(20), REM(21), REM(22), REM(23),
		REM(24), REM(25), REM(26), REM(27),
		REM(28), REM(29), REM(30), REM(31),
	};

	static const char glyph[] = " ..::%";

	int max_pr = 0;

	int i = 0;
	for (int b=0; b<32; b++)
	{
		int p[3];
		p[2] = rem[b];
		int B[2] = { flo[b],std::min(MCV, flo[b] + 1) };
		for (int g = 0; g < 32; g++)
		{
			p[1] = rem[g];
			int G[2] = { flo[g],std::min(MCV, flo[g] + 1) };
			for (int r = 0; r < 32; r++,i++)
			{
				p[0] = rem[r];
				int R[2] = { flo[r],std::min(MCV, flo[r] + 1) };

				float best_sd = -1;
				float best_pr;
				int best_lo;
				int best_hi;

				// check all pairs of 8 cube verts
				for (int lo = 0; lo < 7; lo++)
				{
					int v0[3] = { R[lo & 1], G[(lo & 2) >> 1], B[(lo & 4) >> 2] };

					int pv0[3]=
					{
						R[0] * 31 + p[0] - v0[0] * 31,
						G[0] * 31 + p[1] - v0[1] * 31,
						B[0] * 31 + p[2] - v0[2] * 31,
					};

					for (int hi = lo + 1; hi < 8; hi++)
					{
						int v1[3] = { R[hi & 1], G[(hi & 2) >> 1], B[(hi & 4) >> 2] };
						int v10[3] = { 31*(v1[0] - v0[0]), 31*(v1[1] - v0[1]), 31*(v1[2] - v0[2]) };

						int v10_sqrlen = v10[0] * v10[0] + v10[1] * v10[1] + v10[2] * v10[2];

						float pr = v10_sqrlen ? (v10[0] * pv0[0] + v10[1] * pv0[1] + v10[2] * pv0[2]) / (float)v10_sqrlen : 0.0f;

						// projection point
						float prp[3] = { v10[0] * pr, v10[1] * pr, v10[2] * pr };

						// dist vect
						float prv[3] = { pv0[0] - prp[0], pv0[1] - prp[1], pv0[2] - prp[2] };

						// square dist
						float sd = sqrtf(prv[0] * prv[0] + prv[1] * prv[1] + prv[2] * prv[2]);

						if (sd < best_sd || best_sd < 0)
						{
							best_sd = sd;
							best_pr = pr;
							best_lo = lo;
							best_hi = hi;
						}
					}
				}

				int idx = 3 * (r + 32 * (g + 32 * b));
				int shd = (int)floorf( best_pr * 11 + 0.5f );

				if (shd > 11)
				{
					shd = 11;
				}

				if (shd < 0)
				{
					shd = 0;
				}

				if (shd < 6)
				{
					mat[idx + 0] = 16 + 36 * MCV_TO_5(R[best_lo & 1]) + 6 * MCV_TO_5(G[(best_lo & 2) >> 1]) + MCV_TO_5(B[(best_lo & 4) >> 2]);
					mat[idx + 1] = 16 + 36 * MCV_TO_5(R[best_hi & 1]) + 6 * MCV_TO_5(G[(best_hi & 2) >> 1]) + MCV_TO_5(B[(best_hi & 4) >> 2]);
					mat[idx + 2] = glyph[shd];
				}
				else
				{
					mat[idx + 0] = 16 + 36 * MCV_TO_5(R[best_hi & 1]) + 6 * MCV_TO_5(G[(best_hi & 2) >> 1]) + MCV_TO_5(B[(best_hi & 4) >> 2]);
					mat[idx + 1] = 16 + 36 * MCV_TO_5(R[best_lo & 1]) + 6 * MCV_TO_5(G[(best_lo & 2) >> 1]) + MCV_TO_5(B[(best_lo & 4) >> 2]);
					mat[idx + 2] = glyph[11-shd];
				}
			}
		}
	}

	return 1;
}

void Renderer::RenderFace(float coords[9], uint8_t colors[12], uint32_t visual, void* cookie)
{
	struct Shader
	{
		void Blend(Sample*s, float z, float bc[3])
		{
			if (s->height < z)
			{
				if (global_refl_mode)
				{
					if (z < water + HEIGHT_SCALE / 8)
					{
						if (z > water)
							s->height = water;
						else
							s->height = z;

						int r8 = (int)floor(rgb[0][0] * bc[0] + rgb[1][0] * bc[1] + rgb[2][0] * bc[2]);
						int r5 = (r8 * 249 + 1014) >> 11;
						int g8 = (int)floor(rgb[0][1] * bc[0] + rgb[1][1] * bc[1] + rgb[2][1] * bc[2]);
						int g5 = (g8 * 249 + 1014) >> 11;
						int b8 = (int)floor(rgb[0][2] * bc[0] + rgb[1][2] * bc[1] + rgb[2][2] * bc[2]);
						int b5 = (b8 * 249 + 1014) >> 11;

						s->visual = r5 | (g5 << 5) | (b5 << 10);
						s->diffuse = diffuse;
						s->spare = (s->spare & ~0x4) | 0x8 | 0x3;
					}  
				}
				else 
				{
					if (z >= water - HEIGHT_SCALE / 8)
					{
						if (z < water)
							s->height = water;
						else
							s->height = z;

						int r8 = (int)floor(rgb[0][0] * bc[0] + rgb[1][0] * bc[1] + rgb[2][0] * bc[2]);
						int r5 = (r8 * 249 + 1014) >> 11;
						int g8 = (int)floor(rgb[0][1] * bc[0] + rgb[1][1] * bc[1] + rgb[2][1] * bc[2]);
						int g5 = (g8 * 249 + 1014) >> 11;
						int b8 = (int)floor(rgb[0][2] * bc[0] + rgb[1][2] * bc[1] + rgb[2][2] * bc[2]);
						int b5 = (b8 * 249 + 1014) >> 11;

						s->visual = r5 | (g5 << 5) | (b5 << 10);
						s->diffuse = diffuse;
						s->spare = (s->spare & ~(0x3|0x4)) | 0x8 | 0x1;
					}
				}
			}
		}

		/*
		void Refl(Sample* s, float bc[3]) const
		{
			if (s->height < water)
			{
				int r8 = (int)floor(rgb[0][0] * bc[0] + rgb[1][0] * bc[1] + rgb[2][0] * bc[2]);
				int r5 = (r8 * 249 + 1014) >> 11;
				int g8 = (int)floor(rgb[0][1] * bc[0] + rgb[1][1] * bc[1] + rgb[2][1] * bc[2]);
				int g5 = (g8 * 249 + 1014) >> 11;
				int b8 = (int)floor(rgb[0][2] * bc[0] + rgb[1][2] * bc[1] + rgb[2][2] * bc[2]);
				int b5 = (b8 * 249 + 1014) >> 11;

				s->visual = r5 | (g5 << 5) | (b5 << 10);
				s->diffuse = diffuse;
				s->spare |= 0x8 | 0x3;
			}
		}

		void Fill(Sample* s, float bc[3]) const
		{
			if (s->height >= water)
			{
				int r8 = (int)floor(rgb[0][0] * bc[0] + rgb[1][0] * bc[1] + rgb[2][0] * bc[2]);
				int r5 = (r8 * 249 + 1014) >> 11;
				int g8 = (int)floor(rgb[0][1] * bc[0] + rgb[1][1] * bc[1] + rgb[2][1] * bc[2]);
				int g5 = (g8 * 249 + 1014) >> 11;
				int b8 = (int)floor(rgb[0][2] * bc[0] + rgb[1][2] * bc[1] + rgb[2][2] * bc[2]);
				int b5 = (b8 * 249 + 1014) >> 11;

				s->visual = r5 | (g5 << 5) | (b5 << 10);
				s->diffuse = diffuse;
				s->spare |= 0x8;
			}
			else
				s->height = -1000000;
			//	s->spare = 3;
		}
		*/

		/*
		inline void Diffuse(int dzdx, int dzdy)
		{
			float nl = (float)sqrt(dzdx * dzdx + dzdy * dzdy + HEIGHT_SCALE * HEIGHT_SCALE);
			float df = (dzdx * light[0] + dzdy * light[1] + HEIGHT_SCALE * light[2]) / nl;
			df = df * (1.0f - 0.5f*light[3]) + 0.5f*light[3];
			diffuse = df <= 0 ? 0 : (int)(df * 0xFF);
		}
		*/

		uint8_t* rgb[3]; // per vertex colors
		float water;
		float light[4];
		uint8_t diffuse; // shading experiment
	} shader;


	Renderer* r = (Renderer*)cookie;
	shader.water = r->water;

	// temporarily, let's transform verts for each face separately

	int v[3][4];

	float tmp0[4], tmp1[4], tmp2[4];

	{
		float xyzw[] = { coords[0], coords[1], coords[2], 1.0f };
		Product(r->viewinst_tm, xyzw, tmp0);
		v[0][0] = (int)floor(tmp0[0] + 0.5f);
		v[0][1] = (int)floor(tmp0[1] + 0.5f);
		v[0][2] = (int)floor(tmp0[2] + 0.5f);
		v[0][3] = 0; // clip flags
	}

	{
		float xyzw[] = { coords[3], coords[4], coords[5], 1.0f };
		Product(r->viewinst_tm, xyzw, tmp1);
		v[1][0] = (int)floor(tmp1[0] + 0.5f);
		v[1][1] = (int)floor(tmp1[1] + 0.5f);
		v[1][2] = (int)floor(tmp1[2] + 0.5f);
		v[1][3] = 0; // clip flags
	}

	{
		float xyzw[] = { coords[6], coords[7], coords[8], 1.0f };
		Product(r->viewinst_tm, xyzw, tmp2);
		v[2][0] = (int)floor(tmp2[0] + 0.5f);
		v[2][1] = (int)floor(tmp2[1] + 0.5f);
		v[2][2] = (int)floor(tmp2[2] + 0.5f);
		v[2][3] = 0; // clip flags
	}

	int w = r->sample_buffer.w;
	int h = r->sample_buffer.h;
	Sample* ptr = r->sample_buffer.ptr;

	// normal is const, could be baked into mesh
	float e1[] = { coords[3] - coords[0], coords[4] - coords[1], coords[5] - coords[2] };
	float e2[] = { coords[6] - coords[0], coords[7] - coords[1], coords[8] - coords[2] };

	float n[4] =
	{
		e1[1] * e2[2] - e1[2] * e2[1],
		e1[2] * e2[0] - e1[0] * e2[2],
		e1[0] * e2[1] - e1[1] * e2[0],
		0
	};

	float inst_n[4];
	Product(r->inst_tm, n, inst_n);
	float nn = 1.0f / sqrtf(inst_n[0] * inst_n[0] + inst_n[1] * inst_n[1] + inst_n[2] * inst_n[2]);

	float df = nn * (inst_n[0] * r->light[0] + inst_n[1] * r->light[1] + inst_n[2] * r->light[2]);

	//diffuse = 1.0;

	df = df * (1.0f - 0.5f*r->light[3]) + 0.5f*r->light[3];
	df += 0.5;
	if (df > 1)
		df = 1;
	if (df < 0)
		df = 0;

	shader.diffuse = (int)(df * 0xFF);

	if (global_refl_mode)
	{
		const int* pv[3] = { v[2],v[1],v[0] };
		shader.rgb[0] = colors + 8;
		shader.rgb[1] = colors + 4;
		shader.rgb[2] = colors + 0;

		//for (int i = 0; i < 12; i++)
		//	colors[i] = colors[i] * 3 / 4;

		Rasterize(r->sample_buffer.ptr, r->sample_buffer.w, r->sample_buffer.h, &shader, pv);
	}
	else
	{
		const int* pv[3] = { v[0],v[1],v[2] };
		shader.rgb[0] = colors + 0;
		shader.rgb[1] = colors + 4;
		shader.rgb[2] = colors + 8;
		Rasterize(r->sample_buffer.ptr, r->sample_buffer.w, r->sample_buffer.h, &shader, pv);
	}
}

void Renderer::RenderSprite(Sprite* s, float pos[3], float yaw, int anim, int frame, int reps[4], void* cookie /*Renderer*/)
{
	if (!reps)
	{
		// item! frame contains purpose!
		if (frame != Item::WORLD)
			return;
		anim = frame = 0;

		static int _reps[4] = { -1,-1,-1,-1 };
		reps = _reps;
	}

	if (global_refl_mode && s->projs == 1)
		return;

	Renderer* r = (Renderer*)cookie;

	// transform and append to sprite render list
	if (r->sprites == r->sprites_alloc_size)
	{
		r->sprites_alloc_size += 16;
		r->sprites_alloc = (SpriteRenderBuf*)realloc(r->sprites_alloc, sizeof(SpriteRenderBuf) * r->sprites_alloc_size);
	}

	SpriteRenderBuf* buf = r->sprites_alloc + r->sprites;

	float w_pos[3] = { pos[0] * HEIGHT_CELLS, pos[1] * HEIGHT_CELLS, pos[2] };

	if (global_refl_mode)
	{
		//if (r->int_flag)
		{
			int tx = (int)floor(r->mul[0] * w_pos[0] + r->mul[2] * w_pos[1] + 0.5 + r->add[0]);
			int ty = (int)floor(r->mul[1] * w_pos[0] + r->mul[3] * w_pos[1] + r->mul[5] * w_pos[2] + 0.5 + r->add[1]);

			// convert from samples to cells
			buf->s_pos[0] = (tx - 1) >> 1;
			buf->s_pos[1] = (ty - 1) >> 1;
			buf->s_pos[2] = (int)2*r->water - ((int)floorf(w_pos[2] + 0.5) + HEIGHT_SCALE / 4);
		}
		/*
		else
		{
			int tx = (int)floor(r->mul[0] * w_pos[0] + r->mul[2] * w_pos[1] + 0.5) + r->add[0];
			int ty = (int)floor(r->mul[1] * w_pos[0] + r->mul[3] * w_pos[1] + r->mul[5] * w_pos[2] + 0.5) + r->add[1];

			// convert from samples to cells
			buf->s_pos[0] = (tx - 1) >> 1;
			buf->s_pos[1] = (ty - 2) >> 1;
			buf->s_pos[2] = (int)2 * r->water - ((int)floorf(w_pos[2] + 0.5) + HEIGHT_SCALE / 4);
		}
		*/
	}
	else
	{
		//if (r->int_flag)
		{
			int tx = (int)floor(r->mul[0] * w_pos[0] + r->mul[2] * w_pos[1] + 0.5 + r->add[0]);
			int ty = (int)floor(r->mul[1] * w_pos[0] + r->mul[3] * w_pos[1] + r->mul[5] * w_pos[2] + 0.5 + r->add[1]);

			// convert from samples to cells
			buf->s_pos[0] = (tx - 1) >> 1;
			buf->s_pos[1] = (ty - 1) >> 1;
			buf->s_pos[2] = (int)floorf(w_pos[2] + 0.5) + HEIGHT_SCALE / 4;
		}
		/*
		else
		{
			int tx = (int)floor(r->mul[0] * w_pos[0] + r->mul[2] * w_pos[1] + 0.5) + r->add[0];
			int ty = (int)floor(r->mul[1] * w_pos[0] + r->mul[3] * w_pos[1] + r->mul[5] * w_pos[2] + 0.5) + r->add[1];

			// convert from samples to cells
			buf->s_pos[0] = (tx - 1) >> 1;
			buf->s_pos[1] = (ty - 2) >> 1;
			buf->s_pos[2] = (int)floorf(w_pos[2] + 0.5) + HEIGHT_SCALE / 4;
		}
		*/
	}

	int ang = (int)floor((yaw - r->yaw) * s->angles / 360.0f + 0.5f);
	ang = ang >= 0 ? ang % s->angles : (ang % s->angles + s->angles) % s->angles;
	
	buf->sprite = s;
	buf->angle = ang;
	buf->anim = anim;
	buf->frame = frame;
	buf->reps[0] = reps[0];
	buf->reps[1] = reps[1];
	buf->reps[2] = reps[2];
	buf->reps[3] = reps[3];
	buf->refl = global_refl_mode;

	r->sprites++;
}

void Renderer::RenderMesh(Mesh* m, double* tm, void* cookie)
{
	Renderer* r = (Renderer*)cookie;
	double view_tm[16]=
	{
		r->mul[0] * HEIGHT_CELLS, r->mul[1] * HEIGHT_CELLS, 0.0, 0.0,
		r->mul[2] * HEIGHT_CELLS, r->mul[3] * HEIGHT_CELLS, 0.0, 0.0,
		r->mul[4], r->mul[5], global_refl_mode ? -1.0 : 1.0, 0.0,
		r->add[0], r->add[1], r->add[2], 1.0
	};

	r->inst_tm = tm;
	MatProduct(view_tm, tm, r->viewinst_tm);
	QueryMesh(m, Renderer::RenderFace, r);

	// transform verts int integer coords
	// ...

	// given interpolated RGB -> round to 555, store it in visual
	// copy to diffuse to diffuse
	// mark mash 'auto-material' as 0x8 flag in spare

	// in post pass:
	// if sample has 0x8 flag
	//   multiply rgb by diffuse (into 888 bg=fg)
	// apply color mixing with neighbours
	// if at least 1 sample have mesh bit in spare
	// - round mixed bg rgb to R5G5B5 and use auto_material[32K] -> {bg,fg,gl}
	// else apply gridlines etc.
}

// we could easily make it template of <Sample,Shader>
void Renderer::RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie /*Renderer*/)
{
	struct Shader
	{
		void Blend(Sample*s, float z, float bc[3])
		{
			if (s->height < z)
			{
				if (global_refl_mode)
				{
					if (z < water + HEIGHT_SCALE / 8)
					{
						if (z > water)
							s->height = water;
						else
							s->height = z;

						int u = (int)floor(uv[0] * bc[0] + uv[2] * bc[1] + uv[4] * bc[2]);
						int v = (int)floor(uv[1] * bc[0] + uv[3] * bc[1] + uv[5] * bc[2]);

						/*
						if (u >= VISUAL_CELLS || v >= VISUAL_CELLS)
						{
							// detect overflow
							s->visual = 2;
						}
						else
						*/
						{
							s->visual = map[v * VISUAL_CELLS + u];
							s->diffuse = diffuse;
							s->spare |= parity | 0x3;
							s->spare &= ~(0x4|0x8); // clear mesh and lines
						}
					}
				}
				else
				{
					if (z >= water - HEIGHT_SCALE / 8)
					{
						if (z < water)
							s->height = water;
						else
							s->height = z;

						int u = (int)floor(uv[0] * bc[0] + uv[2] * bc[1] + uv[4] * bc[2]);
						int v = (int)floor(uv[1] * bc[0] + uv[3] * bc[1] + uv[5] * bc[2]);

						/*
						if (u >= VISUAL_CELLS || v >= VISUAL_CELLS)
						{
							// detect overflow
							s->visual = 2;
						}
						else
						*/
						{
							uint16_t m = map[v * VISUAL_CELLS + u];
							if (m & 0x8000)
								s->height += HEIGHT_SCALE;

							s->visual = m;
							s->diffuse = diffuse;
							s->spare = (s->spare & ~(0x8|0x3|0x4)) | parity; // clear refl, mesh and line, then add parity
						}
					}
				}
			}
		}

		/*
		void Refl(Sample* s, float bc[3]) const
		{
			if (s->height < water)
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
		}

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
				s->height = -1000000;
			//else
			//	s->spare = 3;
		}
		*/

		inline void Diffuse(int dzdx, int dzdy)
		{
			float nl = (float)sqrt(dzdx * dzdx + dzdy * dzdy + HEIGHT_SCALE * HEIGHT_SCALE);
			float df = (dzdx * light[0] + dzdy * light[1] + HEIGHT_SCALE * light[2]) / nl;
			df = df * (1.0f - 0.5f*light[3]) + 0.5f*light[3];
			diffuse = df <= 0 ? 0 : (int)(df * 0xFF);
		}

		int* uv; // points to array of 6 ints (u0,v0,u1,v1,u2,v2) each is equal to 0 or VISUAL_CELLS
		uint16_t* map; // points to array of VISUAL_CELLS x VISUAL_CELLS ushorts
		float water;
		float light[4];
		uint8_t diffuse; // shading experiment
		uint8_t parity;
	} shader;

	Renderer* r = (Renderer*)cookie;

	double* mul = r->mul;

	int iadd[2] = { (int)r->add[0], (int)r->add[1] };
	double* add = r->add;

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

			if (global_refl_mode)
			{
				if (r->int_flag)
				{
					int tx = (int)floor(mul[0] * vx + mul[2] * vy + 0.5 + add[0]);
					int ty = (int)floor(mul[1] * vx + mul[3] * vy + mul[5] * vz + 0.5 + add[1]);

					xyzf[dy][dx][0] = tx;
					xyzf[dy][dx][1] = ty;
					xyzf[dy][dx][2] = (int)(2 * r->water) - vz;

					// todo: if patch is known to fully fit in screen, set f=0 
					// otherwise we need to check if / which screen edges cull each vertex
					xyzf[dy][dx][3] = (tx < 0) | ((tx > w) << 1) | ((ty < 0) << 2) | ((ty > h) << 3);
				}
				else
				{
					int tx = (int)floor(mul[0] * vx + mul[2] * vy + 0.5) + iadd[0];
					int ty = (int)floor(mul[1] * vx + mul[3] * vy + mul[5] * vz + 0.5) + iadd[1];

					xyzf[dy][dx][0] = tx;
					xyzf[dy][dx][1] = ty;
					xyzf[dy][dx][2] = (int)(2 * r->water) - vz;

					// todo: if patch is known to fully fit in screen, set f=0 
					// otherwise we need to check if / which screen edges cull each vertex
					xyzf[dy][dx][3] = (tx < 0) | ((tx > w) << 1) | ((ty < 0) << 2) | ((ty > h) << 3);
				}
			}
			else
			{
				// transform 
				if (r->int_flag)
				{
					int tx = (int)floor(mul[0] * vx + mul[2] * vy + 0.5 + add[0]);
					int ty = (int)floor(mul[1] * vx + mul[3] * vy + mul[5] * vz + 0.5 + add[1]);

					xyzf[dy][dx][0] = tx;
					xyzf[dy][dx][1] = ty;
					xyzf[dy][dx][2] = vz;

					// todo: if patch is known to fully fit in screen, set f=0 
					// otherwise we need to check if / which screen edges cull each vertex
					xyzf[dy][dx][3] = (tx < 0) | ((tx > w) << 1) | ((ty < 0) << 2) | ((ty > h) << 3);
				}
				else
				{
					int tx = (int)floor(mul[0] * vx + mul[2] * vy + 0.5) + iadd[0];
					int ty = (int)floor(mul[1] * vx + mul[3] * vy + mul[5] * vz + 0.5) + iadd[1];

					xyzf[dy][dx][0] = tx;
					xyzf[dy][dx][1] = ty;
					xyzf[dy][dx][2] = vz;

					// todo: if patch is known to fully fit in screen, set f=0 
					// otherwise we need to check if / which screen edges cull each vertex
					xyzf[dy][dx][3] = (tx < 0) | ((tx > w) << 1) | ((ty < 0) << 2) | ((ty > h) << 3);
				}
			}
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

	shader.light[0] = r->light[0];
	shader.light[1] = r->light[1];
	shader.light[2] = r->light[2];
	shader.light[3] = r->light[3];

	/*
	shader.light[0] = 0;
	shader.light[1] = 0;
	shader.light[2] = 1;
	*/

//	if (shader.parity == 1)
//		return;

	hm = hmap;

	const int (*uv)[2] = r->patch_uv;

	for (int dy = 0; dy < HEIGHT_CELLS; dy++, hm++)
	{
		for (int dx = 0; dx < HEIGHT_CELLS; dx++,diag>>=1, hm++)
		{
			//if (!(diag & 1))
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
				if (global_refl_mode)
				{
					//done
					int lo_uv[] = { uv[dx][0],uv[dy][1], uv[dx][1],uv[dy][0], uv[dx][0],uv[dy][0] };
					const int* lo[3] = { xyzf[dy + 1][dx], xyzf[dy][dx + 1], xyzf[dy][dx] };
					shader.uv = lo_uv;
					shader.Diffuse(-xyzf[dy][dx][2] + xyzf[dy][dx + 1][2], -xyzf[dy][dx][2] + xyzf[dy + 1][dx][2]);
					Rasterize(ptr, w, h, &shader, lo);
				}
				else
				{
					int lo_uv[] = { uv[dx][0],uv[dy][0], uv[dx][1],uv[dy][0], uv[dx][0],uv[dy][1] };
					const int* lo[3] = { xyzf[dy][dx], xyzf[dy][dx + 1], xyzf[dy + 1][dx] };
					shader.uv = lo_uv;
					shader.Diffuse(xyzf[dy][dx][2] - xyzf[dy][dx + 1][2], xyzf[dy][dx][2] - xyzf[dy + 1][dx][2]);
					Rasterize(ptr, w, h, &shader, lo);
				}

				// .__.
				//  \ |
				//   \|
				//    '
				// upper triangle
				if (global_refl_mode)
				{
					//done
					int up_uv[] = { uv[dx][1],uv[dy][0], uv[dx][0],uv[dy][1], uv[dx][1],uv[dy][1] };
					const int* up[3] = { xyzf[dy][dx + 1], xyzf[dy + 1][dx], xyzf[dy + 1][dx + 1] };
					shader.uv = up_uv;
					shader.Diffuse(-xyzf[dy + 1][dx][2] + xyzf[dy + 1][dx + 1][2], -xyzf[dy][dx + 1][2] + xyzf[dy + 1][dx + 1][2]);
					Rasterize(ptr, w, h, &shader, up);
				}
				else
				{
					int up_uv[] = { uv[dx][1],uv[dy][1], uv[dx][0],uv[dy][1], uv[dx][1],uv[dy][0] };
					const int* up[3] = { xyzf[dy + 1][dx + 1], xyzf[dy + 1][dx], xyzf[dy][dx + 1] };
					shader.uv = up_uv;
					shader.Diffuse(xyzf[dy + 1][dx][2] - xyzf[dy + 1][dx + 1][2], xyzf[dy][dx + 1][2] - xyzf[dy + 1][dx + 1][2]);
					Rasterize(ptr, w, h, &shader, up);
				}
			}
			else
			{
				// lower triangle
				//    .
				//   /|
				//  /_|
				// '  '
				if (global_refl_mode)
				{
					// done
					int lo_uv[] = { uv[dx][0],uv[dy][0], uv[dx][1],uv[dy][1], uv[dx][1],uv[dy][0] };
					const int* lo[3] = { xyzf[dy][dx], xyzf[dy + 1][dx + 1], xyzf[dy][dx + 1] };
					shader.uv = lo_uv;
					shader.Diffuse(-xyzf[dy][dx][2] + xyzf[dy][dx + 1][2], -xyzf[dy][dx + 1][2] + xyzf[dy + 1][dx + 1][2]);
					Rasterize(ptr, w, h, &shader, lo);
				}
				else
				{
					int lo_uv[] = { uv[dx][1],uv[dy][0], uv[dx][1],uv[dy][1], uv[dx][0],uv[dy][0] };
					const int* lo[3] = { xyzf[dy][dx + 1], xyzf[dy + 1][dx + 1], xyzf[dy][dx] };
					shader.uv = lo_uv;
					shader.Diffuse(xyzf[dy][dx][2] - xyzf[dy][dx + 1][2], xyzf[dy][dx + 1][2] - xyzf[dy + 1][dx + 1][2]);
					Rasterize(ptr, w, h, &shader, lo);
				}


				// upper triangle
				// .__.
				// | / 
				// |/  
				// '
				if (global_refl_mode)
				{
					//done
					int up_uv[] = { uv[dx][1],uv[dy][1], uv[dx][0],uv[dy][0], uv[dx][0],uv[dy][1] };
					const int* up[3] = { xyzf[dy + 1][dx + 1], xyzf[dy][dx], xyzf[dy + 1][dx]  };
					shader.uv = up_uv;
					shader.Diffuse(-xyzf[dy + 1][dx][2] + xyzf[dy + 1][dx + 1][2], -xyzf[dy][dx][2] + xyzf[dy + 1][dx][2]);
					Rasterize(ptr, w, h, &shader, up);
				}
				else
				{
					int up_uv[] = { uv[dx][0],uv[dy][1], uv[dx][0],uv[dy][0], uv[dx][1],uv[dy][1] };
					const int* up[3] = { xyzf[dy + 1][dx], xyzf[dy][dx], xyzf[dy + 1][dx + 1] };
					shader.uv = up_uv;
					shader.Diffuse(xyzf[dy + 1][dx][2] - xyzf[dy + 1][dx + 1][2], xyzf[dy][dx][2] - xyzf[dy + 1][dx][2]);
					Rasterize(ptr, w, h, &shader, up);
				}
			}
		}
	}


	if (!global_refl_mode) // disabled on reflections
	{
		// grid lines thru middle of patch?
		int mid = (HEIGHT_CELLS + 1) / 2;

		for (int lin = 0; lin <= HEIGHT_CELLS; lin++)
		{
			xyzf[lin][mid][2] += HEIGHT_SCALE / 2;
			if (mid != lin)
				xyzf[mid][lin][2] += HEIGHT_SCALE / 2;
		}

		for (int lin = 0; lin < HEIGHT_CELLS; lin++)
		{
			Bresenham(ptr, w, h, xyzf[lin][mid], xyzf[lin + 1][mid]);
			Bresenham(ptr, w, h, xyzf[mid][lin], xyzf[mid][lin + 1]);
		}
	}
}

static const uint16_t glyph_coverage[256] =
{
	0x0000,0x2222,0x4433,0x3412,0x2312,0x2323,0x2312,0x1111,0x3333,0x1111,0x3333,0x4122,0x2222,0x2203,0x3322,0x3322,
	0x1212,0x2121,0x2222,0x2211,0x3321,0x2222,0x0022,0x2233,0x2211,0x1122,0x2121,0x1212,0x0111,0x2222,0x1122,0x2211,
	0x0000,0x2211,0x1100,0x2322,0x2211,0x1112,0x2222,0x1100,0x0201,0x1201,0x2211,0x1111,0x0011,0x1100,0x0011,0x2102,
	0x3222,0x1211,0x2112,0x2121,0x2221,0x2221,0x1222,0x2101,0x2222,0x2211,0x1111,0x1111,0x1111,0x1111,0x1101,0x2111,
	0x3212,0x2222,0x2322,0x1212,0x2322,0x2312,0x2302,0x1222,0x2222,0x1111,0x2012,0x2322,0x0322,0x3322,0x2322,0x2212,
	0x2302,0x2221,0x2312,0x2221,0x2211,0x2222,0x2211,0x2222,0x2222,0x2211,0x2322,0x1212,0x0320,0x2121,0x1200,0x0011,
	0x1100,0x1122,0x1322,0x1112,0x2122,0x1112,0x1202,0x1122,0x1322,0x1111,0x2121,0x1212,0x1111,0x1222,0x1122,0x1112,
	0x1113,0x1122,0x1112,0x1121,0x1211,0x1122,0x1111,0x1122,0x1112,0x1122,0x1112,0x1211,0x1111,0x1111,0x1200,0x1122,
	0x1212,0x1122,0x2112,0x2222,0x1122,0x1222,0x2222,0x1212,0x2212,0x2212,0x1212,0x1111,0x2211,0x1211,0x2222,0x1122,
	0x1212,0x1122,0x3322,0x2222,0x1122,0x1122,0x1222,0x1222,0x1122,0x2222,0x2222,0x1212,0x1312,0x2222,0x1221,0x3112,
	0x1222,0x1111,0x2122,0x1122,0x1322,0x2222,0x2211,0x2211,0x1112,0x1101,0x1110,0x2232,0x2232,0x1122,0x2211,0x2211,
	0x1111,0x2222,0x3333,0x1111,0x1212,0x1313,0x2222,0x1123,0x1213,0x2222,0x2222,0x2222,0x2222,0x2311,0x1312,0x0112,
	0x2110,0x2211,0x1122,0x2121,0x1111,0x2222,0x3131,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,
	0x3311,0x2222,0x0033,0x3211,0x3121,0x2131,0x1132,0x3333,0x3333,0x1201,0x1021,0x4444,0x0044,0x0404,0x4040,0x4400,
	0x1212,0x2212,0x1201,0x2222,0x1212,0x1112,0x1112,0x1211,0x2222,0x2212,0x2222,0x1222,0x1212,0x2213,0x1211,0x2222,
	0x2211,0x1312,0x0212,0x0211,0x1202,0x2012,0x1111,0x1212,0x2200,0x0000,0x0000,0x2011,0x2200,0x2100,0x2222,0x1111,
};

/*
static const uint16_t palette_rgb[256] =
{
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x001, 0x002, 0x003, 0x004, 0x005,
	0x010, 0x011, 0x012, 0x013, 0x014, 0x015,
	0x020, 0x021, 0x022, 0x023, 0x024, 0x025,
	0x030, 0x031, 0x032, 0x033, 0x034, 0x035,
	0x040, 0x041, 0x042, 0x043, 0x044, 0x045,
	0x050, 0x051, 0x052, 0x053, 0x054, 0x055,

	0x100, 0x101, 0x102, 0x103, 0x104, 0x105,
	0x110, 0x111, 0x112, 0x113, 0x114, 0x115,
	0x120, 0x121, 0x122, 0x123, 0x124, 0x125,
	0x130, 0x131, 0x132, 0x133, 0x134, 0x135,
	0x140, 0x141, 0x142, 0x143, 0x144, 0x145,
	0x150, 0x151, 0x152, 0x153, 0x154, 0x155,

	0x200, 0x201, 0x202, 0x203, 0x204, 0x205,
	0x210, 0x211, 0x212, 0x213, 0x214, 0x215,
	0x220, 0x221, 0x222, 0x223, 0x224, 0x225,
	0x230, 0x231, 0x232, 0x233, 0x234, 0x235,
	0x240, 0x241, 0x242, 0x243, 0x244, 0x245,
	0x250, 0x251, 0x252, 0x253, 0x254, 0x255,

	0x300, 0x301, 0x302, 0x303, 0x304, 0x305,
	0x310, 0x311, 0x312, 0x313, 0x314, 0x315,
	0x320, 0x321, 0x322, 0x323, 0x324, 0x325,
	0x330, 0x331, 0x332, 0x333, 0x334, 0x335,
	0x340, 0x341, 0x342, 0x343, 0x344, 0x345,
	0x350, 0x351, 0x352, 0x353, 0x354, 0x355,

	0x400, 0x401, 0x402, 0x403, 0x404, 0x405,
	0x410, 0x411, 0x412, 0x413, 0x414, 0x415,
	0x420, 0x421, 0x422, 0x423, 0x424, 0x425,
	0x430, 0x431, 0x432, 0x433, 0x434, 0x435,
	0x440, 0x441, 0x442, 0x443, 0x444, 0x445,
	0x450, 0x451, 0x452, 0x453, 0x454, 0x455,

	0x500, 0x501, 0x502, 0x503, 0x504, 0x505,
	0x510, 0x511, 0x512, 0x513, 0x514, 0x515,
	0x520, 0x521, 0x522, 0x523, 0x524, 0x525,
	0x530, 0x531, 0x532, 0x533, 0x534, 0x535,
	0x540, 0x541, 0x542, 0x543, 0x544, 0x545,
	0x550, 0x551, 0x552, 0x553, 0x554, 0x555,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};
*/

int AverageGlyph(const AnsiCell* ptr, int mask)
{
	// SIMPLER:
	// calc averaged coverage over masked quadrants
	// if result is greater than half, return foreground otherwise background

	int cov = glyph_coverage[ptr->gl];

	int num = 0;
	int sum = 0;
	if (mask & 1)
	{
		sum += cov & 0xf;
		num++;
	}
	if (mask & 2)
	{
		sum += (cov >> 4) & 0xf;
		num++;
	}
	if (mask & 4)
	{
		sum += (cov >> 8) & 0xf;
		num++;
	}
	if (mask & 8)
	{
		sum += (cov >> 12) & 0xf;
		num++;
	}

	return sum > num * 2 ? ptr->fg : ptr->bk;

	/*
	// too complex, results in massive color bleeding
	int fg_rgb = palette_rgb[ptr->fg];
	int bk_rgb = palette_rgb[ptr->bk];

	int fg[3] = { fg_rgb & 0xf, (fg_rgb >> 4) & 0xf, (fg_rgb >> 8) & 0xf };
	int bk[3] = { bk_rgb & 0xf, (bk_rgb >> 4) & 0xf, (bk_rgb >> 8) & 0xf };

	int cc[3] = { 0,0,0 };

	int cov = glyph_coverage[ptr->gl];
	int fg_cov[4] = { cov & 0xf, (cov >> 4) & 0xf, (cov >> 8) & 0xf, (cov >> 12) & 0xf };
	int bk_cov[4] = { 4 - fg_cov[0], 4 - fg_cov[1], 4 - fg_cov[2], 4 - fg_cov[3] };

	int num = 0;

	if (mask & 1)
	{
		cc[0] += fg[0] * fg_cov[0] + bk[0] * bk_cov[0];
		cc[1] += fg[1] * fg_cov[0] + bk[1] * bk_cov[0];
		cc[2] += fg[2] * fg_cov[0] + bk[2] * bk_cov[0];
		num++;
	}
	if (mask & 2)
	{
		cc[0] += fg[0] * fg_cov[1] + bk[0] * bk_cov[1];
		cc[1] += fg[1] * fg_cov[1] + bk[1] * bk_cov[1];
		cc[2] += fg[2] * fg_cov[1] + bk[2] * bk_cov[1];
		num++;
	}
	if (mask & 4)
	{
		cc[0] += fg[0] * fg_cov[2] + bk[0] * bk_cov[2];
		cc[1] += fg[1] * fg_cov[2] + bk[1] * bk_cov[2];
		cc[2] += fg[2] * fg_cov[2] + bk[2] * bk_cov[2];
		num++;
	}
	if (mask & 8)
	{
		cc[0] += fg[0] * fg_cov[3] + bk[0] * bk_cov[3];
		cc[1] += fg[1] * fg_cov[3] + bk[1] * bk_cov[3];
		cc[2] += fg[2] * fg_cov[3] + bk[2] * bk_cov[3];
		num++;
	}

	if (!num)
		return 0;

	cc[0] = (cc[0] + num * 2) / (num * 4);
	cc[1] = (cc[1] + num * 2) / (num * 4);
	cc[2] = (cc[2] + num * 2) / (num * 4);

	return 16 + cc[0] + 6 * cc[1] + 36 * cc[2];
	*/
}

void Renderer::RenderSprite(AnsiCell* ptr, int width, int height, Sprite* s, bool refl, int anim, int frame, int angle, int pos[3])
{
	// intersect frame with screen buffer
	int i = frame + angle * s->anim[anim].length;
	if (refl)
		i += s->anim[anim].length * s->angles;

	Sprite::Frame* f = s->atlas + s->anim[anim].frame_idx[i];

	int dx = f->ref[0] / 2;
	int dy = f->ref[1] / 2;

	int left   = pos[0] - dx;
	int right  = left + f->width;
	int bottom = pos[1] - dy;
	int top    = bottom + f->height;

	left = std::max(0, left);
	right = std::min(width, right);

	if (left >= right)
		return;

	bottom = std::max(0, bottom);
	top = std::min(height, top);

	if (bottom >= top)
		return;

	int sample_xy = 2 + 2 * (2 + 2 * width + 2);
	int sample_dx = 2;
	int sample_dy = 2 * (2 + 2 * width + 2);
	int sample_ofs[4] = { 0, 1, 2 + 2 * width + 2, 2 + 2 * width + 2 + 1 };

	//static const float height_scale = HEIGHT_SCALE / 1.5; // WHY?????  HS*DBL/ZOOM ?

	static const float ds = 2.0 * (/*zoom*/ 1.0 * /*scale*/ 3.0) / VISUAL_CELLS * 0.5 /*we're not dbl_wh*/;
	static const float dz_dy = HEIGHT_SCALE / (cos(30 * M_PI / 180) * HEIGHT_CELLS * ds);

	for (int y = bottom; y < top; y++)
	{
		int fy = y - pos[1] + dy;
		for (int x = left; x < right; x++)
		{
			int fx = x - pos[0] + dx;
			AnsiCell* dst = ptr + x + width * y;
			const AnsiCell* src = f->cell + fx + fy * f->width;

			int depth_passed = 0;

			Sample* s00 = sample_buffer.ptr + sample_xy + x * sample_dx + y * sample_dy;
			Sample* s01 = s00 + 1;
			Sample* s10 = s00 + 2 + 2 * width + 2;
			Sample* s11 = s10 + 1;

			// spare is in full blocks, ref in half!
			float height = (2 * src->spare + f->ref[2]) * 0.5 * dz_dy + pos[2]; // *height_scale + pos[2]; // transform!
			if (refl && src->gl == 31)
			{
				int a = 0;
			}
			if (!refl && height >= water || refl && height <= water)
			{
				if (refl)
				{
					int a = 0;
				}

				// early rejection
				if (src->bk == 255 && src->fg == 255 ||
					(src->gl == 32 || src->gl == 0) && src->bk == 255 ||
					src->gl == 219 && src->fg == 255)
				{
					// NOP
				}
				else
				// full block write with FG & BK
				if (src->bk != 255 && src->fg != 255)
				{
					int mask = 0;
					if (height >= s00->height)
					{
						s00->height = height;
						mask|=1;
					}
					if (height >= s01->height)
					{
						s01->height = height;
						mask |= 2;
					}
					if (height >= s10->height)
					{
						s10->height = height;
						mask |= 4;
					}
					if (height >= s11->height)
					{
						s11->height = height;
						mask |= 8;
					}

					if (mask == 0xF)
					{
						*dst = *src;
					}
					else
					if (mask == 0x0)
					{
					}
					else
					if (mask==0x3) // lower
					{
						dst->bk = AverageGlyph(dst, 0xC);
						dst->fg = AverageGlyph(src, 0x3);
						dst->gl = 220;
					}
					else
					if (mask == 0xC) // upper
					{
						dst->bk = AverageGlyph(dst, 0x3);
						dst->fg = AverageGlyph(src, 0xC);
						dst->gl = 223;
					}
					else
					if (mask == 0x5) // left
					{
						dst->bk = AverageGlyph(dst, 0xA);
						dst->fg = AverageGlyph(src, 0x5);
						dst->gl = 221;
					}
					else
					if (mask == 0xA) // right
					{
						dst->bk = AverageGlyph(dst, 0x5);
						dst->fg = AverageGlyph(src, 0xA);
						dst->gl = 222;
					}
					else
					{
						dst->bk = AverageGlyph(dst, 0xF-mask);
						dst->fg = AverageGlyph(dst, mask);
						if (mask == 1 || mask == 2 || mask == 4 || mask == 8)
							dst->gl = 176;
						else
						if (mask == 9 || mask == 6)
							dst->gl = 177;
						else
							dst->gl = 178;
					}
				}
				else
				// full block write with BK
				if (src->bk != 255 && (src->gl == 32 || src->gl == 0))
				{
					int mask = 0;
					if (height >= s00->height)
					{
						s00->height = height;
						mask |= 1;
					}
					if (height >= s01->height)
					{
						s01->height = height;
						mask |= 2;
					}
					if (height >= s10->height)
					{
						s10->height = height;
						mask |= 4;
					}
					if (height >= s11->height)
					{
						s11->height = height;
						mask |= 8;
					}

					if (mask == 0xF)
					{
						dst->gl = 219;
						dst->fg = src->bk;
					}
					else
					if (mask == 0x0)
					{
					}
					else
					if (mask==0x3) // lower
					{
						dst->bk = AverageGlyph(dst, 0xC);
						dst->fg = src->bk;
						dst->gl = 220;
					}
					else
					if (mask == 0xC) // upper
					{
						dst->bk = AverageGlyph(dst, 0x3);
						dst->fg = src->bk;
						dst->gl = 223;
					}
					else
					if (mask == 0x5) // left
					{
						dst->bk = AverageGlyph(dst, 0xA);
						dst->fg = src->bk;
						dst->gl = 221;
					}
					else
					if (mask == 0xA) // right
					{
						dst->bk = AverageGlyph(dst, 0x5);
						dst->fg = src->bk;
						dst->gl = 222;
					}
					else
					{
						dst->bk = AverageGlyph(dst, 0xF-mask);
						dst->fg = src->bk;
						if (mask == 1 || mask == 2 || mask == 4 || mask == 8)
							dst->gl = 176;
						else
						if (mask == 9 || mask == 6)
							dst->gl = 177;
						else
							dst->gl = 178;
					}
				}
				else
				// full block write with FG
				if (src->fg != 255 && src->gl == 219)
				{
					int mask = 0;
					if (height >= s00->height)
					{
						s00->height = height;
						mask |= 1;
					}
					if (height >= s01->height)
					{
						s01->height = height;
						mask |= 2;
					}
					if (height >= s10->height)
					{
						s10->height = height;
						mask |= 4;
					}
					if (height >= s11->height)
					{
						s11->height = height;
						mask |= 8;
					}

					if (mask == 0xF)
					{
						dst->gl = 219;
						dst->fg = src->bk;
					}
					else
					if (mask == 0x0)
					{
					}
					else
					if (mask==0x3) // lower
					{
						dst->bk = AverageGlyph(dst, 0xC);
						dst->fg = src->fg;
						dst->gl = 220;
					}
					else
					if (mask == 0xC) // upper
					{
						dst->bk = AverageGlyph(dst, 0x3);
						dst->fg = src->fg;
						dst->gl = 223;
					}
					else
					if (mask == 0x5) // left
					{
						dst->bk = AverageGlyph(dst, 0xA);
						dst->fg = src->fg;
						dst->gl = 221;
					}
					else
					if (mask == 0xA) // right
					{
						dst->bk = AverageGlyph(dst, 0x5);
						dst->fg = src->fg;
						dst->gl = 222;
					}
					else
					{
						dst->bk = AverageGlyph(dst, 0xF-mask);
						dst->fg = src->fg;
						if (mask == 1 || mask == 2 || mask == 4 || mask == 8)
							dst->gl = 176;
						else
						if (mask == 9 || mask == 6)
							dst->gl = 177;
						else
							dst->gl = 178;
					}
				}
				else
				// half block transparaent
				if (src->gl >= 220 && src->gl <= 223)
				{
					int mask = 0;
					if (src->bk == 255 && src->gl == 220 || src->fg == 255 && src->gl == 223) // lower
					{
						if (height >= s00->height)
						{
							s00->height = height;
							mask |= 1;
						}
						if (height >= s01->height)
						{
							s01->height = height;
							mask |= 2;
						}
					}
					else
					if (src->bk == 255 && src->gl == 221 || src->fg == 255 && src->gl == 222) // left
					{
						if (height >= s00->height)
						{
							s00->height = height;
							mask |= 1;
						}
						if (height >= s10->height)
						{
							s10->height = height;
							mask |= 4;
						}
					}
					else
					if (src->bk == 255 && src->gl == 222 || src->fg == 255 && src->gl == 221) // right
					{
						if (height >= s01->height)
						{
							s01->height = height;
							mask |= 2;
						}
						if (height >= s11->height)
						{
							s11->height = height;
							mask |= 8;
						}
					}
					else
					if (src->bk == 255 && src->gl == 223 || src->fg == 255 && src->gl == 220) // upper
					{
						if (height >= s10->height)
						{
							s10->height = height;
							mask |= 4;
						}
						if (height >= s11->height)
						{
							s11->height = height;
							mask |= 8;
						}
					}

					int color = src->bk == 255 ? src->fg : src->bk;
					if (mask == 0x0)
					{
					}
					else
					if (mask==0x3) // lower
					{
						dst->bk = AverageGlyph(dst, 0xC);
						dst->fg = color;
						dst->gl = 220;
					}
					else
					if (mask == 0xC) // upper
					{
						dst->bk = AverageGlyph(dst, 0x3);
						dst->fg = color;
						dst->gl = 223;
					}
					else
					if (mask == 0x5) // left
					{
						dst->bk = AverageGlyph(dst, 0xA);
						dst->fg = color;
						dst->gl = 221;
					}
					else
					if (mask == 0xA) // right
					{
						dst->bk = AverageGlyph(dst, 0x5);
						dst->fg = color;
						dst->gl = 222;
					}
					else
					{
						dst->bk = AverageGlyph(dst, 0xF-mask);
						dst->fg = src->fg;
						dst->gl = 176;
					}
				}
				else
				{
					// something else with transparency
					int mask = 0;
					if (height >= s00->height)
					{
						s00->height = height;
						mask|=1;
					}
					if (height >= s01->height)
					{
						s01->height = height;
						mask |= 2;
					}
					if (height >= s10->height)
					{
						s10->height = height;
						mask |= 4;
					}
					if (height >= s11->height)
					{
						s11->height = height;
						mask |= 8;
					}

					if (mask == 0xF)
					{
						dst->bk = AverageGlyph(dst, 0xF);
						dst->fg = src->fg;
						dst->gl = src->gl;
					}
					else
					if (mask == 0x0)
					{
					}
					else
					if (mask==0x3) // lower
					{
						dst->bk = AverageGlyph(dst, 0xC);
						dst->fg = AverageGlyph(src, 0x3);
						dst->gl = 220;
					}
					else
					if (mask == 0xC) // upper
					{
						dst->bk = AverageGlyph(dst, 0x3);
						dst->fg = AverageGlyph(src, 0xC);
						dst->gl = 223;
					}
					else
					if (mask == 0x5) // left
					{
						dst->bk = AverageGlyph(dst, 0xA);
						dst->fg = AverageGlyph(src, 0x5);
						dst->gl = 221;
					}
					else
					if (mask == 0xA) // right
					{
						dst->bk = AverageGlyph(dst, 0x5);
						dst->fg = AverageGlyph(src, 0xA);
						dst->gl = 222;
					}
					else
					{
						dst->bk = AverageGlyph(dst, 0xF-mask);
						dst->fg = AverageGlyph(dst, mask);
						if (mask == 1 || mask == 2 || mask == 4 || mask == 8)
							dst->gl = 176;
						else
						if (mask == 9 || mask == 6)
							dst->gl = 177;
						else
							dst->gl = 178;
					}
				}
			}

			if (dst->bk == 255 || dst->fg == 255)
			{
				int a = 0;
			}
			
			///////////////////////////

			/*
			if (src->bk != 255)
			{
				if (src->fg != 255)
				{
					// check if at least 2/4 samples passes depth test, update all 4
					// ...

					if (!refl && height >= water || refl && height <= water)
					{
						if (height >= s00->height)
						{
							s00->height = height;
							depth_passed++;
						}
						if (height >= s01->height)
						{
							s01->height = height;
							depth_passed++;
						}
						if (height >= s10->height)
						{
							s10->height = height;
							depth_passed++;
						}
						if (height >= s11->height)
						{
							s11->height = height;
							depth_passed++;
						}
					}

					if (depth_passed >= 3)
					{
						*dst = *src;
						//s00->height = height;
						//s01->height = height;
						//s10->height = height;
						//s11->height = height;
					}
				}
				else
				{
					// check if at least 1/2 bk sample passes depth test, update both
					// ...

					if (!refl && height >= water || refl && height <= water)
					{
						if (height >= s00->height)
						{
							s00->height = height;
							depth_passed++;
						}
						if (height >= s01->height)
						{
							s01->height = height;
							depth_passed++;
						}
						if (height >= s10->height)
						{
							s10->height = height;
							depth_passed++;
						}
						if (height >= s11->height)
						{
							s11->height = height;
							depth_passed++;
						}
					}

					if (depth_passed >= 3)
					{
						if (dst->gl == 0xDC && src->gl == 0xDF || dst->gl == 0xDD && src->gl == 0xDE ||
							dst->gl == 0xDF && src->gl == 0xDC || dst->gl == 0xDE && src->gl == 0xDD)
						{
							dst->fg = src->bk;
						}
						else
						{
							dst->bk = src->bk;
							dst->gl = src->gl;
						}

						// s00->height = height;
						// s01->height = height;
						// s10->height = height;
						// s11->height = height;
					}
				}
			}
			else
			{
				if (src->fg != 255)
				{
					// check if at least 1/2 fg samples passes depth test, update both
					// ...
					if (!refl && height >= water || refl && height <= water)
					{
						if (height >= s00->height)
						{
							s00->height = height;
							depth_passed++;
						}
						if (height >= s01->height)
						{
							s01->height = height;
							depth_passed++;
						}
						if (height >= s10->height)
						{
							s10->height = height;
							depth_passed++;
						}
						if (height >= s11->height)
						{
							s11->height = height;
							depth_passed++;
						}
					}

					if (depth_passed >= 3)
					{
						if (dst->gl == 0xDC && src->gl == 0xDF || dst->gl == 0xDD && src->gl == 0xDE ||
							dst->gl == 0xDF && src->gl == 0xDC || dst->gl == 0xDE && src->gl == 0xDD)
						{
							dst->bk = src->fg;
						}
						else
						{
							dst->fg = src->fg;
							dst->gl = src->gl;
						}

						//s00->height = height;
						//s01->height = height;
						//s10->height = height;
						//s11->height = height;
					}
				}
			}
			*/
		}
	}
}

Renderer* CreateRenderer(uint64_t stamp)
{
	Renderer* r = (Renderer*)malloc(sizeof(Renderer));

	r->Init();
	r->stamp = stamp;
	return r;
}

void DeleteRenderer(Renderer* r)
{
	r->Free();
	free(r);
}

bool Render(Renderer* r, uint64_t stamp, Terrain* t, World* w, float water, float zoom, float yaw, const float pos[3], const float lt[4], int width, int height, AnsiCell* ptr, Sprite* sprite, int anim, int frame, float dir)
{
	AnsiCell* out_ptr = ptr;

	double dt = stamp - r->stamp;
	r->stamp = stamp;
	r->pn_time += 0.02 * dt / 16666.0; // dt is in microsecs
	if (r->pn_time >= 1000000000000.0)
		r->pn_time = 0.0;


#ifdef DBL
	float scale = 3.0;
#else
	float scale = 1.5;
#endif

	zoom *= scale;

#ifdef DBL
	int dw = 4+2*width;
	int dh = 4+2*height;
#else
	int dw = 1 + width + 1;
	int dh = 1 + height + 1;
#endif

	float ds = 2*zoom / VISUAL_CELLS;

	if (!r->sample_buffer.ptr)
	{
		r->int_flag = true;
		for (int uv=0; uv<HEIGHT_CELLS; uv++)
		{
			r->patch_uv[uv][0] = uv * VISUAL_CELLS / HEIGHT_CELLS;
			r->patch_uv[uv][1] = (uv+1) * VISUAL_CELLS / HEIGHT_CELLS;
		};


		r->sample_buffer.w = dw;
		r->sample_buffer.h = dh;
		r->sample_buffer.ptr = (Sample*)malloc(dw*dh * sizeof(Sample) * 2); // upper half is clear cache

		for (int cl = dw * dh; cl < 2*dw*dh; cl++)
		{
			r->sample_buffer.ptr[cl].height = -1000000;
			r->sample_buffer.ptr[cl].spare = 0x8;
			r->sample_buffer.ptr[cl].diffuse = 0xFF;
			r->sample_buffer.ptr[cl].visual = 0xC | (0xC << 5) | (0x1B << 10);
		}
	}
	else
	if (r->sample_buffer.w != dw || r->sample_buffer.h != dh)
	{
		r->int_flag = true;
		r->sample_buffer.w = dw;
		r->sample_buffer.h = dh;
		free(r->sample_buffer.ptr);
		r->sample_buffer.ptr = (Sample*)malloc(dw*dh * sizeof(Sample) * 2); // upper half is clear cache

		for (int cl = dw * dh; cl < 2 * dw*dh; cl++)
		{
			r->sample_buffer.ptr[cl].height = -1000000;
			r->sample_buffer.ptr[cl].spare = 0x8;
			r->sample_buffer.ptr[cl].diffuse = 0xFF;
			r->sample_buffer.ptr[cl].visual = 0xC | (0xC << 5) | (0x1B << 10);
		}
	}
	else
	{
		if (pos[0] != r->pos[0] || pos[1] != r->pos[1] || pos[2] != r->pos[2])
		{
			r->int_flag = true;
		}

		if (yaw != r->yaw)
		{
			r->int_flag = false;
		}
	}


	r->pos[0] = pos[0];
	r->pos[1] = pos[1];
	r->pos[2] = pos[2];
	r->yaw = yaw;

	r->light[0] = lt[0];
	r->light[1] = lt[1];
	r->light[2] = lt[2];
	r->light[3] = lt[3];

	// memset(r->sample_buffer.ptr, 0x00, dw*dh * sizeof(Sample));
	memcpy(r->sample_buffer.ptr, r->sample_buffer.ptr + dw * dh, dw*dh * sizeof(Sample));

	// for every cell we need to know world's xy coord where z is at the water level


	static const double sin30 = sin(M_PI*30.0/180.0); 
	static const double cos30 = cos(M_PI*30.0/180.0);

	/*
	static int frame = 0;
	frame++;
	if (frame == 200)
		frame = 0;
	water += HEIGHT_SCALE * 5 * sinf(frame*M_PI*0.01);
	*/

	// water integerificator (there's 4 instead of 2 because reflection goes 2x faster than water)
	int water_i = (int)floor(water / (HEIGHT_SCALE / (4 * ds * cos30)));
	water = (float)(water_i * (HEIGHT_SCALE / (4 * ds * cos30)));

	r->water = water;

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
	//tm[12] = dw*0.5 - (pos[0] * tm[0] + pos[1] * tm[4] + pos[2] * tm[8]) * HEIGHT_CELLS;
	//tm[13] = dh*0.5 - (pos[0] * tm[1] + pos[1] * tm[5] + pos[2] * tm[9]) * HEIGHT_CELLS;
	tm[12] = dw*0.5 - (pos[0] * tm[0] * HEIGHT_CELLS + pos[1] * tm[4] * HEIGHT_CELLS + pos[2] * tm[8]);
	tm[13] = dh*0.5 - (pos[0] * tm[1] * HEIGHT_CELLS + pos[1] * tm[5] * HEIGHT_CELLS + pos[2] * tm[9]);
	tm[14] = 0.0; //-1.0;
	tm[15] = 1.0;

	r->mul[0] = tm[0];
	r->mul[1] = tm[1];
	r->mul[2] = tm[4];
	r->mul[3] = tm[5];
	r->mul[4] = 0;
	r->mul[5] = tm[9];

	// if yaw didn't change, make it INTEGRAL (and EVEN in case of DBL)
	r->add[0] = tm[12];
	r->add[1] = tm[13] + 0.5;
	r->add[2] = tm[14];

	if (r->int_flag)
	{
		int x = (int)floor(r->add[0] + 0.5);
		int y = (int)floor(r->add[1] + 0.5);

		#ifdef DBL
		x &= ~1;
		y &= ~1;
		#endif

		r->add[0] = (double)x;
		r->add[1] = (double)y;
	}

	double proj_tm[] = { r->mul[0], r->mul[1], r->mul[2], r->mul[3], r->mul[4], r->mul[5], r->add[0], r->add[1], r->add[2] };

	int planes = 5;
	int view_flags = 0xAA; // should contain only bits that face viewing direction

	double clip_world[5][4];

	double clip_left[4] =   { 1, 0, 0, 1 };
	double clip_right[4] =  {-1, 0, 0, 1 };
	double clip_bottom[4] = { 0, 1, 0, 1 };
	double clip_top[4] =    { 0,-1, 0, 1 };
	double clip_water[4] =  { 0, 0, 1, -((r->water-1)*2.0/0xffff - 1.0) };

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
		TransposeProduct(clip_tm, clip_water, clip_world[4]);
	}

	r->sprites = 0;
	QueryTerrain(t, planes, clip_world, view_flags, Renderer::RenderPatch, r);
	QueryWorldCB cb = { Renderer::RenderMesh , Renderer::RenderSprite };
	QueryWorld(w, planes, clip_world, &cb, r);

	// HANDLE drawing sprites
	// ...

	// player shadow
	double inv_tm[16];
	Invert(tm, inv_tm);

	void* GetMaterialArr();
	Material* matlib = (Material*)GetMaterialArr();

	int sh_x = width+1;// & ~1;
	
	for (int y = 0; y < dh; y++)
	{
		for (int x = sh_x-5; x <= sh_x+5; x++)
		{
			Sample* s = r->sample_buffer.ptr + x + y * dw;
			if (abs(s->height - pos[2]) <= 64)
			{
				double screen_space[] = { (double)x,(double)y,s->height,1.0 };
				double world_space[4];

				Product(inv_tm, screen_space, world_space);
				double dx = world_space[0]/HEIGHT_CELLS - pos[0];
				double dy = world_space[1]/HEIGHT_CELLS - pos[1];
				double sq_xy = dx*dx + dy*dy;
				if (sq_xy <= 2.00)
				{
					int dz = (int)(2*(pos[2] - s->height) + 2*sq_xy);
					if (dz<180)
						dz=180;
					if (dz>180)
						dz=255;

					if (s->spare & 0x8)
					{
						s->diffuse = s->diffuse * dz / 255;
					}
					else
					{
						int mat = s->visual & 0xFF;
						int shd = (s->visual >> 8) & 0x7F;

						int r = (matlib[mat].shade[1][shd].bg[0] * 249 + 1014) >> 11;
						int g = (matlib[mat].shade[1][shd].bg[1] * 249 + 1014) >> 11;
						int b = (matlib[mat].shade[1][shd].bg[2] * 249 + 1014) >> 11;
						s->visual = r | (g << 5) | (b << 10);

						// if this is terrain sample, convert it to rgb first
						// s->visual = ;
						s->spare |= 0x8;
						s->spare &= ~4;
						s->diffuse = dz;
					}
				}
			}
		}
	}

	////////////////////
	// REFL

	// once again for reflections
	tm[8] = -tm[8];
	tm[9] = -tm[9];
	tm[10] = -tm[10]; // let them simply go below 0 :)

	//tm[12] = dw*0.5 - (pos[0] * tm[0] + pos[1] * tm[4] + ((2 * water / HEIGHT_CELLS) - pos[2]) * tm[8]) * HEIGHT_CELLS;
	//tm[13] = dh*0.5 - (pos[0] * tm[1] + pos[1] * tm[5] + ((2 * water / HEIGHT_CELLS) - pos[2]) * tm[9]) * HEIGHT_CELLS;
	tm[12] = dw*0.5 - (pos[0] * tm[0] * HEIGHT_CELLS + pos[1] * tm[4] * HEIGHT_CELLS + ((2 * water) - pos[2]) * tm[8]);
	tm[13] = dh*0.5 - (pos[0] * tm[1] * HEIGHT_CELLS + pos[1] * tm[5] * HEIGHT_CELLS + ((2 * water) - pos[2]) * tm[9]);
	tm[14] = 2*r->water;

	r->mul[0] = tm[0];
	r->mul[1] = tm[1];
	r->mul[2] = tm[4];
	r->mul[3] = tm[5];
	r->mul[4] = 0;
	r->mul[5] = tm[9];

	// if yaw didn't change, make it INTEGRAL (and EVEN in case of DBL)
	r->add[0] = tm[12];
	r->add[1] = tm[13] + 0.5;
	r->add[2] = tm[14];

	if (r->int_flag)
	{
		int x = (int)floor(r->add[0] + 0.5);
		int y = (int)floor(r->add[1] + 0.5);

		#ifdef DBL
		x &= ~1;
		y &= ~1;
		#endif

		r->add[0] = (double)x;
		r->add[1] = (double)y;
	}

	double refl_tm[] = { r->mul[0], r->mul[1], r->mul[2], r->mul[3], r->mul[4], r->mul[5], r->add[0], r->add[1], r->add[2] };

	clip_water[2] = -1; // was +1
	clip_water[3] = +((r->water+1)*-2.0 / 0xffff + 1.0); // was -((r->water-1)*2.0/0xffff - 1.0)

	{
		// somehow it works
		double clip_tm[16];
		clip_tm[0] = +cosyaw / (0.5 * dw) * ds * HEIGHT_CELLS;
		clip_tm[1] = -sinyaw * sin30 / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[2] = 0;
		clip_tm[3] = 0;
		clip_tm[4] = +sinyaw / (0.5 * dw) * ds * HEIGHT_CELLS;
		clip_tm[5] = +cosyaw * sin30 / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[6] = 0;
		clip_tm[7] = 0;
		clip_tm[8] = 0;
		clip_tm[9] = -cos30 / HEIGHT_SCALE / (0.5 * dh) * ds * HEIGHT_CELLS;
		clip_tm[10] = -2. / 0xffff;
		clip_tm[11] = 0;
		clip_tm[12] = -(pos[0] * clip_tm[0] + pos[1] * clip_tm[4] + (2 * r->water - pos[2]) * clip_tm[8]);
		clip_tm[13] = -(pos[0] * clip_tm[1] + pos[1] * clip_tm[5] + (2 * r->water - pos[2]) * clip_tm[9]);
		clip_tm[14] = +1.0;
		clip_tm[15] = 1.0;

		TransposeProduct(clip_tm, clip_left, clip_world[0]);
		TransposeProduct(clip_tm, clip_right, clip_world[1]);
		TransposeProduct(clip_tm, clip_bottom, clip_world[2]);
		TransposeProduct(clip_tm, clip_top, clip_world[3]);
		TransposeProduct(clip_tm, clip_water, clip_world[4]);
	}

	global_refl_mode = true;
	QueryTerrain(t, planes, clip_world, view_flags, Renderer::RenderPatch, r);
	QueryWorld(w, planes, clip_world, &cb, r);

	global_refl_mode = false;

	// clear and write new water ripples from player history position
	// do not emit wave if given z is greater than water level!
	memset(out_ptr, 0, sizeof(AnsiCell)*width*height);

	/*
	for (int h = 0; h < 64; h++)
	{
		float* xyz = hist[h];
		if (xyz[2] < water)
		{
			// draw ellipse
		}
	}
	*/

	Sample* src = r->sample_buffer.ptr + 2 + 2 * dw;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++, ptr++)
		{
			// given interpolated RGB -> round to 555, store it in visual
			// copy to diffuse to diffuse
			// mark mash 'auto-material' as 0x8 flag in spare

			// in post pass:
			// if sample has 0x8 flag
			//   multiply rgb by diffuse (into 888 bg=fg)
			// apply color mixing with neighbours
			// if at least 1 sample have mesh bit in spare
			// - round mixed bg rgb to R5G5B5 and use auto_material[32K] -> {bg,fg,gl}
			// else apply gridlines etc.


#ifdef DBL

// average 4 backgrounds
// mask 11 (something rendered)
			int spr[4] = { src[0].spare & 11, src[1].spare & 11, src[dw].spare & 11, src[dw + 1].spare & 11 };
			int mat[4] = { src[0].visual & 0x00FF , src[1].visual & 0x00FF, src[dw].visual & 0x00FF, src[dw + 1].visual & 0x00FF };
			int dif[4] = { src[0].diffuse , src[1].diffuse, src[dw].diffuse, src[dw + 1].diffuse };
			int vis[4] = { src[0].visual, src[1].visual, src[dw].visual, src[dw + 1].visual };

			// TODO:
			// every material must have 16x16 map and uses visual shade to select Y and lighting to select X
			// animated materials additionaly pre shifts and wraps visual shade by current time scaled by material's 'speed'

			int e_lo = (src[-dw].visual >> 15) + (src[-dw + 1].visual >> 15);
			int e_mi = (src[0].visual >> 15) + (src[1].visual >> 15);
			int e_hi = (src[dw].visual >> 15) + (src[dw + 1].visual >> 15);

			int elv;
			if (e_lo <= 1)
			{
				if (e_hi <= 1)
					elv = 3; // lo
				else
					elv = 2; // raise
			}
			else
			{
				if (e_hi <= 1)
					elv = 0; // lower
				else
					elv = 1; // hi
			}

			/*
			int shd = 0; // (src[0].visual >> 8) & 0x007F;

			int gl = matlib[mat[0]].shade[1][shd].gl;
			int bg[3] = { 0,0,0 };
			int fg[3] = { 0,0,0 };
			for (int i = 0; i < 4; i++)
			{
				bg[0] += matlib[mat[i]].shade[1][shd].bg[0] * dif[i];
				bg[1] += matlib[mat[i]].shade[1][shd].bg[1] * dif[i];
				bg[2] += matlib[mat[i]].shade[1][shd].bg[2] * dif[i];
				fg[0] += matlib[mat[i]].shade[1][shd].fg[0] * dif[i];
				fg[1] += matlib[mat[i]].shade[1][shd].fg[1] * dif[i];
				fg[2] += matlib[mat[i]].shade[1][shd].fg[2] * dif[i];
			}
			*/

			int shd = (dif[0] + dif[1] + dif[2] + dif[3] + 17 * 2) / (17 * 4); // 17: FF->F, 4: avr
			int gl = matlib[mat[0]].shade[elv][shd].gl;

			int bg[3] = { 0,0,0 }; // 4
			int fg[3] = { 0,0,0 };

			int half_h[2][2] = { {0,1},{2,3} };
			int half_v[2][2] = { {0,2},{1,3} };
			int bg_h[2][3] = { { 0,0,0 },{ 0,0,0 } }; // 0+1 \ 2+3 
			int bg_v[2][3] = { { 0,0,0 },{ 0,0,0 } }; // 0+2 | 1+3

			bool use_auto_mat = false;

			int err_h = 0;
			int err_v = 0;

			// if cell contains both refl and non-refl terrain enable auto-mat
			bool has_refl = (spr[0] & 3) == 3 || (spr[1] & 3) == 3 || (spr[2] & 3) == 3 || (spr[3] & 3) == 3;
			bool has_norm = (spr[0] & 3) == 1 || (spr[1] & 3) == 1 || (spr[2] & 3) == 1 || (spr[3] & 3) == 1;
			if (has_refl && has_norm)
			{
				use_auto_mat = true;
			}

			for (int m = 0; m < 2; m++)
			{
				for (int i = 0; i < 4; i++)
				{
					//if (spr[i])
					{
						if (spr[i] & 0x8)
						{
							int r = ((vis[i] & 0x1F) * 527 + 23) >> 6;
							int g = (((vis[i] >> 5) & 0x1F) * 527 + 23) >> 6;
							int b = (((vis[i] >> 10) & 0x1F) * 527 + 23) >> 6;

							if ((spr[i] & 0x3) == 3)
							{
								r = r * dif[i] / 400;
								g = g * dif[i] / 400;
								b = b * dif[i] / 400;
							}
							else
							{
								r = r * dif[i] / 255;
								g = g * dif[i] / 255;
								b = b * dif[i] / 255;
							}

							if (i == 0 || i == 1)
							{
								if (m)
								{
									err_h += abs(bg_h[0][0] - 4 * r);
									err_h += abs(bg_h[0][1] - 4 * g);
									err_h += abs(bg_h[0][2] - 4 * b);
								}
								else
								{
									bg_h[0][0] += 2 * r;
									bg_h[0][1] += 2 * g;
									bg_h[0][2] += 2 * b;
								}
							}
							if (i == 2 || i == 3)
							{
								if (m)
								{
									err_h += abs(bg_h[1][0] - 4 * r);
									err_h += abs(bg_h[1][1] - 4 * g);
									err_h += abs(bg_h[1][2] - 4 * b);
								}
								else
								{
									bg_h[1][0] += 2 * r;
									bg_h[1][1] += 2 * g;
									bg_h[1][2] += 2 * b;
								}
							}

							if (i == 0 || i == 2)
							{
								if (m)
								{
									err_v += abs(bg_v[0][0] - 4 * r);
									err_v += abs(bg_v[0][1] - 4 * g);
									err_v += abs(bg_v[0][2] - 4 * b);
								}
								else
								{
									bg_v[0][0] += 2 * r;
									bg_v[0][1] += 2 * g;
									bg_v[0][2] += 2 * b;
								}
							}
							if (i == 1 || i == 3)
							{
								if (m)
								{
									err_v += abs(bg_v[1][0] - 4 * r);
									err_v += abs(bg_v[1][1] - 4 * g);
									err_v += abs(bg_v[1][2] - 4 * b);
								}
								else
								{
									bg_v[1][0] += 2 * r;
									bg_v[1][1] += 2 * g;
									bg_v[1][2] += 2 * b;
								}
							}

							if (!m)
							{
								bg[0] += r;
								bg[1] += g;
								bg[2] += b;
								use_auto_mat = true;
							}
						}
						else
						{
							int r = matlib[mat[i]].shade[elv][shd].bg[0];
							int g = matlib[mat[i]].shade[elv][shd].bg[1];
							int b = matlib[mat[i]].shade[elv][shd].bg[2];

							if ((spr[i] & 0x3) == 3)
							{
								r = r * 255 / 400;
								g = g * 255 / 400;
								b = b * 255 / 400;
							}

							if (i == 0 || i == 1)
							{
								if (m)
								{
									err_h += abs(bg_h[0][0] - 4 * r);
									err_h += abs(bg_h[0][1] - 4 * g);
									err_h += abs(bg_h[0][2] - 4 * b);
								}
								else
								{
									bg_h[0][0] += 2 * r;
									bg_h[0][1] += 2 * g;
									bg_h[0][2] += 2 * b;
								}
							}
							if (i == 2 || i == 3)
							{
								if (m)
								{
									err_h += abs(bg_h[1][0] - 4 * r);
									err_h += abs(bg_h[1][1] - 4 * g);
									err_h += abs(bg_h[1][2] - 4 * b);
								}
								else
								{
									bg_h[1][0] += 2*r;
									bg_h[1][1] += 2*g;
									bg_h[1][2] += 2*b;
								}
							}

							if (i == 0 || i == 2)
							{
								if (m)
								{
									err_v += abs(bg_v[0][0] - 4 * r);
									err_v += abs(bg_v[0][1] - 4 * g);
									err_v += abs(bg_v[0][2] - 4 * b);
								}
								else
								{
									bg_v[0][0] += 2*r;
									bg_v[0][1] += 2*g;
									bg_v[0][2] += 2*b;
								}
							}
							if (i == 1 || i == 3)
							{
								if (m)
								{
									err_v += abs(bg_v[1][0] - 4 * r);
									err_v += abs(bg_v[1][1] - 4 * g);
									err_v += abs(bg_v[1][2] - 4 * b);
								}
								else
								{
									bg_v[1][0] += 2*r;
									bg_v[1][1] += 2*g;
									bg_v[1][2] += 2*b;
								}
							}

							if (!m)
							{
								bg[0] += r;
								bg[1] += g;
								bg[2] += b;

								if ((spr[i] & 0x3) == 0x3)
								{
									fg[0] += matlib[mat[i]].shade[elv][shd].fg[0] * 255 / 400;
									fg[1] += matlib[mat[i]].shade[elv][shd].fg[1] * 255 / 400;
									fg[2] += matlib[mat[i]].shade[elv][shd].fg[2] * 255 / 400;
								}
								else
								{
									fg[0] += matlib[mat[i]].shade[elv][shd].fg[0];
									fg[1] += matlib[mat[i]].shade[elv][shd].fg[1];
									fg[2] += matlib[mat[i]].shade[elv][shd].fg[2];
								}
							}
						}
					}
				}
			}

			if (use_auto_mat)
			{
				// WORKS REALY WELL! 
				bool vh_near = true;

				if (err_h * 1000 < err_v * 999)
				{
					vh_near = false;
					// _FG_
					//  BK
					ptr->gl = 0xDF;

					int auto_mat_lo = 3 * (bg_h[0][0] / 33 + 32 * (bg_h[0][1] / 33) + 32 * 32 * (bg_h[0][2] / 33));
					int auto_mat_hi = 3 * (bg_h[1][0] / 33 + 32 * (bg_h[1][1] / 33) + 32 * 32 * (bg_h[1][2] / 33));

					ptr->bk = auto_mat[auto_mat_lo + 0];
					ptr->fg = auto_mat[auto_mat_hi + 0];
				}
				else
				if (err_v * 1000 < err_h * 999)
				{
					vh_near = false;
					// B|F
					// K|G
					ptr->gl = 0xDE;

					int auto_mat_lt = 3 * (bg_v[0][0] / 33 + 32 * (bg_v[0][1] / 33) + 32 * 32 * (bg_v[0][2] / 33));
					int auto_mat_rt = 3 * (bg_v[1][0] / 33 + 32 * (bg_v[1][1] / 33) + 32 * 32 * (bg_v[1][2] / 33));

					ptr->bk = auto_mat[auto_mat_lt + 0];
					ptr->fg = auto_mat[auto_mat_rt + 0];
				}

				
				if (ptr->bk == ptr->fg || vh_near)
				{
					// avr4
					int auto_mat_idx = 3 * (bg[0] / 33 + 32 * (bg[1] / 33) + 32 * 32 * (bg[2] / 33));
					ptr->gl = auto_mat[auto_mat_idx + 2];
					ptr->bk = auto_mat[auto_mat_idx + 0];
					ptr->fg = auto_mat[auto_mat_idx + 1];
					ptr->spare = 0xFF;
				}
			}
			else
			{
				int bk_rgb[3] =
				{
					(bg[0] + 102) / 204,
					(bg[1] + 102) / 204,
					(bg[2] + 102) / 204
				};

				ptr->gl = gl;
				ptr->bk = 16 + 36*bk_rgb[0] + bk_rgb[1] * 6 + bk_rgb[2];
				ptr->fg = 16 + (36*((fg[0] + 102) / 204) + (((fg[1] + 102) / 204) * 6) + ((fg[2] + 102) / 204));
				ptr->spare = 0xFF;

				// collect line bits

				if (elv == 3) // only low elevation
				{
					int linecase = ((src[0].spare & 0x4) >> 2) | ((src[1].spare & 0x4) >> 1) | (src[dw].spare & 0x4) | ((src[dw + 1].spare & 0x4) << 1);
					static const int linecase_glyph[] = { 0, ',', ',', ',', '`', ';', ';', ';', '`', ';', ';', ';', '`', ';', ';', ';' };
					if (linecase)
						ptr->gl = linecase_glyph[linecase];
				}

				if (elv == 1 || elv == 3) // no elev change
				{
					// silhouette repetitoire:  _-/\| (should not be used by materials?)
					float z_hi = src[dw].height + src[dw + 1].height;
					float z_lo = src[0].height + src[1].height;
					float z_pr = src[-dw].height + src[1 - dw].height;

					float minus = z_lo - z_hi;
					float under = z_pr - z_lo;

					static const float thresh = 1 * HEIGHT_SCALE;

					if (minus > under)
					{
						if (minus > thresh)
						{
							ptr->gl = 0xC4; // '-'
							bk_rgb[0] = std::max(0, bk_rgb[0] - 1);
							bk_rgb[1] = std::max(0, bk_rgb[1] - 1);
							bk_rgb[2] = std::max(0, bk_rgb[2] - 1);
							ptr->fg = 16 + 36 * bk_rgb[0] + bk_rgb[1] * 6 + bk_rgb[2];
						}
					}
					else
					{
						if (under > thresh)
						{
							ptr->gl = 0x5F; // '_'
							bk_rgb[0] = std::max(0, bk_rgb[0] - 1);
							bk_rgb[1] = std::max(0, bk_rgb[1] - 1);
							bk_rgb[2] = std::max(0, bk_rgb[2] - 1);
							ptr->fg = 16 + 36 * bk_rgb[0] + bk_rgb[1] * 6 + bk_rgb[2];
						}
					}
				}
			}

			if (src[0].height < water && src[1].height < water && src[dw].height < water && src[dw+1].height < water)
			{
				double s[4] = { 2.0*x,2.0*y,water,1.0 };
				double w[4]; 
				Product(inv_tm, s, w); // convert from screen to world

				w[0] = round(w[0]);
				w[1] = round(w[1]);

				double d = r->pn.octaveNoise0_1(w[0] * 0.05, w[1] * 0.05, r->pn_time, 4);

				int id = (int)(d * 5) - 2;

				if (id < -1)
					id = 2;
				if (id > 1)
					id = -2;

				if (id > 0)
				{
					int c = ptr->fg - 16;
					int cr = c / 36;
					c -= cr * 36;
					int cg = c / 6;
					c -= cr * 6;
					int cb = c;

					if (cr < 5 && cg < 5 /*&& cb < 5*/)
					{
						if (cb < 5)
							ptr->fg += 1 + 6 + 36;
						else
							ptr->fg += 6 + 36;
					}
				}
				else
				if (id < 0)
				{
					int c = ptr->fg - 16;
					int cr = c / 36;
					c -= cr * 36;
					int cg = c / 6;
					c -= cr * 6;
					int cb = c;

					if (cr > 0 && cg > 0 /*&& cb > 0*/)
					{
						if (cb > 0)
							ptr->fg -= 1 + 6 + 36;
						else
							ptr->fg -= 6 + 36;
					}
				}
			}

			// xterm conv

			src += 2;


			
			#else
			
			int mat = src[0].visual & 0x00FF;
			int shd = 0; // (src[0].visual >> 8) & 0x007F;
			int elv = 0; // (src[0].visual >> 15) & 0x0001;

			// fill from material
			const MatCell* cell = &(matlib[mat].shade[1][shd]);
			const uint8_t* bg = matlib[mat].shade[1][shd].bg;
			const uint8_t* fg = matlib[mat].shade[1][shd].fg;

			ptr->gl = cell->gl;
			ptr->bk = 16 + (((bg[0] + 25) / 51) + (((bg[1] + 25) / 51) * 6) + (((bg[2] + 25) / 51) * 36));
			ptr->fg = 16 + (((fg[0] + 25) / 51) + (((fg[1] + 25) / 51) * 6) + (((fg[2] + 25) / 51) * 36));
			ptr->spare = 0xFF;

			src++;
			#endif

		}

		#ifdef DBL
		src += 4 + dw;
		#else
		src += 2;
		#endif
	}

#if 0

	int ang = (int)floor((player_dir-yaw) * player_sprite->angles / 360.0f + 0.5f);
	/*
	if (ang < 0)
		ang += player_sprite->angles * (1 - ang / player_sprite->angles);
	else
	if (ang >= player_sprite->angles)
		ang -= ang / player_sprite->angles;
	*/
	ang = ang >= 0 ? ang % player_sprite->angles : (ang % player_sprite->angles + player_sprite->angles) % player_sprite->angles;


	int anim = 1;
	
	// PLAYER
	int fr = player_stp/1024 % player_sprite->anim[anim].length;

	// WOLFIE
	// fr = player_stp/512 % player_sprite->anim[anim].length;

	if (player_stp < 0)
	{
		anim = 0;
		fr = 0;
	}

	static const float dy_dz = (cos(30 * M_PI / 180) * HEIGHT_CELLS * (ds / 2/*we're not dbl_wh*/)) / HEIGHT_SCALE;

	int player_pos[3] =
	{
		width / 2,
		height / 2,
		(int)floor(pos[2]+0.5) + HEIGHT_SCALE / 4
	};

	static uint64_t attack_tim = stamp;
	static int attack_frm = 18;	

	
	if (anim == 0 )
	{
		int attack_ofs = (stamp - attack_tim) / 16667; // scale by microsec to 60 fps
		attack_frm += attack_ofs;
		attack_tim += (uint64_t)attack_ofs * 16667;

		int sub_frm = (attack_frm % 80);

		/*
		static int attack_anim[30] =
		{
			0,0, 1,1, 2,2, 3,3, 4,4,
			4,4,4,4, 3,3,3,3, 2,2,2,2, 1,1,1,1, 0,0,0,0
		};
		*/

		// sleep/wake
		static int attack_anim[80] =
		{
			0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4,
			4,4,4,4, 3,3,3,3, 2,2,2,2, 1,1,1,1, 0,0,0,0,
		};


		if (sub_frm >= sizeof(attack_anim)/sizeof(int))
			sub_frm = 0;
		else
			sub_frm = attack_anim[sub_frm];

		fr = sub_frm;

		int pos_push[3] = { player_pos[0] , player_pos[1] , player_pos[2] };

		// player_pos[1] = height / 2 + (int)floor((2 * r->water - pos[2]) * dy_dz + 0.5);
		player_pos[1] = height / 2 - (int)floor(2 * (pos[2] - r->water)*dy_dz + 0.5);

		// player_pos[2] = (int)floor(2 * r->water - pos[2] + 0.5);
		player_pos[2] = (int)floor(2 * r->water - pos[2] + 0.5) - HEIGHT_SCALE / 4;

		r->RenderSprite(out_ptr, width, height, attack_sprite, true, anim, fr, ang, player_pos);
		r->RenderSprite(out_ptr, width, height, attack_sprite, false, anim, fr, ang, pos_push);
	}
	else
	{
		// rewind
		attack_tim = stamp;
		attack_frm = 18;	

		r->RenderSprite(out_ptr, width, height, player_sprite, false, anim, fr, ang, player_pos);

		// player_pos[1] = height / 2 + (int)floor((2 * r->water - pos[2]) * dy_dz + 0.5);
		player_pos[1] = height / 2 - (int)floor(2 * (pos[2] - r->water)*dy_dz + 0.5);

		// player_pos[2] = (int)floor(2 * r->water - pos[2] + 0.5);
		player_pos[2] = (int)floor(2 * r->water - pos[2] + 0.5) - HEIGHT_SCALE / 4;

		r->RenderSprite(out_ptr, width, height, player_sprite, true, anim, fr, ang, player_pos);
	}

#else
	if (sprite)
	{
		int ang = (int)floor((dir - yaw) * sprite->angles / 360.0f + 0.5f);
		ang = ang >= 0 ? ang % sprite->angles : (ang % sprite->angles + sprite->angles) % sprite->angles;

		{
			int player_pos[3]=
			{
				width / 2,
				height / 2,
				(int)floor(pos[2] + 0.5) + HEIGHT_SCALE / 4 
			};
			r->RenderSprite(out_ptr, width, height, sprite, false, anim, frame, ang, player_pos);
		}

		{
			static const float dy_dz = (cos(30 * M_PI / 180) * HEIGHT_CELLS * (ds / 2/*we're not dbl_wh*/)) / HEIGHT_SCALE;

			int player_pos[3] =
			{
				width / 2,
				height / 2 - (int)floor(2 * (pos[2] - r->water)*dy_dz + 0.5),
				(int)floor(2 * r->water - pos[2] + 0.5) - HEIGHT_SCALE / 4
			};
			r->RenderSprite(out_ptr, width, height, sprite, true, anim, frame, ang, player_pos);
		}
	}
#endif


	// lets check drawing sprites in world space
	for (int s=0; s<r->sprites; s++)
	{
		SpriteRenderBuf* buf = r->sprites_alloc + s;

		// IT IS PERFECTLY STICKED TO WORLD!
		// it may not perectly stick to character but its fine! (its kinda character is not perfectly positioned)


		int frame = buf->frame;
		int anim = buf->anim;
		r->RenderSprite(out_ptr, width, height, buf->sprite, buf->refl, anim, frame, buf->angle, buf->s_pos);
	}

	/*
	int invpos[3] = { 1,1,0 };
	if (inventory_sprite)
		r->RenderSprite(out_ptr, width, height, inventory_sprite, false, 0, 0, 0, invpos);
	*/

	return true;
}
