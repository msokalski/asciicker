
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "term.h"
#include "platform.h"
#include "render.h"
#include "fast_rand.h"
#include "matrix.h"
#include "gl.h"

#include "physics.h"

#define CODE(...) #__VA_ARGS__
#define DEFN(a,s) "#define " #a #s "\n"


struct TERM_LIST
{
	TERM_LIST* prev;
	TERM_LIST* next;
	A3D_WND* wnd;

	Physics* phys;

	int mouse_b;
	int mouse_x;
	int mouse_y;
	bool mouse_j;
	bool mouse_rot;
	float mouse_rot_x;
	float mouse_rot_yaw;

	float yaw;

	uint8_t keys[32];
	bool IsKeyDown(int key)
	{
		return (keys[key >> 3] & (1 << (key & 0x7))) != 0;
	}

	static const int max_width = 160; // 160;
	static const int max_height = 90; // 90;
	AnsiCell buf[max_width*max_height];
	GLuint tex;
	GLuint prg;
	GLuint vbo;
	GLuint vao;
};

// HACK: get it from editor
extern Terrain* terrain;
extern World* world;
int GetGLFont(int wh[2], const int wnd_wh[2]);

TERM_LIST* term_head = 0;
TERM_LIST* term_tail = 0;

// SUPER_HACK LIVE VIEW
extern float pos_x, pos_y, pos_z;
extern float rot_yaw;
extern float global_lt[];
extern int probe_z;

void term_render(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	PhysicsIO io;

	float speed = 1;
	if (term->IsKeyDown(A3D_LSHIFT) || term->IsKeyDown(A3D_RSHIFT))
		speed *= 0.5;
	io.x_force = (int)(term->IsKeyDown(A3D_RIGHT) || term->IsKeyDown(A3D_D)) - (int)(term->IsKeyDown(A3D_LEFT) || term->IsKeyDown(A3D_A));
	io.y_force = (int)(term->IsKeyDown(A3D_UP) || term->IsKeyDown(A3D_W)) - (int)(term->IsKeyDown(A3D_DOWN) || term->IsKeyDown(A3D_S));

	float len = sqrtf(io.x_force*io.x_force + io.y_force*io.y_force);
	if (len > 0)
		speed /= len;
	io.x_force *= speed;
	io.y_force *= speed;

	io.torque = (int)(term->IsKeyDown(A3D_DELETE) || term->IsKeyDown(A3D_PAGEUP) || term->IsKeyDown(A3D_F1) || term->IsKeyDown(A3D_Q)) -
		(int)(term->IsKeyDown(A3D_INSERT) || term->IsKeyDown(A3D_PAGEDOWN) || term->IsKeyDown(A3D_F2) || term->IsKeyDown(A3D_E));
	io.water = probe_z;
	io.jump = term->IsKeyDown(A3D_LALT) || term->IsKeyDown(A3D_RALT) || term->IsKeyDown(A3D_SPACE) || term->mouse_j;
	//io.slow = term->IsKeyDown(A3D_LSHIFT) || term->IsKeyDown(A3D_RSHIFT);


	int wnd_wh[2];

	a3dGetRect(wnd, 0, wnd_wh);

	int fnt_wh[2];
	int fnt_tex = GetGLFont(fnt_wh, wnd_wh);

	int width = wnd_wh[0] / (fnt_wh[0] >> 4);
	int height = wnd_wh[1] / (fnt_wh[1] >> 4);

	if (width > term->max_width)
		width = term->max_width;
	if (height > term->max_height)
		height = term->max_height;

	uint64_t stamp = a3dGetTime();

	if (term->mouse_rot)
	{
		/*
		float ox = (wnd_wh[0] - width * (fnt_wh[0] >> 4)) *0.5f;
		float mx = (term->mouse_x - ox) / (fnt_wh[0] >> 4);
		io.torque = -2 * (mx * 2 - width) / (float)width;
		*/

		float sensitivity = 200.0f / wnd_wh[0];
		float yaw = term->mouse_rot_yaw - sensitivity * (term->mouse_x - term->mouse_rot_x);
		io.torque = 1000000;
		io.yaw = yaw;
	}
	else
	if (term->mouse_b)
	{
		float ox = (wnd_wh[0] - width*(fnt_wh[0]>>4)) *0.5f;
		float oy = (wnd_wh[1] - height*(fnt_wh[1]>>4)) *0.5f;

		float mx = (term->mouse_x - ox) / (fnt_wh[0]>>4);
		float my = (term->mouse_y - oy) / (fnt_wh[1]>>4);

		float speed = 1.0;
		io.x_force = speed*2*(mx*2 - width) / (float)width;
		io.y_force = speed*2*(height - my*2) / (float)height;
	}

	Animate(term->phys, stamp, &io);

	term->yaw = io.yaw;

	if (!io.jump)
	{
		term->keys[A3D_LALT/8] &= ~(1<<(A3D_LALT&7));
		term->keys[A3D_RALT/8] &= ~(1<<(A3D_RALT&7));
		term->keys[A3D_SPACE/8] &= ~(1<<(A3D_SPACE&7));
		term->mouse_j = false;
	}


	// FPS DUMPER
	{
		static int frames = 0;
		frames++;
		static uint64_t p = a3dGetTime();
		uint64_t t = stamp;
		uint64_t d = t - p;
		if (d > 1000000)
		{
			double fps = 1000000.0 * frames / (double)d;
			printf("fps = %.2f, x = %.2f, y = %.2f, z = %.2f\n", fps, io.pos[0],io.pos[1],io.pos[2]);
			p = t;
			frames = 0;
		}
	}

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

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

	Render(stamp,terrain, world, io.water, zoom, io.yaw, io.pos, lt, width, height, term->buf, io.player_dir, io.player_stp, io.dt, io.xyz);

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
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	if ((mi & 0xF) == RIGHT_DN && term->mouse_b == 0)
	{
		term->mouse_rot = true;
		term->mouse_rot_x = x;
		term->mouse_rot_yaw = term->yaw;
	}
	else
	if ((mi & 0xF) == RIGHT_UP)
		term->mouse_rot = false;

	if ((mi & 0xF) == LEFT_DN || (mi & 0xF) == RIGHT_DN)
	{
		term->mouse_b++;
		if (term->mouse_b==2)
			term->mouse_j = true;
	}

	if ((mi & 0xF) == LEFT_UP || (mi & 0xF) == RIGHT_UP)
	{
		if (term->mouse_b)
			term->mouse_b--;
	}

	term->mouse_x = x;
	term->mouse_y = y;
}

