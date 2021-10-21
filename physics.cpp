
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "matrix.h"
#include "physics.h"
#include "audio.h"
#include "game.h" // SpriteReq for audio props

struct SoupItem
{
	float tri[3][3];
	int material; // for audio :)
	float nrm[4]; // {nrm, w} is plane equ

#if 0
	float CheckCollision2(const float sphere_pos[3], const float sphere_vel[3], float contact_pos[3])
	{
		float col_time = 2;

		float col[3] = // on sphere @ time 0.0
		{
			sphere_pos[0] - nrm[0],
			sphere_pos[1] - nrm[1],
			sphere_pos[2] - nrm[2]
		};

		float face = DotProduct(col, nrm) + nrm[3];
		float closer = -DotProduct(sphere_vel, nrm);

		// float contact_pos[3]; // collision point @ collision time

		if (face <= 0)
		{
			// back: project collision point along plane normal
			/*
			(col[0] + nrm[0] * t) * nrm[0] +
			(col[1] + nrm[1] * t) * nrm[1] +
			(col[2] + nrm[2] * t) * nrm[2] + nrm[3] == 0
			*/

			// DotProduct(col,nrm) + t * DotProduct(nrm, nrm) + nrm[3] == 0
			// DotProduct(col,nrm) + t + nrm[3] == 0

			if (closer > 0)
			{
				/*
				contact_pos[0] = col[0] - face * nrm[0];
				contact_pos[1] = col[1] - face * nrm[1];
				contact_pos[2] = col[2] - face * nrm[2];
				*/
				contact_pos[0] = col[0];
				contact_pos[1] = col[1];
				contact_pos[2] = col[2];
				col_time = 0;
			}
			else
			{
				// allow to resolve
				col_time = 2;
			}
		}
		else
		{
			if (closer > 0)
			{
				// front: project collision point along velocity
				/*
				(col[0] + sphere_vel[0] * t) * nrm[0] +
				(col[1] + sphere_vel[1] * t) * nrm[1] +
				(col[2] + sphere_vel[2] * t) * nrm[2] + nrm[3] == 0
				*/

				// DotProduct(col,nrm) + t * DotProduct(sphere_vel, nrm) + nrm[3] == 0

				col_time = face / closer;

				contact_pos[0] = col[0] + col_time * sphere_vel[0];
				contact_pos[1] = col[1] + col_time * sphere_vel[1];
				contact_pos[2] = col[2] + col_time * sphere_vel[2];
			}
			else
			{
				// no hit in the past
				col_time = 2;
			}
		}

		if (col_time > 1)
			return col_time;

		// if contact_pos is in not inside triangle
		// change it to be nearest point on triangle

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
			// at the moment it can happen (no safe distance is maintained)
			//assert(col_time > 0);
			return col_time;
		}

		//return 2;

		// find closest point on edges (clamped to vertices)
		float closest[3];
		float closest_sd = 1.e+20f;

		for (int e = 0; e < 3; e++)
		{
			float sqr_len = edge[e][0] * edge[e][0] + edge[e][1] * edge[e][1] + edge[e][2] * edge[e][2];

			float line = DotProduct(vect[e], edge[e]);
			if (line <= 0)
			{
				float d[3] = 
				{
					contact_pos[0] - tri[e][0],
					contact_pos[1] - tri[e][1],
					contact_pos[2] - tri[e][2],
				};

				float sd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
				if (sd < closest_sd)
				{
					closest_sd = sd;
					closest[0] = tri[e][0];
					closest[1] = tri[e][1];
					closest[2] = tri[e][2];
				}
			}
			else
			if (line >= sqr_len)
			{
				int f = e==2 ? 0 : e + 1;

				float d[3] =
				{
					contact_pos[0] - tri[f][0],
					contact_pos[1] - tri[f][1],
					contact_pos[2] - tri[f][2],
				};

				float sd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
				if (sd < closest_sd)
				{
					closest_sd = sd;
					closest[0] = tri[f][0];
					closest[1] = tri[f][1];
					closest[2] = tri[f][2];
				}
			}
			else
			{
				line /= sqr_len;
				float p[3] =
				{
					tri[e][0] + line * edge[e][0],
					tri[e][1] + line * edge[e][1],
					tri[e][2] + line * edge[e][2]
				};

				float d[3] =
				{
					contact_pos[0] - p[0],
					contact_pos[1] - p[1],
					contact_pos[2] - p[2],
				};

				float sd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
				if (sd < closest_sd)
				{
					closest_sd = sd;
					closest[0] = p[0];
					closest[1] = p[1];
					closest[2] = p[2];
				}
			}
		}

		assert(closest_sd < 1.e+19f);

		contact_pos[0] = closest[0];
		contact_pos[1] = closest[1];
		contact_pos[2] = closest[2];

		// reproject contact pos onto sphere in -vel direction

