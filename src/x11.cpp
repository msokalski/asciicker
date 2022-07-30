#if defined(__linux__) || defined(__APPLE__)
#ifndef USE_SDL

#include <stdio.h>
#include <stdlib.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h> // compile with -pthread
#include <pty.h> // link  with -lutil

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>

// better than Xinerama but incompatible 
// with _NET_WM_FULLSCREEN_MONITORS
//#include <X11/extensions/Xrandr.h>

#include <dirent.h>
#include <sys/stat.h>

#include <time.h>
#include <unistd.h> // usleep

#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "rgba8.h"
#include "gl.h"

#include <GL/glx.h>
#include <GL/glxext.h>

#include "platform.h"

/*
Bool __glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx, int line)
{
	printf("%d\n",line);
	return glXMakeCurrent(dpy, drawable, ctx);
}

#define glXMakeCurrent(_dpy, _dr, _rc) __glXMakeCurrent(_dpy, _dr, _rc, __LINE__)
*/


Display* dpy=0;
A3D_WND* wnd_head=0;
A3D_WND* wnd_tail=0;

struct A3D_WND
{
	A3D_WND* prev;
	A3D_WND* next;

	XIM im;
	XIC ic;
	Window win;
	GLXContext rc;
	void* cookie;

	bool mapped;
	WndMode wndmode;
	int wndrect[4]; 
	bool wnddirty;

	PlatformInterface platform_api;
	int mouse_b;
	int mouse_x;
	int mouse_y;
	bool track;
	bool closing;
	int force_key;

	int gwa_width;
	int gwa_height;
};

A3D_PTY* head_pty = 0;
A3D_PTY* tail_pty = 0;

struct A3D_PTY
{
	int fd;
	int pd[2];

	pid_t pid;
	A3D_PTY* next;
	A3D_PTY* prev;
	A3D_VT* vt;
};

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

void a3dPushContext(A3D_PUSH_CONTEXT* ctx)
{
	ctx->data[0] = dpy;
	ctx->data[1] = (void*)glXGetCurrentDrawable();
	ctx->data[2] = glXGetCurrentContext();
}

void a3dPopContext(const A3D_PUSH_CONTEXT* ctx)
{
	if (dpy)
		glXMakeCurrent((Display*)ctx->data[0], (GLXDrawable)ctx->data[1], (GLXContext)ctx->data[2]);
}

void a3dSwitchContext(const A3D_WND* wnd)
{
	if (dpy)
		glXMakeCurrent(dpy, wnd->win, wnd->rc);
}

