
// PLATFORM: LINUX

// sudo apt install libx11-dev
// sudo apt-get install libglu1-mesa-dev
// gcc -o asciiid_x11 asciiid_x11.cpp -lGL -lX11 -L/usr/X11R6/lib -I/usr/X11R6/include

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <time.h>
#include <unistd.h> // usleep

#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "gl.h"

#include <GL/glx.h>

#include <GL/glxext.h> // <- we'll need it to request versioned context

#include "asciiid_platform.h"

bool wndmode = false;
bool mapped = false;
char title[256]="A3D";
Display* dpy;
Window win;

static PlatformInterface platform_api;
static int mouse_b = 0;
static int mouse_x = 0;
static int mouse_y = 0;
static bool track = false;
static bool closing = false;
static int force_key = A3D_NONE;

const char* caps[]=
{
	"A3D_NONE",
	"A3D_BACKSPACE",
	"A3D_TAB",
	"A3D_ENTER",
	"A3D_PAUSE",
	"A3D_ESCAPE",
	"A3D_SPACE",
	"A3D_PAGEUP",
	"A3D_PAGEDOWN",
	"A3D_END",
	"A3D_HOME",
	"A3D_LEFT",
	"A3D_UP",
	"A3D_RIGHT",
	"A3D_DOWN",
	"A3D_PRINT",
	"A3D_INSERT",
	"A3D_DELETE",
	"A3D_0",
	"A3D_1",
	"A3D_2",
	"A3D_3",
	"A3D_4",
	"A3D_5",
	"A3D_6",
	"A3D_7",
	"A3D_8",
	"A3D_9",
	"A3D_A",
	"A3D_B",
	"A3D_C",
	"A3D_D",
	"A3D_E",
	"A3D_F",
	"A3D_G",
	"A3D_H",
	"A3D_I",
	"A3D_J",
	"A3D_K",
	"A3D_L",
	"A3D_M",
	"A3D_N",
	"A3D_O",
	"A3D_P",
	"A3D_Q",
	"A3D_R",
	"A3D_S",
	"A3D_T",
	"A3D_U",
	"A3D_V",
	"A3D_W",
	"A3D_X",
	"A3D_Y",
	"A3D_Z",
	"A3D_LWIN",
	"A3D_RWIN",
	"A3D_APPS",
	"A3D_NUMPAD_0",
	"A3D_NUMPAD_1",
	"A3D_NUMPAD_2",
	"A3D_NUMPAD_3",
	"A3D_NUMPAD_4",
	"A3D_NUMPAD_5",
	"A3D_NUMPAD_6",
	"A3D_NUMPAD_7",
	"A3D_NUMPAD_8",
	"A3D_NUMPAD_9",
	"A3D_NUMPAD_MULTIPLY",
	"A3D_NUMPAD_DIVIDE",
	"A3D_NUMPAD_ADD",
	"A3D_NUMPAD_SUBTRACT",
	"A3D_NUMPAD_DECIMAL",
	"A3D_NUMPAD_ENTER",
	"A3D_F1",
	"A3D_F2",
	"A3D_F3",
	"A3D_F4",
	"A3D_F5",
	"A3D_F6",
	"A3D_F7",
	"A3D_F8",
	"A3D_F9",
	"A3D_F10",
	"A3D_F11",
	"A3D_F12",
	"A3D_F13",
	"A3D_F14",
	"A3D_F15",
	"A3D_F16",
	"A3D_F17",
	"A3D_F18",
	"A3D_F19",
	"A3D_F20",
	"A3D_F21",
	"A3D_F22",
	"A3D_F23",
	"A3D_F24",
	"A3D_CAPSLOCK",
	"A3D_NUMLOCK",
	"A3D_SCROLLLOCK",
	"A3D_LSHIFT",
	"A3D_RSHIFT",
	"A3D_LCTRL",
	"A3D_RCTRL",
	"A3D_LALT",
	"A3D_RALT",
	"A3D_OEM_COLON",	
	"A3D_OEM_PLUS",		
	"A3D_OEM_COMMA",	
	"A3D_OEM_MINUS",	
	"A3D_OEM_PERIOD",	
	"A3D_OEM_SLASH",	
	"A3D_OEM_TILDE",	
	"A3D_OEM_OPEN",     
	"A3D_OEM_CLOSE",    
	"A3D_OEM_BACKSLASH",
	"A3D_OEM_QUOTATION",
};	