		/*
			( p+v*t - ps )^2 = r^2

			( p-ps + v*t )^2 = r^2
			(p-ps)^2 + t*2*(p-ps)*v + t^2*v^2 = r^2

			A := v^2
			B := 2*dot(p-ps,v)
			C := (p-ps)^2 - r^2
		*/

		float p_ps[3] =
		{
			contact_pos[0] - sphere_pos[0],
			contact_pos[1] - sphere_pos[1],
			contact_pos[2] - sphere_pos[2],
		};

		float A = sphere_vel[0] * sphere_vel[0] + sphere_vel[1] * sphere_vel[1] + sphere_vel[2] * sphere_vel[2];
		float B = -2 * DotProduct(p_ps, sphere_vel); // negative as we project in -vel dir
		float C = DotProduct(p_ps, p_ps) - 1.0f;

		float D = B * B - 4 * A*C;

		if (D < 0)
			col_time = 2;
		else
		{
			col_time = (-B - sqrtf(D)) / (2 * A);
			if (col_time < 0)
				col_time = 2;
		}

		return col_time;
	}
#endif

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

struct Physics
{
    uint64_t stamp;

	SoupItem* soup;
	int soup_alloc;
	int soup_items;

	double* collect_tm;
	float collect_mul_xy;
	float collect_mul_z;

	int mat;

	float max_height;

    //bool collision_failure;

    float slope;
    float water;
    float player_dir;
    int player_stp;
    float yaw_vel;
    float yaw;
    float vel[3];
    float pos[3];

	float accum_contact;

    Terrain* terrain;
    World* world;

	static void FaceCollect(float coords[9], uint8_t* colors, uint32_t visual, void* cookie)
	{
		if (visual&(1 << 31)) // skip lines (but could be worked out to collide lines too!)
			return;
		if (colors[3] > 128 || colors[7] > 128 || colors[11] > 128) // skip leafs
			return;

		Physics* phys = (Physics*)cookie;
		SoupItem* item = phys->soup + phys->soup_items;

		// check face color
		// greenish, greish, yellowish, redish

		int rgb[3]= // 0..765
		{
			colors[0]+colors[4]+colors[8],
			colors[1]+colors[5]+colors[9],
			colors[2]+colors[6]+colors[10]
		};

		int sat=0,lum=0,mat=3;
		if (rgb[1]>=rgb[2] && rgb[1]>=rgb[0])
		{
			// yellow - cyan
			lum = rgb[1];
			mat=3; // green
			if (rgb[0]>rgb[2])
			{
				// yellow-green
				sat = rgb[0]-rgb[2];
			}
			else
			{
				// green-cyan
				sat = rgb[2]-rgb[0];
			}
		}
		else
		if (rgb[0]>=rgb[1] && rgb[0]>=rgb[2])
		{
			// magenta-yellow
			lum = rgb[0];
			mat=1; // wood

			if (rgb[1]>rgb[2])
			{
				// red-yellow
				sat = rgb[1]-rgb[2];
			}
			else
			{
				// magenta-red
				sat = rgb[2]-rgb[1];
			}
		}
		else
		//if (rgb[2]>=rgb[0] && rgb[2]>=rgb[1])
		{
			// cyan-magenta
			lum = rgb[2];
			mat=1; // wood

			if (rgb[0]>rgb[1])
			{
				// blue-magenta
				sat = rgb[0]-rgb[1];
			}
			else
			{
				// cyan-blue
				sat = rgb[1]-rgb[0];
			}
		}

		if (sat*10<lum)
		{
			mat = 0; // rock
		}

		item->material = mat;

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

		Product(phys->collect_tm, v[0], tmv);
		phys->max_height = fmaxf(tmv[2], phys->max_height);

		item->tri[0][0] = tmv[0] * phys->collect_mul_xy;
		item->tri[0][1] = tmv[1] * phys->collect_mul_xy;
		item->tri[0][2] = tmv[2] * phys->collect_mul_z;

		Product(phys->collect_tm, v[1], tmv);
		phys->max_height = fmaxf(tmv[2], phys->max_height);

		item->tri[1][0] = tmv[0] * phys->collect_mul_xy;
		item->tri[1][1] = tmv[1] * phys->collect_mul_xy;
		item->tri[1][2] = tmv[2] * phys->collect_mul_z;

		Product(phys->collect_tm, v[2], tmv);
		phys->max_height = fmaxf(tmv[2], phys->max_height);

		item->tri[2][0] = tmv[0] * phys->collect_mul_xy;
		item->tri[2][1] = tmv[1] * phys->collect_mul_xy;
		item->tri[2][2] = tmv[2] * phys->collect_mul_z;



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

		phys->soup_items ++;
	}

	static void SpriteCollect(Inst* inst, Sprite* s, float pos[3], float yaw, int anim, int frame, int reps[4], void* cookie)
	{
		// no collisions with sprites at the moment
	}

