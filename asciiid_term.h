#pragma once

#include <stdint.h>

struct TermCell
{
    uint8_t bg[3];
    uint8_t fg[3];
    uint8_t h;
    uint8_t ch;
};

struct TermScreen
{
    int width, height;
    TermCell cell[1];
};

TermScreen* CreateScreen(int width, int height);
void DeleteScreen(TermScreen* screen);
void ClearScreen(TermScreen* screen, int x, int y, int w, int h, const TermCell* fill);

// specialized in asciiid_xterm
int PrintScreen(const TermScreen* screen, const uint8_t ipal[1<<24]);
