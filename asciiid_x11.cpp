
// PLATFORM: LINUX

// sudo apt install libx11-dev
// sudo apt-get install libglu1-mesa-dev
// gcc -o asciiid_x11 asciiid_x11.cpp -lGL -lX11 -L/usr/X11R6/lib -I/usr/X11R6/include

#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>

#include <stdint.h>
#include <string.h>

#include "gl.h"

#include <GL/glx.h>
#include <GL/glxext.h>

#include "asciiid_platform.h"

Display* dpy;
Window win;

static PlatformInterface platform_api;
static int mouse_b = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static bool track = false;
static bool closing = false;

static const int ks_to_vk[256] =
{
	0,

	XK_BackSpace,
	XK_Tab,
	XK_Return,

	XK_Pause,
	XK_Escape,

	XK_space, // printable!
	XK_Page_Up,
	XK_Page_Down,
	XK_End,
	XK_Home,
	XK_Left,
	XK_Up,
	XK_Right,
	XK_Down,

	XK_Print,
	XK_Insert,
	XK_Delete,

	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',

	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z',

	XK_Meta_L,
	XK_Meta_R,
	XK_Menu,

	XK_KP_0,
	XK_KP_1,
	XK_KP_2,
	XK_KP_3,
	XK_KP_4,
	XK_KP_5,
	XK_KP_6,
	XK_KP_7,
	XK_KP_8,
	XK_KP_9,
	XK_KP_Multiply,
	XK_KP_Divide,
	XK_KP_Add,
	XK_KP_Subtract,
	XK_KP_Decimal,
	XK_KP_Enter,

	XK_F1,
	XK_F2,
	XK_F3,
	XK_F4,
	XK_F5,
	XK_F6,
	XK_F7,
	XK_F8,
	XK_F9,
	XK_F10,
	XK_F11,
	XK_F12,
	XK_F13,
	XK_F14,
	XK_F15,
	XK_F16,
	XK_F17,
	XK_F18,
	XK_F19,
	XK_F20,
	XK_F21,
	XK_F22,
	XK_F23,
	XK_F24,

	XK_Caps_Lock,
	XK_Num_Lock,
	XK_Scroll_Lock,

	XK_Shift_L,
	XK_Shift_R,
	XK_Control_L,
	XK_Control_R,
	XK_Alt_L,
	XK_Alt_R,

	// these should come in pairs (regular & shifted) 

	XK_semicolon, 		//XK_semicolon,	// ';:'
	XK_equal, 		//XK_plus,		// '=+' any country
	XK_comma, 		//XK_less,		// ',<' any country
	XK_minus, 		//XK_underscore,	// '-_' any country
	XK_period, 		//XK_greater,		// '.>' any country
	XK_slash, 		//XK_question,		// '/?' for US
	XK_grave, 		//XK_asciitilde,	// '`~' for US

	XK_bracketleft, 	//XK_braceleft,   	//  '[{' for US
	XK_bracketright, 	//XK_braceright,	//  ']}' for US
	XK_backslash, 		//XK_bar,		//  '\|' for US
	XK_apostrophe, 		//XK_quotedbl		//  ''"' for US
	0,
};