	static void MeshCollect(Mesh* m, double tm[16], void* cookie)
	{
		Physics* phys = (Physics*)cookie;

		int faces = GetMeshFaces(m);
		if (phys->soup_alloc < phys->soup_items + faces)
		{
			phys->soup_alloc = 1414 * phys->soup_alloc / 1000 + faces;
			phys->soup = (SoupItem*)realloc(phys->soup, sizeof(SoupItem) * phys->soup_alloc);
		}

		phys->collect_tm = tm;

		QueryMesh(m, FaceCollect, cookie);
	}
	
	static void PatchCollect(Patch* p, int x, int y, int view_flags, void* cookie)
	{
		Physics* phys = (Physics*)cookie;

		int faces = 2 * HEIGHT_CELLS*HEIGHT_CELLS;

		if (phys->soup_alloc < phys->soup_items + faces)
		{
			phys->soup_alloc = 1414 * phys->soup_alloc / 1000 + faces;
			phys->soup = (SoupItem*)realloc(phys->soup, sizeof(SoupItem) * phys->soup_alloc);
		}

		SoupItem* item = phys->soup + phys->soup_items;
		uint16_t diag = GetTerrainDiag(p);
		uint16_t* hmap = GetTerrainHeightMap(p);

		uint16_t* vmap = GetTerrainVisualMap(p);

		static const double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;
		bool hit = false;

		int rot = GetTerrainDiag(p);

		float hi = GetTerrainHi(p);
		phys->max_height = fmaxf(hi, phys->max_height);

		for (int hy = 0; hy < HEIGHT_CELLS; hy++)
		{
			for (int hx = 0; hx < HEIGHT_CELLS; hx++)
			{
				uint16_t vis = *vmap;
				int elv = vis>>15;
				int mat = vis&0x3F;
				vmap+=2; // 2x visual cells / height cell

				if (mat == 4)
					mat = 0; // rock
				else
				if (mat == 5)
					mat = 5; // blood
				else
				if (mat != 2) // else 2->2 (dirt)
					mat = 3+elv; // [hi]grass

				float x0 = (x + hx * sxy) * phys->collect_mul_xy, x1 = x0 + sxy * phys->collect_mul_xy;
				float y0 = (y + hy * sxy) * phys->collect_mul_xy, y1 = y0 + sxy * phys->collect_mul_xy;

				float v[4][3] =
				{
					{x0,y0,(float)hmap[hy*(HEIGHT_CELLS+1) + hx] * phys->collect_mul_z},
					{x1,y0,(float)hmap[hy*(HEIGHT_CELLS + 1) + hx + 1] * phys->collect_mul_z},
					{x0,y1,(float)hmap[(hy + 1)*(HEIGHT_CELLS + 1) + hx] * phys->collect_mul_z},
					{x1,y1,(float)hmap[(hy + 1)*(HEIGHT_CELLS + 1) + hx + 1] * phys->collect_mul_z},
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

						assert(fabsf(item->nrm[0]) + fabsf(item->nrm[1]) + fabsf(item->nrm[2]) > 0.001);

						float nrm = 1.0f / sqrtf(
							item->nrm[0] * item->nrm[0] + 
							item->nrm[1] * item->nrm[1] + 
							item->nrm[2] * item->nrm[2]);

						item->nrm[0] *= nrm;
						item->nrm[1] *= nrm;
						item->nrm[2] *= nrm;
						item->nrm[3] = -(v[2][0] * item->nrm[0] + v[2][1] * item->nrm[1] + v[2][2] * item->nrm[2]);

						item->material = mat;
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

						item->material = mat;
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

						item->material = mat;
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

						item->material = mat; 
						item++;
					}
				}

				rot >>= 1;
			}

			vmap+=VISUAL_CELLS; // 2x visual cells / height cell
		}
		phys->soup_items += faces;
	}    
};

