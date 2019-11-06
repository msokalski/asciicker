
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "asciiid_term.h"
#include "asciiid_platform.h"
#include "asciiid_render.h"
#include "fast_rand.h"
#include "matrix.h"
#include "gl.h"

#define CODE(...) #__VA_ARGS__
#define DEFN(a,s) "#define " #a #s "\n"

struct SoupItem
{
	float tri[3][3];
	float nrm[4]; // {nrm, w} is plane equ

	float CheckCollision(const float sphere_pos[3], const float sphere_vel[3], float contact_pos[3])
	{
		float col[3] = // on sphere @ time 0.0
		{
			sphere_pos[0] - nrm[0],
			sphere_pos[1] - nrm[1],
			sphere_pos[2] - nrm[2]
		};

		float vel_dot_nrm = -DotProduct(sphere_vel, nrm);
		float plane_t = 2;

		if (vel_dot_nrm > 0) // else: backface or (almost) parallel
		{
			float dist = DotProduct(col, nrm) + nrm[3];

			if (dist > 0)
			{
				plane_t = dist / vel_dot_nrm;
			}
			else
			if (dist > -1)
			{
				// embedded!
				dist = 1.0f + dist;
				contact_pos[0] = col[0] - dist * nrm[0];
				contact_pos[1] = col[1] - dist * nrm[1];
				contact_pos[2] = col[2] - dist * nrm[2];
				plane_t = 0;
			}
			else
				return 2;

			// contact pos on triangle plane
			contact_pos[0] = col[0] + plane_t * sphere_vel[0];
			contact_pos[1] = col[1] + plane_t * sphere_vel[1];
			contact_pos[2] = col[2] + plane_t * sphere_vel[2];

			// is it inside triangle?

			float edge[3][3] =
			{
				{tri[1][0] - tri[0][0], tri[1][1] - tri[0][1], tri[1][2] - tri[0][2]},
				{tri[2][0] - tri[1][0], tri[2][1] - tri[1][1], tri[2][2] - tri[1][2]},
				{tri[0][0] - tri[2][0], tri[0][1] - tri[2][1], tri[0][2] - tri[2][2]}
			};

			// collision point relative to given edge 
			float vect[3][3] =
			{
				{ contact_pos[0] - tri[0][0], contact_pos[1] - tri[0][1], contact_pos[2] - tri[0][2]},
				{ contact_pos[0] - tri[1][0], contact_pos[1] - tri[1][1], contact_pos[2] - tri[1][2]},
				{ contact_pos[0] - tri[2][0], contact_pos[1] - tri[2][1], contact_pos[2] - tri[2][2]},
			};

			float cross[3][3];
			float dot[3];

			CrossProduct(edge[0], vect[0], cross[0]);
			dot[0] = DotProduct(cross[0], nrm);

			CrossProduct(edge[1], vect[1], cross[1]);
			dot[1] = DotProduct(cross[1], nrm);

			CrossProduct(edge[2], vect[2], cross[2]);
			dot[2] = DotProduct(cross[2], nrm);

			if (dot[0] >= 0 && dot[1] >= 0 && dot[2] >= 0)
			{
				// inside
				return plane_t > 1 ? 2 : plane_t; // last thing to check is range (2=out of range)
			}
			else
			{
				 plane_t = 2;

				// spheres (ps,r)
				/*
				( p+v*t - ps )^2 = r^2

				( p-ps + v*t )^2 = r^2
				(p-ps)^2 + t*2*(p-ps)*v + t^2*v^2 = r^2

				A := v^2
				B := 2*dot(p-ps,v)
				C := (p-ps)^2 - r^2
				*/
				{
					float A = DotProduct(sphere_vel, sphere_vel);

					for (int s = 0; s < 3; s++)
					{
						float p_ps[3] =
						{
							sphere_pos[0] - tri[s][0],
							sphere_pos[1] - tri[s][1],
							sphere_pos[2] - tri[s][2]
						};

						float B = 2 * DotProduct(p_ps, sphere_vel);
						float C = DotProduct(p_ps, p_ps) - 1;

						float D = B * B - 4 * A*C;
						if (D >= 0)
						{
							// pick smaller root (A is positive, so take -sqrt)
							float t = (-B - sqrtf(D)) / (2 * A);

							if (t >= 0 && t <= 1)
							{
								if (t < plane_t)
								{
									plane_t = t;
									contact_pos[0] = tri[s][0];
									contact_pos[1] = tri[s][1];
									contact_pos[2] = tri[s][2];
								}
							}
						}
					}
				}

				// cylinders (pc,vx,r)
				/*
							   __       __
				( p+v*t - pc - vc * dot(vc, p+v*t - pc) )^2 = r^2

				( p+v*t - pc - vc * dot(vc, p+v*t - pc) / vc^2 )^2 = r^2

				( (p+v*t-pc)*vc^2 - vc * dot(vc, p+v*t - pc) )^2 = r^2 * vc^4

				( (p-pc)*vc^2 + t*v*vc^2 - vc * dot(vc, p-pc) - t * vc * dot(vc,v) )^2 = r^2 * vc^4

				( (p-pc)*vc^2 - vc * dot(vc, p-pc) + t * (v*vc^2 - vc * dot(vc,v)) )^2 = r^2 * vc^4

				U := (p-pc)*vc^2 - vc * dot(vc, p-pc)
				V := v*vc^2 - vc*dot(vc,v)

				( U + t * V )^2 = r^2 * vc^4

				U^2 + t*2*U*V + t^2*V^2 - r^2 * vc^4 = 0

				A = V^2
				B = 2*dot(U,V)
				C = U^2 - (r*vc^2)^2
				*/
				{
					for (int c = 0; c < 3; c++)
					{
						float vcvc = DotProduct(edge[c], edge[c]);
						float p_pc[3] =
						{
							sphere_pos[0] - tri[c][0],
							sphere_pos[1] - tri[c][1],
							sphere_pos[2] - tri[c][2]
						};

						float vc_dot_p_pc = DotProduct(edge[c], p_pc);

						float U[3] =
						{
							p_pc[0] * vcvc - edge[c][0] * vc_dot_p_pc,
							p_pc[1] * vcvc - edge[c][1] * vc_dot_p_pc,
							p_pc[2] * vcvc - edge[c][2] * vc_dot_p_pc
						};

						float vc_dot_v = DotProduct(edge[c], sphere_vel);

						float V[3] =
						{
							sphere_vel[0] * vcvc - edge[c][0] * vc_dot_v,
							sphere_vel[1] * vcvc - edge[c][1] * vc_dot_v,
							sphere_vel[2] * vcvc - edge[c][2] * vc_dot_v
						};

						float A = DotProduct(V, V);
						float B = 2 * DotProduct(U, V);
						float C = DotProduct(U, U) - vcvc * vcvc;

						float D = B * B - 4 * A*C;
						if (D >= 0)
						{
							// pick smaller root (A is positive, so take -sqrt)
							float t = (-B - sqrtf(D)) / (2 * A);

							if (t >= 0 && t <= 1)
							{
								if (t < plane_t)
								{
									float _pc[3] =
									{
										sphere_pos[0] + t * sphere_vel[0] - tri[c][0],
										sphere_pos[1] + t * sphere_vel[1] - tri[c][1],
										sphere_pos[2] + t * sphere_vel[2] - tri[c][2]
									};

									float h_mul_vc = DotProduct(_pc, edge[c]);
									if (h_mul_vc >= 0 && h_mul_vc <= vcvc)
									{
										plane_t = t;
										float h_div_vc = h_mul_vc / vcvc;
										contact_pos[0] = tri[c][0] + edge[c][0] * h_div_vc;
										contact_pos[1] = tri[c][1] + edge[c][1] * h_div_vc;
										contact_pos[2] = tri[c][2] + edge[c][2] * h_div_vc;
									}
								}
							}
						}
					}
				}
			}
		}
		return plane_t;
	}
};

