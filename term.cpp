
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "term.h"
#include "platform.h"
#include "fast_rand.h"
#include "matrix.h"
#include "gl.h"
#include "gl45_emu.h"

#include "game.h"

#define CODE(...) #__VA_ARGS__
#define DEFN(a,s) "#define " #a #s "\n"

void ToggleFullscreen(Game* g);
bool IsFullscreen(Game* g);

struct TERM_LIST
{
	TERM_LIST* prev;
	TERM_LIST* next;
	A3D_WND* wnd;

	void(*close)();

	Game* game;
	//Physics* phys;

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

	GLint uni_ansi_vp;
	GLint uni_ansi;
	GLint uni_font;
	GLint uni_ansi_wh;
	GLint att_uv;
	GLint out_color;
};

// HACK: get it from editor
extern Terrain* terrain;
extern World* world;
int GetGLFont(int wh[2], const int wnd_wh[2]);
bool NextGLFont();
bool PrevGLFont();

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

	// dispatch all queued messages to game
	// 1. flush list
	// 2. reverse order
	// 3. dispatch every message with term->game->OnMessage()

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

	if (server)
		server->Proc();

	uint64_t stamp = a3dGetTime();
	term->game->Render(stamp, term->buf, width, height);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	char utf8[64];
	sprintf(utf8, "ASCIIID Term %d x %d", width, height);
	a3dSetTitle(wnd, utf8);

	int vp_wh[2] =
	{
		width * (fnt_wh[0] >> 4),
		height * (fnt_wh[1] >> 4)
	};

	int vp_xy[2] =
	{
		(wnd_wh[0] - vp_wh[0]) / 2,
		(wnd_wh[1] - vp_wh[1]) / 2
	};

	// copy term->buf to some texture
	gl3TextureSubImage2D(term->tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, term->buf);

	glViewport(vp_xy[0], vp_xy[1], vp_wh[0], vp_wh[1]);

	glUseProgram(term->prg);

	glUniform2i(/*0*/term->uni_ansi_vp, width, height);

	glUniform1i(/*1*/term->uni_ansi, 0);
	//glBindTextureUnit(0, term->tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,term->tex);	

	glUniform1i(/*2*/term->uni_font, 1);
	//glBindTextureUnit(1, fnt_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D,fnt_tex);	

	glUniform2i(/*3*/term->uni_ansi_wh, term->max_width, term->max_height);
	
	glBindVertexArray(term->vao);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUseProgram(0);
	glBindVertexArray(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D,0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,0);
	/*
	glBindTextureUnit(0, 0);
	glBindTextureUnit(1, 0);
	*/
}

void term_mouse(A3D_WND* wnd, int x, int y, MouseInfo mi)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	switch (mi & 0xF)
	{
		case MouseInfo::MIDDLE_DN:	
		{
			int p[2] = { x,y };
			term->game->ScreenToCell(p);
			render_break_point[0] = p[0];
			render_break_point[1] = p[1];
			break;
		}

		case MouseInfo::MOVE:		term->game->OnMouse(Game::GAME_MOUSE::MOUSE_MOVE, x, y); break;
		case MouseInfo::LEFT_DN:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_LEFT_BUT_DOWN, x, y); break;
		case MouseInfo::LEFT_UP:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_LEFT_BUT_UP, x, y); break;
		case MouseInfo::RIGHT_DN:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN, x, y); break;
		case MouseInfo::RIGHT_UP:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_RIGHT_BUT_UP, x, y); break;
		case MouseInfo::WHEEL_UP:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_WHEEL_UP, x, y); break;
		case MouseInfo::WHEEL_DN:	term->game->OnMouse(Game::GAME_MOUSE::MOUSE_WHEEL_DOWN, x, y); break;
	}
}

void term_resize(A3D_WND* wnd, int w, int h)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	int fnt_wh[2] = { 0,0 };
	int wnd_wh[2] = { w,h };
	int fnt_tex = GetGLFont(fnt_wh, wnd_wh);

	term->game->OnSize(w, h, fnt_wh[0] >> 4, fnt_wh[1] >> 4);
}

