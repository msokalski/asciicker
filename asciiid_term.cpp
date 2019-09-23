
#include <malloc.h>
#include <stdio.h>
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
	float pos[3];
	int water;

	static const int max_width = 160;
	static const int max_height = 90;
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

void term_render(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	return;

	int wnd_wh[2];

	a3dGetRect(wnd, 0, wnd_wh);

	int fnt_wh[2];
	int fnt_tex = GetGLFont(fnt_wh);

	int width = wnd_wh[0] / (fnt_wh[0] >> 4);
	int height = wnd_wh[1] / (fnt_wh[1] >> 4);

	// todo:
	// limit to 160x90

	int pitch = width;

	int xy[2]=
	{
		(wnd_wh[0] - width * (fnt_wh[0] >> 4)) / 2,
		(wnd_wh[1] - height * (fnt_wh[1] >> 4)) / 2,
	};

	float zoom = 1.0;
	Render(terrain, world, term->water, zoom, term->yaw, term->pos, width, height, pitch, term->buf);

	// copy term->buf to some texture
	glTextureSubImage2D(term->tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, term->buf);


	glUniform2i(0, width, height);

	glUniform1i(1, 0);
	glBindTextureUnit(0, term->tex);

	glUniform1i(2, 1);
	glBindTextureUnit(1, fnt_tex);

	glUniform2i(3, term->max_width, term->max_height);
	
	glBindVertexArray(term->vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

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

	int loglen = 999;
	char logstr[1000] = "";
	GLuint shader[2] = { 0,0 };

	glCreateTextures(GL_TEXTURE_2D, 1, &term->tex);
	glTextureStorage2D(term->tex, 1, GL_RGBA8, term->max_width, term->max_height);

	const char* term_vs_src =
		CODE(#version 450\n)
		CODE(
			layout(location = 0) uniform ivec2 ansi_vp;  // viewport size in cells
			in vec2 uv; // normalized to viewport size
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
			uniform vec3 pal[252] = vec3[252](
				P(0, 0, 0), P(0, 0, 1), P(0, 0, 2), P(0, 0, 3), P(0, 0, 4), P(0, 0, 5),
				P(0, 1, 0), P(0, 1, 1), P(0, 1, 2), P(0, 1, 3), P(0, 1, 4), P(0, 1, 5),
				P(0, 2, 0), P(0, 2, 1), P(0, 2, 2), P(0, 2, 3), P(0, 2, 4), P(0, 2, 5),
				P(0, 3, 0), P(0, 3, 1), P(0, 3, 2), P(0, 3, 3), P(0, 3, 4), P(0, 3, 5),
				P(0, 4, 0), P(0, 4, 1), P(0, 4, 2), P(0, 4, 3), P(0, 4, 4), P(0, 4, 5),
				P(0, 5, 0), P(0, 5, 1), P(0, 5, 2), P(0, 5, 3), P(0, 5, 4), P(0, 5, 5),
				P(0, 6, 0), P(0, 6, 1), P(0, 6, 2), P(0, 6, 3), P(0, 6, 4), P(0, 6, 5),

				P(1, 0, 0), P(1, 0, 1), P(1, 0, 2), P(1, 0, 3), P(1, 0, 4), P(1, 0, 5),
				P(1, 1, 0), P(1, 1, 1), P(1, 1, 2), P(1, 1, 3), P(1, 1, 4), P(1, 1, 5),
				P(1, 2, 0), P(1, 2, 1), P(1, 2, 2), P(1, 2, 3), P(1, 2, 4), P(1, 2, 5),
				P(1, 3, 0), P(1, 3, 1), P(1, 3, 2), P(1, 3, 3), P(1, 3, 4), P(1, 3, 5),
				P(1, 4, 0), P(1, 4, 1), P(1, 4, 2), P(1, 4, 3), P(1, 4, 4), P(1, 4, 5),
				P(1, 5, 0), P(1, 5, 1), P(1, 5, 2), P(1, 5, 3), P(1, 5, 4), P(1, 5, 5),
				P(1, 6, 0), P(1, 6, 1), P(1, 6, 2), P(1, 6, 3), P(1, 6, 4), P(1, 6, 5),

				P(2, 0, 0), P(2, 0, 1), P(2, 0, 2), P(2, 0, 3), P(2, 0, 4), P(2, 0, 5),
				P(2, 1, 0), P(2, 1, 1), P(2, 1, 2), P(2, 1, 3), P(2, 1, 4), P(2, 1, 5),
				P(2, 2, 0), P(2, 2, 1), P(2, 2, 2), P(2, 2, 3), P(2, 2, 4), P(2, 2, 5),
				P(2, 3, 0), P(2, 3, 1), P(2, 3, 2), P(2, 3, 3), P(2, 3, 4), P(2, 3, 5),
				P(2, 4, 0), P(2, 4, 1), P(2, 4, 2), P(2, 4, 3), P(2, 4, 4), P(2, 4, 5),
				P(2, 5, 0), P(2, 5, 1), P(2, 5, 2), P(2, 5, 3), P(2, 5, 4), P(2, 5, 5),
				P(2, 6, 0), P(2, 6, 1), P(2, 6, 2), P(2, 6, 3), P(2, 6, 4), P(2, 6, 5),

				P(3, 0, 0), P(3, 0, 1), P(3, 0, 2), P(3, 0, 3), P(3, 0, 4), P(3, 0, 5),
				P(3, 1, 0), P(3, 1, 1), P(3, 1, 2), P(3, 1, 3), P(3, 1, 4), P(3, 1, 5),
				P(3, 2, 0), P(3, 2, 1), P(3, 2, 2), P(3, 2, 3), P(3, 2, 4), P(3, 2, 5),
				P(3, 3, 0), P(3, 3, 1), P(3, 3, 2), P(3, 3, 3), P(3, 3, 4), P(3, 3, 5),
				P(3, 4, 0), P(3, 4, 1), P(3, 4, 2), P(3, 4, 3), P(3, 4, 4), P(3, 4, 5),
				P(3, 5, 0), P(3, 5, 1), P(3, 5, 2), P(3, 5, 3), P(3, 5, 4), P(3, 5, 5),
				P(3, 6, 0), P(3, 6, 1), P(3, 6, 2), P(3, 6, 3), P(3, 6, 4), P(3, 6, 5),

				P(4, 0, 0), P(4, 0, 1), P(4, 0, 2), P(4, 0, 3), P(4, 0, 4), P(4, 0, 5),
				P(4, 1, 0), P(4, 1, 1), P(4, 1, 2), P(4, 1, 3), P(4, 1, 4), P(4, 1, 5),
				P(4, 2, 0), P(4, 2, 1), P(4, 2, 2), P(4, 2, 3), P(4, 2, 4), P(4, 2, 5),
				P(4, 3, 0), P(4, 3, 1), P(4, 3, 2), P(4, 3, 3), P(4, 3, 4), P(4, 3, 5),
				P(4, 4, 0), P(4, 4, 1), P(4, 4, 2), P(4, 4, 3), P(4, 4, 4), P(4, 4, 5),
				P(4, 5, 0), P(4, 5, 1), P(4, 5, 2), P(4, 5, 3), P(4, 5, 4), P(4, 5, 5),
				P(4, 6, 0), P(4, 6, 1), P(4, 6, 2), P(4, 6, 3), P(4, 6, 4), P(4, 6, 5),

				P(5, 0, 0), P(5, 0, 1), P(5, 0, 2), P(5, 0, 3), P(5, 0, 4), P(5, 0, 5),
				P(5, 1, 0), P(5, 1, 1), P(5, 1, 2), P(5, 1, 3), P(5, 1, 4), P(5, 1, 5),
				P(5, 2, 0), P(5, 2, 1), P(5, 2, 2), P(5, 2, 3), P(5, 2, 4), P(5, 2, 5),
				P(5, 3, 0), P(5, 3, 1), P(5, 3, 2), P(5, 3, 3), P(5, 3, 4), P(5, 3, 5),
				P(5, 4, 0), P(5, 4, 1), P(5, 4, 2), P(5, 4, 3), P(5, 4, 4), P(5, 4, 5),
				P(5, 5, 0), P(5, 5, 1), P(5, 5, 2), P(5, 5, 3), P(5, 5, 4), P(5, 5, 5),
				P(5, 6, 0), P(5, 6, 1), P(5, 6, 2), P(5, 6, 3), P(5, 6, 4), P(5, 6, 5)
			);

			layout(location = 0) out vec4 color;
			layout(location = 1) uniform sampler2D ansi;
			layout(location = 2) uniform sampler2D font;
			layout(location = 3) uniform ivec2 ansi_wh;  // ansi texture size (in cells), constant = 160x90
			in vec2 cell_coord;
			void main()
			{
				// sample ansi buffer
				vec2 quot_cell = floor(cell_coord);
				vec2 frac_cell = fract(cell_coord);

				vec2 ansi_coord = (quot_cell + vec2(0.5)) / ansi_wh;

				vec4 cell = texture(ansi, ansi_coord);

				int glyph_idx = int(round(cell.b * 255.0));
				vec2 glyph_coord = ( vec2(glyph_idx & 0xF, glyph_idx >> 4) + frac_cell ) / vec2(16.0);

				float glyph_alpha = texture(font, glyph_coord).a;

				vec3 fg_color = pal[int(round(cell.r * 255.0))];
				vec3 bg_color = pal[int(round(cell.g * 255.0))];

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

	const char* utf8 = "ASCIIID Term";

	a3dSetTitle(wnd, utf8);
	a3dSetIcon(wnd, "./icons/app.png");
	a3dSetVisible(wnd, true);
}

void term_keyb_char(A3D_WND* wnd, wchar_t chr)
{
}

void term_keyb_key(A3D_WND* wnd, KeyInfo ki, bool down)
{
}

void term_keyb_focus(A3D_WND* wnd, bool set)
{
}

void term_close(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	glDeleteTextures(1, &term->tex);
	glDeleteVertexArrays(1, &term->vao);
	glDeleteBuffers(1, &term->vbo);
	glDeleteProgram(term->prg);

	a3dClose(wnd);

	if (term->buf)
		free(term->buf);

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

	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	term->yaw = yaw;
	term->pos[0] = pos[0];
	term->pos[1] = pos[1];
	term->pos[2] = pos[2];
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