struct TERM_LIST
{
	TERM_LIST* prev;
	TERM_LIST* next;
	A3D_WND* wnd;

	uint64_t stamp;

	float yaw;
	float yaw_vel;

	float player_dir;
	int player_stp;

	float pos[3];
	float water;

	float vel[3];

	float slope;

	uint8_t keys[32];
	bool IsKeyDown(int key)
	{
		return (keys[key >> 3] & (1 << (key & 0x7))) != 0;
	}

	static const int max_width = 320; // 160;
	static const int max_height = 180; // 90;
	AnsiCell buf[max_width*max_height];
	GLuint tex;
	GLuint prg;
	GLuint vbo;
	GLuint vao;

	/*
	int col_num[3];
	int col_msh;
	float col_tm[16];
	float col_nrm[3][3];
	*/

	SoupItem* soup;
	int soup_alloc;
	int soup_items;

	double* collect_tm;
	float collect_mul_xy;
	float collect_mul_z;

	bool collision_failure;

	static void FaceCollect(float coords[9], uint8_t* colors, uint32_t visual, void* cookie)
	{
		TERM_LIST* term = (TERM_LIST*)cookie;
		SoupItem* item = term->soup + term->soup_items;

		// multiply coords by collect_tm
		// then multiply x & y by collect_mul_xy and z by collect_mul_z
		// ...

		float v[3][4]=
		{
			{coords[0],coords[1],coords[2],1},
			{coords[3],coords[4],coords[5],1},
			{coords[6],coords[7],coords[8],1},
		};

		float tmv[4];

		Product(term->collect_tm, v[0], tmv);

		item->tri[0][0] = tmv[0] * term->collect_mul_xy;
		item->tri[0][1] = tmv[1] * term->collect_mul_xy;
		item->tri[0][2] = tmv[2] * term->collect_mul_z;

		Product(term->collect_tm, v[1], tmv);

		item->tri[1][0] = tmv[0] * term->collect_mul_xy;
		item->tri[1][1] = tmv[1] * term->collect_mul_xy;
		item->tri[1][2] = tmv[2] * term->collect_mul_z;

		Product(term->collect_tm, v[2], tmv);

		item->tri[2][0] = tmv[0] * term->collect_mul_xy;
		item->tri[2][1] = tmv[1] * term->collect_mul_xy;
		item->tri[2][2] = tmv[2] * term->collect_mul_z;

		{
			float* v[3] = { item->tri[0], item->tri[1], item->tri[2] };
			float e1[3] = { v[0][0] - v[2][0],v[0][1] - v[2][1],v[0][2] - v[2][2] };
			float e2[3] = { v[1][0] - v[2][0],v[1][1] - v[2][1],v[1][2] - v[2][2] };
			CrossProduct(e1, e2, item->nrm);
			float nrm = 1.0f / sqrtf(
				item->nrm[0] * item->nrm[0] +
				item->nrm[1] * item->nrm[1] +
				item->nrm[2] * item->nrm[2]);
			item->nrm[0] *= nrm;
			item->nrm[1] *= nrm;
			item->nrm[2] *= nrm;
			item->nrm[3] = -(v[2][0] * item->nrm[0] + v[2][1] * item->nrm[1] + v[2][2] * item->nrm[2]);
		}

		term->soup_items ++;
	}

	static void MeshCollect(Mesh* m, double tm[16], void* cookie)
	{
		TERM_LIST* term = (TERM_LIST*)cookie;

		int faces = GetMeshFaces(m);
		if (term->soup_alloc < term->soup_items + faces)
		{
			term->soup_alloc = 1414 * term->soup_alloc / 1000 + faces;
			term->soup = (SoupItem*)realloc(term->soup, sizeof(SoupItem) * term->soup_alloc);
		}

		term->collect_tm = tm;

		QueryMesh(m, FaceCollect, cookie);
	}
	
