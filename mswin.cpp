#ifdef _WIN32
#ifndef USE_SDL
// PLATFORM: MS-WINDOWS 

#include <Windows.h>
#pragma comment(lib,"OpenGL32.lib")

#include <crtdbg.h>

#include <stdint.h>

#include "rgba8.h"

#include "gl.h"
#include "wglext.h"

#include "platform.h"


A3D_PTY* head_pty = 0;
A3D_PTY* tail_pty = 0;

struct A3D_PTY
{
	/*
	int fd;
	pid_t pid;
	A3D_PTY* next;
	A3D_PTY* prev;
	A3D_VT* vt;
	*/
};

LARGE_INTEGER coarse_perf, timer_freq;
uint64_t coarse_micro;

A3D_WND* wnd_head = 0;
A3D_WND* wnd_tail = 0;

struct A3D_WND
{
	PlatformInterface platform_api;

	HWND hwnd;
	HDC dc;
	HGLRC rc;

	A3D_WND* prev;
	A3D_WND* next;
	void* cookie;

	int mouse_b = 0;
	int mouse_x = 0;
	int mouse_y = 0;
	bool track = false;
	bool mapped = false;
	WndMode wndmode = A3D_WND_NORMAL;
	int exit_full_xywh[4] = { 0,0,0,0 }; // always a3dGetRect
};

void a3dPushContext(A3D_PUSH_CONTEXT* ctx)
{
	ctx->data[0] = 0;
	ctx->data[1] = wglGetCurrentDC();
	ctx->data[2] = wglGetCurrentContext();
}

void a3dPopContext(const A3D_PUSH_CONTEXT* ctx)
{
	wglMakeCurrent((HDC)ctx->data[1], (HGLRC)ctx->data[2]);
}

void a3dSwitchContext(const A3D_WND* wnd)
{
	wglMakeCurrent(wnd->dc, wnd->rc);
}

static const unsigned char ki_to_vk[256] =
{
	0,

	VK_BACK,
	VK_TAB,
	VK_RETURN,

	VK_PAUSE,
	VK_ESCAPE,

	VK_SPACE,
	VK_PRIOR,
	VK_NEXT,
	VK_END,
	VK_HOME,
	VK_LEFT,
	VK_UP,
	VK_RIGHT,
	VK_DOWN,

	VK_PRINT,
	VK_INSERT,
	VK_DELETE,

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

	VK_LWIN,
	VK_RWIN,
	VK_APPS,

	VK_NUMPAD0,
	VK_NUMPAD1,
	VK_NUMPAD2,
	VK_NUMPAD3,
	VK_NUMPAD4,
	VK_NUMPAD5,
	VK_NUMPAD6,
	VK_NUMPAD7,
	VK_NUMPAD8,
	VK_NUMPAD9,
	VK_MULTIPLY,
	VK_DIVIDE,
	VK_ADD,
	VK_SUBTRACT,
	VK_DECIMAL,
	VK_RETURN, // numpad!

	VK_F1,
	VK_F2,
	VK_F3,
	VK_F4,
	VK_F5,
	VK_F6,
	VK_F7,
	VK_F8,
	VK_F9,
	VK_F10,
	VK_F11,
	VK_F12,
	VK_F13,
	VK_F14,
	VK_F15,
	VK_F16,
	VK_F17,
	VK_F18,
	VK_F19,
	VK_F20,
	VK_F21,
	VK_F22,
	VK_F23,
	VK_F24,

	VK_CAPITAL,
	VK_NUMLOCK,
	VK_SCROLL,

	VK_LSHIFT,
	VK_RSHIFT,
	VK_LCONTROL,
	VK_RCONTROL,
	VK_LMENU,
	VK_RMENU,

	VK_OEM_1,		// ';:' for US
	VK_OEM_PLUS,	// '+' any country
	VK_OEM_COMMA,	// ',' any country
	VK_OEM_MINUS,	// '-' any country
	VK_OEM_PERIOD,	// '.' any country
	VK_OEM_2,		// '/?' for US
	VK_OEM_3,		// '`~' for US

	VK_OEM_4,       //  '[{' for US
	VK_OEM_6,		//  ']}' for US
	VK_OEM_5,		//  '\|' for US
	VK_OEM_7,		//  ''"' for US
	0,
};