void term_init(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)malloc(sizeof(TERM_LIST));
	term->wnd = wnd;

	uint64_t stamp = a3dGetTime();

	float pos[3] = { pos_x, pos_y, pos_z };
	float yaw = rot_yaw;
	float dir = 0;
	int water = probe_z;
	term->game = CreateGame(water, pos, yaw, dir, stamp);

	int loglen = 999;
	char logstr[1000] = "";
	GLuint shader[2] = { 0,0 };

	term->tex = 0;
	gl3CreateTextures(GL_TEXTURE_2D, 1, &term->tex);

	if (!term->tex)
	{
		printf("glCreateTextures failed\n");
		exit(-1);
	}
	
	gl3TextureStorage2D(term->tex, 1, GL_RGBA8, term->max_width, term->max_height);

	const char* term_vs_src =
		CODE(#version 330\n)
		CODE(
			/*layout(location = 0)*/ uniform ivec2 ansi_vp;  // viewport size in cells
			/*layout(location = 0)*/ in vec2 uv; // normalized to viewport size
			out vec2 cell_coord;
			void main()
			{
				gl_Position = vec4(2.0*uv - vec2(1.0), 0.0, 1.0);
				cell_coord = uv * ansi_vp;
			}
		);

	const char* term_fs_src =
		CODE(#version 330\n)
		DEFN(P(r, g, b), vec3(r / 6., g / 7., b / 6.))
		CODE(

			/*layout(location = 0)*/ out vec4 color;
			/*layout(location = 1)*/ uniform sampler2D ansi;
			/*layout(location = 2)*/ uniform sampler2D font;
			/*layout(location = 3)*/ uniform ivec2 ansi_wh;  // ansi texture size (in cells), constant = 160x90
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

	term->uni_ansi_vp = glGetUniformLocation(term_prg,"ansi_vp");
	term->uni_ansi = glGetUniformLocation(term_prg,"ansi");
	term->uni_font = glGetUniformLocation(term_prg,"font");
	term->uni_ansi_wh = glGetUniformLocation(term_prg,"ansi_wh");
	term->att_uv = glGetAttribLocation(term_prg,"uv");
	term->out_color = glGetFragDataLocation(term_prg,"color");	

	float vbo_data[] = {0,0, 1,0, 1,1, 0,1};

	GLuint term_vbo = 0;
	//glCreateBuffers(1, &term_vbo);
	glGenBuffers(1, &term_vbo);

	if (!term_vbo)
	{
		printf("glCreateBuffers failed\n");
		exit(-1);
	}

	//glNamedBufferStorage(term_vbo, 4 * sizeof(float[2]), 0, GL_DYNAMIC_STORAGE_BIT);
	//glNamedBufferSubData(term_vbo, 0, 4 * sizeof(float[2]), vbo_data);
	glBindBuffer(GL_ARRAY_BUFFER, term_vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float[2]), vbo_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	term->vbo = term_vbo;

	GLuint term_vao = 0;
	//glCreateVertexArrays(1, &term_vao);
	glGenVertexArrays(1, &term_vao);

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
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);
	term->game->OnKeyb(Game::GAME_KEYB::KEYB_CHAR, (int)chr);
}

void term_keyb_key(A3D_WND* wnd, KeyInfo ki, bool down)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	// if (ki&A3D_AUTO_REPEAT)
	//	return;

	if (down)
	{
		if (ki == A3D_NUMPAD_ADD)
		{
			NextGLFont();
		}
		else
		if (ki == A3D_NUMPAD_SUBTRACT)
		{
			PrevGLFont();
		}
		else
		if (ki >= A3D_F5 && ki <= A3D_F8)
		{
			// send press
			if (!(ki & A3D_AUTO_REPEAT))
				term->game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, ki);
		}
		else
		if (ki == A3D_F11)
		{
			ToggleFullscreen(term->game);
		}
		else
			term->game->OnKeyb(Game::GAME_KEYB::KEYB_DOWN, ki);
	}
	else
		if (ki != A3D_F11)
			term->game->OnKeyb(Game::GAME_KEYB::KEYB_UP, ki);
}

void term_keyb_focus(A3D_WND* wnd, bool set)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);
	term->game->OnFocus(set);
}

void term_close(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	if (term->game)
		DeleteGame(term->game);

	glDeleteTextures(1, &term->tex);
	glDeleteVertexArrays(1, &term->vao);
	glDeleteBuffers(1, &term->vbo);
	glDeleteProgram(term->prg);

	if (term->close)
		term->close();

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

bool TermOpen(A3D_WND* share, float yaw, float pos[3], void(*close)())
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
	gd.alpha_bits = 0;
	gd.depth_bits = 0;
	gd.stencil_bits = 0;

#ifdef USE_GL3
	gd.version[0] = 3;
	gd.version[1] = 3;
#else
	gd.version[0] = 4;
	gd.version[1] = 5;
#endif

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
	term->close = close;
	

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
	A3D_PUSH_CONTEXT push;
	a3dPushContext(&push);

	TERM_LIST* term = term_head;
	while (term)
	{
		TERM_LIST* next = term->next;

		a3dSwitchContext(term->wnd);
		term_close(term->wnd);
		term = next;
	}

	a3dPopContext(&push);

	term_head = 0;
	term_tail = 0;
}

void TermResizeAll()
{
	int wh[2];
	TERM_LIST* term = term_head;
	while (term)
	{
		TERM_LIST* next = term->next;
		a3dGetRect(term->wnd, 0, wh);
		term_resize(term->wnd, wh[0],wh[1]);
		term = next;
	}
}

void ToggleFullscreen(Game* g)
{
	TERM_LIST* term = term_head;
	while (term)
	{
		if (term->game == g)
			break;
		term = term->next;
	}

	if (!term)
		return;

	if (a3dGetRect(term->wnd,0,0) != A3D_WND_FULLSCREEN)
		a3dSetRect(term->wnd, 0, A3D_WND_FULLSCREEN);
	else
		a3dSetRect(term->wnd, 0, A3D_WND_NORMAL);

}

bool IsFullscreen(Game* g)
{
	TERM_LIST* term = term_head;
	while (term)
	{
		if (term->game == g)
			break;
		term = term->next;
	}

	if (!term)
		return false;

	if (a3dGetRect(term->wnd,0,0) == A3D_WND_FULLSCREEN)
		return true;
	return false;
}