	static void PatchCollect(Patch* p, int x, int y, int view_flags, void* cookie)
	{
		TERM_LIST* term = (TERM_LIST*)cookie;

		int faces = 2 * HEIGHT_CELLS*HEIGHT_CELLS;

		if (term->soup_alloc < term->soup_items + faces)
		{
			term->soup_alloc = 1414 * term->soup_alloc / 1000 + faces;
			term->soup = (SoupItem*)realloc(term->soup, sizeof(SoupItem) * term->soup_alloc);
		}

		SoupItem* item = term->soup + term->soup_items;
		uint16_t diag = GetTerrainDiag(p);
		uint16_t* hmap = GetTerrainHeightMap(p);

		static const double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;
		bool hit = false;

		int rot = GetTerrainDiag(p);

		for (int hy = 0; hy < HEIGHT_CELLS; hy++)
		{
			for (int hx = 0; hx < HEIGHT_CELLS; hx++)
			{
				float x0 = (x + hx * sxy) * term->collect_mul_xy, x1 = x0 + sxy * term->collect_mul_xy;
				float y0 = (y + hy * sxy) * term->collect_mul_xy, y1 = y0 + sxy * term->collect_mul_xy;

				float v[4][3] =
				{
					{x0,y0,(float)hmap[hy*(HEIGHT_CELLS+1) + hx] * term->collect_mul_z},
					{x1,y0,(float)hmap[hy*(HEIGHT_CELLS + 1) + hx + 1] * term->collect_mul_z},
					{x0,y1,(float)hmap[(hy + 1)*(HEIGHT_CELLS + 1) + hx] * term->collect_mul_z},
					{x1,y1,(float)hmap[(hy + 1)*(HEIGHT_CELLS + 1) + hx + 1] * term->collect_mul_z},
				};

				if (rot & 1)
				{
					// v[2], v[0], v[1]
					{
						item->tri[0][0] = v[2][0];
						item->tri[0][1] = v[2][1];
						item->tri[0][2] = v[2][2];

						item->tri[1][0] = v[0][0];
						item->tri[1][1] = v[0][1];
						item->tri[1][2] = v[0][2];

						item->tri[2][0] = v[1][0];
						item->tri[2][1] = v[1][1];
						item->tri[2][2] = v[1][2];

						float e1[3] = { v[0][0] - v[2][0],v[0][1] - v[2][1],v[0][2] - v[2][2] };
						float e2[3] = { v[1][0] - v[2][0],v[1][1] - v[2][1],v[1][2] - v[2][2] };
						CrossProduct(e1, e2, item->nrm);
						float nrm = 1.0f / sqrtf(
							item->nrm[0] * item->nrm[0] + 
							item->nrm[1] * item->nrm[1] + 
							item->nrm[2] * item->nrm[2]);
						item->nrm[0] *= nrm;
						item->nrm[1] *= nrm;
						item->nrm[2] *= nrm;
						item->nrm[3] = -(v[2][0] * item->nrm[0] + v[2][1] * item->nrm[1] + v[2][2] * item->nrm[2]);

						item++;
					}

					// v[2], v[1], v[3]
					{
						item->tri[0][0] = v[2][0];
						item->tri[0][1] = v[2][1];
						item->tri[0][2] = v[2][2];

						item->tri[1][0] = v[1][0];
						item->tri[1][1] = v[1][1];
						item->tri[1][2] = v[1][2];

						item->tri[2][0] = v[3][0];
						item->tri[2][1] = v[3][1];
						item->tri[2][2] = v[3][2];

						float e1[3] = { v[1][0] - v[2][0],v[1][1] - v[2][1],v[1][2] - v[2][2] };
						float e2[3] = { v[3][0] - v[2][0],v[3][1] - v[2][1],v[3][2] - v[2][2] };
						CrossProduct(e1, e2, item->nrm);
						float nrm = 1.0f / sqrtf(
							item->nrm[0] * item->nrm[0] +
							item->nrm[1] * item->nrm[1] +
							item->nrm[2] * item->nrm[2]);
						item->nrm[0] *= nrm;
						item->nrm[1] *= nrm;
						item->nrm[2] *= nrm;
						item->nrm[3] = -(v[2][0] * item->nrm[0] + v[2][1] * item->nrm[1] + v[2][2] * item->nrm[2]);

						item++;
					}
				}
				else
				{
					// v[0], v[3], v[2]
					{
						item->tri[0][0] = v[0][0];
						item->tri[0][1] = v[0][1];
						item->tri[0][2] = v[0][2];

						item->tri[1][0] = v[3][0];
						item->tri[1][1] = v[3][1];
						item->tri[1][2] = v[3][2];

						item->tri[2][0] = v[2][0];
						item->tri[2][1] = v[2][1];
						item->tri[2][2] = v[2][2];

						float e1[3] = { v[3][0] - v[0][0],v[3][1] - v[0][1],v[3][2] - v[0][2] };
						float e2[3] = { v[2][0] - v[0][0],v[2][1] - v[0][1],v[2][2] - v[0][2] };
						CrossProduct(e1, e2, item->nrm);
						float nrm = 1.0f / sqrtf(
							item->nrm[0] * item->nrm[0] +
							item->nrm[1] * item->nrm[1] +
							item->nrm[2] * item->nrm[2]);
						item->nrm[0] *= nrm;
						item->nrm[1] *= nrm;
						item->nrm[2] *= nrm;
						item->nrm[3] = -(v[0][0] * item->nrm[0] + v[0][1] * item->nrm[1] + v[0][2] * item->nrm[2]);

						item++;
					}

					// v[0], v[1], v[3]
					{
						item->tri[0][0] = v[0][0];
						item->tri[0][1] = v[0][1];
						item->tri[0][2] = v[0][2];

						item->tri[1][0] = v[1][0];
						item->tri[1][1] = v[1][1];
						item->tri[1][2] = v[1][2];

						item->tri[2][0] = v[3][0];
						item->tri[2][1] = v[3][1];
						item->tri[2][2] = v[3][2];

						float e1[3] = { v[1][0] - v[0][0],v[1][1] - v[0][1],v[1][2] - v[0][2] };
						float e2[3] = { v[3][0] - v[0][0],v[3][1] - v[0][1],v[3][2] - v[0][2] };
						CrossProduct(e1, e2, item->nrm);
						float nrm = 1.0f / sqrtf(
							item->nrm[0] * item->nrm[0] +
							item->nrm[1] * item->nrm[1] +
							item->nrm[2] * item->nrm[2]);
						item->nrm[0] *= nrm;
						item->nrm[1] *= nrm;
						item->nrm[2] *= nrm;
						item->nrm[3] = -(v[0][0] * item->nrm[0] + v[0][1] * item->nrm[1] + v[0][2] * item->nrm[2]);

						item++;
					}
				}

				rot >>= 1;
			}
		}
		term->soup_items += faces;
	}