static const unsigned char vk_to_ki[256] =
{
	0,					// 0x00
	0,					// VK_LBUTTON        0x01
	0,					// VK_RBUTTON        0x02
	0,					// VK_CANCEL         0x03
	0,					// VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */
	0,					// VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
	0,					// VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */
	0,					// * 0x07 : reserved
	A3D_BACKSPACE,		// VK_BACK           0x08
	A3D_TAB,			// VK_TAB            0x09
	0,					// reserved
	0,					// reserved
	0,					// VK_CLEAR          0x0C
	A3D_ENTER,			// VK_RETURN         0x0D   - CHECK IF EXTENED then 124 (numpad ENTER)
	0,					// unassigned
	0,					// unassigned
	A3D_LSHIFT,			// VK_SHIFT          0x10 - CHECK IF EXTENED then 108 (VK_RSHIFT)
	A3D_LCTRL,			// VK_CONTROL        0x11 - CHECK IF EXTENED then 110 (VK_RCONTROL)
	A3D_LALT,			// VK_MENU           0x12 - CHECK IF EXTENED then 112 (VK_RMENU)
	A3D_PAUSE,			// VK_PAUSE          0x13
	A3D_CAPSLOCK,		// VK_CAPITAL        0x14
	0,					// VK_KANA           0x15
	0,					// * 0x16 : unassigned
	0,					// VK_JUNJA          0x17
	0,					// VK_FINAL          0x18
	0,					// VK_HANJA          0x19 // VK_KANJI          0x19
	0,					// 0x1A : unassigned
	A3D_ESCAPE,			// VK_ESCAPE         0x1B
	0,					// VK_CONVERT        0x1C
	0,					// VK_NONCONVERT     0x1D
	0,					// VK_ACCEPT         0x1E
	0,					// VK_MODECHANGE     0x1F
	A3D_SPACE,			// VK_SPACE          0x20
	A3D_PAGEUP,			// VK_PRIOR          0x21
	A3D_PAGEDOWN,		// VK_NEXT           0x22
	A3D_END,			// VK_END            0x23
	A3D_HOME,			// VK_HOME           0x24
	A3D_LEFT,			// VK_LEFT           0x25
	A3D_UP,				// VK_UP             0x26
	A3D_RIGHT,			// VK_RIGHT          0x27
	A3D_DOWN,			// VK_DOWN           0x28
	0,					// VK_SELECT         0x29
	A3D_PRINT,			// VK_PRINT          0x2A
	0,					// VK_EXECUTE        0x2B
	0,					// VK_SNAPSHOT       0x2C
	A3D_INSERT,			// VK_INSERT         0x2D
	A3D_DELETE,			// VK_DELETE         0x2E
	0,					// VK_HELP           0x2F
	A3D_0,				// VK_0				 0x30
	A3D_1,				// VK_1				 0x31
	A3D_2,				// VK_2				 0x32
	A3D_3,				// VK_3				 0x33
	A3D_4,				// VK_4				 0x34
	A3D_5,				// VK_5				 0x35
	A3D_6,				// VK_6				 0x36
	A3D_7,				// VK_7				 0x37
	A3D_8,				// VK_8				 0x38
	A3D_9,				// VK_9				 0x39
	0,					// * 0x3A - unassigned
	0,					// * 0x3B - unassigned
	0,					// * 0x3C - unassigned
	0,					// * 0x3D - unassigned
	0,					// * 0x3E - unassigned
	0,					// * 0x3F - unassigned
	0,					// - 0x40 : unassigned
	A3D_A,				// VK_A				 0x41
	A3D_B,				// VK_B				 0x42
	A3D_C,				// VK_C				 0x43
	A3D_D,				// VK_D				 0x44
	A3D_E,				// VK_E				 0x45
	A3D_F,				// VK_F				 0x46
	A3D_G,				// VK_G				 0x47
	A3D_H,				// VK_H				 0x48
	A3D_I,				// VK_I				 0x49
	A3D_J,				// VK_J				 0x4A
	A3D_K,				// VK_K              0x4B
	A3D_L,				// VK_L              0x4C
	A3D_M,				// VK_M              0x4D
	A3D_N,				// VK_N              0x4E
	A3D_O,				// VK_O              0x4F
	A3D_P,				// VK_P              0x50
	A3D_Q,				// VK_Q              0x51
	A3D_R,				// VK_R              0x52
	A3D_S,				// VK_S              0x53
	A3D_T,				// VK_T              0x54
	A3D_U,				// VK_U				 0x55
	A3D_V,				// VK_V				 0x56
	A3D_W,				// VK_W				 0x57
	A3D_X,				// VK_X				 0x58
	A3D_Y,				// VK_Y				 0x59
	A3D_Z,				// VK_Z              0x5A
	A3D_LWIN,			// VK_LWIN           0x5B
	A3D_RWIN,			// VK_RWIN           0x5C
	A3D_APPS,			// VK_APPS           0x5D
	0,					// * 0x5E : reserved
	0,					// VK_SLEEP          0x5F
	A3D_NUMPAD_0,		// VK_NUMPAD0        0x60
	A3D_NUMPAD_1,		// VK_NUMPAD1        0x61
	A3D_NUMPAD_2,		// VK_NUMPAD2        0x62
	A3D_NUMPAD_3,		// VK_NUMPAD3        0x63
	A3D_NUMPAD_4,		// VK_NUMPAD4        0x64
	A3D_NUMPAD_5,		// VK_NUMPAD5        0x65
	A3D_NUMPAD_6,		// VK_NUMPAD6        0x66
	A3D_NUMPAD_7,		// VK_NUMPAD7        0x67
	A3D_NUMPAD_8,		// VK_NUMPAD8        0x68
	A3D_NUMPAD_9,		// VK_NUMPAD9        0x69
	A3D_NUMPAD_MULTIPLY,// VK_MULTIPLY       0x6A
	A3D_NUMPAD_ADD,		// VK_ADD            0x6B
	0,					// VK_SEPARATOR      0x6C
	A3D_NUMPAD_SUBTRACT,// VK_SUBTRACT       0x6D
	A3D_NUMPAD_DECIMAL, // VK_DECIMAL        0x6E
	A3D_NUMPAD_DIVIDE,	// VK_DIVIDE         0x6F
	A3D_F1,				// VK_F1             0x70
	A3D_F2,				// VK_F2             0x71
	A3D_F3,				// VK_F3             0x72
	A3D_F4,				// VK_F4             0x73
	A3D_F5,				// VK_F5             0x74
	A3D_F6,				// VK_F6             0x75
	A3D_F7,				// VK_F7             0x76
	A3D_F8,				// VK_F8             0x77
	A3D_F9,				// VK_F9             0x78
	A3D_F10,			// VK_F10            0x79
	A3D_F11,			// VK_F11            0x7A
	A3D_F12,			// VK_F12            0x7B
	A3D_F13,			// VK_F13            0x7C
	A3D_F14,			// VK_F14            0x7D
	A3D_F15,			// VK_F15            0x7E
	A3D_F16,			// VK_F16            0x7F
	A3D_F17,			// VK_F17            0x80
	A3D_F18,			// VK_F18            0x81
	A3D_F19,			// VK_F19            0x82
	A3D_F20,			// VK_F20            0x83
	A3D_F21,			// VK_F21            0x84
	A3D_F22,			// VK_F22            0x85
	A3D_F23,			// VK_F23            0x86
	A3D_F24,			// VK_F24            0x87
	0,					// VK_NAVIGATION_VIEW     0x88 // reserved
	0,					// VK_NAVIGATION_MENU     0x89 // reserved
	0,					// VK_NAVIGATION_UP       0x8A // reserved
	0,					// VK_NAVIGATION_DOWN     0x8B // reserved
	0,					// VK_NAVIGATION_LEFT     0x8C // reserved
	0,					// VK_NAVIGATION_RIGHT    0x8D // reserved
	0,					// VK_NAVIGATION_ACCEPT   0x8E // reserved
	0,					// VK_NAVIGATION_CANCEL   0x8F // reserved
	A3D_NUMLOCK,		// VK_NUMLOCK        0x90
	A3D_SCROLLLOCK,		// VK_SCROLL         0x91
	0,					// VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad AND VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
	0,					// VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
	0,					// VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
	0,					// VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
	0,					// VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key
	0,					// 0x97 : unassigned
	0,					// 0x98 : unassigned
	0,					// 0x99 : unassigned
	0,					// 0x9A : unassigned
	0,					// 0x9B : unassigned
	0,					// 0x9C : unassigned
	0,					// 0x9D : unassigned
	0,					// 0x9E : unassigned
	0,					// 0x9F : unassigned
	A3D_LSHIFT,			// VK_LSHIFT         0xA0
	A3D_RSHIFT,			// VK_RSHIFT         0xA1
	A3D_LCTRL,			// VK_LCONTROL       0xA2
	A3D_RCTRL,			// VK_RCONTROL       0xA3
	A3D_LALT,			// VK_LMENU          0xA4
	A3D_RALT,			// VK_RMENU          0xA5
	0,					// VK_BROWSER_BACK        0xA6
	0,					// VK_BROWSER_FORWARD     0xA7
	0,					// VK_BROWSER_REFRESH     0xA8
	0,					// VK_BROWSER_STOP        0xA9
	0,					// VK_BROWSER_SEARCH      0xAA
	0,					// VK_BROWSER_FAVORITES   0xAB
	0,					// VK_BROWSER_HOME        0xAC
	0,					// VK_VOLUME_MUTE         0xAD
	0,					// VK_VOLUME_DOWN         0xAE
	0,					// VK_VOLUME_UP           0xAF
	0,					// VK_MEDIA_NEXT_TRACK    0xB0
	0,					// VK_MEDIA_PREV_TRACK    0xB1
	0,					// VK_MEDIA_STOP          0xB2
	0,					// VK_MEDIA_PLAY_PAUSE    0xB3
	0,					// VK_LAUNCH_MAIL         0xB4
	0,					// VK_LAUNCH_MEDIA_SELECT 0xB5
	0,					// VK_LAUNCH_APP1         0xB6
	0,					// VK_LAUNCH_APP2         0xB7
	0,					// 0xB8 : reserved
	0,					// 0xB9 : reserved
	A3D_OEM_COLON,		// VK_OEM_1          0xBA   // ';:' for US
	A3D_OEM_PLUS,		// VK_OEM_PLUS       0xBB   // '+' any country
	A3D_OEM_COMMA,		// VK_OEM_COMMA      0xBC   // ',' any country
	A3D_OEM_MINUS,		// VK_OEM_MINUS      0xBD   // '-' any country
	A3D_OEM_PERIOD,		// VK_OEM_PERIOD     0xBE   // '.' any country
	A3D_OEM_SLASH,		// VK_OEM_2          0xBF   // '/?' for US
	A3D_OEM_TILDE,		// VK_OEM_3          0xC0   // '`~' for US
	0,					// 0xC1 : reserved
	0,					// 0xC2 : reserved
	0,					//VK_GAMEPAD_A                         0xC3 // reserved
	0,					//VK_GAMEPAD_B                         0xC4 // reserved
	0,					//VK_GAMEPAD_X                         0xC5 // reserved
	0,					//VK_GAMEPAD_Y                         0xC6 // reserved
	0,					//VK_GAMEPAD_RIGHT_SHOULDER            0xC7 // reserved
	0,					//VK_GAMEPAD_LEFT_SHOULDER             0xC8 // reserved
	0,					//VK_GAMEPAD_LEFT_TRIGGER              0xC9 // reserved
	0,					//VK_GAMEPAD_RIGHT_TRIGGER             0xCA // reserved
	0,					//VK_GAMEPAD_DPAD_UP                   0xCB // reserved
	0,					//VK_GAMEPAD_DPAD_DOWN                 0xCC // reserved
	0,					//VK_GAMEPAD_DPAD_LEFT                 0xCD // reserved
	0,					//VK_GAMEPAD_DPAD_RIGHT                0xCE // reserved
	0,					//VK_GAMEPAD_MENU                      0xCF // reserved
	0,					//VK_GAMEPAD_VIEW                      0xD0 // reserved
	0,					//VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON    0xD1 // reserved
	0,					//VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON   0xD2 // reserved
	0,					//VK_GAMEPAD_LEFT_THUMBSTICK_UP        0xD3 // reserved
	0,					//VK_GAMEPAD_LEFT_THUMBSTICK_DOWN      0xD4 // reserved
	0,					//VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT     0xD5 // reserved
	0,					//VK_GAMEPAD_LEFT_THUMBSTICK_LEFT      0xD6 // reserved
	0,					//VK_GAMEPAD_RIGHT_THUMBSTICK_UP       0xD7 // reserved
	0,					//VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN     0xD8 // reserved
	0,					//VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT    0xD9 // reserved
	0,					//VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT     0xDA // reserved
	A3D_OEM_OPEN,		// VK_OEM_4          0xDB  //  '[{' for US
	A3D_OEM_BACKSLASH,	// VK_OEM_5          0xDC  //  '\|' for US
	A3D_OEM_CLOSE,		// VK_OEM_6          0xDD  //  ']}' for US
	A3D_OEM_QUOTATION,	// VK_OEM_7          0xDE  //  ''"' for US
	0,					// VK_OEM_8          0xDF
	0,					// 0xE0 : reserved
	0,					// VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
	0,					// VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
	0,					// VK_ICO_HELP       0xE3  //  Help key on ICO
	0,					// VK_ICO_00         0xE4  //  00 key on ICO
	0,					// VK_PROCESSKEY     0xE5
	0,					// VK_ICO_CLEAR      0xE6
	0,					// VK_PACKET         0xE7
	0,					// * 0xE8 : unassigned
	0,					// VK_OEM_RESET      0xE9
	0,					// VK_OEM_JUMP       0xEA
	0,					// VK_OEM_PA1        0xEB
	0,					// VK_OEM_PA2        0xEC
	0,					// VK_OEM_PA3        0xED
	0,					// VK_OEM_WSCTRL     0xEE
	0,					// VK_OEM_CUSEL      0xEF
	0,					// VK_OEM_ATTN       0xF0
	0,					// VK_OEM_FINISH     0xF1
	0,					// VK_OEM_COPY       0xF2
	0,					// VK_OEM_AUTO       0xF3
	0,					// VK_OEM_ENLW       0xF4
	0,					// VK_OEM_BACKTAB    0xF5
	0,					// VK_ATTN           0xF6
	0,					// VK_CRSEL          0xF7
	0,					// VK_EXSEL          0xF8
	0,					// VK_EREOF          0xF9
	0,					// VK_PLAY           0xFA
	0,					// VK_ZOOM           0xFB
	0,					// VK_NONAME         0xFC
	0,					// VK_PA1            0xFD
	0,					// VK_OEM_CLEAR      0xFE
	0					// 0xFF
};