static const unsigned char ki_to_kc[] =
{
	0,		// A3D_NONE
	22,		// A3D_BACKSPACE
	23,		// A3D_TAB
	36,		// A3D_ENTER

	127,	// A3D_PAUSE
	9,		// A3D_ESCAPE

	65,		// A3D_SPACE
	112,	// A3D_PAGEUP
	117,	// A3D_PAGEDOWN
	115,	// A3D_END
	110,	// A3D_HOME
	113,	// A3D_LEFT
	111,	// A3D_UP
	114,	// A3D_RIGHT
	116,	// A3D_DOWN

	0,		// A3D_PRINT
	118,	// A3D_INSERT
	119,	// A3D_DELETE

	19,		// A3D_0
	10,		// A3D_1
	11,		// A3D_2
	12,		// A3D_3
	13,		// A3D_4
	14,		// A3D_5
	15,		// A3D_6
	16,		// A3D_7
	17,		// A3D_8
	18,		// A3D_9

	38,		// A3D_A
	56,		// A3D_B
	54,		// A3D_C
	40,		// A3D_D
	26,		// A3D_E
	41,		// A3D_F
	42,		// A3D_G
	43,		// A3D_H
	31,		// A3D_I
	44,		// A3D_J
	45,		// A3D_K
	46,		// A3D_L
	58,		// A3D_M
	57,		// A3D_N
	32,		// A3D_O
	33,		// A3D_P
	24,		// A3D_Q
	27,		// A3D_R
	39,		// A3D_S
	28,		// A3D_T
	30,		// A3D_U
	55,		// A3D_V
	25,		// A3D_W
	53,		// A3D_X
	29,		// A3D_Y
	52,		// A3D_Z

	0,		// A3D_LWIN
	0,		// A3D_RWIN
	0,		// A3D_APPS

	90,		// A3D_NUMPAD_0
	87,		// A3D_NUMPAD_1
	88,		// A3D_NUMPAD_2
	89,		// A3D_NUMPAD_3
	83,		// A3D_NUMPAD_4
	84,		// A3D_NUMPAD_5
	85,		// A3D_NUMPAD_6
	79,		// A3D_NUMPAD_7
	80,		// A3D_NUMPAD_8
	81,		// A3D_NUMPAD_9
	63,		// A3D_NUMPAD_MULTIPLY
	106,	// A3D_NUMPAD_DIVIDE
	86,		// A3D_NUMPAD_ADD
	82,		// A3D_NUMPAD_SUBTRACT
	91,		// A3D_NUMPAD_DECIMAL
	104,	// A3D_NUMPAD_ENTER

	67,		// A3D_F1
	68,		// A3D_F2
	69,		// A3D_F3
	70,		// A3D_F4
	71,		// A3D_F5
	72,		// A3D_F6
	73,		// A3D_F7
	74,		// A3D_F8
	75,		// A3D_F9
	76,		// A3D_F10
	95,		// A3D_F11
	96,		// A3D_F12
	0,		// A3D_F13
	0,		// A3D_F14
	0,		// A3D_F15
	0,		// A3D_F16
	0,		// A3D_F17
	0,		// A3D_F18
	0,		// A3D_F19
	0,		// A3D_F20
	0,		// A3D_F21
	0,		// A3D_F22
	0,		// A3D_F23
	0,		// A3D_F24

	66,		// A3D_CAPSLOCK
	77,		// A3D_NUMLOCK
	78,		// A3D_SCROLLLOCK

	50,		// A3D_LSHIFT
	62,		// A3D_RSHIFT
	37,		// A3D_LCTRL
	105,	// A3D_RCTRL
	64,		// A3D_LALT
	108,	// A3D_RALT

	47,		// A3D_OEM_COLON
	21,		// A3D_OEM_PLUS
	59,		// A3D_OEM_COMMA
	20,		// A3D_OEM_MINUS
	60,		// A3D_OEM_PERIOD
	61,		// A3D_OEM_SLASH
	49,		// A3D_OEM_TILDE

	34,		// A3D_OEM_OPEN
	35,		// A3D_OEM_CLOSE
	51,		// A3D_OEM_BACKSLASH
	48,		// A3D_OEM_QUOTATION
};

