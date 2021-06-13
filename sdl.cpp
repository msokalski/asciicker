#ifdef USE_SDL

#include <sys/stat.h>

#include <time.h>
#include <malloc.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "rgba8.h"
#include "platform.h"

#include "upng.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

typedef void SDL_WINDOW;
typedef void SDL_GL_CTX;

A3D_WND* wnd_head = 0;
A3D_WND* wnd_tail = 0;

struct A3D_WND
{
	A3D_WND* prev;
	A3D_WND* next;

	SDL_Window* win;
	SDL_GLContext rc;

	bool mapped;
	void* cookie;

	PlatformInterface platform_api;
};

A3D_WND* a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd, A3D_WND* share)
{
	A3D_WND* wnd = (A3D_WND*)malloc(sizeof(A3D_WND));

	wnd->prev = wnd_tail;
	wnd->next = 0;
	if (wnd_tail)
		wnd_tail->next = wnd;
	else
		wnd_head = wnd;
	wnd_tail = wnd;

	// init wnd fields!
	wnd->platform_api = *pi;
	wnd->cookie = 0;
	wnd->mapped = false;

	SDL_SetWindowData(wnd->win, "a3d", wnd);

	if (share)
	{
		SDL_GL_MakeCurrent(share->win, share->rc);
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	}

	int x=0, y=0, w = 800, h = 600;
	if (gd->wnd_xywh)
	{
		x = gd->wnd_xywh[0];
		y = gd->wnd_xywh[1];
		w = gd->wnd_xywh[2];
		h = gd->wnd_xywh[3];
	}

	wnd->win = SDL_CreateWindow("ASCIIID SDL", x, y, w, h, 
		SDL_WINDOW_ALLOW_HIGHDPI | 
		SDL_WINDOW_OPENGL | 
		SDL_WINDOW_HIDDEN);

	wnd->rc = SDL_GL_CreateContext(wnd->win);

	if (share)
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

	// note
	// wnd->rc is now current

	if (wnd->platform_api.init)
		wnd->platform_api.init(wnd);

	SDL_GL_GetDrawableSize(wnd->win, &w, &h);
	
	if (wnd->platform_api.resize)
		wnd->platform_api.resize(wnd, w,h);

	return wnd;
}

void a3dClose(A3D_WND* wnd)
{
	SDL_GL_DeleteContext(wnd->rc);
	SDL_DestroyWindow(wnd->win);

	if (wnd->prev)
		wnd->prev->next = wnd->next;
	else
		wnd_head = wnd->next;

	if (wnd->next)
		wnd->next->prev = wnd->prev;
	else
		wnd_tail = wnd->prev;

	free(wnd);
}

WndMode a3dGetRect(A3D_WND* wnd, int* xywh, int* client_wh)
{
	if (client_wh)
		SDL_GL_GetDrawableSize(wnd->win, client_wh+0, client_wh+1);
	if (xywh)
	{
		SDL_GetWindowPosition(wnd->win, xywh + 0, xywh + 1);
		SDL_GetWindowSize(wnd->win, xywh + 2, xywh + 3);
	}
	return WndMode::A3D_WND_NORMAL; // not critical
}

bool a3dSetRect(A3D_WND* wnd, const int* xywh, WndMode wnd_mode)
{
	SDL_SetWindowPosition(wnd->win, xywh[0], xywh[1]);
	SDL_SetWindowSize(wnd->win, xywh[2], xywh[3]);
	return true;
}

bool a3dIsMaximized(A3D_WND* wnd)
{
	return false; // not critical
}

void a3dSetVisible(A3D_WND* wnd, bool set)
{
	wnd->mapped = set;

	if (set)
		SDL_ShowWindow(wnd->win);
	else
		SDL_HideWindow(wnd->win);
}

bool a3dGetVisible(A3D_WND* wnd)
{
	return wnd->mapped;
}

void a3dSetCookie(A3D_WND* wnd, void* cookie)
{
	wnd->cookie = cookie;
}

void* a3dGetCookie(A3D_WND* wnd)
{
	return wnd->cookie;
}

KeyInfo SDL2A3D[/*128*/] = 
{
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,

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

	A3D_1,
	A3D_2,
	A3D_3,
	A3D_4,
	A3D_5,
	A3D_6,
	A3D_7,
	A3D_8,
	A3D_9,
	A3D_0,

    A3D_ENTER,
    A3D_ESCAPE,
    A3D_BACKSPACE,
    A3D_TAB,
    A3D_SPACE,

    A3D_OEM_MINUS,
    A3D_OEM_PLUS,
    A3D_OEM_OPEN,
    A3D_OEM_CLOSE,
    A3D_OEM_BACKSLASH,
    A3D_NONE, //NONUSHASH
    A3D_OEM_COLON,
    A3D_OEM_QUOTATION,
    A3D_OEM_TILDE,
    A3D_OEM_COMMA,
    A3D_OEM_PERIOD,
    A3D_OEM_SLASH,

    A3D_CAPSLOCK,

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

    A3D_PRINT,
    A3D_SCROLLLOCK,
    A3D_PAUSE,
    A3D_INSERT,

    A3D_HOME,
    A3D_PAGEUP,
    A3D_DELETE,
    A3D_END,
    A3D_PAGEDOWN,
    A3D_RIGHT,
    A3D_LEFT,
    A3D_DOWN,
    A3D_UP,

    A3D_NONE, //NUMLOCKCLEAR

    A3D_NUMPAD_DIVIDE,
    A3D_NUMPAD_MULTIPLY,
    A3D_NUMPAD_SUBTRACT,
    A3D_NUMPAD_ADD,
    A3D_NUMPAD_ENTER,
	A3D_NUMPAD_1,
	A3D_NUMPAD_2,
	A3D_NUMPAD_3,
	A3D_NUMPAD_4,
	A3D_NUMPAD_5,
	A3D_NUMPAD_6,
	A3D_NUMPAD_7,
	A3D_NUMPAD_8,
	A3D_NUMPAD_9,
	A3D_NUMPAD_0,
	A3D_NUMPAD_DECIMAL,

    A3D_NONE, //NONUSBACKSLASH
    A3D_APPS,
    A3D_NONE, //POWER

    A3D_NONE, //EQUALS
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

	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE,
	A3D_NONE
};