	/*
	static void FaceCollision(float coords[9], uint8_t* colors, uint32_t visual, void* cookie)
	{
		TERM_LIST* term = (TERM_LIST*)cookie;

		double radius_cells = 2; // in full x-cells
		double patch_cells = 3.0 * HEIGHT_CELLS; // patch size in screen cells (zoom is 3.0)
		double world_patch = VISUAL_CELLS; // patch size in world coords
		double world_radius = radius_cells / patch_cells * world_patch;

		double height_cells = 8.0;
		// 2/3 == 1/(zoom*sin30)
		double world_height = height_cells * 2/3  / cos(30 * M_PI / 180) * HEIGHT_SCALE;

		float v0[] = { coords[0],coords[1],coords[2],1 };
		float v1[] = { coords[3],coords[4],coords[5],1 };
		float v2[] = { coords[6],coords[7],coords[8],1 };
		float tri[12];
		Product(term->col_tm, v0, tri + 0);
		Product(term->col_tm, v1, tri + 4);
		Product(term->col_tm, v2, tri + 8);

		float zs = 1.0f / HEIGHT_SCALE;

		tri[2] *= zs;
		tri[6] *= zs;
		tri[10] *= zs;

		float sphere[3][4] =
		{
			{ term->pos[0], term->pos[1], term->pos[2] * zs + 1*world_radius, world_radius },
			{ term->pos[0], term->pos[1], term->pos[2] * zs + 2*world_radius, world_radius },
			{ term->pos[0], term->pos[1], term->pos[2] * zs + 3*world_radius, world_radius },
		};

		for (int i=0; i<3; i++)
		if (SphereIntersectTriangle(sphere[i], tri + 0, tri + 4, tri + 8))
		{
			float a[3] = { tri[4] - tri[0], tri[5] - tri[1], tri[6] - tri[2] };
			float b[3] = { tri[8] - tri[0], tri[9] - tri[1], tri[10] - tri[2] };

			float n[3];
			CrossProduct(a, b, n);

			float l = 1.0f / sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);

			term->col_nrm[i][0] += n[0] * l;
			term->col_nrm[i][1] += n[1] * l;
			term->col_nrm[i][2] += n[2] * l;

			term->col_num[i]++;
		}
	}

	static void MeshCollision(Mesh* m, double tm[16], void* cookie)
	{
		TERM_LIST* term = (TERM_LIST*)cookie;
		term->col_msh++;
		for (int i=0; i<16; i++)
			term->col_tm[i] = (float)(tm[i]);
		QueryMesh(m, FaceCollision, cookie);
	}
	*/
};

// HACK: get it from editor
extern Terrain* terrain;
extern World* world;
int GetGLFont(int wh[2]);

TERM_LIST* term_head = 0;
TERM_LIST* term_tail = 0;

// SUPER_HACK LIVE VIEW
extern float pos_x, pos_y, pos_z;
extern float rot_yaw;
extern float global_lt[];
extern int probe_z;

