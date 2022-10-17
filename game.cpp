#include <string.h>
#include <stdarg.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "game.h"
#include "enemygen.h"
#include "platform.h"
#include "network.h"
#include "matrix.h"
#include "fast_rand.h"

#include "font1.h"
#include "gamepad.h"
#include "audio.h"

uint8_t ConvertToCP437(uint32_t uc)
{
	static const uint8_t tab00A1[95]=
	{
		0xAD,0x9B,0x9C,0x00,0x9D,0x00,0x15,0x00,0x00,0xA6,0xAE,0xAA,0x00,0x00,0x00,0xF8,
		0xF1,0xFD,0x00,0x00,0xE6,0x14,0xFA,0x00,0x00,0xA7,0xAF,0xAC,0xAB,0x00,0xA8,0x00,
		0x00,0x00,0x00,0x8E,0x8F,0x92,0x80,0x00,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xA5,0x00,0x00,0x00,0x00,0x99,0x00,0x00,0x00,0x00,0x00,0x9A,0x00,0x00,0xE1,0x85,
		0xA0,0x83,0x00,0x84,0x86,0x91,0x87,0x8A,0x82,0x88,0x89,0x8D,0xA1,0x8C,0x8B,0x00,
		0xA4,0x95,0xA2,0x93,0x00,0x94,0xF6,0x00,0x97,0xA3,0x96,0x81,0x00,0x00,0x98
	};

	/*
	static const uint8_t tab0192[1]=
	{
		0x9F
	};
	*/

	static const uint8_t tab0393[52]=
	{
		0xE2,0x00,0x00,0x00,0x00,0xE9,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xE4,0x00,0x00,0xE8,0x00,0x00,0xEA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,
		0x00,0xEB,0xEE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE3,0x00,0x00,
		0xE5,0xE7,0x00,0xED
	};

	static const uint8_t tab2022[134]=
	{
		0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x13,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x9E
	};

	static const uint8_t tab2190[25]=
	{
		0x1B,0x18,0x1A,0x19,0x1D,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x17
	};

	static const uint8_t tab2219[77]=
	{
		0xF9,0xFB,0x00,0x00,0x00,0xEC,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xEF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF7,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0xF3,0xF2
	};

	static const uint8_t tab2302[32]=
	{
		0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA9,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF4,0xF5
	};

	static const uint8_t tab2500[218]=
	{
		0xC4,0x00,0xB3,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xDA,0x00,0x00,0x00,
		0xBF,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0xD9,0x00,0x00,0x00,0xC3,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0xB4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC2,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0xC1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC5,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xCD,0xBA,0xD5,0xD6,0xC9,0xB8,0xB7,0xBB,0xD4,0xD3,0xC8,0xBE,0xBD,0xBC,0xC6,0xC7,
		0xCC,0xB5,0xB6,0xB9,0xD1,0xD2,0xCB,0xCF,0xD0,0xCA,0xD8,0xD7,0xCE,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xDF,0x00,0x00,0x00,0xDC,0x00,0x00,0x00,0xDB,0x00,0x00,0x00,0xDD,0x00,0x00,0x00,
		0xDE,0xB0,0xB1,0xB2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x00,0x00,0x00,
		0x00,0x00,0x1E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x1F,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x0A
	};

	static const uint8_t tab263A[50]=
	{
		0x01,0x02,0x0F,0x00,0x00,0x00,0x0C,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x05,0x00,0x03,0x04,0x00,0x00,0x00,
		0x0D,0x0E
	};

	uint8_t cp = 0x20;

	if (uc >= 0x2219)
	{
		if (uc < 0x2219 + sizeof(tab2219))
			cp = tab2219[uc - 0x2219];
		else
		if (uc >= 0x2302)
		{
			if (uc < 0x2302 + sizeof(tab2302))
				cp = tab2302[uc - 0x2302];
			if (uc >= 0x2500)
			{
				if (uc < 0x2500 + sizeof(tab2500))
					cp = tab2500[uc - 0x2500];
				else
				if (uc >= 0x263A)
				{
					if (uc < 0x263A + sizeof(tab263A))
						cp = tab263A[uc - 0x263A];
				}
			}
		}
	}
	else
	if (uc >= 0x0020)
	{
		if (uc < 0x7F)
			cp = (char)uc;
		else
		if (uc >= 0x00A1)
		{
			if (uc < 0x00A1 + sizeof(tab00A1))
				cp = tab00A1[uc - 0x00A1];
			else
			if (uc == 0x0192) // tab0192
				cp = 0x9F;
			else
			if (uc >= 0x0393)
			{
				if (uc < 0x0393 + sizeof(tab0393))
					cp = tab0393[uc - 0x0393];
				else
				if (uc >= 0x2022)
				{
					if (uc < 0x2022 + sizeof(tab2022))
						cp = tab2022[uc - 0x2022];
					else
					if (uc >= 0x2190)
					{
						if (uc < 0x2190 + sizeof(tab2190))
							cp = tab2190[uc - 0x2190];
					}
				}
			}
		}
	}

	return cp;
}

void ConvertToCP437(char* cp437, const char* _utf8)
{
	const uint8_t* utf8 = (const uint8_t*)_utf8;

	int i=0,j=0;
	while (1)
	{
		uint32_t uc;

		// unify, eat upto 4 bytes
		if (utf8[i]<128) // 01111111 (7bits)
		{
			uc = utf8[i];
			i++;
		}
		else
		if (utf8[i]<128+64) // 10xxxxxx (err)
		{
			// err
			i++;
			continue;
		}
		else
		if (utf8[i]<128+64+32) // 110xxxxx 10xxxxxx (11bits)
		{
			if ((utf8[i+1]>>6) != 2)
			{
				// err
				i++;
				continue;
			}

			uc = ((utf8[i]&0x3F)<<6) | (utf8[i+1]&0x3F);
			i+=2;
		}
		else
		if (utf8[i]<128+64+32+16) // 1110xxxx 10xxxxxx 10xxxxxx (16bits)
		{
			if ((utf8[i+1]>>6) != 2 || (utf8[i+2]>>6) != 2)
			{
				// err
				i++;
				continue;
			}

			uc = ((utf8[i]&0xF)<<12) | ((utf8[i+1]&0x3F)<<6) | (utf8[i+2]&0x3F);
			i+=3;
		}
		else
		if (utf8[i]<128+64+32+16+8) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (21bits)
		{
			if ((utf8[i+1]>>6) != 2 || (utf8[i+2]>>6) != 2 || (utf8[i+3]>>6) != 2)
			{
				// err
				i++;
				continue;
			}

			uc = ((utf8[i]&0x7)<<18) | ((utf8[i+1]&0x3F)<<12) | ((utf8[i+2]&0x3F)<<6) | (utf8[i+3]&0x3F);
			i+=4;
		}
		else // 11111xxx (err)
		{
			// err
			i++;
			continue;
		}

		if (uc==0)
		{
			cp437[j++] = 0;
			break;
		}

		uint8_t cp = ConvertToCP437(uc);
		cp437[j++] = (char)cp;
	}
}

extern char base_path[];

static const int stand_us_per_frame = 30000;
static const int fall_us_per_frame = 30000;
static const int attack_us_per_frame = 20000;

static const uint8_t black = 16;
static const uint8_t white =   16 + 5 * 1 + 5 * 6 + 5 * 36;
static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;
static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
static const uint8_t lt_red = 16 + 1 * 1 + 1 * 6 + 5 * 36;
static const uint8_t dk_red = 16 + 0 * 1 + 0 * 6 + 3 * 36;
static const uint8_t cyan = 16 + 5 * 1 + 5 * 6 + 1 * 36;
static const uint8_t yellow = 16 + 1 * 1 + 5 * 6 + 5 * 36;
static const uint8_t lt_blue = 16 + 5 * 1 + 1 * 6 + 1 * 36;
static const uint8_t dk_blue = 16 + 3 * 1 + 0 * 6 + 0 * 36;
static const uint8_t brown = 16 + 0 * 1 + 2 * 6 + 3 * 36;
static const uint8_t lt_green = 16 + 1 * 1 + 5 * 6 + 1 * 36;
static const uint8_t dk_green = 16 + 0 * 1 + 3 * 6 + 0 * 36;

extern Terrain* terrain;
extern World* world;

Game* prime_game = 0;

// asciiid pseudo-multiplayer
// or game npcs?
Character* player_head = 0;
Character* player_tail = 0;

char player_name[32*4] = "player";
char player_name_cp437[32] = "player";

void ChatLog(const char* fmt, ...)
{
	// move it to game_app/web/srv and asciid
	// we dont want to printf in -term mode!
	va_list args;
	va_start(args,fmt);
	vprintf(fmt,args);
	va_end(args);
}

void SyncConf();
const char* GetConfPath();

bool GetGamePadConfPath(char* path, const char* name, int axes, int buttons)
{
	const char* cfg = GetConfPath();
	const char* filepart1 = strrchr(cfg,'/');
	const char* filepart2 = strrchr(cfg,'\\');

	if (!filepart1 && !filepart2)
		return false;

	if (!filepart2)
		filepart2 = filepart1;
	if (!filepart1)
		filepart1 = filepart2;

	int pos1 = filepart1 - cfg;
	int pos2 = filepart2 - cfg;
	int pos = pos1>pos2 ? pos1 : pos2;

	memcpy(path,cfg,pos+1);
	sprintf(path+pos+1,"asciicker_(%s)_A%d_B%d.cfg",name,axes,buttons);

	for (int i=pos+1; path[i]; i++)
	{
		// fix characters not valid for path
		if (path[i]<=32 || path[i]>=127)
			path[i]='_';
		else
		if (path[i]=='<')
			path[i]='[';
		else
		if (path[i]=='>')
			path[i]=']';
		else
		if (path[i]=='/' || path[i]=='\\' || path[i]=='|' || path[i]=='?' || path[i]=='*')
			path[i]='.';
		else
		if (path[i]=='\"')
			path[i]='\'';
		else
		if (path[i]==':')
			path[i]=';';
	}

	return true;
}

bool ReadGamePadConf(uint8_t map[256], const char* name, int axes, int buttons)
{
	char path[1024];
	if (!GetGamePadConfPath(path,name,axes,buttons))
		return false;

	FILE* f = fopen(path,"rb");
	if (!f)
		return false;

	int n = 2*axes+buttons;
	int r = fread(map,1,n,f);
	fclose(f);

	return n==r;
}

bool WriteGamePadConf(const uint8_t* map, const char* name, int axes, int buttons)
{
	char path[1024];
	if (!GetGamePadConfPath(path,name,axes,buttons))
		return false;

	FILE* f = fopen(path,"wb");
	if (!f)
		return false;

	int n = 2*axes+buttons;
	int w = fwrite(map,1,n,f);
	fclose(f);

	SyncConf();

	return n==w;
}


void ReadConf(Game* g)
{
	FILE* f = fopen(GetConfPath(), "rb");
	if (f)
	{
		//printf("ReadConf ok\n");
		int r = fread(g->talk_mem, sizeof(Game::TalkMem), 4, f);

		r = fread(&g->perspective, 1, 1, f);
		r = fread(&g->blood, 1, 1, f);

		fclose(f);
	}
	else
	{
		//printf("ReadConf err\n");
	}
}

void WriteConf(Game* g)
{

	FILE* f = fopen(GetConfPath(), "wb");
	if (f)
	{
		//printf("WriteConf ok\n");
		fwrite(g->talk_mem, sizeof(Game::TalkMem), 4, f);

		fwrite(&g->perspective, 1, 1, f);
		fwrite(&g->blood, 1, 1, f);

		fclose(f);
	}
	else
	{
		//printf("WriteConf err\n");
	}
	
	SyncConf();
}

struct HPBar
{
	static const int height = 4;

	void Paint(AnsiCell* ptr, int width, int height, float val, int xyw[3], bool flip) const
	{
		int pos[2] = { xyw[0], xyw[1] };
		int size = xyw[2];

		int dx = 1;
		uint8_t left = 221;
		uint8_t right = 222;
		uint8_t lt = lt_red;
		uint8_t dk = dk_red;
		uint8_t ul = yellow;
		uint8_t left_line = 218;
		uint8_t right_line = 191;

		if (flip)
		{
			dx = -1;
			pos[0] += size - 1;
			left = 222;
			right = 221;
			lt = lt_blue;
			dk = dk_blue;
			ul = cyan;
			left_line = 191;
			right_line = 218;
		}

		AnsiCell* row[4]=
		{
			pos[1] >= 0 && pos[1] < height ? ptr + pos[1]*width : 0,
			pos[1] + 1 >= 0 && pos[1] + 1 < height ? ptr + (pos[1] + 1) * width : 0,
			pos[1] + 2 >= 0 && pos[1] + 2 < height ? ptr + (pos[1] + 2) * width : 0,
			pos[1] + 3 >= 0 && pos[1] + 3 < height ? ptr + (pos[1] + 3) * width : 0,
		};

		int cols[] = { 1,1,4,1,1,1,1 };

		int dw = (size < 10 ? 10 : size) - 10;
		int d = dw / 4;
		dw -= 4 * d;

		cols[2] += d;
		cols[3] += d;
		cols[4] += d;
		cols[5] += d;

		for (int c = 5; dw > 0; c--)
		{
			dw--;
			cols[c]++;
		}

		int x_thresh = pos[0] + dx * (1 + (int)(val * (size - 2) + 0.5f));
		int perc = (int)(val * 100 + 0.5f);
		char str[]="           xxxx"; // shut wrngz up

		if (perc<100)
			sprintf(str+1,"%d%%",perc);
		else
			sprintf(str,"%d%%",perc);

		int str_len = 4;

		if (flip)
		{
			for (int i = 0; i < str_len / 2; i++)
			{
				char swp = str[i];
				str[i] = str[str_len - 1 - i];
				str[str_len - 1 - i] = swp;
			}
		}

		// bottom
		if (row[0])
		{
			int x = pos[0] + dx; // cols[1]
			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[0] + x;
				ac->bk = AverageGlyph(ac, 0x3);
				ac->fg = black;
				ac->gl = 223;
			}
			x += dx;

			for (int i = 0; i < cols[2]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[0] + x;
					ac->bk = x*dx < x_thresh*dx ? dk : dk_grey;
					ac->fg = black;
					ac->gl = 220;
				}
				x += dx;
			}

			for (int i = 0; i < cols[3]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[0] + x;
					ac->bk = AverageGlyph(ac, 0x3);
					ac->fg = black;
					ac->gl = 223;
				}
				x += dx;
			}
		}

		if (row[1])
		{
			int x = pos[0];
			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[1] + x;
				ac->bk = AverageGlyph(ac, 0x5);
				ac->fg = black;
				ac->gl = right;
			}
			x += dx;

			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[1] + x;
				if (x*dx < x_thresh*dx)
				{
					ac->bk = dk;
					ac->fg = black;
					ac->gl = 32;
				}
				else
				{
					ac->bk = dk_grey;
					ac->fg = black;
					ac->gl = 177;
				}
			}
			x += dx;

			for (int i = 0; i < cols[2]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[1] + x;
					if (x*dx < x_thresh*dx)
					{
						ac->bk = dk;
						ac->fg = white;
						ac->gl = i<str_len ? str[i] : 32;
					}
					else
					{
						if (i == 0)
						{
							if (perc >= 100)
							{
								ac->bk = dk_grey;
								ac->fg = white;
								ac->gl = str[i];
							}
							else
							{
								ac->bk = dk_grey;
								ac->fg = black;
								ac->gl = 176;
							}
						}
						else
						{
							ac->bk = dk_grey;
							ac->fg = white;
							ac->gl = i < str_len ? str[i] : 32;
						}
					}
				}
				x += dx;
			}

			for (int i = 0; i < cols[3]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[1] + x;
					ac->bk = (x*dx < x_thresh*dx) ? dk : dk_grey;
					ac->fg = white;
					ac->gl = 32;
				}
				x += dx;
			}

			for (int i = 0; i < cols[4]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[1] + x;
					ac->bk = (x*dx < x_thresh*dx) ? dk : dk_grey;
					ac->fg = black;
					ac->gl = 220;
				}
				x += dx;
			}

			for (int i = 0; i < cols[5]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[1] + x;
					ac->bk = AverageGlyph(ac,0x3);
					ac->fg = black;
					ac->gl = 223;
				}
				x += dx;
			}
		}

		if (row[2])
		{
			int x = pos[0];
			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[2] + x;
				ac->bk = AverageGlyph(ac, 0x5);
				ac->fg = black;
				ac->gl = right;
			}
			x += dx;

			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[2] + x;
				if (x*dx < x_thresh*dx)
				{
					ac->bk = dk;
					if ((x + dx)*dx < x_thresh*dx)
					{
						ac->fg = flip ? lt : ul;
						ac->gl = left_line;
					}
					else
					{
						ac->fg = lt;
						ac->gl = 44;
					}
				}
				else
				{
					ac->bk = dk_grey;
					ac->fg = black;
					ac->gl = 178;
				}
			}
			x += dx;

			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[2] + x;
				if (x*dx < x_thresh*dx)
				{
					ac->bk = dk;
					if ((x + dx)*dx < x_thresh*dx)
					{
						ac->fg = lt;
						ac->gl = 196;
					}
					else
					{
						ac->fg = flip ? ul : lt;
						ac->gl = right_line;
					}
				}
				else
				{
					ac->bk = dk_grey;
					ac->fg = black;
					ac->gl = 177;
				}
			}
			x += dx;

			int j = cols[2] + cols[3] + cols[4];
			for (int i = 1; i < j; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[2] + x;
					if (x*dx < x_thresh*dx)
					{
						ac->bk = dk;
						if ((x + dx)*dx < x_thresh*dx)
						{
							ac->fg = lt;
							ac->gl = 196;
						}
						else
						{
							ac->fg = flip ? ul : lt;
							ac->gl = right_line;
						}
					}
					else
					{
						ac->bk = dk_grey;
						ac->fg = black;
						ac->gl = 176;
					}
				}
				x += dx;
			}

			for (int i = 0; i < cols[5]; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[2] + x;
					if (x*dx < x_thresh*dx)
					{
						ac->bk = dk;
						if ((x + dx)*dx < x_thresh*dx)
						{
							ac->fg = lt;
							ac->gl = 196;
						}
						else
						{
							ac->fg = flip ? ul : lt;
							ac->gl = right_line;
						}
					}
					else
					{
						ac->bk = dk_grey;
						ac->fg = black;
						if (i < cols[5] - 2)
							ac->gl = 176;
						else
						if (i < cols[5] - 1)
							ac->gl = 177;
						else
							ac->gl = 178;
					}
				}
				x += dx;
			}

			if (x >= 0 && x < width)
			{
				AnsiCell* ac = row[2] + x;
				ac->bk = AverageGlyph(ac, 0xA);
				ac->fg = black;
				ac->gl = left;
			}
			x += dx;
		}

		if (row[3])
		{
			int x = pos[0] + dx;
			int j = cols[1] + cols[2] + cols[3] + cols[4] + cols[5];
			for (int i = 0; i < j; i++)
			{
				if (x >= 0 && x < width)
				{
					AnsiCell* ac = row[3] + x;
					ac->bk = AverageGlyph(ac, 0xC);
					ac->fg = black;
					ac->gl = 220;
				}
				x += dx;
			}
		}
	}
};

struct TalkBox
{
	int max_width, max_height;
	int size[2];
	int cursor_xy[2];
	int cursor_pos;
	int len;

	void Paint(AnsiCell* ptr, int width, int height, int x, int y, bool cursor, const char* name=0) const
	{
		// x,y is at smoke spot, box will be centered above it

		struct Cookie
		{
			const TalkBox* box;
			AnsiCell* ptr;
			int width, height, x, y;
			int span;
			int rows;
			static void Print(int dx, int dy, const char* str, int len, void* cookie)
			{
				Cookie* c = (Cookie*)cookie;
				if (c->y - dy < 0 || c->y - dy >= c->height)
					return;

				AnsiCell* ar = c->ptr + c->x + c->width * (c->y - dy);

				for (int i=0; i<len; i++)
				{
					if (str[i] == '\n')
					{
						for (int x = dx + i; x < c->span; x++)
						{
							if (x + c->x < 0 || x + c->x >= c->width)
								continue;

							AnsiCell* ac = ar + x;
							ac->fg = white;							
							ac->bk = dk_grey;
							ac->gl = ' ';
							ac->spare = 0;
						}
						c->rows++;
						break;
					}

					if (c->x + dx + i < 0 || c->x + dx + i >= c->width)
						continue;

					AnsiCell* ac = ar + i + dx;
					ac->fg = white;
					ac->bk = dk_grey;
					ac->gl = str[i];
					ac->spare = 0;
				}
			}
		};

		int w = size[0]+3;
		int left = x - w/2;
		int right = left + w -1;
		int center = left+w/2;

		int bottom = y;
		int lower = y + 1;
		int upper = y + 4 + size[1];

		Cookie cookie = { this, ptr, width, height, left+2, y + size[1]+2, size[0], 0 };
		int bl = Reflow(0, 0, Cookie::Print, &cookie);
		// assert(bl >= 0);

		AnsiCell* ll = ptr + left + lower * width;
		AnsiCell* bc = ptr + center + bottom * width;
		AnsiCell* lr = ptr + right + lower * width;
		AnsiCell* ul = ptr + left + upper * width;
		AnsiCell* ur = ptr + right + upper * width;

		if (center >= 0 && center < width)
		{
			if (bottom >= 0 && bottom < height)
			{
				bc->bk = black;
				bc->fg = lt_grey;
				bc->gl = 179;
			}

			bc += width;

			if (lower >= 0 && lower < height)
			{
				bc->bk = black;
				bc->fg = lt_grey;
				bc->gl = 194;
			}
		}

		if (lower >= 0 && lower < height)
		{
			if (left >= 0 && left < width)
			{
				ll->bk = black;
				ll->fg = lt_grey;
				ll->gl = 192;
			}

			if (right >= 0 && right < width)
			{
				lr->bk = black;
				lr->fg = lt_grey;
				lr->gl = 217;
			}
		}

		if (upper >= 0 && upper < height)
		{
			if (left >= 0 && left < width)
			{
				ul->bk = black;
				ul->fg = lt_grey;
				ul->gl = 218;
			}

			if (right >= 0 && right < width)
			{
				ur->bk = black;
				ur->fg = lt_grey;
				ur->gl = 191;
			}
		}

		if (lower >= 0 && lower < height)
		{
			AnsiCell* row = ptr + lower * width;
			for (int i = left + 1; i < right; i++)
			{
				if (i >= 0 && i < width && i != center)
				{
					row[i].bk = black;
					row[i].fg = lt_grey;
					row[i].gl = 196;
				}
			}
		}

		if (upper >= 0 && upper < height)
		{
			AnsiCell* row = ptr + upper * width;
			int i = left + 1;

			if (name)
			{
				for (int j=0; i < right; i++,j++)
				{
					if (!name[j])
						break;

					if (i >= 0 && i < width)
					{
						row[i].bk = black;
						row[i].fg = white;
						row[i].gl = name[j];
					}
				}
			}

			for (; i < right; i++)
			{
				if (i >= 0 && i < width)
				{
					row[i].bk = black;
					row[i].fg = lt_grey;
					row[i].gl = 196;
				}
			}
		}

		if (lower + 1 >= 0 && lower + 1 < height)
		{
			AnsiCell* row = ptr + (lower + 1) * width;

			for (int i = left + 2; i < right; i++)
			{
				if (i >= 0 && i < width)
				{
					row[i].bk = dk_grey;
					row[i].fg = black;
					row[i].gl = ' ';
				}
			}
		}

		for (int i = lower+1; i <= upper-1; i++)
		{
			if (i >= 0 && i < height)
			{
				AnsiCell* row = ptr + i * width;
				if (left >= 0 && left < width)
				{
					row[left].bk = black;
					row[left].fg = lt_grey;
					row[left].gl = 179;
				}

				if (left + 1 >= 0 && left + 1 < width)
				{
					row[left + 1].bk = dk_grey;
					row[left + 1].fg = black;
					row[left + 1].gl = ' ';
				}

				if (right >= 0 && right < width)
				{
					row[right].bk = black;
					row[right].fg = lt_grey;
					row[right].gl = 179;
				}
			}
		}

		if (upper - 1 >= 0 && upper - 1 < height)
		{
			AnsiCell* row = ptr + (upper-1) * width;

			for (int i = left + 2; i < right; i++)
			{
				if (i >= 0 && i < width)
				{
					row[i].bk = dk_grey;
					row[i].fg = black;
					row[i].gl = ' ';
				}
			}
		}

		if (cursor)
		{
			int cx = left + 2 + cursor_xy[0];
			int cy = upper - 2 - cursor_xy[1];
			if (cx >= 0 && cx < width && cy >= 0 && cy < height)
			{
				AnsiCell* row = ptr + cx + cy * width;
				uint8_t swap = row->fg;
				row->fg = row->bk;
				row->bk = swap;
			}
		}
	}

	void MoveCursorHead()
	{
		cursor_pos = 0;
		int _pos[2];
		int bl = Reflow(0, _pos);

		assert(bl >= 0);

		cursor_xy[0] = _pos[0];
		cursor_xy[1] = _pos[1];
	}

	void MoveCursorTail()
	{
		cursor_pos = len;
		int _pos[2];
		int bl = Reflow(0, _pos);

		assert(bl >= 0);

		cursor_xy[0] = _pos[0];
		cursor_xy[1] = _pos[1];
	}

	void MoveCursorHome()
	{
		cursor_xy[0] = 0;
		int _pos[2];
		int bl = Reflow(0, _pos);
		assert(bl >= 0);
		cursor_xy[0] = bl & 0xFF;
		cursor_pos = bl >> 8;
	}

	void MoveCursorEnd()
	{
		cursor_xy[0] = max_width;
		int _pos[2];
		int bl = Reflow(0, _pos);
		assert(bl >= 0);
		cursor_xy[0] = bl & 0xFF;
		cursor_pos = bl >> 8;
	}

	void MoveCursorX(int dx)
	{
		if (dx < 0 && cursor_pos>0 || dx > 0 && cursor_pos < len)
		{
			cursor_pos += dx;
			if (cursor_pos < 0)
				cursor_pos = 0;
			if (cursor_pos > len)
				cursor_pos = len;

			int _pos[2];
			int bl = Reflow(0, _pos);

			assert(bl >= 0);

			cursor_xy[0] = _pos[0];
			cursor_xy[1] = _pos[1];
		}
	}

	void MoveCursorY(int dy)
	{
		if (dy < 0 && cursor_xy[1]>0 || dy > 0 && cursor_xy[1] < size[1] - 1)
		{
			cursor_xy[1] += dy;
			assert(cursor_xy[1]>=0 && cursor_xy[1] < size[1]);

			int bl = Reflow(0, 0);
			assert(bl>=0);
			cursor_pos = bl>>8;
			cursor_xy[0] = bl&0xFF;
		}
	}
	
	bool Input(int ch)
	{
		// insert / delete char, update size and cursor pos
		if (ch == 127)
		{
			if (cursor_pos == len)
				return false;
			if (cursor_pos < len-1)
				memmove(buf + cursor_pos, buf + cursor_pos + 1, len - 1 - cursor_pos);
			len--;

			int _size[2], _pos[2];
			int bl = Reflow(_size, _pos);

			assert(bl >= 0 || bl == -2 && cursor_xy[1] == size[1] - 1 && _size[1] == cursor_xy[1]);

			size[0] = _size[0];
			size[1] = _size[1];
			cursor_xy[0] = _pos[0];
			cursor_xy[1] = _pos[1];
		}
		else
		if (ch == 8)
		{
			if (cursor_pos > 0)
			{
				if (cursor_pos < len)
					memmove(buf + cursor_pos - 1, buf + cursor_pos, len - cursor_pos);
				cursor_pos--;
				len--;

				int _size[2], _pos[2];
				int bl = Reflow(_size, _pos);

				// detect nasty case when deleting char in last line causes num of lines to decrease
				// resulting in original cursor_xy[1] is out of nuber of lines range (after modification)
				assert(bl >= 0 || bl==-2 && cursor_xy[1]==size[1]-1 && _size[1]==cursor_xy[1]);

				size[0] = _size[0];
				size[1] = _size[1];
				cursor_xy[0] = _pos[0];
				cursor_xy[1] = _pos[1];
			}
			else
				return false;
		}
		else
		{
			if (len < 256)
			{
				if (cursor_pos < len)
					memmove(buf + cursor_pos + 1, buf + cursor_pos, len - cursor_pos);
				buf[cursor_pos] = ch;
				cursor_pos++;
				len++;

				int _size[2],_pos[2];
				int bl = Reflow(_size, _pos);
				if (bl >= 0)
				{
					size[0] = _size[0];
					size[1] = _size[1];
					cursor_xy[0] = _pos[0];
					cursor_xy[1] = _pos[1];
				}
				else
				{
					// revert!!!
					if (cursor_pos < len)
						memmove(buf + cursor_pos - 1, buf + cursor_pos, len - cursor_pos);
					cursor_pos--;
					len--;

					return false;
				}
			}
			else
				return false;
		}

		return true;
	}

	// returns -1 on overflow, otherwise (b<<8) | l 
	// where l = 'current line' length and b = buffer offset at 'current line' begining
	// if _pos is null 'current line' is given directly by cursor_xy[1] otherwise indirectly by cursor_pos
	int Reflow(int _size[2], int _pos[2], void (*print)(int x, int y, const char* str, int len, void* cookie)=0, void* cookie=0) const
	{
		// ALWAYS cursor_pos -> _xy={x,y} and _pos={prevline_pos,nextline_pos}

		int x = 0, y = 0;
		int cx = -1, cy = -1;
		int wordlen = 0;

		int ret = -2; // reflow ok but cursor_xy[1] too big

		int w = 2;

		// todo:
		// actually we need to call print() only on y++ and last line!

		for (int c = 0; c < len; c++)
		{
			assert(x < max_width);

			if (c == cursor_pos)
			{
				cx = x;
				cy = y;
			}

			if (y==cursor_xy[1])
			{
				if (x<=cursor_xy[0])
					ret = (c << 8) | x;
			}				

			if (buf[c] == ' ')
			{
				if (print)
				{
					print(x - wordlen, y, buf + c - wordlen, wordlen+1, cookie); // +1 to include space char
				}

				wordlen = 0;
				x++;

				if (x > w)
					w = x;

				if (x == max_width)
				{
					if (print)
					{
						print(x, y, "\n", 1, cookie);
					}
					x = 0;
					y++;

					if (y == max_height && max_height)
						return -1;
				}
			}
			else
			if (buf[c] == '\n')
			{
				if (print)
				{
					print(x - wordlen, y, buf + c - wordlen, wordlen+1, cookie); // including '\n'
				}

				if (x >= w) // moved
					w = x+1;

				wordlen = 0;
				x = 0;
				y++;
				if (y == max_height && max_height)
					return -1;
			}
			else
			{
				if (x == max_width - 1)
				{
					if (x == wordlen) // break the word!
					{
						if (y==cursor_xy[1])
						{
							// overwrite possibly bigger ret!
							if ((x-1)<=cursor_xy[0])
								ret = ((c-1) << 8) | (x-1);
						}

						w = max_width;

						if (print)
						{
							print(0, y, buf+c-wordlen, wordlen, cookie);
							print(x, y, "\n", 1, cookie);
						}

						wordlen = 0;
						y++;
						x = 0;
						c--; // current char must be moved to the next line

						if (y == max_height && max_height)
							return -1;

						continue;
					}
					else // try wrapping the word
					{
						if (y==cursor_xy[1])
						{
							// overwrite possibly bigger ret!
							if ((x - wordlen - 1)<=cursor_xy[0])
								ret = ((c-wordlen-1) << 8) | (x-wordlen-1);
						}

						if (print)
						{
							print(x - wordlen, y, "\n", 1, cookie);
						}

						c -= wordlen+1;
						wordlen = 0;
						x = 0;
						y++;

						if (y == max_height && max_height)
							return -1;

						continue;
					}
				}

				x++;
				wordlen++;
			}
		}

		if (y==cursor_xy[1])
		{
			if (x<=cursor_xy[0])
				ret = (len << 8) | x;		
		}

		if (print)
		{
			print(x - wordlen, y, buf + len - wordlen, wordlen, cookie);
			print(x, y, "\n", 1, cookie);
		}

		if (x >= w)
		{
			w = x + 1; // ensure extra space char at ending
		}

		// terminator handler
		{
			if (len == cursor_pos)
			{
				cx = x;
				cy = y;
			}

			if (y==cursor_xy[1])
			{
				if (x<=cursor_xy[0])
					ret = (len << 8) | x;
			}			
		}

		if (_size)
		{
			_size[0] = w;
			_size[1] = y + 1;
		}

		if (_pos)
		{
			if (cursor_pos == len)
			{
				_pos[0] = x;
				_pos[1] = y;
			}
			else
			{
				_pos[0] = cx;
				_pos[1] = cy;
			}
		}

		// this is possible that when pressing backspace
		// when x=0 and y>0 in last line, we will not reach current line (1)
		// fix it so caller won't blame us.
		assert(ret>=0 || y<cursor_xy[1]);

		return ret;
	}

	char buf[256];
};