static const unsigned char kc_to_ki[128]=
{
	// ok

	0,0,0,0,0,0,0,0,
	0,						// 8
	A3D_ESCAPE,				// 9
	A3D_1,					// 10
	A3D_2,					// 11
	A3D_3,					// 12
	A3D_4,					// 13
	A3D_5,					// 14
	A3D_6,					// 15
	A3D_7,					// 16
	A3D_8,					// 17
	A3D_9,					// 18
	A3D_0,					// 19
	A3D_OEM_MINUS,			// 20
	A3D_OEM_PLUS,			// 21
	A3D_BACKSPACE,			// 22
	A3D_TAB,				// 23
	A3D_Q,					// 24
	A3D_W,					// 25
	A3D_E,					// 26
	A3D_R,					// 27
	A3D_T,					// 28
	A3D_Y,					// 29
	A3D_U,					// 30
	A3D_I,					// 31
	A3D_O,					// 32
	A3D_P,					// 33
	A3D_OEM_OPEN,			// 34
	A3D_OEM_CLOSE,			// 35
	A3D_ENTER,				// 36
	A3D_LCTRL,				// 37
	A3D_A,					// 38
	A3D_S,					// 39
	A3D_D,					// 40
	A3D_F,					// 41
	A3D_G,					// 42
	A3D_H,					// 43
	A3D_J,					// 44
	A3D_K,					// 45
	A3D_L,					// 46
	A3D_OEM_COLON,			// 47
	A3D_OEM_QUOTATION,		// 48
	A3D_OEM_TILDE,			// 49
	A3D_LSHIFT,				// 50
	A3D_OEM_BACKSLASH,		// 51
	A3D_Z,					// 52
	A3D_X,					// 53
	A3D_C,					// 54
	A3D_V,					// 55
	A3D_B,					// 56
	A3D_N,					// 57
	A3D_M,//58				// 58
	A3D_OEM_COMMA,			// 59
	A3D_OEM_PERIOD,			// 60
	A3D_OEM_SLASH,			// 61
	A3D_RSHIFT,				// 62
	A3D_NUMPAD_MULTIPLY,	// 63
	A3D_LALT,				// 64
	A3D_SPACE,				// 65
	A3D_CAPSLOCK,			// 66
	A3D_F1,					// 67
	A3D_F2,					// 68
	A3D_F3,					// 69
	A3D_F4,					// 70
	A3D_F5,					// 71
	A3D_F6,					// 72
	A3D_F7,					// 73
	A3D_F8,					// 74
	A3D_F9,					// 75
	A3D_F10,				// 76
	A3D_NUMLOCK,			// 77
	A3D_SCROLLLOCK,			// 78
	A3D_NUMPAD_7,			// 79
	A3D_NUMPAD_8,			// 80
	A3D_NUMPAD_9,			// 81
	A3D_NUMPAD_SUBTRACT,	// 82
	A3D_NUMPAD_4,			// 83
	A3D_NUMPAD_5,			// 84
	A3D_NUMPAD_6,			// 85
	A3D_NUMPAD_ADD,			// 86
	A3D_NUMPAD_1,			// 87
	A3D_NUMPAD_2,			// 88
	A3D_NUMPAD_3,			// 89
	A3D_NUMPAD_0,			// 90
	A3D_NUMPAD_DECIMAL,		// 91
	0,						// 92
	0,						// 93
	0,						// 94
	A3D_F11,				// 95
	A3D_F12,				// 96

	// bad

	0,						// 97
	0,						// 98
	0,						// 99
	0,						// 100
	0,						// 101
	0,						// 102
	0,						// 103
	A3D_NUMPAD_ENTER,		// 104
	A3D_RCTRL,				// 105
	A3D_NUMPAD_DIVIDE,		// 106
	0,						// 107
	A3D_RALT,				// 108
	0,						// 109
	A3D_HOME,				// 110
	A3D_UP,					// 111
	A3D_PAGEUP,				// 112
	A3D_LEFT,				// 113
	A3D_RIGHT,				// 114
	A3D_END,				// 115
	A3D_DOWN,				// 116
	A3D_PAGEDOWN,			// 117
	A3D_INSERT,				// 118
	A3D_DELETE,				// 119
	0,						// 120
	0,						// 121
	0,						// 122
	0,						// 123
	0,						// 124
	0,						// 125
	0,						// 126

	// ok

	A3D_PAUSE,				// 127
};