// creates window & initialized GL
A3D_WND* a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd, A3D_WND* share)
{
	struct PUSH
	{
		PUSH()
		{
			dr = 0;
			rc = 0;
			if (dpy)
			{
				pop=true;
				rc = glXGetCurrentContext();
				dr = glXGetCurrentDrawable();
			}
			else
				pop=false;
		}

		~PUSH()
		{
			if (dpy && pop)
				glXMakeCurrent(dpy, dr, rc);
		}

		bool pop;
		GLXDrawable dr;
		GLXContext rc;

	} push;

	if (!pi || !gd)
		return 0;

	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
		glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
	
	if (!glXCreateContextAttribsARB)
		return 0;

	int auto_bits = (gd->color_bits - gd->alpha_bits) / 3;
	int color_bits[4] = {auto_bits,auto_bits,auto_bits,gd->alpha_bits};
	switch (gd->color_bits)
	{
		case 32:
			color_bits[0] = 8;
			color_bits[1] = 8;
			color_bits[2] = 8;
			color_bits[3] = gd->alpha_bits ? 8 : 0;
			break;

		case 16:
			color_bits[0] = 5;
			color_bits[1] = gd->alpha_bits ? 5 : 6;
			color_bits[2] = 5;
			color_bits[3] = gd->alpha_bits ? 1 : 0;
			break;
	}

	// Get a matching FB config
	static int visual_attribs[] =
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , color_bits[0],
		GLX_GREEN_SIZE      , color_bits[1],
		GLX_BLUE_SIZE       , color_bits[2],
		GLX_ALPHA_SIZE      , color_bits[3],
		GLX_DEPTH_SIZE      , gd->depth_bits,
		GLX_STENCIL_SIZE    , gd->stencil_bits,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};

	GLXContext              glc;
	XWindowAttributes       gwa;
	XEvent                  xev;

	int wndrect[4];

	XIM im = 0;
	bool im_ok = false;
	if (XSupportsLocale())
	{
		if (XSetLocaleModifiers("@im=none"))
		{
			im_ok = true;
		}
	}	

	// dpy is global
	if (!dpy)
	{
		dpy = XOpenDisplay(NULL);
		if (!dpy)
			return 0;
	}

	Bool DetectableAutoRepeat = false;
	XkbSetDetectableAutoRepeat(dpy, True, &DetectableAutoRepeat);

	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	Window root = DefaultRootWindow(dpy);
	if (!root)
	{
		XCloseDisplay(dpy);
		return 0;
	}

	#if 0
		XVisualInfo* vi = glXChooseVisual(dpy, 0, att);
		if (!vi)
		{
			XCloseDisplay(dpy);
			return 0;
		}
	#else
		int fbcount;
		GLXFBConfig* fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attribs, &fbcount);
		if (!fbc)
		{
			printf( "Failed to retrieve a framebuffer config\n" );
			exit(1);
		}

		// Pick the FB config/visual with the most samples per pixel
		int best_fbc = -1, smallest_err = 0;

		int i;
		for (i=0; i<fbcount; ++i)
		{
			XVisualInfo *vi = glXGetVisualFromFBConfig( dpy, fbc[i] );
			if ( vi )
			{
				int samp_buf, samples;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_SAMPLES       , &samples  );

				int rgba_size[4];
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_RED_SIZE , rgba_size+0 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_GREEN_SIZE , rgba_size+1 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_BLUE_SIZE , rgba_size+2 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_ALPHA_SIZE , rgba_size+3 );

				int depth, stencil;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_DEPTH_SIZE , &depth );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_STENCIL_SIZE , &stencil );

				int accum_size[4], aux;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_ACCUM_RED_SIZE , accum_size+0 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_ACCUM_GREEN_SIZE , accum_size+1 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_ACCUM_BLUE_SIZE , accum_size+2 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_ACCUM_ALPHA_SIZE , accum_size+3 );
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_AUX_BUFFERS , &aux );

				int draw_type;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_DRAWABLE_TYPE , &draw_type );

				int stereo;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_STEREO , &stereo );

				int vis_type;
				glXGetFBConfigAttrib( dpy, fbc[i], GLX_X_VISUAL_TYPE, &vis_type);

				int err_smp = samp_buf * samples;
				int err_rgb = 
					abs(rgba_size[0] - color_bits[0]) +
					abs(rgba_size[1] - color_bits[1]) +
					abs(rgba_size[2] - color_bits[2]) +
					abs(rgba_size[3] - color_bits[3]);
				int err_dps = 
					abs(depth - gd->depth_bits) +
					abs(stencil - gd->stencil_bits);

				int err = err_smp + err_rgb + err_dps;					

				int err_acc = 
					accum_size[0] +
					accum_size[1] +
					accum_size[2] +
					accum_size[3];	

				err += aux + err * aux + err_acc;

				err += vis_type != GLX_TRUE_COLOR;
				err += 2 * (draw_type != GLX_WINDOW_BIT);
				err += 4 * stereo;
				
				if ( best_fbc < 0 || err < smallest_err )
				{
					best_fbc = i;
					smallest_err = err;
				}
			}
			XFree( vi );
		}

		// GLXFBConfig bestFbc = fbc[ best_fbc ];
		GLXFBConfig bestFbc = fbc[ best_fbc ];

		// Be sure to free the FBConfig list allocated by glXChooseFBConfig()
		XFree( fbc );

		// Get a visual
		XVisualInfo *vi = glXGetVisualFromFBConfig( dpy, bestFbc );
		// printf( "Chosen visual ID = 0x%x\n", (unsigned)vi->visualid );
	#endif

	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

	if (!cmap)
	{

		XCloseDisplay(dpy);
		return 0;
	}

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

	if (gd->wnd_xywh)
	{
		wndrect[0] = gd->wnd_xywh[0];
		wndrect[1] = gd->wnd_xywh[1];
		wndrect[2] = gd->wnd_xywh[2];
		wndrect[3] = gd->wnd_xywh[3];
	}
	else
	{
		wndrect[0] = 0;
		wndrect[1] = 0;
		wndrect[2] = 800;
		wndrect[3] = 600;
	}
	
	Window win = XCreateWindow(dpy, /*share ? share->win :*/ root, wndrect[0]+wndrect[2]/2 - 400, wndrect[1]+wndrect[3]/2 - 300, 800, 600, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

	XFreeColormap(dpy, cmap); // swa (including cmap) is no longer used

	if (!win)
	{
		XFree(vi);
		XCloseDisplay(dpy);
		return 0;
	}

	XSetWMProtocols(dpy, win, &wm_delete_window, 1);		

	char app_name[]="asciiid";
	XStoreName(dpy, win, app_name);

	// surpress 'UNKNOWN' 
	// name will be taken from 'Name' property set in ~/.local/share/application/*.desktop file 
	// if there is no such file it will use class name 'A3D'
	/*
	[Desktop Entry]
			Name=ASCIIID <--- this will be used
			Exec=/home/user/asciiid/.run/asciiid
			Path=/home/user/asciiid
			Icon=/home/user/asciiid/icons/app.png
			Type=Application
			Categories=Games;Utilities
	*/

	XClassHint* ch = XAllocClassHint();
	char cls_name[] = "A3D";
	ch->res_class=cls_name;
	ch->res_name=app_name;
	XSetClassHint(dpy, win, ch);
	XFree(ch);

	int attribs[] = {
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | (gd->flags & GraphicsDesc::DEBUG_CONTEXT ? GLX_CONTEXT_DEBUG_BIT_ARB : 0),
		GLX_CONTEXT_MAJOR_VERSION_ARB, gd->version[0],
		GLX_CONTEXT_MINOR_VERSION_ARB, gd->version[1],
		GLX_CONTEXT_PROFILE_MASK_ARB, strcmp(GALOGEN_API_PROFILE,"core") == 0 ? GLX_CONTEXT_CORE_PROFILE_BIT_ARB : 0,
		0};

	#if 0
		int nelements;
		GLXFBConfig *fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &nelements);	
		if (!fbc)
		{
			XFree(vi);
			printf("CANNOT CHOOSE FBCONFIG\n");
			return 0;
		}

		glc = glXCreateContextAttribsARB(dpy, *fbc, share ? share->rc : 0, true, attribs);
		//glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		XFree(fbc);
	#else
		glc = glXCreateContextAttribsARB(dpy, bestFbc, share ? share->rc : 0, true, attribs);
		//glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	#endif

	XFree(vi);

	if (!glc)
	{
		printf("CANNOT CREATE GL CONTEXT\n");
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		return 0;
	}


 	if (!glXMakeCurrent(dpy, win, glc))
	{
		printf("CANNOT MAKE GL CONTEXT CURRENT\n");
		glXDestroyContext(dpy, glc);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		return 0;
	}

	char* ver = (char*)glGetString(GL_VERSION);
	if (ver[0] < gd->version[0]+'0' || ver[0] > '9' || ver[1] != '.' || ver[0] == gd->version[0] + '0' && (ver[2] < gd->version[1] + '0' || ver[2] > '9'))
	{
		printf("GL_VERSION (%d.%d.x) requirement is not met by %s\n", gd->version[0], gd->version[1], ver);

		glXMakeCurrent(dpy, 0, 0);
		glXDestroyContext(dpy, glc);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		return 0;
	}

	// try to connect to IM, if anything fails here
	// we'd simply stick ascii codes 

	XIC ic = 0;
	if (im_ok)
	{
		im = XOpenIM(dpy, NULL, NULL, NULL);
		if (im) 
		{
			char *failed_arg = 0;
			XIMStyles *styles = 0;
			failed_arg = XGetIMValues(im, XNQueryInputStyle, &styles, NULL);
			if (styles)
				XFree(styles);
			if (!failed_arg)
			{
				ic = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, win, NULL);
				if (!ic)
				{
					XCloseIM(im);
					im = 0;
					im_ok = false;
				}
				else
				{
					XSetICFocus(ic);
				}
			}
			else
			{
				XFree(failed_arg);
				XCloseIM(im);
				im = 0;
				im_ok = false;
			}
		}
	}

 	/*
	// HAS NO EFFECT, only going fullscreen on all monitors at once results in FLIP mode
	// anything else uses BLIT mode even with this hint enabled :(
	Atom bypass_wm_atom = XInternAtom(dpy,"_NET_WM_BYPASS_COMPOSITOR",False);

	if (bypass_wm_atom)
	{
		static const unsigned long bypassCompositor = 1;

		XChangeProperty(dpy,
						win,
						bypass_wm_atom,
						XA_CARDINAL,
						32,
						PropModeReplace,
						(const unsigned char*)&bypassCompositor,
						1);
	}
	*/

	A3D_WND* wnd = (A3D_WND*)malloc(sizeof(A3D_WND));

	// init wnd fields!
	wnd->platform_api = *pi;
	wnd->cookie = 0;
	
	wnd->win = win;
	wnd->rc = glc;
	wnd->ic = ic;
	wnd->im = 0;//im;

	wnd->mapped = false;
	wnd->wndmode = gd->wnd_mode == A3D_WND_CURRENT ? A3D_WND_NORMAL : gd->wnd_mode;
	wnd->wndrect[0] = wndrect[0]; 
	wnd->wndrect[1] = wndrect[1]; 
	wnd->wndrect[2] = wndrect[2]; 
	wnd->wndrect[3] = wndrect[3]; 
	wnd->wnddirty = true;
	wnd->mouse_b = 0;
	wnd->mouse_x = 0;
	wnd->mouse_y = 0;
	wnd->track = false;
	wnd->force_key = A3D_NONE;

	wnd->prev = wnd_tail;
	wnd->next = 0;
	if (wnd_tail)
		wnd_tail->next = wnd;
	else
		wnd_head = wnd;
	wnd_tail = wnd;

	if (wnd->platform_api.init)
		wnd->platform_api.init(wnd);

	XSync(dpy,False);

	// send initial notifications:
	XGetWindowAttributes(dpy, win, &gwa);
	wnd->gwa_width = gwa.width;
	wnd->gwa_height = gwa.height;

	if (wnd->platform_api.resize)
		wnd->platform_api.resize(wnd,wnd->gwa_width,wnd->gwa_height);
	
	return wnd;
}