bool Server::Proc(const uint8_t* ptr, int size)
{
	switch (ptr[0])
	{
		case 'j': // hello!
		{
			STRUCT_BRC_JOIN* join = (STRUCT_BRC_JOIN*)ptr;
			Human* h = others + join->id;
			memset(h, 0, sizeof(Human));

			strcpy(h->name, join->name);
			ConvertToCP437(h->name_cp437,h->name);

			h->prev = 0;
			h->next = head;
			if (head)
				head->prev = h;
			else
				tail = h;
			head = h;

			// def pose
			h->req.action = (join->am >> 4) & 0xF;
			h->req.mount = join->am & 0xF;
			h->req.armor = (join->sprite >> 12) & 0xF;
			h->req.helmet = (join->sprite >> 8) & 0xF;
			h->req.shield = (join->sprite >> 4) & 0xF;
			h->req.weapon = join->sprite & 0xF;

			h->clr = 0; // from server

			h->sprite = GetSprite(&h->req, h->clr);

			h->anim = join->anim;
			h->frame = join->frame;

			h->dir = join->dir;
			h->pos[0] = join->pos[0];
			h->pos[1] = join->pos[1];
			h->pos[2] = join->pos[2];

			// insert Inst to the world
			int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;
			int reps[4] = { 0,0,0,0 };
			h->inst = CreateInst(world, h->sprite, flags, h->pos, h->dir, h->anim, h->frame, reps, 0, -1/*not storyline*/);

			break;
		}
		case 'e': // cya!
		{
			STRUCT_BRC_EXIT* leave = (STRUCT_BRC_EXIT*)ptr;
			Human* h = others + leave->id;

			// free talks
			for (int i=0; i<h->talks; i++)
				free(h->talk[i].box);

			if (h->prev)
				h->prev->next = h->next;
			else
				head = (Human*)h->next;
			if (h->next)
				h->next->prev = h->prev;
			else
				tail = (Human*)h->prev;

			// if has world's Inst, remove it
			if (h->inst)
				DeleteInst(h->inst);
			h->inst = 0;

			break;
		}

		case 'p': // you can move!
		{
			STRUCT_BRC_POSE* pose = (STRUCT_BRC_POSE*)ptr;
			Human* h = others + pose->id;

			h->req.action = (pose->am >> 4) & 0xF;
			h->req.mount = pose->am & 0xF;
			h->req.armor = (pose->sprite >> 12) & 0xF;
			h->req.helmet = (pose->sprite >> 8) & 0xF;
			h->req.shield = (pose->sprite >> 4) & 0xF;
			h->req.weapon = pose->sprite & 0xF;

			h->sprite = GetSprite(&h->req, h->clr);

			h->anim = pose->anim;
			h->frame = pose->frame;

			h->dir = pose->dir;
			h->pos[0] = pose->pos[0];
			h->pos[1] = pose->pos[1];
			h->pos[2] = pose->pos[2];

			if (h->inst)
			{
				int reps[4];
				UpdateSpriteInst(world, h->inst, h->sprite, h->pos, h->dir, h->anim, h->frame, reps);
			}

			break;
		}

		case 't': // you can even talk!
		{
			STRUCT_BRC_TALK* talk = (STRUCT_BRC_TALK*)ptr;
			Human* h = others + talk->id;

			if (h->pos[2] > -100)
			{
				TalkBox* box = 0;
				if (h->talks == 3)
				{
					box = h->talk[0].box;
					h->talks--;
					for (int i = 0; i < h->talks; i++)
						h->talk[i] = h->talk[i + 1];
				}
				else
				{
					box = (TalkBox*)malloc(sizeof(TalkBox));
				}

				Human* h = others + talk->id;
				ChatLog("%s : %.*s\n", h->name, talk->len, talk->str);
				
				memset(box, 0, sizeof(TalkBox));
				memcpy(box->buf, talk->str, talk->len);
				box->buf[talk->len] = 0;
				box->len = talk->len;

				box->max_width = 33;
				box->max_height = 7; // 0: off
				int s[2],p[2];
				box->Reflow(s,p);
				box->size[0] = s[0];
				box->size[1] = s[1];
				box->cursor_xy[0] = p[0];
				box->cursor_xy[1] = p[1];

				int idx = h->talks;
				h->talk[idx].box = box;
				h->talk[idx].pos[0] = h->pos[0];
				h->talk[idx].pos[1] = h->pos[1];
				h->talk[idx].pos[2] = h->pos[2];
				h->talk[idx].stamp = stamp;
				h->talks++;
			}
			break;
		}

		case 'l':
		{
			STRUCT_RSP_LAG* lag = (STRUCT_RSP_LAG*)ptr;
			uint32_t s1 = 0;
			s1 |= lag->stamp[0] << 8;
			s1 |= lag->stamp[1] << 16;
			s1 |= lag->stamp[2] << 24;
					
			uint32_t s2 = (uint32_t)stamp << 8;

			int latency = (s2 - s1 + 128) >> 8;

			/*
			char buf[32];
			sprintf(buf,"lag: %d\n", latency);
			Log(buf);
			*/

			lag_ms = (latency + 500) / 1000;
			lag_wait = false;

			// store it in server
			break;
		}

	default:
		return false;
	}


	return true;
}

#if 0
struct KeyCap
{
	// key_width  = border(1) + bevels(2) + size(min 2) + WIDTH_DELTA*current_size_multiplier(0-9)
	// row_width  = SUM(key_width[0..N-1]) + border(1)
	// key_pos(i) = row_xofs + SUM(key_width[0..i-1])
	enum FLAGS
	{
		WIDTH_DELTA1 = 1,
		WIDTH_DELTA2 = 2,
		WIDTH_DELTA3 = 3,
		// WIDTH_DELTA ... upto 7
		DARK_COLOR = 8,
	};
	int flags; // left = 0, right = 1
	int size;
	int a3d_key;
	const char* cap[6][2];

	// other calculabe fields 
	int Paint(AnsiCell* ptr, int width, int height, int x, int y, int width_mul, const uint8_t key[32]) const
	{
		static const AnsiCell bevel_norm[2][3] =
		{
			{ {white,lt_grey,176}, { black,lt_grey,32 }, {dk_grey,lt_grey,176} },
			{ {lt_grey,dk_grey,176}, { black,dk_grey,32 }, {black,dk_grey,176} }
		};

		static const AnsiCell bevel_down[2][3] =
		{
			{ {lt_red,dk_red,176}, { white,dk_red,32 }, {black,dk_red,176} },
			{ {lt_red,dk_red,176}, { white,dk_red,32 }, {black,dk_red,176} },
		};

		const AnsiCell(*bevel)[3];
		if (key[a3d_key >> 3] & (1 << (a3d_key & 7)))
			bevel = bevel_down;
		else
			bevel = bevel_norm;

		int b = (flags >> 3) & 1;
		int w = (flags & 0x7) * width_mul + size + 2;

		AnsiCell* top = ptr + width * (y + 3) + x;
		AnsiCell* upper = ptr + width * (y + 2) + x;
		AnsiCell* lower = ptr + width * (y + 1) + x;
		AnsiCell* bottom = ptr + width * (y + 0) + x;

		int dx_lo = -x;
		int dx_hi = width - x;

		int dx = 0;
		if (y + 3 >= 0 && y + 3 < height)
		{
			for (; dx < w - 1; dx++)
				if (dx >= dx_lo && dx < dx_hi)
					top[dx] = bevel[b][0];
			if (dx >= dx_lo && dx < dx_hi)
				top[dx] = bevel[b][1];
		}

		int cap_index = 0;
		for (int i = 0; i <= width_mul; i++)
			if (cap[i][0])
				cap_index = i;
			else
				break;

		const char* upper_cap = cap[cap_index][0];
		const char* lower_cap = cap[cap_index][1];

		if (y + 2 >= 0 && y + 2 < height)
		{
			if (0 >= dx_lo && 0 < dx_hi)
				upper[0] = bevel[b][0];

			dx = 1;
			for (; dx < w - 1 && upper_cap[dx - 1]; dx++)
			{
				if (dx >= dx_lo && dx < dx_hi)
				{
					upper[dx] = bevel[b][1];
					upper[dx].gl = upper_cap[dx - 1];
				}
			}
			for (; dx < w - 1; dx++)
				if (dx >= dx_lo && dx < dx_hi)
					upper[dx] = bevel[b][1];
			if (dx >= dx_lo && dx < dx_hi)
				upper[dx] = bevel[b][2];
		}

		if (y + 1 >= 0 && y + 1 < height)
		{
			if (0 >= dx_lo && 0 < dx_hi)
				lower[0] = bevel[b][0];
			dx = 1;
			for (; dx < w - 1 && lower_cap[dx - 1]; dx++)
			{
				if (dx >= dx_lo && dx < dx_hi)
				{
					lower[dx] = bevel[b][1];
					lower[dx].gl = lower_cap[dx - 1];
				}
			}
			for (; dx < w - 1; dx++)
				if (dx >= dx_lo && dx < dx_hi)
					lower[dx] = bevel[b][1];
			if (dx >= dx_lo && dx < dx_hi)
				lower[dx] = bevel[b][2];
		}

		if (y >= 0 && y < height)
		{
			if (0 >= dx_lo && 0 < dx_hi)
				bottom[0] = bevel[b][1];
			dx = 1;
			for (; dx < w; dx++)
				if (dx >= dx_lo && dx < dx_hi)
					bottom[dx] = bevel[b][2];
		}

		return w + 1;
	}
};

static const KeyCap row0[] =
{
	{ 0x01, 1, A3D_1, {{"!","1"}, {0,0}} },
	{ 0x01, 1, A3D_2, {{"@","2"}, {0,0}} },
	{ 0x01, 1, A3D_3, {{"#","3"}, {0,0}} },
	{ 0x01, 1, A3D_4, {{"$","4"}, {0,0}} },
	{ 0x01, 1, A3D_5, {{"%","5"}, {0,0}} },
	{ 0x01, 1, A3D_6, {{"^","6"}, {0,0}} },
	{ 0x01, 1, A3D_7, {{"&","7"}, {0,0}} },
	{ 0x01, 1, A3D_8, {{"*","8"}, {0,0}} },
	{ 0x01, 1, A3D_9, {{"(","9"}, {0,0}} },
	{ 0x01, 1, A3D_0, {{")","0"}, {0,0}} },
	{ 0x09, 3, A3D_BACKSPACE, {{"\x11""Bk"," Sp"}, {"\x11""Bck"," Spc"}, {"\x11""Back","Space"}, {"\x11 Back"," Space"}, {"\x11  Back","  Space"}, {0,0}} },
};

static const KeyCap row1[] =
{
	{ 0x01, 1, A3D_Q, {{"Q",""}, {0,0}} },
	{ 0x01, 1, A3D_W, {{"W",""}, {0,0}} },
	{ 0x01, 1, A3D_E, {{"E",""}, {0,0}} },
	{ 0x01, 1, A3D_R, {{"R",""}, {0,0}} },
	{ 0x01, 1, A3D_T, {{"T",""}, {0,0}} },
	{ 0x01, 1, A3D_Y, {{"Y",""}, {0,0}} },
	{ 0x01, 1, A3D_U, {{"U",""}, {0,0}} },
	{ 0x01, 1, A3D_I, {{"I",""}, {0,0}} },
	{ 0x01, 1, A3D_O, {{"O",""}, {0,0}} },
	{ 0x01, 1, A3D_P, {{"P",""}, {0,0}} },
	{ 0x01, 1, A3D_OEM_TILDE, {{"~","`"}, {0,0}} },
};

static const KeyCap row2[] =
{
	{ 0x01, 1, A3D_OEM_QUOTATION, {{"\"","\'"}, {0,0}} },
	{ 0x01, 1, A3D_A, {{"A",""}, {0,0}} },
	{ 0x01, 1, A3D_S, {{"S",""}, {0,0}} },
	{ 0x01, 1, A3D_D, {{"D",""}, {0,0}} },
	{ 0x01, 1, A3D_F, {{"F",""}, {0,0}} },
	{ 0x01, 1, A3D_G, {{"G",""}, {0,0}} },
	{ 0x01, 1, A3D_H, {{"H",""}, {0,0}} },
	{ 0x01, 1, A3D_J, {{"J",""}, {0,0}} },
	{ 0x01, 1, A3D_K, {{"K",""}, {0,0}} },
	{ 0x01, 1, A3D_L, {{"L",""}, {0,0}} },
	{ 0x01, 1, A3D_OEM_COLON, {{":",";"}, {0,0}} },
};

static const KeyCap row3[] =
{
	{ 0x01, 1, A3D_OEM_BACKSLASH, {{"|","\\"}, {0,0}} },
	{ 0x01, 1, A3D_Z, {{"Z",""}, {0,0}} },
	{ 0x01, 1, A3D_X, {{"X",""}, {0,0}} },
	{ 0x01, 1, A3D_C, {{"C",""}, {0,0}} },
	{ 0x01, 1, A3D_V, {{"V",""}, {0,0}} },
	{ 0x01, 1, A3D_B, {{"B",""}, {0,0}} },
	{ 0x01, 1, A3D_N, {{"N",""}, {0,0}} },
	{ 0x01, 1, A3D_M, {{"M",""}, {0,0}} },
	{ 0x01, 1, A3D_OEM_COMMA, {{"<",","}, {0,0}} },
	{ 0x01, 1, A3D_OEM_PERIOD, {{">","."}, {0,0}} },
	{ 0x01, 1, A3D_OEM_SLASH, {{"?","/"}, {0,0}} },
};

static const KeyCap row4[] =
{
	{ 0x0A, 3, A3D_LSHIFT, {{"Shf"," \x1E"}, {"Shift","  \x1E"}, {"Shift \x1E",""}, {0,0}} },
	{ 0x01, 1, A3D_OEM_MINUS, {{"_","-"}, {0,0}} },
	{ 0x01, 1, A3D_OEM_PLUS, {{"+","="}, {0,0}} },
	{ 0x03,15, A3D_SPACE, {{"Space",""}, {0,0}} },
	{ 0x01, 1, A3D_OEM_OPEN, {{"{","["}, {0,0}} },
	{ 0x01, 1, A3D_OEM_CLOSE, {{"}","]"}, {0,0}} },
	{ 0x0A, 3, A3D_ENTER, {{"Ent","\x11\xC4\xD9"}, {"Enter","\x11\xC4\xD9"}, {" Enter"," \x11\xC4\xD9"}, {"\x11\xC4\xD9 Enter",""}, {0,0}} },
};

struct KeyRow
{
	int xofs;
	int caps;
	const KeyCap* row;

	// other calculabe fields 
	// ...

	void Paint(AnsiCell* ptr, int width, int height, int x, int y, int width_mul, const uint8_t key[32], int xarr[]) const
	{
		int xadv = x + xofs;
		for (int cap = 0; cap < caps; cap++)
		{
			if (xarr)
				xarr[cap] = xadv;
			xadv += row[cap].Paint(ptr, width, height, xadv, y, width_mul, key);
		}
		if (xarr)
			xarr[caps] = xadv;
	}
};

struct Keyb
{
	int add_size;
	int mul_size;
	KeyRow rows[5];

public:
	static const int hide = 1 + 5 * 5 + 1; // 1 cell above bottom + 5 rows x 5 cells + 1 border

	int GetCap(int dx, int dy, int width_mul, char* ch, bool shift_on) const
	{
		if (ch)
			*ch = 0;

		// calc row
		static const int yadv = 5;
		if (dy < 1 || dy >= 5 * yadv) // check if in range and not over any horizontal divider
			return -1;
		const KeyRow* r = rows + 4 - (dy - 1) / yadv;

		int xadv = 1 + r->xofs;
		if (dx < xadv)
			return -1;
		for (int c = 0; c < r->caps; c++)
		{
			int w = (r->row[c].flags & 0x7) * width_mul + r->row[c].size + 2;

			if (dx >= xadv)
			{
				if (dx < xadv + w)
				{
					if (dy % 5 == 0)
						return 0;

					if (ch)
					{
						if (r->row[c].a3d_key == A3D_BACKSPACE)
							*ch = 8;
						else
						if (r->row[c].a3d_key == A3D_ENTER)
							*ch = '\n';
						else
						if (r->row[c].a3d_key == A3D_SPACE)
							*ch = ' ';
						else
						if (r->row[c].a3d_key != A3D_LSHIFT)
						{
							if (shift_on)
								*ch = r->row[c].cap[0][0][0];
							else
							{
								if (r->row[c].cap[0][0][0] >= 'A' && r->row[c].cap[0][0][0] <= 'Z')
									*ch = r->row[c].cap[0][0][0] + 'a' - 'A';
								else
									*ch = r->row[c].cap[0][1][0];
							}
						}
					}
					return r->row[c].a3d_key;
				}
			}
			else
				return 0;

			xadv += w + 1;
		}

		return -1;
	}

	int Width(int width_mul) const
	{
		return add_size + width_mul * mul_size;
	}

	void Paint(AnsiCell* ptr, int x, int y, int width, int height, int width_mul, const uint8_t key[32]) const
	{
		static const int yadv = 5;

		int xofs = 1+x;
		int yofs = 1+y;

		static const AnsiCell line = { dk_grey, black, 0, 0 };

		int xarr[2][16];
		for (int i = 0; i < 5; i++)
		{
			rows[4 - i].Paint(ptr, width, height, xofs, yofs, width_mul, key, xarr[i&1]);

			int caps = rows[4 - i].caps;

			int yend = yofs + yadv - 1;
			AnsiCell* below = ptr + (yofs - 1)*width;

			if (yofs < 1 || yofs-1 >= height)
				below = 0;

			// vertical lines
			for (int j = 0; j <= caps; j++)
			{
				int from = xarr[i&1][j] - 1;
				if (from >= 0 && from < width)
				{
					for (int v = yofs; v < yend; v++)
					{
						if (v >= 0 && v < height)
						{
							ptr[width*v + from] = line;
							ptr[width*v + from].gl = 179;
						}
					}
				}
			}

			if (i == 0)
			{
				if (below)
				{
					if (xarr[0][0] - 1 >= 0 && xarr[0][0] - 1 < width)
					{
						below[xarr[0][0] - 1] = line;
						below[xarr[0][0] - 1].gl = 192;
					}

					for (int j = 0; j < caps; j++)
					{
						int from = xarr[0][j];
						int to = xarr[0][j + 1] - 1;

						for (int h = from; h < to; h++)
						{
							if (h >= 0 && h < width)
							{
								below[h] = line;
								below[h].gl = 196;
							}
						}

						if (to >= 0 && to < width)
						{
							below[to] = line;
							below[to].gl = j == caps - 1 ? 217 : 193;
						}
					}
				}
			}
			else
			{
				int prev_caps = rows[5 - i].caps;

				int* upper = xarr[i & 1];
				int* lower = xarr[(i & 1) ^ 1];

				if (below)
				{
					int h,l,u;

					if (lower[0] < upper[0])
					{
						if (lower[0] - 1 >= 0 && lower[0] - 1 < width)
						{
							below[lower[0] - 1] = line;
							below[lower[0] - 1].gl = 218;
						}
						h = lower[0];
						l = 1;
						u = 0;
					}
					else
					if (lower[0] > upper[0])
					{
						if (upper[0] - 1 >= 0 && upper[0] - 1 < width)
						{
							below[upper[0] - 1] = line;
							below[upper[0] - 1].gl = 192;
						}
						h = upper[0];
						l = 0;
						u = 1;
					}
					else
					{
						if (upper[0] - 1 >= 0 && upper[0] - 1 < width)
						{
							below[upper[0] - 1] = line;
							below[upper[0] - 1].gl = 195;
						}
						h = upper[0];
						l = u = 1;
					}

					int e;
					if (lower[prev_caps] < upper[caps])
					{
						if (upper[caps] - 1 >= 0 && upper[caps] - 1 < width)
						{
							below[upper[caps] - 1] = line;
							below[upper[caps] - 1].gl = 217;
						}
						e = upper[caps]-1;
					}
					else
					if (lower[prev_caps] > upper[caps])
					{
						if (lower[prev_caps] - 1 >= 0 && lower[prev_caps] - 1 < width)
						{
							below[lower[prev_caps] - 1] = line;
							below[lower[prev_caps] - 1].gl = 191;
						}
						e = lower[prev_caps] - 1;
					}
					else
					{
						if (upper[caps] - 1 >= 0 && upper[caps] - 1 < width)
						{
							below[upper[caps] - 1] = line;
							below[upper[caps] - 1].gl = 180;
						}
						e = upper[caps] - 1;
					}

					for (;h < e; h++)
					{
						if (h>=0 && h<width)
							below[h] = line;
						if (h == upper[u] - 1)
						{
							u++;
							if (h == lower[l] - 1)
							{
								l++;
								if (h >= 0 && h < width)
									below[h].gl = 197;
							}
							else
								if (h >= 0 && h < width)
									below[h].gl = 193;
						}
						else
						if (h == lower[l] - 1)
						{
							l++;
							if (h >= 0 && h < width)
								below[h].gl = 194;
						}
						else
							if (h >= 0 && h < width)
								below[h].gl = 196;
					}
				}

				if (i == 4)
				{
					AnsiCell* above = ptr + (yofs + yadv - 1)*width;
					if (yofs + yadv < 1 || yofs + yadv - 1 >= height)
						above = 0;

					if (above)
					{
						if (upper[0] - 1 >= 0 && upper[0] - 1 < width)
						{
							above[upper[0] - 1] = line;
							above[upper[0] - 1].gl = 218;
						}

						for (int j = 0; j < caps; j++)
						{
							int from = upper[j];
							int to = upper[j + 1] - 1;

							for (int h = from; h < to; h++)
							{
								if (h >= 0 && h < width)
								{
									above[h] = line;
									above[h].gl = 196;
								}
							}

							if (to >= 0 && to < width)
							{
								above[to] = line;
								above[to].gl = j == caps - 1 ? 191 : 194;
							}
						}
					}
				}
			}

			yofs += yadv;
		}
	}
};

static const Keyb keyb =
{
	47, 11,
	{
		{ 0,11, row0 },
		{ 2,11, row1 },
		{ 0,11, row2 },
		{ 2,11, row3 },
		{ 0, 7, row4 },
	}
};

#endif

Sprite* keyb_sprite[5] = { 0,0,0,0,0 };
Sprite* caps_sprite[3] = { 0,0,0 };

enum
{
	// private virtual keys
	KBD_COMMA = A3D_MAPEND, KBD_PERIOD, KBD_QUESTION, KBD_PLUS, KBD_MINUS, KBD_MULTIPLY, KBD_SLASH, KBD_UNDERLINE, KBD_EQUAL,
	KBD_EXCLAMATION, KBD_MONKEY, KBD_HASH, KBD_DOLLAR, KBD_PERCENT, KBD_DASH, KBD_AMPERSAND,
	KBD_OPEN, KBD_CLOSE, KBD_CURLYOPEN, KBD_CURLYCLOSE, KBD_BRACKETOPEN, KBD_BRACKETCLOSE, KBD_SMALLER, KBD_GREATER, KBD_TILDE,
	KBD_BACKSLASH, KBD_COLON, KBD_SEMICOLON, KBD_APOSTROPHE, KBD_QUOTATION, KBD_BACKQUOTE, KBD_PIPE
};

static const int caps_plane[3][3][10] =
{
	{
		{ A3D_Q, A3D_W, A3D_E, A3D_R, A3D_T, A3D_Y, A3D_U, A3D_I, A3D_O, A3D_P },
		{ A3D_A, A3D_S, A3D_D, A3D_F, A3D_G, A3D_H, A3D_J, A3D_K, A3D_L, A3D_ENTER },
		{ A3D_LSHIFT, A3D_Z, A3D_X, A3D_C, A3D_V, A3D_B, A3D_N, A3D_M, A3D_SPACE, A3D_RSHIFT },
	},
	{
		{ A3D_0, A3D_1, A3D_2, A3D_3, A3D_4, A3D_5, A3D_6, A3D_7, A3D_8, A3D_9 },
		{ KBD_COMMA, KBD_PERIOD, KBD_QUESTION, KBD_PLUS, KBD_MINUS, KBD_MULTIPLY, KBD_SLASH, KBD_UNDERLINE, KBD_EQUAL, A3D_ENTER},
		{ A3D_LSHIFT, KBD_BACKSLASH, KBD_COLON, KBD_SEMICOLON, KBD_APOSTROPHE, KBD_QUOTATION, KBD_BACKQUOTE, KBD_PIPE, A3D_SPACE, A3D_RSHIFT}
	},
	{
		{ A3D_0, A3D_1, A3D_2, A3D_3, A3D_4, A3D_5, A3D_6, A3D_7, A3D_8, A3D_9 },
		{ KBD_OPEN, KBD_CLOSE, KBD_CURLYOPEN, KBD_CURLYCLOSE, KBD_BRACKETOPEN, KBD_BRACKETCLOSE, KBD_SMALLER, KBD_GREATER, KBD_TILDE, A3D_ENTER},
		{ A3D_LSHIFT, KBD_EXCLAMATION, KBD_MONKEY, KBD_HASH, KBD_DOLLAR, KBD_PERCENT, KBD_DASH, KBD_AMPERSAND, A3D_SPACE, A3D_RSHIFT}
	},
};

static const char char_plane[3][3][10] =
{
	{
		{ 'q','w','e','r','t','y','u','i','o','p' },
		{ 'a','s','d','f','g','h','j','k','l', '\n' },
		{  1 ,'z','x','c','v','b','n','m',' ', 2  },
	},
	{
		{ '0','1','2','3','4','5','6','7','8','9' },
		{ ',','.','?','+','-','*','/','_','=', '\n'  },
		{  1,'\\',':',';','\'','"','`','|',' ', 2  },
	},
	{
		{ '0','1','2','3','4','5','6','7','8','9' },
		{ '(',')','{','}','[',']','<','>','~', '\n'  },
		{  1 ,'!','@','#','$','%','^','&',' ', 2  },
	}
};

struct Keyb
{
	int plane = 0;
	int sect = 0;
	int dir = 11; // 11: sector mode, 0-10: direction mode
	bool pad_plane = false;

	int GetPadCap(char* ch, bool shift_on)
	{
		if (dir == 11)
		{
			if (ch)
				*ch = 0;
			return 0;
		}


		int i = 0;
		int j = 0;
		switch (dir) 
		{
			case 0: i = 1; j = 1; break;
			case 1: i = 2; j = 2; break;
			case 2: i = 3; j = 2; break;
			case 3: i = 2; j = 1; break;
			case 4: i = 3; j = 0; break;
			case 5: i = 2; j = 0; break;
			case 6: i = 1; j = 0; break;
			case 7: i = 0; j = 0; break;
			case 8: i = 0; j = 1; break;
			case 9: i = 0; j = 2; break;
			case 10: i = 1; j = 2; break;
		}

		i += sect * 3;

		char cc = char_plane[plane][2-j][i];
		if (shift_on)
		{
			if (cc >= 'a' && cc <= 'z')
				cc += 'A' - 'a';
			if (cc == ' ')
				cc = 8; // shift + space = backspace !!!
		}

		if (ch)
			*ch = cc;

		return caps_plane[plane][2-j][i];
	}

	int GetCap(int dx, int dy, int width, int height, char* ch, bool shift_on) const
	{
		int sprite_w = 2 * (width - 1) / 21 + 1;
		int sprite_h = 0;
		int sprite_i = 0;
		int delta_x = 0;
		int delta_y = 0;
		int delta_d = 0;
		int caps_dy = 0;

		if (sprite_w < 9)
		{
			sprite_w = 7;
			sprite_h = 8;
			delta_x = 3;
			delta_y = 5;
			delta_d = 0;
			caps_dy = 0;
			sprite_i = 0;
		}
		else
		if (sprite_w < 11)
		{
			sprite_w = 9;
			sprite_h = 8;
			delta_x = 4;
			delta_y = 5;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 1;
		}
		else
		if (sprite_w < 13)
		{
			sprite_w = 11;
			sprite_h = 10;
			delta_x = 5;
			delta_y = 6;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 2;
		}
		else
		if (sprite_w < 15)
		{
			sprite_w = 13;
			sprite_h = 13;
			delta_x = 6;
			delta_y = 8;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 3;
		}
		else
		{
			sprite_w = 15;
			sprite_h = 15;
			delta_x = 7;
			delta_y = 9;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 4;
		}

		int keyb_w = 21 * (sprite_w - 1) / 2 + 1;
		int keyb_h = 2 * delta_y + sprite_h;

		int center_x = (width - keyb_w) / 2;


		static int press_j = -1;
		static int press_i = -1;

		static int clicker = 0;
		clicker++;
		if (clicker == 10)
		{
			press_j = fast_rand() % 3;
			press_i = fast_rand() % 10;
		}
		if (clicker == 15)
		{
			clicker = 0;
			press_j = -1;
			press_i = -1;
		}

		int hide = 0;

		for (int j = 0; j < 3; j++)
		{
			for (int i = 0; i < 10; i++)
			{
				int x = center_x + i * (sprite_w - 1) + (j & 1) * delta_x;
				int y = j * delta_y - hide;

				Sprite::Frame* sf = keyb_sprite[sprite_i]->atlas;

				int cx = dx - x;
				int cy = dy - y;
				if (cx >= 0 && cy >= 0 && cx < sprite_w && cy < sprite_h)
				{
					AnsiCell* ac = sf->cell + cy * sf->width + cx;

					// opaqueness test
					if (ac->bk != 255 && ac->gl != 32 && ac->gl != 0 || ac->fg != 255 && ac->gl != 219)
					{
						if (ch)
						{
							char cc = char_plane[plane][2 - j][i];
							if (shift_on)
							{
								if (cc >= 'a' && cc <= 'z')
									cc += 'A' - 'a';
								if (cc == ' ')
									cc = 8; // shift + space = backspace !!!
							}
							*ch = cc;
						}

						return caps_plane[plane][2 - j][i];
					}
				}
			}
		}

		if (ch)
			*ch = 0;
		return -1; // return 0 if it was very close to keyb
	}

	int Width(int width, int height) const
	{
		int sprite_w = 2 * (width - 1) / 21 + 1;
		int sprite_h = 0;
		int sprite_i = 0;
		int delta_x = 0;
		int delta_y = 0;
		int delta_d = 0;
		int caps_dy = 0;

		if (sprite_w < 9)
		{
			sprite_w = 7;
			sprite_h = 8;
			delta_x = 3;
			delta_y = 5;
			delta_d = 0;
			caps_dy = 0;
			sprite_i = 0;
		}
		else
		if (sprite_w < 11)
		{
			sprite_w = 9;
			sprite_h = 8;
			delta_x = 4;
			delta_y = 5;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 1;
		}
		else
		if (sprite_w < 13)
		{
			sprite_w = 11;
			sprite_h = 10;
			delta_x = 5;
			delta_y = 6;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 2;
		}
		else
		if (sprite_w < 15)
		{
			sprite_w = 13;
			sprite_h = 13;
			delta_x = 6;
			delta_y = 8;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 3;
		}
		else
		{
			sprite_w = 15;
			sprite_h = 15;
			delta_x = 7;
			delta_y = 9;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 4;
		}

		int keyb_w = 21 * (sprite_w - 1) / 2 + 1;
		int keyb_h = 2 * delta_y + sprite_h;

		return keyb_w;
	}

	int Height(int width, int height) const
	{
		int sprite_w = 2 * (width - 1) / 21 + 1;
		int sprite_h = 0;
		int sprite_i = 0;
		int delta_x = 0;
		int delta_y = 0;
		int delta_d = 0;
		int caps_dy = 0;

		if (sprite_w < 9)
		{
			sprite_w = 7;
			sprite_h = 8;
			delta_x = 3;
			delta_y = 5;
			delta_d = 0;
			caps_dy = 0;
			sprite_i = 0;
		}
		else
		if (sprite_w < 11)
		{
			sprite_w = 9;
			sprite_h = 8;
			delta_x = 4;
			delta_y = 5;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 1;
		}
		else
		if (sprite_w < 13)
		{
			sprite_w = 11;
			sprite_h = 10;
			delta_x = 5;
			delta_y = 6;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 2;
		}
		else
		if (sprite_w < 15)
		{
			sprite_w = 13;
			sprite_h = 13;
			delta_x = 6;
			delta_y = 8;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 3;
		}
		else
		{
			sprite_w = 15;
			sprite_h = 15;
			delta_x = 7;
			delta_y = 9;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 4;
		}

		int keyb_w = 21 * (sprite_w - 1) / 2 + 1;
		int keyb_h = 2 * delta_y + sprite_h;

		return keyb_h;
	}

	void Paint(AnsiCell* ptr, int width, int height, int hide, const uint8_t key[32], bool gamepad) const
	{
		// hide should be netween 0 and Height()

		// shift modifies appeariance of space->BS and enter->LF, (possibly caps az->AZ)
		bool shift_on = key[A3D_LSHIFT >> 3] & (1 << (A3D_LSHIFT & 7));
		shift_on |= key[A3D_RSHIFT >> 3] & (1 << (A3D_RSHIFT & 7));


		int sprite_w = 2 * (width - 1) / 21 + 1;
		int sprite_h = 0;
		int sprite_i = 0;
		int delta_x = 0;
		int delta_y = 0;
		int delta_d = 0;
		int caps_dy = 0;

		if (sprite_w < 9)
		{
			sprite_w = 7;
			sprite_h = 8;
			delta_x = 3;
			delta_y = 5;
			delta_d = 0;
			caps_dy = 0;
			sprite_i = 0;
		}
		else
		if (sprite_w < 11)
		{
			sprite_w = 9;
			sprite_h = 8;
			delta_x = 4;
			delta_y = 5;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 1;
		}
		else
		if (sprite_w < 13)
		{
			sprite_w = 11;
			sprite_h = 10;
			delta_x = 5;
			delta_y = 6;
			delta_d = 1;
			caps_dy = 0;
			sprite_i = 2;
		}
		else
		if (sprite_w < 15)
		{
			sprite_w = 13;
			sprite_h = 13;
			delta_x = 6;
			delta_y = 8;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 3;
		}
		else
		{
			sprite_w = 15;
			sprite_h = 15;
			delta_x = 7;
			delta_y = 9;
			delta_d = 1;
			caps_dy = 1;
			sprite_i = 4;
		}

		int keyb_w = 21 * (sprite_w - 1) / 2 + 1;
		int keyb_h = 2 * delta_y + sprite_h;

		int center_x = (width - keyb_w) / 2;


		/*
		static int press_j = -1;
		static int press_i = -1;

		static int clicker = 0;
		clicker++;
		if (clicker == 10)
		{
			press_j = fast_rand() % 3;
			press_i = fast_rand() % 10;
		}
		if (clicker == 15)
		{
			clicker = 0;
			press_j = -1;
			press_i = -1;
		}
		*/

		for (int j = 2; j >= 0; j--)
		{
			for (int i = 9; i >= 0; i--)
			{
				int x = center_x + i * (sprite_w - 1) + (j & 1) * delta_x;
				int y = j * delta_y - hide;

				Sprite::Frame* sf = keyb_sprite[sprite_i]->atlas;

				bool press = false;

				int clip[] = { 0,0,sprite_w,sprite_h };
				if (gamepad)
				{
					if (j == 2)  // 4
					{
						if (sect == 0 && i < 4 ||
							sect == 1 && i >= 3 && i < 7 ||
							sect == 2 && i >= 6 && i < 10)
						{
							bool hi = false;
							int k = i - 3 * sect;
							switch (dir)
							{
								case 9: hi = k == 0; break; // leftmost
								case 10: hi = k == 1; break; // left
								case 1: hi = k == 2; break; // right
								case 2: hi = k == 3; break; // rightmost
							}

							if (hi)
							{
								clip[0] = 2 * sprite_w;
								clip[2] = 3 * sprite_w;
								if (!press)
									y -= delta_d;
								press = true;
							}
							else
							{
								clip[0] = sprite_w;
								clip[2] = 2 * sprite_w;
							}
						}
					}
					else
					if (j == 1) // 3
					{
						if (sect == 0 && i < 3 ||
							sect == 1 && i >= 3 && i < 6 ||
							sect == 2 && i >= 6 && i < 9)
						{
							bool hi = false;
							int k = i - 3 * sect;
							switch (dir)
							{
								case 8: hi = k == 0; break; // left
								case 0: hi = k == 1; break; // center
								case 3: hi = k == 2; break; // right
							}

							if (hi)
							{
								clip[0] = 2 * sprite_w;
								clip[2] = 3 * sprite_w;
								if (!press)
									y -= delta_d;
								press = true;
							}
							else
							{
								clip[0] = sprite_w;
								clip[2] = 2 * sprite_w;
							}
						}
					}
					else 
					if (j == 0) // 4
					{
						if (sect == 0 && i < 4 ||
							sect == 1 && i >= 3 && i < 7 ||
							sect == 2 && i >= 6 && i < 10)
						{
							bool hi = false;
							int k = i - 3 * sect;
							switch (dir)
							{
								case 7: hi = k == 0; break; // leftmost
								case 6: hi = k == 1; break; // left
								case 5: hi = k == 2; break; // right
								case 4: hi = k == 3; break; // rightmost
							}

							if (hi)
							{
								clip[0] = 2 * sprite_w;
								clip[2] = 3 * sprite_w;
								if (!press)
									y -= delta_d;
								press = true;
							}
							else
							{
								clip[0] = sprite_w;
								clip[2] = 2 * sprite_w;
							}
						}
					}
				}

				int cap = caps_plane[plane][2-j][i];

				if (key[cap>>3] & (1<<(cap&7)))
				{
					if (!press)
						y -= delta_d;
					clip[0] = 2 * sprite_w;
					clip[2] = 3 * sprite_w;
				}

				BlitSprite(ptr, width, height, sf, x, y, clip);

				int caps_clip[] = {i*5, j*5, (i+1) * 5, (j+1) * 5 };

				Sprite::Frame* caps_sf = caps_sprite[plane]->atlas;
				BlitSprite(ptr, width, height, caps_sf, x + sprite_w/2 - 2, y + sprite_h/2 - 2 + caps_dy, caps_clip);
			}
		}

	}
};