/*
LRESULT WINAPI a3dWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	int mi = 0;
	int rep = 1;

	switch (m)
	{
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

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
		glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
	
	if (!glXCreateContextAttribsARB)
		return false;

	GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	GLXContext              glc;
	XWindowAttributes       gwa;
	XEvent                  xev;

	// dpy is global
	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return false;

	Bool DetectableAutoRepeat = false;
	XkbSetDetectableAutoRepeat(dpy, True, &DetectableAutoRepeat);

	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

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
	swa.event_mask = 
		StructureNotifyMask |
		PropertyChangeMask | 
		VisibilityChangeMask | 
		FocusChangeMask |
		ExposureMask | 
		PointerMotionMask | 
		KeyPressMask | 
		KeyReleaseMask | 
		ButtonPressMask | 
		ButtonReleaseMask |
		EnterWindowMask |
		LeaveWindowMask;

	// win is global
	wndmode = true;
	win = XCreateWindow(dpy, root, 0, 0, 800, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	if (!win)
	{
		XCloseDisplay(dpy);
		return false;
	}

	XSetWMProtocols(dpy, win, &wm_delete_window, 1);		

 	// XMapWindow(dpy, win);
	mapped = false;

	XStoreName(dpy, win, "asciiid");

	int nelements;
	GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &nelements);	
	int attribs[] = {
		GLX_CONTEXT_FLAGS_ARB, gd->flags & GraphicsDesc::DEBUG_CONTEXT ? GLX_CONTEXT_DEBUG_BIT_ARB : 0,
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 6,
		0};

	glc = glXCreateContextAttribsARB(dpy, *fbc, 0, true, attribs);
 	//glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);

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
	XGetWindowAttributes(dpy, win, &gwa);
	int w = gwa.width, h = gwa.height;

	if (platform_api.resize)
		platform_api.resize(w,h);

	//int key_seq = 0;
	//printf("%s:\n",caps[key_seq]);

	while (!closing)
	{
		if (XPending(dpy))
		{
			XNextEvent(dpy, &xev);

			if (xev.type == ClientMessage)
			{
				if ((Atom)xev.xclient.data.l[0] == wm_delete_window) 
				{
					glXMakeCurrent(dpy, None, NULL);
					glXDestroyContext(dpy, glc);
					XDestroyWindow(dpy,win);
					break;
				}
			}

			if (xev.type == ConfigureNotify)
			{
				if (xev.xconfigure.width != w || xev.xconfigure.height != h)
				{
					w = xev.xconfigure.width;
					h = xev.xconfigure.height;
					if (platform_api.resize)
						platform_api.resize(w, h);
				}
			}
			else
			if (xev.type == FocusIn)
			{
				if (platform_api.keyb_focus)
					platform_api.keyb_focus(true);
			}
			else
			if (xev.type == FocusOut)
			{
				if (platform_api.keyb_focus)
					platform_api.keyb_focus(false);
			}
			else
			if (xev.type == Expose) 
			{
				if (platform_api.render && mapped)
					platform_api.render();
			}
			else 
			if(xev.type == EnterNotify) 
			{
				int state = 0;
				state |= xev.xcrossing.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xcrossing.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xcrossing.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				if (platform_api.mouse)
					platform_api.mouse(xev.xcrossing.x,xev.xcrossing.y,(MouseInfo)(MouseInfo::ENTER | state));
			}
			else 
			if(xev.type == LeaveNotify) 
			{
				int state = 0;
				state |= xev.xcrossing.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xcrossing.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xcrossing.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				if (platform_api.mouse)
					platform_api.mouse(xev.xcrossing.x,xev.xcrossing.y, (MouseInfo)(MouseInfo::LEAVE | state));
			}			
			else 
			if(xev.type == KeyPress) 
			{
				if (platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						//printf("PRESS: %s\n",caps[kc_to_ki[kc]]);

						force_key = kc_to_ki[kc];
						platform_api.keyb_key((KeyInfo)kc_to_ki[kc],true);
						force_key = A3D_NONE;
					}
				}

                XComposeStatus composeStatus;
                char asciiCode[ 32 ];
                KeySym keySym;
                int len;

				// todo switch to XwcLookupString ...
				// thar will require use of: XOpenIM, XCreateIC, XSetICFocus, 
                len = XLookupString( &xev.xkey, asciiCode, sizeof(asciiCode),
                                     &keySym, &composeStatus);

				if (platform_api.keyb_char)
					for (int i=0; i<len; i++)
						platform_api.keyb_char((wchar_t)asciiCode[i]);
			}
			else 
			if(xev.type == KeyRelease) 
			{
				bool physical = DetectableAutoRepeat;

				if (!physical)
				{
					XEvent nev;
					XPeekEvent(dpy, &nev);

					// autorepeat guessing...
					if (nev.type != KeyPress || 
						nev.xkey.time != xev.xkey.time ||
						nev.xkey.keycode != xev.xkey.keycode)
					{
						physical = true;
					}
				}

				if (physical && platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						//printf("RELEASE: %s\n",caps[kc_to_ki[kc]]);
						platform_api.keyb_key((KeyInfo)kc_to_ki[kc],false);
					}			
				}
			}
			else
			if (xev.type == ButtonPress)
			{
				int state = 0;
				MouseInfo mi = (MouseInfo)0;
				switch (xev.xbutton.button)
				{
					case Button1: 
						mi = MouseInfo::LEFT_DN; 
						state |= MouseInfo::LEFT;
						state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
						state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
						break;
					case Button3: 
						mi = MouseInfo::RIGHT_DN; 
						state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
						state |= MouseInfo::RIGHT;
						state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
						break;
					case Button2: 
						mi = MouseInfo::MIDDLE_DN; 
						state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
						state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
						state |= MouseInfo::MIDDLE;
						break;
					case Button5: 
						mi = MouseInfo::WHEEL_DN; 
						state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
						state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
						state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
						break;
					case Button4: 
						mi = MouseInfo::WHEEL_UP; 
						state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
						state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
						state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
						break;
				}

				mi = (MouseInfo)(mi |state);

				if (platform_api.mouse)
					platform_api.mouse(xev.xbutton.x,xev.xbutton.y,mi);				
			}
			else
			if (xev.type == ButtonRelease)
			{
				int state = 0;
				MouseInfo mi = (MouseInfo)0;
				switch (xev.xbutton.button)
				{
				case Button1:
					mi = MouseInfo::LEFT_UP;
					state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
					state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
					break;
				case Button3:
					mi = MouseInfo::RIGHT_UP;
					state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
					state |= xev.xbutton.state & Button2Mask ? MouseInfo::MIDDLE : 0;
					break;
				case Button2:
					mi = MouseInfo::MIDDLE_UP;
					state |= xev.xbutton.state & Button1Mask ? MouseInfo::LEFT : 0;
					state |= xev.xbutton.state & Button3Mask ? MouseInfo::RIGHT : 0;
					break;
				}

				mi = (MouseInfo)(mi | state);

				if (platform_api.mouse)
					platform_api.mouse(xev.xbutton.x,xev.xbutton.y,mi);
			}
			else
			if (xev.type == MotionNotify)
			{
				int state = 0;
				state |= xev.xmotion.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xmotion.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xmotion.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				MouseInfo mi = (MouseInfo)(MouseInfo::MOVE | state);

				if (platform_api.mouse)
					platform_api.mouse(xev.xmotion.x,xev.xmotion.y,mi);
			}
		}
		else
		{
			/*
			int c=0;
			char bits[32]={0};
			XQueryKeymap(dpy, bits);
			for (int i=0; i<256; i++)
			{
				if (bits[i>>3] & (1<<(i&7)))
				{
					printf("%d",i);
					c++;
				}
			}
			if (c)
				printf("\n");
			*/

			if (platform_api.render && mapped)
				platform_api.render();
