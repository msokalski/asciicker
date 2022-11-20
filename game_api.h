#ifndef GAME_API_H
#define GAME_API_H

struct Game;

#define AKAPI_BUF_SIZE (65536+256/8)
extern "C" void akAPI_Call(int id);
extern void* akAPI_Buff;
extern Game* game;

void akAPI_Init();
void akAPI_Free();

 // implemented by game_app or game_web
void akAPI_Exec(const char* str, int len = -1, bool root=false);
void akAPI_CB(int id);

#if defined GAME || defined EMSCRIPTEN
bool akAPI_OnSay(const char* str, int len);
bool akAPI_OnItem(int action, int story_id, int kind, int subkind, int weight, const char* str);
#else
bool akAPI_OnSay(const char* str, int len) {return false;}
bool akAPI_OnItem(int action, int story_id, int kind, int subkind, int weight, const char* str) {return false;}
#endif

#endif