void term_animate(TERM_LIST* term)
{
	uint64_t stamp = a3dGetTime();
	uint64_t elaps = stamp - term->stamp;
	term->stamp = stamp;
	float dt = elaps * (60.0f / 1000000.0f);

	// dt = 1;

	float xy_speed = 0.13;

	static const float radius_cells = 2; // in full x-cells
	static const float patch_cells = 3.0 * HEIGHT_CELLS; // patch size in screen cells (zoom is 3.0)
	static const float world_patch = VISUAL_CELLS; // patch size in world coords
	static const float world_radius = radius_cells / patch_cells * world_patch;

	static const float height_cells = 7.5;

	// 2/3 = 1/(zoom*sin30)
	static const float world_height = height_cells * 2 / 3 / (float)cos(30 * M_PI / 180) * HEIGHT_SCALE;

	// YAW
	{
		int da = 0;
		if (term->IsKeyDown(A3D_E))
			da--;
		if (term->IsKeyDown(A3D_Q))
			da++;

		term->yaw_vel += dt * da;

		if (term->yaw_vel > 10)
			term->yaw_vel = 10;
		else
		if (term->yaw_vel < -10)
			term->yaw_vel = -10;

		if (abs(term->yaw_vel) < 1 && !da)
			term->yaw_vel = 0;

		term->yaw += dt * 0.5f * term->yaw_vel;

		float vel_damp = powf(0.9f, dt);
		term->yaw_vel *= vel_damp;
		term->yaw_vel *= vel_damp;
	}

	// VEL & ACC
	int ix = 0, iy = 0; 
	{
		if (term->IsKeyDown(A3D_A))
			ix--;
		if (term->IsKeyDown(A3D_D))
			ix++;
		if (term->IsKeyDown(A3D_W))
			iy++;
		if (term->IsKeyDown(A3D_S))
			iy--;

		float dir[3][3] =
		{
			{315,  0 , 45},
			{270, -1 , 90},
			{225, 180, 135},
		};

		if (dir[iy + 1][ix + 1] >= 0)
			term->player_dir = dir[iy + 1][ix + 1] + term->yaw;

		if (ix || iy)
		{
			float cs = cosf(term->slope);
			float dn = 1.0 / sqrtf(ix * ix + iy * iy);
			float dx = ix * dn * cs, dy = iy * dn * cs;

			term->vel[0] += (float)(dt * (dx * cos(term->yaw * (M_PI / 180)) - dy * sin(term->yaw * (M_PI / 180))));
			term->vel[1] += (float)(dt * (dx * sin(term->yaw * (M_PI / 180)) + dy * cos(term->yaw * (M_PI / 180))));
		}

		float sqr_vel_xy = term->vel[0] * term->vel[0] + term->vel[1] * term->vel[1];
		if (sqr_vel_xy < 1 && !ix && !iy)
		{
			term->vel[0] = 0;
			term->vel[1] = 0;
			term->player_stp = -1;
		}
		else
		{
			// speed limit is 27 for air / ground and 10 for full in water
			float xy_limit = 27 - 17*(term->water - term->pos[2]) / world_height;
			if (xy_limit > 27)
				xy_limit = 27;
			if (xy_limit < 10)
				xy_limit = 10;

			if (sqr_vel_xy > xy_limit)
			{
				float n = sqrt(xy_limit / sqr_vel_xy);
				sqr_vel_xy = xy_limit;
				term->vel[0] *= n;
				term->vel[1] *= n;
			}

			if (term->player_stp < 0)
				term->player_stp = 0;

			// so 8 frame walk anim divides stp / 1024 to get frame num
			term->player_stp = (~(1 << 31))&(term->player_stp + (int)(64 * sqrt(sqr_vel_xy)));

			float vel_damp = powf(0.9f, dt);
			term->vel[0] *= vel_damp;
			term->vel[1] *= vel_damp;
		}


		// newton vs archimedes 
		float cnt = 0.6;
		float acc = (term->water - (term->pos[2] + cnt*world_height)) / (2*cnt*world_height);
		if (acc < 0-cnt)
			acc = 0-cnt;
		if (acc > 1-cnt)
			acc = 1-cnt;

		term->vel[2] += dt * acc;

//		if (term->vel[2] < -1)
//			term->vel[2] = -1;

		// water resistance
		float res = (term->water - term->pos[2]) / world_height;
		if (res < 0)
			res = 0;
		if (res > 1)
			res = 1;

		float xy_res = powf(1.0 - 0.5 * res, dt);
		float z_res = powf(1.0 - 0.1 * res, dt);

		term->vel[0] *= xy_res;
		term->vel[1] *= xy_res;
		term->vel[2] *= z_res;
	}

	// POS - troubles!
	float contact_normal_z = 0;
	{
		////////////////////
		float dx = dt * term->vel[0];
		float dy = dt * term->vel[1];

		double cx = term->pos[0] + dx * 0.5;
		double cy = term->pos[1] + dy * 0.5;
		double th = 0.1;

		double qx = fabs(dx) * 0.5 + world_radius + th;
		double qy = fabs(dy) * 0.5 + world_radius + th;

		double clip_world[4][4] =
		{
			{ 1, 0, 0, qx - cx },
			{-1, 0, 0, qx + cx },
			{ 0, 1, 0, qy - cy },
			{ 0,-1, 0, qy + cy },
		//	{ 0, 0, 1,            0 - term->pos[2] },
		//	{ 0, 0,-1, world_height + term->pos[2] }
		};

		// create triangle soup of (SoupItem):
		term->soup_items = 0;
		term->collect_mul_xy = 1.0 / world_radius;
		term->collect_mul_z = 2.0 / world_height;
		QueryWorld(world, 4, clip_world, TERM_LIST::MeshCollect, term);
		QueryTerrain(terrain, 4, clip_world, 0xAA, TERM_LIST::PatchCollect, term);

		// note: term should keep soup allocation, resize it x2 if needed

		// transform Z so our ellipsolid becomes a sphere
		// just multiply: 
		//   px,py, dx,dy, and all verts x,y coords by 1.0/horizontal_radius
		//   pz, dz and all verts z coords by 1.0/(HEIGHT_SCALE*vertical_radius)

		float sphere_pos[3] =  // set current sphere center
		{
			term->pos[0] * term->collect_mul_xy,
			term->pos[1] * term->collect_mul_xy,
			(term->pos[2] + world_height*0.5) * term->collect_mul_z,
		};

		float sphere_vel[3] = 
		{
			xy_speed * term->vel[0] * dt * term->collect_mul_xy,
			xy_speed * term->vel[1] * dt * term->collect_mul_xy,
			term->vel[2] * dt * term->collect_mul_z,
		}; // set velocity (must include gravity impact)
			
		const float xy_thresh = 0.002f;
		const float z_thresh = 0.001f;

		int items = term->soup_items;
		int iters_left = 10;
		bool ignore_roof = false;
		retry_without_roof:
		while (abs(sphere_vel[0])>xy_thresh || abs(sphere_vel[1]) > xy_thresh || abs(sphere_vel[2]) > z_thresh)
		{
			SoupItem* collision_item = 0;
			float collision_time = 2.0f; // (greater than current velocity range)
			float collision_pos[3];

			for (int i=0; i<items; i++)
			{
				SoupItem* item = term->soup + i;

				float contact_pos[3];
				float time = item->CheckCollision(sphere_pos, sphere_vel, contact_pos); // must return >=2 if no collision occurs

				if (time < collision_time)
				{
					if (!ignore_roof || contact_pos[2] < sphere_pos[2] + 0.5)
					{
						collision_item = item;
						collision_time = time;
						collision_pos[0] = contact_pos[0];
						collision_pos[1] = contact_pos[1];
						collision_pos[2] = contact_pos[2];
					}
				}
			}

			if (!collision_item)
			{
				sphere_pos[0] += sphere_vel[0];
				sphere_pos[1] += sphere_vel[1];
				sphere_pos[2] += sphere_vel[2];
				break;
			}

			// collision_time = max(0, collision_time - 0.001f); // fix to prevent intersections with current sliding triangle plane
			collision_time -= 0.001f;
					
			sphere_pos[0] += sphere_vel[0] * collision_time;
			sphere_pos[1] += sphere_vel[1] * collision_time;
			sphere_pos[2] += sphere_vel[2] * collision_time;

			float remain = 1.0f - collision_time;
			sphere_vel[0] *= remain;
			sphere_vel[1] *= remain;
			sphere_vel[2] *= remain;

			float slide_normal[3] =
			{
				sphere_pos[0] - collision_pos[0],
				sphere_pos[1] - collision_pos[1],
				sphere_pos[2] - collision_pos[2]
			};

			// move bit away from plane
			/*
			sphere_pos[0] += slide_normal[0] * 0.001;
			sphere_pos[1] += slide_normal[1] * 0.001;
			sphere_pos[2] += slide_normal[2] * 0.001;
			*/

			float project = DotProduct(sphere_vel, slide_normal);
			sphere_vel[0] -= slide_normal[0] * project;
			sphere_vel[1] -= slide_normal[1] * project;
			sphere_vel[2] -= slide_normal[2] * project;

			contact_normal_z = max(contact_normal_z, slide_normal[2]);

			if (!--iters_left)
				break;
		}

		if (iters_left)
			term->collision_failure = false;
		else
		if (!term->collision_failure && !ix && !iy && contact_normal_z>0)
		{
			// something's wrong
			// relax - ignore collisions from top
			term->collision_failure = true;
			printf("CRITICAL! move to resolve\n");
		}

		if (term->collision_failure && !ignore_roof)
		{
			ignore_roof = true;
			goto retry_without_roof;
		}

		printf("iters_left:%d\n", iters_left);

		// convert back to world coords
		// just multiply: 
		//   px,py by horizontal_radius
		//   and pz by (HEIGHT_SCALE*vertical_radius)

		// we are done, update 

		float pos[3] =
		{
			sphere_pos[0] / term->collect_mul_xy,
			sphere_pos[1] / term->collect_mul_xy,
			sphere_pos[2] / term->collect_mul_z - world_height * 0.5f
		};

		/*

		float vel[3] =
		{
			(pos[0] - term->pos[0]) / dt,
			(pos[1] - term->pos[1]) / dt,
			(pos[2] - term->pos[2]) / (dt*HEIGHT_SCALE)
		};

		float org_vel[3] =
		{
			term->vel[0],
			term->vel[1],
			term->vel[2]
		};

		term->vel[0] *= xy_speed;
		term->vel[1] *= xy_speed;
		term->vel[2] /= HEIGHT_SCALE;
		float vn = sqrtf(term->vel[0] * term->vel[0] + term->vel[1] * term->vel[1] + term->vel[2] * term->vel[2]);

		if (vn > 0.001)
		{


			vn = 1.0 / vn;
			// leave direction only
			term->vel[0] *= vn;
			term->vel[1] *= vn;
			term->vel[2] *= vn;

			// project
			vn = DotProduct(term->vel, vel);

			// apply magnitude from poisition offs
			term->vel[0] *= vn / xy_speed;
			term->vel[1] *= vn / xy_speed;
			term->vel[2] *= vn * HEIGHT_SCALE;

			printf("iters_left:%d, in: %f,%f out: %f,%f\n", iters_left, org_vel[0], org_vel[1], term->vel[0], term->vel[1]);

			// average org and new
			term->vel[0] = term->vel[0] * 0.0 + org_vel[0] * 1.0;
			term->vel[1] = term->vel[1] * 0.0 + org_vel[1] * 1.0;
			term->vel[2] = term->vel[2] * 0.0 + org_vel[2] * 1.0;
		}
		else
		{
			term->vel[0] = 0;
			term->vel[1] = 0;
			term->vel[2] = 0;
		}
		*/

		float org_vel[3] =
		{
			term->vel[0],
			term->vel[1],
			term->vel[2]
		};

		term->vel[0] = (pos[0] - term->pos[0]) / (xy_speed * dt);
		term->vel[1] = (pos[1] - term->pos[1]) / (xy_speed * dt);
		term->vel[2] = (pos[2] - term->pos[2]) / dt;

		float adz = fmaxf(0,term->vel[2]) / HEIGHT_SCALE * 4;
		float adxy = xy_speed * sqrtf(term->vel[0] * term->vel[0] + term->vel[1] * term->vel[1]);
		term->slope = atan2(adz, adxy);

		// printf("iters_left:%d, in: %f,%f out: %f,%f\n", iters_left, org_vel[0], org_vel[1], term->vel[0], term->vel[1]);

		// slippery threshold?
		// use org (no-slippery) use new (full slippery)

		term->vel[0] = 1.0 * term->vel[0] + org_vel[0] * 0.0;
		term->vel[1] = 1.0 * term->vel[1] + org_vel[1] * 0.0;
		term->vel[2] = 1.0 * term->vel[2] + org_vel[2] * 0.0;

		// printf("contact_normal_z:%f, vel[0]: %f, vel[1]:%f, vel[2]:%f\n", contact_normal_z,fabsf(term->vel[0]),fabsf(term->vel[1]),fabsf(term->vel[2]));

		if (ix || iy || contact_normal_z <= 0.0 || fabsf(term->vel[0]) > 0.1 || fabsf(term->vel[1]) > 0.1 || fabsf(term->vel[2]) > 0.1*16)
		{
			term->pos[0] = pos[0];
			term->pos[1] = pos[1];
			term->pos[2] = pos[2];
		}
		else
		{
			term->vel[0] = 0;
			term->vel[1] = 0;
			term->vel[2] = 0;
		}
	}

	// jump
	if (contact_normal_z > 0.0)
	{
		if (term->IsKeyDown(A3D_SPACE))
		{
			term->vel[2]+= 10;
			term->keys[A3D_SPACE >> 3] &= ~(1 << (A3D_SPACE & 7));
		}
	} 


	// OLD POS
	// after updating x,y,z by time and keyb bits
	// we need to fix z so player doesn't penetrate terrain
	/*
	{
		double p[3] = { term->pos[0],term->pos[1],-1 };
		double v[3] = { 0,0,-1 };
		double r[4] = { 0,0,0,1 };
		double n[3];
		Patch* patch = HitTerrain(terrain, p, v, r, n);

		double r2[4] = { 0,0,0,1 };
		double n2[4];

		{
			// it's almost ok, but need to exclude all meshes hanging above player
			// so initialize p[2] to plyer's center.z (instead of -1)
			double height_cells = 8.0;

			// 2/3 = 1.0/(zoom*sin30)
			double world_height = height_cells * 2/3 / cos(30 * M_PI / 180) * HEIGHT_SCALE;
			p[2] = term->pos[2] + world_height*0.5;
		}

		Inst* inst = HitWorld(world, p, v, r2, n2, true); // true = positive only

		if (inst && (!patch || patch && r2[2] > r[2]))
		{
			n[0] = n2[0];
			n[1] = n2[1];
			n[2] = n2[2];
			r[2] = r2[2];
			patch = 0;
		}

		if (inst || patch)
		{
			if (!patch && r[2] - term->pos[2] > 48) // do pasa
			{
				// nie pozwalamy na wslizg
				term->pos[0] = push[0];
				term->pos[1] = push[1];
				term->pos[2] = push[2];
				term->vel[0] = 0;
				term->vel[1] = 0;
				term->vel[2] = 0;
				return;
			}


			// we need contact to jump
			if (term->IsKeyDown(A3D_SPACE) && r[2] - term->pos[2] > -16)
				term->vel[2] = 10;
			else
				term->keys[A3D_SPACE >> 3] &= ~(1 << (A3D_SPACE & 7));

			if (r[2] >= term->pos[2])
			{
				double n_len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
				term->player_slope = min(0.1,n[2] / n_len);
				term->pos[2] = r[2];
				if (term->vel[2]<0)
					term->vel[2] = 0;
			}
			else
				term->player_slope = 0.1;
		}
		else
		{
			// we need contact to jump
			if (term->IsKeyDown(A3D_SPACE) && term->pos[2] < 16)
				term->vel[2] = 10;
			else
				term->keys[A3D_SPACE >> 3] &= ~(1 << (A3D_SPACE & 7));

			term->player_slope = 0.1;
			if (term->pos[2] < 0)
				term->pos[2] = 0;
		}
	}
	*/
}