//			else
//				usleep(20000); // 1/50s
		}
	}

	XCloseDisplay(dpy);

	return true;
}

void a3dClose()
{
	if (!closing)
	{
		XDestroyWindow(dpy,win);
		memset(&platform_api, 0, sizeof(PlatformInterface));
		closing = true;
	}
}

uint64_t a3dGetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void a3dSwapBuffers()
{
	glXSwapBuffers(dpy, win);
}

bool a3dGetKeyb(KeyInfo ki)
{
	/*
	unsigned int n;
	XkbGetIndicatorState(dpy, XkbUseCoreKbd, &n);
	*/

	if (ki <= 0 || ki >= A3D_MAPEND)
		return false;

	if (ki == force_key)
		return true;

	int kc = ki_to_kc[ki];
	if (!kc)
		return false;

	char bits[32]={0};
	XQueryKeymap(dpy, bits);

	return ( bits[kc >> 3] & (1 << (kc & 7)) ) != 0;
}

void a3dSetTitle(const wchar_t* name)
{
//	snprintf(title,255,"%ls", name);
//	XStoreName(dpy, win, title);

    /* This variable will store the window name property. */
    XTextProperty window_name_property;
    /* This variable will store the icon name property. */
	XTextProperty icon_name_property;

    char window_name[] = "hello, world";
	char icon_name[] = "small world";

	char* window_name_ptr = window_name;
	char* icon_name_ptr = icon_name;

	int rc;
    //rc = XStringListToTextProperty(&window_name_ptr, 1, &window_name_property);		
    rc = XStringListToTextProperty(&icon_name_ptr, 1, &icon_name_property);		

    //XSetWMName(dpy, win, &window_name_property);
	XSetWMIconName(dpy, win, &icon_name_property);	

	XFlush(dpy);
}

