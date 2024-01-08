#ifndef MAINMENU_H
#define MAINMENU_H

#include "game.h"

#if !defined EDITOR && !defined SERVER

int LoadMainMenuSprites(const char* base_path);
void FreeMainMenuSprites();

void MainMenu_Show();

void MainMenu_Render(uint64_t _stamp, AnsiCell* ptr, int width, int height);
void MainMenu_OnSize(int w, int h, int fw, int fh);
void MainMenu_OnKeyb(GAME_KEYB keyb, int key);
void MainMenu_OnMouse(GAME_MOUSE mouse, int x, int y);
void MainMenu_OnTouch(GAME_TOUCH touch, int id, int x, int y);
void MainMenu_OnFocus(bool set);
void MainMenu_OnPadMount(bool connect);
void MainMenu_OnPadButton(int b, bool down);
void MainMenu_OnPadAxis(int a, int16_t pos);
#else
inline void MainMenu_Show() {}
inline int LoadMainMenuSprites(const char* base_path) { return 0; }
inline void FreeMainMenuSprites() {}
inline void MainMenu_Render(uint64_t _stamp, AnsiCell* ptr, int width, int height){}
inline void MainMenu_OnSize(int w, int h, int fw, int fh){}
inline void MainMenu_OnKeyb(GAME_KEYB keyb, int key){}
inline void MainMenu_OnMouse(GAME_MOUSE mouse, int x, int y){}
inline void MainMenu_OnTouch(GAME_TOUCH touch, int id, int x, int y){}
inline void MainMenu_OnFocus(bool set){}
inline void MainMenu_OnPadMount(bool connect){}
inline void MainMenu_OnPadButton(int b, bool down){}
inline void MainMenu_OnPadAxis(int a, int16_t pos){}
#endif

#endif