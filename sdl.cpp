
#ifdef USE_SDL

#include <sys/stat.h>

#include <time.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "rgba8.h"
#include "platform.h"

#include "upng.h"

#ifdef _WIN32
#include <SDL.h>
#include <SDL_opengl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif
#ifdef __linux__
#include <GL/gl.h>
#elif defined(__APPLE__) 
#include <OpenGL/gl.h>
#endif

A3D_WND* wnd_head = 0;
A3D_WND* wnd_tail = 0;

struct GlobalSDL
{
	GlobalSDL()
	{
		gamepad = 0;
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
		//SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
		SDL_Init(SDL_INIT_EVERYTHING);
	}

	~GlobalSDL()
	{
		if (gamepad)
		{
			SDL_GameControllerClose(gamepad);
			gamepad = 0;
		}
		SDL_Quit();
	}

	SDL_GameController* gamepad;
};

static GlobalSDL sdl;

struct A3D_WND
{
	A3D_WND* prev;
	A3D_WND* next;

	SDL_Window* win;
	SDL_GLContext rc;

	bool mapped;
	void* cookie;
	int captured;

	PlatformInterface platform_api;
};

A3D_WND* a3dOpen(const PlatformInterface* pi, const GraphicsDesc* gd, A3D_WND* share)
{
	struct PUSH
	{
		PUSH()
		{
			rc = SDL_GL_GetCurrentContext();
			dr = SDL_GL_GetCurrentWindow();
		}

		~PUSH()
		{
			if (dr && rc)
				SDL_GL_MakeCurrent(dr, rc);
		}

		SDL_Window* dr;
		SDL_GLContext rc;
	} push;

	A3D_WND* wnd = (A3D_WND*)malloc(sizeof(A3D_WND));

	wnd->captured = 0;
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

	/*
		Buzz:
		SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);		
		SDL_Haptic* haptic = SDL_HapticOpenFromJoystick(joy);
		if (SDL_HapticRumbleSupported(haptic))
		{
			SDL_HapticRumbleInit(haptic);
			SDL_HapticRumblePlay(haptic, 1.0, 3000);
			// SDL_HapticRumbleStop(haptic);
		}
	*/

	if (share)
	{
		SDL_GL_MakeCurrent(share->win, share->rc);
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	}

	int x=100, y=100, w = 800, h = 600;
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
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_HIDDEN);

	SDL_SetWindowData(wnd->win, "a3d", wnd);

	if (0)
	{
		// let SDL choose rgba sizes
		// stressing it too much results in no context!
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	}

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, gd->depth_bits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, gd->stencil_bits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, gd->flags & GraphicsDesc::FLAGS::DOUBLE_BUFFER ? 1:0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG |
		(gd->flags & GraphicsDesc::DEBUG_CONTEXT ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gd->version[0]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gd->version[1]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	wnd->rc = SDL_GL_CreateContext(wnd->win);

	if (share)
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

	// note
	// wnd->rc is now current

	SDL_GL_GetDrawableSize(wnd->win, &w, &h);

	if (wnd->platform_api.init)
		wnd->platform_api.init(wnd);

	if (wnd->platform_api.resize)
		wnd->platform_api.resize(wnd, w,h);

	return wnd;
}

void a3dClose(A3D_WND* wnd)
{
	struct PUSH
	{
		PUSH()
		{
			rc = SDL_GL_GetCurrentContext();
			dr = SDL_GL_GetCurrentWindow();
		}

		~PUSH()
		{
			if (dr && rc)
				SDL_GL_MakeCurrent(dr, rc);
		}

		SDL_Window* dr;
		SDL_GLContext rc;

	} push;

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

	uint32_t f = SDL_GetWindowFlags(wnd->win);
	if (f & SDL_WINDOW_FULLSCREEN)
		return WndMode::A3D_WND_FULLSCREEN;
	return WndMode::A3D_WND_NORMAL; // not critical
}