void term_render(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	// FPS DUMPER
	{
		static int frames = 0;
		frames++;
		static uint64_t p = a3dGetTime();
		uint64_t t = a3dGetTime();
		uint64_t d = t - p;
		if (d > 1000000)
		{
			double fps = 1000000.0 * frames / (double)d;
			printf("fps = %.2f, x = %.2f, y = %.2f, z = %.2f\n", fps, term->pos[0],term->pos[1],term->pos[2]);
			p = t;
			frames = 0;
		}
	}

	term->water = probe_z; // hack
	term_animate(term);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	int wnd_wh[2];

	a3dGetRect(wnd, 0, wnd_wh);

	int fnt_wh[2];
	int fnt_tex = GetGLFont(fnt_wh);

	int width = wnd_wh[0] / (fnt_wh[0] >> 4);
	int height = wnd_wh[1] / (fnt_wh[1] >> 4);

	if (width > term->max_width)
		width = term->max_width;
	if (height > term->max_height)
		height = term->max_height;

	char utf8[64];
	sprintf(utf8, "ASCIIID Term %d x %d", width, height);
	a3dSetTitle(wnd, utf8);

	int vp_wh[2] =
	{
		width * (fnt_wh[0] >> 4),
		height * (fnt_wh[1] >> 4)
	};

	int vp_xy[2]=
	{
		(wnd_wh[0] - vp_wh[0]) / 2,
		(wnd_wh[1] - vp_wh[1]) / 2
	};

	float zoom = 1.0;
	//Render(terrain, world, term->water, zoom, term->yaw, term->pos, width, height, term->buf);

	/*
	float pos[3] = { pos_x, pos_y, pos_z };
	float yaw = rot_yaw;
	*/

	float ln = 1.0f/sqrtf(global_lt[0] * global_lt[0] + global_lt[1] * global_lt[1] + global_lt[2] * global_lt[2]);
	float lt[4] =
	{
		global_lt[0] * ln,
		global_lt[1] * ln,
		global_lt[2] * ln,
		global_lt[3]
	};

	// local light override
	/*
	lt[0] = cos((yaw-90) * M_PI / 180);
	lt[1] = sin((yaw-90) * M_PI / 180);
	lt[2] = 0.5;
	ln = 1.0f / sqrtf(lt[0] * lt[0] + lt[1] * lt[1] + lt[2] * lt[2]);
	lt[0] *= ln;
	lt[1] *= ln;
	lt[2] *= ln;
	*/

	float pos_cpy[3] = { term->pos[0],term->pos[1],term->pos[2] };
	Render(terrain, world, (float)probe_z/*term->water*/, zoom, term->yaw, pos_cpy, lt, width, height, term->buf, term->player_dir, term->player_stp);

	// copy term->buf to some texture
	glTextureSubImage2D(term->tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, term->buf);

	glViewport(vp_xy[0], vp_xy[1], vp_wh[0], vp_wh[1]);

	glUseProgram(term->prg);

	glUniform2i(0, width, height);

	glUniform1i(1, 0);
	glBindTextureUnit(0, term->tex);

	glUniform1i(2, 1);
	glBindTextureUnit(1, fnt_tex);

	glUniform2i(3, term->max_width, term->max_height);
	
	glBindVertexArray(term->vao);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUseProgram(0);
	glBindVertexArray(0);

	glBindTextureUnit(0, 0);
	glBindTextureUnit(1, 0);
}

