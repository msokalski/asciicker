#pragma once

#include <stdint.h>

/*
struct AudioDesc
{
	int bits;
	int channels;
	int frequency;
};
*/

struct A3D_WND;

struct A3D_PUSH_CONTEXT
{
	void* data[3];
};

void a3dPushContext(A3D_PUSH_CONTEXT* ctx);
void a3dPopContext(const A3D_PUSH_CONTEXT* ctx);
void a3dSwitchContext(const A3D_WND* wnd);

enum WndMode
{
	A3D_WND_CURRENT = 0,
	A3D_WND_NORMAL,
	A3D_WND_FRAMELESS,
	A3D_WND_FULLSCREEN,
};

struct GraphicsDesc
{
	enum FLAGS
	{
		DEBUG_CONTEXT = 1,
		DOUBLE_BUFFER = 2,
	};

	int flags;

	int version[2]; // [0]:Major, [1]:Minor

	int color_bits; // (incl. alpha)
	int alpha_bits;
	int depth_bits;
	int stencil_bits;

	// intial wnd spec
	// can be null for defaults: [0,0,800,600], A3D_WND_NORMAL
	const int* wnd_xywh;
	WndMode wnd_mode;
};

enum MouseInfo
{
	// messages
	MOVE = 1,
	LEFT_DN = 2,
	LEFT_UP = 3,
	RIGHT_DN = 4,
	RIGHT_UP = 5,
	MIDDLE_DN = 6,
	MIDDLE_UP = 7,
	WHEEL_UP = 8,
	WHEEL_DN = 9,
	ENTER = 10,
	LEAVE = 11,

	// but flags
	LEFT = 0x10,
	RIGHT = 0x20,
	MIDDLE = 0x40,

	INSIDE = 0x80
};

enum KeyInfo
{
	A3D_NONE = 0,

	A3D_BACKSPACE,
	A3D_TAB,
	A3D_ENTER,

	A3D_PAUSE,
	A3D_ESCAPE,

	A3D_SPACE,
	A3D_PAGEUP,
	A3D_PAGEDOWN,
	A3D_END,
	A3D_HOME,
	A3D_LEFT,
	A3D_UP,
	A3D_RIGHT,
	A3D_DOWN,

	A3D_PRINT,
	A3D_INSERT,
	A3D_DELETE,

	A3D_0,
	A3D_1,
	A3D_2,
	A3D_3,
	A3D_4,
	A3D_5,
	A3D_6,
	A3D_7,
	A3D_8,
	A3D_9,

	A3D_A,
	A3D_B,
	A3D_C,
	A3D_D,
	A3D_E,
	A3D_F,
	A3D_G,
	A3D_H,
	A3D_I,
	A3D_J,
	A3D_K,
	A3D_L,
	A3D_M,
	A3D_N,
	A3D_O,
	A3D_P,
	A3D_Q,
	A3D_R,
	A3D_S,
	A3D_T,
	A3D_U,
	A3D_V,
	A3D_W,
	A3D_X,
	A3D_Y,
	A3D_Z,

	A3D_LWIN,
	A3D_RWIN,
	A3D_APPS,

	A3D_NUMPAD_0,
	A3D_NUMPAD_1,
	A3D_NUMPAD_2,
	A3D_NUMPAD_3,
	A3D_NUMPAD_4,
	A3D_NUMPAD_5,
	A3D_NUMPAD_6,
	A3D_NUMPAD_7,
	A3D_NUMPAD_8,
	A3D_NUMPAD_9,
	A3D_NUMPAD_MULTIPLY,
	A3D_NUMPAD_DIVIDE,
	A3D_NUMPAD_ADD,
	A3D_NUMPAD_SUBTRACT,
	A3D_NUMPAD_DECIMAL,
	A3D_NUMPAD_ENTER,

	A3D_F1,
	A3D_F2,
	A3D_F3,
	A3D_F4,
	A3D_F5,
	A3D_F6,
	A3D_F7,
	A3D_F8,
	A3D_F9,
	A3D_F10,
	A3D_F11,
	A3D_F12,
	A3D_F13,
	A3D_F14,
	A3D_F15,
	A3D_F16,
	A3D_F17,
	A3D_F18,
	A3D_F19,
	A3D_F20,
	A3D_F21,
	A3D_F22,
	A3D_F23,
	A3D_F24,

	A3D_CAPSLOCK,
	A3D_NUMLOCK,
	A3D_SCROLLLOCK,

	A3D_LSHIFT,
	A3D_RSHIFT,
	A3D_LCTRL,
	A3D_RCTRL,
	A3D_LALT,
	A3D_RALT,

	A3D_OEM_COLON,		// ';:' for US
	A3D_OEM_PLUS,		// '=+' any country
	A3D_OEM_COMMA,		// ',<' any country
	A3D_OEM_MINUS,		// '-_' any country
	A3D_OEM_PERIOD,		// '.>' any country
	A3D_OEM_SLASH,		// '/?' for US
	A3D_OEM_TILDE,		// '`~' for US

	A3D_OEM_OPEN,       //  '[{' for US
	A3D_OEM_CLOSE,      //  ']}' for US
	A3D_OEM_BACKSLASH,  //  '\|' for US
	A3D_OEM_QUOTATION,  //  ''"' for US

	A3D_MAPEND,
	A3D_AUTO_REPEAT = 256
};

