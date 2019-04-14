// nvbug.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <string.h>

#include "gl.h"

#include "rgba8.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h" // beta: ImGuiItemFlags_Disabled

#include "imgui_impl_opengl3.h"

#include "asciiid_platform.h"
#include "texheap.h"
#include "terrain.h"
#include "asciiid_urdo.h"

#include "matrix.h"

#include "fast_rand.h"

Terrain* terrain = 0;
int fonts_loaded = 0;
int palettes_loaded = 0;

void* GetMaterialArr();
void* GetPaletteArr();
void* GetFontArr();

struct Cell
{
	uint8_t fg[3];	// foreground color
	uint8_t gl;		// glyph code
	uint8_t bg[3];	// background color

	uint8_t flags;  
	// transparency mask :
	// 0x1 - fg 
	// 0x2 - gl 
	// 0x4 - bg

	// blend modes 3x3 bits:
	// 0x03 2-bits fg blend mode (0:replace, 1:multiply, 2:screen, 3:transparent)
	// 0x04 glyph write mask (0:replace, 1:keep)
	// 0x18 2-bits bg blend mode (0:replace, 1:multiply, 2:screen, 3:transparent)
	// 3 bits left!
};

struct MyMaterial
{
	static void Free()
	{
		glDeleteTextures(1,&tex);
	}