void a3dLoop()
{
	XSync(dpy,False);

	// for all created wnds, force size notifications
	A3D_WND* wnd = wnd_head;
	while (wnd)
	{
		if (wnd->platform_api.resize)
			wnd->platform_api.resize(wnd, wnd->gwa_width, wnd->gwa_height);
		wnd = wnd->next;
	}


	Window win;
	XEvent                  xev;

	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

	Bool DetectableAutoRepeat = false;
	XkbSetDetectableAutoRepeat(dpy, True, &DetectableAutoRepeat);

	PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)
		glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");

	while (wnd_head)
	{
		while (XPending(dpy))
		{
			XNextEvent(dpy, &xev);

			win = xev.xany.window;
			if (XFilterEvent(&xev, win))
				printf("%s","XFilter CONSUMED!\n");

			// find wnd
			A3D_WND* wnd = wnd_head;
			while (wnd)
			{
				if (wnd->win == win)
					break;
				wnd=wnd->next;
			}

			if (!wnd)
				continue;

			glXMakeCurrent(dpy, win, wnd->rc);

			if (xev.type == ClientMessage)
			{
				if ((Atom)xev.xclient.data.l[0] == wm_delete_window) 
				{
					if (wnd->platform_api.close)
						wnd->platform_api.close(wnd);
					else
					{
						if (wnd->ic)
							XDestroyIC(wnd->ic);
						if (wnd->im)
							XCloseIM(wnd->im);
						glXMakeCurrent(dpy, None, NULL);
						glXDestroyContext(dpy, wnd->rc);
						XDestroyWindow(dpy,win);

						if (wnd->prev)
							wnd->prev->next = wnd->next;
						else
							wnd_head = wnd->next;

						if (wnd->next)
							wnd->next->prev = wnd->prev;
						else
							wnd_tail = wnd->prev;

						free(wnd);
						break;
					}
				}
			}
			else
        	if (xev.type == MappingNotify)
			{
            	XRefreshKeyboardMapping(&xev.xmapping);
			}
            else
			if (xev.type == ConfigureNotify)
			{
				if (xev.xconfigure.width != wnd->gwa_width || xev.xconfigure.height != wnd->gwa_height)
				{
					wnd->gwa_width = xev.xconfigure.width;
					wnd->gwa_height = xev.xconfigure.height;
					if (wnd->platform_api.resize)
						wnd->platform_api.resize(wnd, wnd->gwa_width, wnd->gwa_height);
				}
			}
			else
			if (xev.type == FocusIn)
			{
				if (wnd->platform_api.keyb_focus)
					wnd->platform_api.keyb_focus(wnd,true);
				if (wnd->ic)
					XSetICFocus(wnd->ic);					
			}
			else
			if (xev.type == FocusOut)
			{
				if (wnd->platform_api.keyb_focus)
					wnd->platform_api.keyb_focus(wnd,false);
				if (wnd->ic)
					XUnsetICFocus(wnd->ic);					
			}
			else
			if (xev.type == Expose) 
			{
				/*
				if (wnd->platform_api.render && mapped)
					wnd->platform_api.render(wnd);
				*/
			}
			else 
			if(xev.type == EnterNotify) 
			{
				int state = 0;
				state |= xev.xcrossing.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xcrossing.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xcrossing.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				if (wnd->platform_api.mouse)
					wnd->platform_api.mouse(wnd,xev.xcrossing.x,xev.xcrossing.y,(MouseInfo)(MouseInfo::ENTER | state));
			}
			else 
			if(xev.type == LeaveNotify) 
			{
				int state = 0;
				state |= xev.xcrossing.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xcrossing.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xcrossing.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				if (wnd->platform_api.mouse)
					wnd->platform_api.mouse(wnd,xev.xcrossing.x,xev.xcrossing.y, (MouseInfo)(MouseInfo::LEAVE | state));
			}			
			else 
			if(xev.type == KeyPress) 
			{
				if (wnd->platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						wnd->force_key = kc_to_ki[kc];
						wnd->platform_api.keyb_key(wnd,(KeyInfo)kc_to_ki[kc],true);
						wnd->force_key = A3D_NONE;
					}
				}

				if (wnd->ic)
				{
					int count = 0;
					KeySym keysym = 0;
					char buf[20];
					Status status = 0;
					count = Xutf8LookupString(wnd->ic, (XKeyPressedEvent*)&xev, buf, 20, &keysym, &status);

					if (count)
					{
						if (wnd->platform_api.keyb_char)
						{
							uint8_t* c = (uint8_t*)buf;
							for (int i=0; i<count;)
							{
								uint8_t* c = (uint8_t*)buf + i;
								wchar_t w = 0;

								if (*c==0)
									break;
								else
								if (*c<0x80)
								{
									// 7bits
									w = c[0]&0x7F;
									i+=1;
								}
								else
								if ( (*c & 0xF8) == 0xF0 )
								{
									// 21 bits
									w = c[0]&0x7;
									w = (w << 6) | (c[1] & 0x3F);
									w = (w << 6) | (c[2] & 0x3F);
									w = (w << 6) | (c[3] & 0x3F);
									i+=4;
								}
								else
								if ( (*c & 0xF0) == 0xE0 )
								{
									w = c[0]&0xF;
									w = (w << 6) | (c[1] & 0x3F);
									w = (w << 6) | (c[2] & 0x3F);
									i+=3;
								}
								else
								if ( (*c & 0xE0) == 0xC0 )
								{
									w = c[0]&0x1F;
									w = (w << 6) | (c[1] & 0x3F);
									i+=2;
								}

								wnd->platform_api.keyb_char(wnd,w);
							}
						}
					}
				}
				else
				{
					XComposeStatus composeStatus;
					char asciiCode[ 32 ];
					KeySym keySym;
					int len;

					len = XLookupString( &xev.xkey, asciiCode, sizeof(asciiCode),
										&keySym, &composeStatus);

					if (wnd->platform_api.keyb_char)
						for (int i=0; i<len; i++)
							wnd->platform_api.keyb_char(wnd,(wchar_t)asciiCode[i]);
				}
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

				if (physical && wnd->platform_api.keyb_key)
				{
					int kc = xev.xkey.keycode;
					if (kc>=0 && kc<128) 
					{
						wnd->platform_api.keyb_key(wnd,(KeyInfo)kc_to_ki[kc],false);
					}			
				}

				/*
                int count = 0;
                KeySym keysym = 0;
                char buf[20];
                Status status = 0;
                count = XLookupString((XKeyEvent*)&xev, buf, 20, &keysym, NULL);

                if (count)
                    printf("in release buffer: %.*s\n", count, buf);

                printf("released KEY: %d\n", (int)keysym);				
				*/
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

				if (wnd->platform_api.mouse)
					wnd->platform_api.mouse(wnd,xev.xbutton.x,xev.xbutton.y,mi);				
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

				if (wnd->platform_api.mouse)
					wnd->platform_api.mouse(wnd,xev.xbutton.x,xev.xbutton.y,mi);
			}
			else
			if (xev.type == MotionNotify)
			{
				int state = 0;
				state |= xev.xmotion.state & Button1Mask ? MouseInfo::LEFT : 0;
				state |= xev.xmotion.state & Button3Mask ? MouseInfo::RIGHT : 0;
				state |= xev.xmotion.state & Button2Mask ? MouseInfo::MIDDLE : 0;

				MouseInfo mi = (MouseInfo)(MouseInfo::MOVE | state);

				if (wnd->platform_api.mouse)
					wnd->platform_api.mouse(wnd,xev.xmotion.x,xev.xmotion.y,mi);
			}
		}

		A3D_WND* wnd = wnd_head;
		Window swap = 0;
		Window focused = 0;

		int revert_to;
		XGetInputFocus(dpy, &focused, &revert_to);

		while (wnd)
		{
			if (wnd->platform_api.render && wnd->mapped)
			{
				if (swap)
				{
					if (glXSwapIntervalEXT)
						glXSwapIntervalEXT(dpy, swap, 0);
					glXSwapBuffers(dpy, swap);
				}

				glXMakeCurrent(dpy, wnd->win, wnd->rc);
				wnd->platform_api.render(wnd);
				swap = wnd->win;
			}

			wnd = wnd->next;
		}

		if (swap)
		{
			if (glXSwapIntervalEXT)
				glXSwapIntervalEXT(dpy, swap, 1);
			glXSwapBuffers(dpy, swap);
		}
	}

	XCloseDisplay(dpy);
	dpy = 0;
}

