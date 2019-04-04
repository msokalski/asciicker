
// PLATFORM: LINUX

// sudo apt install libx11-dev
// sudo apt-get install libglu1-mesa-dev
// gcc -o asciiid_x11 asciiid_x11.cpp -lGL -lX11 -L/usr/X11R6/lib -I/usr/X11R6/include

#include<stdio.h>
#include<stdlib.h>
#include<X11/X.h>
#include<X11/Xlib.h>
#include <X11/keysym.h>

#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "gl.h"

#include <GL/glx.h>
//#include <GL/glxext.h>

#include "asciiid_platform.h"

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
	"A3D_OEM_COLON",		// ';:' for US
	"A3D_OEM_PLUS",		// '+' any country
	"A3D_OEM_COMMA",		// '",' any country
	"A3D_OEM_MINUS",		// '-' any country
	"A3D_OEM_PERIOD",		// '.' any country
	"A3D_OEM_SLASH",		// '/?' for US
	"A3D_OEM_TILDE",		// '`~' for US
	"A3D_OEM_OPEN",       //  '[{' for US
	"A3D_OEM_CLOSE",      //  ']}' for US
	"A3D_OEM_BACKSLASH",  //  '\|' for US
	"A3D_OEM_QUOTATION",  //  ''"' for US
};	