	static void Init()
	{
		MyMaterial* m = (MyMaterial*)GetMaterialArr();

		uint8_t g[3] = {'`',' ',','};
		uint8_t f[3] = {0xFF,0,0};
		for (int s=0; s<16; s++)
		{
			for (int r=0; r<3; r++)
			{
				m[0].shade[r][s].fg[0]=f[r];
				m[0].shade[r][s].fg[1]=f[r];
				m[0].shade[r][s].fg[2]=f[r];

				m[0].shade[r][s].gl = g[r];

				m[0].shade[r][s].bg[0]=0xCF;
				m[0].shade[r][s].bg[1]=0xCF;
				m[0].shade[r][s].bg[2]=0xCF;

				m[0].shade[r][s].flags = 0;
			}
		}

		for (int i = 1; i < 256; i++)
		{
			for (int r = 0; r < 3; r++)
			{
				for (int s = 0; s < 16; s++)
				{
					m[i].shade[r][s].bg[0] = fast_rand() & 0xFF;
					m[i].shade[r][s].bg[1] = fast_rand() & 0xFF;
					m[i].shade[r][s].bg[2] = fast_rand() & 0xFF;
					m[i].shade[r][s].fg[0] = fast_rand() & 0xFF;
					m[i].shade[r][s].fg[1] = fast_rand() & 0xFF;
					m[i].shade[r][s].fg[2] = fast_rand() & 0xFF;
					m[i].shade[r][s].gl = fast_rand() & 0xFF;
					m[i].shade[r][s].flags = 0;
				}
			}
		}

		glCreateTextures(GL_TEXTURE_2D, 1, &tex);

		glTextureStorage2D(tex, 1, GL_RGBA8UI, 128, 256);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(tex, 0, 0, 0, 128, 256, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, m->shade );
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	void Update()
	{
		MyMaterial* m = (MyMaterial*)GetMaterialArr();
		int y = (int)(this-m);
		// update this single material texture slice !
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(tex, 0, 0, y, 128, 1, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, shade);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	static GLuint tex; // single texture for all materials 128x256

	Cell shade[4][16]; // each cell has 2 texels !!! shade[3] is currently spare space.
};

GLuint MyMaterial::tex = 0;
MyMaterial mat[256];

struct MyPalette
{
	static void Init()
	{
		MyPalette* p = (MyPalette*)GetPaletteArr();
		for (int j = 0; j < 256; j++)
			for (int i = 0; i < 768; i++)
				p[j].rgb[i] = fast_rand() & 0xFF;
	}

	static bool Scan(A3D_DirItem item, const char* name, void* cookie)
	{
		if (!(item&A3D_FILE))
			return true;

		char buf[4096];
		snprintf(buf, 4095, "%s/%s", (char*)cookie, name);
		buf[4095] = 0;

		a3dLoadImage(buf, 0, MyPalette::Load);
		return true;
	}

	static void Load(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
	{
		if (palettes_loaded == 256)
			return;

		MyPalette* p = (MyPalette*)GetPaletteArr() + palettes_loaded;

		uint32_t* buf = (uint32_t*)malloc(w*h * sizeof(uint32_t));
		Convert_RGBA8(buf, f, w, h, data, palsize, palbuf);

		// extract palette by sampling at centers of w/16 x h/16 patches
		int hx = (w + 16) / 32;
		int hy = (h + 16) / 32;

		for (int y = 0; y < 16; y++)
		{
			int row = w * (y * h / 16 + hy) + hx;
			for (int x = 0; x < 16; x++)
			{
				uint32_t rgb = buf[x * w / 16 + row];

				p->rgb[3 * (x + y * 16) + 0] = rgb & 0xFF;
				p->rgb[3 * (x + y * 16) + 1] = (rgb>>8) & 0xFF;
				p->rgb[3 * (x + y * 16) + 2] = (rgb>>16) & 0xFF;
			}
		}

		free(buf);
		palettes_loaded++;
	}

	uint8_t rgb[3 * 256];
} pal[256];

struct MyFont
{
	static bool Scan(A3D_DirItem item, const char* name, void* cookie)
	{
		if (!(item&A3D_FILE))
			return true;

		char buf[4096];
		snprintf(buf,4095,"%s/%s",(char*)cookie,name);
		buf[4095]=0;

		a3dLoadImage(buf, 0, MyFont::Load);
		return true;
	}

	static int Sort(const void* a, const void* b)
	{
		MyFont* fa = (MyFont*)a;
		MyFont* fb = (MyFont*)b;

		int qa = fa->width*fa->height;
		int qb = fb->width*fb->height;

		return qa - qb;
	}

	static void Free()
	{
		MyFont* fnt = (MyFont*)GetFontArr();
		for (int i=0; i<fonts_loaded; i++)
		{
			glDeleteTextures(1,&fnt[i].tex);
		}
	}

	static void Load(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
	{
		if (fonts_loaded==256)
			return;
			
		MyFont* fnt = (MyFont*)GetFontArr() + fonts_loaded;

		fnt->width = w;
		fnt->height = h;

		int ifmt = GL_RGBA8;
		int fmt = GL_RGBA;
		int type = GL_UNSIGNED_BYTE;

		uint32_t* buf = (uint32_t*)malloc(w * h * sizeof(uint32_t));

		uint8_t rgb[3] = { 0xff,0xff,0xff };
		ConvertLuminanceToAlpha_RGBA8(buf, rgb, f, w, h, data, palsize, palbuf);

		glCreateTextures(GL_TEXTURE_2D, 1, &fnt->tex);
		glTextureStorage2D(fnt->tex, 1, ifmt, w, h);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(fnt->tex, 0, 0, 0, w, h, fmt, type, buf ? buf : data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		float white_transp[4] = { 1,1,1,0 };

		glTextureParameteri(fnt->tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(fnt->tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(fnt->tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(fnt->tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTextureParameterfv(fnt->tex, GL_TEXTURE_BORDER_COLOR, white_transp);


		/*
		// if we want to filter font we'd have first to
		// modify 3 things in font sampling by shader:
		// - clamp uv to glyph boundary during sampling
		// - fade result by distance normalized to 0.5 of texel 
		//   between unclamped uv to clamping glyph boundary
		// - use manual lod as log2(font_zoom)

		int max_lod = 0;
		while (!((w & 1) | (h & 1)))
		{
			max_lod++;
			w >>= 1;
			h >>= 1;
		}
		glGenerateTextureMipmap(fnt->tex);
		glTextureParameteri(fnt->tex, GL_TEXTURE_MAX_LOD, max_lod);
		*/

		if (buf)
			free(buf);

		fonts_loaded++;

		qsort(GetFontArr(), fonts_loaded, sizeof(MyFont), MyFont::Sort);
	}

	void SetTexel(int x, int y, uint8_t val)
	{
		uint8_t texel[4] = { 0xFF,0xFF,0xFF,val };
		glTextureSubImage2D(tex, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texel);
	}

	uint8_t GetTexel(int x, int y)
	{
		uint8_t texel[4];
		glGetTextureSubImage(tex, 0, x, y, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4, texel);
		return texel[3];
	}

	int width;
	int height;

	GLuint tex;
} font[256];

void* GetMaterialArr()
{
	return mat;
}

void* GetPaletteArr()
{
	return pal;
}

void* GetFontArr()
{
	return font;
}


int active_font = 0;
int active_glyph = 0x40; //@
int active_palette = 0;
int active_material = 0;

/*
float dawn_color[3] = { 1,.8f,0 };
float noon_color[3] = { 1,1,1 };
float dusk_color[3] = { 1,.2f,0 };
float midnight_color[3] = { .1f,.1f,.5f };
*/

float font_size = 10;// 0.125;// 16; // so every visual cell appears as 16px
float rot_yaw = 45;
float rot_pitch = 30;//90;

float lit_yaw = 45;
float lit_pitch = 30;//90;
float lit_time = 12.0f;

bool spin_anim = false;
float pos_x = 0, pos_y = 0, pos_z = 0;
int mouse_in = 0;

int panning = 0;
int panning_x = 0;
int panning_y = 0;
double panning_dx = 0;
double panning_dy = 0;

float zoom_wheel = 0;

int spinning = 0;
int spinning_x = 0;
int spinning_y = 0;

int creating = 0; // +1 = add, -1 = del

int painting = 0; 
const float STAMP_R = 0.50;
const float STAMP_A = 1.00;
float br_radius = 10;
float br_alpha = 0.5f;
int painting_x = 0;
int painting_y = 0;
double painting_dx;
double painting_dy;
double paint_dist;

bool diag_flipped = false;
bool br_limit = false;
int probe_z = 0;

uint64_t g_Time; // in microsecs

#define QUOT(a) #a
#define DEFN(a) "#define " #a " " QUOT(a) "\n"
#define CODE(...) #__VA_ARGS__

struct RenderContext
{
	void Create()
	{
		int err = glGetError();
		glCreateBuffers(1, &vbo);
		err = glGetError();
		glNamedBufferStorage(vbo, TERRAIN_TEXHEAP_CAPACITY * sizeof(GLint[5]), 0, GL_DYNAMIC_STORAGE_BIT);
		err = glGetError();

		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribIPointer(0, 4, GL_INT, sizeof(GLint[5]), (void*)0);
		glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(GLint[5]), (void*)sizeof(GLint[4]));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glCreateBuffers(1, &ghost_vbo);
		err = glGetError();
		glNamedBufferStorage(ghost_vbo, sizeof(GLint[3*4*HEIGHT_CELLS]), 0, GL_DYNAMIC_STORAGE_BIT);
		err = glGetError();

		glCreateVertexArrays(1, &ghost_vao);
		glBindVertexArray(ghost_vao);
		glBindBuffer(GL_ARRAY_BUFFER, ghost_vbo);
		glVertexAttribIPointer(0, 3, GL_INT, sizeof(GLint[3]), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		const char* ghost_vs_src =
			CODE(#version 450\n)
			DEFN(HEIGHT_SCALE)
			DEFN(HEIGHT_CELLS)
			DEFN(VISUAL_CELLS)
			CODE(
				layout(location = 0) in ivec3 xyz;
				uniform mat4 tm;
				void main()
				{
					float scale = float(VISUAL_CELLS) / float(HEIGHT_CELLS);
					vec4 pos = vec4(xyz, 1.0);
					pos.xy *= scale;
					gl_Position = tm * pos;
				}
			);

		const char* ghost_fs_src =
			CODE(#version 450\n)
			DEFN(HEIGHT_SCALE)
			DEFN(HEIGHT_CELLS)
			DEFN(VISUAL_CELLS)
			CODE(
				layout(location = 0) out vec4 color;
				uniform vec4 cl;
				void main()
				{
					color = cl;
				}
			);

		const char* vs_src = 
		CODE(#version 450\n)
		DEFN(HEIGHT_SCALE)
		DEFN(HEIGHT_CELLS)
		DEFN(VISUAL_CELLS)
		CODE(
			layout(location = 0) in ivec4 in_xyuv;
			layout(location = 1) in uint in_diag;
			out ivec4 xyuv;
			out uint diag;

			void main()
			{
				xyuv = in_xyuv;
				diag = in_diag;
			}
		);

		const char* gs_src = 
		CODE(#version 450\n)
		DEFN(HEIGHT_SCALE)
		DEFN(HEIGHT_CELLS)
		DEFN(VISUAL_CELLS)
		CODE(
			layout(points) in;
			layout(triangle_strip, max_vertices = 4*HEIGHT_CELLS*HEIGHT_CELLS ) out;

			uniform vec4 br;
			uniform usampler2D z_tex;
			uniform mat4 tm;

			uniform vec3 pr; // .x=height , .y=alpha (alpha=0.5 when probing, otherwise 1.0), .z is br_limit direction (+1/-1 or 0 if disabled)


			in ivec4 xyuv[];
			in uint diag[];

			out vec4 world_xyuv;
			out vec3 uvh;
			flat out vec3 normal;
			
			void main()
			{
				uint z;
				vec4 v;
				ivec2 xy;

				vec3 xyz[4];
				vec2 uv[4];

				float rvh = float(VISUAL_CELLS) / float(HEIGHT_CELLS);
				float dxy = 1.0 / float(HEIGHT_CELLS);
				ivec2 bxy = xyuv[0].xy*HEIGHT_CELLS;

				// todo: emit optimized strips
				// should allow having upto 6x6 patches -> 12 scalars * 6 strips * (6+1) cols * 2 verts = 1008 components (out of 1024)
				// currently max is 4x4 -> 12 scalars * 4*4 quads * 4 verts -> 768 components

				uint rot = diag[0];
				ivec4 order[2] = ivec4[2](ivec4(0, 1, 2, 3), ivec4(1, 3, 0, 2));

				for (int y = 0; y < HEIGHT_CELLS; y++)
				{
					for (int x = 0; x < HEIGHT_CELLS; x++)
					{
						xy = ivec2(x, y + 1);
						uv[0] = (xyuv[0].zw + vec2(xy) / HEIGHT_CELLS) * VISUAL_CELLS;
						z = texelFetch(z_tex, xyuv[0].zw*(HEIGHT_CELLS+1) + xy, 0).r;
						xy = bxy + xy*VISUAL_CELLS;
						xyz[0] = vec3(xy*dxy, z);

						xy = ivec2(x, y);
						uv[1] = (xyuv[0].zw + vec2(xy) / HEIGHT_CELLS) * VISUAL_CELLS;
						z = texelFetch(z_tex, xyuv[0].zw*(HEIGHT_CELLS + 1) + xy, 0).r;
						xy = bxy + xy*VISUAL_CELLS;
						xyz[1] = vec3(xy*dxy, z);

						xy = ivec2(x + 1, y + 1);
						uv[2] = (xyuv[0].zw + vec2(xy) / HEIGHT_CELLS) * VISUAL_CELLS;
						z = texelFetch(z_tex, xyuv[0].zw*(HEIGHT_CELLS + 1) + xy, 0).r;
						xy = bxy + xy * VISUAL_CELLS;
						xyz[2] = vec3(xy*dxy, z);

						xy = ivec2(x + 1, y);
						uv[3] = (xyuv[0].zw + vec2(xy) / HEIGHT_CELLS) * VISUAL_CELLS;
						z = texelFetch(z_tex, xyuv[0].zw*(HEIGHT_CELLS + 1) + xy, 0).r;
						xy = bxy + xy * VISUAL_CELLS;
						xyz[3] = vec3(xy*dxy, z);

						if (br.w != 0.0 && br.z>0)
						{
							for (int i = 0; i < 4; i++)
							{
								vec2 d = xyz[i].xy - br.xy;
								float len = length(d);
								if (len < br.z)
								{
									float gauss = (0.5 + 0.5*cos(len/br.z*3.141592));

									int d = int(round(gauss*gauss * br.w * br.z * HEIGHT_SCALE));

									float z = xyz[i].z + d;

									if (pr.z!=0) // limit enabled
									{
										if (d > 0)
										{
											if (xyz[i].z > pr.x)
												z = xyz[i].z;
											else
											if (z > pr.x)
												z = pr.x;
										}
										else
										if (d < 0)
										{
											if (xyz[i].z < pr.x)
												z = xyz[i].z;
											else
											if (z < pr.x)
												z = pr.x;
										}
									}
									else
									{
										if (z < 0)
											z = 0;
										if (z > 0xffff)
											z = 0xffff;
									}

									xyz[i].z = z;

									// xyz[i].z += int(round(gauss*gauss * br.w * br.z * HEIGHT_SCALE));
									// xyz[i].z = clamp(xyz[i].z, 0, 0xffff);
								}
							}
						}

						vec3 norm[4];
						norm[0] = cross(xyz[1] - xyz[0], xyz[2] - xyz[0]);
						norm[1] = cross(xyz[2] - xyz[3], xyz[1] - xyz[3]);
						norm[2] = cross(xyz[3] - xyz[1], xyz[0] - xyz[1]);
						norm[3] = cross(xyz[0] - xyz[2], xyz[3] - xyz[2]);

						uint r = rot & 1;

						normal = norm[2 * r];
						normal.xy *= 1.0 / HEIGHT_SCALE;

						{
							int i = order[r][0];

							world_xyuv = vec4(xyz[i].xy, uv[i]);
							uvh.xyz = xyz[i] - ivec3(xyuv[0].xy, 0);
							uvh.xyz /= vec3(rvh, rvh, HEIGHT_SCALE);

							gl_Position = tm * vec4(xyz[i], 1.0);
							EmitVertex();
						}
						{
							int i = order[r][1];

							world_xyuv = vec4(xyz[i].xy, uv[i]);
							uvh.xyz = xyz[i] - ivec3(xyuv[0].xy, 0);
							uvh.xyz /= vec3(rvh, rvh, HEIGHT_SCALE);

							gl_Position = tm * vec4(xyz[i], 1.0);
							EmitVertex();
						}
						{
							int i = order[r][2];

							world_xyuv = vec4(xyz[i].xy, uv[i]);
							uvh.xyz = xyz[i] - ivec3(xyuv[0].xy, 0);
							uvh.xyz /= vec3(rvh, rvh, HEIGHT_SCALE);

							gl_Position = tm * vec4(xyz[i], 1.0);
							EmitVertex();
						}

						normal = norm[2 * r + 1];
						normal.xy *= 1.0 / HEIGHT_SCALE;

						{
							int i = order[r][3];

							world_xyuv = vec4(xyz[i].xy, uv[i]);
							uvh.xyz = xyz[i] - ivec3(xyuv[0].xy, 0);
							uvh.xyz /= vec3(rvh, rvh, HEIGHT_SCALE);

							gl_Position = tm * vec4(xyz[i], 1.0);
							EmitVertex();
						}


						rot = rot >> 1;
						EndPrimitive();
					}
				}
			}
		);

		const char* fs_src = 
		CODE(#version 450\n)
		DEFN(HEIGHT_SCALE)
		DEFN(HEIGHT_CELLS)
		DEFN(VISUAL_CELLS)
		CODE(
			layout(location = 0) out vec4 color;

			uniform usampler2D v_tex;
			uniform usampler2D m_tex;
			uniform sampler2D f_tex;

			uniform vec3 lt; // light pos
			// uniform vec3 lc; // light rgb
			uniform vec4 br; // brush
			uniform vec3 qd; // quad diag
			uniform vec3 pr; // .x=height , .y=alpha (alpha=0.5 when probing, otherwise 1.0), .z is br_limit direction (+1/-1 or 0 if disabled)
			uniform float fz; // font zoom

			flat in vec3 normal;
			in vec3 uvh;
			in vec4 world_xyuv;

			float Grid(vec2 d, vec2 p, float s)
			{
				d *= s;
				p = fract(p*s + vec2(0.5));

				float r = 1.0;

				if (d.x < 0.25)
				{
					float a = clamp(-log2(d.x * 4), 0.0, 1.0);
					float m = smoothstep(0.5 - d.x, 0.5, p.x) * smoothstep(0.5 + d.x, 0.5, p.x);
					r *= mix(1.0, pow(1.0 - m, 0.5), a);
				}
				if (d.y < 0.25)
				{
					float a = clamp(-log2(d.y * 4), 0.0, 1.0);
					float m = smoothstep(0.5 - d.y, 0.5, p.y) * smoothstep(0.5 + d.y, 0.5, p.y);
					r *= mix(1.0, pow(1.0 - m, 0.5), a);
				}

				return r;
			}
			
			void main()
			{
				// sample terrain visual
				uint visual = texelFetch(v_tex, ivec2(floor(world_xyuv.zw)), 0).r;
				//visual = 12345;

				vec3 light_pos = normalize(lt);
				float light = 0.5 + 0.5*dot(light_pos, normalize(normal));

				{
					uint matid = visual & 0xFF;
					uint shade = (visual >> 8) & 0xF;
					uint elev  = (visual >> 12) & 0x1;
					uint mode = (visual >> 13) & 0x3;
					// 1 bit left

					if (mode == 1) // replace shade with lighting
						shade = uint(round(light * 15.0));
					else
					if (mode == 2)
						shade = uint(round(light * shade));
					else
					if (mode == 3)
						shade = uint(round(light * 15.0)*(1 - shade) + shade);

					/*
						we could define mode on 2 bits:
						- 0: use shade map than apply lighting to rgb (useful for sculpting w/o defined materials in editor)
						- 1: overwrite shade with lighting   \
						- 2: multiply shade map by lighting   >-- for game
						- 3: screen shade map with lighting  /
					*/

					// convert elev to 0,1,2 material row of shades
					elev = uint(1);

					// sample material array
					// y=0,1 -> descent; y=2,3 -> fill; y=4,5 -> ascent
					uint mat_x = 2 * shade + 32 * elev;
					uvec4 fill_rgbc = texelFetch(m_tex, ivec2(0+mat_x, matid), 0);
					uvec4 fill_rgbp = texelFetch(m_tex, ivec2(1+mat_x, matid), 0);

					//fill_rgbc.w = 44;

					uvec2 font_size = textureSize(f_tex,0);
					uvec2 glyph_size = font_size / 16;

					vec2 glyph_fract = fract(gl_FragCoord.xy * fz / glyph_size);
					glyph_fract.y = 1.0 - glyph_fract.y;
					if (glyph_fract.x < 0)
						glyph_fract.x += 1;
					if (glyph_fract.y < 0)
						glyph_fract.y += 1;
					if (glyph_fract.x >= 1)
						glyph_fract.x -= 1;
					if (glyph_fract.y >= 1)
						glyph_fract.y -= 1;

					// sample font texture (pure alpha)
					vec2 glyph_coord = vec2(fill_rgbc.w & 0xF, fill_rgbc.w >> 4);
					float glyph = texture(f_tex, (glyph_coord + glyph_fract) / 16.0).a;

					// compose glyph
					color = vec4(mix(vec3(fill_rgbp.rgb), vec3(fill_rgbc.rgb), glyph) / 255.0, 1.0);
					//color = vec4(glyph_fract, 0.5, 1.0);

					if (mode == 0) // editing
						color.rgb *= light;
				}

				// color.rgb *= lc;

				// at the moment we assume that visual is simply RGB565 color
				/*
				color.r = float(visual & 0x1f) / 31.0;
				color.g = float((visual>>5) & 0x3f) / 63.0;
				color.b = float((visual>>11) & 0x1f) / 31.0;
				color.a = 1;
				*/

				if (qd.z>0)
				{
					// diagonal flip preview
					float d = float(VISUAL_CELLS) / float(HEIGHT_CELLS);
					if (world_xyuv.x >= qd.x && world_xyuv.x < qd.x + d &&
						world_xyuv.y >= qd.y && world_xyuv.y < qd.y + d)
					{
						//color.rb = mix(color.rb, color.rb * 0.5, qd.z);
						color.rgb = mix(color.rgb, vec3(0, 1, 0), qd.z*0.25);
					}
				}
				else
				if (qd.z < 0)
				{
					float d = float(VISUAL_CELLS);
					// patch delete preview
					if (world_xyuv.x >= qd.x && world_xyuv.x < qd.x + d &&
						world_xyuv.y >= qd.y && world_xyuv.y < qd.y + d)
					{
						//color.rb = mix(color.rb, color.rb * 0.5, qd.z);
						color.rgb = mix(color.rgb, vec3(1, .2, 0), -qd.z*0.25);
					}
				}

				{
					// height probe
					if (uvh.z * HEIGHT_SCALE < pr.x)
					{
						//color.g *= (1.0 - 0.25 * pr.y);
						color.rgb = mix(color.rgb, vec3(0.25, 0.5, 0.75), 0.1 + 0.1 * pr.y);
					}
				}

				if (!gl_FrontFacing)
					color.rgb = 0.25 * (vec3(1.0) - color.rgb);

				float dx = 1.25*length(vec2(dFdxFine(uvh.x), dFdyFine(uvh.x)));
				float dy = 1.25*length(vec2(dFdxFine(uvh.y), dFdyFine(uvh.y)));

				vec2 d = vec2(dx, dy);

				float grid = 1.0;
				grid = min(grid, Grid(d*1.50, uvh.xy, 1.0 / float(HEIGHT_CELLS)));
				grid = min(grid, Grid(d*1.25, uvh.xy, 1.0));
				grid = min(grid, Grid(d*1.00, uvh.xy, float(VISUAL_CELLS) / float(HEIGHT_CELLS)));

				// color.rgb *= grid;
				color.rgb = mix(vec3(0, 0, 1), color.rgb, grid);

				// brush preview
				if (br.w != 0.0)
				{
					float abs_r = abs(br.z);
					float len = length(world_xyuv.xy - br.xy);
					float alf = (abs_r - len) / abs_r;

					float dalf = fwidth(alf);
					float silh = smoothstep(-dalf, 0, alf) * smoothstep(+dalf, 0, alf);

					alf = max(0.0, alf);

					if (br.z>0)
						color.gb *= 1.0 - alf;
					else
						color.rg *= 1.0 - alf;

					color.rgb *= 1.0 - silh*0.25;
				}
			}
		);

		err = glGetError();

		GLsizei loglen = 999;
		char logstr[1000];

		GLenum ghost_st[3] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
		const char* ghost_src[3] = { ghost_vs_src, ghost_fs_src };
		ghost_prg = glCreateProgram();
		GLuint shader[3];

		for (int i = 0; i < 2; i++)
		{
			shader[i] = glCreateShader(ghost_st[i]);
			GLint len = (GLint)strlen(ghost_src[i]);
			glShaderSource(shader[i], 1, &(ghost_src[i]), &len);
			glCompileShader(shader[i]);

			loglen = 999;
			glGetShaderInfoLog(shader[i], loglen, &loglen, logstr);
			logstr[loglen] = 0;

			if (loglen)
				printf("%s", logstr);

			glAttachShader(ghost_prg, shader[i]);
		}

		glLinkProgram(ghost_prg);

		for (int i = 0; i < 2; i++)
			glDeleteShader(shader[i]);

		loglen = 999;
		glGetProgramInfoLog(ghost_prg, loglen, &loglen, logstr);
		logstr[loglen] = 0;

		if (loglen)
			printf("%s", logstr);

		ghost_tm_loc = glGetUniformLocation(ghost_prg, "tm");
		ghost_cl_loc = glGetUniformLocation(ghost_prg, "cl");

		prg = glCreateProgram();

		GLenum st[3] = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };
		const char* src[3] = { vs_src, gs_src, fs_src };

		for (int i = 0; i < 3; i++)
		{
			shader[i] = glCreateShader(st[i]);
			GLint len = (GLint)strlen(src[i]);
			glShaderSource(shader[i], 1, &(src[i]), &len);
			glCompileShader(shader[i]);

			loglen = 999;
			glGetShaderInfoLog(shader[i], loglen, &loglen, logstr);
			logstr[loglen] = 0;

			if (loglen)
				printf("%s", logstr);

			glAttachShader(prg, shader[i]);
		}

		glLinkProgram(prg);

		for (int i = 0; i < 3; i++)
			glDeleteShader(shader[i]);

		loglen = 999;
		glGetProgramInfoLog(prg, loglen, &loglen, logstr);
		logstr[loglen] = 0;

		if (loglen)
			printf("%s", logstr);

		tm_loc = glGetUniformLocation(prg, "tm");
		z_tex_loc = glGetUniformLocation(prg, "z_tex");
		v_tex_loc = glGetUniformLocation(prg, "v_tex");
		m_tex_loc = glGetUniformLocation(prg, "m_tex");
		f_tex_loc = glGetUniformLocation(prg, "f_tex");
		br_loc = glGetUniformLocation(prg, "br");
		qd_loc = glGetUniformLocation(prg, "qd");
		pr_loc = glGetUniformLocation(prg, "pr");
		lt_loc = glGetUniformLocation(prg, "lt");
		//lc_loc = glGetUniformLocation(prg, "lc");
		fz_loc = glGetUniformLocation(prg, "fz");
	}

	void Delete()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteProgram(prg);

		glDeleteVertexArrays(1, &ghost_vao);
		glDeleteBuffers(1, &ghost_vbo);
		glDeleteProgram(ghost_prg);

	}

	void PaintGhost(const double* tm, int px, int py, int pz, uint16_t ghost[4 * HEIGHT_CELLS])
	{
		GLint buf[3 * 4 * HEIGHT_CELLS];
		int g = 0, b = 0;

		px *= HEIGHT_CELLS;
		py *= HEIGHT_CELLS;

		for (int x = 0; x < HEIGHT_CELLS; x++)
		{
			buf[b++] = px + x;
			buf[b++] = py;
			buf[b++] = ghost[g++];
		}

		for (int y = 0; y < HEIGHT_CELLS; y++)
		{
			buf[b++] = px + HEIGHT_CELLS;
			buf[b++] = py + y;
			buf[b++] = ghost[g++];
		}

		for (int x = HEIGHT_CELLS; x > 0; x--)
		{
			buf[b++] = px + x;
			buf[b++] = py + HEIGHT_CELLS;
			buf[b++] = ghost[g++];
		}

		for (int y = HEIGHT_CELLS; y > 0; y--)
		{
			buf[b++] = px;
			buf[b++] = py + y;
			buf[b++] = ghost[g++];
		}

		float ftm[16];// NV bug! workaround
		for (int i = 0; i < 16; i++)
			ftm[i] = (float)tm[i];

		glBindVertexArray(ghost_vao);
		glUseProgram(ghost_prg);

		glUniformMatrix4fv(ghost_tm_loc, 1, GL_FALSE, ftm);

		glNamedBufferSubData(ghost_vbo, 0, sizeof(GLint[3 * 4 * HEIGHT_CELLS]), buf);

		glUniform4f(ghost_cl_loc, 0, 0, 0, 1.0f);
		glLineWidth(2.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 4 * HEIGHT_CELLS);
		glLineWidth(1.0f);

		// flatten
		for (b = 0; b < 4 * HEIGHT_CELLS; b++)
			buf[3 * b + 2] = pz;
		glNamedBufferSubData(ghost_vbo, 0, sizeof(GLint[3 * 4 * HEIGHT_CELLS]), buf);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUniform4f(ghost_cl_loc, 0, 0, 0, 0.2f);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4 * HEIGHT_CELLS);

		glDisable(GL_BLEND);

		glUseProgram(0);
		glBindVertexArray(0);
	}

	void BeginPatches(const double* tm, const float* lt, const float* br, const float* qd, const float* pr)
	{
		glUseProgram(prg);

		static const float br_off[] = { 0,0,1,0 };
		if (!br)
			br = br_off;

		/*
		float* c1;
		float* c2;
		float w;
		if (lit_time < 6)
		{
			w = lit_time / 6.0f;
			c1 = midnight_color;
			c2 = dawn_color;
		}
		else
		if (lit_time < 12)
		{
			w = powf((lit_time-6) / 6.0f, 0.3f);
			c1 = dawn_color;
			c2 = noon_color;
		}
		else
		if (lit_time < 18)
		{
			w = 1.0f - powf(1.0f - (lit_time - 12) / 6.0f, 0.3f);
			c1 = noon_color;
			c2 = dusk_color;
		}
		else
		{
			w = (lit_time - 18) / 6.0f;
			c1 = dusk_color;
			c2 = midnight_color;
		}

		float lit_color[3];
		for (int c=0; c<3; c++)
			lit_color[c] = c1[c]*(1-w) + c2[c]*w;
		*/

		//glUniformMatrix4dv(tm_loc, 1, GL_FALSE, tm);
		float ftm[16];// NV bug! workaround
		for (int i = 0; i < 16; i++)
			ftm[i] = (float)tm[i];

		double font_zoom; // calc using lengths of diagonals

		font_zoom = font[active_font].width * font[active_font].width + font[active_font].height * font[active_font].height;
		font_zoom /= 512.0 * font_size * font_size; 
		font_zoom = sqrt(font_zoom);

		glUniformMatrix4fv(tm_loc, 1, GL_FALSE, ftm);
		glUniform3fv(lt_loc, 1, lt);
		//glUniform3fv(lc_loc, 1, lit_color);
		glUniform1i(z_tex_loc, 0);
		glUniform1i(v_tex_loc, 1);
		glUniform1i(m_tex_loc, 2);
		glUniform1i(f_tex_loc, 3);
		glUniform4fv(br_loc, 1, br);
		glUniform3fv(qd_loc, 1, qd);
		glUniform3fv(pr_loc, 1, pr);
		glUniform1f(fz_loc, (float)font_zoom);
		glBindVertexArray(vao);

		glBindTextureUnit(2, MyMaterial::tex);
		glBindTextureUnit(3, font[active_font].tex);

		head = 0;
		patches = 0;
		draws = 0;
		changes = 0;
		page_tex = 0;

		render_time = a3dGetTime();
	}

	static void RenderPatch(Patch* p, int x, int y, int view_flags, void* cookie)
	{
		RenderContext* rc = (RenderContext*)cookie;
		TexAlloc* ta = GetTerrainTexAlloc(p);

		rc->patches++;

		TexPageBuffer* buf = (TexPageBuffer*)ta->page->user;

		if (buf->size == 0)
		{
			if (rc->head)
				((TexPageBuffer*)rc->head->user)->prev = ta->page;
			buf->prev = 0;
			buf->next = rc->head;
			rc->head = ta->page;
		}

		GLint* patch = buf->data + 5 * buf->size;

		patch[0] = x;
		patch[1] = y;
		patch[2] = ta->x;
		patch[3] = ta->y;
		patch[4] = GetTerrainDiag(p);

		buf->size++;

		if (buf->size == TERRAIN_TEXHEAP_CAPACITY)
		{
			rc->draws++;
			
			if (rc->page_tex != ta->page)
			{
				rc->changes++;
				rc->page_tex = ta->page;

				for (int u=0; u<2; u++)
					glBindTextureUnit(u, rc->page_tex->tex[u]);
			}

			glNamedBufferSubData(rc->vbo, 0, sizeof(GLint[5]) * buf->size, buf->data);
			glDrawArrays(GL_POINTS, 0, buf->size);

			if (buf->prev)
				((TexPageBuffer*)buf->prev->user)->next = buf->next;
			else
				rc->head = buf->next;

			if (buf->next)
				((TexPageBuffer*)buf->next->user)->prev = buf->prev;

			buf->size = 0;
			buf->next = 0;
			buf->prev = 0;
		}

	}

	void EndPatches()
	{
		TexPage* tp = head;
		while (tp)
		{
			TexPageBuffer* buf = (TexPageBuffer*)tp->user;

			if (page_tex != tp)
			{
				changes++;
				page_tex = tp;

				for (int u=0; u<2; u++)
					glBindTextureUnit(u, page_tex->tex[u]);
			}

			draws++;
			glNamedBufferSubData(vbo, 0, sizeof(GLint[5]) * buf->size, buf->data);
			glDrawArrays(GL_POINTS, 0, buf->size);

			tp = buf->next;
			buf->size = 0;
			buf->next = 0;
			buf->prev = 0;
		}

		page_tex = 0;
		head = 0;

		for (int u = 0; u < 4; u++)
			glBindTextureUnit(u,0);

		glBindVertexArray(0);
		glUseProgram(0);

		render_time = a3dGetTime() - render_time;
	}

	GLint tm_loc; // uniform
	GLint lt_loc;
	//GLint lc_loc;
	GLint z_tex_loc;
	GLint v_tex_loc;
	GLint m_tex_loc;
	GLint f_tex_loc;

	GLint br_loc;
	GLint qd_loc;
	GLint pr_loc;

	GLint fz_loc;

	GLuint prg;
	GLuint vao;
	GLuint vbo;

	GLuint ghost_prg;
	GLuint ghost_vbo;
	GLuint ghost_vao;
	GLint ghost_tm_loc;
	GLint ghost_cl_loc;

	TexPage* page_tex;
	TexPage* head;

	int patches; // rendered stats
	int draws;
	int changes;
	uint64_t render_time;
};

RenderContext render_context;

void GL_APIENTRY glDebugCall(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	static const char* source_str[] = // 0x8246 - 0x824B
	{
		"API",
		"WINDOW_SYSTEM",
		"SHADER_COMPILER",
		"THIRD_PARTY",
		"APPLICATION",
		"OTHER"
	};

	const char* src = "?";
	if (source >= 0x8246 && source <= 0x824B)
		src = source_str[source - 0x8246];

	static const char* type_str[] = // 0x824C - 0x8251
	{
		"ERROR",
		"DEPRECATED_BEHAVIOR",
		"UNDEFINED_BEHAVIOR",
		"PORTABILITY",
		"PERFORMANCE",
		"OTHER"
	};

	const char* typ = "?";
	if (type >= 0x824C && type <= 0x8251)
		typ = type_str[type - 0x824C];

	static const char* severity_str[] = // 0x9146 - 0x9148 , 0x826B
	{
		"HIGH",
		"MEDIUM",
		"LOW",
		"NOTIFICATION",
	};

	const char* sev = "?";
	if (severity >= 0x9146 && severity <= 0x9148)
		sev = severity_str[severity - 0x9146];
	else
		if (severity == 0x826B)
		{
			return;
			sev = severity_str[3];
		}

	printf("src:%s type:%s id:%d severity:%s\n%s\n\n", src, typ, id, sev, (const char*)message);
}


struct Gather
{
	int x, y; // patch aligned
	int count; // number of actually queried patches
	int size; // in patches
	int* tmp_x;
	int* tmp_y;
	Patch* patch[1];

	int GetPatchIdx(int px, int py)
	{
		int dx = px - x;
		int dy = py - y;

		int bx = dx / VISUAL_CELLS;
		int by = dy / VISUAL_CELLS;

		assert(bx >= 0 && bx < size && by >= 0 && by < size);
		return bx + by * size;
	}

	int Sample(int hx, int hy) // hx and hy are in height map samples relative to Gather::x,y
	{
		int px = hx / HEIGHT_CELLS;
		int py = hy / HEIGHT_CELLS;

		int sx = hx % HEIGHT_CELLS;
		int sy = hy % HEIGHT_CELLS;

		int idx = px + py * size;
		Patch* p = patch[idx];

		if (!p)
			return -1;

		uint16_t* map = GetTerrainHeightMap(p);

		return map[sx + sy * (HEIGHT_CELLS + 1)];
	}
};

Gather* gather = 0;

static void GatherCB(Patch* p, int x, int y, int view_flags, void* cookie)
{
	gather->count++;
	gather->patch[gather->GetPatchIdx(x, y)] = p;
}

static void StampCB(Patch* p, int x, int y, int view_flags, void* cookie)
{
	double mul = br_alpha * br_radius * HEIGHT_SCALE;
	if (fabs(mul) < 0.499)
		return;

	uint16_t lo, hi;
	GetTerrainLimits(p, &lo, &hi);
	if (hi == 0 && br_alpha < 0 || lo == 0xffff && br_alpha>0)
		return;

	URDO_Patch(p);

	double* xy = (double*)cookie;
	uint16_t* map = GetTerrainHeightMap(p);

	const static double sxy = (double)VISUAL_CELLS / (double)HEIGHT_CELLS;

	double max_r2 = 0;

	for (int i=0, hy = 0; hy <= HEIGHT_CELLS; hy++)
	{
		double dy = y + sxy * hy - xy[1];
		dy *= dy;
		for (int hx = 0; hx <= HEIGHT_CELLS; hx++, i++)
		{
			double dx = x + sxy * hx - xy[0];
			dx *= dx;

			double len = sqrt(dx + dy);
			if (len < br_radius)
			{
				double gauss = 0.5 + 0.5*cos(len / br_radius * M_PI);

				int d = (int)(round(gauss*gauss * mul));
				if (d)
					max_r2 = fmax(max_r2, dx + dy);

				int z = map[i] + d;

				if (br_limit)
				{
					if (d > 0)
					{
						if (map[i] > probe_z)
							z = map[i];
						else
						if (z > probe_z)
							z = probe_z;
					}
					else
					if (d < 0)
					{
						if (map[i] < probe_z)
							z = map[i];
						else
						if (z < probe_z)
							z = probe_z;
					}
				}
				else
				{
					if (z < 0)
						z = 0;
					if (z > 0xffff)
						z = 0xffff;
				}
				map[i] = z;
			}
		}
	}

	xy[2] = fmax(xy[2], max_r2);
	UpdateTerrainHeightMap(p);
}

void Stamp(double x, double y)
{
	// query all patches int radial range br_xyra[2] from x,y
	// get their heightmaps apply brush on height samples and update TexHeap pages 

	ImGuiIO& io = ImGui::GetIO();

	int stamp_mode = 1;
	if (io.KeysDown[A3D_LSHIFT])
		stamp_mode = 2;

	if (stamp_mode == 1)
	{
		URDO_Open();
		double xy[3] = { x,y,0 };
		QueryTerrain(terrain, x, y, br_radius * 1.001, 0x00, StampCB, xy);
		URDO_Close();
	}
	else
	{
		double mul = br_alpha * br_radius * HEIGHT_SCALE;
		if (fabs(mul) < 0.499)
			return;

		// gather
		int size = 4 * (int)ceil(br_radius / VISUAL_CELLS) + 2;
		int tmp_buf_size = sizeof(int)*(size*HEIGHT_CELLS)*(size*HEIGHT_CELLS);
		if (!gather || gather->size != size)
		{
			if (gather)
			{
				free(gather->tmp_x);
				free(gather->tmp_y);
				free(gather);
			}
			int bs = sizeof(Gather) + sizeof(Patch*)*(size*size - 1);
			gather = (Gather*)malloc(bs);
			gather->size = size;

			gather->tmp_x = (int*)malloc(tmp_buf_size);
			gather->tmp_y = (int*)malloc(tmp_buf_size);
		}

		memset(gather->patch, 0, sizeof(Patch*)*(size*size));

		gather->x = (int)floor(x / VISUAL_CELLS - 0.5 * size) * VISUAL_CELLS;
		gather->y = (int)floor(y / VISUAL_CELLS - 0.5 * size) * VISUAL_CELLS;

		gather->count=0;
		QueryTerrain(terrain, x, y, 2.0*br_radius, 0x00, GatherCB, 0);

		if (!gather->count)
			return;

		int* tmp_x = gather->tmp_x;
		memset(tmp_x, -1, tmp_buf_size);

		int r = (int)floor(br_radius * HEIGHT_CELLS / VISUAL_CELLS);
		for (int hy = 0; hy < size * HEIGHT_CELLS; hy++)
		{
			for (int hx = r; hx < size * HEIGHT_CELLS - r; hx++)
			{
				double acc = 0;
				double den = 0;

				for (int sx = hx-r; sx < hx+r; sx++)
				{
					int h = gather->Sample(sx, hy);
					if (h >= 0)
					{
						// HERE we use TRUE gaussian filter (must be separable)
						double len = (double)sx * VISUAL_CELLS / HEIGHT_CELLS + gather->x - x;
						len /= br_radius;
						double gauss = exp(-len * len * 3);

						acc += h * gauss;
						den += gauss;
					}
				}

				if (den > 0)
					tmp_x[hx + hy * size * HEIGHT_CELLS] = (uint16_t)round(acc / den);
				else
					tmp_x[hx + hy * size * HEIGHT_CELLS] = -1;
			}
		}

		int* tmp_y = gather->tmp_y;
		memset(tmp_y, -1, tmp_buf_size);

		for (int hy = r; hy < size * HEIGHT_CELLS - r; hy++)
		{
			for (int hx = r; hx < size * HEIGHT_CELLS - r; hx++)
			{
				double acc = 0;
				double den = 0;

				for (int sy = hy - r; sy < hy + r; sy++)
				{
					int h = tmp_x[hx + sy * size * HEIGHT_CELLS];
					if (h >= 0)
					{
						// HERE we use TRUE gaussian filter (must be separable)
						double len = (double)sy * VISUAL_CELLS / HEIGHT_CELLS + gather->y - y;
						len /= br_radius;
						double gauss = exp(-len*len*3);

						acc += h * gauss;
						den += gauss;
					}
				}

				if (den > 0)
					tmp_y[hx + hy * size * HEIGHT_CELLS] = (uint16_t)round(acc / den);
				else
					tmp_y[hx + hy * size * HEIGHT_CELLS] = -1;
			}
		}

		// run all patches
		URDO_Open();
		for (int py = gather->size/4; py < gather->size - gather->size / 4; py++)
		{
			for (int px = gather->size / 4; px < gather->size - gather->size / 4; px++)
			{
				Patch* p = gather->patch[px + size * py];
				if (p)
				{
					URDO_Patch(p);
					uint16_t* map = GetTerrainHeightMap(p);

					for (int sy = 0; sy <= HEIGHT_CELLS; sy++)
					{
						int hy = (HEIGHT_CELLS * py + sy);
						double dy = gather->y + hy * VISUAL_CELLS / (double)HEIGHT_CELLS - y;
						dy *= dy;
						for (int sx = 0; sx <= HEIGHT_CELLS; sx++)
						{
							int hx = (HEIGHT_CELLS * px + sx);
							double dx = gather->x + hx * VISUAL_CELLS / (double)HEIGHT_CELLS - x;
							dx *= dx;

							double len = sqrt(dx + dy);

							if (len < br_radius)
							{
								double gauss = 0.5 + 0.5*cos(len / br_radius * M_PI);
								gauss *= gauss * br_alpha;

								if (gauss < 0)
								{
									double diff = gauss * (tmp_y[hx + hy * size * HEIGHT_CELLS] - map[sx + sy * (HEIGHT_CELLS + 1)]);
									int z = (int)round(diff) + map[sx + sy * (HEIGHT_CELLS + 1)];
									if (z < 0)
										z = 0;
									if (z > 0xffff)
										z = 0xffff;

									map[sx + sy * (HEIGHT_CELLS + 1)] = z;
								}
								else
								{
									double blend = map[sx + sy * (HEIGHT_CELLS + 1)] * (1.0 - gauss);
									blend += tmp_y[hx + hy * size * HEIGHT_CELLS] * gauss;
									map[sx + sy * (HEIGHT_CELLS + 1)] = (uint16_t)round(blend);
								}
							}
						}
					}

					UpdateTerrainHeightMap(p);
				}
			}
		}
		URDO_Close();
	}
}

void my_render()
{
	ImGuiIO& io = ImGui::GetIO();

	// THINGZ
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	{
		ImGui_ImplOpenGL3_NewFrame();
		{
			// Setup time step
			ImGuiIO& io = ImGui::GetIO();
			uint64_t current_time = a3dGetTime();
			uint64_t delta = current_time - g_Time;
			io.DeltaTime = delta>0 ? delta / 1000000.0f : FLT_MIN;
			g_Time = current_time;
			// Start the frame
			ImGui::NewFrame();
		}

//		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
//		ImGui::SetNextWindowPos(ImVec2(0,0),ImGuiCond_Always);
		//ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(0,0),Dock::Size,0);
//		ImGui::PopStyleVar();

		ImGui::Begin("VIEW", 0, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::CollapsingHeader("View Control", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("VIEW PITCH", &rot_pitch, +30.0f, +90.0f);

			ImGui::SliderFloat("VIEW YAW", &rot_yaw, -180.0f, +180.0f); ImGui::SameLine();
			ImGui::Checkbox("Spin", &spin_anim);

			ImGui::SliderFloat("ZOOM", &font_size, 1.0f, 32.0f);
			ImGui::SameLine();
			ImGui::Text("%dx%d", (int)round(io.DisplaySize.x/font_size), (int)round(io.DisplaySize.y / font_size));
		}

		if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("PATCHES: %d, DRAWS: %d, CHANGES: %d", render_context.patches, render_context.draws, render_context.changes);
			ImGui::Text("RENDER TIME: %6jd [" /*micro*/"\xc2\xb5"/*utf8*/ "s]", render_context.render_time);
			ImGui::Text("%zu BYTES", GetTerrainBytes(terrain));
		}

		if (ImGui::CollapsingHeader("Light Control", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("NOON PITCH", &lit_pitch, 0.0f, +90.0f);
			ImGui::SliderFloat("NOON YAW", &lit_yaw, -180.0f, +180.0f);
			ImGui::SliderFloat("LIGHT TIME", &lit_time, 0, 24);

			/*
			ImGui::ColorEdit3("DAWN", dawn_color);
			ImGui::ColorEdit3("NOON", noon_color);
			ImGui::ColorEdit3("DUSK", dusk_color);
			ImGui::ColorEdit3("MIDNIGHT", midnight_color);
			*/
		}

		ImGui::End();
		/// end of window?
		ImGui::Begin("EDIT", 0, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::CollapsingHeader("Undo / Redo", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (!URDO_CanUndo())
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				ImGui::Button("<<");
				ImGui::SameLine();
				ImGui::Button("<");
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			else
			{
				if (ImGui::Button("<<") || ImGui::IsItemActive() && io.MouseDownDuration[0] > .25f)
					URDO_Undo(0);
				ImGui::SameLine();
				if (ImGui::Button("<") || ImGui::IsItemActive() && io.MouseDownDuration[0] > .25f)
					URDO_Undo(1);
			}
			ImGui::SameLine();
			if (!URDO_CanRedo())
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				ImGui::Button(">");
				ImGui::SameLine();
				ImGui::Button(">>");
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			else
			{
				if (ImGui::Button(">") || ImGui::IsItemActive() && io.MouseDownDuration[0] > .25f)
					URDO_Redo(1);
				ImGui::SameLine();
				if (ImGui::Button(">>") || ImGui::IsItemActive() && io.MouseDownDuration[0] > .25f)
					URDO_Redo(0);
			}
			ImGui::SameLine();
			if (!URDO_CanRedo() && !URDO_CanUndo())
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				ImGui::Button("PURGE");
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			else
				if (ImGui::Button("PURGE"))
					URDO_Purge();
			ImGui::SameLine();
			ImGui::Text("%zu BYTES", URDO_Bytes());
		}

		if (ImGui::CollapsingHeader("Brush", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
			if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
			{
				static int which = -1;
				bool pushed = false;

				if (which != 0)
				{
					pushed = true;
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
				if (ImGui::BeginTabItem("SCULPT"))
				{
					which = 0;
					ImGui::Text("Sculpting modifies terrain height map \n ");

					const char* mode = "";

					if (!painting && io.KeyCtrl && io.KeyShift)
					{
						mode = "HEIGHT PROBE";
					}
					else
					if (!painting && io.KeyCtrl)
						mode = "DIAGONAL FLIP";
					else
					{
						if (io.KeyShift)
							mode = br_alpha >= 0 ? "BLURRING" : "SHARPENING";
						else
							mode = br_alpha >= 0 ? "ASCENT" : "DESCENT";
					}

					ImGui::Text("MODE (shift/ctrl): %s", mode);
					ImGui::SliderFloat("BRUSH RADIUS", &br_radius, 5.f, 100.f);
					ImGui::SliderFloat("BRUSH ALPHA", &br_alpha, -0.5f, +0.5f);


					ImGui::Checkbox("BRUSH HEIGHT LIMIT",&br_limit);
					ImGui::SameLine();

					// Arrow buttons with Repeater
					float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
					ImGui::PushButtonRepeat(true);
					if (ImGui::ArrowButton("##probe_left", ImGuiDir_Left)) { if (probe_z>0) probe_z-=1; }
					ImGui::SameLine(0.0f, spacing);
					if (ImGui::ArrowButton("##probe_right", ImGuiDir_Right)) { if (probe_z<0xffff) probe_z+=1; }
					ImGui::PopButtonRepeat();
					ImGui::SameLine();
					ImGui::Text("%d", probe_z);
					ImGui::Text("%s", "ctrl+shift to probe");

					// ImGui::SliderFloat("BRUSH HEIGHT", &probe_z, 0.0f, 65535.0f);

					ImGui::EndTabItem();
				}
				if (pushed)
				{
					pushed = false;
					ImGui::PopStyleVar();
				}

				if (which != 1)
				{
					pushed = true;
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
				if (ImGui::BeginTabItem("MAT-id"))
				{
					which = 1;
					ImGui::Text("Material channel selects which material \ndefinition should be used (0-255)");
					ImGui::EndTabItem();
				}
				if (pushed)
				{
					pushed = false;
					ImGui::PopStyleVar();
				}

				if (which != 2)
				{
					pushed = true;
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
				if (ImGui::BeginTabItem("sh-MODE"))
				{
					which = 2;
					ImGui::Text("Shade mode channel specifies how lighting \naffects shading ramp (0-3)");
					ImGui::EndTabItem();
				}
				if (pushed)
				{
					pushed = false;
					ImGui::PopStyleVar();
				}

				if (which != 3)
				{
					pushed = true;
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
				if (ImGui::BeginTabItem("sh-RAMP"))
				{
					which = 3;
					ImGui::Text("Shade ramp channel selects a cell \nhorizontaly from a material ramps (0-15)");
					ImGui::EndTabItem();
				}
				if (pushed)
				{
					pushed = false;
					ImGui::PopStyleVar();
				}


				if (which != 4)
				{
					pushed = true;
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
				if (ImGui::BeginTabItem("ELEV"))
				{
					which = 4;
					ImGui::Text("Elevation bits are used to choose ramps \nvertically from material by bit difference");
					ImGui::EndTabItem();
				}
				if (pushed)
				{
					pushed = false;
					ImGui::PopStyleVar();
				}

				ImGui::EndTabBar();
				ImGui::Separator();

			}
		}

		ImGui::End();
		/// end of window?
		ImGui::Begin("FONT", 0, ImGuiWindowFlags_AlwaysAutoResize);

		// fonts related stuff
		float font_width = (float)font[active_font].width;
		float font_height = (float)font[active_font].height;
		if (font_width<256)
		{
			font_width = 256;
			font_height *= 256.0f / font[active_font].width;
		}

		int glyph_w = font[active_font].width / 16;
		int glyph_h = font[active_font].height / 16;
		float glyph_x = (active_glyph & 0xf) * glyph_w / (float)font[active_font].width;
		float glyph_y = (active_glyph >> 4) * glyph_h / (float)font[active_font].height;
		float texel_w = 1.0f / font[active_font].width;
		float texel_h = 1.0f / font[active_font].height;
		float but_w = 13 + 48.0f / (font_width / 16);	

		float but16_w = font_width / 16;
		float but16_h = font_height / 16;



		if (fonts_loaded && ImGui::CollapsingHeader("Fonts", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##fnt_left", ImGuiDir_Left)) { if (active_font > 0) active_font--; }
			ImGui::SameLine(0.0f, spacing);
			if (ImGui::ArrowButton("##fnt_right", ImGuiDir_Right)) { if (active_font < fonts_loaded-1) active_font++; }
			ImGui::PopButtonRepeat();
			ImGui::SameLine();
			ImGui::Text("0x%02X (%d)", active_font, active_font); // path?

			ImGui::Text("CELL SIZE: %dx%d px", font[active_font].width/16, font[active_font].height/16);
			//ImGui::Image((void*)(intptr_t)font.tex, ImVec2(font.width,font.height), ImVec2(0,1), ImVec2(1,0));

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImVec4 tint_normal(1, 1, 1, 0.33f);
			ImVec4 tint_onedim(1, 1, 1, 0.50f);
			ImVec4 tint_active(1, 1, 1, 1.00f);
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					ImVec4* tint = &tint_normal;

					bool pushed = false;
					if (x + y*16 == active_glyph)
					{
						ImVec4 hi = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
						ImGui::PushStyleColor(ImGuiCol_Button,hi);
						tint = &tint_active;
						pushed = true;
					}
					else
					if (x == (active_glyph & 0xf) || y == (active_glyph>>4))
						tint = &tint_onedim;

					ImGui::PushID(x + y * 16);
					if (ImGui::ImageButton((void*)(intptr_t)font[active_font].tex, 
						//ImVec2(font[active_font].width / 16.f, font[active_font].height / 16.f),
						ImVec2(font_width / 16.f, font_height / 16.f),
						ImVec2(x / 16.0f, y / 16.0f), ImVec2((x + 1) / 16.0f, (y + 1)/ 16.0f), 1, ImVec4(0, 0, 0, 0), *tint))
					{
						active_glyph = x + y * 16;
					}
					ImGui::PopID();

					if (pushed)
						ImGui::PopStyleColor();

					if (x<15)
						ImGui::SameLine();
				}
			}
			ImGui::PopStyleVar();
		}

		if (fonts_loaded && ImGui::CollapsingHeader("Character", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##chr_left", ImGuiDir_Left)) { if (active_glyph > 0) active_glyph--; }
			ImGui::SameLine(0.0f, spacing);
			if (ImGui::ArrowButton("##chr_right", ImGuiDir_Right)) { if (active_glyph < 0xff) active_glyph++; }
			ImGui::PopButtonRepeat();
			ImGui::SameLine();
			ImGui::Text("0x%02X (%d)", active_glyph, active_glyph);


			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			for (int y = 0; y < glyph_h ; y++)
			{
				for (int x = 0; x < glyph_w; x++)
				{
					ImGui::PushID(x + y * glyph_w + 256);
					if (ImGui::ImageButton((void*)(intptr_t)font[active_font].tex, ImVec2(but_w, but_w),
						ImVec2(glyph_x + x*texel_w, glyph_y + y*texel_h), ImVec2(glyph_x + (x+1)*texel_w, glyph_y + (y+1)*texel_h), 
						1, ImVec4(0,0,0,.5f), ImVec4(1,1,1,.5)))
					{
						int u = x + glyph_w*(active_glyph&0xF);
						int v = y + glyph_h*(active_glyph>>4);
						// tick that pixel
						uint8_t p = font[active_font].GetTexel(u,v);
						p ^=0xFF;
						font[active_font].SetTexel(u,v,p);
					}
					ImGui::PopID();

					if (x < glyph_w-1)
						ImGui::SameLine();
				}
			}

			ImGui::PopStyleVar();
		}

		ImGui::End();
		/// end of window?
		ImGui::Begin("SKIN", 0, ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::CollapsingHeader("Palettes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##pal_left", ImGuiDir_Left)) { if (active_palette > 0) active_palette--; }
			ImGui::SameLine(0.0f, spacing);
			if (ImGui::ArrowButton("##pal_right", ImGuiDir_Right)) { if (active_palette < 0xff) active_palette++; }
			ImGui::PopButtonRepeat();
			ImGui::SameLine();
			ImGui::Text("0x%02X (%d)", active_palette, active_palette);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					ImVec4 tint(
						pal[active_palette].rgb[3 * (x + 16 * y) + 0] / 255.0f, 
						pal[active_palette].rgb[3 * (x + 16 * y) + 1] / 255.0f,
						pal[active_palette].rgb[3 * (x + 16 * y) + 2] / 255.0f,
						1.0
					);

#if 0
					ImGui::PushID(x + y * 16 + 256 + glyph_w * glyph_h);
					if (ImGui::ImageButton(0/*samples black!*/,
						//ImVec2(glyph_w, glyph_w), 
						ImVec2(but16_w, but16_h),
						ImVec2(0,0), ImVec2(1,1), 1, tint, ImVec4(0,0,0,0)))
					{
						// select that color
					}
					ImGui::PopID();
#endif
					ImGui::PushID(x + y * 16 + 256 + glyph_w * glyph_h);

					if (ImGui::ColorEdit3("", (float*)&tint, ImGuiColorEditFlags_NoInputs, ImVec2(but16_w + 2, but16_h + 2)))
					{
						pal[active_palette].rgb[3 * (x + 16 * y) + 0] = (int)round(tint.x * 255);
						pal[active_palette].rgb[3 * (x + 16 * y) + 1] = (int)round(tint.y * 255);
						pal[active_palette].rgb[3 * (x + 16 * y) + 2] = (int)round(tint.z * 255);
					}
					
					ImGui::PopID();

					if (x < 15)
						ImGui::SameLine();
				}
			}

			ImGui::PopStyleVar();
		}

		if (fonts_loaded && ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			ImGui::PushButtonRepeat(true);
			if (ImGui::ArrowButton("##mat_left", ImGuiDir_Left)) { if (active_material > 0) active_material--; }
			ImGui::SameLine(0.0f, spacing);
			if (ImGui::ArrowButton("##mat_right", ImGuiDir_Right)) { if (active_material < 0xff) active_material++; }
			ImGui::PopButtonRepeat();
			ImGui::SameLine();
			ImGui::Text("0x%02X (%d)", active_material, active_material);

			static bool paint_mat_glyph = true;
			static bool paint_mat_foreground = true;
			static bool paint_mat_background = true;

			static float paint_mat_fg[3] = { .2f, .3f, .4f };
			static float paint_mat_bg[3] = { .2f, .2f, .1f };

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			for (int y = 0; y < 3; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					float glyph_x = (mat[active_material].shade[y][x].gl & 0xF) / 16.0f;
					float glyph_y = (mat[active_material].shade[y][x].gl >> 4) / 16.0f;

					uint8_t* bg = mat[active_material].shade[y][x].bg;
					uint8_t* fg = mat[active_material].shade[y][x].fg;

					ImGui::PushID(x + y * 16 + 512 + glyph_w * glyph_h);
					if (ImGui::ImageButton((void*)(intptr_t)font[active_font].tex, 
						// ImVec2(glyph_w, glyph_h),
						ImVec2(but16_w, but16_h),
						ImVec2(glyph_x, glyph_y), ImVec2(glyph_x + 1 / 16.0f, glyph_y + 1 / 16.0f), 
						1, ImVec4(bg[0] / 255.f, bg[1] / 255.f, bg[2] / 255.f, 1), 
						ImVec4(fg[0] / 255.f, fg[1] / 255.f, fg[2] / 255.f, 1)))
					{
						if (paint_mat_glyph)
							mat[active_material].shade[y][x].gl = active_glyph;

						if (paint_mat_foreground)
						{
							fg[0] = (int)round(paint_mat_fg[0] * 255);
							fg[1] = (int)round(paint_mat_fg[1] * 255);
							fg[2] = (int)round(paint_mat_fg[2] * 255);
						}

						if (paint_mat_background)
						{
							bg[0] = (int)round(paint_mat_bg[0] * 255);
							bg[1] = (int)round(paint_mat_bg[1] * 255);
							bg[2] = (int)round(paint_mat_bg[2] * 255);
						}

						mat[active_material].Update();
					}

					if (ImGui::IsItemClicked(1) && !io.MouseDown[0])
					{
						// this is cell probe
						int a = 0;
						if (paint_mat_foreground)
						{
							paint_mat_fg[0] = fg[0] / 255.0f;
							paint_mat_fg[1] = fg[1] / 255.0f;
							paint_mat_fg[2] = fg[2] / 255.0f;
						}
						if (paint_mat_background)
						{
							paint_mat_bg[0] = bg[0] / 255.0f;
							paint_mat_bg[1] = bg[1] / 255.0f;
							paint_mat_bg[2] = bg[2] / 255.0f;
						}
						if (paint_mat_glyph)
						{
							active_glyph = mat[active_material].shade[y][x].gl;
						}
					}

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						int cookie = x + 16 * y;
						ImGui::SetDragDropPayload("DND_MAT_RAMPING", &cookie, sizeof(int));
						ImGui::Text("RAMPING");
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MAT_RAMPING"))
						{
							IM_ASSERT(payload->DataSize == sizeof(int));
							int cookie = *(const int*)payload->Data;

							int x1 = cookie & 0xF;
							int y1 = cookie >> 4;
							int x2 = x;
							int y2 = y;

							if (y1 > y2)
							{
								int s = y1;
								y1 = y2;
								y2 = s;
							}

							if (x1 > x2)
							{
								int s = x1;
								x1 = x2;
								x2 = s;
							}

							// action!
							for (int dy = y1; dy <= y2; dy++)
							{
								// read endpoints
								Cell c1 = mat[active_material].shade[dy][x1];
								Cell c2 = mat[active_material].shade[dy][x2];

								for (int dx = x1 + 1; dx < x2; dx++)
								{
									Cell* c = &(mat[active_material].shade[dy][dx]);
									float w = (float)(dx - x1) / (float)(x2 - x1);
									// interpolate
									if (paint_mat_foreground)
									{
										c->fg[0] = (int)roundf(c1.fg[0] * (1 - w) + c2.fg[0] * w);
										c->fg[1] = (int)roundf(c1.fg[1] * (1 - w) + c2.fg[1] * w);
										c->fg[2] = (int)roundf(c1.fg[2] * (1 - w) + c2.fg[2] * w);
									}
									if (paint_mat_background)
									{
										c->bg[0] = (int)roundf(c1.bg[0] * (1 - w) + c2.bg[0] * w);
										c->bg[1] = (int)roundf(c1.bg[1] * (1 - w) + c2.bg[1] * w);
										c->bg[2] = (int)roundf(c1.bg[2] * (1 - w) + c2.bg[2] * w);
									}
									if (paint_mat_glyph)
									{
										if (dx - x1 < x2 - dx)
											c->gl = c1.gl;
										else
											c->gl = c2.gl;
									}
								}
							}

							mat[active_material].Update();
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::PopID();

					if (x < 15)
						ImGui::SameLine();
				}
			}

			ImGui::PopStyleVar();

			ImGui::Separator();

			ImGui::Checkbox("Glyph", &paint_mat_glyph); ImGui::SameLine(); ImGui::Text("0x%02X (%d)", active_glyph, active_glyph);
			ImGui::Checkbox("Foreground", &paint_mat_foreground); ImGui::SameLine(); ImGui::ColorEdit3("###FG", paint_mat_fg);
			ImGui::Checkbox("Background", &paint_mat_background); ImGui::SameLine(); ImGui::ColorEdit3("###BG", paint_mat_bg);
		}

		ImGui::End();

		static bool show_demo_window = true;
		static bool show_another_window = false;

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		//if (show_demo_window)
		//	ImGui::ShowDemoWindow(&show_demo_window);

		/*

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::Text("PATCHES: %d, DRAWS: %d, CHANGES: %d", render_context.patches, render_context.draws, render_context.changes);

			ImGui::End();
		}
		*/

		// 3. Show another simple window.
		/*
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}
		*/
	}

	ImGui::Render();

	glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	RenderContext* rc = &render_context;
	double tm[16];

	// currently we're assuming: 1 visual cell = 1 font_size

	double z_scale = 1.0 / HEIGHT_SCALE; // this is a constant, (what fraction of font_size is produced by +1 height_map)

	if (!io.MouseDown[0])
	{
		diag_flipped = false;
	}

	if (!io.MouseDown[1])
	{
		spinning = 0;
	}

	if (!io.MouseDown[2])
	{
		panning = 0;
	}

	if (!io.WantCaptureMouse)
	{
		if (zoom_wheel)
		{
			font_size *= powf(1.1f, zoom_wheel);
			zoom_wheel = 0;
		}

		if (spinning)
		{
			double mdx = spinning_x - round(io.MousePos.x);
			double mdy = -(spinning_y - round(io.MousePos.y));

			rot_yaw += (float)(mdx * 0.1);
			if (rot_yaw < -180)
				rot_yaw += 360;
			if (rot_yaw > 180)
				rot_yaw -= 360;

			rot_pitch += (float)(mdy * 0.1);
			if (rot_pitch > 90)
				rot_pitch = 90;
			if (rot_pitch < 30)
				rot_pitch = 30;


			spinning_x = (int)roundf(io.MousePos.x);
			spinning_y = (int)roundf(io.MousePos.y);
		}
		else
		if (io.MouseDown[1])
		{
			spinning = 1;
			spinning_x = (int)roundf(io.MousePos.x);
			spinning_y = (int)roundf(io.MousePos.y);
		}
	}
	else
	{
		zoom_wheel = 0;
	}
	

	double rx = 0.5 * io.DisplaySize.x / font_size;
	double ry = 0.5 * io.DisplaySize.y / font_size;

	double pitch = rot_pitch * (M_PI / 180);
	double yaw = rot_yaw * (M_PI / 180);


	if (spin_anim)
	{
		rot_yaw += 0.1f;
		if (rot_yaw > 180)
			rot_yaw -= 360;
	}

	if (!io.WantCaptureMouse)
	{
		if (panning)
		{
			double mdx = panning_x - round(io.MousePos.x);
			double mdy = -(panning_y - round(io.MousePos.y)) / sin(pitch);
			pos_x = (float)(panning_dx + (mdx*cos(yaw) - mdy * sin(yaw)) / font_size);
			pos_y = (float)(panning_dy + (mdx*sin(yaw) + mdy * cos(yaw)) / font_size);

			panning_x = (int)roundf(io.MousePos.x);
			panning_y = (int)roundf(io.MousePos.y);

			panning_dx = pos_x;
			panning_dy = pos_y;
		}
		else
		if (io.MouseDown[2])
		{
			panning = 1;
			panning_x = (int)roundf(io.MousePos.x);
			panning_y = (int)roundf(io.MousePos.y);
			panning_dx = pos_x;
			panning_dy = pos_y;
		}
	}

	tm[0] = +cos(yaw)/rx;
	tm[1] = -sin(yaw)*sin(pitch)/ry;
	tm[2] = 0;
	tm[3] = 0;
	tm[4] = +sin(yaw)/rx;
	tm[5] = +cos(yaw)*sin(pitch)/ry;
	tm[6] = 0;
	tm[7] = 0;
	tm[8] = 0;
	tm[9] = +cos(pitch)*z_scale/ry;
	tm[10] = +2./0xffff;
	tm[11] = 0;
	tm[12] = -(pos_x * tm[0] + pos_y * tm[4] + pos_z * tm[8]);
	tm[13] = -(pos_x * tm[1] + pos_y * tm[5] + pos_z * tm[9]);
	tm[14] = -1.0;
	tm[15] = 1.0;

	float br_xyra[4] = { 0,0, br_radius, 0 };
	float br_quad[3] = { 0,0,0 };
	float br_probe[3] = { (float)probe_z, 1.0f, br_limit ? br_alpha : 0.0f };

	bool create_preview = false;
	int create_preview_px = 0;
	int create_preview_py = 0;


	if (!io.WantCaptureMouse && mouse_in)
	{
		if (painting || creating)
		{
			if (creating)
			{
				double mdx = painting_x - round(io.MousePos.x);
				double mdy = -(painting_y - round(io.MousePos.y)) / sin(pitch);
				double dx = -(mdx*cos(yaw) - mdy * sin(yaw)) / font_size;
				double dy = -(mdx*sin(yaw) + mdy * cos(yaw)) / font_size;
				double x = painting_dx + dx;
				double y = painting_dy + dy;

				int px = (int)floor(x / VISUAL_CELLS);
				int py = (int)floor(y / VISUAL_CELLS);

				if (creating < 0)
				{
					// LOCATE & DELETE PATCH IF EXIST
					Patch* p = GetTerrainPatch(terrain, px, py);
					if (p)
						URDO_Delete(terrain, p);
				}
				else
				{
					// IF NO PATCH THERE, CREATE ONE
					Patch* p = GetTerrainPatch(terrain, px, py);
					if (!p)
						p = URDO_Create(terrain, px, py, probe_z);
				}

				painting_dx = x;
				painting_dy = y;
				painting_x = (int)round(io.MousePos.x);
				painting_y = (int)round(io.MousePos.y);

				if (!io.MouseDown[0])
				{
					creating = 0;
					URDO_Close();
				}
			}
			else // painting
			{
				//DRAG and/or DROP
				double mdx = painting_x - round(io.MousePos.x);
				double mdy = -(painting_y - round(io.MousePos.y)) / sin(pitch);
				double dx = -(mdx*cos(yaw) - mdy * sin(yaw)) / font_size;
				double dy = -(mdx*sin(yaw) + mdy * cos(yaw)) / font_size;
				double x = painting_dx + dx;
				double y = painting_dy + dy;

				double dist = paint_dist + sqrt(dx*dx + dy * dy);

				int i = 0;
				float alpha = br_alpha;
				br_alpha *= STAMP_A;
				while (1)
				{
					double w = ((i + 1) * br_radius * STAMP_R - paint_dist) / (dist - paint_dist);

					if (w >= 1)
						break;

					double sx = painting_dx + w * dx;
					double sy = painting_dy + w * dy;

					Stamp(sx, sy);

					i++;
				}
				br_alpha = alpha;

				paint_dist = dist - i * br_radius * STAMP_R;
				painting_dx = x;
				painting_dy = y;
				painting_x = (int)round(io.MousePos.x);
				painting_y = (int)round(io.MousePos.y);

				br_xyra[0] = (float)x;
				br_xyra[1] = (float)y;

				if (!io.MouseDown[0])
				{
					// DROP
					float alpha = br_alpha;
					br_alpha *= (float)pow(paint_dist / (br_radius * STAMP_R) * STAMP_A, 2.0);
					Stamp(x, y);
					br_alpha = alpha;
					br_xyra[3] = 0;
					painting = 0;
					URDO_Close();
				}
				else
					br_xyra[3] = (float)pow(paint_dist / (br_radius * STAMP_R) * STAMP_A, 2.0) * br_alpha;
			}
		}
		else
		{
			// HOVER preview
			// all coords in world space!
			double itm[16];
			Invert(tm, itm);

			double ray_p[4];
			double ray_v[4];

			// mouse ray
			double clip_mouse[4] =
			{
				2.0 * io.MousePos.x / io.DisplaySize.x - 1.0,
				1.0 - 2.0 * io.MousePos.y / io.DisplaySize.y,
				-1.1, // bit under floor
				1
			};

			Product(itm, clip_mouse, ray_p);

			clip_mouse[2] = -1.2; // bit under bit under floor

			Product(itm, clip_mouse, ray_v);

			ray_v[0] -= ray_p[0];
			ray_v[1] -= ray_p[1];
			ray_v[2] -= ray_p[2];

			double hit[4];

			Patch* p = HitTerrain(terrain, ray_p, ray_v, hit);

			if (p)
			{
				if (io.KeyAlt)
				{
					if (io.MouseDown[0])
					{
						URDO_Open();
						creating = -1;

						painting_x = (int)roundf(io.MousePos.x);
						painting_y = (int)roundf(io.MousePos.y);

						painting_dx = hit[0];
						painting_dy = hit[1];
					}
					else
					{
						// paint similar preview as for diag flipping but 
						// hilight entire PATCH (instead of quad) and use RED color

						// add here quad preview
						double qx = floor(hit[0] / VISUAL_CELLS) * VISUAL_CELLS;
						double qy = floor(hit[1] / VISUAL_CELLS) * VISUAL_CELLS;
						br_quad[0] = (float)qx;
						br_quad[1] = (float)qy;
						br_quad[2] = -1.0f;
					}
				}
				else
				if (io.KeyCtrl)
				{
					if (io.KeyShift)
					{
						// add here probe preview
						if (io.MouseDown[0])
						{
							// height-probe
							probe_z = (int)round(hit[2]);
							br_probe[0] = (float)probe_z;
							br_probe[1] = 0.5f;
						}
						else
						{
							// preview
							br_probe[0] = (float)round(hit[2]);
							br_probe[1] = 0.5f;
						}
					}
					else
					{
						// add here quad preview
						double qx = floor(hit[0] * HEIGHT_CELLS / VISUAL_CELLS) * VISUAL_CELLS / HEIGHT_CELLS;
						double qy = floor(hit[1] * HEIGHT_CELLS / VISUAL_CELLS) * VISUAL_CELLS / HEIGHT_CELLS;
						br_quad[0] = (float)qx;
						br_quad[1] = (float)qy;
						br_quad[2] = 1.0f;

						if (!diag_flipped && io.MouseDown[0])
						{
							struct mod_floor
							{
								mod_floor(int d) : y(d) {}
								int mod(int x)
								{
									int r = x % y;
									if (/*(r != 0) && ((r < 0) != (y < 0))*/ r && (r^y)<0) 
										r += y;
									return r;
								}
								int y;
							} mf(HEIGHT_CELLS);

							// floor xy hit coords to height cells
							//int hx = (int)floor(hit[0] * HEIGHT_CELLS / VISUAL_CELLS) % HEIGHT_CELLS;
							//int hy = (int)floor(hit[1] * HEIGHT_CELLS / VISUAL_CELLS) % HEIGHT_CELLS;

							int hx = mf.mod((int)floor(hit[0] * HEIGHT_CELLS / VISUAL_CELLS));
							int hy = mf.mod((int)floor(hit[1] * HEIGHT_CELLS / VISUAL_CELLS));

							{
								uint16_t diag = GetTerrainDiag(p);
								diag ^= 1 << (hx + hy * HEIGHT_CELLS);

								URDO_Diag(p);
								SetTerrainDiag(p, diag);
							}

							// one per click
							diag_flipped = true;
						}
					}
				}
				else
				{
					br_xyra[0] = (float)hit[0];
					br_xyra[1] = (float)hit[1];
					br_xyra[3] = br_alpha;

					if (io.MouseDown[0])
					{
						//BEGIN
						URDO_Open();
						painting = 1;

						painting_x = (int)roundf(io.MousePos.x);
						painting_y = (int)roundf(io.MousePos.y);

						painting_dx = hit[0];
						painting_dy = hit[1];
						paint_dist = 0.0;

						float alpha = br_alpha;
						br_alpha *= STAMP_A;
						Stamp(hit[0], hit[1]);
						br_alpha = alpha;

						// stamped, don't apply preview to it
					}
				}
			}
			else
			{
				if (io.KeyAlt)
				{
					double t = (probe_z - ray_p[2]) / ray_v[2];
					double vx = ray_p[0] + t * ray_v[0];
					double vy = ray_p[1] + t * ray_v[1];

					// probably create 
					if (io.MouseDown[0])
					{
						URDO_Open();
						creating = +1;

						painting_x = (int)roundf(io.MousePos.x);
						painting_y = (int)roundf(io.MousePos.y);

						painting_dx = vx;
						painting_dy = vy;
					}
					else
					{
						create_preview = true;
						create_preview_px = (int)floor(vx / VISUAL_CELLS);
						create_preview_py = (int)floor(vy / VISUAL_CELLS);

						// paint imaginary patch?
						// that requires extra draw command!
					}
				}
			}
		}
	}

	if (panning || spinning)
	{
		br_xyra[3] = 0;
	}

	if (io.KeysDown[A3D_LSHIFT])
	{
		br_xyra[2] = -br_xyra[2];
	}

	// 4 clip planes in clip-space

	double clip_left[4] =   { 1, 0, 0,+1 };
	double clip_right[4] =  {-1, 0, 0,+1 };
	double clip_bottom[4] = { 0, 1, 0,+1 }; 
	double clip_top[4] =    { 0,-1, 0,+1 }; // adjust by max brush descent

	double brush_extent = cos(pitch) * br_xyra[3] * br_xyra[2] / ry;

	if (br_xyra[2] > 0)
	{
		// adjust by max brush ASCENT
		if (br_xyra[3] > 0)
			clip_bottom[3] += brush_extent;

		// adjust by max brush DESCENT
		if (br_xyra[3] < 0)
			clip_top[3] -= brush_extent;
	}

	// transform them to world-space (mul by tm^-1)

	double clip_world[4][4];
	TransposeProduct(tm, clip_left, clip_world[0]);
	TransposeProduct(tm, clip_right, clip_world[1]);
	TransposeProduct(tm, clip_bottom, clip_world[2]);
	TransposeProduct(tm, clip_top, clip_world[3]);

	int planes = 4;
	int view_flags = 0xAA; // should contain only bits that face viewing direction

	double noon_yaw[2] =
	{
		// zero is behind viewer
		-sin(-lit_yaw*M_PI / 180),
		-cos(-lit_yaw*M_PI / 180),
	};

	double dusk_yaw[3] =
	{
		-noon_yaw[1],
		noon_yaw[0],
		0
	};

	double noon_pos[3] =
	{
		noon_yaw[0]*cos(lit_pitch*M_PI / 180),
		noon_yaw[1]*cos(lit_pitch*M_PI / 180),
		sin(lit_pitch*M_PI / 180)
	};

	double lit_axis[3];

	CrossProduct(dusk_yaw, noon_pos, lit_axis);

	double time_tm[16];
	Rotation(lit_axis, (lit_time-12)*M_PI / 12, time_tm);

	double lit_pos[4];
	Product(time_tm, noon_pos, lit_pos);

	float lt[3] =
	{
		(float)lit_pos[0],
		(float)lit_pos[1],
		(float)lit_pos[2]
	};

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);
	rc->BeginPatches(tm, lt, br_xyra, br_quad, br_probe);
	QueryTerrain(terrain, planes, clip_world, view_flags, RenderContext::RenderPatch, rc);
	//printf("rendered %d patches / %d total\n", rc.patches, GetTerrainPatches(terrain));
	rc->EndPatches();

	// overlay patch creation
	// slihouette of newly created patch 

	if (create_preview)
	{
		uint16_t ghost[4 * HEIGHT_CELLS];
		bool exist = CalcTerrainGhost(terrain, create_preview_px, create_preview_py, probe_z, ghost);
		if (!exist)
			rc->PaintGhost(tm, create_preview_px, create_preview_py, probe_z, ghost);
	}

	glDisable(GL_DEPTH_TEST);

	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	a3dSwapBuffers();
}

void my_mouse(int x, int y, MouseInfo mi)
{
	if ((mi & 0xF) == MouseInfo::LEAVE)
	{
		mouse_in = 0;
		return;
	}
	else
	if ((mi & 0xF) == MouseInfo::ENTER)
		mouse_in = 1;

	ImGuiIO& io = ImGui::GetIO();

	io.MousePos = ImVec2((float)x, (float)y);

	switch (mi & 0xF)
	{
		case MouseInfo::WHEEL_DN:
			zoom_wheel--;
			io.MouseWheel -= 1.0;
			break;
		case MouseInfo::WHEEL_UP:
			zoom_wheel++;
			io.MouseWheel += 1.0;
			break;
		case MouseInfo::LEFT_DN:
			io.MouseDown[0] = true;
			break;
		case MouseInfo::LEFT_UP:
			io.MouseDown[0] = false;
			break;
		case MouseInfo::RIGHT_DN:
			io.MouseDown[1] = true;
			break;
		case MouseInfo::RIGHT_UP:
			io.MouseDown[1] = false;
			break;
		case MouseInfo::MIDDLE_DN:
			io.MouseDown[2] = true;
			break;
		case MouseInfo::MIDDLE_UP:
			io.MouseDown[2] = false;
			break;
	}
}

void my_resize(int w, int h)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)w, (float)h);
}


void PngTest(void *cookie, A3D_ImageFormat f, int w, int h, const void *data, int palsize, const void *palbuf)
{
	uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t)*w*h);
	Convert_RGBA8(buf,f,w,h,data,palsize,palbuf);
	a3dSetIconData(A3D_RGBA8, w,h, buf, 0,0);
	free(buf);
}

void my_init()
{
//	a3dLoadImage("./icons/basn0g01.png",0,PngTest); // fixed (flip bits?)
//	a3dLoadImage("./icons/basn0g02.png",0,PngTest); // ok
//	a3dLoadImage("./icons/basn0g04.png",0,PngTest); // ok
//	a3dLoadImage("./icons/basn0g08.png",0,PngTest); // ok
//	a3dLoadImage("./icons/basn0g16.png",0,PngTest); // unsupported grey-16

//	a3dLoadImage("./icons/basn2c08.png",0,PngTest); // ok

//	a3dLoadImage("./icons/basn2c16.png",0,PngTest); // fixed (swap bytes?)
//	a3dLoadImage("./icons/basn3p01.png",0,PngTest); // bad (flip bits?)
//	a3dLoadImage("./icons/basn3p02.png",0,PngTest); // bad (flip bits?)
//	a3dLoadImage("./icons/basn3p04.png",0,PngTest); // bad (flip bits?)

//	a3dLoadImage("./icons/basn3p08.png",0,PngTest); // ok
//	a3dLoadImage("./icons/basn4a08.png",0,PngTest); // ok

//	a3dLoadImage("./icons/basn4a16.png",0,PngTest); // unsupported lum-alpha-16

//	a3dLoadImage("./icons/basn6a08.png",0,PngTest); // ok

	a3dLoadImage("./icons/basn6a16.png",0,PngTest); // bad (swap bytes?)




	printf("RENDERER: %s\n",glGetString(GL_RENDERER));
	printf("VENDOR:   %s\n",glGetString(GL_VENDOR));
	printf("VERSION:  %s\n",glGetString(GL_VERSION));
	printf("SHADERS:  %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));

	MyMaterial::Init();

	char font_dirname[] = "./fonts";
	fonts_loaded = 0;
	a3dListDir(font_dirname, MyFont::Scan, font_dirname);

	MyPalette::Init();
	char pal_dirname[] = "./palettes";
	palettes_loaded = 0;
	a3dListDir(pal_dirname, MyPalette::Scan, pal_dirname);

	g_Time = a3dGetTime();
	render_context.Create();

	glDebugMessageCallback(glDebugCall, 0/*cookie*/);

	// Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.IniFilename = 0;
	io.BackendPlatformName = "imgui_impl_a3d";

	io.KeyMap[ImGuiKey_Tab] = A3D_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = A3D_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = A3D_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = A3D_UP;
	io.KeyMap[ImGuiKey_DownArrow] = A3D_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = A3D_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = A3D_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = A3D_HOME;
	io.KeyMap[ImGuiKey_End] = A3D_END;
	io.KeyMap[ImGuiKey_Insert] = A3D_INSERT;
	io.KeyMap[ImGuiKey_Delete] = A3D_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = A3D_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = A3D_SPACE;
	io.KeyMap[ImGuiKey_Enter] = A3D_ENTER;
	io.KeyMap[ImGuiKey_Escape] = A3D_ESCAPE;
	io.KeyMap[ImGuiKey_A] = A3D_A;
	io.KeyMap[ImGuiKey_C] = A3D_C;
	io.KeyMap[ImGuiKey_V] = A3D_V;
	io.KeyMap[ImGuiKey_X] = A3D_X;
	io.KeyMap[ImGuiKey_Y] = A3D_Y;
	io.KeyMap[ImGuiKey_Z] = A3D_Z;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ImGui_ImplOpenGL3_Init();

	terrain = CreateTerrain();

	// ALTERNATIVE:
	// terrain = CreateTerrain(int x, int y, int w, int h, uint16_t* data);
	// xywh coords are in patches, so data is w*4+1,h*4+1 !!!!!!!!!!!!!!!!

	const int num1 = 8;
	const int num2 = num1*num1;

	uint32_t* rnd = (uint32_t*)malloc(sizeof(uint32_t)*num2);
	int n = num2;
	for (int i = 0; i < num2; i++)
		rnd[i] = i;

	for (int i = 0; i < num2; i++)
	{
		int r = (fast_rand() + fast_rand()*(FAST_RAND_MAX+1)) % n;

		uint32_t uv = rnd[r];
		rnd[r] = rnd[--n];
		uint32_t u = uv % num1;
		uint32_t v = uv / num1;
		AddTerrainPatch(terrain, u, v, 0/*fast_rand()&0x7F*/);
	}

	free(rnd);

	pos_x = num1 * VISUAL_CELLS / 2;
	pos_y = num1 * VISUAL_CELLS / 2;
	pos_z = 0x0;

	a3dSetTitle(L"ASCIIID");

	int full[] = { -1280,0,800,600};
	//int full[] = { 0,0,1920,1080};
	a3dSetRect(full, true);

	a3dSetVisible(true);

	//a3dSetIcon("./icons/app.png");
}

void my_keyb_char(wchar_t chr)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter((unsigned short)chr);
}

void my_keyb_key(KeyInfo ki, bool down)
{
	ImGuiIO& io = ImGui::GetIO();
	if (ki < IM_ARRAYSIZE(io.KeysDown))
		io.KeysDown[ki] = down;
	
	io.KeysDown[A3D_ENTER] = a3dGetKeyb(A3D_ENTER) || a3dGetKeyb(A3D_NUMPAD_ENTER);
	io.KeyAlt = a3dGetKeyb(A3D_LALT) || a3dGetKeyb(A3D_RALT);
	io.KeyCtrl = a3dGetKeyb(A3D_LCTRL) || a3dGetKeyb(A3D_RCTRL);
	io.KeyShift = a3dGetKeyb(A3D_LSHIFT) || a3dGetKeyb(A3D_RSHIFT);
}

void my_keyb_focus(bool set)
{
}

void my_close()
{
	URDO_Purge();
	DeleteTerrain(terrain);
	MyFont::Free();
	MyMaterial::Free();

	if (gather)
	{
		if (gather->tmp_x)
			free(gather->tmp_x);
		if (gather->tmp_y)
			free(gather->tmp_y);
		free(gather);
	}

	a3dClose();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	render_context.Delete();
}

int main(int argc, char *argv[]) 
{
	PlatformInterface pi;
	pi.close = my_close;
	pi.render = my_render;
	pi.resize = my_resize;
	pi.init = my_init;
	pi.keyb_char = my_keyb_char;
	pi.keyb_key = my_keyb_key;
	pi.keyb_focus = my_keyb_focus;
	pi.mouse = my_mouse;

	GraphicsDesc gd;
	gd.color_bits = 32;
	gd.alpha_bits = 8;
	gd.depth_bits = 24;
	gd.stencil_bits = 8;
	gd.flags = (GraphicsDesc::FLAGS) (GraphicsDesc::DEBUG_CONTEXT | GraphicsDesc::DOUBLE_BUFFER);

	a3dOpen(&pi, &gd);

	return 0;
}