void a3dClose(A3D_WND* wnd)
{
	XSync(dpy,False);

	struct PUSH
	{
		PUSH()
		{
			dr = 0;
			rc = 0;
			if (dpy)
			{
				rc = glXGetCurrentContext();
				dr = glXGetCurrentDrawable();
			}
		}

		~PUSH()
		{
			if (dpy)
				glXMakeCurrent(dpy, dr, rc);
		}

		GLXDrawable dr;
		GLXContext rc;

	} push;


	if (push.rc == wnd->rc)	
		push.rc = 0;
	if (push.dr == wnd->win)
		push.dr = 0;

	if (wnd->ic)
		XDestroyIC(wnd->ic);
	if (wnd->im)
		XCloseIM(wnd->im);

	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, wnd->rc);
	XDestroyWindow(dpy,wnd->win);

	if (wnd->prev)
		wnd->prev->next = wnd->next;
	else
		wnd_head = wnd->next;

	if (wnd->next)
		wnd->next->prev = wnd->prev;
	else
		wnd_tail = wnd->prev;

	free(wnd);

	XSync(dpy,False);
}

uint64_t a3dGetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/*
void a3dSwapBuffers()
{
	glXSwapBuffers(dpy, win);
}
*/

bool a3dGetKeyb(A3D_WND* wnd, KeyInfo ki)
{
	/*
	unsigned int n;
	XkbGetIndicatorState(dpy, XkbUseCoreKbd, &n);
	*/

	if (ki <= 0 || ki >= A3D_MAPEND)
		return false;

	if (ki == wnd->force_key)
		return true;

	int kc = ki_to_kc[ki];
	if (!kc)
		return false;

	XSync(dpy,False);

	char bits[32]={0};
	XQueryKeymap(dpy, bits);

	return ( bits[kc >> 3] & (1 << (kc & 7)) ) != 0;
}