/*
LRESULT WINAPI a3dWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	int mi = 0;
	int rep = 1;

	switch (m)
	{
		case WM_CREATE:
			closing = false;
			track = false;
			mouse_b = 0;

			QueryPerformanceFrequency(&timer_freq);
			QueryPerformanceCounter(&coarse_perf);
			coarse_micro = 0;

			// set timer to refresh coarse_perf & coarse_micro every minute
			// to prevent overflows in a3dGetTIme 
			SetTimer(h, 1, 60000, 0);
			return TRUE;

		case WM_ENTERMENULOOP:
		{
			SetTimer(h, 2, 0, 0);
			break;
		}

		case WM_ENTERSIZEMOVE:
		{
			SetTimer(h, 3, 0, 0);
			break;
		}

		case WM_EXITSIZEMOVE:
		{
			KillTimer(h, 3);
			break;
		}

		case WM_EXITMENULOOP:
		{
			KillTimer(h, 2);
			break;
		}

		case WM_TIMER:
		{
			if (w == 1)
			{
				LARGE_INTEGER c;
				QueryPerformanceCounter(&c);
				uint64_t diff = c.QuadPart - coarse_perf.QuadPart;
				uint64_t seconds = diff / timer_freq.QuadPart;
				coarse_perf.QuadPart += seconds * timer_freq.QuadPart;
				coarse_micro += seconds * 1000000;
			}
			else
			if (w == 2 || w == 3)
			{
				if (platform_api.render)
					platform_api.render();
			}
			break;
		}
		
		case WM_CLOSE:
			if (platform_api.close)
				platform_api.close();
			else
			{
				memset(&platform_api, 0, sizeof(PlatformInterface));
				closing = true;
			}
			return 0;

		case WM_SIZE:
			if (platform_api.resize)
				platform_api.resize(LOWORD(l),HIWORD(l));
			return 0;

		case WM_ERASEBKGND:
			return 0;

		case WM_PAINT:
			ValidateRect(h, 0);
			return 0;

		case WM_MOUSEWHEEL:
			rep = GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA;
			if (rep > 0)
				mi = MouseInfo::WHEEL_UP;
			else
			if (rep < 0)
			{
				mi = MouseInfo::WHEEL_DN;
				rep = -rep;
			}
			else
				return 0;
			break;

		case WM_SETFOCUS:
			if (platform_api.keyb_focus)
				platform_api.keyb_focus(true);
			break;

		case WM_KILLFOCUS:
			if (platform_api.keyb_focus)
				platform_api.keyb_focus(false);
			break;

		case WM_CHAR:
			if (platform_api.keyb_char)
				platform_api.keyb_char((wchar_t)w);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
			if (platform_api.keyb_key && w < 256)
			{
				KeyInfo ki = (KeyInfo)vk_to_ki[w];
				if (!ki)
					break;

				if ((l >> 24) & 1) // enh
				{
					if (ki == A3D_LSHIFT)
						ki = A3D_RSHIFT;
					else
					if (ki == A3D_LCTRL)
						ki = A3D_RCTRL;
					else
					if (ki == A3D_LALT)
						ki = A3D_RALT;
					else
					if (ki == A3D_ENTER)
						ki = A3D_NUMPAD_ENTER;
				}

				platform_api.keyb_key(ki, m == WM_KEYDOWN);
			}
			break;

		case WM_MOUSELEAVE:
			track = false;
			if (platform_api.mouse)
				platform_api.mouse(mouse_x, mouse_y, MouseInfo::LEAVE);
			break;

		case WM_MOUSEMOVE:
			mouse_x = (short)LOWORD(l);
			mouse_y = (short)HIWORD(l);
			mi = MouseInfo::MOVE;
			if (!track)
			{
				mi = MouseInfo::ENTER;
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.dwHoverTime = HOVER_DEFAULT;
				tme.hwndTrack = h;
				TrackMouseEvent(&tme);
				track = true;
			}
			break;

		case WM_LBUTTONDOWN:
			mouse_b |= MouseInfo::LEFT;
			mi = MouseInfo::LEFT_DN;
			SetCapture(h);
			break;

		case WM_LBUTTONUP:
			mouse_b &= ~MouseInfo::LEFT;
			mi = MouseInfo::LEFT_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;

		case WM_RBUTTONDOWN:
			mouse_b |= MouseInfo::RIGHT;
			mi = MouseInfo::RIGHT_DN;
			SetCapture(h);
			break;

		case WM_RBUTTONUP:
			mouse_b &= ~MouseInfo::RIGHT;
			mi = MouseInfo::RIGHT_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;

		case WM_MBUTTONDOWN:
			mouse_b |= MouseInfo::MIDDLE;
			mi = MouseInfo::MIDDLE_DN;
			SetCapture(h);
			break;

		case WM_MBUTTONUP:
			mouse_b &= ~MouseInfo::MIDDLE;
			mi = MouseInfo::MIDDLE_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;
	}

	if (mi && platform_api.mouse)
	{
		if (w & MK_LBUTTON)
			mi |= MouseInfo::LEFT;
		if (w & MK_RBUTTON)
			mi |= MouseInfo::RIGHT;
		if (w & MK_MBUTTON)
			mi |= MouseInfo::MIDDLE;

		for (int i=0; i<rep; i++)
			platform_api.mouse(mouse_x, mouse_y, (MouseInfo)mi);

		return 0;
	}

	return DefWindowProc(h, m, w, l);
}
*/