LRESULT WINAPI a3dWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	A3D_WND* wnd = 0;

	if (m == WM_CREATE)
	{
		wnd = (A3D_WND*)malloc(sizeof(A3D_WND));
		wnd->cookie = 0;
		wnd->track = false;
		wnd->mouse_b = 0;

		if (!wnd_head)
		{
			QueryPerformanceFrequency(&timer_freq);
			QueryPerformanceCounter(&coarse_perf);
			coarse_micro = 0;
		}

		SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)wnd);

		// set timer to refresh coarse_perf & coarse_micro every minute
		// to prevent overflows in a3dGetTIme 
		SetTimer(h, 1, 60000, 0);


		wnd->prev = wnd_tail;
		wnd->next = 0;
		if (wnd_tail)
			wnd_tail->next = wnd;
		else
			wnd_head = wnd;
		wnd_tail = wnd;

		return TRUE;
	}

	wnd = (A3D_WND*)GetWindowLongPtr(h, GWLP_USERDATA);

	if (!wnd)
		return DefWindowProcW(h, m, w, l);

	wglMakeCurrent(wnd->dc, wnd->rc);

	int mi = 0;
	int rep = 1;

	switch (m)
	{
		case WM_DESTROY:
		{
			SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)0);

			wglMakeCurrent(0, 0);
			wglDeleteContext(wnd->rc);
			ReleaseDC(h, wnd->dc);

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
			if (w == 1 && wnd == wnd_head)
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
				if (wnd->platform_api.render)
					wnd->platform_api.render(wnd);
			}
			break;
		}
		
		case WM_CLOSE:
			if (wnd->platform_api.close)
				wnd->platform_api.close(wnd);
			else
			{
				DestroyWindow(h);
			}
			return 0;

		case WM_SIZE:
			if (wnd->platform_api.resize)
				wnd->platform_api.resize(wnd,LOWORD(l),HIWORD(l));
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
			if (wnd->platform_api.keyb_focus)
				wnd->platform_api.keyb_focus(wnd,true);
			break;

		case WM_KILLFOCUS:
			if (wnd->platform_api.keyb_focus)
				wnd->platform_api.keyb_focus(wnd, false);
			break;

		case WM_CHAR:
			if (wnd->platform_api.keyb_char)
				wnd->platform_api.keyb_char(wnd, (wchar_t)w);
			break;

		case WM_SYSCOMMAND:
		{
			// prevent enering sysmenu when
			// F10 or ALT is released.
			if (w == SC_KEYMENU)
				return 0;
			break;
		}

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (wnd->platform_api.keyb_key && w < 256)
			{
				KeyInfo ki = (KeyInfo)vk_to_ki[w];
				if (!ki)
					break;

				if ((l >> 24) & 1) // enh
				{
					switch (ki)
					{
					case A3D_LSHIFT:   ki = A3D_RSHIFT;			break;
					case A3D_LCTRL:    ki = A3D_RCTRL;			break;
					case A3D_LALT:     ki = A3D_RALT;			break;
					case A3D_ENTER:    ki = A3D_NUMPAD_ENTER;	break;
					}
				}
				else
				{
					KeyInfo ki_numlock = ki;
					switch (ki)
					{
					case A3D_INSERT:   ki_numlock = A3D_NUMPAD_0;		break;
					case A3D_DELETE:   ki_numlock = A3D_NUMPAD_DECIMAL; break;
					case A3D_END:      ki_numlock = A3D_NUMPAD_1;		break;
					case A3D_DOWN:     ki_numlock = A3D_NUMPAD_2;		break;
					case A3D_PAGEDOWN: ki_numlock = A3D_NUMPAD_3;		break;
					case A3D_LEFT:     ki_numlock = A3D_NUMPAD_4;		break;
					case A3D_RIGHT:    ki_numlock = A3D_NUMPAD_6;		break;
					case A3D_HOME:     ki_numlock = A3D_NUMPAD_7;		break;
					case A3D_UP:       ki_numlock = A3D_NUMPAD_8;		break;
					case A3D_PAGEUP:   ki_numlock = A3D_NUMPAD_9;		break;
					}

					if (ki != ki_numlock)
					{
						if (!(GetKeyState(VK_NUMLOCK) & 1))
							ki = ki_numlock;
					}
				}

				if (m == WM_KEYDOWN && (l & (1 << 30)))
					ki = (KeyInfo)((int)ki | A3D_AUTO_REPEAT);

				wnd->platform_api.keyb_key(wnd, ki, m == WM_KEYDOWN || m == WM_SYSKEYDOWN);

				/*
				if (m == WM_SYSKEYUP || m == WM_KEYUP)
				{
					if (ki == A3D_LALT || ki == A3D_RALT)
					{
						return 0;
					}
				}
				*/
			}

			// delete key does not produce delete char! - let's emulate
			if (wnd->platform_api.keyb_char && w == VK_DELETE && (m == WM_KEYDOWN || m == WM_SYSKEYDOWN))
			{
				wnd->platform_api.keyb_char(wnd, 127);
			}

			break;

		/*
		case WM_CAPTURECHANGED:
		{
		}
		*/

		case WM_MOUSELEAVE:
			wnd->track = false;
			if (wnd->platform_api.mouse)
				wnd->platform_api.mouse(wnd, wnd->mouse_x, wnd->mouse_y, MouseInfo::LEAVE);
			break;

		case WM_MOUSEMOVE:
			wnd->mouse_x = (short)LOWORD(l);
			wnd->mouse_y = (short)HIWORD(l);
			mi = MouseInfo::MOVE;
			if (!wnd->track)
			{
				mi = MouseInfo::ENTER;
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.dwHoverTime = HOVER_DEFAULT;
				tme.hwndTrack = h;
				TrackMouseEvent(&tme);
				wnd->track = true;
			}
			break;

		case WM_LBUTTONDOWN:
			wnd->mouse_b |= MouseInfo::LEFT;
			mi = MouseInfo::LEFT_DN;
			SetCapture(h);
			break;

		case WM_LBUTTONUP:
			wnd->mouse_b &= ~MouseInfo::LEFT;
			mi = MouseInfo::LEFT_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;

		case WM_RBUTTONDOWN:
			wnd->mouse_b |= MouseInfo::RIGHT;
			mi = MouseInfo::RIGHT_DN;
			SetCapture(h);
			break;

		case WM_RBUTTONUP:
			wnd->mouse_b &= ~MouseInfo::RIGHT;
			mi = MouseInfo::RIGHT_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;

		case WM_MBUTTONDOWN:
			wnd->mouse_b |= MouseInfo::MIDDLE;
			mi = MouseInfo::MIDDLE_DN;
			SetCapture(h);
			break;

		case WM_MBUTTONUP:
			wnd->mouse_b &= ~MouseInfo::MIDDLE;
			mi = MouseInfo::MIDDLE_UP;
			if (0 == (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
				ReleaseCapture();
			break;
	}

	if (mi && wnd->platform_api.mouse)
	{
		if (w & MK_LBUTTON)
			mi |= MouseInfo::LEFT;
		if (w & MK_RBUTTON)
			mi |= MouseInfo::RIGHT;
		if (w & MK_MBUTTON)
			mi |= MouseInfo::MIDDLE;

		for (int i=0; i<rep; i++)
			wnd->platform_api.mouse(wnd, wnd->mouse_x, wnd->mouse_y, (MouseInfo)mi);

		return 0;
	}

	return DefWindowProc(h, m, w, l);
}

