#ifndef GAMEPAD_H
#define GAMEPAD_H

void LoadGamePad();
void FreeGamePad();

void ConnectGamePad(const char* name, int axes, int buttons, const uint8_t mapping[]);
void DisconnectGamePad();

// return num of outs
int UpdateGamePadAxis(int a, int16_t pos, uint32_t out[2]);
int UpdateGamePadButton(int b, int16_t pos, uint32_t out[1]);

void PaintGamePad(AnsiCell* ptr, int width, int height);

#endif