bool a3dSetRect(A3D_WND* wnd, const int* xywh, WndMode wnd_mode)
{
	if (xywh)
	{
		SDL_SetWindowPosition(wnd->win, xywh[0], xywh[1]);
		SDL_SetWindowSize(wnd->win, xywh[2], xywh[3]);
	}

	if (wnd_mode == WndMode::A3D_WND_FULLSCREEN)
		SDL_SetWindowFullscreen(wnd->win, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
	if (wnd_mode != WndMode::A3D_WND_CURRENT)
		SDL_SetWindowFullscreen(wnd->win, 0);

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

int A3D2SDL[] = 
{
	SDL_SCANCODE_UNKNOWN,

	SDL_SCANCODE_BACKSPACE,
	SDL_SCANCODE_TAB,
	SDL_SCANCODE_RETURN,

	SDL_SCANCODE_PAUSE,
	SDL_SCANCODE_ESCAPE,

	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_PAGEUP,
	SDL_SCANCODE_PAGEDOWN,
	SDL_SCANCODE_END,
	SDL_SCANCODE_HOME,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_UP,
	SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_DOWN,

	SDL_SCANCODE_PRINTSCREEN,
	SDL_SCANCODE_INSERT,
	SDL_SCANCODE_DELETE,

	SDL_SCANCODE_0,
	SDL_SCANCODE_1,
	SDL_SCANCODE_2,
	SDL_SCANCODE_3,
	SDL_SCANCODE_4,
	SDL_SCANCODE_5,
	SDL_SCANCODE_6,
	SDL_SCANCODE_7,
	SDL_SCANCODE_8,
	SDL_SCANCODE_9,

	SDL_SCANCODE_A,
	SDL_SCANCODE_B,
	SDL_SCANCODE_C,
	SDL_SCANCODE_D,
	SDL_SCANCODE_E,
	SDL_SCANCODE_F,
	SDL_SCANCODE_G,
	SDL_SCANCODE_H,
	SDL_SCANCODE_I,
	SDL_SCANCODE_J,
	SDL_SCANCODE_K,
	SDL_SCANCODE_L,
	SDL_SCANCODE_M,
	SDL_SCANCODE_N,
	SDL_SCANCODE_O,
	SDL_SCANCODE_P,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_R,
	SDL_SCANCODE_S,
	SDL_SCANCODE_T,
	SDL_SCANCODE_U,
	SDL_SCANCODE_V,
	SDL_SCANCODE_W,
	SDL_SCANCODE_X,
	SDL_SCANCODE_Y,
	SDL_SCANCODE_Z,

	SDL_SCANCODE_LGUI,
	SDL_SCANCODE_RGUI,
	SDL_SCANCODE_APPLICATION,

	SDL_SCANCODE_KP_0,
	SDL_SCANCODE_KP_1,
	SDL_SCANCODE_KP_2,
	SDL_SCANCODE_KP_3,
	SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5,
	SDL_SCANCODE_KP_6,
	SDL_SCANCODE_KP_7,
	SDL_SCANCODE_KP_8,
	SDL_SCANCODE_KP_9,
	SDL_SCANCODE_KP_MULTIPLY,
	SDL_SCANCODE_KP_DIVIDE,
	SDL_SCANCODE_KP_PLUS,
	SDL_SCANCODE_KP_MINUS,
	SDL_SCANCODE_KP_DECIMAL,
	SDL_SCANCODE_KP_ENTER,

	SDL_SCANCODE_F1,
	SDL_SCANCODE_F2,
	SDL_SCANCODE_F3,
	SDL_SCANCODE_F4,
	SDL_SCANCODE_F5,
	SDL_SCANCODE_F6,
	SDL_SCANCODE_F7,
	SDL_SCANCODE_F8,
	SDL_SCANCODE_F9,
	SDL_SCANCODE_F10,
	SDL_SCANCODE_F11,
	SDL_SCANCODE_F12,
	SDL_SCANCODE_F13,
	SDL_SCANCODE_F14,
	SDL_SCANCODE_F15,
	SDL_SCANCODE_F16,
	SDL_SCANCODE_F17,
	SDL_SCANCODE_F18,
	SDL_SCANCODE_F19,
	SDL_SCANCODE_F20,
	SDL_SCANCODE_F21,
	SDL_SCANCODE_F22,
	SDL_SCANCODE_F23,
	SDL_SCANCODE_F24,

	SDL_SCANCODE_CAPSLOCK,
	SDL_SCANCODE_NUMLOCKCLEAR,
	SDL_SCANCODE_SCROLLLOCK,

	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_RSHIFT,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_RCTRL,
	SDL_SCANCODE_LALT,
	SDL_SCANCODE_RALT,

	SDL_SCANCODE_SEMICOLON,
	SDL_SCANCODE_EQUALS,
	SDL_SCANCODE_COMMA,
	SDL_SCANCODE_MINUS,
	SDL_SCANCODE_PERIOD,
	SDL_SCANCODE_SLASH,
	SDL_SCANCODE_GRAVE,

	SDL_SCANCODE_LEFTBRACKET,
	SDL_SCANCODE_RIGHTBRACKET,
	SDL_SCANCODE_BACKSLASH,
	SDL_SCANCODE_APOSTROPHE,

	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN
};

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

    A3D_NUMLOCK,

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

void a3dLoop(const LoopInterface* li)
{
	SDL_GameControllerEventState(SDL_ENABLE);
	// for all created wnds, force size notifications
	A3D_WND* wnd = wnd_head;
	while (wnd)
	{
		if (wnd->platform_api.resize)
		{
			int w, h;
			SDL_GL_GetDrawableSize(wnd->win, &w, &h);
			wnd->platform_api.resize(wnd, w, h);
		}

		wnd = wnd->next;
	}

	bool Running = true;
	while (Running)
	{
		SDL_Event Event;
		while (SDL_PollEvent(&Event))
		{
			const char* ev_name = 0;
			#define CASE(t) case t: ev_name = ev_name ? ev_name : #t;

			switch (Event.type)
			{
				case SDL_QUIT: Running = 0; break;

				CASE(SDL_CONTROLLERDEVICEADDED)
				{
					SDL_ControllerDeviceEvent* ev = &Event.cdevice;
					//printf("%s %d\n", ev_name, ev->which);
					if (!sdl.gamepad)
						sdl.gamepad = SDL_GameControllerOpen(ev->which);
					if (sdl.gamepad && li && li->gpad_mount)
					{
						int axes = SDL_CONTROLLER_AXIS_MAX;
						int buttons = SDL_CONTROLLER_BUTTON_MAX;
						uint8_t mapping[2*SDL_CONTROLLER_AXIS_MAX + SDL_CONTROLLER_BUTTON_MAX];
						int imap = 0;
						for (int i=0; i<axes; i++)
						{
							uint8_t m = i<6 ? (0<<7 /*to axis*/) | (0<<6 /*no flip*/) | i : 0xFF;

							if (i==4 || i==5)
								mapping[imap++] = 0xFF; // clear negs, sdl knows it is unsigned
							else
								mapping[imap++] = m | (1<<6); // neg in
							mapping[imap++] = m; // pos in
						}
						for (int i=0; i<buttons; i++)
						{
							if (i>=15)
								mapping[imap++] = 0xFF;
							else
								mapping[imap++] = (1<<7 /*to button*/) | (0<<6 /*no flip*/) | i; 
						}

						li->gpad_mount(SDL_GameControllerName(sdl.gamepad), axes, buttons, mapping);
					}
					break;
				}
				
				CASE(SDL_CONTROLLERDEVICEREMAPPED)
				{
					SDL_ControllerDeviceEvent* ev = &Event.cdevice;
					//printf("%s %d\n", ev_name, ev->which);
					break;
				}
				
				CASE(SDL_CONTROLLERDEVICEREMOVED) 
				{
					SDL_ControllerDeviceEvent* ev = &Event.cdevice;
					//printf("%s %d\n", ev_name, ev->which);
					SDL_GameController* gc = SDL_GameControllerFromInstanceID(ev->which);
					if (gc && gc == sdl.gamepad)
					{
						if (li && li->gpad_unmount)
							li->gpad_unmount();
						SDL_GameControllerClose(sdl.gamepad);
						sdl.gamepad = 0;

						// try reconnecting to something else
						int n = SDL_NumJoysticks();
						for (int i = 0; i < n; i++)
						{
							if (SDL_IsGameController(i))
							{
								sdl.gamepad = SDL_GameControllerOpen(i);
								if (sdl.gamepad)
								{
									if (li && li->gpad_mount)
									{
										int axes = SDL_CONTROLLER_AXIS_MAX;
										int buttons = SDL_CONTROLLER_BUTTON_MAX;
										uint8_t mapping[2*SDL_CONTROLLER_AXIS_MAX + SDL_CONTROLLER_BUTTON_MAX];
										int imap = 0;
										for (int i=0; i<axes; i++)
										{
											uint8_t m = i<6 ? (0<<7 /*to axis*/) | (0<<6 /*no flip*/) | i : 0xFF;

											if (i==4 || i==5)
												mapping[imap++] = 0xFF; // clear negs, sdl knows it is unsigned
											else
												mapping[imap++] = m | (1<<6); // neg in
											mapping[imap++] = m; // pos in
										}
										for (int i=0; i<buttons; i++)
										{
											if (i>=15)
												mapping[imap++] = 0xFF;
											else
												mapping[imap++] = (1<<7 /*to button*/) | (0<<6 /*no flip*/) | i; 
										}

										li->gpad_mount(SDL_GameControllerName(sdl.gamepad), axes, buttons, mapping);
									}
									break;
								}
							}
						}
					}
					break;
				}

				CASE(SDL_CONTROLLERAXISMOTION)
				{
					SDL_ControllerAxisEvent* ev = &Event.caxis;
					const char* aname = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)ev->axis);
					//printf("%s %d %d = %f (%s)\n", ev_name, ev->which, ev->axis, ev->value/32767.0f, aname);
					SDL_GameController* gc = SDL_GameControllerFromInstanceID(ev->which);
					if (gc && gc == sdl.gamepad)
					{
						if (li && li->gpad_axis)
							li->gpad_axis(ev->axis, ev->value == -32768 ? -32767 : ev->value);
					}
					break;
				}

				CASE(SDL_CONTROLLERBUTTONDOWN)
				CASE(SDL_CONTROLLERBUTTONUP)
				{
					SDL_ControllerButtonEvent* ev = &Event.cbutton;
					const char* bname = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)ev->button);
					//printf("%s %d %d = %d (%s)\n", ev_name, ev->which, ev->button, ev->state, bname);
					SDL_GameController* gc = SDL_GameControllerFromInstanceID(ev->which);
					if (gc && gc == sdl.gamepad)
					{
						if (li && li->gpad_button)
							li->gpad_button(ev->button, ev->state ? 32767 : 0);
					}
					break;
				}

				/* PS only?
				case SDL_CONTROLLERTOUCHPADDOWN:  
				case SDL_CONTROLLERTOUCHPADMOTION:
				case SDL_CONTROLLERTOUCHPADUP:
				*/

				/* WII only?
				case SDL_CONTROLLERSENSORUPDATE:  
				*/

				case SDL_TEXTINPUT:
				{
					SDL_TextInputEvent* ev = &Event.text;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (wnd && wnd->platform_api.keyb_char)
					{
						// actually we should convert utf8 to wchar_t
						wnd->platform_api.keyb_char(wnd, (wchar_t)ev->text[0]);
					}
					break;
				}
				
				case SDL_WINDOWEVENT:
				{
					SDL_WindowEvent* ev = &Event.window;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (!wnd)
						break;

					switch (ev->event)
					{
						case SDL_WINDOWEVENT_CLOSE:
							if (wnd->platform_api.close)
								wnd->platform_api.close(wnd);
							break;

						case SDL_WINDOWEVENT_ENTER:
						case SDL_WINDOWEVENT_LEAVE:
							if (wnd->platform_api.mouse)
							{
								int x, y;
								int b = SDL_GetMouseState(&x,&y);

								int mi = ev->event == SDL_WINDOWEVENT_ENTER ? ENTER : LEAVE;

								if (b & SDL_BUTTON(SDL_BUTTON_LEFT))
									mi |= LEFT;
								if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE))
									mi |= MIDDLE;
								if (b & SDL_BUTTON(SDL_BUTTON_RIGHT))
									mi |= RIGHT;

								int w, h;
								SDL_GL_GetDrawableSize(wnd->win, &w, &h);
								if (x >= 0 && x < w && y >= 0 && y < h)
									mi |= INSIDE;

								wnd->platform_api.mouse(wnd, x,y, (MouseInfo)mi);
							}

							break;

						case SDL_WINDOWEVENT_FOCUS_GAINED:
						case SDL_WINDOWEVENT_FOCUS_LOST:

							if (wnd->platform_api.keyb_focus)
								wnd->platform_api.keyb_focus(wnd, ev->event == SDL_WINDOWEVENT_FOCUS_GAINED);
							break;

						case SDL_WINDOWEVENT_SIZE_CHANGED:
							if (wnd->platform_api.resize)
							{
								int w, h;
								SDL_GL_GetDrawableSize(wnd->win, &w, &h);
								wnd->platform_api.resize(wnd, w,h);
							}
							break;
					}

					break;
				}

				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					SDL_KeyboardEvent* ev = &Event.key;

					if (ev->keysym.scancode < 0)
						break; 

					KeyInfo ki;
					
					switch (ev->keysym.scancode)
					{
						// handle big codes that didn't
						// fit well into mapping table
						case SDL_SCANCODE_LCTRL:	ki = A3D_LCTRL;		break;
						case SDL_SCANCODE_LSHIFT:	ki = A3D_LSHIFT;	break;
						case SDL_SCANCODE_LALT:		ki = A3D_LALT;		break;
						case SDL_SCANCODE_LGUI:		ki = A3D_LWIN;		break;
						case SDL_SCANCODE_RCTRL:	ki = A3D_RCTRL;		break;
						case SDL_SCANCODE_RSHIFT:	ki = A3D_RSHIFT;	break;
						case SDL_SCANCODE_RALT:		ki = A3D_RALT;		break;
						case SDL_SCANCODE_RGUI:		ki = A3D_RWIN;		break;

						default:
							if (ev->keysym.scancode < 128)
								ki = SDL2A3D[ev->keysym.scancode];
							else
								ki = A3D_NONE;
					}

					if (ki == A3D_NONE)
						break;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (wnd && wnd->platform_api.keyb_key)
						wnd->platform_api.keyb_key(wnd,ki,Event.type==SDL_KEYDOWN);

					// cure sdl, it doesn't report all keys to SDL_TEXTINPUT
					// TODO: test if sdl on MAC also requires this cure
					if (Event.type == SDL_KEYDOWN)
					{
						switch (ki)
						{
							case A3D_DELETE: wnd->platform_api.keyb_char(wnd, 127); break;
							case A3D_BACKSPACE: wnd->platform_api.keyb_char(wnd, 8); break;
							case A3D_ENTER: 
							case A3D_NUMPAD_ENTER:
								wnd->platform_api.keyb_char(wnd, 13); break;
							// case A3D_TAB: we dont use tab in key_char()
						}
					}

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

					int b = SDL_GetMouseState(0,0);

					int mi = MOVE;

					if (b & SDL_BUTTON(SDL_BUTTON_LEFT))
						mi |= LEFT;
					if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE))
						mi |= MIDDLE;
					if (b & SDL_BUTTON(SDL_BUTTON_RIGHT))
						mi |= RIGHT;

					int w, h;
					SDL_GL_GetDrawableSize(wnd->win, &w, &h);
					if (ev->x >= 0 && ev->x < w && ev->y >= 0 && ev->y < h)
						mi |= INSIDE;

					wnd->platform_api.mouse(wnd,ev->x,ev->y,(MouseInfo)mi);
					break;					
				}

				case SDL_MOUSEWHEEL:
				{
					SDL_MouseWheelEvent* ev = &Event.wheel;

					A3D_WND* wnd = wnd_head;
					while (wnd)
					{
						if (SDL_GetWindowID(wnd->win) == ev->windowID)
							break;
						wnd = wnd->next;
					}

					if (!wnd || !wnd->platform_api.mouse)
						break;

					int x, y;
					int b = SDL_GetMouseState(&x, &y);

					int mi = ev->y > 0 ? WHEEL_UP : ev->y < 0 ? WHEEL_DN : 0;

					if (b & SDL_BUTTON(SDL_BUTTON_LEFT))
						mi |= LEFT;
					if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE))
						mi |= MIDDLE;
					if (b & SDL_BUTTON(SDL_BUTTON_RIGHT))
						mi |= RIGHT;

					int w, h;
					SDL_GL_GetDrawableSize(wnd->win, &w, &h);
					if (x>=0 && x<w && y>=0 && y<h)
						mi |= INSIDE;

					wnd->platform_api.mouse(wnd, x, y, (MouseInfo)mi);
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

					// note:
					// as there is no capture lost event in SDL
					// we can't implement it in reliable way

					if (Event.type == SDL_MOUSEBUTTONDOWN)
					{
						if (!wnd->captured)
						{
							SDL_CaptureMouse(SDL_TRUE);
							wnd->captured = ev->button;
						}
					}
					else
					if (wnd->captured == ev->button)
					{
						SDL_CaptureMouse(SDL_FALSE);
						wnd->captured = 0;
					}

					int x, y;
					int b = SDL_GetMouseState(&x,&y);

					int mi = 0;

					if (b & SDL_BUTTON(SDL_BUTTON_LEFT))
						mi |= LEFT;
					if (b & SDL_BUTTON(SDL_BUTTON_MIDDLE))
						mi |= MIDDLE;
					if (b & SDL_BUTTON(SDL_BUTTON_RIGHT))
						mi |= RIGHT;

					int w, h;
					SDL_GL_GetDrawableSize(wnd->win, &w, &h);
					if (x >= 0 && x < w && y >= 0 && y < h)
						mi |= INSIDE;

					switch (ev->button)
					{
						case SDL_BUTTON_LEFT:   mi |= Event.type == SDL_MOUSEBUTTONDOWN ? LEFT_DN : LEFT_UP; break;
						case SDL_BUTTON_MIDDLE: mi |= Event.type == SDL_MOUSEBUTTONDOWN ? MIDDLE_DN : MIDDLE_UP; break;
						case SDL_BUTTON_RIGHT:  mi |= Event.type == SDL_MOUSEBUTTONDOWN ? RIGHT_DN : RIGHT_UP; break;
					}

					wnd->platform_api.mouse(wnd,ev->x,ev->y, (MouseInfo)mi);
					break;											
				}
			}
		}

		A3D_WND* wnd = wnd_head;
		A3D_WND* swap = 0;

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
			SDL_GL_SetSwapInterval(1);
			SDL_GL_SwapWindow(swap->win);
		}
	}
}

void _a3dSetIconData(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
{
	int wh = w * h;
	uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t)*wh);
	Convert_UI32_AARRGGBB(buf, f, w, h, data, palsize, palbuf);
	SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(buf,w,h,32,w*4,0xFF<<16,0xFF<<8,0xFF,0xFF<<24);
	SDL_SetWindowIcon(((A3D_WND*)cookie)->win, icon);
	SDL_FreeSurface(icon);
	free(buf);
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

void a3dSetTitle(A3D_WND* wnd, const char* utf8_name)
{
	SDL_SetWindowTitle(wnd->win, utf8_name);
}

int a3dGetTitle(A3D_WND* wnd, char* utf8_name, int size)
{
	const char* name = SDL_GetWindowTitle(wnd->win);
	int len = (int)strlen(name);
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
	if (ki < 0 || ki >= 128)
		return false;
	int scan = A3D2SDL[ki];

	int n = 0;
	const Uint8* arr = SDL_GetKeyboardState(&n);
	if (n <= scan)
		return false;

	return arr[scan] != 0;
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