int Animate(Physics* phys, uint64_t stamp, PhysicsIO* io, const SpriteReq* req, bool me)
{
	float xy_speed = 0.13;
	float radius_cells = req->mount ? 3 : 2; // in full x-cells
	float patch_cells = 3.0 * HEIGHT_CELLS; // patch size in screen cells (zoom is 3.0)
	float world_patch = VISUAL_CELLS; // patch size in world coords
	float world_radius = radius_cells / patch_cells * world_patch;
	float height_cells = req->mount ? 9.0 : 7.0; // 7.5; decreased (hair are soft)

	// 2/3 = 1/(zoom*sin30)
	static const float world_height = height_cells * 2 / 3 / (float)cos(30 * M_PI / 180) * HEIGHT_SCALE;

	static const int interval = 15000; // update physics step in [us]

	io->dt = stamp - phys->stamp;
	if (io->dt > 500000)
	{
		// stall
		io->dt = 0;
		phys->stamp = stamp;
	}

	int steps_handled = 0;

	while (stamp - phys->stamp >= interval) // 5ms physics steps ( 200 steps/sec )
	{
		steps_handled++;

		uint64_t elaps = stamp - phys->stamp;
		if (elaps > interval)
			elaps = interval;
		phys->stamp += elaps;
		float dt = elaps * (60.0f / 1000000.0f); 


		const int step_offs = 3*1024;
		const int step_mask = (8*1024-1);
		int prev_step;
		float xy_vel;
		float in_water;

		// by having old and new water level we can (in future) keep player floating on top of waves 
		phys->water = io->water;

		// YAW
		{
			if (io->torque >= 1000000)
			{
				phys->yaw = io->yaw;
				phys->yaw_vel = 0;
				phys->yaw_vel = 0;
			}
			else
			{
				int da = 0;
				if (io->torque < 0)
					da--;
				if (io->torque > 0)
					da++;

				phys->yaw_vel += dt * io->torque; //da;

				if (phys->yaw_vel > 10)
					phys->yaw_vel = 10;
				else
					if (phys->yaw_vel < -10)
						phys->yaw_vel = -10;

				if (fabsf(phys->yaw_vel) < 1 && !da)
					phys->yaw_vel = 0;

				phys->yaw += dt * 0.5f * phys->yaw_vel;

				float vel_damp = powf(0.9f, dt);
				phys->yaw_vel *= vel_damp;
				phys->yaw_vel *= vel_damp;
			}
		}

		// VEL & ACC
		float xy_len = sqrtf(io->x_force * io->x_force + io->y_force * io->y_force);

		int ix = 0, iy = 0;
		{
			if (io->x_force < 0)
				ix--;
			if (io->x_force > 0)
				ix++;
			if (io->y_force > 0)
				iy++;
			if (io->y_force < 0)
				iy--;

			/*
			float dir[3][3] =
			{
				{315,  0 , 45},
				{270, -1 , 90},
				{225, 180, 135},
			};
			*/

			float dx, dy;
			if (xy_len < 0.01)
			{
				xy_len = 0;
				ix = 0;
				iy = 0;
				dx = 0;
				dy = 0;
			}
			else
			{
				dx = io->x_force / xy_len;
				dy = io->y_force / xy_len;
				if (xy_len > 1)
					xy_len = 1;



				// normalize current angle to +/-180
				float cur_ang = phys->player_dir*M_PI/180;
				float tmp_dx = cosf(cur_ang);
				float tmp_dy = sinf(cur_ang);
				cur_ang = atan2(tmp_dy,tmp_dx) * 180 / M_PI;

				// calc new angle normalized to +/-180
				float new_ang = (atan2(dy, dx) * 180 / M_PI + phys->yaw + 90) * M_PI / 180;
				tmp_dx = cosf(new_ang);
				tmp_dy = sinf(new_ang);
				new_ang = atan2(tmp_dy,tmp_dx) * 180 / M_PI;				

				// now we can interpolate in shorter direction
				float delta = new_ang - cur_ang; // +/-360

				// shorter dir
				if (delta < -180)
					delta += 360;
				if (delta > 180)
					delta -= 360;

				phys->player_dir = phys->player_dir + delta * 0.1f;
			}

			/*
			if (dir[iy + 1][ix + 1] >= 0)
				phys->player_dir = dir[iy + 1][ix + 1] + phys->yaw;
			*/

			if (ix || iy)
			{
				float cs = cosf(phys->slope);
				// float dn = 1.0 / sqrtf(io->x_force * io->x_force + io->y_force * io->y_force);
				float dn = 1.0;
				float dx = io->x_force * dn * cs, dy = io->y_force * dn * cs;

				phys->vel[0] += (float)(dt * (dx * cos(phys->yaw * (M_PI / 180)) - dy * sin(phys->yaw * (M_PI / 180))));
				phys->vel[1] += (float)(dt * (dx * sin(phys->yaw * (M_PI / 180)) + dy * cos(phys->yaw * (M_PI / 180))));
			}

			float sqr_vel_xy = phys->vel[0] * phys->vel[0] + phys->vel[1] * phys->vel[1];
			if (sqr_vel_xy < 1 && !ix && !iy)
			{
				phys->vel[0] = 0;
				phys->vel[1] = 0;

				if (req->mount < 2)
				{
					phys->player_stp = -1;
					/*
					if (me)
						AudioWalk(0);
					*/
				}
			}
			else
			{
				// speed limit is 27 for air / ground and 10 for full in water
				float xy_limit = 27 - 17 * (phys->water - phys->pos[2]) / world_height;

				float lim = 27;
				lim *= xy_len * xy_len*xy_len;

				if (xy_limit < 10)
					xy_limit = 10;
				if (xy_limit > lim)
					xy_limit = lim;

				if (sqr_vel_xy > xy_limit)
				{
					float n = sqrt(xy_limit / sqr_vel_xy);
					sqr_vel_xy = xy_limit;
					phys->vel[0] *= n;
					phys->vel[1] *= n;
				}

				if (phys->player_stp < 0)
					phys->player_stp = 0;

				// so 8 frame walk anim divides stp / 1024 to get frame num

				prev_step = (phys->player_stp + step_offs) & step_mask;

				xy_vel = sqrt(sqr_vel_xy);

				if (req->mount>1) // slower for flying mounts
					phys->player_stp = (~(1 << 31))&(phys->player_stp + (int)(24 * xy_vel));
				else
					phys->player_stp = (~(1 << 31))&(phys->player_stp + (int)(64 * xy_vel));

				float vel_damp = powf(0.9f, dt);
				phys->vel[0] *= vel_damp;
				phys->vel[1] *= vel_damp;
			}

			// newton vs archimedes 
			float wave = 2 * (int)((phys->stamp >> 10) & 0x7FF) * (float)M_PI / 0x800;
			float ampl = 0.05;
			if (ix || iy)
				ampl = 0.1;

			float cnt = 0.78 + ampl * sinf(wave);
			float acc = (phys->water - (phys->pos[2] + cnt * world_height)) / (2 * cnt*world_height);
			if (acc < 0 - cnt)
				acc = 0 - cnt;
			if (acc > 1 - cnt)
				acc = 1 - cnt;

			phys->vel[2] += dt * acc;

			//		if (phys->vel[2] < -1)
			//			phys->vel[2] = -1;

			// water resistance
			float res = (phys->water - phys->pos[2]) / world_height;
			if (res < 0)
				res = 0;
			if (res > 1)
				res = 1;

			in_water = res;

			float xy_res = powf(1.0 - 0.5 * res, dt);
			float z_res = powf(1.0 - 0.1 * res, dt);

			if (req->mount>1 && phys->vel[2] < 0)
				z_res = powf(1.0 - 0.1, dt);

			phys->vel[0] *= xy_res;
			phys->vel[1] *= xy_res;
			phys->vel[2] *= z_res;
		}

		phys->vel[0] += io->x_impulse;
		phys->vel[1] += io->y_impulse;

		if (fabsf(io->x_impulse) + fabsf(io->y_impulse) > 1 && phys->vel[2] > 0)
			phys->vel[2] = 0;

		io->x_impulse *= 0.5;
		io->y_impulse *= 0.5;

		int material_votes[6] = {0}; /* rock:0, wood:1, dirt:2, grass:3, hi-grass:4, blood:5, water:6 */

		// POS - troubles!
		float prev_vel_z = phys->vel[2];
		float contact_normal_z = 0;
		{
			////////////////////
			float dx = dt * phys->vel[0];
			float dy = dt * phys->vel[1];

			double cx = phys->pos[0] + dx * 0.5;
			double cy = phys->pos[1] + dy * 0.5;
			double th = 0.1;

			double qx = fabs(dx) * 0.5 + world_radius + th;
			double qy = fabs(dy) * 0.5 + world_radius + th;

			double clip_world[4][4] =
			{
				{ 1, 0, 0, qx - cx },
				{-1, 0, 0, qx + cx },
				{ 0, 1, 0, qy - cy },
				{ 0,-1, 0, qy + cy },
				//	{ 0, 0, 1,            0 - phys->pos[2] },
				//	{ 0, 0,-1, world_height + phys->pos[2] }
			};

			// create triangle soup of (SoupItem):
			phys->soup_items = 0;
			phys->collect_mul_xy = 1.0 / world_radius;
			phys->collect_mul_z = 2.0 / world_height;

			phys->max_height = io->water;

			QueryWorldCB cb = { Physics::MeshCollect , Physics::SpriteCollect };
			QueryWorld(phys->world, 4, clip_world, &cb, phys);
			QueryTerrain(phys->terrain, 4, clip_world, 0xAA, Physics::PatchCollect, phys);

			// note: phys should keep soup allocation, resize it x2 if needed

			// transform Z so our ellipsolid becomes a sphere
			// just multiply: 
			//   px,py, dx,dy, and all verts x,y coords by 1.0/horizontal_radius
			//   pz, dz and all verts z coords by 1.0/(HEIGHT_SCALE*vertical_radius)

			float sphere_pos[3] =  // set current sphere center
			{
				phys->pos[0] * phys->collect_mul_xy,
				phys->pos[1] * phys->collect_mul_xy,
				(phys->pos[2] + world_height * 0.5f) * phys->collect_mul_z,
			};

			float sphere_vel[3] =
			{
				xy_speed * phys->vel[0] * dt * phys->collect_mul_xy,
				xy_speed * phys->vel[1] * dt * phys->collect_mul_xy,
				phys->vel[2] * dt * phys->collect_mul_z,
			}; // set velocity (must include gravity impact)

			const float xy_thresh = 0.002f;
			const float z_thresh = 0.001f;

			int items = phys->soup_items;
			int iters_left = 10;
			// bool ignore_roof = false;

			// retry_without_roof:

			while (fabsf(sphere_vel[0]) > xy_thresh || fabsf(sphere_vel[1]) > xy_thresh || fabsf(sphere_vel[2]) > z_thresh)
			{
				SoupItem* collision_item = 0;
				float collision_time = 2.0f; // (greater than current velocity range)
				float collision_pos[3];

				for (int i = 0; i < items; i++)
				{
					SoupItem* item = phys->soup + i;

					float contact_pos[3];
					float time = item->CheckCollision(sphere_pos, sphere_vel, contact_pos); // must return >=2 if no collision occurs

					assert(time >= 0);

					if (time < collision_time)
					{
						float check[3] =
						{
							sphere_pos[0] + sphere_vel[0] * time - contact_pos[0],
							sphere_pos[1] + sphere_vel[1] * time - contact_pos[1],
							sphere_pos[2] + sphere_vel[2] * time - contact_pos[2],
						};

						float sqr_dist = DotProduct(check, check);

						if (fabsf(sqr_dist) - 1.0f > 0.001)
						{
							assert(0);
							// recheck
							// time = item->CheckCollision2(sphere_pos, sphere_vel, contact_pos); // must return >=2 if no collision occurs
						}

						// if (!ignore_roof || contact_pos[2] < sphere_pos[2] + 0.5)
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

				float full_step[3] =
				{
					sphere_vel[0] * collision_time,
					sphere_vel[1] * collision_time,
					sphere_vel[2] * collision_time
				};

				float slide_normal[3] =
				{
					sphere_pos[0] + full_step[0] - collision_pos[0],
					sphere_pos[1] + full_step[1] - collision_pos[1],
					sphere_pos[2] + full_step[2] - collision_pos[2]
				};

				material_votes[collision_item->material]++;

				float full_len = sqrt(full_step[0] * full_step[0] + full_step[1] * full_step[1] + full_step[2] * full_step[2]);
				float ratio = 0.0;
				if (full_len > 0.01)
					ratio = (full_len - 0.01) / full_len;

				sphere_pos[0] += full_step[0] * ratio;
				sphere_pos[1] += full_step[1] * ratio;
				sphere_pos[2] += full_step[2] * ratio;

				float remain = 1.0f - collision_time;
				if (remain >= 0.99f)
					remain = 0.99f;

				sphere_vel[0] *= remain;
				sphere_vel[1] *= remain;
				sphere_vel[2] *= remain;

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

				contact_normal_z = fmaxf(contact_normal_z, slide_normal[2]);

				if (!--iters_left)
					break;
			}

			/*
			if (iters_left)
				phys->collision_failure = false;
			else
				if (!phys->collision_failure && !ix && !iy && contact_normal_z > 0)
				{
					// something's wrong
					// relax - ignore collisions from top
					phys->collision_failure = true;
					//printf("CRITICAL! move to resolve\n");
				}

			if (phys->collision_failure && !ignore_roof)
			{
				ignore_roof = true;
				goto retry_without_roof;
			}
			*/

			//printf("iters_left:%d\n", iters_left);

			// convert back to world coords
			// just multiply: 
			//   px,py by horizontal_radius
			//   and pz by (HEIGHT_SCALE*vertical_radius)

			// we are done, update 

			float pos[3] =
			{
				sphere_pos[0] / phys->collect_mul_xy,
				sphere_pos[1] / phys->collect_mul_xy,
				sphere_pos[2] / phys->collect_mul_z - world_height * 0.5f
			};

			/*

			float vel[3] =
			{
				(pos[0] - phys->pos[0]) / dt,
				(pos[1] - phys->pos[1]) / dt,
				(pos[2] - phys->pos[2]) / (dt*HEIGHT_SCALE)
			};

			float org_vel[3] =
			{
				phys->vel[0],
				phys->vel[1],
				phys->vel[2]
			};

			phys->vel[0] *= xy_speed;
			phys->vel[1] *= xy_speed;
			phys->vel[2] /= HEIGHT_SCALE;
			float vn = sqrtf(phys->vel[0] * phys->vel[0] + phys->vel[1] * phys->vel[1] + phys->vel[2] * phys->vel[2]);

			if (vn > 0.001)
			{


				vn = 1.0 / vn;
				// leave direction only
				phys->vel[0] *= vn;
				phys->vel[1] *= vn;
				phys->vel[2] *= vn;

				// project
				vn = DotProduct(phys->vel, vel);

				// apply magnitude from poisition offs
				phys->vel[0] *= vn / xy_speed;
				phys->vel[1] *= vn / xy_speed;
				phys->vel[2] *= vn * HEIGHT_SCALE;

				printf("iters_left:%d, in: %f,%f out: %f,%f\n", iters_left, org_vel[0], org_vel[1], phys->vel[0], phys->vel[1]);

				// average org and new
				phys->vel[0] = phys->vel[0] * 0.0 + org_vel[0] * 1.0;
				phys->vel[1] = phys->vel[1] * 0.0 + org_vel[1] * 1.0;
				phys->vel[2] = phys->vel[2] * 0.0 + org_vel[2] * 1.0;
			}
			else
			{
				phys->vel[0] = 0;
				phys->vel[1] = 0;
				phys->vel[2] = 0;
			}
			*/

			float org_vel[3] =
			{
				phys->vel[0],
				phys->vel[1],
				phys->vel[2]
			};

			phys->vel[0] = (pos[0] - phys->pos[0]) / (xy_speed * dt);
			phys->vel[1] = (pos[1] - phys->pos[1]) / (xy_speed * dt);
			phys->vel[2] = (pos[2] - phys->pos[2]) / dt;

			float adz = fmaxf(0, phys->vel[2]) / HEIGHT_SCALE * 4;
			float adxy = xy_speed * sqrtf(phys->vel[0] * phys->vel[0] + phys->vel[1] * phys->vel[1]);
			phys->slope = atan2(adz, adxy);

			// printf("iters_left:%d, in: %f,%f out: %f,%f\n", iters_left, org_vel[0], org_vel[1], phys->vel[0], phys->vel[1]);

			// slippery threshold?
			// use org (no-slippery) use new (full slippery)

			phys->vel[0] = 1.0 * phys->vel[0] + org_vel[0] * 0.0;
			phys->vel[1] = 1.0 * phys->vel[1] + org_vel[1] * 0.0;
			phys->vel[2] = 1.0 * phys->vel[2] + org_vel[2] * 0.0;

			// printf("contact_normal_z:%f, vel[0]: %f, vel[1]:%f, vel[2]:%f\n", contact_normal_z,fabsf(phys->vel[0]),fabsf(phys->vel[1]),fabsf(phys->vel[2]));

			if (ix || iy || contact_normal_z <= 0.0 || fabsf(phys->vel[0]) > 0.1 || fabsf(phys->vel[1]) > 0.1 || fabsf(phys->vel[2]) > 0.1 * 16)
			{
				phys->pos[0] = pos[0];
				phys->pos[1] = pos[1];
				phys->pos[2] = pos[2];
			}
			else
			{
				phys->vel[0] = 0;
				phys->vel[1] = 0;
				phys->vel[2] = 0;
			}
		}

		// votes given, choose new winner
		{
			int mat = phys->mat; // default to prev winner (was grass)
			int votes = 0;
			for (int m=0; m<6; m++)
			{
				if (material_votes[m]>votes)
				{
					votes = material_votes[m];
					mat = m;			
				}
			}
			phys->mat = mat;

			// override if legs are in water
			if (in_water>0.1)
				phys->mat = 6; // voting veto!
		}

		// jump

		float prev_contact = phys->accum_contact;

		phys->accum_contact += fmaxf(0.0f,contact_normal_z);
		if (phys->accum_contact > 5)
			phys->accum_contact = 5;

		if (me && prev_contact < 1.0 && phys->accum_contact >= 1.0)
		//if (me && prev_vel_z < 0 && phys->vel[2] > prev_vel_z)
		{
			// how to find nice energy loss ?
			AudioWalk(0, 65535, req, phys->mat);
		}
		else
		if (me && (in_water>0.5 || phys->accum_contact >= 1.0) && phys->player_stp>=0)
		{
			int volume = (int)(65535 * 1.0f*log10f(xy_vel + 1.0));
			if (volume > 65535)
				volume = 65535;
			int next_step = (phys->player_stp + step_offs) & step_mask;
			if (prev_step < 2048 && next_step >= 2048)
				AudioWalk(1, volume, req, phys->mat);
			else
			if (prev_step < 3 * 2048 && next_step >= 3 * 2048)
				AudioWalk(2, volume, req, phys->mat);
		}


		// if (contact_normal_z > 0.0)
		if (phys->accum_contact >= 1.0 || req->mount>1)
		{
			if (io->jump)
			{

				phys->accum_contact = 0;

				// ensure for mount>1 current height is not > ground + max_fly_height
				if (req->mount < 2 || phys->pos[2] < phys->max_height + 100)
				{

					if (phys->vel[2] < 0)
						phys->vel[2] = 10;
					else
						phys->vel[2] += 10;
				}

				io->jump = false;
			}
		}

		io->grounded = phys->accum_contact >= 1.0;

		phys->accum_contact *= 0.9f;

		if (phys->vel[2] > 20)
			phys->vel[2] = 20;

		if (req->mount > 1)
		{
			if (!io->grounded)
			{
				float v = fmaxf(1.0f, phys->vel[2]);
				phys->player_stp = (~(1 << 31))&(phys->player_stp + (int)(v * 64 * 2));
			}
			else
			if (!io->x_force && !io->y_force)
			{
				phys->player_stp = -1;
			}
		}

		/*
		// what was this for?
		for (int h = 63; h > 0; h--)
		{
			io->xyz[h][0] = io->xyz[h - 1][0];
			io->xyz[h][1] = io->xyz[h - 1][1];
			io->xyz[h][2] = io->xyz[h - 1][2];
		}

		io->xyz[0][0] = phys->pos[0];
		io->xyz[0][1] = phys->pos[1];
		io->xyz[0][2] = phys->pos[2];
		*/
	}

	io->pos[0] = phys->pos[0];
	io->pos[1] = phys->pos[1];
	io->pos[2] = phys->pos[2];

	io->yaw = phys->yaw;
	io->player_dir = phys->player_dir;
	io->player_stp = phys->player_stp;

	return steps_handled;

	// OLD POS
	// after updating x,y,z by time and keyb bits
	// we need to fix z so player doesn't penetrate terrain
	/*
	{
		double p[3] = { phys->pos[0],phys->pos[1],-1 };
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
			p[2] = phys->pos[2] + world_height*0.5;
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
			if (!patch && r[2] - phys->pos[2] > 48) // do pasa
			{
				// nie pozwalamy na wslizg
				phys->pos[0] = push[0];
				phys->pos[1] = push[1];
				phys->pos[2] = push[2];
				phys->vel[0] = 0;
				phys->vel[1] = 0;
				phys->vel[2] = 0;
				return;
			}


			// we need contact to jump
			if (phys->IsKeyDown(A3D_SPACE) && r[2] - phys->pos[2] > -16)
				phys->vel[2] = 10;
			else
				phys->keys[A3D_SPACE >> 3] &= ~(1 << (A3D_SPACE & 7));

			if (r[2] >= phys->pos[2])
			{
				double n_len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
				phys->player_slope = min(0.1,n[2] / n_len);
				phys->pos[2] = r[2];
				if (phys->vel[2]<0)
					phys->vel[2] = 0;
			}
			else
				phys->player_slope = 0.1;
		}
		else
		{
			// we need contact to jump
			if (phys->IsKeyDown(A3D_SPACE) && phys->pos[2] < 16)
				phys->vel[2] = 10;
			else
				phys->keys[A3D_SPACE >> 3] &= ~(1 << (A3D_SPACE & 7));

			phys->player_slope = 0.1;
			if (phys->pos[2] < 0)
				phys->pos[2] = 0;
		}
	}
	*/
}

Physics* CreatePhysics(Terrain* t, World* w, float pos[3], float dir, float yaw, uint64_t stamp)
{
    Physics* phys = (Physics*)malloc(sizeof(Physics));

	phys->stamp = stamp;

	phys->mat = 3; // default to grass

    phys->terrain = t;
    phys->world = w;

	// phys->collision_failure = false;

	phys->soup = 0;
	phys->soup_alloc = 0;
	phys->soup_items = 0;

	phys->yaw = yaw;
	phys->yaw_vel = 0;

	phys->pos[0] = pos[0];
	phys->pos[1] = pos[1];
	phys->pos[2] = pos[2];
	phys->vel[0] = 0;
	phys->vel[1] = 0;
	phys->vel[2] = 0;

	phys->accum_contact = 0;

	// todo:
	// check safe initial position so it won't intersect with anything!!!
	{
		double p[3] = { phys->pos[0],phys->pos[1],-1 };
		double v[3] = { 0,0,-1 };
		double r[4] = { 0,0,0,1 };
		double n[3];
		Patch* patch = HitTerrain(phys->terrain, p, v, r, n);

		if (patch)
			phys->pos[2] = (float)r[2] + 200; // assuming no mesh stands above terrain by more than 200 units
	}

	phys->slope = 0;
	phys->player_dir = dir;
	phys->player_stp = -1;

    return phys;
}

void DeletePhysics(Physics* phys)
{
    if (phys->soup)
        free(phys->soup);
    free(phys);
}

void SetPhysicsPos(Physics* phys, float pos[3], float vel[3])
{
	// TODO: should be save (resolve collisions)
	// ...

	if (pos)
	{
		phys->pos[0] = pos[0];
		phys->pos[1] = pos[1];
		phys->pos[2] = pos[2];
	}

	if (vel)
	{
		phys->vel[0] = vel[0];
		phys->vel[1] = vel[1];
		phys->vel[2] = vel[2];
	}
}

void SetPhysicsYaw(Physics* phys, float yaw, float vel)
{
	phys->yaw = yaw;
	phys->yaw_vel = vel;
}

void SetPhysicsDir(Physics* phys, float dir)
{
	phys->player_dir = dir;
}