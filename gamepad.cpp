
#include "sprite.h"
#include "gamepad.h"

extern char base_path[1024];
static Sprite* gamepad_sprite = 0;
static char gamepad_name[256] = "";
static int gamepad_axes = 0;
static int gamepad_buttons =0;
static bool gamepad_connected = false;

static int16_t gamepad_axis[32] = { 0 };
static int16_t gamepad_button[32] = { 0 };

struct SpriteElem
{
	int src_x, src_y; // position on atlas (top to bottom!)
	int w, h;         // element width, height
	int dst_x, dst_y; // assembly shift (top to bottom!)
};

static const SpriteElem gamepad_proto[] =
{
	{ 8,8, 35,19, 8,8 }, // body

	{ 4,27, 11,12, 4,20 }, // l-handle
	{ 36,27, 11,12, 36,20 }, // r-handle

	{ 1,1, 7,7, 10,11 }, // l-stick bk

	{ 33,51, 7,7, 28,17 }, // r-stick bk

	{ 26,28, 9,10, 15,16 }, // d-pad
	{ 16,28, 9,10, 33,10 }, // abxy
	{ 20,1, 11,6, 20,11 }, // qmg

	// overlays
	// ...

	// movables
	{ 2,9, 5,5, 11,12 }, // l-stick
	{ 34,39, 5,5, 29,18 }, // r-stick

	// movable overlays
	{ 2,15, 5,5, 11,12 }, // l-stick hilight
	{ 34,45, 5,5, 29,18 }, // r-stick hilight

	// move to overlays ...
	{ 46,6, 4,3, 15,19 }, // DL	
	{ 46,10, 4,3, 20,19}, // DR
	{ 47,1, 3,4, 18,16}, // DU
	{ 47,14, 3,4, 18,21}, // DD

	{ 47,35, 3,3, 36,16 }, // A
	{ 47,39, 3,3, 39,13 }, // B
	{ 47,27, 3,3, 33,13 }, // X
	{ 47,31, 3,3, 36,10 }, // Y

	{ 47,43, 3,3, 20,13 }, // Q
	{ 47,47, 3,3, 28,13 }, // M
	{ 47,51, 3,3, 24,11 }, // G

	// move to after stick bkgs
	{ 12,1, 6,2, 12,7}, // Lt
	{ 2,21, 6,2, 12,7}, // Lt hi
	{ 33,1, 6,2, 33,7}, // Rt
	{ 43,21, 6,2, 33,7}, // Rt hi
	{ 11,3, 5,2, 11,8}, // Ls
	{ 1,23, 5,2, 11,8}, // Ls hi
	{ 35,3, 5,2, 35,8}, // Rs
	{ 45,23, 5,2, 35,8}, // Rs hi

	{ 0 }
};

void LoadGamePad()
{
	char path[1024];
	const char* name = "gamepad.xp";
	sprintf(path, "%ssprites/%s", base_path, name);
	gamepad_sprite = LoadSprite(path, name);
}

void FreeGamePad()
{
	FreeSprite(gamepad_sprite);
	gamepad_sprite = 0;
}

uint32_t UpdateGamePadAxis(int a, int16_t pos)
{
	gamepad_axis[a] = pos;
	return 0;
}

uint32_t UpdateGamePadButton(int b, int16_t pos)
{
	gamepad_button[b] = pos;
	return 0;
}

void ConnectGamePad(const char* name, int axes, int buttons, const int16_t axis[], const int16_t button[])
{
	printf("GAMEPAD MOUNT: %s %d axes %d buttons\n", name, axes, buttons);
	strcpy(gamepad_name, name);
	gamepad_buttons = buttons;
	gamepad_axes = axes;
	gamepad_connected = true;

	if (axis)
		memcpy(gamepad_axis, axis, sizeof(int16_t)*axes);
	else
		memset(gamepad_axis, 0, sizeof(int16_t)*axes);
	if (button)
		memcpy(gamepad_button, button, sizeof(int16_t)*buttons);
	else
		memset(gamepad_button, 0, sizeof(int16_t)*buttons);
}

void DisconnectGamePad()
{
	gamepad_name[0]=0;
	gamepad_buttons = 0;
	gamepad_axes = 0;
	gamepad_connected = false;
}

void PaintGamePad(AnsiCell* ptr, int width, int height)
{
	if (!gamepad_sprite)
		return;

	Sprite::Frame* sf = gamepad_sprite->atlas;

	int w = sf->width;
	int h = sf->height;

	int x = (width - w) / 2;
	int y = height - h + 8 - 4; // <- num of buttons, axes and screen height dependent !!!

	int elems = 31;
	for (int e = 0; e < elems; e++)
	{
		// overlays
		if (e == 10 && gamepad_button[7] <= 32767 / 2)
			continue;
		if (e == 11 && gamepad_button[8] <= 32767 / 2)
			continue;

		if (e == 12 && gamepad_button[13] <= 32767 / 2)
			continue;
		if (e == 13 && gamepad_button[14] <= 32767 / 2)
			continue;
		if (e == 14 && gamepad_button[11] <= 32767 / 2)
			continue;
		if (e == 15 && gamepad_button[12] <= 32767 / 2)
			continue;

		if (e == 16 && gamepad_button[0] <= 32767 / 2)
			continue;
		if (e == 17 && gamepad_button[1] <= 32767 / 2)
			continue;
		if (e == 18 && gamepad_button[2] <= 32767 / 2)
			continue;
		if (e == 19 && gamepad_button[3] <= 32767 / 2)
			continue;

		if (e == 20 && gamepad_button[4] <= 32767 / 2)
			continue;
		if (e == 21 && gamepad_button[6] <= 32767 / 2)
			continue;
		if (e == 22 && gamepad_button[5] <= 32767 / 2)
			continue;

		if (e == 24 && gamepad_axis[4] <= 32767 / 2)
			continue;
		if (e == 26 && gamepad_axis[5] <= 32767 / 2)
			continue;
		if (e == 28 && gamepad_button[9] <= 32767 / 2)
			continue;
		if (e == 30 && gamepad_button[10] <= 32767 / 2)
			continue;

		int dx, dy;
		int clip[4];

		dx = x + gamepad_proto[e].dst_x;
		dy = y + h - 1 - (gamepad_proto[e].dst_y + gamepad_proto[e].h - 1);

		if (e == 8 || e == 10)
		{
			if (gamepad_axes > 0)
				dx += gamepad_axis[0] * 2 / 22000;
			if (gamepad_axes > 1)
				dy -= gamepad_axis[1] * 2 / 22000;
		}
		if (e == 9 || e == 11)
		{
			if (gamepad_axes > 2)
				dx += gamepad_axis[2] * 2 / 22000;
			if (gamepad_axes > 3)
				dy -= gamepad_axis[3] * 2 / 22000;
		}

		clip[0] = gamepad_proto[e].src_x;
		clip[1] = h - 1 - (gamepad_proto[e].src_y + gamepad_proto[e].h - 1);
		clip[2] = clip[0] + gamepad_proto[e].w;
		clip[3] = clip[1] + gamepad_proto[e].h;

		BlitSprite(ptr, width, height, sf, dx, dy, clip);
	}
}