Keyb keyb;

////////////////////////////////////////////////////////////////////////////////////


// here we need all character sprites
// ...



Sprite* player_nude = 0;

Sprite* player[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* player_fall[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* player_attack[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };

Sprite* wolfie[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* wolfie_attack[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* wolfie_fall[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 }; // todo

Sprite* bigbee[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* bigbee_attack[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* bigbee_fall[2][ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 }; // todo

// dismount immediately when wolfie and/or player dies, evaluate them separately!
// wolfie dismounting should separate wolfie -> player + wolf

Sprite* wolf[2] = { 0 }; // 2 clr
Sprite* wolf_attack = 0; // todo
Sprite* wolf_fall = 0;   // todo

Sprite* bee[2] = { 0 }; // 2 clr
Sprite* bee_attack = 0; // todo
Sprite* bee_fall = 0;   // todo

Sprite* player_naked = 0; // what to do?

Sprite* character_button = 0;
Sprite* inventory_sprite = 0;
Sprite* fire_sprite = 0;

Sprite* LoadSpriteBP(const char* name, const uint8_t* recolor, bool detached)
{
	char path[1024];
	sprintf(path,"%ssprites/%s", base_path, name);

#ifdef EDITOR
	recolor = 0;
#endif
	return LoadSprite(path,name,recolor,detached);
}

#define LOAD_SPRITE(n) LoadSpriteBP(n, 0, false);

void LoadSprites()
{
#ifdef _WIN32
	_set_printf_count_output(1);
#endif

	// load other sprtes...
	LoadFont1();
	LoadGamePad();

	// main buts
	character_button = LoadSpriteBP("character.xp", 0, false);
	inventory_sprite = LoadSpriteBP("inventory.xp", 0, false);

	keyb_sprite[0] = LoadSpriteBP("keyb-07.xp", 0, false);
	keyb_sprite[1] = LoadSpriteBP("keyb-09.xp", 0, false);
	keyb_sprite[2] = LoadSpriteBP("keyb-11.xp", 0, false);
	keyb_sprite[3] = LoadSpriteBP("keyb-13.xp", 0, false);
	keyb_sprite[4] = LoadSpriteBP("keyb-15.xp", 0, false);

	caps_sprite[0] = LoadSpriteBP("keyb-caps-a.xp", 0, false);
	caps_sprite[1] = LoadSpriteBP("keyb-caps-b.xp", 0, false);
	caps_sprite[2] = LoadSpriteBP("keyb-caps-c.xp", 0, false);

	fire_sprite = LoadSpriteBP("fire.xp", 0, false);

	player_nude = LoadSpriteBP("player-nude.xp", 0, false);

	uint8_t wolf_recolor[] = { 2, 85,85,85, 51,51,51, 170,170,170, 102,102,102, 0,0 };
	wolf[0] = LoadSpriteBP("wolfie.xp", 0, false);
	wolf[1] = LoadSpriteBP("wolfie.xp", wolf_recolor, false);

	bee[0] = LoadSpriteBP("bigbee.xp", 0, false);
	bee[1] = 0;


	// uint8_t enemy_recolor[] = { 4, 170,0,170, 153,51,102, 0,0,170, 0,51,102, 85,85,255, 51,102,153, 255,85,85, 204,102,102 };
	uint8_t enemy_recolor[] = { 4, 170,0,170, 153,0,0, 0,0,170, 0,0,0, 85,85,255, 51,51,51, 255,85,85, 204,102,102, 
								'@','#', 'v','^', '^','v', 191,217, 217,191,  192,218, 218,192, 0,0};
	uint8_t* recolor[2] = { 0, enemy_recolor };

	for (int a = 0; a < ARMOR::SIZE; a++)
	{
		for (int h = 0; h < HELMET::SIZE; h++)
		{
			for (int s = 0; s < SHIELD::SIZE; s++)
			{
				char name[64];

				for (int c = 0; c < 2; c++) // color
				{

					for (int w = 0; w < WEAPON::SIZE; w++)
					{
						sprintf(name, "player-%x%x%x%x.xp", a, h, s, w);
						player[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);

						sprintf(name, "plydie-%x%x%x%x.xp", a, h, s, w);
						player_fall[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);

						sprintf(name, "wolfie-%x%x%x%x.xp", a, h, s, w);
						wolfie[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);
						wolfie_fall[c][a][h][s][w] = 0;

						sprintf(name, "bigbee-%x%x%x%x.xp", a, h, s, w);
						bigbee[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);
						bigbee_fall[c][a][h][s][w] = 0;
					}

					player_attack[c][a][h][s][WEAPON::NONE] = 0;
					wolfie_attack[c][a][h][s][WEAPON::NONE] = 0;
					for (int w = 1; w < WEAPON::SIZE; w++)
					{
						sprintf(name, "attack-%x%x%x%x.xp", a, h, s, w);
						player_attack[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);

						sprintf(name, "wolack-%x%x%x%x.xp", a, h, s, w);
						wolfie_attack[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);

						//sprintf(name, "beeack-%x%x%x%x.xp", a, h, s, w);
						//bigbee_attack[c][a][h][s][w] = LoadSpriteBP(name, recolor[c], false);
						bigbee_attack[c][a][h][s][w] = 0;
					}
				}
			}
		}
	}

	// world sprites
	Sprite* item_sword =  LOAD_SPRITE("item-sword.xp");
	Sprite* item_shield = LOAD_SPRITE("item-shield.xp");
	Sprite* item_hammer = LOAD_SPRITE("item-hammer.xp");
	Sprite* item_helmet = LOAD_SPRITE("item-helmet.xp");
	Sprite* item_mace =   LOAD_SPRITE("item-mace.xp");
	Sprite* item_axe =    LOAD_SPRITE("item-axe.xp");
	Sprite* item_armor =  LOAD_SPRITE("item-armor.xp");
	Sprite* item_crossbow  = LOAD_SPRITE("item-crossbow.xp");
	Sprite* item_flail =  LOAD_SPRITE("item-flail.xp");

	Sprite* item_white_ring = LOAD_SPRITE("item-white-ring.xp");
	Sprite* item_cyan_ring = LOAD_SPRITE("item-cyan-ring.xp");
	Sprite* item_gold_ring = LOAD_SPRITE("item-gold-ring.xp");
	Sprite* item_pink_ring = LOAD_SPRITE("item-pink-ring.xp");

	Sprite* item_meat =   LOAD_SPRITE("item-meat.xp");
	Sprite* item_egg =    LOAD_SPRITE("item-egg.xp");
	Sprite* item_cheese = LOAD_SPRITE("item-cheese.xp");
	Sprite* item_bread =  LOAD_SPRITE("item-bread.xp");
	Sprite* item_beet =   LOAD_SPRITE("item-beet.xp");
	Sprite* item_cucumber = LOAD_SPRITE("item-cucumber.xp");
	Sprite* item_carrot = LOAD_SPRITE("item-carrot.xp");
	Sprite* item_apple =  LOAD_SPRITE("item-apple.xp");
	Sprite* item_cherry = LOAD_SPRITE("item-cherry.xp");
	Sprite* item_plum =   LOAD_SPRITE("item-plum.xp");
	Sprite* item_milk =   LOAD_SPRITE("item-milk.xp");
	Sprite* item_water =  LOAD_SPRITE("item-water.xp");
	Sprite* item_wine =   LOAD_SPRITE("item-wine.xp");
	Sprite* item_red_potion = LOAD_SPRITE("item-red-potion.xp");
	Sprite* item_blue_potion = LOAD_SPRITE("item-blue-potion.xp");
	Sprite* item_green_potion = LOAD_SPRITE("item-green-potion.xp");
	Sprite* item_pink_potion = LOAD_SPRITE("item-pink-potion.xp");
	Sprite* item_cyan_potion = LOAD_SPRITE("item-cyan-potion.xp");
	Sprite* item_gold_potion = LOAD_SPRITE("item-gold-potion.xp");
	Sprite* item_grey_potion = LOAD_SPRITE("item-grey-potion.xp");

	// inventory sprites
	// weapons_2x3
	Sprite* grid_big_mace = LOAD_SPRITE("grid-big-mace.xp");
	Sprite* grid_big_hammer = LOAD_SPRITE("grid-big-hammer.xp");
	Sprite* grid_big_axe = LOAD_SPRITE("grid-big-axe.xp");

	// weapons_1x3
	Sprite* grid_alpha_sword = LOAD_SPRITE("grid-alpha-sword.xp");
	Sprite* grid_plus_sword = LOAD_SPRITE("grid-plus-sword.xp");
	Sprite* grid_small_mace = LOAD_SPRITE("grid-small-mace.xp");

	// weapons_1x2
	Sprite* grid_small_sword = LOAD_SPRITE("grid-small-sword.xp");
	Sprite* grid_small_saber = LOAD_SPRITE("grid-small-saber.xp");
	Sprite* grid_lumber_axe = LOAD_SPRITE("grid-lumber-axe.xp");

	// armory_2x2
	Sprite* grid_light_helmet = LOAD_SPRITE("grid-light-helmet.xp");
	Sprite* grid_heavy_helmet = LOAD_SPRITE("grid-heavy-helmet.xp");
	Sprite* grid_light_shield = LOAD_SPRITE("grid-light-shield.xp");
	Sprite* grid_heavy_shield = LOAD_SPRITE("grid-heavy-shield.xp");
	Sprite* grid_light_armor = LOAD_SPRITE("grid-light-armor.xp");
	Sprite* grid_heavy_armor = LOAD_SPRITE("grid-heavy-armor.xp");

	// weapons_2x2
	Sprite* grid_crossbow = LOAD_SPRITE("grid-crossbow.xp");
	Sprite* grid_flail = LOAD_SPRITE("grid-flail.xp");

	// rings_1x1
	Sprite* grid_white_ring = LOAD_SPRITE("grid-white-ring.xp");
	Sprite* grid_cyan_ring = LOAD_SPRITE("grid-cyan-ring.xp");
	Sprite* grid_gold_ring = LOAD_SPRITE("grid-gold-ring.xp");
	Sprite* grid_pink_ring = LOAD_SPRITE("grid-pink-ring.xp");

	// food_2x2
	Sprite* grid_meat = LOAD_SPRITE("grid-meat.xp");
	Sprite* grid_egg = LOAD_SPRITE("grid-egg.xp");
	Sprite* grid_cheese = LOAD_SPRITE("grid-cheese.xp");
	Sprite* grid_bread = LOAD_SPRITE("grid-bread.xp");
	Sprite* grid_beet = LOAD_SPRITE("grid-beet.xp");
	Sprite* grid_cucumber = LOAD_SPRITE("grid-cucumber.xp");
	Sprite* grid_carrot = LOAD_SPRITE("grid-carrot.xp");
	Sprite* grid_apple = LOAD_SPRITE("grid-apple.xp");
	Sprite* grid_cherry = LOAD_SPRITE("grid-cherry.xp");
	Sprite* grid_plum = LOAD_SPRITE("grid-plum.xp");

	// drinks_2x2
	Sprite* grid_milk = LOAD_SPRITE("grid-milk.xp");
	Sprite* grid_water = LOAD_SPRITE("grid-water.xp");
	Sprite* grid_wine = LOAD_SPRITE("grid-wine.xp");

	// potions_2x2
	Sprite* grid_red_potion = LOAD_SPRITE("grid-red-potion.xp");
	Sprite* grid_blue_potion = LOAD_SPRITE("grid-blue-potion.xp");
	Sprite* grid_green_potion = LOAD_SPRITE("grid-green-potion.xp");
	Sprite* grid_pink_potion = LOAD_SPRITE("grid-pink-potion.xp");
	Sprite* grid_cyan_potion = LOAD_SPRITE("grid-cyan-potion.xp");
	Sprite* grid_gold_potion = LOAD_SPRITE("grid-gold-potion.xp");
	Sprite* grid_grey_potion = LOAD_SPRITE("grid-grey-potion.xp");

	static const ItemProto item_proto[] = 
	{
	//  {kind, sub,                        weight, 3d_sprite,    2d_sprite,   desc}
		{ 'W', PLAYER_WEAPON_INDEX::MACE,     20000, item_mace,     grid_big_mace,        "Giant's Mace" },
		{ 'W', PLAYER_WEAPON_INDEX::HAMMER,   20000, item_hammer,   grid_big_hammer,      "Giant's Hammer" },
		{ 'W', PLAYER_WEAPON_INDEX::HAMMER,   20000, item_hammer,   grid_big_axe,         "Giant's Axe" },
		{ 'W', PLAYER_WEAPON_INDEX::SWORD,    5000,  item_sword,    grid_alpha_sword,     "Alpha Sword" },
		{ 'W', PLAYER_WEAPON_INDEX::SWORD,    5000,  item_sword,    grid_plus_sword,      "Plus Sword" },
		{ 'W', PLAYER_WEAPON_INDEX::MACE,     6000,  item_mace,     grid_small_mace,      "Small Mace" },
		{ 'W', PLAYER_WEAPON_INDEX::SWORD,    3000,  item_sword,    grid_small_sword,     "Small Sword" },
		{ 'W', PLAYER_WEAPON_INDEX::SWORD,    3000,  item_sword,    grid_small_saber,     "Small Saber" },
		{ 'W', PLAYER_WEAPON_INDEX::AXE,      3000,  item_axe,      grid_lumber_axe,      "Lumber Axe" },
		{ 'W', PLAYER_WEAPON_INDEX::CROSSBOW, 4000,  item_crossbow, grid_crossbow,        "Crossbow" },
		{ 'W', PLAYER_WEAPON_INDEX::FLAIL,    5000,  item_flail,    grid_flail,	          "Flail" },

		{ 'R', PLAYER_RING_INDEX::RING_WHITE, 10, item_white_ring, grid_white_ring,  "Unidentified White Ring" },
		{ 'R', PLAYER_RING_INDEX::RING_CYAN,  10, item_cyan_ring,  grid_cyan_ring,   "Unidentified Cyan Ring" },
		{ 'R', PLAYER_RING_INDEX::RING_GOLD,  10, item_gold_ring,  grid_gold_ring,   "Unidentified Gold Ring" },
		{ 'R', PLAYER_RING_INDEX::RING_PINK,  10, item_pink_ring,  grid_pink_ring,   "Unidentified Pink Ring" },

		{ 'H', PLAYER_HELMET_INDEX::HELMET_NORMAL, 1000, item_helmet, grid_light_helmet, "Light Helmet" },
		{ 'H', PLAYER_HELMET_INDEX::HELMET_NORMAL, 2000, item_helmet, grid_heavy_helmet, "Heavy Helmet" },

		{ 'S', PLAYER_SHIELD_INDEX::SHIELD_NORMAL, 4000, item_shield, grid_light_shield, "Light Shield" },
		{ 'S', PLAYER_SHIELD_INDEX::SHIELD_NORMAL, 6000, item_shield, grid_heavy_shield, "Heavy Shield" },

		{ 'A', PLAYER_ARMOR_INDEX::ARMOR_NORMAL,   5000, item_armor, grid_light_armor,   "Light Armor" },
		{ 'A', PLAYER_ARMOR_INDEX::ARMOR_NORMAL,  10000, item_armor, grid_heavy_armor,   "Heavy Armor" },

		{ 'F', PLAYER_FOOD_INDEX::MEAT,     500, item_meat,     grid_meat,     "Meat" },
		{ 'F', PLAYER_FOOD_INDEX::EGG,      250, item_egg,      grid_egg,      "Egg" },
		{ 'F', PLAYER_FOOD_INDEX::CHEESE,   300, item_cheese,   grid_cheese,   "Cheese" },
		{ 'F', PLAYER_FOOD_INDEX::BREAD,    500, item_bread,    grid_bread,    "Bread" },
		{ 'F', PLAYER_FOOD_INDEX::BEET,     500, item_beet,     grid_beet,     "Beet" },
		{ 'F', PLAYER_FOOD_INDEX::CUCUMBER, 250, item_cucumber, grid_cucumber, "Cucumber" },
		{ 'F', PLAYER_FOOD_INDEX::CARROT,   250, item_carrot,   grid_carrot,   "Carrot" },
		{ 'F', PLAYER_FOOD_INDEX::APPLE,    200, item_apple,    grid_apple,    "Apple" },
		{ 'F', PLAYER_FOOD_INDEX::CHERRY,   100, item_cherry,   grid_cherry,   "Cherry" },
		{ 'F', PLAYER_FOOD_INDEX::PLUM,     100, item_plum,     grid_plum,     "Plum" },

		{ 'D', PLAYER_DRINK_INDEX::MILK,    250, item_milk,     grid_milk,    "Milk" },
		{ 'D', PLAYER_DRINK_INDEX::WATER,   250, item_water,    grid_water,   "Water" },
		{ 'D', PLAYER_DRINK_INDEX::WINE,    250, item_wine,     grid_wine,    "Wine" },

		{ 'P', PLAYER_POTION_INDEX::POTION_RED,    150, item_red_potion,    grid_red_potion,   "Healing Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_BLUE,   150, item_blue_potion,   grid_blue_potion,  "Mana Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_GREEN,  150, item_green_potion,  grid_green_potion, "Unidentified Green Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_PINK,   150, item_pink_potion,   grid_pink_potion,  "Unidentified Pink Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_CYAN,   150, item_cyan_potion,   grid_cyan_potion,  "Unidentified Cyan Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_GOLD,   150, item_gold_potion,   grid_gold_potion,  "Unidentified Gold Potion" },
		{ 'P', PLAYER_POTION_INDEX::POTION_GREY,   150, item_grey_potion,   grid_grey_potion,  "Unidentified Grey Potion" },

		{ 0 }
	};

	item_proto_lib = item_proto;
}

Sprite* GetSprite(const SpriteReq* req, int clr)
{
	assert(req);

	if (req->kind == SpriteReq::WOLF)
	{
		if (req->action == ACTION::NONE &&
			req->weapon == WEAPON::NONE &&
			req->shield == SHIELD::NONE &&
			req->helmet == HELMET::NONE &&
			req->armor == ARMOR::NONE &&
			req->mount == MOUNT::NONE)
		{
			return wolf[clr];
		}

		return 0;
	}

	if (req->kind == SpriteReq::BEE)
	{
		if (req->action == ACTION::NONE &&
			req->weapon == WEAPON::NONE &&
			req->shield == SHIELD::NONE &&
			req->helmet == HELMET::NONE &&
			req->armor == ARMOR::NONE &&
			req->mount == MOUNT::NONE)
		{
			return bee[clr];
		}

		return 0;
	}

	if (req->action < 0 || req->action >= ACTION::SIZE)
		return 0;

	if (req->weapon < 0 || req->weapon >= WEAPON::SIZE)
		return 0;

	if (req->shield < 0 || req->shield >= SHIELD::SIZE)
		return 0;

	if (req->helmet < 0 || req->helmet >= HELMET::SIZE)
		return 0;

	if (req->armor < 0 || req->armor >= ARMOR::SIZE)
		return 0;

	switch (req->mount)
	{
		case MOUNT::NONE:
		{
			switch (req->action)
			{
				case ACTION::NONE:
					return player[clr][req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::ATTACK:
					if (req->weapon == WEAPON::REGULAR_CROSSBOW)
						return player[clr][req->armor][req->helmet][req->shield][req->weapon];
					else
						return player_attack[clr][req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::FALL:
				case ACTION::DEAD:
				case ACTION::STAND:
					return player_fall[clr][req->armor][req->helmet][req->shield][req->weapon];
			}
			return 0;
		}

		case MOUNT::WOLF:
		{
			switch (req->action)
			{
				case ACTION::NONE:
					return wolfie[clr][req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::ATTACK:
					if (req->weapon == WEAPON::REGULAR_CROSSBOW)
						return wolfie[clr][req->armor][req->helmet][req->shield][req->weapon];
					else
						return wolfie_attack[clr][req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::FALL:
				case ACTION::DEAD:
				case ACTION::STAND:
					return wolfie_fall[clr][req->armor][req->helmet][req->shield][req->weapon];
			}	
			return 0;
		}

		case MOUNT::BEE:
		{
			switch (req->action)
			{
			case ACTION::NONE:
				return bigbee[clr][req->armor][req->helmet][req->shield][req->weapon];

			case ACTION::ATTACK:
				if (req->weapon == WEAPON::REGULAR_CROSSBOW)
					return bigbee[clr][req->armor][req->helmet][req->shield][req->weapon];
				else
					return bigbee_attack[clr][req->armor][req->helmet][req->shield][req->weapon];

			case ACTION::FALL:
			case ACTION::DEAD:
			case ACTION::STAND:
				return bigbee_fall[clr][req->armor][req->helmet][req->shield][req->weapon];
			}
			return 0;
		}
	}

	return 0;
}

void FreeSprites()
{
	FreeFont1();
	FreeGamePad();

	// handles double refs but not sprite prefs!
	while (Sprite* s = GetFirstSprite())
		FreeSprite(s);
}

Game* CreateGame(int water, float pos[3], float yaw, float dir, uint64_t stamp)
{
	// load defaults
	Game* g = (Game*)malloc(sizeof(Game));
	memset(g, 0, sizeof(Game));

	g->menu_depth = -1;

	g->perspective = true;
	g->blood = true;

	fast_srand(stamp);

#ifndef EDITOR
	EnemyGen* eg = enemygen_head;
	while (eg)
	{
		Character* enemy_master = 0;
		for (int i = 0; i < eg->alive_max; i++)
		{
			NPC_Human* enemy = (NPC_Human*)malloc(sizeof(NPC_Human));
			memset(enemy, 0, sizeof(NPC_Human));

			// init enemy

			enemy->MAX_HP = 100;
			enemy->HP = (i + 1) * enemy->MAX_HP / eg->alive_max;

			enemy->enemy = true;

			//enemy->master = enemy_master;
			enemy->master = 0;

			enemy->target = enemy->master;
			enemy->followers = 0;

			if (i == 0)
				enemy_master = enemy;

			enemy->clr = 1;
			enemy->req.kind = SpriteReq::HUMAN;
			enemy->req.mount = MOUNT::NONE;
			enemy->req.armor = fast_rand() % 11 < eg->armor ? ARMOR::NONE : ARMOR::REGULAR_ARMOR;
			enemy->req.helmet = fast_rand() % 11 < eg->helmet ? HELMET::NONE : HELMET::REGULAR_HELMET;
			enemy->req.shield = fast_rand() % 11 < eg->shield ? SHIELD::NONE : SHIELD::REGULAR_SHIELD;
			enemy->req.weapon = fast_rand() % (eg->sword + eg->crossbow + 1) < eg->sword ?
				WEAPON::REGULAR_SWORD : WEAPON::REGULAR_CROSSBOW;
			enemy->req.action = ACTION::NONE;

			if (enemy->req.armor)
			{
				Item* item = CreateItem();
				item->count = 1;
				item->inst = 0;
				item->proto = item_proto_lib + rand() % 2 + 19;
				item->purpose = Item::OWNED;
				enemy->has[enemy->items].in_use = true;
				enemy->has[enemy->items].item = item;
				enemy->has[enemy->items].story_id = -1;
				enemy->items++;
			}
			if (enemy->req.helmet)
			{
				Item* item = CreateItem();
				item->count = 1;
				item->inst = 0;
				item->proto = item_proto_lib + rand() % 2 + 15;
				item->purpose = Item::OWNED;
				enemy->has[enemy->items].in_use = true;
				enemy->has[enemy->items].item = item;
				enemy->has[enemy->items].story_id = -1;
				enemy->items++;
			}
			if (enemy->req.shield)
			{
				Item* item = CreateItem();
				item->count = 1;
				item->inst = 0;
				item->proto = item_proto_lib + rand() % 2 + 17;
				item->purpose = Item::OWNED;
				enemy->has[enemy->items].in_use = true;
				enemy->has[enemy->items].item = item;
				enemy->has[enemy->items].story_id = -1;
				enemy->items++;
			}
			if (enemy->req.weapon)
			{
				// only sword at the moment
				Item* item = CreateItem();
				item->count = 1;
				item->inst = 0;

				if (enemy->req.weapon == WEAPON::REGULAR_SWORD)
				{
					int id = rand() % 4;
					if (id >= 2)
						id++; // there's a hole in sword ids :(
					item->proto = item_proto_lib + id + 3;
				}
				else
				{
					item->proto = item_proto_lib + 9;
				}

				item->purpose = Item::OWNED;
				enemy->has[enemy->items].in_use = true;
				enemy->has[enemy->items].item = item;
				enemy->has[enemy->items].story_id = -1;
				enemy->items++;
			}


			enemy->sprite = GetSprite(&enemy->req, enemy->clr);
			assert(enemy->sprite);
			enemy->anim = 0; // ???

			enemy->frame = 0;

			int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;
			int reps[4] = { 0,0,0,0 };

			float* e_pos = eg->pos;
			float xyz[3] = { e_pos[0] + fast_rand() % 21 - 10, e_pos[1] + fast_rand() % 21 - 10, e_pos[2] + 200 };

			enemy->gen = eg;

			enemy->pos[0] = enemy->unstuck[0][0] = enemy->unstuck[1][0] = xyz[0];
			enemy->pos[1] = enemy->unstuck[0][1] = enemy->unstuck[1][1] = xyz[1];
			enemy->pos[2] = enemy->unstuck[0][2] = enemy->unstuck[1][2] = xyz[2];
			enemy->inst = CreateInst(world, enemy->sprite, flags, xyz, yaw, enemy->anim, enemy->frame, reps, 0, -1);
			SetInstSpriteData(enemy->inst, enemy);

			for (int it = 0; it < enemy->items; it++)
				enemy->has[it].item->inst = enemy->inst;

			AttachInst(world, enemy->inst);

			enemy->prev = 0;
			enemy->next = player_head;

			if (!player_tail)
				player_tail = enemy;
			else
				player_tail->prev = enemy;

			player_head = enemy;

			enemy->data = CreatePhysics(terrain, world, xyz, 0, 0, stamp);
		}
		eg = eg->next;
	}
#endif


	#if 0
	
	int enemies = 4;
	for (int i = 0; i < enemies; i++) // 1 mater + 5 slaves
	{
		NPC_Human* enemy = (NPC_Human*)malloc(sizeof(NPC_Human));
		memset(enemy, 0, sizeof(NPC_Human));

		// init enemy

		enemy->MAX_HP = 100;
		enemy->HP = (i + 1) * enemy->MAX_HP / enemies;

		enemy->enemy = true;

		//enemy->master = enemy_master;
		enemy->master = 0;

		enemy->target = enemy->master;
		enemy->followers = 0;

		if (i == 0)
			enemy_master = enemy;

		int r = fast_rand();

		enemy->clr = 1;
		enemy->req.kind = SpriteReq::HUMAN;
		enemy->req.mount = MOUNT::NONE;// +((r >> 1) & 1);
		enemy->req.armor = ARMOR::NONE + ((r >> 2) & 1);
		enemy->req.helmet = HELMET::NONE + ((r >> 3) & 1);
		enemy->req.shield = SHIELD::NONE + ((r >> 4) & 1);
		enemy->req.weapon = WEAPON::NONE + 1;// ((r >> 5) % 3);
		enemy->req.action = ACTION::NONE;

		if (enemy->req.armor)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 19;
			item->purpose = Item::OWNED;
			enemy->has[enemy->items].in_use = true;
			enemy->has[enemy->items].item = item;
			enemy->has[enemy->items].story_id = -1;
			enemy->items++;
		}
		if (enemy->req.helmet)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 15;
			item->purpose = Item::OWNED;
			enemy->has[enemy->items].in_use = true;
			enemy->has[enemy->items].item = item;
			enemy->has[enemy->items].story_id = -1;
			enemy->items++;
		}
		if (enemy->req.shield)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 17;
			item->purpose = Item::OWNED;
			enemy->has[enemy->items].in_use = true;
			enemy->has[enemy->items].item = item;
			enemy->has[enemy->items].story_id = -1;
			enemy->items++;
		}
		if (enemy->req.weapon)
		{
			// only sword at the moment
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;

			if (enemy->req.weapon == WEAPON::REGULAR_SWORD)
			{
				int id = rand() % 4;
				if (id >= 2)
					id++; // there's a hole in sword ids :(
				item->proto = item_proto_lib + id + 3;
			}
			else
			{
				item->proto = item_proto_lib + 9;
				
			}

			item->purpose = Item::OWNED;
			enemy->has[enemy->items].in_use = true;
			enemy->has[enemy->items].item = item;
			enemy->has[enemy->items].story_id = -1;
			enemy->items++;
		}


		enemy->sprite = GetSprite(&enemy->req, enemy->clr);
		enemy->anim = 0; // ???

		enemy->frame = 0;

		int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;
		int reps[4] = { 0,0,0,0 };

		float e_pos[] = { -13.6, 202.2, 234.7 };
		float xyz[3] = { e_pos[0] + fast_rand() % 21 - 10, e_pos[1] + fast_rand() % 21 - 10, e_pos[2] + 200 };

		enemy->pos[0] = enemy->unstuck[0][0] = enemy->unstuck[1][0] = xyz[0];
		enemy->pos[1] = enemy->unstuck[0][1] = enemy->unstuck[1][1] = xyz[1];
		enemy->pos[2] = enemy->unstuck[0][2] = enemy->unstuck[1][2] = xyz[2];
		enemy->inst = CreateInst(world, enemy->sprite, flags, xyz, yaw, enemy->anim, enemy->frame, reps, 0, -1);
		SetInstSpriteData(enemy->inst, enemy);

		for (int it = 0; it < enemy->items; it++)
			enemy->has[it].item->inst = enemy->inst;

		AttachInst(world, enemy->inst);

		enemy->prev = 0;
		enemy->next = player_head;

		if (!player_tail)
			player_tail = enemy;
		else
			player_tail->prev = enemy;

		player_head = enemy;

		enemy->data = CreatePhysics(terrain, world, xyz, 0, 0, stamp);
	}
	#endif

	#ifndef EDITOR
	int buddies = 2;
	for (int i = 0; i < buddies; i++)
	{
		NPC_Human* buddy = (NPC_Human*)malloc(sizeof(NPC_Human));
		memset(buddy, 0, sizeof(NPC_Human));
		
		// init buddy!
		buddy->MAX_HP = 100;
		buddy->HP = (i+1) * buddy->MAX_HP / buddies;

		buddy->enemy = false;
		buddy->master = &g->player;
		buddy->target = buddy->master;
		buddy->followers = 0;

		int r = fast_rand();

		buddy->clr = 0;
		buddy->req.kind = SpriteReq::HUMAN;
		buddy->req.mount = MOUNT::NONE;// +((r >> 1) & 1);
		buddy->req.armor = ARMOR::NONE + ((r >> 2) & 1);
		buddy->req.helmet = HELMET::NONE + ((r >> 3) & 1);
		buddy->req.shield = SHIELD::NONE + ((r >> 4) & 1);
		buddy->req.weapon = WEAPON::NONE + 1; // ((r >> 5) % 3);
		buddy->req.action = ACTION::NONE;

		if (buddy->req.armor)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 19;
			item->purpose = Item::OWNED;
			buddy->has[buddy->items].in_use = true;
			buddy->has[buddy->items].item = item;
			buddy->has[buddy->items].story_id = -1;
			buddy->items++;
		}
		if (buddy->req.helmet)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 15;
			item->purpose = Item::OWNED;
			buddy->has[buddy->items].in_use = true;
			buddy->has[buddy->items].item = item;
			buddy->has[buddy->items].story_id = -1;
			buddy->items++;
		}
		if (buddy->req.shield)
		{
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;
			item->proto = item_proto_lib + rand() % 2 + 17;
			item->purpose = Item::OWNED;
			buddy->has[buddy->items].in_use = true;
			buddy->has[buddy->items].item = item;
			buddy->has[buddy->items].story_id = -1;
			buddy->items++;
		}
		if (buddy->req.weapon)
		{
			// only sword at the moment
			Item* item = CreateItem();
			item->count = 1;
			item->inst = 0;

			if (buddy->req.weapon == WEAPON::REGULAR_SWORD)
			{
				int id = rand() % 4;
				if (id >= 2)
					id++; // there's a hole in sword ids :(
				item->proto = item_proto_lib + id + 3;
			}
			else
			{
				item->proto = item_proto_lib + 9;
			}

			item->purpose = Item::OWNED;
			buddy->has[buddy->items].in_use = true;
			buddy->has[buddy->items].item = item;
			buddy->has[buddy->items].story_id = -1;
			buddy->items++;
		}

		buddy->sprite = GetSprite(&buddy->req, buddy->clr);
		buddy->anim = 0; // ???

		buddy->frame = 0;

		int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;
		int reps[4] = { 0,0,0,0 };

		float xyz[3] = { pos[0] + fast_rand()%21 - 10, pos[1] + fast_rand() % 21 - 10, 131 + 200 };

		buddy->pos[0] = buddy->unstuck[0][0] = buddy->unstuck[1][0] = xyz[0];
		buddy->pos[1] = buddy->unstuck[0][1] = buddy->unstuck[1][1] = xyz[1];
		buddy->pos[2] = buddy->unstuck[0][2] = buddy->unstuck[1][2] = xyz[2];
		buddy->inst = CreateInst(world, buddy->sprite, flags, xyz, yaw, buddy->anim, buddy->frame, reps, 0, -1);
		SetInstSpriteData(buddy->inst, buddy);

		for (int it = 0; it < buddy->items; it++)
			buddy->has[it].item->inst = buddy->inst;

		AttachInst(world, buddy->inst);

		buddy->prev = 0;
		buddy->next = player_head;

		if (!player_tail)
			player_tail = buddy;
		else
			player_tail->prev = buddy;

		player_head = buddy;

		buddy->data = CreatePhysics(terrain, world, xyz, 0, 0, stamp);
		//buddy->dist = 10; // 10 + i * 5;
	}
	#endif

	strcpy(g->player.name, player_name);
	strcpy(g->player.name_cp437, player_name_cp437);

	if (server)
	{
		server->last_lag = stamp;
		server->lag_ms = 0;
		server->lag_wait = false;
	}

	ReadConf(g); 

	g->player.prev = 0;
	g->player.next = player_head;
	if (player_head)
		player_head->prev = &g->player;
	else
		player_tail = &g->player;
	player_head = &g->player;

	// just initialized,
	// nowhere modified!
	g->show_buts = true;
	g->bars_pos = 7;

	int width = 112, height = 63;
	g->keyb_hide = 1000;// keyb.Height(width, height);

	g->renderer = CreateRenderer(stamp);
	g->physics = CreatePhysics(terrain, world, pos, dir, yaw, stamp);
	g->stamp = stamp;

	g->player.data = g->physics;

	// init player!
	g->player.MAX_HP = 200;
	g->player.HP = g->player.MAX_HP;
	g->player.master = 0;
	g->player.target = 0;
	g->player.followers = 0;
	g->player.enemy = false; // sounds ridiculous
	g->player.req.kind = SpriteReq::HUMAN;
	g->player.req.mount = MOUNT::NONE; //MOUNT::WOLF;
	g->player.req.armor = ARMOR::NONE;
	g->player.req.helmet = HELMET::NONE;
	g->player.req.shield = SHIELD::NONE; // REGULAR_SHIELD;
	g->player.req.weapon = WEAPON::NONE;
	g->player.req.action = ACTION::NONE;

	g->player.clr = 0;

	g->player.sprite = GetSprite(&g->player.req, g->player.clr);
	g->player.anim = 0; // ???

	g->player.frame = 0;

	g->water = water;
	g->prev_yaw = yaw;

	// fancy part
	// create player sprite instance inside world but never paint
	// this is to be used by other clients only!
	// renderer will hide its client sprite

	int flags = INST_USE_TREE | INST_VISIBLE | INST_VOLATILE;
	int reps[4] = { 0,0,0,0 };
	g->player_inst = CreateInst(world, g->player.sprite, flags, pos, yaw, g->player.anim, g->player.frame, reps, 0, -1/*not in story*/);
	SetInstSpriteData(g->player_inst, &g->player);

	if (!prime_game)
		prime_game = g;
	return g;
}

void DeleteGame(Game* g)
{
	if (g)
	{
		if (g == prime_game)
			prime_game = 0;

		DeleteInst(g->player_inst);

		for (int i = 0; i < g->inventory.my_items; i++)
			DestroyItem(g->inventory.my_item[i].item);

		for (int i = 0; i < g->player.talks; i++)
			free(g->player.talk[i].box);

		if (g->player.talk_box)
			free(g->player.talk_box);
		if (g->renderer)
			DeleteRenderer(g->renderer);
		if (g->physics)
			DeletePhysics(g->physics);

		if (g->player.prev)
			g->player.prev->next = g->player.next;
		else
			player_head = g->player.next;
		if (g->player.next)
			g->player.next->prev = g->player.prev;
		else
			player_tail = g->player.prev;

		#ifndef EDITOR
		Character* h = player_head;
		while (h)
		{
			Character* n = h->next;
			if (h->data != g->physics)
			{
				if (h->prev)
					h->prev->next = h->next;
				else
					player_head = h->next;

				if (h->next)
					h->next->prev = h->prev;
				else
					player_tail = h->prev;

				DeletePhysics((Physics*)h->data);

				ItemOwner* io = 0;
				if (h->req.kind == SpriteReq::HUMAN)
					io = (ItemOwner*)(NPC_Human*)h;
				else
					io = (ItemOwner*)(NPC_Creature*)h;

				for (int i = 0; i < io->items; i++)
				{
					io->has[i].item->inst = 0;
					DestroyItem(io->has[i].item);
				}

				free(h);
			}
			h = n;
		}
		#endif

		free(g);
	}
}

void Game::CancelItemContacts()
{
	for (int i=0; i<4; i++)
	{
		if (input.contact[i].action==Input::Contact::ITEM_GRID_CLICK ||
			input.contact[i].action==Input::Contact::ITEM_LIST_CLICK ||
			input.contact[i].action==Input::Contact::ITEM_GRID_DRAG ||
			input.contact[i].action==Input::Contact::ITEM_LIST_DRAG)
		input.contact[i].action = Input::Contact::NONE;
	}
}

void Game::ExecuteItem(int my_item)
{
	Item* item = inventory.my_item[my_item].item;
	switch (item->proto->kind)
	{
		case 'F': // food
		case 'P': // potion
		case 'D': // drink
		{
			if (item->count > 1)
				item->count--;
			else
			{
				// giving pos==null doesn't create world's instance and destroys item
				if (consume_anims==16)
				{
					memmove(consume_anim,consume_anim+1,sizeof(ConsumeAnim)*15);
					consume_anims--;
				}

				ConsumeAnim* a = consume_anim + consume_anims;

				a->pos[0] = inventory.my_item[my_item].xy[0];
				a->pos[1] = inventory.my_item[my_item].xy[1];
				a->sprite = inventory.my_item[my_item].item->proto->sprite_2d;
				a->stamp = stamp;
				consume_anims++;

				inventory.RemoveItem(my_item, 0, 0);
			}
			break;
		}

		case 'R':
		{
			inventory.my_item[my_item].in_use = !inventory.my_item[my_item].in_use;
			break;
		}

		case 'W':
		{
			if (inventory.my_item[my_item].in_use)
			{
				if (player.SetWeapon(PLAYER_WEAPON_INDEX::WEAPON_NONE))
				{
					inventory.my_item[my_item].in_use = false;
				}
			}
			else
			{
				if (player.SetWeapon(item->proto->sub_kind))
				{
					for (int i = 0; i < inventory.my_items; i++)
					{
						if (inventory.my_item[i].in_use && inventory.my_item[i].item->proto->kind == item->proto->kind)
						{
							inventory.my_item[i].in_use = false;
							break;
						}
					}
					inventory.my_item[my_item].in_use = true;
				}
			}
			break;
		}

		case 'S':
		{
			if (inventory.my_item[my_item].in_use)
			{
				if (player.SetShield(PLAYER_SHIELD_INDEX::SHIELD_NONE))
				{
					inventory.my_item[my_item].in_use = false;
				}
			}
			else
			{
				if (player.SetShield(item->proto->sub_kind))
				{
					for (int i = 0; i < inventory.my_items; i++)
					{
						if (inventory.my_item[i].in_use && inventory.my_item[i].item->proto->kind == item->proto->kind)
						{
							inventory.my_item[i].in_use = false;
							break;
						}
					}
					inventory.my_item[my_item].in_use = true;
				}
			}
			break;
		}

		case 'H':
		{
			if (inventory.my_item[my_item].in_use)
			{
				if (player.SetHelmet(PLAYER_HELMET_INDEX::HELMET_NONE))
				{
					inventory.my_item[my_item].in_use = false;
				}
			}
			else
			{
				if (player.SetHelmet(item->proto->sub_kind))
				{
					for (int i = 0; i < inventory.my_items; i++)
					{
						if (inventory.my_item[i].in_use && inventory.my_item[i].item->proto->kind == item->proto->kind)
						{
							inventory.my_item[i].in_use = false;
							break;
						}
					}
					inventory.my_item[my_item].in_use = true;
				}
			}
			break;
		}

		case 'A':
		{
			if (inventory.my_item[my_item].in_use)
			{
				if (player.SetArmor(PLAYER_ARMOR_INDEX::ARMOR_NONE))
				{
					inventory.my_item[my_item].in_use = false;
				}
			}
			else
			{
				if (player.SetArmor(item->proto->sub_kind))
				{
					for (int i = 0; i < inventory.my_items; i++)
					{
						if (inventory.my_item[i].in_use && inventory.my_item[i].item->proto->kind == item->proto->kind)
						{
							inventory.my_item[i].in_use = false;
							break;
						}
					}
					inventory.my_item[my_item].in_use = true;
				}
			}
			break;
		}
	}
}

int Game::CheckPick(const int cp[2])
{
	// given cp, returns index to inventory.my_item or -1
	Sprite::Frame* sf = inventory_sprite->atlas;

	int width = render_size[0];
	int height = render_size[1];

	int scroll = inventory.scroll;
	if (scroll < 0)
		scroll = 0;
	if (scroll > inventory.layout_max_scroll)
		scroll = inventory.layout_max_scroll;

	int left = inventory.layout_frame[0];
	int right = inventory.layout_frame[2];
	int upper = inventory.layout_frame[3];
	int lower = inventory.layout_frame[1];

	if (cp[0] > left && cp[0]<right && cp[1]>lower && cp[1] < upper)
	{
		int qx = cp[0] - (inventory.layout_x + 4);
		int qy = cp[1] - (inventory.layout_y + inventory.layout_height - 6 - inventory.height * 4 + 1 + scroll);

		for (int i = 0; i < inventory.my_items; i++)
		{
			Sprite::Frame* frame = inventory.my_item[i].item->proto->sprite_2d->atlas;
			int xy[2] = { inventory.my_item[i].xy[0] * 4, inventory.my_item[i].xy[1] * 4 };
			if (qx >= xy[0] && qx < xy[0] + frame->width && qy >= xy[1] && qy < xy[1] + frame->height)
			{
				return i;
			}
		}
	}

	return -1;
}

bool Game::CheckDrop(int c, int drop_xy[2], AnsiCell* ptr, int width, int height)
{
	if (input.contact[c].action == Input::Contact::ITEM_LIST_DRAG ||
		input.contact[c].action == Input::Contact::ITEM_GRID_DRAG)
	{
		Sprite::Frame* sf = inventory_sprite->atlas;

		int scroll = inventory.scroll;
		if (scroll < 0)
			scroll = 0;
		if (scroll > inventory.layout_max_scroll)
			scroll = inventory.layout_max_scroll;

		int cp[2] = { input.contact[c].pos[0], input.contact[c].pos[1] };
		ScreenToCell(cp);

		int left = inventory.layout_frame[0];
		int right = inventory.layout_frame[2];
		int upper = inventory.layout_frame[3];
		int lower = inventory.layout_frame[1];

		if (cp[0] > left && cp[0]<right && cp[1]>lower && cp[1] < upper)
		{
			Item* item = input.contact[c].item;
			Sprite::Frame* frame = item->proto->sprite_2d->atlas;

			// shift coords to bitmask coords
			int qx = cp[0] - (inventory.layout_x + 4);
			int qy = cp[1] - (inventory.layout_y + inventory.layout_height - 6 - inventory.height * 4 + 1 + scroll);

			qx -= frame->width / 2;
			qy -= frame->height / 2;

			int qw = (frame->width + 1) / 4;
			int qh = (frame->height + 1) / 4;

			qx = (qx + 2) / 4;
			qy = (qy + 2) / 4;

			if (qx < 0)
				qx = 0;
			if (qx + qw > inventory.width)
				qx = inventory.width - qw;
			if (qy < 0)
				qy = 0;
			if (qy + qh >= inventory.height)
				qy = inventory.height - qh;

			// check bitmask
			// if no collision paint yellow, otherwise red
			bool fit = true;

			// TODO: 
			// merge test (same proto) upto proto->max_count (not in struct yet)
			// ...

			// only non-split action must take care about extra 'self' space 
			if (input.contact[c].action == Input::Contact::ITEM_GRID_DRAG 
				&& (input.contact[c].drag == 1 || input.contact[c].item->count==1)) 
			{
				int* xy = inventory.my_item[input.contact[c].my_item].xy;

				for (int my = qy; fit && my < qy + qh; my++)
				{
					for (int mx = qx; mx < qx + qw; mx++)
					{
						if (mx >= xy[0] && my >= xy[1] && mx < xy[0] + qw && my < xy[1] + qh)
							continue;

						int m = mx + my * inventory.width;

						if (inventory.bitmask[m >> 3] & (1 << (m & 7)))
						{
							fit = false;
							break;
						}
					}
				}
			}
			else
			{
				for (int my = qy; fit && my < qy + qh; my++)
				{
					for (int mx = qx; mx < qx + qw; mx++)
					{
						int m = mx + my * inventory.width;
						if (inventory.bitmask[m >> 3] & (1 << (m & 7)))
						{
							fit = false;
							break;
						}
					}
				}
			}

			if (drop_xy)
			{
				drop_xy[0] = qx;
				drop_xy[1] = qy;
			}

			if (fit && ptr)
			{
				// REVERSE
				qx = qx * 4 + (inventory.layout_x + 4);
				qy = qy * 4 + (inventory.layout_y + inventory.layout_height - 6 - inventory.height * 4 + 1 + scroll);
				qw = qw * 4 - 1;
				qh = qh * 4 - 1;

				if (qx < 0)
				{
					qw -= qx;
					qx = 0;
				}
				if (qx + qw > width)
					qw = width - qx;

				if (qy < lower + 1)
				{
					qh = qy + qh - (lower + 1);
					qy = lower + 1;
				}
				if (qy + qh > upper)
					qh = upper - qy;

				for (int sy = 0; sy < qh; sy++)
				{
					for (int sx = 0; sx < qw; sx++)
					{
						ptr[qx + sx + (qy + sy)*width].fg = yellow;
					}
				}
			}

			return fit;
		}
		else
		{
			if (ptr && input.contact[c].action == Input::Contact::ITEM_LIST_DRAG)
			{
				if (cp[0] < inventory.layout_x || 
					cp[0] >= inventory.layout_x + inventory.layout_width || 
					cp[1] < inventory.layout_y || 
					cp[1] >= inventory.layout_y + inventory.layout_height)
				{
					Item* item = input.contact[c].item;
					Sprite::Frame* frame = item->proto->sprite_2d->atlas;
					int y = cp[1] - frame->height/2 - 1;
					if (y>=0 && y<height)
					for (int i = 0; i < 5; i++)
					{
						int x = i + cp[0] - 2;
						if (x >= 0 && x < width)
						{
							AnsiCell* ind = ptr + x + y * width;
							ind->bk = AverageGlyph(ind, 0xF);
							ind->gl = "\x11PICK"[i];
							ind->fg = white;
						}
					}					
				}				
			}
			else
			// check if we can remove item from inventory
			// indicate it by returning true and xy set to -1
			if (input.contact[c].action == Input::Contact::ITEM_GRID_DRAG)
			{
				if (cp[0] < inventory.layout_x || 
					cp[0] >= inventory.layout_x + inventory.layout_width || 
					cp[1] < inventory.layout_y || 
					cp[1] >= inventory.layout_y + inventory.layout_height)
				{
					// TODO:
					// check for NPCs to indicate GIVE
					// we'd need to get a query of nearby sprites from render
					// and check if they are getters (characters)
					// PlayerHit(g)

					if (ptr && !inventory.my_item[input.contact[c].my_item].in_use)
					{
						Item* item = input.contact[c].item;
						Sprite::Frame* frame = item->proto->sprite_2d->atlas;
						int y = cp[1] - frame->height/2 - 1;
						if (y>=0 && y<height)
						for (int i = 0; i < 5; i++)
						{
							int x = i + cp[0] - 2;
							if (x >= 0 && x < width)
							{
								AnsiCell* ind = ptr + x + y * width;
								ind->bk = AverageGlyph(ind, 0xF);
								ind->gl = "DROP\x1F"[i];
								ind->fg = white;
							}
						}
					}

					if (drop_xy)
					{
						drop_xy[0] = -1;
						drop_xy[1] = -1;
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool Game::PickItem(Item* item)
{
	// automatically calculates xy in inventory
	int iw = (item->proto->sprite_2d->atlas->width + 1) / 4;
	int ih = (item->proto->sprite_2d->atlas->height + 1) / 4;

	int xx = inventory.width - iw + 1;
	for (int y = inventory.height - ih; y >= 0; y--)
	{
		for (int x = 0; x < xx; x++)
		{
			bool ok = true;
			for (int v = y; v < y + ih && ok; v++)
			{
				for (int u = x; u < x + iw && ok; u++)
				{
					int i = u + v * inventory.width;
					if (inventory.bitmask[i >> 3] & 1 << (i & 7))
						ok = false;
				}
			}

			if (!ok)
				continue;

			int xy[2] = { x,y };
			inventory.InsertItem(item, xy);

			return true;
		}
	}
	return false;
}

bool Game::DropItem(int index)
{
	// automatically calculates pos[3]

	assert(index >= 0 && index < inventory.my_items);
	float ang = rand() % 360;

	double ret[4];
	double dpos[3] =
	{
		player.pos[0] + (float)(2 * cos(ang*M_PI / 180)),
		player.pos[1] + (float)(2 * sin(ang*M_PI / 180)),
		0
	};
	double downward[3] = { 0,0,-1 };
	double z = 0;
	bool ok = false;
	ok = 0 != HitTerrain(terrain, dpos, downward, ret, 0);
	if (ok)
	{
		z = ret[2];
		dpos[2] = player.pos[2] + 3 * HEIGHT_SCALE;
		downward[2] = -dpos[2];
		bool ok2 = 0 != HitWorld(world, dpos, downward, ret, 0, true, false, true);
		if (ok2 && ret[2] > z)
			z = ret[2];
	}
	else
	{
		dpos[2] = player.pos[2] + 3 * HEIGHT_SCALE;
		downward[2] = -dpos[2];
		ok = 0 != HitWorld(world, dpos, downward, ret, 0, true, false, true);
		if (ok)
			z = ret[2];
	}

	if (ok)
	{
		float _pos[3] =
		{
			+(float)dpos[0],
			+(float)dpos[1],
			+(float)z
		};

		inventory.RemoveItem(index, _pos, prev_yaw);
	}

	return ok;
}

void Game::ScreenToCell(int p[2]) const
{
	p[0] = (2*p[0] - input.size[0] + render_size[0] * font_size[0]) / (2 * font_size[0]);
	p[1] = (input.size[1]-1 - 2*p[1] + render_size[1] * font_size[1]) / (2 * font_size[1]);
}

bool Character::SetActionNone(uint64_t stamp)
{
	if (req.action == ACTION::NONE)
		return true;
	int old = req.action;
	req.action = ACTION::NONE;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	anim = 0;
	frame = 0;
	action_stamp = stamp;
	return true;
}

bool Character::SetActionAttack(uint64_t stamp)
{
	if (req.action == ACTION::ATTACK)
		return true;
	if (req.action == ACTION::FALL || 
		req.action == ACTION::STAND || 
		req.action == ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::ATTACK;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	if (req.weapon == WEAPON::REGULAR_CROSSBOW)
	{
		anim = 0;
		frame = 0;
	}
	else
	{
		anim = 0;
		frame = 2;
	}
	action_stamp = stamp;
	hit_tested = false;

	return true;
}

bool Character::SetActionStand(uint64_t stamp)
{
	if (req.action == ACTION::STAND)
		return true;

	if (req.action != ACTION::FALL && req.action != ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::STAND;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	if (old == ACTION::FALL)
	{
		// recalc stamp to match current frame
		action_stamp = stamp - frame * stand_us_per_frame;
	}
	else
	{
		action_stamp = stamp;
		anim = 0;
		frame = 0;
	}

	return true;
}

bool Character::SetActionFall(uint64_t stamp)
{
	if (req.action == ACTION::FALL)
		return true;

	if (req.action == ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::FALL;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	if (old == ACTION::STAND)
	{
		// recalc stamp to match current frame
		action_stamp = stamp - (sprite->anim[anim].length - frame) * fall_us_per_frame;
	}
	else
	{
		action_stamp = stamp;
		anim = 0;
		frame = sprite->anim[anim].length - 1;
	}

	return true;
}

bool Character::SetActionDead(uint64_t stamp)
{
	int old = req.action;
	req.action = ACTION::DEAD;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	action_stamp = stamp;
	anim = 0;
	frame = 0;

	return true;
}

bool Human::SetWeapon(int w)
{
	if (req.action == ACTION::ATTACK)
		return false; 
	if (w == req.weapon)
		return true;

	int old = req.weapon;
	req.weapon = w;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.weapon = old;
		return false;
	}
	sprite = spr;

	return true;
}

bool Human::SetShield(int s)
{
	if (s == req.shield)
		return true;

	int old = req.shield;
	req.shield = s;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.shield = old;
		return false;
	}
	sprite = spr;

	return true;
}

bool Human::SetHelmet(int h)
{
	if (h == req.helmet)
		return true;

	int old = req.helmet;
	req.helmet = h;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.helmet = old;
		return false;
	}
	sprite = spr;

	return true;
}

bool Human::SetArmor(int a)
{
	if (a == req.armor)
		return true;

	int old = req.armor;
	req.armor = a;

	Sprite* spr = GetSprite(&req, clr);
	if (!spr)
	{
		req.armor = old;
		return false;
	}
	sprite = spr;

	return true;
}

bool Human::SetMount(int m)
{
	if (req.action != ACTION::NONE)
		return false;

	if (m == req.mount)
		return true;
	int old = req.mount;
	req.mount = m;

	Sprite* s = GetSprite(&req, clr);
	if (!s)
	{
		req.mount = old;
		return false;
	}
	sprite = s;

	return true;
}

void Game::Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
	if (server)
		server->stamp = _stamp;

	uint64_t fps_window_time = _stamp - fps_window[fps_window_pos]; 
	fps_window[fps_window_pos++] = _stamp;
	if (fps_window_pos == fps_window_size)
		fps_window_pos = 0;

	int FPSx10 = (int)((fps_window_size * 10000000 + (fps_window_time>>1)) / fps_window_time);

	if (_stamp-stamp > 500000) // treat lags longer than 0.5s as stall
		stamp = _stamp;

	// handle dirpad autorep
	if (input.pad_connected && input.pad_autorep>0)
	{
		if (stamp - input.pad_stamp > 500000)
		{
			input.pad_stamp = stamp - 500000 + 50000; // 20Hz
			OnPadButton(input.pad_autorep-1, true);
		}
	}

	if (PressKey && _stamp - PressStamp > 50000 /*500000*/)
	{
		// in render(): 
		// if there is stored key and time elapsed since it was pressed > thresh
		//   then: emulate stored KEY_UP and clear stored key

		char ch = KeybAutoRepChar;
		OnKeyb(GAME_KEYB::KEYB_UP, PressKey);
		PressKey = 0;
		// revert it (OnKeyb nulls it)
		KeybAutoRepChar = ch;		
	}

	int f120 = 1 + (_stamp - stamp) / 8264;
	TalkBox_blink += f120;

	if (KeybAutoRepChar)
	{
		char ch = KeybAutoRepChar;
		while (_stamp - KeybAuroRepDelayStamp >= 500000) // half sec delay
		{
			OnKeyb(GAME_KEYB::KEYB_CHAR, ch);
			KeybAuroRepDelayStamp += 30000;
		}
		// revert it (OnKeyb nulls it)
		KeybAutoRepChar = ch;
	}

	if (render_size[0] != width || render_size[1] != height)
	{
		int kh = keyb.Height(width, height);
		if (!show_keyb || keyb_hide > kh)
			keyb_hide = kh;
	}

	render_size[0] = width;
	render_size[1] = height;

	float lt[4] = { 1,0,1,0.5 };
	float n = lt[0] * lt[0] + lt[1] * lt[1] + lt[2] * lt[2];
	if (n > 0.001)
	{
		lt[0] /= n;
		lt[1] /= n;
		lt[2] /= n;
	}


	PhysicsIO io;
	io.x_impulse = player.impulse[0];
	io.y_impulse = player.impulse[1];
	io.x_force = 0;
	io.y_force = 0;
	io.torque = 0;
	io.water = water;
	io.jump = false;

	bool force_handled = false;
	int  torque_handled = 0; // 0 unhandled, 1 touch, 2 mouse
	int  torque_sign = 0;

	for (int i=0; i<4; i++)
	{
		switch (input.contact[i].action)
		{
			case Input::Contact::FORCE:
				assert(!force_handled);
				force_handled = true;
				if (i==0)
				{
					// relative to center
					io.x_force = 2 * ((input.contact[i].pos[0] * 2 - input.size[0]) / (float)input.size[0] - 2*scene_shift/2 / (float)render_size[0]);
					io.y_force = 2 * ((input.size[1] - input.contact[i].pos[1] * 2) / (float)input.size[1]);					
				}
				else
				{
					// relatice to drag start
					io.x_force = 4 * (input.contact[i].pos[0] - input.contact[i].drag_from[0]) / (float)input.size[0];
					io.y_force = 4 * (input.contact[i].drag_from[1] - input.contact[i].pos[1]) / (float)input.size[0];
				}
				break;
			
			case Input::Contact::TORQUE:
				if (i==0)
				{
					assert(torque_handled==0);
					// mouse absolute
					torque_handled = 2;
					float sensitivity = 200.0f / input.size[0];
					float yaw = input.contact[i].start_yaw - sensitivity * (input.contact[i].pos[0] - input.contact[i].drag_from[0]);
					io.torque = 0;

					double dt = (_stamp - stamp) * 0.000001 * 20;
					yaw_vel = (yaw - prev_yaw); // will be set on ContactEnd
					if (dt < 0)
						dt = 0;
					else
					if (dt > 1)
						dt = 1;
					yaw = prev_yaw + yaw_vel*dt;
					SetPhysicsYaw(physics, yaw, 0);

					/*
					float mouse_dir_x = (input.contact[i].pos[0] * 2 - input.size[0])/2;
					float mouse_dir_y = -(input.contact[i].pos[1] * 2 - input.size[1]);
					if (mouse_dir_x != 0 || mouse_dir_y != 0)
					{
						float mouse_dir = atan2(mouse_dir_y, mouse_dir_x) * 180 / M_PI + 90;
						SetPhysicsDir(physics, mouse_dir + yaw);
					}
					*/
				}
				else
				{
					assert(torque_handled!=2);
					torque_handled = 1;
					// physics timer
					torque_sign -= input.contact[i].margin; // -1 | +1
				}
				break;
		}
	}

	io.torque = torque_sign < 0 ? -1 : torque_sign > 0 ? +1 : 0;

	if (!player.talk_box) // force & torque with keyboard
	{
		if (!force_handled)
		{
			float speed = 1;
			if (input.IsKeyDown(A3D_LSHIFT) || input.IsKeyDown(A3D_RSHIFT))
				speed *= 0.5;

			if (show_inventory)
			{
				io.x_force = (int)input.IsKeyDown(A3D_D) - (int)input.IsKeyDown(A3D_A);
				io.y_force = (int)input.IsKeyDown(A3D_W) - (int)input.IsKeyDown(A3D_S);
			}
			else
			{
				io.x_force = (int)(input.IsKeyDown(A3D_RIGHT) || input.IsKeyDown(A3D_D)) - (int)(input.IsKeyDown(A3D_LEFT) || input.IsKeyDown(A3D_A));
				io.y_force = (int)(input.IsKeyDown(A3D_UP) || input.IsKeyDown(A3D_W)) - (int)(input.IsKeyDown(A3D_DOWN) || input.IsKeyDown(A3D_S));
			}

			float len = sqrtf(io.x_force*io.x_force + io.y_force*io.y_force);
			if (len > 0)
				speed /= len;
			io.x_force *= speed;
			io.y_force *= speed;
		}

		if (menu_depth<0)
		{
			if (input.contact[3].action == Input::Contact::NONE)
			{
				io.x_force += input.pad_axis[0] / 1024 / 32.0f;
				io.y_force -= input.pad_axis[1] / 1024 / 32.0f;
			}
		}

		if (!torque_handled)
		{
			io.torque = (int)(input.IsKeyDown(A3D_DELETE) || input.IsKeyDown(A3D_PAGEUP) || input.IsKeyDown(A3D_F1) || input.IsKeyDown(A3D_Q)) -
				(int)(input.IsKeyDown(A3D_INSERT) || input.IsKeyDown(A3D_PAGEDOWN) || input.IsKeyDown(A3D_F2) || input.IsKeyDown(A3D_E));

			//io.torque = 1;
			//io.y_force = 1;
		}
	}

	if (menu_depth<0)
		io.torque += (input.pad_axis[4] - input.pad_axis[5]) / 1024 / 32.0f;

	io.jump = input.jump;

	if (player.req.action == ACTION::FALL || 
		player.req.action == ACTION::STAND ||
		player.req.action == ACTION::DEAD)
	{
		io.x_force = 0;
		io.y_force = 0;
		io.jump = false;
	}

	if (player.req.action == ACTION::ATTACK && 
		player.req.weapon == WEAPON::REGULAR_CROSSBOW)
	{
		io.x_force = 0;
		io.y_force = 0;
		io.jump = false;
	}

	// blocked by enemies? (closest one)
	// ...

	static bool prev_grounded = false;

	if (prev_grounded && player.req.mount == MOUNT::BEE)
	{
		float len = sqrtf(io.x_force*io.x_force + io.y_force*io.y_force);
		if (len > 0.5f)
		{
			float mul = 0.5 / len;
			io.x_force *= mul;
			io.y_force *= mul;
		}
	}

	int steps = Animate(physics, _stamp, &io, &player.req, true);

	if (io.grounded && blood)
		BloodLeak(&player, steps);

	prev_grounded = io.grounded;

	player.impulse[0] = io.x_impulse;
	player.impulse[1] = io.y_impulse;

	if (steps > 0)
	{
		input.jump = false;
	}

	// if (!show_inventory)
	{
		// animate buddies & enemies
		Character* h = player_head;

		while (h)
		{
			if (h->data != physics)
			{
				Physics* p = (Physics*)h->data;

				PhysicsIO pio;
				pio.x_impulse = h->impulse[0];
				pio.y_impulse = h->impulse[1];
				pio.x_force = 0;
				pio.y_force = 0;
				pio.torque = 0;
				pio.water = water;
				pio.jump = false;

				if (h->target)
					h->target->followers--;
				h->target = 0;

				if (h->req.action != ACTION::DEAD && h->req.action != ACTION::FALL)
				{

					Character* buddy_ch = 0;
					float buddy_nd = 5;
					float buddy_cd = 0;
					int buddy_nn = 0;

					// find closest enemy
					float min_target_dist = 0;
					float max_target_dist = 0; // give up if blocked by others and closer than this distance
					float distance = 0; // distance to target before update step

					float master_distance = 0;
					if (h->master)
					{
						float dx = h->master->pos[0] - h->pos[0];
						float dy = h->master->pos[1] - h->pos[1];
						master_distance = sqrtf(dx*dx + dy * dy);
					}

					{
						Character* h2 = player_head;
						Character* enemy_ch = 0;
						float enemy_cd = 0;
						int enemy_cf = 0;

						while (h2)
						{
							// not ally (can be true for player)
							if (h2->enemy != h->enemy && h2->req.action != ACTION::DEAD)
							{
								// enemy
								float bx = h2->pos[0] - h->pos[0];
								float by = h2->pos[1] - h->pos[1];
								float d = (bx * bx + by * by);

								if (h2->req.action!=ACTION::DEAD &&  h->shoot_by == h2 && 
									stamp >  500000 + h->shoot_by_stamp &&
									stamp < 5000000 + h->shoot_by_stamp)
								{
									d *= 0.2;
								}

								if (!enemy_ch || d * (h2->followers + 4) < enemy_cd*(enemy_cf + 4))
								{
									enemy_cf = h2->followers;
									enemy_cd = d;
									enemy_ch = h2;
								}
							}
							else
							if (h2->data != physics && h2 != h && h2->req.action != ACTION::DEAD)
							{
								// buddy
								float bx = h2->pos[0] - h->pos[0];
								float by = h2->pos[1] - h->pos[1];
								float d = bx * bx + by * by;
								if (!buddy_ch || d < buddy_cd)
								{
									buddy_cd = d;
									buddy_ch = h2;
								}

								if (d < buddy_nd*buddy_nd)
								{
									buddy_nn++;
								}
							}

							h2 = h2->next;
						}

						float ret_md = 40; // skip enemies if distance to master is greater
						float max_ed = 20; // max distance to enemy (if greater don't chase)
						float min_ed_contact = 3;  // min distance to enemy (if smaller then attack instead of chase)
						float min_ed_archer = 10;  
						float min_md = 10; // min distance to master (if smaller don't come any closer)

						if (enemy_ch && enemy_cd < max_ed*max_ed && master_distance < ret_md)
						{
							h->target = enemy_ch;
							h->target->followers++;

							if (h->req.weapon == WEAPON::REGULAR_CROSSBOW)
							{
								min_target_dist = min_ed_archer;
								max_target_dist = min_ed_archer + 3;
							}
							else
							{
								min_target_dist = min_ed_contact;
								max_target_dist = min_ed_contact + 3;
							}
						}
						else
						if (h->master)
						{
							h->target = h->master;
							h->target->followers++;
							min_target_dist = min_md;
							max_target_dist = min_md + 30;
						}
					}

					if (h->target)
					{
						float dx = h->target->pos[0] - h->pos[0];
						float dy = h->target->pos[1] - h->pos[1];
						float d = sqrtf(dx*dx + dy * dy);

						if (d < 10)
						{
							dx *= 0.7;
							dy *= 0.7;
						}

						distance = d;

						if (d > min_target_dist)
						{
							if (d > 15)
							{
								pio.x_force = dx / d;
								pio.y_force = dy / d;
							}
							else
							{
								pio.x_force = dx / 15;
								pio.y_force = dy / 15;
							}

							if (buddy_ch && buddy_cd < buddy_nd*buddy_nd)
							{
								// cd = sqrtf(cd);

								// check if our force dir is towards or away of that buddy
								float bx = buddy_ch->pos[0] - h->pos[0];
								float by = buddy_ch->pos[1] - h->pos[1];

								if (pio.x_force * bx + pio.y_force * by > 0)
								{
									// towards, so we need to adjust force to slide

									if (d < max_target_dist && buddy_nn>1)
									{
										// filter out if late to the party
										h->jump = false;
										pio.x_force = 0;
										pio.y_force = 0;
									}
									else
									{
										float x1[3] = { bx,by,0 };
										float y1[3] = { dx,dy,0 };
										float z1[3];
										CrossProduct(x1, y1, z1);
										CrossProduct(z1, y1, x1);

										float len = sqrtf(x1[0] * x1[0] + x1[1] * x1[1]);
										x1[0] *= d / len;
										x1[1] *= d / len;

										pio.x_force = 0.1*x1[0];
										pio.y_force = 0.1*x1[1];
									}
								}
							}
						}
						else
						{
							if (h->target != h->master)
							{
								// we need to randomize _stamp here
								// so first in list NCPs arn't favored 

								h->SetActionAttack(_stamp);
							}
						}
					}

					// if previously we applied a force
					// but movement response wasn't significant
					// force jump this time

					if (h->jump)
					{
						pio.jump = true;
						h->jump = false;
					}

					if (h->stuck >= 100 && h->stuck < 200)
					{
						// go opposite
						pio.x_force = -pio.x_force;
						pio.y_force = -pio.y_force;
					}
					else
					if (h->stuck >= 200 && h->stuck < 300)
					{
						// go around
						if (h->around == 0)
						{
							float t = pio.x_force;
							pio.x_force = -pio.y_force;
							pio.y_force = t;
						}
						else
						{
							float t = pio.x_force;
							pio.x_force = pio.y_force;
							pio.y_force = -t;
						}
					}
					else
					if (h->stuck >= 300 && h->stuck < 400)
					{
						// keep jumping
						// even if on way back to target
					}
					else
					if (h->stuck >= 400)
					{
						h->stuck = 0;
					}

					if (h->req.action == ACTION::ATTACK &&
						h->req.weapon == WEAPON::REGULAR_CROSSBOW)
					{
						pio.x_force = 0;
						pio.y_force = 0;
						pio.jump = false;
					}

					int s = Animate(p, _stamp, &pio, &h->req, false);

					if (pio.grounded && blood)
						BloodLeak(h, s);

					if (h->target)
					{
						float adv[2] = { pio.pos[0] - h->unstuck[1][0], pio.pos[1] - h->unstuck[1][1] };
						if (adv[0] * adv[0] + adv[1] * adv[1] > 2.0f)
						{
							h->unstuck[0][0] = h->unstuck[1][0];
							h->unstuck[0][1] = h->unstuck[1][1];
							h->unstuck[0][2] = h->unstuck[1][2];
							h->unstuck[1][0] = pio.pos[0];
							h->unstuck[1][1] = pio.pos[1];
							h->unstuck[1][2] = pio.pos[2];
						}


						int s_stucks = 5;
						if (h->stuck < 100 && h->stuck + s * s_stucks >= 100)
						{
							pio.pos[0] = h->pos[0] = h->unstuck[1][0] = h->unstuck[0][0];
							pio.pos[1] = h->pos[1] = h->unstuck[1][1] = h->unstuck[0][1];
							pio.pos[2] = h->pos[2] = h->unstuck[1][2] = h->unstuck[0][2];
							float vel[3] = { 0,0,0 };
							SetPhysicsPos(p, pio.pos, vel);
						}

						if (fabsf(pio.x_impulse) > 0.001 || fabsf(pio.y_impulse) > 0.001)
						{
							h->stuck = 0;
						}


						if (h->stuck >= 100)
						{
							h->jump = true;
							h->stuck += s * s_stucks;
						}

						if (s && h->stuck < 100 && fabsf(pio.x_force) + fabsf(pio.y_force) > 0.5)
						{
							// check new dist
							float dx = h->target->pos[0] - pio.pos[0];
							float dy = h->target->pos[1] - pio.pos[1];
							float d2 = sqrtf(dx*dx + dy * dy);

							if (distance - d2 < 0.001*s)
							{
								h->jump = true;

								// if tried for more than 3 times
								// try go around?
								if (h->stuck < 100)
								{
									h->stuck += s * s_stucks;
									if (h->stuck >= 100)
									{
										h->around = fast_rand() & 1;
									}
								}
							}
						}
					}
				}
				else
				{
					if (h->req.action == ACTION::ATTACK &&
						h->req.weapon == WEAPON::REGULAR_CROSSBOW)
					{
						pio.x_force = 0;
						pio.y_force = 0;
						pio.jump = false;
					}
					int s = Animate(p, _stamp, &pio, &h->req, false);

					if (pio.grounded && blood)
						BloodLeak(h, s);
				}

				h->impulse[0] = pio.x_impulse;
				h->impulse[1] = pio.y_impulse;

				// COPIED FROM PLAYER
				switch (h->req.action)
				{
					case ACTION::ATTACK:
					{
						switch (h->req.weapon)
						{
							case PLAYER_WEAPON_INDEX::SWORD:
							{
								//static const int frames[] = { 2,2,2,1,1,1,0,0,0,0,0,0,0,0, 0,1,2,3,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3 };

								// SWOOSH:                                                    <--------->
								static const int frames[] = { 7,7,7,1,1,1,0,0,0,0,0,0,0,0, 0,1,2,3,4,4,4,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5, 6,6,6,6,6,6,6 };
								int frame_index = (_stamp - h->action_stamp) / attack_us_per_frame;

								if (frame_index > 21 && !h->hit_tested)
								{
									// do hit test, once per attack!
									h->hit_tested = true;

									// check if target is enemy and is in weapon range
									if (h->target && h->target->enemy != h->enemy)
									{
										float dx = h->target->pos[0] - h->pos[0];
										float dy = h->target->pos[1] - h->pos[1];
										float d = sqrtf(dx*dx + dy * dy);
										if (d < 3)
										{
											int hp = h->target->HP;
											h->target->HP -= rand() % 100;

											{
												h->target->leak += (hp - h->target->HP) / 5;

												float r = fast_rand() % 20 * 0.1f + 0.6;
												if (hp > 0 && h->target->HP <= 0)
													r = fmaxf(r,2.5f);

												float dR = 1.0;
												float dr = dR * sqrtf((fast_rand() & 0xfff) / (float)0xfff);
												float dt = (fast_rand() & 0xfff) * (float)(2.0 * M_PI) / (float)0xfff;
												float xy[2] = { h->target->pos[0] + dr * cosf(dt), h->target->pos[0] + dr * sinf(dt) };
												if (blood)
													PaintTerrain(xy, r, 5/*blood*/);
											}

											float d = 15.0f / sqrtf(dx*dx + dy * dy);
											h->target->impulse[0] += dx * d; 
											h->target->impulse[1] += dy * d;

											if (h->target->HP <= 0)
											{
												if (h->target->req.mount != MOUNT::NONE)
												{
													((Human*)h->target)->SetMount(MOUNT::NONE);
													h->target->HP = hp;
												}
												else
												{
													h->target->dir = atan2(-dy, -dx) * 180 / M_PI /* + phys->yaw == ZERO*/ + 90;
													Physics* p = (Physics*)h->target->data;
													SetPhysicsDir(p, h->target->dir);

													h->target->HP = 0;
													h->target->SetActionFall(_stamp);
												}
											}
										}
									}
								}

								// if frameindex jumps from first half to second half of frames
								// sample scene at hit location, if theres something emit particles in color(s) of hit object
								// if this is human sprite, emitt red particles

								assert(frame_index >= 0);
								if (frame_index >= sizeof(frames) / sizeof(int))
									h->SetActionNone(_stamp);
								else
									h->frame = frames[frame_index];
								break;
							}

							case PLAYER_WEAPON_INDEX::CROSSBOW:
							{
								// just delay
								//static const int frames[] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
								int frame_index = (_stamp - h->action_stamp) / attack_us_per_frame;

								// if frameindex jumps from first half to second half of frames
								// sample scene at hit location, if theres something emit particles in color(s) of hit object
								// if this is human sprite, emitt red particles

								int frames = 10;
								assert(frame_index >= 0);

								if (2 * frame_index >= frames)
								{
									// here we should release arrow
								}

								if (frame_index >= frames)
									h->SetActionNone(_stamp);
								break;
							}
						}

						break;
					}

					case ACTION::FALL:
					{
						// animate, check if finished -> stay at last frame
						int frame = (_stamp - h->action_stamp) / stand_us_per_frame;
						assert(frame >= 0);
						if (frame >= h->sprite->anim[h->anim].length)
							h->SetActionDead(_stamp);
						else
							h->frame = h->sprite->anim[h->anim].length-1 - frame;
						break;
					}

					case ACTION::STAND:
					{
						// animate, check if finished -> switch to NONE
						int frame = (_stamp - h->action_stamp) / stand_us_per_frame;
						assert(frame >= 0);
						if (frame >= h->sprite->anim[h->anim].length)
							h->SetActionNone(_stamp);
						else
							h->frame = frame;
						break;
					}

					case ACTION::DEAD:
					{
						// nutting
						break;
					}

					case ACTION::NONE:
					{
						// animate / idle depending on physics output
						if (pio.player_stp < 0)
						{
							// choose sprite by active items
							h->anim = 0;
							h->frame = 0;
						}
						else
						{
							// choose sprite by active items
							h->anim = 1;
							h->frame = pio.player_stp / 1024 % h->sprite->anim[h->anim].length;
						}
						break;
					}
				}

				if (h->target && h->target != h->master && h->req.action == ACTION::ATTACK)
				{
					// force direction
					float dx = h->target->pos[0] - pio.pos[0];
					float dy = h->target->pos[1] - pio.pos[1];
					pio.player_dir = atan2(dy, dx) * 180 / M_PI /* + phys->yaw == ZERO*/ + 90;
				}

				h->pos[0] = pio.pos[0];
				h->pos[1] = pio.pos[1];
				h->pos[2] = pio.pos[2];
				h->dir = pio.player_dir;

				int reps[] = { 0,0,0,0 };

				UpdateSpriteInst(world, h->inst, h->sprite, pio.pos, pio.player_dir, h->anim, h->frame, reps);
			}

			h = h->next;
		}
	}

	prev_yaw = io.yaw;

	player.pos[0] = io.pos[0];
	player.pos[1] = io.pos[1];
	player.pos[2] = io.pos[2];

	switch (player.req.action)
	{
		case ACTION::ATTACK:
		{
			switch (player.req.weapon)
			{
				case PLAYER_WEAPON_INDEX::SWORD:
				{
					//static const int frames[] = { 2,2,2,1,1,1,0,0,0,0,0,0,0,0, 0,1,2,3,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4, 3,3,3,3,3,3,3 };

					// SWOOSH:                                                    <--------->
					static const int frames[] = { 7,7,7,1,1,1,0,0,0,0,0,0,0,0, 0,1,2,3,4,4,4,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5, 6,6,6,6,6,6,6 };
					int frame_index = (_stamp - player.action_stamp) / attack_us_per_frame;

					Character* h = &player;
					if (frame_index > 21 && !h->hit_tested)
					{
						// do hit test, once per attack!
						h->hit_tested = true;

						// find target for player
						{
							Character* h2 = player_head;
							Character* ch = 0;
							float cd = 0;
							while (h2)
							{
								if (h2->data != physics && h2->enemy)
								{
									// check if distance is at max 3
									float dx = h2->pos[0] - h->pos[0];
									float dy = h2->pos[1] - h->pos[1];

									float dd = dx * dx + dy * dy;
									if (dd < /*3 * 3*/ 4*4) // player's sword a bit longer
									{
										// check if direction is in +/-22.5deg
										float dif = atan2(dy, dx) * 180 / M_PI + 90 - h->dir;
										dif = fmodf(dif, 360);
										if (dif < -180)
											dif += 360;
										if (dif > +180)
											dif -= 360;

										if (fabsf(dif) <= 90)
										{
											if (!ch || cd < dd)
											{
												cd = dd;
												ch = h2;
											}
										}
									}
								}

								h2 = h2->next;
							}

							h->target = ch;
						}


						// check if target is enemy and is in weapon range
						if (h->target && h->target->enemy != h->enemy)
						{
							float dx = h->target->pos[0] - h->pos[0];
							float dy = h->target->pos[1] - h->pos[1];
							float d = sqrtf(dx*dx + dy * dy);
							if (d < 3)
							{
								int hp = h->target->HP;
								h->target->HP -= rand() % 100;
								//h->target->HP = 0;

								{
									h->target->leak += (hp - h->target->HP) / 5;

									float r = fast_rand() % 20 * 0.1f + 0.6;
									if (hp > 0 && h->target->HP <= 0)
										r = fmaxf(r, 2.5f);

									float dR = 1.0;
									float dr = dR * sqrtf((fast_rand() & 0xfff) / (float)0xfff);
									float dt = (fast_rand() & 0xfff) * (float)(2.0 * M_PI) / (float)0xfff;
									float xy[2] = { h->target->pos[0] + dr * cosf(dt), h->target->pos[0] + dr * sinf(dt) };
									if (blood)
										PaintTerrain(xy, r, 5/*blood*/);
								}

								float d = 15.0f / sqrtf(dx*dx + dy * dy);
								h->target->impulse[0] += dx * d;
								h->target->impulse[1] += dy * d;

								if (h->target->HP <= 0)
								{
									if (h->target->req.mount != MOUNT::NONE)
									{
										((Human*)h->target)->SetMount(MOUNT::NONE);
										h->target->HP = hp;
									}
									else
									{
										h->target->dir = atan2(-dy, -dx) * 180 / M_PI /* + phys->yaw == ZERO*/ + 90;
										Physics* p = (Physics*)h->target->data;
										SetPhysicsDir(p, h->target->dir);

										h->target->HP = 0;
										h->target->SetActionFall(_stamp);
									}
								}
							}
						}
					}
					assert(frame_index >= 0);
					if (frame_index >= sizeof(frames) / sizeof(int))
						player.SetActionNone(_stamp);
					else
						player.frame = frames[frame_index];
					break;
				}

				case PLAYER_WEAPON_INDEX::CROSSBOW:
				{
					// just delay
					//static const int frames[] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
					int frame_index = (_stamp - player.action_stamp) / attack_us_per_frame;

					// if frameindex jumps from first half to second half of frames
					// sample scene at hit location, if theres something emit particles in color(s) of hit object
					// if this is human sprite, emitt red particles

					io.x_force = 0;
					io.y_force = 0;

					int frames = 10;
					assert(frame_index >= 0);

					if (2*frame_index >= frames)
					{
						// here we should release arrow
					}

					if (frame_index >= frames)
						player.SetActionNone(_stamp);
					break;
				}
			}

			break;
		}

		case ACTION::FALL:
		{
			// animate, check if finished -> stay at last frame
			int frame = (_stamp - player.action_stamp) / stand_us_per_frame;
			assert(frame >= 0);
			if (frame >= player.sprite->anim[player.anim].length)
				player.SetActionDead(_stamp);
			else
				player.frame = player.sprite->anim[player.anim].length-1 - frame;
			break;
		}

		case ACTION::STAND:
		{
			// animate, check if finished -> switch to NONE
			int frame = (_stamp - player.action_stamp) / stand_us_per_frame;
			assert(frame >= 0);
			if (frame >= player.sprite->anim[player.anim].length)
				player.SetActionNone(_stamp);
			else
				player.frame = frame;
			break;
		}

		case ACTION::DEAD:
		{
			// nutting
			break;
		}

		case ACTION::NONE:
		{
			// animate / idle depending on physics output
			if (io.player_stp < 0)
			{
				// choose sprite by active items
				player.anim = 0;
				player.frame = 0;
			}
			else
			{
				// choose sprite by active items
				player.anim = 1;
				player.frame = io.player_stp / 1024 % player.sprite->anim[player.anim].length;
			}
			break;
		}
	}

	// update / animate:
	player.dir = io.player_dir;

	// update player inst in world
	int reps[4] = { 0,0,0,0 };
	UpdateSpriteInst(world, player_inst, player.sprite, player.pos, player.dir, player.anim, player.frame, reps);

	int inventory_width = 39;

	if (show_inventory && scene_shift < inventory_width) // inventory width with margins is 58
	{
		scene_shift += f120;
		if (scene_shift > inventory_width)
			scene_shift = inventory_width;
	}
	else
	if (!show_inventory && scene_shift > 0)
	{
		scene_shift-=f120;
		if (scene_shift < 0)
			scene_shift = 0;
	}

	int ss[2] = { scene_shift/2 , 0 };

	::Render(renderer, _stamp, terrain, world, water, 1.0, io.yaw, io.pos, lt,
		width, height, ptr, player_inst, ss, perspective);

	if (input.shoot /*&& !player.shooting*/ && 
		player.req.weapon == WEAPON::REGULAR_CROSSBOW &&
		stamp - player.shoot_stamp > 1000000) // 1s cooldown (would be nice to animate xbow reload)
	{
		// this should be done inside SetActionAttack() if weapon is crossbow

		// find closest enemy in current direction
		// if found change dir to be exactly on the line between player and target
		
		Character* h2 = player_head;
		Character* t = 0;
		float _dang = 60;
		float _ang = player.dir;
		float _dist = 0;
		while (h2)
		{
			// temporarily commented!!!
			if (h2->enemy && h2->data != physics && h2->req.action != ACTION::DEAD)
			{
				float dx = h2->pos[0] - player.pos[0];
				float dy = h2->pos[1] - player.pos[1];
				float dz = h2->pos[2] - player.pos[2];

				float d = dx * dx + dy * dy;

				// check min dist, max dist and max slope
				if (d >= 2*2 + 2*2 && d < 30*30 + 30*30 && dz*dz < d*8*8 )
				{
					float a = atan2(dy, dx) * 180 / M_PI + 90;
					float da = a - player.dir;
					
					da = fmodf(da, 360);
					if (da < -180)
						da += 360;
					if (da > +180)
						da -= 360;

					da = fabsf(da);
					if (da < _dang) // +/-60 deg
					{
						t = h2;
						_dang = da;
						_ang = a;
						_dist = d;
					}
				}
			}
			h2 = h2->next;
		}

		if (/*t &&*/ player.SetActionAttack(_stamp))
		{
			if (_ang < 0)
				_ang += 360;
			SetPhysicsDir(physics, _ang);
			player.dir = _ang;
			Sprite* sprite = GetSprite(&player.req, player.clr);

			int ang = (int)floor((_ang - io.yaw) * sprite->angles / 360.0f + 0.5f);
			ang = ang >= 0 ? ang % sprite->angles : (ang % sprite->angles + sprite->angles) % sprite->angles;

			int i = player.frame + ang * sprite->anim[player.anim].length;
			Sprite::Frame* f = sprite->atlas + sprite->anim[player.anim].frame_idx[i];

			int from[3] =
			{
				width / 2 + ss[0] + (f->meta_xy[0] + 1) / 2,
				height / 2 + ss[1] + (f->meta_xy[1] + 1) / 2,
				(int)player.pos[2] + 40 // should depend on meta_xy[2] :o
			};

			if (UnprojectCoords3D(renderer, from, player.shoot_from))
			{
				if (t)
				{
					player.shoot_to[0] = t->pos[0] * HEIGHT_CELLS;
					player.shoot_to[1] = t->pos[1] * HEIGHT_CELLS;
					player.shoot_to[2] = t->pos[2] + 40;
				}
				else
				{
					player.shoot_to[0] = player.pos[0] * HEIGHT_CELLS + 1000 * cos((player.dir - 90) * M_PI / 180);
					player.shoot_to[1] = player.pos[1] * HEIGHT_CELLS + 1000 * sin((player.dir - 90) * M_PI / 180);
					player.shoot_to[2] = player.pos[2] + 40;
				}

				double p[3] = { player.shoot_from[0], player.shoot_from[1], player.shoot_from[2] };
				double v[3] = { player.shoot_to[0] - p[0], player.shoot_to[1] - p[1], player.shoot_to[2] - p[2] };
				double r1[3] = { 0,0,0 };
				double r2[3] = { 0,0,0 };

				p[0] /= HEIGHT_CELLS;
				p[1] /= HEIGHT_CELLS;

				v[0] *= 1000.0 / HEIGHT_CELLS;
				v[1] *= 1000.0 / HEIGHT_CELLS;
				v[2] *= 1000.0;

				/*
				v[0] *= 1.0 / HEIGHT_CELLS;
				v[1] *= 1.0 / HEIGHT_CELLS;
				*/

				HideInst(player_inst);
				Inst* inst = HitWorld(world, p, v, r1, 0, true, false, true, false);
				bool ground = HitTerrain(terrain, p, v, inst ? r2 : r1, 0, true);

				if (inst && ground)
				{
					double d1 = (r1[0] - p[0])*v[0] + (r1[1] - p[1])*v[1] + (r1[2] - p[2])*v[2];
					double d2 = (r2[0] - p[0])*v[0] + (r2[1] - p[1])*v[1] + (r2[2] - p[2])*v[2];
					if (d2 < d1)
					{
						inst = 0;
						r1[0] = r2[0];
						r1[1] = r2[1];
						r1[2] = r2[2];
					}
					else
					{
						ground = false;
					}
				}

				bool hit = true;

				if (inst || ground)
				{
					float dx = r1[0] * HEIGHT_CELLS - player.shoot_from[0];
					float dy = r1[1] * HEIGHT_CELLS - player.shoot_from[1];
					
					if (dx*dx + dy * dy < _dist * HEIGHT_CELLS * HEIGHT_CELLS)
					{
						hit = false;
						player.shoot_to[0] = r1[0] * HEIGHT_CELLS;
						player.shoot_to[1] = r1[1] * HEIGHT_CELLS;
						player.shoot_to[2] = r1[2];
					}
				}

				ShowInst(player_inst);

				// set shoot params in human
				player.shoot_stamp = stamp;
				player.shooting = true;

				if (hit)
				{
					if (t)
					{
						player.shoot_target = t;
						t->shoot_by = &player;
						t->shoot_by_stamp = stamp;
					}
					else
						player.shoot_target = 0;
				}
				else
					player.shoot_target = 0;

				//printf(hit ? "HIT!\n" : "MISS\n");
			}
		}

		#if 0
		float pos[3];
		float dir;
		int anim;
		int frame;
		int reps[4];
		Sprite* sprite = GetInstSprite(player_inst, pos, &dir, &anim, &frame, reps);
		int ang = (int)floor((dir - io.yaw) * sprite->angles / 360.0f + 0.5f);
		ang = ang >= 0 ? ang % sprite->angles : (ang % sprite->angles + sprite->angles) % sprite->angles;

		int i = frame + ang * sprite->anim[anim].length;
		// if (refl)
		//	i += s->anim[anim].length * s->angles;
		Sprite::Frame* f = sprite->atlas + sprite->anim[anim].frame_idx[i];

		int from[3] =
		{
			width / 2 + ss[0] + (f->meta_xy[0]+1) / 2,
			height / 2 + ss[1] + (f->meta_xy[1]+1) / 2,
			(int)pos[2] + 40 // should depend on meta_xy[2] :o
		};

		player.SetActionAttack(stamp);

		if (UnprojectCoords3D(renderer, from, player.shoot_from))
		{
			// player.shoot_from[2] += 50;

			// read x,y,h at current mouse pos
			// input.shoot_to;
			// ...

			if (UnprojectCoords2D(renderer, input.shoot_xy, player.shoot_to))
			{
				//printf("shot_to.z = %f\n", player.shoot_to[2]);
				//player.shoot_to[0] /= HEIGHT_CELLS;
				//player.shoot_to[1] /= HEIGHT_CELLS;

				// if distance is too long or height diff to big
				// don't shoot!
				// ...

				// check for intermediate collisions
				// shorten shoot_to to closest collider
				// HitTerrain();
				// HitWorld();

				double p[3] = { player.shoot_from[0], player.shoot_from[1], player.shoot_from[2] };
				double v[3] = { player.shoot_to[0] - p[0], player.shoot_to[1] - p[1], player.shoot_to[2] - p[2] };
				double r1[3] = { 0,0,0 };
				double r2[3] = { 0,0,0 };

				p[0] /= HEIGHT_CELLS;
				p[1] /= HEIGHT_CELLS;
				v[0] *= 1000.0 / HEIGHT_CELLS;
				v[1] *= 1000.0 / HEIGHT_CELLS;
				v[2] *= 1000.0;

				HideInst(player_inst);
				Inst* inst = HitWorld(world, p, v, r1, 0, true, false, true);
				bool ground = HitTerrain(terrain, p, v, inst ? r2 : r1, 0, true);

				if (inst && ground)
				{
					double d1 = (r1[0] - p[0])*v[0] + (r1[1] - p[1])*v[1] + (r1[2] - p[2])*v[2];
					double d2 = (r2[0] - p[0])*v[0] + (r2[1] - p[1])*v[1] + (r2[2] - p[2])*v[2];
					if (d2 < d1)
					{
						r1[0] = r2[0];
						r1[1] = r2[1];
						r1[2] = r2[2];
					}
				}
				
				if (inst || ground)
				{
					player.shoot_to[0] = r1[0] *HEIGHT_CELLS;
					player.shoot_to[1] = r1[1] *HEIGHT_CELLS;
					player.shoot_to[2] = r1[2];
				}
				else
				{
					player.shoot_to[0] = (p[0] + v[0])*HEIGHT_CELLS;
					player.shoot_to[1] = (p[1] + v[1])*HEIGHT_CELLS;
					player.shoot_to[2] = (p[2] + v[2]);
				}

				ShowInst(player_inst);

				// set shoot params in human
				player.shoot_stamp = stamp;
				player.shooting = true;
			}
		}
		#endif
	}

	if (input.shoot)
		input.shoot = false;

	Item** inrange = GetNearbyItems(renderer);

	{
		AnsiCell status;
		char status_text[80];
		int len_left = 4;
		int len_right = 4;
		if (server)
		{
			int len = sprintf(status_text,"ON LINE %4d | %d.%d fps", server->lag_ms, FPSx10/10, FPSx10%10);
			len_left = len/2;
			len_right = len - len_left;

			status.fg = 16;
			status.bk = dk_green;
			status.gl=' ';
			status.spare = 0;
		}
		else
		{
			int len = sprintf(status_text,"OFF LINE | %d.%d fps", FPSx10/10, FPSx10%10);
			len_left = len/2;
			len_right = len - len_left;

			status.fg = yellow;
			status.bk = dk_red;
			status.gl=' ';
			status.spare = 0;
		}
		AnsiCell* top = ptr + (height-1)*width;
		int x = 0;
		int center = width/2;
		for (; x<center - len_left; x++)
			if (x>=0 && x<width)
				top[x] = status;
		for (; x<center + len_right; x++)
		{
			int i = x - (center - len_left);
			status.gl = status_text[i];
			if (x>=0 && x<width)
				top[x] = status;
		}
		status.gl = ' ';
		for (; x<width; x++)
			if (x>=0 && x<width)
				top[x] = status;
	}
	

	// NET_TODO:
	// compare inrange with server confirmed inrange
	// if differs request change by ITEM command
	// ...

	// NET_TODO:
	// construct inrange array containing only intersection of confirmed and rendered items
	// use it for rendering list of pickup items
	// ...

	// inrange list MUST be already booked by server for us as exclusive!
	// no other player will be able to see these items in their lists!
	// in this way we are sure we can handle picking up safely

	Character* h;
	if (server)
	{
		h = server->head;
		if (!h)
			h = &player;
	}
	else
	{
		h = player_head; // asciid multi-term
	}

	int num = 0;
	while (h)
	{
		// assuming only humans can talk (rethink)
		if (h->req.kind == SpriteReq::HUMAN)
		{
			num++;
			// todo: ALL TALKS should be sorted by ascending stamp
			// should be Z tested! (so maybe better move it to render)
			// and should have some kind of fade out
			Human* human = (Human*)h;
			for (int i = 0; i < human->talks; i++)
			{
				int speed = 100000 + human->talk[i].box->len*400000/255; // 100000 for len=0 , 500000 for len=255
				int elaps = stamp - human->talk[i].stamp;
				int dy = elaps / speed; // 10 dy per sec (len=0)
			
				if (dy <= 30)
				{
					int view[3];
					ProjectCoords(renderer, human->talk[i].pos, view);
					human->talk[i].box->Paint(ptr, width, height, view[0], view[1] + 8 + dy, false, human->name_cp437);
				}
				else
				if (h == &player || server)
				{
					// each player handles its own talks (except server players, they need help)
					free(human->talk[i].box);
					human->talks--;
					for (int j = i; j < human->talks; j++)
						human->talk[j] = human->talk[j + 1];
					i--;
				}
			}
		}

		if (!h->next && server && h!=&player)
			h = &player; // what a hack! -- just forcing this player to handle its own talkbox (server case)
		else
			h = h->next;
	}

	int contact_items = 0;
	int contact_item[4] = { -1,-1,-1,-1 };

	inventory.UpdateLayout(width,height,scene_shift,bars_pos);

	if (scene_shift > 0)
	{
		Sprite::Frame* sf = inventory_sprite->atlas;

		int clip[] = { 0,0,inventory.layout_width,8 };
		BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y, clip);

		clip[1] = 8; clip[3] = 9;
		for (int i=0; i< inventory.layout_reps[0]; i++)
			BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + i, clip);

		clip[1] = 9; clip[3] = 17;
		BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + inventory.layout_reps[0], clip);

		clip[1] = 17; clip[3] = 18;
		for (int i = 0; i < inventory.layout_reps[1]; i++)
			BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + inventory.layout_reps[0] + 8 + i, clip);

		clip[1] = 18; clip[3] = 26;
		BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + inventory.layout_reps[0] + 8 + inventory.layout_reps[1], clip);

		clip[1] = 26; clip[3] = 27;
		for (int i = 0; i < inventory.layout_reps[2]; i++)
			BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + inventory.layout_reps[0] + 8 + inventory.layout_reps[1] + 8 + i, clip);

		clip[1] = 27; clip[3] = 35;
		BlitSprite(ptr, width, height, sf, inventory.layout_x, inventory.layout_y + 8 + inventory.layout_reps[0] + 8 + inventory.layout_reps[1] + 8 + inventory.layout_reps[2], clip);

		if (inventory.animate_scroll)
		{
			if (!inventory.my_items)
				inventory.animate_scroll = false;
			else
			{
				int i = inventory.focus;
				int iy = inventory.layout_y + inventory.my_item[i].xy[1] * 4 + inventory.layout_height - 6 - (inventory.height * 4 - 1) + inventory.scroll;

				Sprite::Frame* isf = inventory.my_item[i].item->proto->sprite_2d->atlas;

				if (iy < inventory.layout_y + 9)
				{
					int d = inventory.layout_y + 9 - iy;
					inventory.scroll += d<f120 ? d : f120;
				}
				if (iy + isf->height > inventory.layout_y + inventory.layout_height - 5 - 2)
				{
					int d = (iy + isf->height)-(inventory.layout_y + inventory.layout_height - 5 - 2);
					inventory.scroll -= d < f120 ? d : f120;
				}
			}
		}

		if (inventory.animate_scroll)
			inventory.smooth_scroll = inventory.scroll;
		else
		{
			if (inventory.smooth_scroll < 0)
				inventory.smooth_scroll = 0;
			if (inventory.smooth_scroll > inventory.layout_max_scroll)
				inventory.smooth_scroll = inventory.layout_max_scroll;

			if (inventory.smooth_scroll < inventory.scroll)
			{
				int d = inventory.scroll - inventory.smooth_scroll;
				inventory.scroll-= d < f120 ? d : f120;
			}
			else
			if (inventory.smooth_scroll > inventory.scroll)
			{
				int d = inventory.smooth_scroll - inventory.scroll;
				inventory.scroll+= d < f120 ? d : f120;
			}
		}

		if (inventory.scroll < 0)
			inventory.scroll = 0;
		if (inventory.scroll > inventory.layout_max_scroll)
			inventory.scroll = inventory.layout_max_scroll;

		int scroll = inventory.scroll;

		int dst_clip[4] = { inventory.layout_x + 4, inventory.layout_y + 8, inventory.layout_x + 4 + 4 * inventory.width, inventory.layout_y + inventory.layout_height - 5 -1};
		int frm_clip[4] = { inventory.layout_x + 3, inventory.layout_y + 7, inventory.layout_x + 5 + 4 * inventory.width, inventory.layout_y + inventory.layout_height - 4 -1};

		AnsiCell item_bk = { black,brown,32,0 };
		AnsiCell item_inuse_bk = { yellow,lt_red, 249/*dot*/,0 };

		// for all contacts dragging items 
		// check if they can drop at where they are, 
		// paint hilight rects

		if (show_inventory)
		{
			for (int c = 0; c < 4; c++)
				CheckDrop(c, 0, ptr, width, height);
		}

		for (int i = 0; i < consume_anims; i++)
		{
			ConsumeAnim* a = consume_anim + i;
			int elaps = (_stamp - a->stamp) / 50000; // 20 frames a sec (0.25 sec duration for 5x5 sprite)
			int max_elaps = a->sprite->atlas->height;
			if (elaps >= max_elaps)
			{
				if (a->sprite == item_proto_lib[40].sprite_2d)
				{
					// grey potion hack
					player.SetMount(MOUNT::WOLF);
				}
				else
				if (a->sprite == item_proto_lib[39].sprite_2d)
				{
					// gold potion hack
					player.SetMount(MOUNT::BEE);
				}
				else
				if (a->sprite == item_proto_lib[38].sprite_2d)
				{
					// cyan potion hack
					player.SetMount(MOUNT::NONE);
				}
				else
				if (a->sprite == item_proto_lib[34].sprite_2d)
				{
					// healing potion hack
					player.HP = player.MAX_HP;
				}
				

				memmove(a,a+1,sizeof(ConsumeAnim)*(consume_anims-i-1));
				consume_anims--;
				i--;
				continue;
			}

			int ix = inventory.layout_x + a->pos[0]*4 + 4;
			int iy = inventory.layout_y + a->pos[1]*4 + inventory.layout_height - 6 - (inventory.height*4-1) + scroll;

			int clip[4] = { ix, iy, ix + a->sprite->atlas->width, iy + a->sprite->atlas->height };
			if (clip[0] < dst_clip[0])
				clip[0] = dst_clip[0];
			if (clip[1] < dst_clip[1])
				clip[1] = dst_clip[1];
			if (clip[2] > dst_clip[2])
				clip[2] = dst_clip[2];
			if (clip[3] > dst_clip[3])
				clip[3] = dst_clip[3];

			BlitSprite(ptr, width, height, a->sprite->atlas, ix, iy + elaps, clip, false, 0);
		}

		int focus_rect[4];
		Item* focus_item = 0;
		for (int i = 0; i < inventory.my_items; i++)
		{
			int ix = inventory.layout_x + inventory.my_item[i].xy[0]*4 + 4;
			int iy = inventory.layout_y + inventory.my_item[i].xy[1]*4 + inventory.layout_height - 6 - (inventory.height*4-1) + scroll;

			Sprite::Frame* isf = inventory.my_item[i].item->proto->sprite_2d->atlas;

			// if being dragged, attach to contact!
			int in_contact = -1;
			for (int c = 0; c < 4; c++)
			{
				if (input.contact[c].action == Input::Contact::ITEM_GRID_DRAG &&
					input.contact[c].my_item == i)
				{
					in_contact = c;
					break;
				}
			}
				
			if (in_contact>=0)
			{
				// fill bk, defer sprite attached to contact
				int fill[4]; // need to clip, can be scrolled out!
				fill[0] = ix < dst_clip[0] ? dst_clip[0] : ix;
				fill[1] = iy < dst_clip[1] ? dst_clip[1] : iy;
				fill[2] = ix+isf->width > dst_clip[2] ? dst_clip[2] : ix+isf->width;
				fill[3] = iy+isf->height > dst_clip[3] ? dst_clip[3] : iy+isf->height;

				if (inventory.my_item[i].in_use)
					FillRect(ptr, width, height, fill[0], fill[1], fill[2]-fill[0], fill[3]-fill[1], item_inuse_bk);
				else
					FillRect(ptr, width, height, fill[0], fill[1], fill[2]-fill[0], fill[3]-fill[1], item_bk);

				contact_item[contact_items] = in_contact; // deferred render
				contact_items++;
			}
			else
			{
				if (inventory.my_item[i].in_use)
					BlitSprite(ptr, width, height, isf, ix, iy, dst_clip, false, &item_inuse_bk);
				else
					BlitSprite(ptr, width, height, isf, ix, iy, dst_clip, false, &item_bk);
			}

			// FOCUS
			if (i == inventory.focus /*&& in_contact<0*/)
			{
				// deferred
				focus_rect[0] = ix - 1;
				focus_rect[1] = iy - 1;
				focus_rect[2] = isf->width + 2;
				focus_rect[3] = isf->height + 2;
				focus_item = inventory.my_item[i].item;
			}
			else
				PaintFrame(ptr, width, height, ix - 1, iy - 1, isf->width + 2, isf->height + 2, frm_clip, black/*fg*/, 255/*bk*/, true/*dbl-line*/,true/*combine*/);
		}

		if (focus_item)
		{
			PaintFrame(ptr, width, height, focus_rect[0], focus_rect[1], focus_rect[2], focus_rect[3], frm_clip, white/*fg*/, 255/*bk*/, true/*dbl-line*/, false/*combine*/);
			if (inventory.layout_y + 6 >= 0)
			{
				Item* item = focus_item;
				const char* str = item->proto->desc;
				for (int s = 0; str[s]; s++)
				{
					if (inventory.layout_x + 4 + s >= 0 && inventory.layout_x + 4 + s < width)
					{
						AnsiCell* ac = ptr + inventory.layout_x + 4 + s + (inventory.layout_y + 6)*width;
						ac->gl = str[s];
					}
				}
			}
		}

		if (scroll > 0 && inventory.layout_y + inventory.layout_height - 6 >=0 && 
			inventory.layout_y + inventory.layout_height - 6 <height)
		{
			// overwrite upper inventory clip-line with ----
			AnsiCell* row = ptr + (inventory.layout_y + inventory.layout_height - 6)*width;
			for (int dx = inventory.layout_x + 3; dx < inventory.layout_x + 36; dx++)
			{
				if (dx >= 0 && dx < width)
				{
					row[dx].fg = black;
					row[dx].gl = 196;
				}
			}
		}

		if (scroll < inventory.layout_max_scroll && inventory.layout_y + 7 >= 0 && inventory.layout_y + 7 < height)
		{
			// overwrite lower inventory clip-line with ----
			AnsiCell* row = ptr + (inventory.layout_y + 7)*width;
			for (int dx = inventory.layout_x + 3; dx < inventory.layout_x + 36; dx++)
			{
				if (dx >= 0 && dx < width)
				{
					row[dx].fg = black;
					row[dx].gl = 196;
				}
			}
		}
	}

	// ptr is valid till next Render
	// store it in Game for mouse handling

	if (!player.talk_box)
	{
		int items = 0, items_width = 0;
		int max_height = 0;
		while (inrange[items])
		{
			Sprite::Frame* frame = inrange[items]->proto->sprite_2d->atlas;

			if (1 + items_width + frame->width + items >= width - scene_shift)
				break;

			max_height = max_height < frame->height ? frame->height : max_height;
			items_width += frame->width;
			items++;
		}

		// store 
		// NET_TODO: store intersection of confirmed and rendered items!
		items_count = items;
		items_inrange = inrange;

		// crop pad_item index (note: it is in range (0..items_count, where 0 means no selection)
		input.pad_item = input.pad_item < items_count ? input.pad_item : items_count;

		int clip_width = width - scene_shift/2;

		// int items_x = (width - (items_width + items - 1)) / 2;
		int items_x = scene_shift/2 + (width - (items_width + items - 1)) / 2;
		int items_y = height / 2 - 2;

		items_y -= (items_y - max_height) / 2; // center below player

		if (items)
		{
			int y = items_y - max_height - 1;
			AnsiCell* ac;
			ac = ptr + items_x + y * width;
			ac->bk = AverageGlyph(ac, 0xF);
			ac->fg = black;
			ac->gl = 192;

			y++;

			for (; y < items_y; y++)
			{
				AnsiCell* ac;
				ac = ptr + items_x + y * width;
				ac->bk = AverageGlyph(ac, 0xF);
				ac->fg = black;
				ac->gl = 179;
			}

			ac = ptr + items_x + y * width;
			ac->bk = AverageGlyph(ac, 0xF);
			ac->fg = black;
			ac->gl = 218;

			items_x++;
		}

		for (int i = 0; i < items; i++)
		{
			items_xarr[i] = items_x-1;

			Sprite::Frame* frame = inrange[i]->proto->sprite_2d->atlas;

			if (i+1 == input.pad_item)
			{
				int x0 = items_x;
				int x1 = items_x + frame->width - 1;
				int y0 = items_y - max_height;
				int y1 = items_y - 1;

				AnsiCell* ac;
				for (int y=y0; y<=y1; y++)
				{
					for (int x=x0; x<=x1; x++)
					{
						ac = ptr + x + y * width;
						ac->bk = brown;
						ac->fg = black;
						ac->gl = 32;
					}
				}
			}			

			// check if in contact
			int in_contact = -1;
			for (int c = 0; c < 4; c++)
			{
				if (input.contact[c].action == Input::Contact::ITEM_LIST_DRAG ||
					input.contact[c].action == Input::Contact::ITEM_GRID_DRAG)
				{
					if (input.contact[c].item == inrange[i])
					{
						in_contact = c;
						break;
					}
				}
			}

			int y = items_y - (max_height + frame->height) / 2;

			if (in_contact < 0)
			{
				if (input.last_hit_char == '1' + i)
				{
					// check if in inventory there is xy to put this item at
					// ...

					if (!PickItem(inrange[i]))
					{
						// display status: "INVENTORY FULL / FRAGGED"
					}
				}

				BlitSprite(ptr, width, height, frame, items_x, y);
			}
			else
			{
				contact_item[contact_items] = in_contact; // deferred render
				contact_items++;
			}

			for (int x = items_x; x < items_x + frame->width; x++)
			{
				AnsiCell* ac;
				ac = ptr + x + items_y * width;
				ac->bk = AverageGlyph(ac, 0xF);
				ac->fg = black;
				ac->gl = 196;
				ac = ptr + x + (items_y - max_height - 1) * width;
				if (in_contact<0 && x == items_x + frame->width / 2)
				{
					ac->bk = black;
					ac->fg = white;
					ac->gl = '1' + i;
				}
				else
				{
					ac->bk = AverageGlyph(ac, 0xF);
					ac->fg = black;
					ac->gl = 196;
				}
			}

			items_x += frame->width;

			y = items_y - max_height - 1;

			AnsiCell* ac;
			ac = ptr + items_x + y * width;
			ac->bk = AverageGlyph(ac, 0xF);
			ac->fg = black;

			if (i == items - 1) // L
			{
				ac->gl = 217;
			}
			else // T
			{
				ac->gl = 193;
			}

			y++;

			for (; y < items_y; y++)
			{
				AnsiCell* ac;
				ac = ptr + items_x + y * width;
				ac->bk = AverageGlyph(ac, 0xF);
				ac->fg = black;
				ac->gl = 179;
			}

			ac = ptr + items_x + y * width;
			ac->bk = AverageGlyph(ac, 0xF);
			ac->fg = black;

			if (i == items - 1) // L
			{
				ac->gl = 191;
			}
			else // T
			{
				ac->gl = 194;
			}

			items_x++;
		}

		items_xarr[items] = items_x - 1;
		items_ylo = items_y - max_height - 1;
		items_yhi = items_y;

		if (input.pad_item)
		{
			// redraw frame hilight
			int x0 = items_xarr[input.pad_item-1];
			int x1 = items_xarr[input.pad_item];
			int y0 = items_ylo;
			int y1 = items_yhi;

			int xc = (x0+x1)/2;

			AnsiCell* ac;

			ac = ptr + x0 + y0 * width;
			ac->fg = yellow;
			ac->gl = 192; // ll

			ac = ptr + x1 + y0 * width;
			ac->fg = yellow;
			ac->gl = 217; // lr

			ac = ptr + x0 + y1 * width;
			ac->fg = yellow;
			ac->gl = 218; // ul

			ac = ptr + x1 + y1 * width;
			ac->fg = yellow;
			ac->gl = 191; // ur

			for (int x=x0+1; x< x1; x++)
			{
				ac = ptr + x + y0 * width;
				ac->fg = yellow;
				if (x != xc)
					ac->gl = 196; // hor

				ac = ptr + x + y1 * width;
				ac->fg = yellow;
				ac->gl = 196; // hor
			}
			for (int y=y0+1; y< y1; y++)
			{
				ac = ptr + x0 + y * width;
				ac->fg = yellow;
				ac->gl = 179; // ver

				ac = ptr + x1 + y * width;
				ac->fg = yellow;
				ac->gl = 179; // ver
			}
		}
	}
	else
	{
		items_count = 0;
	}


	for (int ic = 0; ic < contact_items; ic++)
	{
		int in_contact = contact_item[ic];
		int cp[2] = { input.contact[in_contact].pos[0], input.contact[in_contact].pos[1] };
		ScreenToCell(cp);

		Sprite::Frame* frame = input.contact[in_contact].item->proto->sprite_2d->atlas;

		cp[0] -= frame->width / 2;

		if (ic==0) // if dragged by touch, leave it above finger 
			cp[1] -= frame->height / 2;

		// if this item is currently in use
		// trap it inside inventory
		if (input.contact[in_contact].action == Input::Contact::ITEM_GRID_DRAG &&
			inventory.my_item[input.contact[in_contact].my_item].in_use)
		{
			if (cp[0] < inventory.layout_x)
				cp[0] = inventory.layout_x;
			if (cp[0] + frame->width > inventory.layout_x + inventory.layout_width)
				cp[0] = inventory.layout_x + inventory.layout_width - frame->width;

			if (cp[1] < inventory.layout_y)
				cp[1] = inventory.layout_y;
			if (cp[1] + frame->height > inventory.layout_y + inventory.layout_height)
				cp[1] = inventory.layout_y + inventory.layout_height - frame->height;
		}

		BlitSprite(ptr, width, height, frame, cp[0], cp[1]);
	}

	{
		HPBar bar;

		int bar_w = (width - 2 * 7) / 3;
		int hp_xyw[] = { bars_pos, height - bar.height, bar_w };
		int mp_xyw[] = { width - bars_pos - bar_w, height - bar.height, bar_w };

		//static int f = 0;
		//f++;
		// float val = 0.5*(1.0 + sinf(f*0.02));

		float val = (float)player.HP / player.MAX_HP;
		bar.Paint(ptr, width, height, val, hp_xyw, false);
		bar.Paint(ptr, width, height, 1.0f, mp_xyw, true);

		BlitSprite(ptr, width, height, character_button->atlas + 0, bars_pos-7, height - character_button->atlas[0].height);
		BlitSprite(ptr, width, height, character_button->atlas + 1, 7-bars_pos + width - character_button->atlas[1].width, height - character_button->atlas[1].height);
	}

	if (player.talk_box)
		player.talk_box->Paint(ptr, width, height, width / 2 + scene_shift/2, height / 2 + 8, (TalkBox_blink & 63) < 32);

	if (show_buts && bars_pos < 7)
		bars_pos++;
	if (!show_buts && bars_pos > 0)
		bars_pos--;

	int kh = keyb.Height(width, height);

	if (show_keyb || keyb_hide < kh)
	{
		#if 0
		int mul, keyb_width;
		for (int mode = 1; mode <= 16; mode++)
		{
			mul = mode;
			keyb_width = keyb.Width(mul);
			if (keyb_width + 1 >= width)
			{
				mul--;
				keyb_width = keyb.Width(mul);
				break;
			}
		}

		keyb_pos[0] = (width - keyb_width) / 2;
		keyb_pos[1] = 1 - keyb_hide;
		keyb_mul = mul;

		uint8_t key[32];
		for (int i = 0; i < 32; i++)
			key[i] = keyb_key[i] | input.key[i];
		keyb.Paint(ptr, keyb_pos[0], keyb_pos[1], width, height, keyb_mul, key);
		#endif

		// w = (sprite_w-1) * 10.5 + 1

		// (sprite_w-1) * 10.5 = w-1
		// sprite_w-1 = (w-1)/10.5
		// sprite_w = (w-1)/10.5 + 1


		uint8_t key[32];
		for (int i = 0; i < 32; i++)
			key[i] = keyb_key[i] | input.key[i];
		keyb.Paint(ptr, width, height, keyb_hide, key, input.pad_connected);
	}
	
	if (show_keyb)
	{
		if (keyb_hide > kh)
			keyb_hide = kh;

		if (keyb_hide > 0)
		{
			keyb_hide -= f120;
			if (keyb_hide < 0)
				keyb_hide = 0;
		}
	}
	else
	{
		if (keyb_hide < kh)
		{
			keyb_hide += f120;
			if (keyb_hide > kh)
				keyb_hide = kh;
		}
	}

	input.last_hit_char = 0;

	stamp = _stamp;

	if (server)
	{
		if (stamp - server->last_lag >= 100000 && !server->lag_wait) // 10x per sec
		{
			server->last_lag = stamp;
			server->lag_wait = true;

			STRUCT_REQ_LAG req_lag = { 0 };
			req_lag.token = 'L';
			uint32_t s = (uint32_t)stamp;
			req_lag.stamp[0] = s & 0xFF;
			req_lag.stamp[1] = (s >> 8) & 0xFF;
			req_lag.stamp[2] = (s >> 16) & 0xFF;
			
			server->Send((const uint8_t*)&req_lag, sizeof(STRUCT_REQ_LAG));
		}

		if (steps)
		{
			STRUCT_REQ_POSE req_pose = { 0 };
			req_pose.token = 'P';
			req_pose.am = (player.req.action<<4) | player.req.mount;
			req_pose.anim= player.anim;
			req_pose.frame = player.frame;
			req_pose.pos[0] = player.pos[0];
			req_pose.pos[1] = player.pos[1];
			req_pose.pos[2] = player.pos[2];
			req_pose.dir = player.dir;
			req_pose.sprite = 
				(player.req.armor << 12) | 
				(player.req.helmet << 8) |
				(player.req.shield << 4) |
				player.req.weapon; // 0xAHSW

			server->Send((const uint8_t*)&req_pose, sizeof(STRUCT_REQ_POSE));
		}
	}

	if (show_gamepad)
		PaintGamePad(ptr, width, height, stamp);
		
	PaintMenu(ptr, width, height);
	/*
	Font1Paint(ptr, width, height, 10, 22, "0.123456789\nZOMBEK DOMBEK\nZIBALABULGAMUF?", 0);
	Font1Paint(ptr, width, height, 10, 10, "0.123456789\nZOMBEK DOMBEK\nZIBALABULGAMUF?", 1);
	*/

	if (input.shot)
	{
		input.shot = false;
		FILE* f = fopen("./shot.xp", "wb");
		if (f)
		{
			uint32_t hdr[4] = { (uint32_t)-1, (uint32_t)1, (uint32_t)width, (uint32_t)height };
			fwrite(hdr, sizeof(uint32_t), 4, f);
			for (int x = 0; x < width; x++)
			{
				for (int y = height - 1; y >= 0; y--)
				{
					AnsiCell* c = ptr + y * width + x;
					int fg = c->fg - 16;
					int f_r = (fg % 6) * 51; fg /= 6;
					int f_g = (fg % 6) * 51; fg /= 6;
					int f_b = (fg % 6) * 51; fg /= 6;

					int bk = c->bk - 16;
					int b_r = (bk % 6) * 51; bk /= 6;
					int b_g = (bk % 6) * 51; bk /= 6;
					int b_b = (bk % 6) * 51; bk /= 6;

					uint8_t f_rgb[3] = { (uint8_t)f_b,(uint8_t)f_g,(uint8_t)f_r };
					uint8_t b_rgb[3] = { (uint8_t)b_b,(uint8_t)b_g,(uint8_t)b_r };
					uint32_t chr = c->gl;

					fwrite(&chr, sizeof(uint32_t), 1, f);
					fwrite(f_rgb, 1, 3, f);
					fwrite(b_rgb, 1, 3, f);
				}
			}

			fclose(f);
		}
	}
}

void Game::OnSize(int w, int h, int fw, int fh)
{
	bool pad = input.pad_connected;
	memset(&input, 0, sizeof(Input));
	input.pad_connected = pad;
	input.size[0] = w;
	input.size[1] = h;
	font_size[0] = fw;
	font_size[1] = fh;
}

void Game::OnKeyb(GAME_KEYB keyb, int key)
{
	/*
	if (keyb==KEYB_CHAR)
	{
		int freq = 100 + fast_rand()%100;
		CallAudio((uint8_t*)&freq,sizeof(freq));
	}
	*/

	if (keyb == GAME_KEYB::KEYB_DOWN)	
	{
		bool auto_rep = (key & A3D_AUTO_REPEAT) != 0;
		int shot_key = key & ~A3D_AUTO_REPEAT;

		if (shot_key == A3D_F10 && !auto_rep)
			input.shot = true;
	}

	// handle layers first ...
	if (menu_depth>=0)
	{
		MenuKeyb(keyb,key);
		return;
	}

	if (show_gamepad)
	{
		int k = -1;
		switch (keyb)
		{
			case GAME_KEYB::KEYB_CHAR:
			{
				switch (key)
				{
					case ' ': k = 0; break;
					case '\n': k = 1; break;
					case 8:
					case '\\':
					case 27: k = 2; break;

					default:
						if (key>32 && key<127)
							k = key;
				}
				break;
			}

			case GAME_KEYB::KEYB_PRESS:
			case GAME_KEYB::KEYB_DOWN:
			{
				switch (key)
				{
					case A3D_ENTER: k = 1; break;
					case A3D_ESCAPE: k = 2; break;
					case A3D_UP: k = 3; break;
					case A3D_DOWN: k = 4; break;
					case A3D_LEFT: k = 5; break;
					case A3D_RIGHT: k = 6; break;
				}
				break;
			}

			default:
				break;
		}

		if (k>=0)
			GamePadKeyb(k, stamp);

		return;
	}
	

	// if nothing focused 

	// in case it comes from the real keyboard
	// if emulated, theoretically caller must revert it
	// but in practice it will reset it later to emulated cap
	KeybAutoRepChar = 0;

	if (keyb == GAME_KEYB::KEYB_DOWN)
	{
		if (key == A3D_ESCAPE)
		{
			// cancel all contacts
			for (int i = 0; i < 4; i++)
			{
				input.contact[i].action = Input::Contact::NONE;
			}
		}

		bool auto_rep = (key & A3D_AUTO_REPEAT) != 0;
		key &= ~A3D_AUTO_REPEAT;

		if ((key == A3D_TAB || key == A3D_ESCAPE) && !auto_rep)
		{
			if (!player.talk_box && key == A3D_TAB/* && show_buts*/)
			{
				CancelItemContacts();
				//show_buts = false;
				TalkBox_blink = 32;
				player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
				memset(player.talk_box, 0, sizeof(TalkBox));
				player.talk_box->max_width = 33;
				player.talk_box->max_height = 7; // 0: off
				int s[2],p[2];
				player.talk_box->Reflow(s,p);
				player.talk_box->size[0] = s[0];
				player.talk_box->size[1] = s[1];
				player.talk_box->cursor_xy[0] = p[0];
				player.talk_box->cursor_xy[1] = p[1];
			}
			else
			if (player.talk_box)
			{
				//show_buts = true;

				if (player.talk_box->len > 0 && key == A3D_TAB)
				{
					if (player.talks == 3)
					{
						free(player.talk[0].box);
						player.talks--;
						for (int i = 0; i < player.talks; i++)
							player.talk[i] = player.talk[i + 1];
					}

					int idx = player.talks;
					player.talk[idx].box = player.talk_box;
					player.talk[idx].pos[0] = player.pos[0];
					player.talk[idx].pos[1] = player.pos[1];
					player.talk[idx].pos[2] = player.pos[2];
					player.talk[idx].stamp = stamp;

					if (server)
					{
						STRUCT_REQ_TALK req_talk = { 0 };
						req_talk.token = 'T';
						req_talk.len = player.talk[idx].box->len;
						memcpy(req_talk.str, player.talk[idx].box->buf, player.talk[idx].box->len);
						server->Send((const uint8_t*)&req_talk, 4 + req_talk.len);
					}

					ChatLog("%s : %.*s\n", player.name, player.talk[player.talks].box->len, player.talk[player.talks].box->buf);
					player.talks++;

					// alloc new
					player.talk_box = 0;

					TalkBox_blink = 32;
					player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
					memset(player.talk_box, 0, sizeof(TalkBox));
					player.talk_box->max_width = 33;
					player.talk_box->max_height = 7; // 0: off
					int s[2], p[2];
					player.talk_box->Reflow(s, p);
					player.talk_box->size[0] = s[0];
					player.talk_box->size[1] = s[1];
					player.talk_box->cursor_xy[0] = p[0];
					player.talk_box->cursor_xy[1] = p[1];
				}
				else
				{
					free(player.talk_box);
					player.talk_box = 0;
				}

				if (show_keyb)
					memset(keyb_key, 0, 32);
				show_keyb = false;
				KeybAutoRepChar = 0;
				KeybAutoRepCap = 0;
				for (int i=0; i<4; i++)
				{
					if (input.contact[i].action == Input::Contact::KEYBCAP)
						input.contact[i].action = Input::Contact::NONE;
				}
			}
			else
			if (show_inventory && key == A3D_ESCAPE)
			{
				CancelItemContacts();
				show_inventory = false;
			}
		}

		if (!player.talk_box && !auto_rep)
		{
			if (key == A3D_SPACE)
				input.jump = true;

			/*
			// HANDLED AS CHAR (common for all)
			if (key == A3D_ENTER)
				player.SetActionAttack(stamp);
			*/

			// god mode
			if (key == A3D_HOME)
				player.SetActionStand(stamp);
			if (key == A3D_END)
				player.SetActionFall(stamp);

			if (key == A3D_OEM_PERIOD)
				player.SetMount(MOUNT::WOLF);
			if (key == A3D_OEM_SLASH)
				player.SetMount(MOUNT::BEE);
			if (key == A3D_OEM_COMMA)
				player.SetMount(MOUNT::NONE);
		}

		if (!auto_rep)
			input.key[key >> 3] |= 1 << (key & 0x7);

		// temp (also with auto_rep)
		TalkBox_blink = 0;
		if (player.talk_box)
		{
			if (key == A3D_PAGEUP)
				player.talk_box->MoveCursorHead();
			if (key == A3D_PAGEDOWN)
				player.talk_box->MoveCursorTail();
			if (key == A3D_HOME)
				player.talk_box->MoveCursorHome();
			if (key == A3D_END)
				player.talk_box->MoveCursorEnd();
			if (key == A3D_LEFT)
				player.talk_box->MoveCursorX(-1);
			if (key == A3D_RIGHT)
				player.talk_box->MoveCursorX(+1);
			if (key == A3D_UP)
				player.talk_box->MoveCursorY(-1);
			if (key == A3D_DOWN)
				player.talk_box->MoveCursorY(+1);

			int mem_idx = -1;
			switch (key)
			{
				case A3D_F5: mem_idx = 0; break;
				case A3D_F6: mem_idx = 1; break;
				case A3D_F7: mem_idx = 2; break;
				case A3D_F8: mem_idx = 3; break;
			}

			if (mem_idx >= 0)
			{
				// store, even if empty
				memcpy(talk_mem[mem_idx].buf, player.talk_box->buf, 256);
				talk_mem[mem_idx].len = player.talk_box->len;
				WriteConf(this);
			}				
		}
		else
		{
			int mem_idx = -1;
			switch (key)
			{
				case A3D_F5: mem_idx = 0; break;
				case A3D_F6: mem_idx = 1; break;
				case A3D_F7: mem_idx = 2; break;
				case A3D_F8: mem_idx = 3; break;
			}

			if (mem_idx >= 0 && talk_mem[mem_idx].len > 0)
			{
				// immediate post
				TalkBox* box = 0;
				if (player.talks == 3)
				{
					box = player.talk[0].box;
					player.talks--;
					for (int i = 0; i < player.talks; i++)
						player.talk[i] = player.talk[i + 1];
				}
				else
					box = (TalkBox*)malloc(sizeof(TalkBox));

				memset(box, 0, sizeof(TalkBox));
				memcpy(box->buf, talk_mem[mem_idx].buf, 256);
				box->len = talk_mem[mem_idx].len;
				box->cursor_pos = box->len;

				box->max_width = 33;
				box->max_height = 7; // 0: off
				int s[2], p[2];
				box->Reflow(s, p);
				box->size[0] = s[0];
				box->size[1] = s[1];
				box->cursor_xy[0] = p[0];
				box->cursor_xy[1] = p[1];

				player.talk[player.talks].pos[0] = player.pos[0];
				player.talk[player.talks].pos[1] = player.pos[1];
				player.talk[player.talks].pos[2] = player.pos[2];
				player.talk[player.talks].box = box;
				player.talk[player.talks].stamp = stamp;

				if (server)
				{
					int idx = player.talks;
					STRUCT_REQ_TALK req_talk = { 0 };
					req_talk.token = 'T';
					req_talk.len = player.talk[idx].box->len;
					memcpy(req_talk.str, player.talk[idx].box->buf, player.talk[idx].box->len);
					server->Send((const uint8_t*)&req_talk, 4 + req_talk.len);
				}				

				ChatLog("%s : %.*s\n", player.name, player.talk[player.talks].box->len, player.talk[player.talks].box->buf);
				player.talks++;
			}

			if (show_inventory)
			{
				if (input.contact[3].action == Input::Contact::ITEM_GRID_DRAG)
				{
					switch (key)
					{
						case A3D_UP:
							input.contact[3].pos[1]-=font_size[1];
							break;
						case A3D_DOWN:
							input.contact[3].pos[1]+=font_size[1];
							break;
						case A3D_LEFT:
							input.contact[3].pos[0]-=font_size[0];
							break;
						case A3D_RIGHT:
							input.contact[3].pos[0]+=font_size[0];
							break;
					}
				}
				else
				{
					switch (key)
					{
						case A3D_UP:
							inventory.FocusNext(0, 1);
							break;
						case A3D_DOWN:
							inventory.FocusNext(0, -1);
							break;
						case A3D_LEFT:
							inventory.FocusNext(-1, 0);
							break;
						case A3D_RIGHT:
							inventory.FocusNext(1, 0);
							break;
					}
				}
			}
		}
	}
	else
	if (keyb == GAME_KEYB::KEYB_UP)
		input.key[key >> 3] &= ~(1 << (key & 0x7));
	else
	if (keyb == GAME_KEYB::KEYB_CHAR)
	{
		input.last_hit_char = key;

		if (!player.talk_box)
		{
			if (key == '\n' || key == '\r')
			{
				input.shoot = true;
				input.shoot_xy[0] = -1;
				input.shoot_xy[1] = -1;

				player.SetActionAttack(stamp);
			}

			if (key == '\\' || key == '|')
			{
				// perspective = !perspective;
				ToggleMenu(0);
			}

			if (key == '0')
			{
				// drop last item from inventory
				if (inventory.my_items)
				{
					if (inventory.my_items > 0)
					{
						// only if not in use
						if (!inventory.my_item[inventory.focus].in_use)
							DropItem(inventory.focus);
					}
				}
			}

			if (key=='y' || key=='Y')
			{
				if (show_inventory)
				{
					Input::Contact* con = input.contact+3;
					if (con->action == Input::Contact::NONE && inventory.focus>=0)
					{
						// cancel all contacts
						// ... or prevent entering 'y' state if already there is item contact
						CancelItemContacts();

						// get xy from focused item
						int* xy = inventory.my_item[inventory.focus].xy;

						// keyb_y = 0
						// gamepad_x = 1
						// gamepad_y = 2

						int but = 0; // keyb_y
						// StartContact(3/*KEYB/PAD*/, xy[0],xy[1], but);

						// synthetize contact
						
						con->action = Input::Contact::ITEM_GRID_DRAG;
						con->item = inventory.my_item[inventory.focus].item;


						con->my_item = inventory.focus; // ?

						// calc synthetized scrren position for contact

						int ix = inventory.layout_x + xy[0]*4 + 4;
						int iy = inventory.layout_y + xy[1]*4 + 
								 inventory.layout_height - 6 - (inventory.height*4-1) + 
								 inventory.scroll;

						// centered on item
						ix += con->item->proto->sprite_2d->atlas->width/2;
						iy += con->item->proto->sprite_2d->atlas->height/2;	

						// shift slightly UP (so user can see we're in moving state)
						iy++;

						// flip y axis (sceen coords are top to bottom)
						iy = render_size[1] - 1 - iy;	

						con->drag = 0; // button
						con->pos[0] = ix*font_size[0] + font_size[0]/2;
						con->pos[1] = iy*font_size[1] + font_size[1]/2;
						con->drag_from[0] = con->pos[0];
						con->drag_from[1] = con->pos[1];

						con->keyb_cap = -1;
						con->margin = 0;
						con->player_hit = false;
						con->start_yaw = 0;
					}
					else
					if (con->action == Input::Contact::ITEM_GRID_DRAG)
					{
						// try to emplace item
						EndContact(3,con->pos[0],con->pos[1]);
					}
				}
			}

			if (key=='i' || key=='I')
			{
				if (show_inventory)
				{
					CancelItemContacts();
					show_inventory = false;
				}
				else
				{
					show_inventory = true;
				}			
			}

			if (key == 'u' || key == 'U')
			{
				if (inventory.focus >= 0)
				{
					if (show_inventory)
					{
						CancelItemContacts();
						ExecuteItem(inventory.focus);
					}
					else
					{
						// using food, potion, drink cannot be undone
						// so not allowed to use when inventory is hidden
						Item* item = inventory.my_item[inventory.focus].item;
						switch (item->proto->kind)
						{
							case 'F': // food
							case 'P': // potion
							case 'D': // drink
								break;
							default:
								CancelItemContacts();
								ExecuteItem(inventory.focus);
						}
					}
				}
			}
		}

		if (key != 9) // we skip all TABs
		{
			if (key == '\r') // windows only? todo: check linux and browsers
				key = '\n';
			if ((key < 32 || key > 127) && key!=8 && key!='\n')
				return;

			// if type box is visible pass this input to it
			// printf("CH:%d (%c)\n", key, key);

			TalkBox_blink = 0;
			if (player.talk_box)
				player.talk_box->Input(key);
		}
	}
	else
	if (keyb == GAME_KEYB::KEYB_PRESS)
	{
		// it is like a KEYB_CHAR (not producing releases) but for non-printable keys
		// main input from terminals 
		// ....

		TalkBox_blink = 0;
		if (player.talk_box)
		{
			if (key == A3D_PAGEUP)
				player.talk_box->MoveCursorHead();
			if (key == A3D_PAGEDOWN)
				player.talk_box->MoveCursorTail();
			if (key == A3D_HOME)
				player.talk_box->MoveCursorHome();
			if (key == A3D_END)
				player.talk_box->MoveCursorEnd();			
			if (key == A3D_LEFT)
				player.talk_box->MoveCursorX(-1);
			if (key == A3D_RIGHT)
				player.talk_box->MoveCursorX(+1);
			if (key == A3D_UP)
				player.talk_box->MoveCursorY(-1);
			if (key == A3D_DOWN)
				player.talk_box->MoveCursorY(+1);

			int mem_idx = -1;
			switch (key)
			{
				case A3D_F5: mem_idx = 0; break;
				case A3D_F6: mem_idx = 1; break;
				case A3D_F7: mem_idx = 2; break;
				case A3D_F8: mem_idx = 3; break;
			}

			if (mem_idx >= 0)
			{
				// store, even if empty
				memcpy(talk_mem[mem_idx].buf, player.talk_box->buf, 256);
				talk_mem[mem_idx].len = player.talk_box->len;
				WriteConf(this);
			}
		}
		else
		{
			int mem_idx = -1;
			switch (key)
			{
				case A3D_F5: mem_idx = 0; break;
				case A3D_F6: mem_idx = 1; break;
				case A3D_F7: mem_idx = 2; break;
				case A3D_F8: mem_idx = 3; break;
			}

			if (mem_idx >= 0 && talk_mem[mem_idx].len > 0)
			{
				// immediate post
				TalkBox* box = 0;
				if (player.talks == 3)
				{
					box = player.talk[0].box;
					player.talks--;
					for (int i = 0; i < player.talks; i++)
						player.talk[i] = player.talk[i + 1];
				}
				else
					box = (TalkBox*)malloc(sizeof(TalkBox));

				memset(box, 0, sizeof(TalkBox));
				memcpy(box->buf, talk_mem[mem_idx].buf, 256);
				box->len = talk_mem[mem_idx].len;
				box->cursor_pos = box->len;

				box->max_width = 33;
				box->max_height = 7; // 0: off
				int s[2], p[2];
				box->Reflow(s, p);
				box->size[0] = s[0];
				box->size[1] = s[1];
				box->cursor_xy[0] = p[0];
				box->cursor_xy[1] = p[1];

				player.talk[player.talks].pos[0] = player.pos[0];
				player.talk[player.talks].pos[1] = player.pos[1];
				player.talk[player.talks].pos[2] = player.pos[2];
				player.talk[player.talks].box = box;
				player.talk[player.talks].stamp = stamp;

				if (server)
				{
					int idx = player.talks;
					STRUCT_REQ_TALK req_talk = { 0 };
					req_talk.token = 'T';
					req_talk.len = player.talk[idx].box->len;
					memcpy(req_talk.str, player.talk[idx].box->buf, player.talk[idx].box->len);
					server->Send((const uint8_t*)&req_talk, 4 + req_talk.len);
				}				

				ChatLog("%s : %.*s\n", player.name, player.talk[player.talks].box->len, player.talk[player.talks].box->buf);
				player.talks++;
			}


			// simulate key down / up based on a time relaxation
			// for: QWEASD and cursor keys

			// here: 
			// if new key is different than stored key
			//   then: emulate stored KEY_UP and new KEY_DOWN
			// store current stamp
			// store new key

			// in render(): 
			// if there is stored key and time elapsed since it was pressed > thresh
			//   then: emulate stored KEY_UP and clear stored key

			if (key != PressKey)
			{
				OnKeyb(GAME_KEYB::KEYB_UP, PressKey);
				PressKey = 0;

				// here we can filter keys
				if (key != A3D_TAB && (key<A3D_F5 || key>A3D_F8))
				{
					PressKey = key;
					PressStamp = stamp;
					OnKeyb(GAME_KEYB::KEYB_DOWN, PressKey);
				}
			}
			else
			{
				PressStamp = stamp; // - 500000 + 50000;
			}
		}

		if (key == A3D_TAB)
		{
			// HANDLED BY EMULATION!
			if (!player.talk_box/* && show_buts*/)
			{
				CancelItemContacts();
				//show_buts = false;
				TalkBox_blink = 32;
				player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
				memset(player.talk_box, 0, sizeof(TalkBox));
				player.talk_box->max_width = 33;
				player.talk_box->max_height = 7; // 0: off
				int s[2],p[2];
				player.talk_box->Reflow(s,p);
				player.talk_box->size[0] = s[0];
				player.talk_box->size[1] = s[1];
				player.talk_box->cursor_xy[0] = p[0];
				player.talk_box->cursor_xy[1] = p[1];
			}
			else
			{
				//show_buts = true;
				free(player.talk_box);
				player.talk_box = 0;
				if (show_keyb)
					memset(keyb_key, 0, 32);
				show_keyb = false;
				KeybAutoRepChar = 0;
				KeybAutoRepCap = 0;
				for (int i=0; i<4; i++)
				{
					if (input.contact[i].action == Input::Contact::KEYBCAP)
						input.contact[i].action = Input::Contact::NONE;
				}
			}
		}		
	}
}

static bool PlayerHit(Game* g, int x, int y)
{
	int down[2] = { x,y };
	g->ScreenToCell(down);

	down[0] -= g->scene_shift/2;

	// if talkbox is open check it too
	if (g->player.talk_box)
	{
		int w = g->player.talk_box->size[0] + 3;
		int left = g->render_size[0] / 2 - w / 2;
		int center = left + w / 2;
		int right = left + w - 1;
		int bottom = g->render_size[1] / 2 + 8;
		int lower = bottom + 1;
		int upper = bottom + 4 + g->player.talk_box->size[1];

		if (down[0] >= left && down[0] <= right && down[1] >= lower && down[1] <= upper || down[0] == center && down[1] == bottom)
			return true;
	}

	Sprite* s = g->player.sprite;
	if (s)
	{
		int ang = (int)floor((g->player.dir - g->prev_yaw) * s->angles / 360.0f + 0.5f);
		ang = ang >= 0 ? ang % s->angles : (ang % s->angles + s->angles) % s->angles;
		int i = g->player.frame + ang * s->anim[g->player.anim].length;
		// refl: i += s->anim[anim].length * s->angles;
		Sprite::Frame* f = s->atlas + s->anim[g->player.anim].frame_idx[i];

		int sx = down[0] - g->render_size[0] / 2 + f->ref[0] / 2;
		int sy = down[1] - g->render_size[1] / 2 + f->ref[1] / 2;

		if (sx >= 0 && sx < f->width && sy >= 0 && sy < f->height)
		{
			AnsiCell* ac = f->cell + sx + f->width * sy;
			if (ac->fg != 255 && ac->gl != 0 && ac->gl != 32 || ac->bk != 255 && ac->gl != 219)
			{
				return true;
			}
		}
	}

	return false;
}

void Game::StartContact(int id, int x, int y, int b)
{
	Input::Contact* con = input.contact + id;

	int cp[2] = { x,y };
	ScreenToCell(cp);

	bool hit = false;
	int mrg = 0;
	int cap = -1;
	float yaw = 0;
	Item* item = 0;

	if (/*show_buts &&*/ cp[1] >= render_size[1] - 6 && (cp[0] < bars_pos || cp[0] >= render_size[0] - bars_pos) && b == 1)
	{
		// main but
		// perform action immediately
		// ...

		if (cp[0] >= render_size[0] - bars_pos)
		{
			// temporarily switch ortho/perspective
			//perspective = !perspective;
			if (id==0)
				ToggleMenu(1);
			else
				ToggleMenu(2);

			// temporarily drop item
			/*
			if (inventory.my_items > 0)
			{
				// only if not in use
				if (!inventory.my_item[inventory.focus].in_use)
					DropItem(inventory.focus);
			}
			*/

			// TODO:
			// SHOW HELP!
		}
		else
		{
			if (show_inventory)
			{
				CancelItemContacts();
				show_inventory = false;
			}
			else
			{
				show_inventory = true;
			}
			
		}

		// make contact dead
		con->action = Input::Contact::NONE;
		con->drag = b;
		con->pos[0] = x;
		con->pos[1] = y;
		con->drag_from[0] = x;
		con->drag_from[1] = y;

		con->item = item;
		con->keyb_cap = cap;
		con->margin = mrg;
		con->player_hit = hit;
		con->start_yaw = yaw;
		return;
	}

	if (show_inventory && !player.talk_box)
	{
		// if this is touch and theres another grid click touch
		// synthetize consumption / use / unuse
		if (id>0)
		{
			for (int i=1; i<4; i++)
			{
				if (i==id)
					continue;
				if (input.contact[i].action == Input::Contact::ITEM_GRID_CLICK)
				{
					ExecuteItem(input.contact[i].my_item);
					input.contact[i].action = Input::Contact::NONE;
					con->action = Input::Contact::NONE;
					return;
				}
			}
		}

		bool can_scroll = false;
		bool inside = false;

		int _x = x, _y = y;

		{ // protect x,y
			int width = render_size[0];
			int height = render_size[1];
			Sprite::Frame* sf = inventory_sprite->atlas;

			// check if inside widget
			int left = inventory.layout_frame[0];
			int right = inventory.layout_frame[2];
			int upper = inventory.layout_frame[3];
			int lower = inventory.layout_frame[1];

			if (cp[0] >= inventory.layout_x && 
				cp[0] < inventory.layout_x + inventory.layout_width && 
				cp[1] >= inventory.layout_y && 
				cp[1] < inventory.layout_y + inventory.layout_height)
			{
				inside = true;

				if (cp[0] <= left || cp[0] >= right || cp[1] <= lower || cp[1] >= upper)
				{
					if (b==1)
						can_scroll = true;
				}
				else
				{
					int my_item = CheckPick(cp);
					// check for unused space
					if (my_item < 0 && b==1)
						can_scroll = true;
					else
					if (my_item >=0)
					{
						// if this item is not handled by any other contact

						bool can_click = true;

						for (int c = 0; c < 4; c++)
						{
							if (input.contact[c].action == Input::Contact::ITEM_LIST_CLICK ||
								input.contact[c].action == Input::Contact::ITEM_LIST_DRAG ||
								input.contact[c].action == Input::Contact::ITEM_GRID_CLICK ||
								input.contact[c].action == Input::Contact::ITEM_GRID_DRAG)
							{
								// if (my_item == input.contact->my_item)
								// ENFORCED TO ONE SCROLL AND ONE CLICK/DRAG CONTACT AT ONCE
								{
									can_click = false;
									break;
								}
							}
						}

						if (can_click)
						{
							inventory.SetFocus(my_item);
							inventory.animate_scroll = true;

							// note: in case of mouse, it can be right or left click
							// left is for entire group move/remove, right is for 1 entity move/remove (split) 
							con->action = Input::Contact::ITEM_GRID_CLICK;
							con->my_item = my_item;
							con->item = inventory.my_item[my_item].item;

							con->drag = b;
							con->pos[0] = _x;
							con->pos[1] = _y;
							con->drag_from[0] = _x;
							con->drag_from[1] = _y;

							con->keyb_cap = cap;
							con->margin = mrg;
							con->player_hit = hit;
							con->start_yaw = yaw;
							return;
						}
					}
				}
			}
		}

		if (can_scroll)
		{
			for (int i = 0; i < 4; i++)
			{
				if (input.contact[i].action == Input::Contact::ITEM_GRID_SCROLL)
				{
					can_scroll = false;
					break;
				}
			}

			if (can_scroll)
			{
				inventory.animate_scroll = false;
				con->action = Input::Contact::ITEM_GRID_SCROLL;
				con->scroll = inventory.scroll;

				con->drag = b;
				con->pos[0] = x;
				con->pos[1] = y;
				con->drag_from[0] = x;
				con->drag_from[1] = y;

				con->item = item;
				con->keyb_cap = cap;
				con->margin = mrg;
				con->player_hit = hit;
				con->start_yaw = yaw;
				return;
			}
		}
		else
		if (inside)
		{
			con->action = Input::Contact::NONE;
			con->drag = b;
			con->pos[0] = x;
			con->pos[1] = y;
			con->drag_from[0] = x;
			con->drag_from[1] = y;

			con->item = item;
			con->keyb_cap = cap;
			con->margin = mrg;
			con->player_hit = hit;
			con->start_yaw = yaw;
			return;
		}
	}

	if (items_count && cp[1] >= items_ylo && cp[1] <= items_yhi && b == 1)
	{
		// check item pickup
		if (cp[1] > items_ylo && cp[1] < items_yhi)
		{
			for (int i = 0; i < items_count; i++)
			{
				if (cp[0] > items_xarr[i] && cp[0] < items_xarr[i + 1])
				{
					// ensure no other contact with this item
					bool ok = true;
					for (int c = 0; c < 4; c++)
					{
						if (input.contact[c].action == Input::Contact::ITEM_LIST_CLICK ||
							input.contact[c].action == Input::Contact::ITEM_LIST_DRAG ||
							input.contact[c].action == Input::Contact::ITEM_GRID_CLICK ||
							input.contact[c].action == Input::Contact::ITEM_GRID_DRAG)
						{
							ok = false;
							break;
						}
					}

					if (!ok)
						break;

					con->action = Input::Contact::ITEM_LIST_CLICK;
					con->item = items_inrange[i];

					con->drag = b;
					con->pos[0] = x;
					con->pos[1] = y;
					con->drag_from[0] = x;
					con->drag_from[1] = y;

					con->keyb_cap = cap;
					con->margin = mrg;
					con->player_hit = hit;
					con->start_yaw = yaw;
					return;

					/*
					if (!PickItem(items_inrange[i]))
					{
						// display status: "INVENTORY FULL / FRAGGED"
					}

					con->action = Input::Contact::NONE;
					con->drag = b;
					con->pos[0] = x;
					con->pos[1] = y;
					con->drag_from[0] = x;
					con->drag_from[1] = y;

					con->item = item;
					con->keyb_cap = cap;
					con->margin = mrg;
					con->player_hit = hit;
					con->start_yaw = yaw;
					return;
					*/
				}
			}
		}

		// slightly missed (border / frame between items in list)
		if (cp[0] >= items_xarr[0] && cp[0] <= items_xarr[items_count])
		{
			con->action = Input::Contact::NONE;
			con->drag = b;
			con->pos[0] = x;
			con->pos[1] = y;
			con->drag_from[0] = x;
			con->drag_from[1] = y;

			con->item = item;
			con->keyb_cap = cap;
			con->margin = mrg;
			con->player_hit = hit;
			con->start_yaw = yaw;
			return;
		}
	}

	{
		if (show_keyb)
		{
			bool left_shift = ((input.key[A3D_LSHIFT >> 3] | keyb_key[A3D_LSHIFT >> 3]) & (1 << (A3D_LSHIFT & 7))) != 0;
			bool right_shift = ((input.key[A3D_RSHIFT >> 3] | keyb_key[A3D_RSHIFT >> 3]) & (1 << (A3D_RSHIFT & 7))) != 0;
			bool shift_on = left_shift || right_shift;
			
			char ch=0;
			
			#if 0
			cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, &ch, shift_on);
			#endif
			cap = keyb.GetCap(cp[0], cp[1], render_size[0], render_size[1], &ch, shift_on);

			if (left_shift && cap == A3D_RSHIFT)
			{
				keyb.plane = (keyb.plane + 1) % 3; // cycle++
				keyb.pad_plane = false; // prevent resetting by pad
			}
			else
			if (right_shift && cap == A3D_LSHIFT)
			{
				keyb.plane = (keyb.plane + 2) % 3; // cycle--
				keyb.pad_plane = false; // prevent resetting by pad
			}
			

			if (b!=1 && cap > 0)
				cap = 0;

			if (ch>=32 && ch<127 || ch==8 || ch=='\n')
				Buzz();

			if (cap>0)
			{
				// ensure one contact per keycap
				for (int i=0; i<4; i++)
				{
					if (i==id)
						continue;
					if (input.contact[i].action == Input::Contact::KEYBCAP && input.contact[i].keyb_cap == cap)
						cap = 0;
				}
			}

			if (cap > 0)
			{
				if (cap == A3D_LSHIFT)
				{
					if (id==0)
						keyb_key[cap >> 3] ^= 1 << (cap & 7);  // toggle shift
					else
						keyb_key[cap >> 3] |= 1 << (cap & 7);
				}
				else
				{
					if (ch)
					{
						if (ch == '\n' && !( keyb_key[A3D_LSHIFT >> 3] & (1 << (A3D_LSHIFT & 7)) ) )
						{
							if (player.talk_box->len > 0)
							{
								if (player.talks == 3)
								{
									free(player.talk[0].box);
									player.talks--;
									for (int i = 0; i < player.talks; i++)
										player.talk[i] = player.talk[i + 1];
								}

								int idx = player.talks;
								player.talk[idx].box = player.talk_box;
								player.talk[idx].pos[0] = player.pos[0];
								player.talk[idx].pos[1] = player.pos[1];
								player.talk[idx].pos[2] = player.pos[2];
								player.talk[idx].stamp = stamp;

								if (server)
								{
									STRUCT_REQ_TALK req_talk = { 0 };
									req_talk.token = 'T';
									req_talk.len = player.talk[idx].box->len;
									memcpy(req_talk.str, player.talk[idx].box->buf, player.talk[idx].box->len);
									server->Send((const uint8_t*)&req_talk, 4 + req_talk.len);
								}

								ChatLog("%s : %.*s\n", player.name, player.talk[player.talks].box->len, player.talk[player.talks].box->buf);
								player.talks++;

								// alloc new
								player.talk_box = 0;

								TalkBox_blink = 32;
								player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
								memset(player.talk_box, 0, sizeof(TalkBox));
								player.talk_box->max_width = 33;
								player.talk_box->max_height = 7; // 0: off
								int s[2], p[2];
								player.talk_box->Reflow(s, p);
								player.talk_box->size[0] = s[0];
								player.talk_box->size[1] = s[1];
								player.talk_box->cursor_xy[0] = p[0];
								player.talk_box->cursor_xy[1] = p[1];
							}

							ch = 0;
						}
						else
							OnKeyb(GAME_KEYB::KEYB_CHAR, ch); // like from terminal!
					}
					keyb_key[cap >> 3] |= 1 << (cap & 7);  // just to hilight keycap
				}
				con->keyb_cap = cap;

				// setup autorepeat initial delay...
				// not for shift
				KeybAutoRepCap = cap;
				KeybAuroRepDelayStamp = stamp;
				KeybAutoRepChar = ch; // must be nulled on any real keyb input!

				con->action = Input::Contact::KEYBCAP;
			}

			if (cap == 0)
				con->action = Input::Contact::NONE;
		}

		if (cap<0)
		{
			// ensure not on inventory
			Sprite::Frame* sf = inventory_sprite->atlas;

			int width = render_size[0];
			int height = render_size[1];

			// ensure not on inventory
			if (cp[0] < scene_shift && 
				cp[1]>= inventory.layout_y && 
				cp[1]< inventory.layout_y+ inventory.layout_height)
			{
				con->action = Input::Contact::NONE;	
			}
			else
			if (id==0 && b==2)
			{
				// absolute mouse torque (mrg=0)
				con->action = Input::Contact::TORQUE;
				yaw = prev_yaw;

				// ensure no timer torque is pending
				for (int i=1; i<4; i++)
				{
					if (input.contact[i].action == Input::Contact::TORQUE)
					{
						con->action = Input::Contact::NONE;
						yaw = 0;
						break;
					}
				}
			}
			else
			{
				if (PlayerHit(this, x, y))
					con->action = Input::Contact::PLAYER;
				else
				// ADDED SCENE_SHIFT FOR LEFT-TORQUE TOUCH!  
				if (id>0 && cp[0] < 5+scene_shift && cp[0] > scene_shift &&
					input.contact[0].action != Input::Contact::TORQUE)
				{
					mrg = -1;
					con->action = Input::Contact::TORQUE;
				}
				else
				if (id>0 && cp[0] >= render_size[0]-5 && 
					input.contact[0].action != Input::Contact::TORQUE)
				{
					mrg = +1;
					con->action = Input::Contact::TORQUE;
				}
				else
				if (b==1)
				{
					con->action = Input::Contact::FORCE;

					// ensure no other contact is in force mode
					for (int i=0; i<4; i++)
					{
						if (i==id)
							continue;

						if (input.contact[i].action == Input::Contact::FORCE)
						{
							if (id != 0 && i != 0) // both are touches
							{
								int cp2[2] = { input.contact[i].drag_from[0], input.contact[i].drag_from[1] };
								ScreenToCell(cp2);
								
								if (2 * cp2[0] - scene_shift < render_size[0])
								{
									if (player.req.weapon > 0)
									{
										if (player.req.weapon == WEAPON::REGULAR_CROSSBOW)
										{
											input.shoot = true;
											input.shoot_xy[0] = cp[0];
											input.shoot_xy[1] = cp[1];

											// here we should hittest to see what's there
											// enemy mesh, enemy sprite ?
										}
										else
											player.SetActionAttack(stamp);
									}
									else
										input.jump = true;
								}
								else
								{
									input.jump = true;
								}
							}
							else
							{
								/*
								// if we have a weapon then attack
								if (player.req.weapon>0)
									player.SetActionAttack(stamp);
								else
									input.jump = true;
								*/
								input.jump = true;
							}
							con->action = Input::Contact::NONE;
							break;
						}
					}
				}
				else
				{
					con->action = Input::Contact::NONE;				
				}
			}
		}
	}

	con->drag = b;

	con->pos[0] = x;
	con->pos[1] = y;
	con->drag_from[0] = x;
	con->drag_from[1] = y;

	con->item = item;
	con->keyb_cap = cap;
	con->margin = mrg;
	con->player_hit = hit;
	con->start_yaw = yaw;
}

void Game::MoveContact(int id, int x, int y)
{
	Input::Contact* con = input.contact + id;	
	con->pos[0] = x;
	con->pos[1] = y;

	switch (con->action)
	{
		case Input::Contact::ITEM_GRID_CLICK:
		{
			// if moved my 2 or more cells, change it into DRAG
			int down[2] = { con->drag_from[0], con->drag_from[1] };
			ScreenToCell(down);

			int up[2] = { x, y };
			ScreenToCell(up);

			int rel[2] = { up[0] - down[0], up[1] - down[1] };
			if (rel[0] * rel[0] + rel[1] * rel[1] >= 4)
			{
				// turn into moving/splitting/removing item
				con->action = Input::Contact::ITEM_GRID_DRAG;
			}
			break;
		}

		case Input::Contact::ITEM_GRID_SCROLL:
		{
			int down[2] = { con->drag_from[0], con->drag_from[1] };
			ScreenToCell(down);

			int up[2] = { x, y };
			ScreenToCell(up);

			Sprite::Frame* sf = inventory_sprite->atlas;

			int scroll = con->scroll + up[1] - down[1];
			if (scroll < 0)
				scroll = 0;
			if (scroll > inventory.layout_max_scroll)
				scroll = inventory.layout_max_scroll;

			inventory.scroll = scroll;
			inventory.smooth_scroll = inventory.scroll;


			break;
		}
		case Input::Contact::ITEM_LIST_CLICK:
		{
			int cp[2] = { x, y };
			ScreenToCell(cp);

			// locate in list
			for (int i = 0; i < items_count; i++)
			{
				if (con->item == items_inrange[i])
				{
					if (cp[1] >= items_ylo && cp[1] <= items_yhi &&
						cp[0] > items_xarr[i] && cp[0] < items_xarr[i + 1])
					{
						// still inside item rect, nutting to do
						return;
					}

					// dragged out of the list
					con->action = Input::Contact::ITEM_LIST_DRAG;

					if (!show_inventory)
						show_inventory = true;

					return;
				}
			}

			// not in the list anymore
			con->action = Input::Contact::NONE;
			break;
		}

		case Input::Contact::PLAYER:
		{
			int down[2] = { con->drag_from[0], con->drag_from[1] };
			ScreenToCell(down);

			int up[2] = { x, y };
			ScreenToCell(up);

			up[0] -= down[0];
			up[1] -= down[1];

			if (up[0] * up[0] > 1 || up[1] * up[1] > 1 || !PlayerHit(this, x, y))
			{
				con->action = Input::Contact::FORCE;
				for (int i=0; i<4; i++)
				{
					if (i==id)
						continue;
					if (input.contact[i].action == Input::Contact::FORCE)
					{
						con->action = Input::Contact::NONE;
						break;
					}
				}
			}
			break;
		}

		case Input::Contact::KEYBCAP:
		{			
			int cp[2] = { x,y };
			ScreenToCell(cp);
			#if 0 
			int cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, 0,false);
			#endif
			int cap = keyb.GetCap(cp[0], cp[1], render_size[0], render_size[1], 0, false);

			if (cap != con->keyb_cap)
			{
				con->action = Input::Contact::NONE;
				
				int uncap = con->keyb_cap;
				if (uncap != A3D_LSHIFT || id!=0)
					keyb_key[uncap >> 3] &= ~(1 << (uncap & 7));  // un-hilight keycap

				if (uncap == KeybAutoRepCap)
				{
					KeybAutoRepCap = 0;
					KeybAutoRepChar = 0;
				}
			}
			break;
		}
	}
}

void Game::EndContact(int id, int x, int y)
{
	Input::Contact* con = input.contact + id;

	// any contact end must cancel autorep
	con->pos[0] = x;
	con->pos[1] = y;

	switch (con->action)
	{
		case Input::Contact::TORQUE:
		{
			SetPhysicsYaw(physics, prev_yaw, yaw_vel);
			break;
		}

		case Input::Contact::ITEM_GRID_CLICK:
		{
			// eat/use/unuse
			// (only with right click)
			if (con->drag==2)
				ExecuteItem(con->my_item);

			break;
		}

		case Input::Contact::ITEM_GRID_DRAG:
		{
			if (con->drag == 2 )
			{
				// SPLIT
			}

			int drop_at[2];
			if (CheckDrop(id, drop_at, 0, render_size[0], render_size[1]))
			{
				if (drop_at[0] < 0 || drop_at[1] < 0)
				{
					// REMOVE! (if not in use)
					if (!inventory.my_item[con->my_item].in_use)
						DropItem(con->my_item);
					break;
				}

				// clear bitmask first (NOT FOR SPLIT)
				int* xy = inventory.my_item[con->my_item].xy;
				Item* item = con->item;
				int x0 = xy[0];
				int x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
				int y0 = xy[1];
				int y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;
				for (int y = y0; y < y1; y++)
				{
					for (int x = x0; x < x1; x++)
					{
						int i = x + y * inventory.width;
						inventory.bitmask[i >> 3] &= ~(1 << (i & 7));
					}
				}

				inventory.my_item[con->my_item].xy[0] = drop_at[0];
				inventory.my_item[con->my_item].xy[1] = drop_at[1];

				// SET BITMASK
				x0 = xy[0];
				x1 = x0 + (item->proto->sprite_2d->atlas->width + 1) / 4;
				y0 = xy[1];
				y1 = y0 + (item->proto->sprite_2d->atlas->height + 1) / 4;
				for (int y = y0; y < y1; y++)
				{
					for (int x = x0; x < x1; x++)
					{
						int i = x + y * inventory.width;
						inventory.bitmask[i >> 3] |= 1 << (i & 7);
					}
				}
			}

			break;
		}

		case Input::Contact::ITEM_LIST_CLICK:
		case Input::Contact::ITEM_LIST_DRAG:
		{
			int cp[2] = { x, y };
			ScreenToCell(cp);

			// locate in list
			for (int i = 0; i < items_count; i++)
			{
				if (con->item == items_inrange[i])
				{
					if (con->action == Input::Contact::ITEM_LIST_CLICK)
					{
						if (cp[1] >= items_ylo && cp[1] <= items_yhi &&
							cp[0] > items_xarr[i] && cp[0] < items_xarr[i + 1])
						{
							// TRY TO PICK
							PickItem(items_inrange[i]);
							break;
						}
					}
					else
					if (show_inventory)
					{
						// TRY TO INSERT INTO INVENTORY at given pos
						int drop_at[2];
						if (CheckDrop(id, drop_at, 0, render_size[0], render_size[1]))
						{
							inventory.InsertItem(items_inrange[i], drop_at);
						}
					}

					break;
				}
			}

			break;
		}

		case Input::Contact::KEYBCAP:
		{
			// maybe we should clear it also when another cap is pressed?
			if (con->keyb_cap!=A3D_LSHIFT || id!=0)
				keyb_key[con->keyb_cap >> 3] &= ~(1 << (con->keyb_cap & 7));  // un-hilight keycap

			if (KeybAutoRepCap == con->keyb_cap)
			{
				KeybAutoRepCap = 0;
				KeybAutoRepChar = 0;
			}
			break;
		}

		case Input::Contact::PLAYER:
		{
			int down[2] = { con->drag_from[0], con->drag_from[1] };
			ScreenToCell(down);

			int up[2] = { x, y };
			ScreenToCell(up);

			up[0] -= down[0];
			up[1] -= down[1];

			if (up[0] * up[0] <= 1 && up[1] * up[1] <= 1)
			{
				if (player.talk_box)
				{
					// start showing main buts
					//show_buts = true;

					// close talk_box (and keyb if also open)
					free(player.talk_box);
					player.talk_box = 0;
					if (show_keyb)
						memset(keyb_key, 0, 32);
					show_keyb = false;
					KeybAutoRepChar = 0;
					KeybAutoRepCap = 0;
					for (int i=0; i<4; i++)
					{
						if (input.contact[i].action == Input::Contact::KEYBCAP)
							input.contact[i].action = Input::Contact::NONE;
					}
				}
				else
				//if (show_buts)
				{
					CancelItemContacts();
					//show_buts = false;
					// open talk_box (and keyb if not open)
					TalkBox_blink = 32;
					player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
					memset(player.talk_box, 0, sizeof(TalkBox));
					player.talk_box->max_width = 33;
					player.talk_box->max_height = 7; // 0: off
					int s[2],p[2];
					player.talk_box->Reflow(s,p);
					player.talk_box->size[0] = s[0];
					player.talk_box->size[1] = s[1];
					player.talk_box->cursor_xy[0] = p[0];
					player.talk_box->cursor_xy[1] = p[1];
				
					show_keyb = true;
				}
			}

			break;
		}
	}

	con->action = Input::Contact::NONE;
	con->drag = 0;
}

int Game::GetContact(int id)
{
	Input::Contact* con = input.contact + id;
	return con->drag;
}


// note:
//   only left-button is moved with mouse, 
//   other emu-touches remains on their initial pos

// #define TOUCH_EMU


#ifdef TOUCH_EMU
int FirstFree(int size, int* arr)
{
	for (int id=1; id<=size; id++)
	{
		int i = 0;
		for (; i<size; i++)
			if (arr[i] == id)
				break;
		if (i==size)
			return id;
	}

	return -1;
}
#endif

void Game::OnMouse(GAME_MOUSE mouse, int x, int y)
{
	/*
	if (mouse==MOUSE_LEFT_BUT_DOWN)
	{
		int freq = 100 + fast_rand()%100;
		CallAudio((uint8_t*)&freq,sizeof(freq));
	}
	*/

	// handle layers first ...
	if (menu_depth>=0)
	{
		MenuMouse(mouse,x,y);
		return;
	}

	if (show_gamepad)
	{
		int ev = -1;
		switch (mouse)
		{
			case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: ev = 0; break;
			case GAME_MOUSE::MOUSE_MOVE: ev = 1; break;
			case GAME_MOUSE::MOUSE_LEFT_BUT_UP: ev = 2; break;

			default:
				break;
		}

		if (ev>=0)
		{
			int p[2] = {x,y};
			ScreenToCell(p);
			GamePadContact(0,ev,p[0],p[1], stamp);
		}

		return;
	}

	#ifdef TOUCH_EMU
	static int buts_id[3] = {-1,-1,-1}; // L,R,M
	// emulate touches for easier testing
	switch (mouse)
	{
		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 
		case GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN: 
		case GAME_MOUSE::MOUSE_MIDDLE_BUT_DOWN: 
		{
			int idx = ((int)mouse-1) >> 1;
			assert(buts_id[idx]<0);
			buts_id[idx] = FirstFree(3,buts_id);
			OnTouch(GAME_TOUCH::TOUCH_BEGIN,buts_id[idx],x,y);

			for (int i = 0; i < 1; i++)
				if (i != idx && buts_id[i]>0)
					OnTouch(GAME_TOUCH::TOUCH_MOVE,buts_id[i],x,y);
			
			break;
		}

		case GAME_MOUSE::MOUSE_LEFT_BUT_UP: 
		case GAME_MOUSE::MOUSE_RIGHT_BUT_UP: 
		case GAME_MOUSE::MOUSE_MIDDLE_BUT_UP: 
		{
			int idx = ((int)mouse-2) >> 1;
			if (buts_id[idx]>0)
			{
				OnTouch(GAME_TOUCH::TOUCH_END,buts_id[idx],x,y);
				buts_id[idx] = -1;

				for (int i = 0; i < 1; i++)
					if (i != idx && buts_id[i]>0)
						OnTouch(GAME_TOUCH::TOUCH_MOVE,buts_id[i],x,y);
			}

			break;
		}

		case GAME_MOUSE::MOUSE_MOVE: 
			for (int i = 0; i < 1; i++)
				if (buts_id[i]>0)
					OnTouch(GAME_TOUCH::TOUCH_MOVE,buts_id[i],x,y);
			break;
	}
	return;
	#endif

	switch (mouse)
	{
		// they are handled
		// after switch !!!
		case MOUSE_WHEEL_DOWN:
			if (scene_shift)
			{
				int cp[2] = { x,y };
				ScreenToCell(cp);
				// if mouse on x-visible part of inventory
				if (cp[0]<scene_shift)
				{
					Sprite::Frame* sf = inventory_sprite->atlas;

					if (cp[1]>= inventory.layout_y && 
						cp[1]< inventory.layout_y+ inventory.layout_height)
					{
						inventory.animate_scroll = false;
						inventory.smooth_scroll += 5;
					}
				}
			}
			break;
		case MOUSE_WHEEL_UP:
			if (scene_shift)
			{
				int cp[2] = { x,y };
				ScreenToCell(cp);
				// if mouse on x-visible part of inventory
				if (cp[0]<scene_shift)
				{
					Sprite::Frame* sf = inventory_sprite->atlas;

					if (cp[1]>= inventory.layout_y && 
						cp[1]< inventory.layout_y+ inventory.layout_height)
					{
						inventory.animate_scroll = false;
						inventory.smooth_scroll -= 5;
					}
				}
			}
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 

			if (input.but != 0)
			{
				if (input.contact[0].action == Input::Contact::TORQUE)
				{

					if (player.req.weapon == WEAPON::REGULAR_CROSSBOW && !input.shoot)
					{
						input.shoot = true;
						//input.shoot_xy[0] = cp[0];
						//input.shoot_xy[1] = cp[1];

						// here we should hittest to see what's there
						// enemy mesh, enemy sprite ?
					}
					else
						player.SetActionAttack(stamp);

					/*
					int cp[2] = { x,y };
					ScreenToCell(cp);
					input.shoot = true;
					input.shoot_xy[0] = cp[0];
					input.shoot_xy[1] = cp[1];
					// input.contact[0].action = Input::Contact::NONE;
					*/
				}

				input.but |= 0x1;
				break;
			}

			if (input.but == 0x0)
				StartContact(0, x,y, 1);
			else
				MoveContact(0, x,y);
			input.but |= 0x1;
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_UP: 
			if (GetContact(0) == 1)
				EndContact(0, x,y);
			input.but &= ~0x1;
			break;

		case GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN: 
			if (input.but == 0)
				StartContact(0, x,y, 2);
			else
			{
				if (input.contact[0].action == Input::Contact::FORCE)
				{
					/*
					// if we have a weapon then attack
					if (player.req.weapon>0)
						player.SetActionAttack(stamp);
					else
						input.jump = true;
					*/
					input.jump = true;
				}
				MoveContact(0, x,y);
			}
			input.but |= 0x2;
			break;

		case GAME_MOUSE::MOUSE_RIGHT_BUT_UP: 
			if (GetContact(0) == 2)
				EndContact(0, x,y);
			input.but &= ~0x2;
			break;

		case GAME_MOUSE::MOUSE_MIDDLE_BUT_DOWN: 
			if (input.but == 0)
				StartContact(0, x,y, 3);
			else
				MoveContact(0, x,y);
			input.but |= 0x4;
			break;

		case GAME_MOUSE::MOUSE_MIDDLE_BUT_UP: 
			if (GetContact(0) == 3)
				EndContact(0, x,y);
			input.but &= ~0x4;
			break;

		case GAME_MOUSE::MOUSE_MOVE: 
			if (GetContact(0))
				MoveContact(0, x,y);
			break;
	}
}

void Game::OnTouch(GAME_TOUCH touch, int id, int x, int y)
{
	if (id<1 || id>3)
		return;
	/*
	if (touch==TOUCH_BEGIN)
	{
		int freq = 100 + fast_rand()%100;
		CallAudio((uint8_t*)&freq,sizeof(freq));
	}
	*/


	// handle layers first ...
	if (menu_depth>=0)
	{
		MenuTouch(touch,id,x,y);
		return;
	}


	if (show_gamepad)
	{
		int ev = -1;
		switch (touch)
		{
			case GAME_TOUCH::TOUCH_BEGIN: ev = 0; break;
			case GAME_TOUCH::TOUCH_MOVE: ev = 1; break;
			case GAME_TOUCH::TOUCH_END: ev = 2; break;
			case GAME_TOUCH::TOUCH_CANCEL: ev = 3; break;
		}

		if (ev>=0)
		{
			int p[2] = {x,y};
			ScreenToCell(p);
			GamePadContact(id,ev,p[0],p[1], stamp);
		}

		return;
	}

	switch (touch)
	{
		case TOUCH_BEGIN:
			StartContact(id, x,y, 1);
			break;

			break;
		case TOUCH_MOVE:
			MoveContact(id, x,y);
			break;

		case TOUCH_END:
			EndContact(id, x,y);
			break;

		case TOUCH_CANCEL:
			if (input.contact[id].action == Input::Contact::KEYBCAP)
			{
				// this should be always true
				if (input.contact[id].keyb_cap!=A3D_LSHIFT || id!=0)
					keyb_key[input.contact[id].keyb_cap >> 3] &= ~(1 << (input.contact[id].keyb_cap & 7));  // un-hilight keycap
			 
				if (input.contact[id].keyb_cap == KeybAutoRepCap)
				{
					KeybAutoRepCap = 0;
					KeybAutoRepChar = 0;
				}
			}
			input.contact[id].drag = 0;
			input.contact[id].action = Input::Contact::NONE;
			break;
	}
}

void Game::OnFocus(bool set)
{
	// if loosing focus, clear all tracking / dragging / keyboard states
	if (!set)
	{
		KeybAutoRepCap = 0;
		KeybAutoRepChar = 0;
		for (int i=0; i<4; i++)
		{
			input.contact[i].action = Input::Contact::NONE;
			input.contact[i].drag = 0;
		}

		int w = input.size[0], h = input.size[1];
		bool pad = input.pad_connected;
		memset(&input, 0, sizeof(Input));
		input.pad_connected = pad;
		input.size[0] = w;
		input.size[1] = h;
	}
}

void Game::OnMessage(const uint8_t* msg, int len)
{
	// NET_TODO:
	// this is called by JS or game_app (already on game's thread)
}

void Game::OnPadMount(bool connect)
{
	input.pad_connected = connect;
	input.pad_button = 0;
	input.pad_autorep = 0;
	input.pad_item = 0;
	input.pad_stamp = stamp;
	memset(input.pad_axis, 0, sizeof(int16_t) * 32);
	if (connect)
		OnPadAxis(-1, 0);

	if (menu_depth>=0)
	{
		MenuPadMount(connect);
	}
}

void Game::OnPadButton(int b, bool down)
{
	if (show_gamepad)
	{
		return;
	}		

	if (input.pad_autorep == 2+1 && !player.talk_box)
	{
		// clear and or ignore delete autorep if not in chat_box
		input.pad_autorep = 0;
		if (b==2 && !down)
			return;
	}

	if (b >= 0 && b < 32)
	{
		if (down)
		{
			if (b>=11 && b <=14 || b==2 && player.talk_box) // autorep dirpad and delete char in talk_box
			{
				if (input.pad_autorep != b+1)
				{
					input.pad_autorep = b+1; // +1 so mem-setting to 0 is fine
					input.pad_stamp = stamp;
				}
			}
			input.pad_button |= 1 << b;
		}
		else
		{
			input.pad_autorep = 0;
			input.pad_button &= ~(1 << b);
		}

		if (menu_depth>=0)
		{
			// handle menu navi
			MenuPadButton(b,down);
			return;
		}

		if (down)
		{
			if (!player.talk_box)
			{
				switch (b)
				{
					case 0:
					{
						input.shoot = true;
						input.shoot_xy[0] = -1;
						input.shoot_xy[1] = -1;
						player.SetActionAttack(stamp);
						break;
					}

					case 1: 
					{
						input.jump = true;
						break;
					}

					case 2:
					{
						// item_grid (inventory ops)
						if (show_inventory)
						{
							Input::Contact* con = input.contact+3;
							if (con->action == Input::Contact::NONE && inventory.focus>=0)
							{
								// cancel all contacts
								// ... or prevent entering 'y' state if already there is item contact
								CancelItemContacts();

								// get xy from focused item
								int* xy = inventory.my_item[inventory.focus].xy;

								// keyb_y = 0
								// gamepad_x = 1
								// gamepad_y = 2

								int but = 0; // keyb_y
								// StartContact(3/*KEYB/PAD*/, xy[0],xy[1], but);

								// synthetize contact
								
								// note: until dirpad we're in CLICK state (not DRAG)
								con->action = Input::Contact::ITEM_GRID_CLICK;

								con->item = inventory.my_item[inventory.focus].item;


								con->my_item = inventory.focus; // ?

								// calc synthetized scrren position for contact

								int ix = inventory.layout_x + xy[0]*4 + 4;
								int iy = inventory.layout_y + xy[1]*4 + 
										inventory.layout_height - 6 - (inventory.height*4-1) + 
										inventory.scroll;

								// centered on item
								ix += con->item->proto->sprite_2d->atlas->width/2;
								iy += con->item->proto->sprite_2d->atlas->height/2;	

								// note: until dirpad we're in CLICK state (not DRAG)
								// shift slightly UP (so user can see we're in moving state)
								// iy++;

								// flip y axis (sceen coords are top to bottom)
								iy = render_size[1] - 1 - iy;	

								con->drag = 0; // button
								con->pos[0] = ix*font_size[0] + font_size[0]/2;
								con->pos[1] = iy*font_size[1] + font_size[1]/2;
								con->drag_from[0] = con->pos[0];
								con->drag_from[1] = con->pos[1];

								con->keyb_cap = -1;
								con->margin = 0;
								con->player_hit = false;
								con->start_yaw = 0;
							}
						}
						break;
					}

					case 3:
					{
						// item_list (pickup popup)
						Input::Contact* con = input.contact+3;
						if (con->action == Input::Contact::NONE && !player.talk_box && items_count)
						{
							// hilight first
							input.pad_item = 0+1;
						}
						break;
					}

					case 5:
					{
						if (show_inventory)
						{
							// show gampad help for inventory operations
							// and item pick up
						}
						else
						{
							// show gamepad help for run, jump, attack
							// camera rot, open inventory, open chat
							// and item pick up
						}
						// lock processing any input until any key is pressed
						// then close this vidget
						break;
					}

					case 6:
					{
						// mini-menu
						//perspective = !perspective;
						//show_buts = !show_buts; // just test
						ToggleMenu(3);
						break;
					}

					case 9:
					{
						CancelItemContacts();
						show_inventory = !show_inventory;
						break;
					}

					case 10:
					{
						//if (show_buts)
						{
							CancelItemContacts();
							//show_buts = false;
							// open talk_box (and keyb if not open)
							TalkBox_blink = 32;
							player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
							memset(player.talk_box, 0, sizeof(TalkBox));
							player.talk_box->max_width = 33;
							player.talk_box->max_height = 7; // 0: off
							int s[2],p[2];
							player.talk_box->Reflow(s,p);
							player.talk_box->size[0] = s[0];
							player.talk_box->size[1] = s[1];
							player.talk_box->cursor_xy[0] = p[0];
							player.talk_box->cursor_xy[1] = p[1];
				
							show_keyb = true;
						}

						break;
					}

					case 11:
					{
						if (show_inventory)
						{
							if (input.contact[3].action == Input::Contact::NONE)
								inventory.FocusNext(0, 1);
							else
							{
								input.contact[3].pos[1]-=font_size[1];
								if (input.contact[3].action == Input::Contact::ITEM_GRID_CLICK)
									input.contact[3].action = Input::Contact::ITEM_GRID_DRAG;
							}
						}
						break;
					}
					case 12:
					{
						if (show_inventory)
						{
							if (input.contact[3].action == Input::Contact::NONE)
								inventory.FocusNext(0, -1);
							else
							{
								input.contact[3].pos[1]+=font_size[1];
								if (input.contact[3].action == Input::Contact::ITEM_GRID_CLICK)
									input.contact[3].action = Input::Contact::ITEM_GRID_DRAG;
							}
						}

						break;
					}
					case 13:
					{
						if (input.pad_item>1)
						{
							input.pad_item--;
						}
						else
						if (show_inventory)
						{
							if (input.contact[3].action == Input::Contact::NONE)
								inventory.FocusNext(-1, 0);
							else
							{
								input.contact[3].pos[0]-=font_size[0];
								if (input.contact[3].action == Input::Contact::ITEM_GRID_CLICK)
									input.contact[3].action = Input::Contact::ITEM_GRID_DRAG;
							}
						}
						break;
					}
					case 14:
					{
						if (input.pad_item && input.pad_item<items_count)
						{
							input.pad_item++;
						}
						else
						if (show_inventory)
						{
							if (input.contact[3].action == Input::Contact::NONE)
								inventory.FocusNext(1, 0);
							else
							{
								input.contact[3].pos[0]+=font_size[0];
								if (input.contact[3].action == Input::Contact::ITEM_GRID_CLICK)
									input.contact[3].action = Input::Contact::ITEM_GRID_DRAG;
							}
						}
						break;
					}
				}
			}
			else
			{
				switch (b)
				{
					case 5: // guide / logo
					{
						// show gampad help for typing
						// lock processing any input until any key is pressed
						// then close this vidget
						break;
					}

					case 6: // start
					{
						// mini-menu
						//perspective = !perspective;
						//show_buts = !show_buts; // just test
						ToggleMenu(3);
						break;
					}

					case 9:
					{
						show_inventory = !show_inventory;
						break;
					}

					case 10:
					{
						// start showing main buts
						//show_buts = true;

						// close talk_box (and keyb if also open)
						free(player.talk_box);
						player.talk_box = 0;
						if (show_keyb)
							memset(keyb_key, 0, 32);
						show_keyb = false;
						KeybAutoRepChar = 0;
						KeybAutoRepCap = 0;
						for (int i = 0; i < 4; i++)
						{
							if (input.contact[i].action == Input::Contact::KEYBCAP)
								input.contact[i].action = Input::Contact::NONE;
						}
						break;
					}

					case 11:
						player.talk_box->MoveCursorY(-1);
						break;
					case 12:
						player.talk_box->MoveCursorY(+1);
						break;
					case 13:
						player.talk_box->MoveCursorX(-1);
						break;
					case 14:
						player.talk_box->MoveCursorX(+1);
						break;

					case 2: // backspace
						player.talk_box->Input(8);
						break;
					case 3: // SEND
					{
						Buzz();
						if (player.talk_box->len > 0)
						{
							if (player.talks == 3)
							{
								free(player.talk[0].box);
								player.talks--;
								for (int i = 0; i < player.talks; i++)
									player.talk[i] = player.talk[i + 1];
							}

							int idx = player.talks;
							player.talk[idx].box = player.talk_box;
							player.talk[idx].pos[0] = player.pos[0];
							player.talk[idx].pos[1] = player.pos[1];
							player.talk[idx].pos[2] = player.pos[2];
							player.talk[idx].stamp = stamp;

							if (server)
							{
								STRUCT_REQ_TALK req_talk = { 0 };
								req_talk.token = 'T';
								req_talk.len = player.talk[idx].box->len;
								memcpy(req_talk.str, player.talk[idx].box->buf, player.talk[idx].box->len);
								server->Send((const uint8_t*)&req_talk, 4 + req_talk.len);
							}

							ChatLog("%s : %.*s\n", player.name, player.talk[player.talks].box->len, player.talk[player.talks].box->buf);
							player.talks++;

							// alloc new
							player.talk_box = 0;

							TalkBox_blink = 32;
							player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
							memset(player.talk_box, 0, sizeof(TalkBox));
							player.talk_box->max_width = 33;
							player.talk_box->max_height = 7; // 0: off
							int s[2], p[2];
							player.talk_box->Reflow(s, p);
							player.talk_box->size[0] = s[0];
							player.talk_box->size[1] = s[1];
							player.talk_box->cursor_xy[0] = p[0];
							player.talk_box->cursor_xy[1] = p[1];
						}
						break;
					}	
				}
			}
		}
		else
		{
			// up!
			// todo: check if this button release should generate keyb char!
			// ...

			if (b==2)
			{
				if (input.contact[3].action!=Input::Contact::NONE)
				{
					if (input.contact[3].action==Input::Contact::ITEM_GRID_CLICK)
						ExecuteItem(inventory.focus);
					EndContact(3,input.contact[3].pos[0],input.contact[3].pos[1]);
				}
			}

			if (b==3)
			{
				if (input.pad_item>0 && input.pad_item<=items_count)
					PickItem(items_inrange[input.pad_item-1]);

				// reset item pickup hilight 
				input.pad_item = 0;
			}

			if (show_keyb)
			{
				if (b == 0 || b == 1)
				{
					bool shift_on = b == 1;
					char ch = 0;
					int key = keyb.GetPadCap(&ch,shift_on);
					if (ch)
					{
						Buzz();
						TalkBox_blink = 0;
						if (player.talk_box)
							player.talk_box->Input(ch);
					}
				}
			}
		}
	}

	// just update states
	OnPadAxis(-1, 0);
}

void Game::OnPadAxis(int a, int16_t pos)
{
	if (show_gamepad)
	{
		return;
	}		

	if (a>=0 && a<32)
		input.pad_axis[a] = pos;

	if (menu_depth>=0)
	{
		// handle menu navi
		MenuPadAxis(a,pos);
		return;
	}

	//if (show_keyb)
	{
		if (show_keyb && (input.pad_button & 3))
		{
			// locked plane and sect, change dir
			if (a == 0 || a == 1 || a == -1)
			{
				int dir = 11, ang;
				if (input.pad_axis[0] >= -20000 && input.pad_axis[0] <= +20000 &&
					input.pad_axis[1] >= -20000 && input.pad_axis[1] <= +20000)
				{
					dir = 0;
				}
				else
				{
					if (input.pad_axis[0] < 0) // left
					{
						ang = (int)(atan2(-input.pad_axis[1], -input.pad_axis[0]) / M_PI * 180);
						if (ang < -54)
							dir = 6;
						else
						if (ang < -18)
							dir = 7;
						else
						if (ang < +18)
							dir = 8;
						else
						if (ang < +54)
							dir = 9;
						else
							dir = 10;
					}
					else // right
					{
						ang = (int)(atan2(input.pad_axis[1], input.pad_axis[0]) / M_PI * 180);
						if (ang < -54)
							dir = 1;
						else
						if (ang < -18)
							dir = 2;
						else
						if (ang < +18)
							dir = 3;
						else
						if (ang < +54)
							dir = 4;
						else
							dir = 5;
					}
				}

				keyb.dir = dir;
				//printf("ang=%d, dir=%d\n", ang,dir);
			}
		}
		else
		{
			// change sect, undefine dir
			if (a == 0 || a == -1)
			{
				keyb.dir = 11;
				if (input.pad_axis[0] < -10000)
					keyb.sect = 0;
				else
				if (input.pad_axis[0] > +10000)
					keyb.sect = 2;
				else
					keyb.sect = 1;
			}
			else
			if (a == 1 || a == -1)
			{
				keyb.dir = 11;
				if (input.pad_axis[1] < -10000)
				{
					keyb.plane = 1;
					keyb.pad_plane = true; // indicate we can reset plane by stick
				}
				else
				if (input.pad_axis[1] > +10000)
				{
					keyb.plane = 2;
					keyb.pad_plane = true; // indicate we can reset plane by stick
				}
				else
				{
					// prevent resetting plane to 0
					// if user set it with mouse or touch
					// to something else
					if (keyb.pad_plane)
						keyb.plane = 0;
				}
			}
		}
	}
}

struct MatIDStamp
{
	static void SetMatCB(Patch* p, int x, int y, int view_flags, void* cookie)
	{
		MatIDStamp* t = (MatIDStamp*)cookie;

		double r2 = t->r * t->r;
		double* hit = t->hit;
		int matid = t->matid;

		uint16_t* visual = GetTerrainVisualMap(p);

		bool diff = false;
		diff = true;

		for (int v = 0, i = 0; v < VISUAL_CELLS; v++)
		{
			for (int u = 0; u < VISUAL_CELLS; u++, i++)
			{
				double dx = u + x - hit[0];
				double dy = v + y - hit[1];
				if (dx*dx + dy * dy < r2)
				{
					int old = visual[i] & 0xFF;
					if (old != matid)
					{
						diff = true;
						visual[i] = (visual[i] & ~0x00FF) | matid;
					}
				}
			}
		}

		if (diff)
			UpdateTerrainVisualMap(p);
	}

	int matid;
	double hit[2];
	double r;
};

void PaintTerrain(float* xy, float r, int matid)
{
#ifndef EDITOR
	MatIDStamp stamp;
	stamp.hit[0] = xy[0];
	stamp.hit[1] = xy[1];
	stamp.r = r;
	stamp.matid = matid;

	QueryTerrain(terrain, xy[0], xy[1], r * 0.501, 0x00, MatIDStamp::SetMatCB, &stamp);
#endif
}

void BloodLeak(Character* c, int steps)
{
	c->leak_steps += steps;

	if (!c->leak)
		c->leak_steps = 0;
	else
	if (c->leak_steps >= 5)
	{
		c->leak_steps -= c->leak_steps / 5 * 5;

		float dR = 1.0;
		float dr = dR * sqrtf((fast_rand() & 0xfff) / (float)0xfff);
		float dt = (fast_rand() & 0xfff) * (float)(2.0 * M_PI) / (float)0xfff;
		float xy[2] = { c->pos[0] + dr * cosf(dt), c->pos[1] + dr * sinf(dt) };
		PaintTerrain(xy, fast_rand() % 20 * 0.1f, 5/*blood*/);

		c->leak--;
	}
}

// they are global (not related to game / player or anything)
// but if something calls them, we can be certainly sure we have exactly 1 game object
// so let's use prime_game blindly

void GamePadMount(const char* name, int axes, int buttons, const uint8_t mapping[])
{
	ConnectGamePad(name, axes, buttons, mapping);

	uint8_t map[256];
	if (ReadGamePadConf(map,name,axes,buttons))
		SetGamePadMapping(map);

	// do specialized readconf with {name,axes,buttons} query
	// if found, replace current mapping: GamePadLoad(map);

	if (prime_game)
		prime_game->OnPadMount(true);
}

void GamePadUnmount()
{
	DisconnectGamePad();

	if (prime_game)
		prime_game->OnPadMount(false);
}

void GamePadButton(int b, int16_t pos)
{
	uint32_t out[1];
	int outs = UpdateGamePadButton(b, pos < 0 ? 0 : pos, out);

	if (prime_game)
	{
		for (int o=0; o<outs; o++)
		{
			uint32_t map = out[o];
			switch ((map >> 16) & 0xFF)
			{
				case 0:
					prime_game->OnPadAxis(map>>24, (int16_t)(map&0xFFFF));
					break;
				case 1:
					prime_game->OnPadButton(map>>24, (int16_t)(map&0xFFFF) >= 16384);
					break;
			}
		}
	}
}

void GamePadAxis(int a, int16_t pos)
{
	uint32_t out[4];
	int outs = UpdateGamePadAxis(a, pos, out);

	// 0 if unmapped
	// 1 button/axis
	// 2 buttons/axes
	// 2 axes (when mapped on L/R-Joy)
	// 4 buttons (when mapped on D-Pad)

	if (prime_game)
	{
		for (int o=0; o<outs; o++)
		{
			uint32_t map = out[o];
			switch ((map >> 16) & 0xFF)
			{
				case 0:
					prime_game->OnPadAxis(map>>24, (int16_t)(map&0xFFFF));
					break;
				case 1:
					prime_game->OnPadButton(map>>24, (int16_t)(map&0xFFFF) >= 16384);
					break;
			}
		}
	}
}

////////////////////////////////////////////////////////

struct Menu
{
	const char* str; // if 0 this is terminator
	const Menu* sub; // for terminator this is back menu
	void (*action)(Game* g);
	bool (*getter)(Game* g);
};

void menu_perspective(Game* g)
{
	g->perspective = !g->perspective;
	WriteConf(g);
}

bool menu_perspective_getter(Game* g)
{
	return g->perspective;
}

void menu_blood(Game* g)
{
	g->blood = !g->blood;
	WriteConf(g);
}

bool menu_blood_getter(Game* g)
{
	return g->blood;
}

void exit_handler(int signum);
void menu_yes_exit(Game* g)
{
	#ifdef USE_SDL
	exit(0);
	#else
	exit_handler(0);
	#endif
}

void menu_no_exit(Game* g)
{
	g->menu_depth--;
	g->menu_temp = g->menu_stack[g->menu_depth];
}

// TODO:
// - MAKE SIMILAR HACK FOR WEB !
// - ON TTY: system("setfont ./fonts/font-%d.psf"); ?
#ifndef SERVER
bool NextGLFont();
bool PrevGLFont();
void ToggleFullscreen(Game* g);
bool IsFullscreen(Game* g);
#endif

void menu_fullscreen(Game* g)
{
	#ifndef SERVER
	ToggleFullscreen(g);
	#endif
}

bool menu_fullscreen_getter(Game* g)
{
	#ifndef SERVER
	return IsFullscreen(g);
	#endif
	return false;
}

void menu_zoomin(Game* g)
{
	#ifndef SERVER
	NextGLFont();
	#endif
}

void menu_zoomout(Game* g)
{
	#ifndef SERVER
	PrevGLFont();
	#endif
}

void gamepad_close(void* _g)
{
	Game* g = (Game*)_g;
	if (g)
	{
		g->show_gamepad = false;
		g->show_buts = true;

		const uint8_t* map = GetGamePadMapping();
		int axes, buttons;
		const char* name = GetGamePad(&axes, &buttons);
		if (map && name)
			WriteGamePadConf(map,name,axes,buttons);
	}
}

void menu_gamepad(Game* g)
{
	g->CloseMenu();
	g->show_gamepad = true;
	g->show_buts = false;
	GamePadOpen(gamepad_close,(void*)g);
}

static const Menu settings_menu[]=
{
	{"ZOOM IN", 0, menu_zoomin, 0},
	{"ZOOM OUT", 0, menu_zoomout, 0},
	{"FULL SCREEN", 0, menu_fullscreen, menu_fullscreen_getter},
	{"PERSPECTIVE", 0, menu_perspective, menu_perspective_getter},
	{"SHOW BLOOD", 0, menu_blood, menu_blood_getter},
	{0}
};

static const Menu controls_menu[]=
{
	{"KEYBOARD", 0, 0, 0},
	{"MOUSE", 0, 0, 0},
	{"TOUCH", 0, 0, 0},
	{"GAMEPAD", 0, menu_gamepad, 0},
	{0}
};

static const Menu exit_menu[]=
{
	{"NO", 0, menu_no_exit, 0},
	{"YES", 0, menu_yes_exit, 0},
	{0}
};


static const Menu game_menu[]=
{
	{"SETTINGS", settings_menu, 0, 0},
	{"CONTROLS", controls_menu, 0, 0},
	{"EXIT?", exit_menu, 0, 0},
	{0}
};


void Game::OpenMenu(int method)
{
	if (menu_depth>=0)
		return;

	menu_temp = 0;
	menu_down = 0;
	menu_down_x = 0;
	menu_down_y = 0;

	show_gamepad = false;
	// will be cleared by menu
	// show_buts = true; 


	if (player.talk_box)
	{
		free(player.talk_box);
		player.talk_box = 0;
	}

	if (show_keyb)
		memset(keyb_key, 0, 32);
	show_keyb = false;
	KeybAutoRepChar = 0;
	KeybAutoRepCap = 0;
	for (int i=0; i<4; i++)
	{
		if (input.contact[i].action == Input::Contact::KEYBCAP)
			input.contact[i].action = Input::Contact::NONE;
	}

	CancelItemContacts();
	show_inventory = false;	

	show_buts = false;
	menu_depth = 0;

	menu_stack[menu_depth] = method != 1 && method != 2 ? 0 : -1;
}

void Game::CloseMenu()
{
	if (menu_depth<0)
		return;
	show_buts = true;
	menu_depth = -1;

	// clear input
	input.but = 0;
}

void Game::ToggleMenu(int method)
{
	if (menu_depth>=0)
		CloseMenu();
	else
		OpenMenu(method);
}

int Game::HitMenu(int hx, int hy)
{
	if (menu_depth<0)
		return -3;

	int cp[2] = { hx, hy };
	ScreenToCell(cp);
	hx=cp[0];
	hy=cp[1];

	const Menu* m = game_menu;
	const char* title = "MENU";
	for (int d=0; d<menu_depth; d++)
	{
		title = m[ menu_stack[d] ].str;
		m = m[ menu_stack[d] ].sub;
	}

	// right align
	int x = render_size[0]-5;
	int y = render_size[1]-10;

	// title test
	{
		int w = 0, h = 0;
		Font1Size(title,&w,&h);

		if (hx >= 3+x-w /*&& hx<3+x*/ && hy >=y && hy<y+h)
		{
			// title hit
			return -1;
		}

		y -= h+2;
	}

	int i=0;
	while(m[i].str)
	{
		int w = 0, h = 0;
		Font1Size(m[i].str,&w,&h);

		if (hx >= x-w /*&& hx < x*/ && hy>=y && hy<y+h)
		{
			// item hit
			return i;
		}

		y -= h+1;
		i++;
	}

	return -2;
}

void Game::PaintMenu(AnsiCell* ptr, int width, int height)
{
	if (menu_depth<0)
		return;

	const Menu* m = game_menu;
	const char* title = "MENU";
	for (int d=0; d<menu_depth; d++)
	{
		title = m[ menu_stack[d] ].str;
		m = m[ menu_stack[d] ].sub;
	}

	// right align
	int x = width-5;
	int y = height-10;

	// paint title
	{
		int w = 0, h = 0;
		Font1Size(title,&w,&h);
		Font1Paint(ptr,width,height,3+x-w,y,title,FONT1_PINK_SKIN);
		y -= h+2;
	}


	int i=0;
	while(m[i].str)
	{
		int w = 0, h = 0;
		Font1Size(m[i].str,&w,&h);

		int skin = i == menu_stack[menu_depth] ? FONT1_GOLD_SKIN : FONT1_GREY_SKIN;
		Font1Paint(ptr,width,height,x-w,y,m[i].str,skin);

		const char* str = 0;
		if (m[i].sub)
			str = "\x03";
		else
		if (m[i].getter)
			str = m[i].getter(this) ? "\x02" : "\x01";

		if (str)
			Font1Paint(ptr,width,height,x,y,str,FONT1_PINK_SKIN);

		y -= h+1;
		i++;
	}
	
}

void Game::MenuKeyb(GAME_KEYB keyb, int key)
{
	if (menu_down)
		return; // captured by mouse/touch

	if (keyb==KEYB_DOWN && (key==A3D_ENTER || key==A3D_NUMPAD_ENTER))
	{
		// handle only char->press!
		return;
	}

	if (keyb==KEYB_CHAR && (key=='\\' || key=='|') ||
		keyb==KEYB_DOWN && key==A3D_ESCAPE)
	{
		CloseMenu();
		return;
	}

	if (keyb==KEYB_CHAR && key==8)
	{
		keyb=KEYB_PRESS;
		key=A3D_BACKSPACE;
	}

	if (keyb==KEYB_CHAR && (key=='\n' || key=='\r'))
	{
		keyb=KEYB_PRESS;
		key=A3D_ENTER;
	}

	if (keyb==KEYB_DOWN || keyb==KEYB_PRESS)
	{
		const Menu* m = game_menu;
		for (int d=0; d<menu_depth; d++)
			m = m[ menu_stack[d] ].sub;

		if (menu_stack[menu_depth]>=0)
		{
			if (key==A3D_RIGHT && m[ menu_stack[menu_depth] ].sub || key==A3D_ENTER)
			{
				if (m[ menu_stack[menu_depth] ].sub)
				{
					menu_depth++;
					menu_stack[menu_depth]=0;
					menu_temp = menu_stack[menu_depth];
				}
				else
				if (m[ menu_stack[menu_depth] ].action)
				{
					m[ menu_stack[menu_depth] ].action(this);
				}
				return;
			}
		}
		else
		if (key==A3D_RIGHT || key==A3D_ENTER)
		{
			menu_stack[menu_depth]=menu_temp;
		}

		if (key==A3D_LEFT || keyb==KEYB_PRESS && key==A3D_BACKSPACE)
		{
			if (menu_depth==0)
			{
				CloseMenu();
				return;
			}
			menu_depth--;
			menu_temp = menu_stack[menu_depth];
			return;
		}

		if (key==A3D_DOWN)
		{
			if (menu_stack[menu_depth] < 0)
				menu_stack[menu_depth] = menu_temp;
			else
			if (m[menu_stack[menu_depth]+1].str)
			{
				menu_stack[menu_depth]++;
				menu_temp = menu_stack[menu_depth];
			}
			return;
		}

		if (key==A3D_UP)
		{
			if (menu_stack[menu_depth] < 0)
				menu_stack[menu_depth] = menu_temp;
			else
			if (menu_stack[menu_depth]>0)
			{
				menu_stack[menu_depth]--;
				menu_temp = menu_stack[menu_depth];
			}
			return;
		}
	}
}


void Game::MenuMouse(GAME_MOUSE mouse, int x, int y)
{
	if (menu_down==2)
		return; // captured by touch

	if (mouse == GAME_MOUSE::MOUSE_MOVE)
	{
		if (menu_down)
		{
			// retest
			int hit = HitMenu(x,y);
			if (hit != menu_stack[menu_depth])
				menu_stack[menu_depth] = -1;
		}
	}

	if (mouse == GAME_MOUSE::MOUSE_LEFT_BUT_DOWN)
	{
		menu_down = 1;

		int hit = HitMenu(x,y);
		if (hit<-1)
		{
			CloseMenu();
			return;						
		}

		if (hit>=0)
		{
			menu_stack[menu_depth]=hit;
			menu_temp = menu_stack[menu_depth];
		}
		else
			menu_stack[menu_depth]=-1;

		return;
	}

	if (mouse == GAME_MOUSE::MOUSE_LEFT_BUT_UP)
	{
		if (menu_down)
		{
			// retest
			int hit = HitMenu(x,y);
			if (hit == menu_stack[menu_depth])
			{
				if (hit==-1)
				{
					// go back
					if (menu_depth==0)
					{
						CloseMenu();
						return;						
					}
					else
					{
						menu_depth--;
						menu_temp = menu_stack[menu_depth];
					}
				}
				else
				if (hit>=0)
				{
					const Menu* m = game_menu;
					for (int d=0; d<menu_depth; d++)
						m = m[ menu_stack[d] ].sub;		

					// action!
					if (m[ menu_stack[menu_depth] ].sub)
					{
						menu_depth++;
						menu_stack[menu_depth]=-1; // clear next hilight
						menu_temp = 0;
					}
					else
					if (m[ menu_stack[menu_depth] ].action)
					{
						m[ menu_stack[menu_depth] ].action(this);
					}
				}
			}
		}

		menu_down = 0;
		menu_stack[menu_depth]=-1;
	}
}

void Game::MenuTouch(GAME_TOUCH touch, int id, int x, int y)
{
	if (menu_down==1)
		return; // captured by mouse

	if (id==1)
	{
		switch(touch)
		{
			case GAME_TOUCH::TOUCH_BEGIN:
			{
				menu_down = 2;
				int hit = HitMenu(x,y);
				if (hit<-1)
				{
					CloseMenu();
					return;						
				}

				if (hit>=0)
				{
					menu_stack[menu_depth]=hit;
					menu_temp = menu_stack[menu_depth];
				}
				else
					menu_stack[menu_depth]=-1;

				break;
			}

			case GAME_TOUCH::TOUCH_MOVE:
				if (menu_down)
				{
					// retest
					int hit = HitMenu(x,y);
					if (hit != menu_stack[menu_depth])
						menu_stack[menu_depth] = -1;
				}
				break;

			case GAME_TOUCH::TOUCH_END:
			{
				if (menu_down)
				{
					// retest
					int hit = HitMenu(x,y);
					if (hit == menu_stack[menu_depth])
					{
						if (hit==-1)
						{
							// go back
							if (menu_depth==0)
							{
								CloseMenu();
								return;						
							}
							else
							{
								menu_depth--;
								menu_temp = menu_stack[menu_depth];
							}
						}
						else
						if (hit>=0)
						{
							const Menu* m = game_menu;
							for (int d=0; d<menu_depth; d++)
								m = m[ menu_stack[d] ].sub;		

							// action!
							if (m[ menu_stack[menu_depth] ].sub)
							{
								menu_depth++;
								menu_stack[menu_depth]=-1; // clear next hilight
								menu_temp = 0;
							}
							else
							if (m[ menu_stack[menu_depth] ].action)
							{
								m[ menu_stack[menu_depth] ].action(this);
							}
						}
					}
				}

				menu_down = 0;
				menu_stack[menu_depth]=-1;				
				break;
			}

			case GAME_TOUCH::TOUCH_CANCEL:
				menu_down = 0;
				menu_stack[menu_depth]=-1;
				break;
		}
	}
}

void Game::MenuPadMount(bool connected)
{
}

void Game::MenuPadButton(int b, bool down)
{
	if (menu_down)
		return; // captured by mouse/touch

	if (!down)
		return;

	const Menu* m = game_menu;
	for (int d=0; d<menu_depth; d++)
		m = m[ menu_stack[d] ].sub;		

	switch (b)
	{
		case 0:
		{
			if (menu_stack[menu_depth]>=0)
			{
				if (m[ menu_stack[menu_depth] ].sub)
				{
					menu_depth++;
					menu_stack[menu_depth]=0;
					menu_temp = menu_stack[menu_depth];
				}
				else
				if (m[ menu_stack[menu_depth] ].action)
				{
					m[ menu_stack[menu_depth] ].action(this);
				}
			}
			else
				menu_stack[menu_depth]=menu_temp;
			break;
		}

		case 1: 
		{
			// jump
			break;
		}

		case 5:
		{
			break;
		}

		case 6:
		{
			CloseMenu();
			break;
		}

		case 9:
		{
			// left shoulder
			break;
		}

		case 10:
		{
			// right shoulder
			break;
		}

		case 11:
		{
			// dir up
			if (menu_stack[menu_depth]<0)
				menu_stack[menu_depth]=menu_temp;
			else
			if (menu_stack[menu_depth]>0)
			{
				menu_stack[menu_depth]--;			
				menu_temp = menu_stack[menu_depth];
			}
			break;
		}
		case 12:
		{
			// dir down
			if (menu_stack[menu_depth]<0)
				menu_stack[menu_depth]=menu_temp;
			else
			if (m[menu_stack[menu_depth]+1].str)
			{
				menu_stack[menu_depth]++;			
				menu_temp = menu_stack[menu_depth];
			}
			break;
		}
		case 13:
		{
			// dir left
			if (menu_depth==0)
			{
				CloseMenu();
				return;
			}
			menu_depth--;
			menu_temp = menu_stack[menu_depth];
			break;
		}
		case 14:
		{
			if (menu_stack[menu_depth]>=0)
			{
				// dir right
				// only sub, with dir_right
				// action requires main button
				if (m[ menu_stack[menu_depth] ].sub)
				{
					menu_depth++;
					menu_stack[menu_depth]=0;
					menu_temp = menu_stack[menu_depth];
				}
			}
			else
				menu_stack[menu_depth]=menu_temp;
			break;
		}
	}
}

void Game::MenuPadAxis(int a, int16_t pos)
{
}