void term_resize(A3D_WND* wnd, int w, int h)
{
}

void term_init(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)malloc(sizeof(TERM_LIST));
	term->wnd = wnd;

	term->mouse_rot = false;
	term->mouse_j = false;
	term->mouse_b = 0;
	term->mouse_x = 0;
	term->mouse_y = 0;


	uint64_t stamp = a3dGetTime();
	float yaw = rot_yaw;
	float pos[3]={pos_x,pos_y,pos_z};
	float dir = 0;

	term->phys = CreatePhysics(terrain,world,pos,dir,yaw,stamp);

	int loglen = 999;
	char logstr[1000] = "";
	GLuint shader[2] = { 0,0 };

	term->tex = 0;
	glCreateTextures(GL_TEXTURE_2D, 1, &term->tex);
	if (!term->tex)
	{
		printf("glCreateTextures failed\n");
		exit(-1);
	}
	
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

			/*
			vec3 XTermPal(int p)
			{
				p -= 16;
				if (p < 0 || p >= 216)
					return vec3(0, 0, 0);

				int r = p % 6;
				p = (p - r) / 6;
				int g = p % 6;
				p = (p - g) / 6;
				
				return vec3(p, g, r) * 0.2;
			}
			*/

			vec3 Pal(float p)
			{
				p = clamp(floor(p - 16.0 + 0.5), 0.0, 215.0);

				float blue = floor(p / 36.0);
				p -= 36.0*blue;

				float green = floor(p / 6.0);
				float red = p - 6.0*green;

				return vec3(blue, green, red) * 0.2;
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

				/*
				vec3 fg_color = XTermPal(int(round(cell.r * 255.0)));
				vec3 bg_color = XTermPal(int(round(cell.g * 255.0)));
				*/

				vec3 fg_color = Pal(cell.x*255.00);
				vec3 bg_color = Pal(cell.y*255.00);

				color = vec4(mix(bg_color, fg_color, glyph_alpha), 1.0);
			}
		);

	GLenum term_st[3] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	const char* term_src[3] = { term_vs_src, term_fs_src };
	GLuint term_prg = glCreateProgram();
	if (!term_prg)
	{
		printf("glCreateProgram failed\n");
		exit(-1);
	}

	for (int i = 0; i < 2; i++)
	{
		shader[i] = glCreateShader(term_st[i]);
		if (!shader[i])
		{
			printf("glCreateShader failed\n");
			exit(-1);
		}

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

	GLuint term_vbo = 0;
	glCreateBuffers(1, &term_vbo);
	if (!term_vbo)
	{
		printf("glCreateBuffers failed\n");
		exit(-1);
	}

	glNamedBufferStorage(term_vbo, 4 * sizeof(float[2]), 0, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(term_vbo, 0, 4 * sizeof(float[2]), vbo_data);

	term->vbo = term_vbo;

	GLuint term_vao = 0;
	glCreateVertexArrays(1, &term_vao);
	if (!term_vao)
	{
		printf("glCreateVertexArrays failed\n");
		exit(-1);
	}

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
	{
		if (ki == A3D_F11)
		{
			if (a3dGetRect(wnd,0,0) != A3D_WND_FULLSCREEN)
				a3dSetRect(wnd, 0, A3D_WND_FULLSCREEN);
			else
				a3dSetRect(wnd, 0, A3D_WND_NORMAL);
		}
		term->keys[ki >> 3] |= 1 << (ki & 0x7);
	}
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

	if (term->phys)
		DeletePhysics(term->phys);

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

bool TermOpen(A3D_WND* share, float yaw, float pos[3])
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

	if (!wnd)
		return false;

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

	return true;
}

void TermCloseAll()
{
	TERM_LIST* term = term_head;
	while (term)
	{
		TERM_LIST* next = term->next;
		term_close(term->wnd);
		term = next;
	}

	term_head = 0;
	term_tail = 0;
}

