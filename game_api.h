#ifndef GAME_API_H
#define GAME_API_H

struct Game;

#define AKAPI_BUF_SIZE 65536
extern "C" void akAPI_Call(int id);
extern void* akAPI_Buff;
extern Game* game;

void akAPI_Init();
void akAPI_Free();

 // implemented by game_app or game_web
void akAPI_Exec(const char* str, int len = -1, bool root=false);

#endif