int kc_to_ki[128]=
{
	0,0,0,0,0,0,0,0,
	0, // 8
	A3D_ESCAPE, // 9
	A3D_1, // 10
	A3D_2, // 11
	A3D_3, // 12
	A3D_4, // 13
	A3D_5, // 14
	A3D_6, // 15
	A3D_7, // 16
	A3D_8, // 17
	A3D_9, // 18
	A3D_0, // 19
	A3D_OEM_MINUS,
	A3D_OEM_PLUS,
	A3D_BACKSPACE,
	A3D_TAB,
	A3D_Q,
	A3D_W,
	A3D_E,
	A3D_R,
	A3D_T,
	A3D_Y,
	A3D_U,
	A3D_I,
	A3D_O,
	A3D_P,
	A3D_OEM_OPEN,
	A3D_OEM_CLOSE, // 35
	A3D_ENTER,
	A3D_LCTRL,
	A3D_A,
	A3D_S,
	A3D_D,
	A3D_F,
	A3D_G,//42
	A3D_H,
	A3D_J,
	A3D_K,
	A3D_L,
	A3D_OEM_COLON,
	A3D_OEM_QUOTATION,
	A3D_OEM_TILDE,
	A3D_LSHIFT,
	A3D_OEM_BACKSLASH, // 51
	A3D_Z,
	A3D_X,
	A3D_C,
	A3D_V,
	A3D_B,
	A3D_N,
	A3D_M,//58
	A3D_OEM_COMMA,
	A3D_OEM_PERIOD,
	A3D_OEM_SLASH,
	A3D_RSHIFT,//62
	A3D_NUMPAD_MULTIPLY,//63
	A3D_LALT,
	A3D_SPACE,
	A3D_CAPSLOCK,
	A3D_F1,//67
	A3D_F2,
	A3D_F3,
	A3D_F4,
	A3D_F5,
	A3D_F6,
	A3D_F7,
	A3D_F8,
	A3D_F9,
	A3D_F10,
	A3D_NUMLOCK,//77
	A3D_SCROLLLOCK,//78
	A3D_NUMPAD_7,//79
	A3D_NUMPAD_8,
	A3D_NUMPAD_9,
	A3D_NUMPAD_SUBTRACT,
	A3D_NUMPAD_4,
	A3D_NUMPAD_5,
	A3D_NUMPAD_6,
	A3D_NUMPAD_ADD,//86
	A3D_NUMPAD_1,
	A3D_NUMPAD_2,
	A3D_NUMPAD_3, // 89
	A3D_NUMPAD_0,
	A3D_NUMPAD_DECIMAL,
	0,//92
	0,//93
	0,//94
	A3D_F11,//95
	A3D_F12,//96
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	A3D_NUMPAD_ENTER,//104
	A3D_RCTRL,//105
	A3D_NUMPAD_DIVIDE,//106
	0,//107
	A3D_RALT,//108
	0,
	A3D_HOME,//110
	A3D_UP,
	A3D_PAGEUP,
	A3D_LEFT,
	A3D_RIGHT,
	A3D_END,
	A3D_DOWN,
	A3D_PAGEDOWN,
	A3D_INSERT,
	A3D_DELETE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	A3D_PAUSE,//127
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
	win = XCreateWindow(dpy, root, 0, 0, 800, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	if (!win)
	{
		XCloseDisplay(dpy);
		return false;
	}

	XSetWMProtocols(dpy, win, &wm_delete_window, 1);		

 	XMapWindow(dpy, win);
	mapped = true;

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
	XGetWindowAttributes(dpy, win, &gwa);
	int w = gwa.width, h = gwa.height;

	if (platform_api.resize)
		platform_api.resize(w,h);

	//int key_seq = 0;
	//printf("%s:\n",caps[key_seq]);

	while (!closing)
	{
		XGetWindowAttributes(dpy, win, &gwa);
		if (gwa.width!=w || gwa.height!=h)
		{
			w = gwa.width;
			h = gwa.height;			
			if (platform_api.resize)
				platform_api.resize(w,h);			
		}

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

			if (xev.type == Expose) 
			{
				if (platform_api.render)
					platform_api.render();
			}
			else 
			if(xev.type == EnterNotify) 
			{
				// xev.xcrossing.state can contain:
				// Button1Mask, Button2Mask, Button3Mask

				// todo:
				// add it to mi state :)
				
				if (platform_api.mouse)
					platform_api.mouse(xev.xcrossing.x,xev.xcrossing.y,MouseInfo::ENTER);
			}
			else 
			if(xev.type == LeaveNotify) 
			{
				// xev.xcrossing.state can contain:
				// Button1Mask, Button2Mask, Button3Mask

				// todo:
				// add it to mi state :)				

				if (platform_api.mouse)
					platform_api.mouse(xev.xcrossing.x,xev.xcrossing.y,MouseInfo::LEAVE);
			}			
			else 
			if(xev.type == KeyPress) 
			{
				/*
				if (key_seq>=0)
				{
					printf("(seq%d)\t%d\t0x%04x\n",key_seq,(int)xev.xkey.keycode,(int)xev.xkey.keycode);
					key_seq++;
					if (caps[key_seq])
						printf("%s:\n",caps[key_seq]);
					else
						key_seq = -1;
				}
				*/
				if (platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						printf("PRESS: %s\n",caps[kc_to_ki[kc]]);
						platform_api.keyb_key((KeyInfo)kc_to_ki[kc],true);
					}
				}
			}
			else 
			if(xev.type == KeyRelease) 
			{
				if (platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						printf("RELEASE: %s\n",caps[kc_to_ki[kc]]);
						platform_api.keyb_key((KeyInfo)kc_to_ki[kc],false);
					}			
				}
			}
			else
			if (xev.type == ButtonPress)
			{
				MouseInfo mi = (MouseInfo)0;
				switch (xev.xbutton.button)
				{
					case Button1: mi = MouseInfo::LEFT_DN; break;
					case Button3: mi = MouseInfo::RIGHT_DN; break;
					case Button2: mi = MouseInfo::MIDDLE_DN; break;
					case Button5: mi = MouseInfo::WHEEL_DN; break;
					case Button4: mi = MouseInfo::WHEEL_UP; break;
				}

				// xev.xbutton.state can contain:
				// Button1Mask, Button2Mask, Button3Mask

				// todo:
				// add it to mi state :)

				if (platform_api.mouse)
					platform_api.mouse(xev.xbutton.x,xev.xbutton.y,mi);				
			}
			else
			if (xev.type == ButtonRelease)
			{
				MouseInfo mi = (MouseInfo)0;
				switch (xev.xbutton.button)
				{
					case Button1: mi = MouseInfo::LEFT_UP; break;
					case Button3: mi = MouseInfo::RIGHT_UP; break;
					case Button2: mi = MouseInfo::MIDDLE_UP; break;
				}

				// xev.xbutton.state can contain:
				// Button1Mask, Button2Mask, Button3Mask

				// todo:
				// add it to mi state :)

				if (platform_api.mouse)
					platform_api.mouse(xev.xbutton.x,xev.xbutton.y,mi);
			}
			else
			if (xev.type == MotionNotify)
			{
				// xev.xmotion.state can contain:
				// Button1Mask, Button2Mask, Button3Mask

				// todo:
				// add it to mi state :)

				if (platform_api.mouse)
					platform_api.mouse(xev.xmotion.x,xev.xmotion.y,MouseInfo::MOVE);
			}
		}
		else
		{
			if (platform_api.render)
				platform_api.render();
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
	snprintf(title,255,"%ls", name);
	XStoreName(dpy, win, title);
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
	return true;
}

void a3dSetRect(const int* xywh, bool wnd_mode)
{
	XMoveResizeWindow(dpy,win, xywh[0], xywh[1], xywh[2], xywh[3]);
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
	Window focused;
	int revert_to;
	XGetInputFocus(dpy, &focused, &revert_to);
	return focused == win;
}
