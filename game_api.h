#ifndef GAME_API_H
#define GAME_API_H

struct Game;

#define AKAPI_BUF_SIZE 65536
extern "C" void akAPI_Call(int id);
extern void* akAPI_Buff;
extern Game* game;

void akAPI_Init();
void akAPI_Free();

#endif
