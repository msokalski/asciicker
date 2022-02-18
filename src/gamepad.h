#ifndef GAMEPAD_H
#define GAMEPAD_H

void LoadGamePad();
void FreeGamePad();

void SetGamePadMapping(const uint8_t* map);
const uint8_t* GetGamePadMapping();
const char* GetGamePad(int* axes, int* buttons);

void ConnectGamePad(const char* name, int axes, int buttons, const uint8_t mapping[]);
void DisconnectGamePad();

// return num of outs
int UpdateGamePadAxis(int a, int16_t pos, uint32_t out[4]);
int UpdateGamePadButton(int b, int16_t pos, uint32_t out[1]);

void PaintGamePad(AnsiCell* ptr, int width, int height, uint64_t stamp);

void GamePadOpen( void (*close)(void* _g), void* g );

void GamePadContact(int id, int ev, int x, int y, uint64_t stamp);
/*
    id = contact (0:lmb, 1:touch0, 2:touch1, ...)
    ev = 0:begin 1;move 2:end 3:cancel
*/

void GamePadKeyb(int key, uint64_t stamp);
/*
    key = 5:l,6:r,3:u,4:d, 1:enter, 0:space, 2:(backslash,escape,backspace), 7:(c,C), 8:(r,R)
*/

#endif