void a3dSetTitle(A3D_WND* wnd, const char* name)
{
	int rc;
	size_t len = strlen(name);

	Atom wm_name_atom, wm_icon_name_atom, utf8_string_atom;

	wm_name_atom = XInternAtom(dpy, "_NET_WM_NAME", False);
	wm_icon_name_atom = XInternAtom(dpy, "_NET_WM_ICON_NAME", False);
	utf8_string_atom = XInternAtom(dpy, "UTF8_STRING", False);

	rc = XChangeProperty(dpy, wnd->win, wm_name_atom, utf8_string_atom, 8, PropModeReplace, (unsigned char*)name, len);
	rc = XChangeProperty(dpy, wnd->win, wm_icon_name_atom, utf8_string_atom, 8, PropModeReplace, (unsigned char*)name, len);

	XSync(dpy,False);
}

int a3dGetTitle(A3D_WND* wnd, char* utf8_name, int size)
{
	XSync(dpy,False);

	Atom wm_name_atom = XInternAtom(dpy,"_NET_WM_NAME", False);
	Atom utf8_string_atom = XInternAtom(dpy,"UTF8_STRING", False);
	Atom type;
	int format;
	unsigned long nitems, after;
	char* data = 0;

	if (Success == XGetWindowProperty(dpy, wnd->win, wm_name_atom, 0, 65536, false, utf8_string_atom, &type, &format, &nitems, &after, (unsigned char**)&data)) 
	{
		if (data) 
		{
			size_t len = strlen(data);
			len = len < size-1 ? len : size-1;
			memcpy(utf8_name, data, len+1);
			utf8_name[len] = 0;
			XFree(data);
			return len;
		}
	}

	return 0;
}

void a3dSetVisible(A3D_WND* wnd, bool visible)
{
	wnd->mapped = visible;
	if (visible)
	{
		XMapWindow(dpy, wnd->win);
		if (wnd->wnddirty)
			a3dSetRect(wnd,wnd->wndrect,wnd->wndmode);
	}
	else
	{
		XUnmapWindow(dpy, wnd->win);
	}

	XSync(dpy,False);
}

bool a3dGetVisible(A3D_WND* wnd)
{
	XSync(dpy,False);
	return wnd->mapped;
}

bool a3dIsMaximized(A3D_WND* wnd)
{
	return false;
}

// resize
WndMode a3dGetRect(A3D_WND* wnd, int* xywh, int* client_wh)
{
	XSync(dpy,False);
	if (xywh || client_wh)
	{
		int lrtb[4] = {0,0,0,0};
		// deduce offsetting
		if (wnd->wndmode == A3D_WND_NORMAL && xywh)
		{
			long* extents;
			Atom actual_type;
			int actual_format;
			unsigned long nitems, bytes_after;
			unsigned char* data = 0;
			int result;

			Atom frame_extends_atom = XInternAtom(dpy, "_NET_FRAME_EXTENTS", False);

			if (frame_extends_atom)
			{
				result = XGetWindowProperty(
					dpy, wnd->win, frame_extends_atom,
					0, 4, False, AnyPropertyType, 
					&actual_type, &actual_format, 
					&nitems, &bytes_after, &data);

				if (result == Success) 
				{
					if ((nitems == 4) && (bytes_after == 0)) 
					{
						extents = (long *)data;
						lrtb[0] = (int)extents[0];
						lrtb[1] = (int)extents[1];
						lrtb[2] = (int)extents[2];
						lrtb[3] = (int)extents[3];
					}
					XFree(data);			
				}
			}
		}

		Window root;
		
		int x,y;
		unsigned int w,h,b,d;
		XGetGeometry(dpy,wnd->win,&root,&x,&y,&w,&h,&b,&d);

		if (client_wh)
		{
			client_wh[0] = w;
			client_wh[1] = h;
		}

		if (xywh)
		{
			int rx,ry;
			Window c;
			XTranslateCoordinates(dpy,wnd->win,root,0,0,&rx,&ry,&c);

			// if _MOTIF_WM_HINTS and/or _NET_FRAME_EXTENTS are unsupported
			// this may lead to unconsistent rect setter with getter !!!

			xywh[0] = rx - lrtb[0];
			xywh[1] = ry - lrtb[2];
			xywh[2] = lrtb[0] + w + lrtb[1];
			xywh[3] = lrtb[2] + h + lrtb[3];
		}
	}
	return wnd->wndmode;
}

