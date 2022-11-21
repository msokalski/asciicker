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
bool akAPI_OnSay(const char* str, int len, bool* allowed=0);
bool akAPI_OnItem(int action, int story_id, int kind, int subkind, int weight, const char* str,
                  bool* allowed=0, int* out_story_id=0, const char** out_desc=0);
#else
bool akAPI_OnSay(const char* str, int len, bool* allowed=0) {return false;}
bool akAPI_OnItem(int action, int story_id, int kind, int subkind, int weight, const char* str,
                  bool* allowed=0, int* out_story_id=0, const char** out_desc=0) {return false;}
#endif

#endif