void a3dLoop()
{
	bool Running = true;
	while (Running)
	{
		SDL_Event Event;
		while (SDL_PollEvent(&Event))
		{
			switch (Event.type)
			{
				case SDL_QUIT: Running = 0; break;
				
				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					SDL_KeyboardEvent* ev = &Event.key;

					if (ev->keysym.scancode < 0 || ev->keysym.scancode >= 128)
						break; 

					KeyInfo ki = SDL2A3D[ev->keysym.scancode];
					if (ki == A3D_NONE)
						break;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (!wnd)
						break;

					if (wnd->platform_api.keyb_key)
					{
						wnd->platform_api.keyb_key(wnd,ki,Event.type==SDL_KEYDOWN);
					}

					/*
					if (wnd->platform_api.keyb_char && Event.type==SDL_KEYDOWN)
					{
						wnd->platform_api.keyb_char(wnd,(wchar_t)Event.key.keysym.unicode);
					}
					*/

					break;
				}

				case SDL_MOUSEMOTION:
				{
					SDL_MouseMotionEvent* ev = &Event.motion;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (!wnd || !wnd->platform_api.mouse)
						break;

					wnd->platform_api.mouse(wnd,ev->x,ev->y,MOVE);
					break;					
				}

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				{
					SDL_MouseButtonEvent* ev = &Event.button;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (!wnd || !wnd->platform_api.mouse)
						break;

					wnd->platform_api.mouse(wnd,ev->x,ev->y,
						Event.type == SDL_MOUSEBUTTONDOWN ? LEFT_DN: LEFT_UP );
					break;											
				}
				case SDL_MOUSEWHEEL:
					break;
			}

			if (Event.type == SDL_KEYDOWN)
			{
				switch (Event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					Running = 0;
					break;
				default:
					break;
				}
			}
			else if (Event.type == SDL_QUIT)
			{
				Running = 0;
			}
		}

		A3D_WND* wnd = wnd_head;
		A3D_WND* swap = 0;
		A3D_WND* share = 0;

		while (wnd)
		{
			if (wnd->platform_api.render && wnd->mapped)
			{
				if (swap)
				{
					SDL_GL_SetSwapInterval(0);
					SDL_GL_SwapWindow(swap->win);
				}

				SDL_GL_MakeCurrent(wnd->win, wnd->rc);
				wnd->platform_api.render(wnd);
				swap = wnd;
			}

			wnd = wnd->next;
		}

		if (swap)
		{
			SDL_GL_SetSwapInterval(0);
			SDL_GL_SwapWindow(swap->win);
		}
	}
}

bool a3dSetIcon(A3D_WND* wnd, const char* path)
{
	// not critical
	return false;
}

bool a3dSetIconData(A3D_WND* wnd, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
{
	// not critical
	return false;
}

void a3dSetTitle(A3D_WND* wnd, const char* utf8_name)
{
	SDL_SetWindowTitle(wnd->win, utf8_name);
}

int a3dGetTitle(A3D_WND* wnd, char* utf8_name, int size)
{
	const char* name = SDL_GetWindowTitle(wnd->win);
	int len = strlen(name);
	len = len < size-1 ? len : size-1;
	if (utf8_name && size>0)
	{
		if (len>0)
			memcpy(utf8_name,name,len);
		utf8_name[len]=0;
	}

	return len;
}


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

bool a3dGetKeyb(A3D_WND* wnd, KeyInfo ki)
{
	return false; // later
}

void a3dPushContext(A3D_PUSH_CONTEXT* ctx)
{
	ctx->data[0] = (void*)SDL_GL_GetCurrentWindow();
	ctx->data[1] = SDL_GL_GetCurrentContext();
}

void a3dPopContext(const A3D_PUSH_CONTEXT* ctx)
{
	SDL_GL_MakeCurrent((SDL_Window*)ctx->data[0], (SDL_GLContext)ctx->data[1]);
}

void a3dSwitchContext(const A3D_WND* wnd)
{
	SDL_GL_MakeCurrent(wnd->win, wnd->rc);
}

#if defined __linux__ || defined __APPLE__
#include <dirent.h>
#include <unistd.h>

uint64_t a3dGetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int a3dListDir(const char* dir_path, bool(*cb)(A3D_DirItem item, const char* name, void* cookie), void* cookie)
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
			snprintf(fullpath, 4095, "%s/%s", dir_path, dir->d_name);
			struct stat s;
			if (0 != lstat(fullpath, &s))
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

		if (cb && !cb(item, dir->d_name, cookie))
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
		if (len < size - 1)
		{
			dir_path[len] = '/';
			dir_path[len + 1] = 0;
		}
	}
	return true;
}
#else
#include <windows.h>
#pragma comment(lib,"SDL2.lib")

LARGE_INTEGER coarse_perf, timer_freq;
uint64_t coarse_micro;

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
		dir_path[len + 1] = 0;
	}
	return len > 0 && len + 1 < size;
}

#endif // windows
#endif // USE_SDL


