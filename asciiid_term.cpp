
#include <malloc.h>
#include <stdio.h>
#include "asciiid_term.h"
#include "asciiid_platform.h"
#include "fast_rand.h"
#include "gl.h"

struct TERM_LIST
{
	TERM_LIST* prev;
	TERM_LIST* next;
	A3D_WND* wnd;
};

TERM_LIST* term_head = 0;
TERM_LIST* term_tail = 0;

void term_render(A3D_WND* wnd)
{
	TERM_LIST* term = (TERM_LIST*)a3dGetCookie(wnd);

	glClearColor((fast_rand() & 0xFF) / 255.0f, (fast_rand() & 0xFF) / 255.0f, (fast_rand() & 0xFF) / 255.0f, (fast_rand() & 0xFF) / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	// access terrain & world
	// calc view transform and clipping planes

	// invoke CPU rasterization into our framebuffer

	// use our GL textures, shaders and editor's current font texture to blit framebuffer to screen
	// ...
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

	// TODO:
	// get editor angles and translation 
	// clamp & round them to term constrains

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

void TermOpen(A3D_WND* share)
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

	a3dOpen(&pi, &gd, share);
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