int a3dGetTitle(wchar_t* name, int size)
{
	if (!name)
		return strlen(title);
	return swprintf(name,size,L"%s",title);
}

void a3dSetVisible(bool visible)
{
	mapped = visible;
	if (visible)
		XMapWindow(dpy, win);
	else
		XUnmapWindow(dpy, win);
}

bool a3dGetVisible()
{
	return mapped;
}

// resize
bool a3dGetRect(int* xywh)
{
	XWindowAttributes gwa;
    XGetWindowAttributes(dpy, win, &gwa);
	xywh[0]=gwa.x;
	xywh[1]=gwa.y;
	xywh[2]=gwa.width;
	xywh[3]=gwa.height;
	return wndmode;
}

void a3dSetRect(const int* xywh, bool wnd_mode)
{
	wndmode = wnd_mode;
	// todo: handle decorations
	XMoveResizeWindow(dpy,win, xywh[0], xywh[1], xywh[2], xywh[3]);
}

// mouse
MouseInfo a3dGetMouse(int* x, int* y) // returns but flags, mouse wheel has no state
{
	// TODO
	Window root;
	Window child;
	int root_x, root_y;
	int win_x, win_y;
	unsigned int mask;

	if (XQueryPointer(dpy, win, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
	{
		int state = 0;
		state |= mask & Button1Mask ? MouseInfo::LEFT : 0;
		state |= mask & Button3Mask ? MouseInfo::RIGHT : 0;
		state |= mask & Button2Mask ? MouseInfo::MIDDLE : 0;

		if (x)
			*x = win_x;
		if (y)
			*y = win_y;

		return (MouseInfo)(MouseInfo::MOVE | state);
	}

	return (MouseInfo)0;
}

// keyb_focus
bool a3dGetFocus()
{
	Window focused;
	int revert_to;
	XGetInputFocus(dpy, &focused, &revert_to);
	return focused == win;
}


#include "upng.h"

bool a3dLoadImage(const char* path, void* cookie, void(*cb)(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf))
{
	upng_t* upng;

	upng = upng_new_from_file(path);
	if (!upng)
		return false;

	if (upng_get_error(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return false;
	}

	if (upng_decode(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return false;
	}

	int format, width, height, depth;
	format = upng_get_format(upng);
	width = upng_get_width(upng);
	height = upng_get_height(upng);
	depth = upng_get_bpp(upng) / 8;

	const void* buf = upng_get_buffer(upng);
	const void* pal_buf = upng_get_pal_buffer(upng);
	unsigned pal_size = upng_get_pal_size(upng);

	// todo:
	// add it to queue & call on next XPending's 'else' 
	cb(cookie, (A3D_ImageFormat)format, width, height, buf, pal_size, pal_buf);

	upng_free(upng);
	return true;
}

void _a3dSetIconData(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
{
    static Atom netWmIcon = XInternAtom(dpy,"_NET_WM_ICON",False);

	int wh = w*h;

	unsigned long* wh_buf = (unsigned long*)malloc(sizeof(unsigned long)*(2+wh));
	wh_buf[0]=w;
	wh_buf[1]=h;

	unsigned long* buf = wh_buf + 2;

	// convert to 0x[0]AARRGGBB unsigned long!!!

	switch (f)
	{
		case A3D_RGB8:
		{
			const uint8_t* src = (const uint8_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = src[3 * i + 2] | (src[3 * i + 1] << 8) | (src[3 * i + 0] << 16) | 0xFF000000;
			break;
		}
		case A3D_RGB16:
		{
			const uint16_t* src = (const uint16_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = (src[3 * i + 2] >> 8) | ((src[3 * i + 1] >> 8) << 8) | ((src[3 * i + 0] >> 8) << 16) | 0xFF000000;
			break;
		}
		case A3D_RGBA8:
		{
			const uint8_t* src = (const uint8_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = src[4 * i + 2] | (src[4 * i + 1] << 8) | (src[4 * i + 0] << 16) | (src[4 * i + 3] << 24);
			break;
		}
		case A3D_RGBA16:
		{
			const uint16_t* src = (const uint16_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = (src[4 * i + 2] >> 8) | ((src[4 * i + 1] >> 8) << 8) | ((src[4 * i + 0] >> 8) << 16) | ((src[4 * i + 3] >> 8) << 24);
			break;
		}
		case A3D_LUMINANCE1:
			break;
		case A3D_LUMINANCE2:
			break;
		case A3D_LUMINANCE4:
			break;
		case A3D_LUMINANCE8:
		{
			const uint8_t* src = (const uint8_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = src[i] | (src[i] << 8) | (src[i] << 16) | 0xFF000000;
			break;
		}
		case A3D_LUMINANCE_ALPHA1:
			break;
		case A3D_LUMINANCE_ALPHA2:
			break;
		case A3D_LUMINANCE_ALPHA4:
			break;
		case A3D_LUMINANCE_ALPHA8:
		{
			const uint8_t* src = (const uint8_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
				buf[i] = src[2 * i + 0] | (src[2 * i + 0] << 8) | (src[2 * i + 0] << 16) | (src[2 * i + 1] << 24);
			break;
		}

		case A3D_INDEX1_RGB:
		case A3D_INDEX1_RGBA:
			break;

		case A3D_INDEX2_RGB:
		case A3D_INDEX2_RGBA:
			break;

		case A3D_INDEX4_RGB:
		case A3D_INDEX4_RGBA:
			break;

		case A3D_INDEX8_RGB:
		case A3D_INDEX8_RGBA:
		{
			const uint8_t* src = (const uint8_t*)data;
			int n = w * h;
			for (int i = 0; i < n; i++)
			{
				if (src[i] >= palsize)
					buf[i] = 0;
				else
				{
					uint32_t p = ((uint32_t*)palbuf)[src[i]];
					buf[i] = (p & 0xFF00FF00) | ((p << 16) & 0x00FF0000) | ((p>>16) & 0x000000FF);
				}
			}
			break;
		}
	}

    XChangeProperty(dpy, win, netWmIcon, XA_CARDINAL, 32, PropModeReplace, 
					(const unsigned char*)wh_buf, 2 + wh);

	free(wh_buf);
}

bool a3dSetIconData(A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
{
	_a3dSetIconData(0, f, w, h, data, palsize, palbuf);
	return true;
}

bool a3dSetIcon(const char* path)
{
	return a3dLoadImage(path, 0, _a3dSetIconData);
}


#include <dirent.h>
#include <sys/stat.h>

int a3dListDir(const char* dir_path, bool (*cb)(A3D_DirItem item, const char* name, void* cookie), void* cookie)
{
    DIR* d;
    struct dirent* dir;

    d = opendir(dir_path);	
	if (!d)
		return -1;

	int num = 0;
	while ((dir = readdir(d)) != 0)
	{
		A3D_DirItem item;
		if (dir->d_type == DT_UNKNOWN)
		{
			char fullpath[4096];
			snprintf(fullpath,4095,"%s/%s",dir_path,dir->d_name);
			struct stat s;
			if (0!=lstat(fullpath,&s))
				continue;

			if (s.st_mode == S_IFDIR)
				item = A3D_DIRECTORY;
			else
			if (dir->d_type == S_IFREG)
				item = A3D_FILE;
			else
				continue;				
		}
		else
		{
			if (dir->d_type == DT_DIR)
				item = A3D_DIRECTORY;
			else
			if (dir->d_type == DT_REG)
				item = A3D_FILE;
			else
				continue;
		}
		 
		printf("%s\n", dir->d_name);
		if (cb && !cb(item, dir->d_name,cookie))
			break;
		num++;
	}

	closedir(d);
	return num;	
}