#include <stdio.h>
/*
LONG WINAPI a3dExceptionProc(EXCEPTION_POINTERS *ExceptionInfo)
{
	printf("Exception has been ignored!\n");
	return EXCEPTION_CONTINUE_EXECUTION;
}
*/

// creates window & initialized GL
A3D_WND* a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd, A3D_WND* share)
{
	// push current context!
	struct PUSH
	{
		PUSH()
		{
			if (wnd_head)
			{
				pop = true;
				dc = wglGetCurrentDC();
				rc = wglGetCurrentContext();
			}
			else
			{
				pop = false;
			}

		}

		~PUSH()
		{
			if (pop)
				wglMakeCurrent(dc, rc);
		}

		bool pop;
		HDC dc;
		HGLRC rc;

	} push;

	//_CrtSetBreakAlloc(69);

	if (!pi || !gd)
		return false;

	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = a3dWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(0);
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"A3DWNDCLASS";

	if (!wnd_head && !RegisterClass(&wc))
		return false;

	DWORD styles = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	styles |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;

	HWND h = CreateWindow(wc.lpszClassName, L"", styles,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		share ? share->hwnd : 0, 0, wc.hInstance, 0);

	if (!h)
	{
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	A3D_WND* wnd = (A3D_WND*)GetWindowLongPtr(h, GWLP_USERDATA);
	memset(&wnd->platform_api, 0, sizeof(PlatformInterface));
	wnd->hwnd = h;

	styles |= WS_POPUP; // add it later (hack to make CW_USEDEFAULT working)
	SetWindowLong(h, GWL_STYLE, styles);

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags =
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		gd->flags & GraphicsDesc::DOUBLE_BUFFER ? PFD_DOUBLEBUFFER : 0 |
		gd->depth_bits ? 0 : PFD_DEPTH_DONTCARE;

	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = gd->color_bits;
	pfd.cAlphaBits = gd->alpha_bits;
	pfd.cDepthBits = gd->depth_bits;
	pfd.cStencilBits = gd->stencil_bits;

	HDC dc = GetDC(h);
	int pfi = ChoosePixelFormat(dc, &pfd);
	if (!pfi)
	{
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	if (!SetPixelFormat(dc, pfi, &pfd))
	{
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	HGLRC rc = wglCreateContext(dc);
	if (!rc)
	{
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	if (!wglMakeCurrent(dc, rc))
	{
		wglDeleteContext(rc);
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	// 
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)(wglGetProcAddress("wglCreateContextAttribsARB"));

	int  contextAttribs[] = {
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | (gd->flags & GraphicsDesc::DEBUG_CONTEXT ? WGL_CONTEXT_DEBUG_BIT_ARB : 0),
		WGL_CONTEXT_MAJOR_VERSION_ARB, gd->version[0]/*GALOGEN_API_VER_MAJ*/,
		WGL_CONTEXT_MINOR_VERSION_ARB, gd->version[1]/*GALOGEN_API_VER_MIN*/,
		WGL_CONTEXT_PROFILE_MASK_ARB, strcmp(GALOGEN_API_PROFILE,"core")==0 ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : 0,
		0
	};

	wglMakeCurrent(0, 0);
	wglDeleteContext(rc);

	rc = wglCreateContextAttribsARB(dc, share ? share->rc : 0, contextAttribs);
	if (!rc)
	{
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	if (!wglMakeCurrent(dc, rc))
	{
		wglDeleteContext(rc);
		ReleaseDC(h, dc);
		DestroyWindow(h);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return false;
	}

	wnd->platform_api = *pi;

	bool mapped = false;
	wnd->wndmode = A3D_WND_NORMAL;

	wnd->dc = dc;
	wnd->rc = rc;

	if (wnd->platform_api.init)
		wnd->platform_api.init(wnd);

	// send initial notifications:
	RECT r;
	GetClientRect(h, &r);
	if (wnd->platform_api.resize)
	{
		wglMakeCurrent(dc, rc);
		wnd->platform_api.resize(wnd, r.right, r.bottom);
	}

	return wnd;

	// LPTOP_LEVEL_EXCEPTION_FILTER old_exception_proc = SetUnhandledExceptionFilter(a3dExceptionProc);

	/*
	while (!wnd->closing)
	{
		MSG msg;
		while (!wnd->closing && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!wnd->closing && wnd->platform_api.render)
			wnd->platform_api.render();
	}

	_CrtMemDumpAllObjectsSince(&mem_state);

	wglMakeCurrent(0, 0);
	wglDeleteContext(rc);
	ReleaseDC(h, dc);
	DestroyWindow(h);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	// SetUnhandledExceptionFilter(old_exception_proc);

	return true;
	*/
}

void a3dLoop() // infinite untill all windows are destroyed
{
	// while (!wnd->closing)

	// for all created wnds, force size notifications
	A3D_WND* wnd = wnd_head;
	while (wnd)
	{
		if (wnd->platform_api.resize)
		{
			RECT rc;
			GetClientRect(wnd->hwnd, &rc);
			wnd->platform_api.resize(wnd, rc.right, rc.bottom);
		}
		wnd = wnd->next;
	}

	while (wnd_head)
	{
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		WGLSWAP swap[WGL_SWAPMULTIPLE_MAX];
		int num = 0;

		A3D_WND* wnd = wnd_head;
		while (wnd)
		{
			if (wnd->platform_api.render && (GetWindowLong(wnd->hwnd,GWL_STYLE)&WS_VISIBLE))
			{
				wglMakeCurrent(wnd->dc, wnd->rc);
				wnd->platform_api.render(wnd);
				swap[num].hdc = wnd->dc;
				swap[num].uiFlags = 1;
				num++;

				if (num == WGL_SWAPMULTIPLE_MAX)
				{
					wglSwapMultipleBuffers(num,swap);
					num = 0;
				}
			}
			wnd = wnd->next;
		}

		if (num == 1)
			SwapBuffers(swap[0].hdc);
		else
		if (num)
			wglSwapMultipleBuffers(num, swap);
	}

	UnregisterClass(L"A3DWNDCLASS", GetModuleHandle(0));

	// SetUnhandledExceptionFilter(old_exception_proc);
}

void a3dClose(A3D_WND* wnd)
{
	struct PUSH
	{
		PUSH()
		{
			dc = wglGetCurrentDC();
			rc = wglGetCurrentContext();
		}

		~PUSH()
		{
			wglMakeCurrent(dc, rc);
		}

		HDC dc;
		HGLRC rc;

	} push;

	DestroyWindow(wnd->hwnd);
	/*
	memset(&wnd->platform_api, 0, sizeof(PlatformInterface));
	wnd->closing = true;
	*/
}

void a3dSetCookie(A3D_WND* wnd, void* cookie)
{
	wnd->cookie = cookie;
}

void* a3dGetCookie(A3D_WND* wnd)
{
	return wnd->cookie;
}

uint64_t a3dGetTime()
{
	struct SafeTimer
	{
		static uint64_t Get1()
		{
			QueryPerformanceFrequency(&timer_freq);
			LARGE_INTEGER c;
			QueryPerformanceCounter(&c);
			uint64_t diff = c.QuadPart - coarse_perf.QuadPart;
			return coarse_micro + diff * 1000000 / timer_freq.QuadPart;
		}

		static uint64_t Get2()
		{
			QueryPerformanceFrequency(&timer_freq);
			LARGE_INTEGER c;
			QueryPerformanceCounter(&c);
			uint64_t diff = c.QuadPart - coarse_perf.QuadPart;
			return coarse_micro + diff * 1000000 / timer_freq.QuadPart;
		}
	};

	static uint64_t(*Get)() = SafeTimer::Get1;
	Get = SafeTimer::Get2;
	return Get();
}

/*
void a3dSwapBuffers()
{
	// no need wnd
	// swap buffers can be called only inside render cb
	// (caller makes appropriate MakeCurrent call)
	SwapBuffers(wglGetCurrentDC());
}
*/

bool a3dGetKeyb(A3D_WND* wnd, KeyInfo ki)
{
	if (ki < 0 || ki >= A3D_MAPEND || ki_to_vk[ki] == 0)
		return false;
	if (GetKeyState(ki_to_vk[ki]) & 0x8000)
		return true;
	return false;
}

void a3dSetTitle(A3D_WND* wnd, const char* utf8_name)
{
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, utf8_name, -1, NULL, 0);
	wchar_t* wstr = (wchar_t*)malloc(sizeof(wchar_t)*(1+wchars_num));
	MultiByteToWideChar(CP_UTF8, 0, utf8_name, -1, wstr, wchars_num);
	wstr[wchars_num]=0;
	HWND hWnd = wnd->hwnd;
	SetWindowTextW(hWnd, wstr);
	free(wstr);
}

int a3dGetTitle(A3D_WND* wnd, char* utf8_name, int size)
{
	HWND hWnd = wnd->hwnd;
	int wchars_num = GetWindowTextLength(hWnd);
	if (utf8_name && size>0)
	{
		wchar_t* wstr = (wchar_t*)malloc(sizeof(wchar_t)*(1+wchars_num));
		GetWindowTextW(hWnd,wstr,1+wchars_num);
		wstr[wchars_num]=0;
		BOOL incomplete = FALSE;
		int bytes = WideCharToMultiByte(CP_UTF8, 0, wstr, wchars_num, utf8_name, size, "?", &incomplete);
		utf8_name[size-1]=0;
		free(wstr);
		return bytes;
	}
	
	return 3*wchars_num; // 3 bytes / char is enough to store BMP 
}

void a3dSetVisible(A3D_WND* wnd, bool visible)
{
	HWND hWnd = wnd->hwnd;
	wnd->mapped = visible;
	ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);
}

bool a3dGetVisible(A3D_WND* wnd)
{
	return wnd->mapped;
}

WndMode a3dGetRect(A3D_WND* wnd, int* xywh, int* client_wh)
{
	HWND hWnd = wnd->hwnd;
	RECT r;

	if (client_wh)
	{
		GetClientRect(hWnd, &r);
		client_wh[0] = r.right;
		client_wh[1] = r.bottom;
	}

	if (xywh)
	{
		GetWindowRect(hWnd, &r);
		xywh[0] = r.left;
		xywh[1] = r.top;
		xywh[2] = r.right - r.left;
		xywh[3] = r.bottom - r.top;

		if (wnd->wndmode == A3D_WND_NORMAL)
		{
			HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONULL);
			if (mon)
			{
				WINDOWPLACEMENT wp;
				wp.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(hWnd, &wp);

				MONITORINFO nfo;
				nfo.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(mon, &nfo);

				if (wp.rcNormalPosition.left != xywh[0] - nfo.rcWork.left ||
					wp.rcNormalPosition.top != xywh[1] - nfo.rcWork.top ||
					wp.rcNormalPosition.right - wp.rcNormalPosition.left != xywh[2] ||
					wp.rcNormalPosition.bottom - wp.rcNormalPosition.top != xywh[3])
				{
					int left = xywh[0] - nfo.rcWork.left;
					int right = nfo.rcWork.right - (xywh[0] + xywh[2]);
					int top = xywh[1] - nfo.rcWork.top;
					int bottom = nfo.rcWork.bottom - (xywh[1] + xywh[3]);

					// all this mess was to clip docked window rect to workarea of docking monitor :)

					if (left < 0)
					{
						xywh[0] -= left;
						xywh[2] += left;
					}

					if (top < 0)
					{
						xywh[1] -= top;
						xywh[3] += top;
					}

					if (right < 0)
						xywh[2] += right;

					if (bottom < 0)
						xywh[3] += bottom;
				}
			}
		}
	}

	return wnd->wndmode;
}

bool a3dIsMaximized(A3D_WND* wnd)
{
	HWND hWnd = wnd->hwnd;
	return wnd->wndmode == A3D_WND_NORMAL && (WS_MAXIMIZE & GetWindowLong(hWnd, GWL_STYLE));
}

bool a3dSetRect(A3D_WND* wnd, const int* xywh, WndMode wnd_mode)
{
	if (!wnd->mapped)
		return false;

	HWND hWnd = wnd->hwnd;

	if (wnd_mode == A3D_WND_FULLSCREEN)
	{
		DWORD s = GetWindowLong(hWnd, GWL_STYLE);
		s &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

		if (wnd->wndmode != A3D_WND_FULLSCREEN)
		{
			a3dGetRect(wnd,wnd->exit_full_xywh, 0);
		}

		struct EnumMon
		{
			EnumMon() : num(0) {}
			static BOOL CALLBACK Enum(HMONITOR h, HDC dc, LPRECT pr, LPARAM em)
			{
				EnumMon* e = (EnumMon*)em;
				if (e->num < 16)
				{
					e->r[e->num][0] = pr->left;
					e->r[e->num][1] = pr->top;
					e->r[e->num][2] = pr->right - pr->left;
					e->r[e->num][3] = pr->bottom - pr->top;
				}
				e->num++;
				return TRUE;
			}
			int r[16][4];
			int num;
		};

		EnumMon em;
		EnumDisplayMonitors(NULL, 0, EnumMon::Enum, (LPARAM)&em);

		if (em.num > 16)
			em.num = 16;

		int wnd_xywh[4];
		if (!xywh)
		{
			a3dGetRect(wnd,wnd_xywh, 0);
			xywh = wnd_xywh;
		}

		// - locate monitor which is covered with greatest area by win

		int left_mon = -1;
		int right_mon = -1;
		int bottom_mon = -1;
		int top_mon = -1;

		for (int i = 0; i < em.num; i++)
		{
			int a, b;

			a = xywh[0] + xywh[2]; b = em.r[i][0] + em.r[i][2];
			int min_x = a < b ? a : b;
			a = xywh[0]; b = em.r[i][0];
			int max_x = a > b ? a : b;
			a = xywh[1] + xywh[3]; b = em.r[i][1] + em.r[i][3];
			int min_y = a < b ? a : b;
			a = xywh[1]; b = em.r[i][1];
			int max_y = a > b ? a : b;

			if (max_x < min_x && max_y < min_y)
			{
				if (left_mon < 0 || em.r[i][0] < em.r[left_mon][0])
					left_mon = i;

				if (right_mon < 0 || em.r[i][0] + em.r[i][2] > em.r[right_mon][0] + em.r[right_mon][2])
					right_mon = i;

				if (top_mon < 0 || em.r[i][1] < em.r[top_mon][1])
					top_mon = i;

				if (bottom_mon < 0 || em.r[i][1] + em.r[i][3] > em.r[bottom_mon][1] + em.r[bottom_mon][3])
					bottom_mon = i;
			}
		}

		if (left_mon < 0)
			return false;

		wnd_xywh[0] = em.r[left_mon][0];
		wnd_xywh[1] = em.r[top_mon][1];
		wnd_xywh[2] = em.r[right_mon][0] + em.r[right_mon][2] - em.r[left_mon][0];
		wnd_xywh[3] = em.r[bottom_mon][1] + em.r[bottom_mon][3] - em.r[top_mon][1];

		wnd->wndmode = wnd_mode;

		s &= ~WS_MAXIMIZE;
		SetWindowLong(hWnd, GWL_STYLE, s);
		SetWindowPos(hWnd, 0, wnd_xywh[0], wnd_xywh[1], wnd_xywh[2], wnd_xywh[3], SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
		return true;
	}

	if (wnd_mode == A3D_WND_FRAMELESS)
	{
		// remove decorations
		DWORD s = GetWindowLong(hWnd, GWL_STYLE);
		s &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

		int wnd_xywh[4];
		if (!xywh)
		{
			if (wnd->wndmode == A3D_WND_FULLSCREEN)
			{
				wnd_xywh[0] = wnd->exit_full_xywh[0];
				wnd_xywh[1] = wnd->exit_full_xywh[1];
				wnd_xywh[2] = wnd->exit_full_xywh[2];
				wnd_xywh[3] = wnd->exit_full_xywh[3];
			}
			else
			{
				a3dGetRect(wnd,wnd_xywh, 0);
			}
			xywh = wnd_xywh;
		}

		wnd->wndmode = wnd_mode;

		s &= ~WS_MAXIMIZE;
		SetWindowLong(hWnd, GWL_STYLE, s);
		SetWindowPos(hWnd, 0, xywh[0], xywh[1], xywh[2], xywh[3], SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
		return true;
	}

	if (wnd_mode == A3D_WND_NORMAL)
	{
		// add decorations
		DWORD s = GetWindowLong(hWnd, GWL_STYLE);
		s |= (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

		int wnd_xywh[4];
		if (!xywh)
		{
			if (wnd->wndmode == A3D_WND_FULLSCREEN)
			{
				wnd_xywh[0] = wnd->exit_full_xywh[0];
				wnd_xywh[1] = wnd->exit_full_xywh[1];
				wnd_xywh[2] = wnd->exit_full_xywh[2];
				wnd_xywh[3] = wnd->exit_full_xywh[3];
			}
			else
			{
				a3dGetRect(wnd,wnd_xywh, 0);
			}

			xywh = wnd_xywh;
		}

		wnd->wndmode = wnd_mode;

		s &= ~WS_MAXIMIZE;
		SetWindowLong(hWnd, GWL_STYLE, s);
		SetWindowPos(hWnd, 0, xywh[0], xywh[1], xywh[2], xywh[3], SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
		return true;
	}

	return false;
}


// mouse
MouseInfo a3dGetMouse(A3D_WND* wnd, int* x, int* y) // returns but flags, mouse wheel has no state
{
	HWND hWnd = wnd->hwnd;
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
}

void a3dSetFocus(A3D_WND* wnd)
{
	HWND hWnd = wnd->hwnd;
	SetFocus(hWnd);
}

bool a3dGetFocus(A3D_WND* wnd)
{
	HWND hWnd = wnd->hwnd;
	return GetFocus() == hWnd;
}

void a3dCharSync(A3D_WND* wnd)
{
	// seams to be handled by OS
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
	A3D_WND* wnd = (A3D_WND*)cookie;

	BITMAPV5HEADER bi;
	bi.bV5Size = sizeof(BITMAPV5HEADER);
	bi.bV5Width = w;
	bi.bV5Height = -h;
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask = 0x00FF0000;
	bi.bV5GreenMask = 0x0000FF00;
	bi.bV5BlueMask = 0x000000FF;
	bi.bV5AlphaMask = 0xFF000000;

	HDC hdc;
	hdc = GetDC(NULL);

	uint32_t* buf = 0;

	HBITMAP hBitmap;
	hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
		(void **)&buf, NULL, (DWORD)0);

	ReleaseDC(0, hdc);

	Convert_UI32_AARRGGBB(buf,f,w,h,data,palsize,palbuf);

	HBITMAP hMonoBitmap = CreateBitmap(w, h, 1, 1, NULL);

	ICONINFO ii;
	ii.fIcon = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmMask = hMonoBitmap;
	ii.hbmColor = hBitmap;

	HICON hIcon = CreateIconIndirect(&ii);

	DeleteObject(hBitmap);
	DeleteObject(hMonoBitmap);

	HICON hSmall, hBig;

	HWND hWnd = wnd->hwnd;
	hSmall = (HICON)SendMessage(hWnd, WM_SETICON, 0, (LPARAM)hIcon);
	hBig = (HICON)SendMessage(hWnd, WM_SETICON, 1, (LPARAM)hIcon);

	if (hSmall)
		DestroyIcon(hSmall);

	if (hBig && hBig!=hSmall)
		DestroyIcon(hBig);
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

int a3dListDir(const char* dir_path, bool(*cb)(A3D_DirItem item, const char* name, void* cookie), void* cookie)
{
	WIN32_FIND_DATAA wfd;

	char patt[1024];
	snprintf(patt, 1023, "%s/*", dir_path);

	HANDLE h = FindFirstFileA(patt, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	if (!h)
		return 0;

	int num = 0;

	do
	{
		num++;
		A3D_DirItem item = wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? A3D_DIRECTORY : A3D_FILE;
		if (!cb(item, wfd.cFileName, cookie))
			break;
	} while (FindNextFileA(h, &wfd));

	FindClose(h);
	return num;
}

bool a3dSetCurDir(const char* dir_path)
{
	return SetCurrentDirectoryA(dir_path);
}

bool a3dGetCurDir(char* dir_path, int size)
{
	int len = (int)GetCurrentDirectoryA(size, dir_path);
	if (len + 1 < size)
	{
		dir_path[len] = '\\';
		dir_path[len+1] = 0;
	}
	return len > 0 && len + 1 < size;
}

struct A3D_THREAD
{
	HANDLE th;

	void* (*entry)(void*);
	void* arg;

	static DWORD WINAPI wrap(LPVOID p)
	{
		A3D_THREAD* t = (A3D_THREAD*)p;
		t->arg = t->entry(t->arg);
		return 0;
	}
};

A3D_THREAD* a3dCreateThread(void* (*entry)(void*), void* arg)
{
	HANDLE th;
	DWORD id;

	A3D_THREAD* t = (A3D_THREAD*)malloc(sizeof(A3D_THREAD));
	t->arg = arg;
	t->entry = entry;
	th = CreateThread(0, 0, A3D_THREAD::wrap, arg, 0, &id);
	if (!th)
	{
		free(t);
		return 0;
	}
	t->th = th;
	return t;
}

void* a3dWaitForThread(A3D_THREAD* thread)
{
	WaitForSingleObject(thread->th,INFINITE);
	CloseHandle(thread->th);
	void* ret = thread->arg;
	free(thread);
	return ret;
}

struct A3D_MUTEX
{
	CRITICAL_SECTION mu;
};

A3D_MUTEX* a3dCreateMutex()
{
	A3D_MUTEX* m = (A3D_MUTEX*)malloc(sizeof(A3D_MUTEX));
	InitializeCriticalSection(&m->mu);
	return m;
}

void a3dDeleteMutex(A3D_MUTEX* mutex)
{
	DeleteCriticalSection(&mutex->mu);
	free(mutex);
}

void a3dMutexLock(A3D_MUTEX* mutex)
{
	EnterCriticalSection(&mutex->mu);
}

void a3dMutexUnlock(A3D_MUTEX* mutex)
{
	LeaveCriticalSection(&mutex->mu);
}

#if 0 
void a3dSetPtyVT(A3D_PTY* pty, A3D_VT* vt)
{
	pty->vt = vt;
}

A3D_VT* a3dGetPtyVT(A3D_PTY* pty)
{
	return pty->vt;
}

A3D_PTY* a3dOpenPty(int w, int h, const char* path, char* const argv[], char* const envp[])
{
	// CreateProcess, pipe i/o
	// instead of ioctl: 
	// client app should create a hidden window having class name "<PID>_IOCTL" and any tittle 
	// host app will send to it WM_COPYDATA message with {int signal; int data_size, uint8_t[data_size]}
	// if client does not create that window - it will not be notified about win size changes :)
	
	return 0;

	/*
	// TODO: must be safe even when called some pty callback!

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
		//error
        if (pty_fd>=0)
            close(pty_fd);
		return 0;
	}

	// parent

	A3D_PTY* pty = (A3D_PTY*)malloc(sizeof(A3D_PTY));
	pty->vt = 0;
	pty->next = 0;
	pty->prev = tail_pty;
	if (tail_pty)
		tail_pty->next = pty;
	else
		head_pty = pty;
	tail_pty = pty;
	
	pty->fd = pty_fd;
	pty->pid = pid;

	if (pty_poll)
		free(pty_poll);
	pty_poll = 0;

	pty_num++;

	return pty;
	*/
}

int a3dReadPTY(A3D_PTY* pty, void* buf, size_t size)
{
	return 0;
	//return read(pty->fd, buf, size);
}

int a3dWritePTY(A3D_PTY* pty, const void* buf, size_t size)
{
	return 0;
	//return write(pty->fd, buf, size);
}

void a3dResizePTY(A3D_PTY* pty, int w, int h)
{
	/*
    // recalc new vt w,h
    struct winsize ws;
    ws.ws_col = w;
    ws.ws_row = h;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    ioctl(pty->fd, TIOCSWINSZ, &ws);
	*/
}

void a3dClosePTY(A3D_PTY* pty)
{
	// TODO: must be safe even if called from this or another pty callback!
	/*

	close(pty->fd);

	int stat;
	waitpid(pty->pid, &stat, 0);

	if (pty->prev)
		pty->prev->next = pty->next;
	else
		head_pty = pty->next;

	if (pty->next)
		pty->next->prev = pty->prev;
	else
		tail_pty = pty->prev;

	free(pty);

	pty_num--;

	if (pty_poll)
		free(pty_poll);
	pty_poll = 0;
	*/
}

#endif

#endif // USE_SDL
#endif // __WIN32__