struct PlatformInterface
{
	void(*init)(A3D_WND* wnd);
	void(*render)(A3D_WND* wnd);
	void(*resize)(A3D_WND* wnd, int w, int h);
	void(*close)(A3D_WND* wnd);
	void(*keyb_key)(A3D_WND* wnd, KeyInfo vk, bool down);
	void(*keyb_char)(A3D_WND* wnd, wchar_t ch);
	void(*keyb_focus)(A3D_WND* wnd, bool set);
	void(*mouse)(A3D_WND* wnd, int x, int y, MouseInfo mi);

	// asset loader
	void(*image)(void* cookie, int width, int height, int channels, int depth, void* data);
	void(*sound)(void* cookie, int samples, int channels, int depth, void* data);

	// todo:
	//void(*audio)(int samples, void* buffer);
};

A3D_WND* a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd, A3D_WND* share);
void a3dClose(A3D_WND* wnd); // if PlatformInterface::close==null it is called automaticaly when user closes window

void a3dSetCookie(A3D_WND* wnd, void* cookie);
void* a3dGetCookie(A3D_WND* wnd);

struct LoopInterface
{
	void(*gpad_mount)(const char* name, int axes, int buttons, const uint8_t mapping[]);
	void(*gpad_unmount)();
	void(*gpad_button)(int b, int16_t pos);
	void(*gpad_axis)(int a, int16_t pos);
};

bool a3dGetGamePad();
bool a3dGetGamePadButton(int b);
int16_t a3dGetGamePadAxis(int a);

void a3dLoop(const LoopInterface* li=0);

//void a3dSwapBuffers();
uint64_t a3dGetTime(); // in microsecs, wraps every 584542 years

void a3dSetTitle(A3D_WND* wnd, const char* utf8_name);
int a3dGetTitle(A3D_WND* wnd, char* utf8_name, int size);

void a3dSetVisible(A3D_WND* wnd, bool set);
bool a3dGetVisible(A3D_WND* wnd);

bool a3dIsMaximized(A3D_WND* wnd);

// resize
WndMode a3dGetRect(A3D_WND* wnd, int* xywh, int* client_wh);
bool a3dSetRect(A3D_WND* wnd, const int* xywh, WndMode wnd_mode);

// mouse
MouseInfo a3dGetMouse(A3D_WND* wnd, int* x, int* y); // returns but flags, mouse wheel has no state

// keyb_key
bool a3dGetKeyb(A3D_WND* wnd, KeyInfo ki); // return true if vk is down, keyb_char has no state

// keyb_focus
bool a3dGetFocus(A3D_WND* wnd);
void a3dSetFocus(A3D_WND* wnd);

void a3dCharSync(A3D_WND* wnd); // call in case of internal widget input re-focusing

enum A3D_ImageFormat
{
	A3D_NULL,
	A3D_RGB8,
	A3D_RGB16,
	A3D_RGBA8,
	A3D_RGBA16,
	A3D_LUMINANCE1,
	A3D_LUMINANCE2,
	A3D_LUMINANCE4,
	A3D_LUMINANCE8,
	A3D_LUMINANCE16,
	A3D_LUMINANCE_ALPHA8,
	A3D_LUMINANCE_ALPHA16,
	A3D_INDEX1_RGB,
	A3D_INDEX2_RGB,
	A3D_INDEX4_RGB,
	A3D_INDEX8_RGB,
	A3D_INDEX1_RGBA,
	A3D_INDEX2_RGBA,
	A3D_INDEX4_RGBA,
	A3D_INDEX8_RGBA,
};

bool a3dLoadImage(const char* path, void* cookie, void(*cb)(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf));

bool a3dSetIcon(A3D_WND* wnd, const char* path);
bool a3dSetIconData(A3D_WND* wnd, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf);

enum A3D_DirItem
{
	A3D_DIRECTORY,
	A3D_FILE
};

int a3dListDir(const char* dir_path, bool (*cb)(A3D_DirItem item, const char* name, void* cookie), void* cookie);

bool a3dSetCurDir(const char* dir_path);
bool a3dGetCurDir(char* dir_path, int size);

struct A3D_VT;
A3D_VT* a3dCreateVT(int w, int h, const char* path, char* const argv[], char* const envp[]);
void a3dDestroyVT(A3D_VT* vt);

// should be replaced with a3dWriteKey(ki) a3dWriteChar(ch) and a3dMouse(x,y,mi)
int a3dWriteVT(A3D_VT* vt, const void* buf, size_t size);

bool a3dGetVTCursorsMode(A3D_VT* vt);

// TESTING!
int a3dDumpVT(A3D_VT* vt, int tw, int th);

// simple thread api
struct A3D_THREAD;
A3D_THREAD* a3dCreateThread(void* (*entry)(void*), void*);
void* a3dWaitForThread(A3D_THREAD* thread);

struct A3D_MUTEX;
A3D_MUTEX* a3dCreateMutex();
void a3dDeleteMutex(A3D_MUTEX* mutex);
void a3dMutexLock(A3D_MUTEX* mutex);
void a3dMutexUnlock(A3D_MUTEX* mutex);

struct A3D_PTY;
A3D_PTY* a3dOpenPty(int w, int h, const char* path, char* const argv[], char* const envp[]);
int a3dReadPTY(A3D_PTY* pty, void* buf, size_t size);
int a3dWritePTY(A3D_PTY* pty, const void* buf, size_t size);
void a3dResizePTY(A3D_PTY* pty, int w, int h);
void a3dUnblockPTY(A3D_PTY* pty);
void a3dClosePTY(A3D_PTY* pty);