bool a3dSetRect(A3D_WND* wnd, const int* xywh, WndMode wnd_mode)
{
	if (wnd_mode == A3D_WND_CURRENT)
		wnd_mode = wnd->wndmode;

	if (!wnd->mapped)
	{
		// emulate success, 
		// even on unmapped windows

		wnd->wndmode = wnd_mode;
		wnd->wnddirty = true;
		if (!xywh)
			a3dGetRect(wnd,wnd->wndrect, 0);
		else
		{
			wnd->wndrect[0] = xywh[0];
			wnd->wndrect[1] = xywh[1];
			wnd->wndrect[2] = xywh[2];
			wnd->wndrect[3] = xywh[3];
		}
		
		return true;
	}

	// xywh=rect wnd_mode=true -> [exit from full screen] then resize
	// xywh=NULL wnd_mode=true -> [exit from full screen]
	// xywh=rect wnd_mode=false -> enter full screen on monitors crossing given rect
	// xywh=NULL wnd_mode=false -> enter full screen on signle nearest monitor

	if (wnd_mode == A3D_WND_FULLSCREEN)
	{
		int wnd_xywh[4];
		if (!xywh)
		{
			a3dGetRect(wnd,wnd_xywh, 0);
			xywh = wnd_xywh;
		}

		// - locate monitor which is covered with greatest area by win

		int num;
		XineramaScreenInfo* xi = XineramaQueryScreens(dpy, &num);

		int left_mon = -1;
		int right_mon = -1;
		int bottom_mon = -1;
		int top_mon = -1;

		for (int i=0; i<num; i++)
		{
			int a,b;

			a = xywh[0]+xywh[2]; b = xi[i].x_org + xi[i].width;
			int min_x = a < b ? a : b;
			a =  xywh[0]; b = xi[i].x_org;
			int max_x = a > b ? a : b;
			a = xywh[1]+xywh[3]; b = xi[i].y_org + xi[i].height;
			int min_y = a < b ? a : b;
			a = xywh[1]; b = xi[i].y_org;
			int max_y = a > b ? a : b;

			if (max_x < min_x && max_y < min_y)
			{
				if (left_mon < 0 || xi[i].x_org < xi[left_mon].x_org)
					left_mon = i;

				if (right_mon < 0 || xi[i].x_org + xi[i].width > xi[right_mon].x_org + xi[right_mon].width)
					right_mon = i;

				if (top_mon < 0 || xi[i].y_org < xi[top_mon].y_org)
					top_mon = i;

				if (bottom_mon < 0 || xi[i].y_org + xi[i].height > xi[bottom_mon].y_org + xi[bottom_mon].height)
					bottom_mon = i;
			}
		}

		if (left_mon < 0)
			return false;

		XClientMessageEvent xcm;
		xcm.type = ClientMessage;
		xcm.serial = 0;	/* # of last request processed by server */
		xcm.send_event=False;
		xcm.display=dpy;
		xcm.window=wnd->win;
		xcm.message_type = XInternAtom(dpy, "_NET_WM_FULLSCREEN_MONITORS", False);
		xcm.format = 32;

		xcm.data.l[0] = top_mon; /* topmost*/
		xcm.data.l[1] = bottom_mon; /* bottommost */
		xcm.data.l[2] = left_mon; /* leftmost */
		xcm.data.l[3] = right_mon; /* rightmost */
		xcm.data.l[4] = 1; /* source indication */

		XFree(xi);

		XSendEvent(dpy,DefaultRootWindow(dpy),False,SubstructureRedirectMask | SubstructureNotifyMask,(XEvent*)&xcm);
	}
	else
	if ( wnd_mode == A3D_WND_NORMAL ||  wnd_mode == A3D_WND_FRAMELESS)
	{
		struct MotifHints
		{
			unsigned long   flags;
			unsigned long   functions;
			unsigned long   decorations;
			long            inputMode;
			unsigned long   status;
		} hints;

		//code to remove decoration
		hints.flags = 2;
		hints.functions = 0;
		hints.decorations = wnd_mode == A3D_WND_NORMAL;
		hints.inputMode = 0;
		hints.status = 0;
		Atom motif_hints_atom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
		XChangeProperty(dpy,wnd->win,motif_hints_atom,motif_hints_atom,32,PropModeReplace,(unsigned char *)&hints,5);
	}

	wnd->wndmode = wnd_mode;

	if (wnd_mode == A3D_WND_FULLSCREEN)
	{
		XClientMessageEvent xcm;

		xcm.type = ClientMessage;
		xcm.serial = 0;	/* # of last request processed by server */
		xcm.send_event=False;
		xcm.display=dpy;
		xcm.window=wnd->win;
		xcm.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
		xcm.format=32;
		xcm.data.l[0]=1;//XInternAtom(dpy, "_NET_WM_STATE_ADD", False);
		xcm.data.l[1]=XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		xcm.data.l[2]=0; // no second property
		xcm.data.l[3]=1; // source indication (1=normal)
		xcm.data.l[4]=0; // 0 tail fill

		// don't resize window when entering fullscreen!
		// XMoveResizeWindow(dpy,win, xywh[0], xywh[1], xywh[2], xywh[3]);

		XSendEvent(dpy,DefaultRootWindow(dpy),False,SubstructureRedirectMask | SubstructureNotifyMask,(XEvent*)&xcm);
	}
	else
	{
		XClientMessageEvent xcm;

		xcm.type = ClientMessage;
		xcm.serial = 0;	/* # of last request processed by server */
		xcm.send_event=False;
		xcm.display=dpy;
		xcm.window=wnd->win;
		xcm.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
		xcm.format=32;
		xcm.data.l[0]=0;//XInternAtom(dpy, "_NET_WM_STATE_REMOVE", False);
		xcm.data.l[1]=XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		xcm.data.l[2]=0; // no second property
		xcm.data.l[3]=1; // source indication (1=normal)
		xcm.data.l[4]=0; // 0 tail fill

		XSendEvent(dpy,DefaultRootWindow(dpy),False,SubstructureRedirectMask | SubstructureNotifyMask,(XEvent*)&xcm);

		if (xywh)
		{
			// resize if requested, after exiting fullscreen!
			if (wnd_mode==A3D_WND_NORMAL)
			{
				int lrtb[4]={0,0,0,0};

				long* extents;
				Atom actual_type;
				int actual_format;
				unsigned long nitems, bytes_after;
				unsigned char* data = 0;
				int result;

				Atom frame_extends_atom = XInternAtom(dpy, "_NET_FRAME_EXTENTS", False);

				if (frame_extends_atom)
				{
					result = XGetWindowProperty(
						dpy, wnd->win, frame_extends_atom,
						0, 4, False, AnyPropertyType, 
						&actual_type, &actual_format, 
						&nitems, &bytes_after, &data);

					if (result == Success) 
					{
						if ((nitems == 4) && (bytes_after == 0)) 
						{
							extents = (long *)data;
							lrtb[0] = (int)extents[0];
							lrtb[1] = (int)extents[1];
							lrtb[2] = (int)extents[2];
							lrtb[3] = (int)extents[3];
						}
						XFree(data);			
					}
				}

				XMoveResizeWindow(dpy,wnd->win, xywh[0], xywh[1], xywh[2] - lrtb[0] - lrtb[1], xywh[3] - lrtb[2] - lrtb[3]);
			}
			else
				XMoveResizeWindow(dpy,wnd->win, xywh[0], xywh[1], xywh[2], xywh[3]);
		}
	}

	XSync(dpy,False);
	return true;
}

