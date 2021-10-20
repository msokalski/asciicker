
#include "sprite.h"
#include "font1.h"

static Sprite* font1_sprite[3] = { 0,0,0 }; // grey, gold, plain
extern char base_path[1024];

static const int font1_skins = 3;
static const int font1_rows = 4;
static const int font1_cols = 13;
static const int font1_size = 44;
static const int font1_cell_w = 5;
static const int font1_cell_h = 5;
static uint8_t font1_yadv = 4;
static uint8_t font1_xadv[] =
{
	// top to bottom
	3,  3,  3,  3,  3,  3,  3,  3,  2,  3,  3,  3,  4,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  3,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  2,  3,  4,
	3,  3,  3,  3,  3
};

static const int font1_cmap_size = 96; // size of cmap array
static const int font1_cmap_invd = 99; // invalid character index in cmap
static uint8_t font1_cmap[] =
{
	99,39,40,41,42,43,99,99,99,99,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,
	36,99,99,99,99,99,99,99,99,99,99,99,99,99,37,99,
	26,27,28,29,30,31,32,33,34,35,99,99,99,99,99,38,
	99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
	15,16,17,18,19,20,21,22,23,24,25,99,99,99,99,99
};

void LoadFont1()
{
	char path[1024+20];
	const char* name = "font-1.xp";
	sprintf(path, "%ssprites/%s", base_path, name);
	font1_sprite[0] = LoadSprite(path, name);
	uint8_t recolor1[] = { 3, 85,85,85, 255,255,85, 170,170,170, 255,204,0, 255,255,255, 255,204,0 };
	font1_sprite[1] = LoadSprite(path, name, recolor1);
	uint8_t recolor2[] = { 3, 85,85,85, 255,153,255, 170,170,170, 255,0,255, 255,255,255, 255,51,255 };
	font1_sprite[2] = LoadSprite(path, name, recolor2);
}

void FreeFont1()
{
	FreeSprite(font1_sprite[0]);
	FreeSprite(font1_sprite[1]);
}

void Font1Size(const char* str, int* w, int* h)
{
	int width = 0;
	int y = font1_yadv;
	int x = 0;

	while (*str)
	{
		if (*str == '\n')
		{
			width = x > width ? x : width;
			y += font1_yadv;
			x = 0;
		}
		else
		if (*str >=0 && *str < font1_cmap_size)
		{
			uint8_t chr = font1_cmap[*str];
			if (chr != font1_cmap_invd)
				x += font1_xadv[chr];
		}

		str++;
	}

	if (w)
		*w = x > width ? x : width;
	if (h)
		*h = y;
}

void Font1Paint(AnsiCell* ptr, int width, int height, int dx, int dy, const char* str, int skin)
{
	int y = dy;
	int x = dx;

	if (skin < 0 || skin >= font1_skins)
		return;

	Sprite::Frame* sf = font1_sprite[skin]->atlas;

	while (*str)
	{
		if (*str == '\n')
		{
			y -= font1_yadv;
			x = dx;
		}
		else
		if (*str >= 0 && *str < font1_cmap_size)
		{
			if (*str == '?')
			{
				int a = 0;
			}
			uint8_t chr = font1_cmap[*str];
			if (chr != font1_cmap_invd)
			{
				int col = chr % font1_cols;
				int row = chr / font1_cols;

				int up_row = font1_rows - 1 - row;

				int clip[] = { col * font1_cell_w, up_row * font1_cell_h, (col+1) * font1_cell_w, (up_row+1) * font1_cell_h };
				BlitSprite(ptr, width, height, sf, x, y, clip);

				x += font1_xadv[chr];
			}
		}

		str++;
	}
}