void term_mouse(A3D_WND* wnd, int x, int y, MouseInfo mi)
{
	if (mi == LEFT_DN)
	{
		// get heigth from frame
		// get highest of 4 samples at mouse coord
		float z = 0;

		// transform screen_x, screen_y, z back to worldspace
		
		// store goto xy info handled by animate()
		
		// todo later:
		// calc path at some resolution to avoid obstacles


		// should be cleared on any WSAD key down

	}
}

void term_resize(A3D_WND* wnd, int w, int h)
{
}

void term_init(A3D_WND* wnd)
{
	/*
	SoupItem it;
	it.tri[0][0] = 0;
	it.tri[0][1] = 10;
	it.tri[0][2] = 0;

	it.tri[1][0] = 0;
	it.tri[1][1] = 0;
	it.tri[1][2] = 10;

	it.tri[2][0] = 10;
	it.tri[2][1] = 0;
	it.tri[2][2] = 0;

	it.nrm[0] = 1 / sqrtf(3);
	it.nrm[1] = 1 / sqrtf(3);
	it.nrm[2] = 1 / sqrtf(3);
	it.nrm[3] = -10 / sqrtf(3);

	float pos[3] = { .5,.5,10 };
	float vel[3] = { 0,0,-10 };
	float col[3];
	it.CheckCollision(pos, vel, col);
	*/


	TERM_LIST* term = (TERM_LIST*)malloc(sizeof(TERM_LIST));
	term->wnd = wnd;

	term->stamp = a3dGetTime();

	term->collision_failure = false;

	term->soup = 0;
	term->soup_alloc = 0;
	term->soup_items = 0;

	term->yaw = rot_yaw;
	term->yaw_vel = 0;

	term->water = (float)probe_z;
	term->pos[0] = pos_x;
	term->pos[1] = pos_y;
	term->pos[2] = pos_z;
	term->vel[0] = 0;
	term->vel[1] = 0;
	term->vel[2] = 0;

	// todo:
	// check safe initial position so it won't intersect with anything!!!
	{
		double p[3] = { term->pos[0],term->pos[1],-1 };
		double v[3] = { 0,0,-1 };
		double r[4] = { 0,0,0,1 };
		double n[3];
		Patch* patch = HitTerrain(terrain, p, v, r, n);

		if (patch)
			term->pos[2] = (float)r[2] + 200; // assuming no mesh stands above terrain by more than 200 units
	}

	term->slope = 0;
	term->player_dir = 0;
	term->player_stp = -1;

	int loglen = 999;
	char logstr[1000] = "";
	GLuint shader[2] = { 0,0 };

	glCreateTextures(GL_TEXTURE_2D, 1, &term->tex);
	glTextureStorage2D(term->tex, 1, GL_RGBA8, term->max_width, term->max_height);

	const char* term_vs_src =
		CODE(#version 450\n)
		CODE(
			layout(location = 0) uniform ivec2 ansi_vp;  // viewport size in cells
			layout(location = 0) in vec2 uv; // normalized to viewport size
			out vec2 cell_coord;
			void main()
			{
				gl_Position = vec4(2.0*uv - vec2(1.0), 0.0, 1.0);
				cell_coord = uv * ansi_vp;
			}
		);

	const char* term_fs_src =
		CODE(#version 450\n)
		DEFN(P(r, g, b), vec3(r / 6., g / 7., b / 6.))
		CODE(

			layout(location = 0) out vec4 color;
			layout(location = 1) uniform sampler2D ansi;
			layout(location = 2) uniform sampler2D font;
			layout(location = 3) uniform ivec2 ansi_wh;  // ansi texture size (in cells), constant = 160x90
			in vec2 cell_coord;

			vec3 XTermPal(int p)
			{
				p -= 16;
				if (p < 0 || p >= 216)
					return vec3(0, 0, 0);

				int r = p % 6;
				p = (p - r) / 6;
				int g = p % 6;
				p = (p - g) / 6;
				
				return vec3(r, g, p) * 0.2;
			}

			void main()
			{
				// sample ansi buffer
				vec2 quot_cell = floor(cell_coord);
				vec2 frac_cell = fract(cell_coord);

				vec2 ansi_coord = (quot_cell + vec2(0.5)) / ansi_wh;

				vec4 cell = texture(ansi, ansi_coord);

				int glyph_idx = int(round(cell.b * 255.0));

				frac_cell.y = 1.0 - frac_cell.y;
				vec2 glyph_coord = ( vec2(glyph_idx & 0xF, glyph_idx >> 4) + frac_cell ) / vec2(16.0);
				float glyph_alpha = texture(font, glyph_coord).a;

				vec3 fg_color = XTermPal(int(round(cell.r * 255.0)));
				vec3 bg_color = XTermPal(int(round(cell.g * 255.0)));

				color = vec4(mix(bg_color, fg_color, glyph_alpha), 1.0);
			}
		);

	GLenum term_st[3] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	const char* term_src[3] = { term_vs_src, term_fs_src };
	GLuint term_prg = glCreateProgram();

	for (int i = 0; i < 2; i++)
	{
		shader[i] = glCreateShader(term_st[i]);
		GLint len = (GLint)strlen(term_src[i]);
		glShaderSource(shader[i], 1, &(term_src[i]), &len);
		glCompileShader(shader[i]);

		loglen = 999;
		glGetShaderInfoLog(shader[i], loglen, &loglen, logstr);
		logstr[loglen] = 0;

		if (loglen)
			printf("%s", logstr);

		glAttachShader(term_prg, shader[i]);
	}

	glLinkProgram(term_prg);

	for (int i = 0; i < 2; i++)
		glDeleteShader(shader[i]);

	loglen = 999;
	glGetProgramInfoLog(term_prg, loglen, &loglen, logstr);
	logstr[loglen] = 0;

	if (loglen)
		printf("%s", logstr);

	term->prg = term_prg;

	float vbo_data[] = {0,0, 1,0, 1,1, 0,1};

	GLuint term_vbo;
	glCreateBuffers(1, &term_vbo);
	glNamedBufferStorage(term_vbo, 4 * sizeof(float[2]), 0, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(term_vbo, 0, 4 * sizeof(float[2]), vbo_data);

	term->vbo = term_vbo;

	GLuint term_vao;
	glCreateVertexArrays(1, &term_vao);
	glBindVertexArray(term_vao);
	glBindBuffer(GL_ARRAY_BUFFER, term_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float[2]), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	term->vao = term_vao;

	term->prev = term_tail;
	term->next = 0;
	if (term_tail)
		term_tail->next = term;
	else
		term_head = term;
	term_tail = term;

	a3dSetCookie(wnd, term);
	a3dSetIcon(wnd, "./icons/app.png");
	a3dSetVisible(wnd, true);
}

void term_keyb_char(A3D_WND* wnd, wchar_t chr)
{
}

void term_keyb_key(A3D_WND* wnd, KeyInfo ki, bool down)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	if (ki&A3D_AUTO_REPEAT)
		return;

	if (down)
		term->keys[ki >> 3] |= 1 << (ki & 0x7);
	else
		term->keys[ki >> 3] &= ~(1 << (ki & 0x7));
}

void term_keyb_focus(A3D_WND* wnd, bool set)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);
	memset(term->keys, 0, 32);
}