// mouse
MouseInfo a3dGetMouse(A3D_WND* wnd, int* x, int* y) // returns but flags, mouse wheel has no state
{
	// TODO
	Window root;
	Window child;
	int root_x, root_y;
	int win_x, win_y;
	unsigned int mask;

	if (XQueryPointer(dpy, wnd->win, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask))
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

void a3dSetCookie(A3D_WND* wnd, void* cookie)
{
	wnd->cookie = cookie;
}

void* a3dGetCookie(A3D_WND* wnd)
{
	return wnd->cookie;
}

void a3dSetFocus(A3D_WND* wnd)
{
	XSetInputFocus(dpy, wnd->win, RevertToNone, CurrentTime);
}

bool a3dGetFocus(A3D_WND* wnd)
{
	Window focused;
	int revert_to;
	XGetInputFocus(dpy, &focused, &revert_to);
	return focused == wnd->win;
}

void a3dCharSync(A3D_WND* wnd)
{
	if (wnd->ic)
		Xutf8ResetIC(wnd->ic);					
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
    static Atom netWmIcon = XInternAtom(dpy,"_NET_WM_ICON", False);
	A3D_WND* wnd = (A3D_WND*)cookie;

	int wh = w*h;

	unsigned long* wh_buf = (unsigned long*)malloc(sizeof(unsigned long)*(2+wh));
	wh_buf[0]=w;
	wh_buf[1]=h;

	unsigned long* buf = wh_buf + 2;

	// convert to 0x[0]AARRGGBB unsigned long!!!
	Convert_UL_AARRGGBB(buf,f,w,h,data,palsize,palbuf);

    XChangeProperty(dpy, wnd->win, netWmIcon, XA_CARDINAL, 32, PropModeReplace, 
					(const unsigned char*)wh_buf, 2 + wh);

	free(wh_buf);
}

bool a3dSetIconData(A3D_WND* wnd, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
{
	_a3dSetIconData(wnd, f, w, h, data, palsize, palbuf);
	return true;
}

bool a3dSetIcon(A3D_WND* wnd, const char* path)
{
	return a3dLoadImage(path, wnd, _a3dSetIconData);
}

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
		 
		if (cb && !cb(item, dir->d_name,cookie))
			break;
		num++;
	}

	closedir(d);
	return num;	
}

bool a3dSetCurDir(const char* dir_path)
{
	return chdir(dir_path) == 0;
}

bool a3dGetCurDir(char* dir_path, int size)
{
	if (!dir_path)
		return false;
	if (getcwd(dir_path, size))
	{
		int len = strlen(dir_path);
		if (len<size-1)
		{
			dir_path[len]='/';
			dir_path[len+1]=0;
		}
	}
	return true;
}

struct A3D_THREAD
{
	pthread_t th;
};

A3D_THREAD* a3dCreateThread(void* (*entry)(void*), void* arg)
{
	pthread_t th;
	pthread_create(&th, 0, entry, arg);
	if (!th)
		return 0;

	A3D_THREAD* t = (A3D_THREAD*)malloc(sizeof(A3D_THREAD));
	t->th = th;
	return t;
}

void* a3dWaitForThread(A3D_THREAD* thread)
{
	void* ret = 0;
	pthread_join(thread->th,&ret);
	free(thread);
	return ret;
}

struct A3D_MUTEX
{
	pthread_mutex_t mu;
};

