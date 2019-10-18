
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "asciiid_term.h"
#include "asciiid_platform.h"
#include "asciiid_render.h"
#include "fast_rand.h"
#include "gl.h"

#define CODE(...) #__VA_ARGS__
#define DEFN(a,s) "#define " #a #s "\n"

struct TERM_LIST
{
	TERM_LIST* prev;
	TERM_LIST* next;
	A3D_WND* wnd;

	float yaw;
	float player_dir;
	int player_stp;

	float pos[3];
	float water;

	float vel[3];

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
	// YAW
	{
		int da = 0;
		if (term->IsKeyDown(A3D_E))
			da--;
		if (term->IsKeyDown(A3D_Q))
			da++;

		term->yaw += 4*da;
	}

	// XY: innertia / friction / vel threshold
	{
		int dx = 0, dy = 0;
		if (term->IsKeyDown(A3D_A))
			dx--;
		if (term->IsKeyDown(A3D_D))
			dx++;
		if (term->IsKeyDown(A3D_W))
			dy++;
		if (term->IsKeyDown(A3D_S))
			dy--;


		float dir[3][3] =
		{
			{315,  0 , 45},
			{270, -1 , 90},
			{225, 180, 135},
		};

		if (dir[dy + 1][dx + 1] >= 0)
			term->player_dir = dir[dy + 1][dx + 1] + term->yaw;

		term->vel[0] += dx * cos(term->yaw * (M_PI / 180)) - dy * sin(term->yaw * (M_PI / 180));
		term->vel[1] += dx * sin(term->yaw * (M_PI / 180)) + dy * cos(term->yaw * (M_PI / 180));

		float sqr_vel_xy = term->vel[0] * term->vel[0] + term->vel[1] * term->vel[1];
		if (sqr_vel_xy < 1 && !dx && !dy)
		{
			term->vel[0] = 0;
			term->vel[1] = 0;
			term->player_stp = -1;
		}
		else
		{
			if (sqr_vel_xy > 27)
			{
				float n = sqrt(27 / sqr_vel_xy);
				sqr_vel_xy = 27;
				term->vel[0] *= n;
				term->vel[1] *= n;
			}

			term->pos[0] += 0.1 * term->vel[0];
			term->pos[1] += 0.1 * term->vel[1];

			if (term->player_stp < 0)
				term->player_stp = 0;

			term->player_stp = (~(1<<31))&(term->player_stp + (int)(1 * sqrt(sqr_vel_xy)));

			term->vel[0] *= 0.9;
			term->vel[1] *= 0.9;
		}
	}

	// after updating x,y,z by time and keyb bits
	// we need to fix z so player doesn't penetrate terrain
	{
		double p[3] = { term->pos[0],term->pos[1],-1 };
		double v[3] = { 0,0,-1 };
		double r[4] = { 0,0,0,1 };
		Patch* patch = HitTerrain(terrain, p, v, r);

		if (patch)
			term->pos[2] = r[2];
		else
			term->pos[2] = 0;
	}
}

void term_render(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

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
}

void term_resize(A3D_WND* wnd, int w, int h)
{
}

void term_init(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)malloc(sizeof(TERM_LIST));
	term->wnd = wnd;

	term->yaw = rot_yaw;
	term->water = probe_z;
	term->pos[0] = pos_x;
	term->pos[1] = pos_y;
	term->pos[2] = pos_z;
	term->vel[0] = 0;
	term->vel[1] = 0;
	term->vel[2] = 0;

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

	term->yaw = yaw;
	term->pos[0] = pos[0];
	term->pos[1] = pos[1];
	term->pos[2] = pos[2];
	term->water = 0;
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