// creates window & initialized GL
bool a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd/*, const AudioDesc* ad*/)
{
	if (!pi || !gd)
		return false;


	GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	GLXContext              glc;
	XWindowAttributes       gwa;
	XEvent                  xev;

	// dpy is global
	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return false;

	Window root = DefaultRootWindow(dpy);
	if (!root)
	{
		XCloseDisplay(dpy);
		return false;
	}

	XVisualInfo* vi = glXChooseVisual(dpy, 0, att);
	if (!vi)
	{
		XCloseDisplay(dpy);
		return false;
	}

	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | VisibilityChangeMask | KeyPressMask | PointerMotionMask | StructureNotifyMask;

	// win is global
	win = XCreateWindow(dpy, root, 0, 0, 800, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	if (!win)
	{
		XCloseDisplay(dpy);
		return false;
	}

 	XMapWindow(dpy, win);
	XStoreName(dpy, win, "asciiid");

 	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	if (!glc)
	{
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		return false;
	}

 	if (!glXMakeCurrent(dpy, win, glc))
	{
		glXDestroyContext(dpy, glc);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		return false;
	}

	platform_api = *pi;

	if (platform_api.init)
		platform_api.init();

	// send initial notifications:
	/*
	RECT r;
	GetClientRect(h, &r);
	if (platform_api.resize)
		platform_api.resize(r.right,r.bottom);
	*/

	while (!closing)
	{
		XNextEvent(dpy, &xev);
		
		if(xev.type == Expose) 
		{
		        XGetWindowAttributes(dpy, win, &gwa);
			
			// TODO
			// if gwa != previous send resize cb


			if (platform_api.render)
				platform_api.render();
		}
		        
		else if(xev.type == KeyPress) 
		{
		        glXMakeCurrent(dpy, None, NULL);
		        glXDestroyContext(dpy, glc);
		        XDestroyWindow(dpy, win);
		        XCloseDisplay(dpy);
		        break;
		}
	}

	return true;
}

void a3dClose()
{
	memset(&platform_api, 0, sizeof(PlatformInterface));
	closing = true;
}

uint64_t a3dGetTime()
{
	static uint64_t dummy = 0;
	return dummy+=10000; // 10ms dummy advance
	// TODO:
	/*
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	uint64_t diff = c.QuadPart - coarse_perf.QuadPart;
	return coarse_micro + diff * 1000000 / timer_freq.QuadPart;
	// we can handle diff upto 3 minutes @ 100GHz clock
	// this is why we refresh coarse time every minute on WM_TIMER
	*/
}

void a3dSwapBuffers()
{
	glXSwapBuffers(dpy, win);
}

bool a3dGetKeyb(KeyInfo ki)
{
	return 0;
	// TODO
	/*
	return 0 != (1 & (GetKeyState(ki_to_vk[ki])>>15));
	*/
}

void a3dSetTitle(const wchar_t* name)
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	SetWindowText(hWnd, name);
	*/
}

int a3dGetTitle(wchar_t* name, int size)
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	if (name)
		GetWindowText(hWnd, name, size);
	return GetWindowTextLength(hWnd)+1;
	*/
	return 0;
}

void a3dSetVisible(bool visible)
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);
	*/
}

bool a3dGetVisible()
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	if (GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE)
		return true;
	*/
	return false;
}

// resize
bool a3dGetRect(int* xywh)
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	RECT r;
	GetWindowRect(hWnd, &r);
	if (xywh)
	{
		xywh[0] = r.left;
		xywh[1] = r.top;
		xywh[2] = r.right - r.left;
		xywh[3] = r.bottom - r.top;
	}

	if (GetWindowLong(hWnd, GWL_STYLE) & WS_CAPTION)
		return true;
	*/
	return false;
}

void a3dSetRect(const int* xywh, bool wnd_mode)
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	DWORD s = GetWindowLong(hWnd, GWL_STYLE);
	DWORD ns = s;

	RECT r;
	RECT nr;
	GetWindowRect(hWnd, &r);

	nr.left = xywh[0];
	nr.top = xywh[1];
	nr.right = xywh[2] + xywh[0];
	nr.bottom = xywh[3] + xywh[1];

	if (wnd_mode)
		ns |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
	else
		ns &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

	if (r.left != nr.left || r.top != nr.top ||
		r.right != nr.right || r.bottom != nr.bottom)
	{
		if (ns != s)
		{
			SetWindowLong(hWnd, GWL_STYLE, ns);
			SetWindowPos(hWnd, 0, nr.left, nr.top, nr.right - nr.left, nr.bottom - nr.top,
				SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
		}
		else
			SetWindowPos(hWnd, 0, nr.left, nr.top, nr.right - nr.left, nr.bottom - nr.top, SWP_NOZORDER);
	}
	else
	if (ns != s)
		SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);
	*/
}


// mouse
MouseInfo a3dGetMouse(int* x, int* y) // returns but flags, mouse wheel has no state
{
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hWnd, &p);
	if (x)
		*x = p.x;
	if (y)
		*y = p.y;

	int fl = 0;

	if (0x8000 & GetKeyState(VK_LBUTTON))
		fl |= MouseInfo::LEFT;
	if (0x8000 & GetKeyState(VK_RBUTTON))
		fl |= MouseInfo::RIGHT;
	if (0x8000 & GetKeyState(VK_MBUTTON))
		fl |= MouseInfo::MIDDLE;

	return (MouseInfo)fl;
	*/
	return (MouseInfo)0;
}

// keyb_key
bool a3dGetKeybKey(KeyInfo ki) // return true if vk is down, keyb_char has no state
{
	if (ki < 0 || ki >= A3D_MAPEND)
		return false;
	// TODO
	/*
	if (GetKeyState(ki_to_vk[ki]) & 0x8000)
		return true;
	*/
	return false;
}

// keyb_focus
bool a3dGetFocus()
{
	return false;
	// TODO
	/*
	HWND hWnd = WindowFromDC(wglGetCurrentDC());
	return GetFocus() == hWnd;
	*/
}