A3D_MUTEX* a3dCreateMutex()
{
	A3D_MUTEX* m = (A3D_MUTEX*)malloc(sizeof(A3D_MUTEX));
	pthread_mutex_init(&m->mu, 0);
	return m;
}

void a3dDeleteMutex(A3D_MUTEX* mutex)
{
	pthread_mutex_destroy(&mutex->mu);
	free(mutex);
}

void a3dMutexLock(A3D_MUTEX* mutex)
{
	pthread_mutex_lock(&mutex->mu);
}

void a3dMutexUnlock(A3D_MUTEX* mutex)
{
	pthread_mutex_unlock(&mutex->mu);
}

void a3dSetPtyVT(A3D_PTY* pty, A3D_VT* vt)
{
	pty->vt = vt;
}

A3D_VT* a3dGetPtyVT(A3D_PTY* pty)
{
	return pty->vt;
}

/*
A3D_PTY* a3dOpenPty(int w, int h, const char* path, char* const argv[], char* const envp[])
{
	A3D_PTY* pty = (A3D_PTY*)malloc(sizeof(A3D_PTY));
	if (!pty)
		return 0;

	int pfd[2];
	if (pipe(pfd) != 0)	
	{
		free(pty);
		return 0;
	}		

    struct winsize ws;
    ws.ws_col = w;
    ws.ws_row = h;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;		

	int master,slave;
	char name[1024];
	openpty(&master, &slave, name, 0, &ws);

	pid_t pid = fork();
	if (pid == 0)
	{
		setsid();

		int tfd = open(name, O_RDWR);
		ioctl(tfd, TIOCSCTTY, 0);
		close(tfd);

		// Redirect stdin/stdout/stderr to the pty
		if (dup2(slave, 1) == -1) 
			exit(-1);
		if (dup2(slave, 2) == -1) 
			exit(-1);
		if (dup2(slave, 0) == -1) 
			exit(-1);

		close(slave);

		signal(SIGPIPE, SIG_DFL);

		if (!envp)
			envp = environ;

		execvpe(path, argv, envp);
		exit(-1);
	}

	pty->vt = 0;
	pty->next = 0;
	pty->prev = tail_pty;
	if (tail_pty)
		tail_pty->next = pty;
	else
		head_pty = pty;
	tail_pty = pty;
	
	pty->fd = master;
	pty->pd[0] = pfd[0];
	pty->pd[1] = pfd[1];
	pty->pid = pid;

	return pty;
}
*/

A3D_PTY* a3dOpenPty(int w, int h, const char* path, char* const argv[], char* const envp[])
{
	A3D_PTY* pty = (A3D_PTY*)malloc(sizeof(A3D_PTY));
	if (!pty)
		return 0;

	int pfd[2];
	if (pipe(pfd) != 0)	
	{
		free(pty);
		return 0;
	}

    struct winsize ws;
    ws.ws_col = w;
    ws.ws_row = h;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    char name[64]="";
	int pty_fd = -1;

    pid_t pid = forkpty(&pty_fd, name, 0, &ws);
    if (pid == 0)
    {
        // child
	    ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);

		char* no_args[] = {0};
		if (!envp)
			envp = environ;
		if (!argv)
			argv = no_args;

        execvpe(path, argv, envp);
        exit(1);
    }

    if (pid < 0 || pty_fd < 0)
    {
		free(pty);
		close(pfd[0]);
		close(pfd[1]);

		//error
        if (pty_fd>=0)
            close(pty_fd);
		return 0;
	}

	// parent

	pty->vt = 0;
	pty->next = 0;
	pty->prev = tail_pty;
	if (tail_pty)
		tail_pty->next = pty;
	else
		head_pty = pty;
	tail_pty = pty;
	
	pty->fd = pty_fd;
	pty->pd[0] = pfd[0];
	pty->pd[1] = pfd[1];
	pty->pid = pid;

	return pty;
}


int a3dReadPTY(A3D_PTY* pty, void* buf, size_t size)
{
	fd_set set;
	FD_ZERO (&set);
	FD_SET(pty->fd, &set);
	FD_SET(pty->pd[0], &set);

	int max_fd = pty->fd > pty->pd[0] ? pty->fd : pty->pd[0];
	int sel = select(max_fd+1, &set, 0,0,0);
	if (FD_ISSET(pty->pd[0], &set))
		return -1;

	return read(pty->fd, buf, size);
}

int a3dWritePTY(A3D_PTY* pty, const void* buf, size_t size)
{
	return write(pty->fd, buf, size);
}

void a3dResizePTY(A3D_PTY* pty, int w, int h)
{
    // recalc new vt w,h
    struct winsize ws;
    ws.ws_col = w;
    ws.ws_row = h;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

	while ( ioctl(pty->fd, TIOCSWINSZ, &ws) == -1 )
	{
		if (errno != EINTR)
			break;
	}

	// kill(pty->pid,SIGWINCH);
}

void a3dUnblockPTY(A3D_PTY* pty)
{
	// force select to exit and set pd[0] (no more reads)
	if ( write(pty->pd[1], "\n", 1) <= 0)
	{
		// weird, it is our own pipe.
	}
}

void a3dClosePTY(A3D_PTY* pty)
{
	a3dUnblockPTY(pty);

	int stat;
//	kill(pty->pid, SIGKILL);
//	write(pty->fd,"\n",1);
	int ret = close(pty->fd);
	waitpid(pty->pid, &stat, 0);

	close(pty->pd[0]);
	close(pty->pd[1]);

	if (pty->prev)
		pty->prev->next = pty->next;
	else
		head_pty = pty->next;

	if (pty->next)
		pty->next->prev = pty->prev;
	else
		tail_pty = pty->prev;

	free(pty);
}

#endif // USE_SDL
#endif // __linux__
