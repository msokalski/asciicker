#ifndef FONT1_H
#define FONT1_H

void LoadFont1();
void FreeFont1();

void Font1Size(const char* str, int* w, int* h);
void Font1Paint(AnsiCell* ptr, int width, int height, int x, int y, const char* str, int skin = 0);

#endif