void term_close(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	if (term->soup)
		free(term->soup);

	glDeleteTextures(1, &term->tex);
	glDeleteVertexArrays(1, &term->vao);
	glDeleteBuffers(1, &term->vbo);
	glDeleteProgram(term->prg);

	a3dClose(wnd);

	if (term->prev)
		term->prev->next = term->next;
	else
		term_head = term->next;

	if (term->next)
		term->next->prev = term->prev;
	else
		term_tail = term->prev;

	free(term);
}

void TermOpen(A3D_WND* share, float yaw, float pos[3])
{
	PlatformInterface pi;
	pi.close = term_close;
	pi.render = term_render;
	pi.resize = term_resize;
	pi.init = term_init;
	pi.keyb_char = term_keyb_char;
	pi.keyb_key = term_keyb_key;
	pi.keyb_focus = term_keyb_focus;
	pi.mouse = term_mouse;

	// pi.ptydata = my_ptydata;

	GraphicsDesc gd;
	gd.color_bits = 32;
	gd.alpha_bits = 8;
	gd.depth_bits = 0;
	gd.stencil_bits = 0;
	gd.flags = (GraphicsDesc::FLAGS) (GraphicsDesc::DEBUG_CONTEXT | GraphicsDesc::DOUBLE_BUFFER);

	int rc[] = { 0,0,1920 * 2,1080 + 2 * 1080 };
	gd.wnd_mode = A3D_WND_NORMAL;
	gd.wnd_xywh = 0;

	A3D_WND* wnd = a3dOpen(&pi, &gd, share);

	//a3dSetRect(wnd, 0, A3D_WND_FULLSCREEN);
	//a3dSetVisible(share, false);

	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	/*
	term->yaw = yaw;
	term->pos[0] = pos[0];
	term->pos[1] = pos[1];
	term->pos[2] = pos[2];
	term->water = 0;
	*/
}

void TermCloseAll()
{
	TERM_LIST* term = term_head;
	while (term)
	{
		TERM_LIST* next = term->next;
		a3dClose(term->wnd);
		free(term);
		term = next;
	}

	term_head = 0;
	term_tail = 0;
}

