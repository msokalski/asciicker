#include <string.h>
#include <math.h>
#include "game.h"
#include "platform.h"


struct KeyCap
{
	// key_width  = border(1) + bevels(2) + size(min 2) + WIDTH_DELTA*current_size_multiplier(0-9)
	// row_width  = SUM(key_width[0..N-1]) + border(1)
	// key_pos(i) = row_xofs + SUM(key_width[0..i-1])
	enum FLAGS
	{
		WIDTH_DELTA1 = 1,
		WIDTH_DELTA2 = 2,
		WIDTH_DELTA3 = 3, // unused
		WIDTH_DELTA4 = 4,
		RIGHT_ALIGN = 8,
		DARK_COLOR = 0x10,
	};
	int flags; // left = 0, right = 1
	int size;
	int a3d_key;
	const char* cap[5][2];

	// other calculabe fields 
	int Paint(AnsiCell* ptr, int width, int height, int x, int y, int width_mul) const
	{
		static const uint8_t white = 16 + 5 * 1 + 5 * 6 + 5 * 36;
		static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;
		static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
		static const uint8_t black = 16 + 0;
		static const AnsiCell bevel[2][3] =
		{
			{ {white,lt_grey,176}, { black,lt_grey,32 }, {dk_grey,lt_grey,176} },
			{ {lt_grey,dk_grey,176}, { black,dk_grey,32 }, {black,dk_grey,176} }
		};

		int b = (flags >> 4) & 1;
		int w = (flags & 0x7) * width_mul + size + 2;

		AnsiCell* top = ptr + width * (y + 3) + x;
		AnsiCell* upper = ptr + width * (y + 2) + x;
		AnsiCell* lower = ptr + width * (y + 1) + x;
		AnsiCell* bottom = ptr + width * (y + 0) + x;

		int dx = 0;
		for (; dx < w - 1; dx++)
			top[dx] = bevel[b][0];
		top[dx] = bevel[b][1];

		int cap_index = 0;
		for (int i = 0; i < width_mul; i++)
			if (cap[i][0])
				cap_index = i;

		const char* upper_cap = cap[cap_index][0];
		const char* lower_cap = cap[cap_index][1];

		upper[0] = bevel[b][0];
		dx = 1;
		for (; dx < w - 1 && upper_cap[dx - 1]; dx++)
		{
			upper[dx] = bevel[b][1];
			upper[dx].gl = upper_cap[dx - 1];
		}
		for (; dx < w - 1; dx++)
			upper[dx] = bevel[b][1];
		upper[dx] = bevel[b][2];

		lower[0] = bevel[b][0];
		dx = 1;
		for (; dx < w - 1 && lower_cap[dx - 1]; dx++)
		{
			lower[dx] = bevel[b][1];
			lower[dx].gl = lower_cap[dx - 1];
		}
		for (; dx < w - 1; dx++)
			lower[dx] = bevel[b][1];
		lower[dx] = bevel[b][2];

		bottom[0] = bevel[b][1];
		dx = 1;
		for (; dx < w; dx++)
			bottom[dx] = bevel[b][2];

		return w + 1;
	}
};

const static KeyCap row0[] =
{
	{ 0x01, 2, A3D_1, {{"!","1"}, {0,0}} },
	{ 0x01, 2, A3D_2, {{"@","2"}, {0,0}} },
	{ 0x01, 2, A3D_3, {{"#","3"}, {0,0}} },
	{ 0x01, 2, A3D_4, {{"$","4"}, {0,0}} },
	{ 0x01, 2, A3D_5, {{"%","5"}, {0,0}} },
	{ 0x01, 2, A3D_6, {{"^","6"}, {0,0}} },
	{ 0x01, 2, A3D_7, {{"&","7"}, {0,0}} },
	{ 0x01, 2, A3D_8, {{"*","8"}, {0,0}} },
	{ 0x01, 2, A3D_9, {{"(","9"}, {0,0}} },
	{ 0x01, 2, A3D_0, {{")","0"}, {0,0}} },
	{ 0x19, 4, A3D_BACKSPACE, {{"\x11""Bck"," Spc"}, {"\x11""Back","Space"}, {"\x11 Back"," Space"}, {"\x11  Back","  Space"}, {0,0}} },
};

const static KeyCap row1[] =
{
	{ 0x01, 2, A3D_Q, {{"Q",""}, {0,0}} },
	{ 0x01, 2, A3D_W, {{"W",""}, {0,0}} },
	{ 0x01, 2, A3D_E, {{"E",""}, {0,0}} },
	{ 0x01, 2, A3D_R, {{"R",""}, {0,0}} },
	{ 0x01, 2, A3D_T, {{"T",""}, {0,0}} },
	{ 0x01, 2, A3D_Y, {{"Y",""}, {0,0}} },
	{ 0x01, 2, A3D_U, {{"U",""}, {0,0}} },
	{ 0x01, 2, A3D_I, {{"I",""}, {0,0}} },
	{ 0x01, 2, A3D_O, {{"O",""}, {0,0}} },
	{ 0x01, 2, A3D_P, {{"P",""}, {0,0}} },
	{ 0x01, 2, A3D_OEM_TILDE, {{"~","`"}, {0,0}} },
};

const static KeyCap row2[] =
{
	{ 0x01, 2, A3D_OEM_QUOTATION, {{"\"","\'"}, {0,0}} },
	{ 0x01, 2, A3D_A, {{"A",""}, {0,0}} },
	{ 0x01, 2, A3D_S, {{"S",""}, {0,0}} },
	{ 0x01, 2, A3D_D, {{"D",""}, {0,0}} },
	{ 0x01, 2, A3D_F, {{"F",""}, {0,0}} },
	{ 0x01, 2, A3D_G, {{"G",""}, {0,0}} },
	{ 0x01, 2, A3D_H, {{"H",""}, {0,0}} },
	{ 0x01, 2, A3D_J, {{"J",""}, {0,0}} },
	{ 0x01, 2, A3D_K, {{"K",""}, {0,0}} },
	{ 0x01, 2, A3D_L, {{"L",""}, {0,0}} },
	{ 0x01, 2, A3D_OEM_COLON, {{":",";"}, {0,0}} },
};

const static KeyCap row3[] =
{
	{ 0x01, 2, A3D_OEM_BACKSLASH, {{"|","\\"}, {0,0}} },
	{ 0x01, 2, A3D_Z, {{"Z",""}, {0,0}} },
	{ 0x01, 2, A3D_X, {{"X",""}, {0,0}} },
	{ 0x01, 2, A3D_C, {{"C",""}, {0,0}} },
	{ 0x01, 2, A3D_V, {{"V",""}, {0,0}} },
	{ 0x01, 2, A3D_B, {{"B",""}, {0,0}} },
	{ 0x01, 2, A3D_N, {{"N",""}, {0,0}} },
	{ 0x01, 2, A3D_M, {{"M",""}, {0,0}} },
	{ 0x01, 2, A3D_OEM_COMMA, {{"<",","}, {0,0}} },
	{ 0x01, 2, A3D_OEM_PERIOD, {{">","."}, {0,0}} },
	{ 0x01, 2, A3D_OEM_SLASH, {{"?","/"}, {0,0}} },
};

const static KeyCap row4[] =
{
	{ 0x10, 7, A3D_LSHIFT, {{"Shift \x1E",""}, {0,0}} },
	{ 0x01, 2, A3D_OEM_MINUS, {{"_","-"}, {0,0}} },
	{ 0x01, 2, A3D_OEM_PLUS, {{"+","="}, {0,0}} },
	{ 0x05,22-6, A3D_SPACE, {{"Space",""}, {0,0}} },
	{ 0x01, 2, A3D_OEM_OPEN, {{"{","["}, {0,0}} },
	{ 0x01, 2, A3D_OEM_CLOSE, {{"}","]"}, {0,0}} },
	{ 0x1A, 5, A3D_ENTER, {{"Enter","\x11\xC4\xD9"}, {" Enter"," \x11\xC4\xD9"}, {"\x11\xC4\xD9 Enter",""}, {0,0}} },
};

struct KeyRow
{
	int xofs;
	int caps;
	const KeyCap* row;

	// other calculabe fields 
	// ...

	void Paint(AnsiCell* ptr, int width, int height, int x, int y, int width_mul) const
	{
		int xadv = x + xofs;
		for (int cap = 0; cap < caps; cap++)
			xadv += row[cap].Paint(ptr, width, height, xadv, y, width_mul);
	}
};

struct Keyb
{
	KeyRow rows[5];

	void Paint(AnsiCell* ptr, int width, int height, int width_mul) const
	{
		const static int yadv = 5;

		int xofs = 1;
		int yofs = 1;

		for (int i = 0; i < 5; i++)
		{
			rows[4 - i].Paint(ptr, width, height, xofs, yofs, width_mul);
			yofs += yadv;
		}
	}
};

const static Keyb keyb =
{
	{
		{ 0,11, row0 },
		{ 2,11, row1 },
		{ 0,11, row2 },
		{ 2,11, row3 },
		{ 0, 7, row4 },
	}
};

////////////////////////////////////////////////////////////////////////////////////


// here we need all character sprites
// ...

extern Terrain* terrain;
extern World* world;

struct KeyConfig
{
};

Sprite* player_0000=0;
Sprite* player_0001=0;
Sprite* player_0010=0;
Sprite* player_0011=0;

Sprite* plydie_0000=0;
Sprite* plydie_0001=0;
Sprite* plydie_0010=0;
Sprite* plydie_0011=0;

Sprite* attack_0001=0;
Sprite* attack_0011=0;

Sprite* wolfie=0;
Sprite* woldie=0;

Sprite* wolfie_0000=0;
Sprite* wolfie_0001=0;
Sprite* wolfie_0010=0;
Sprite* wolfie_0011=0;

Sprite* wolack_0001=0;
Sprite* wolack_0011=0;

void LoadSprites()
{
	player_0000 = LoadSprite("./sprites/player-0000.xp", "player_0000.xp", 0);
	wolfie_0011 = LoadSprite("./sprites/wolfie-0011.xp", "wolfie-0011.xp", 0);
	plydie_0000 = LoadSprite("./sprites/plydie-0000.xp", "plydie-0000.xp", 0);
}

void FreeSprites()
{
	FreeSprite(player_0000);
}

Game* CreateGame(int water, float pos[3], float yaw, float dir, uint64_t stamp)
{
	// load defaults
	Game* g = (Game*)malloc(sizeof(Game));
	memset(g, 0, sizeof(Game));

	g->renderer = CreateRenderer(stamp);
	g->physics = CreatePhysics(terrain, world, pos, dir, yaw, stamp);
	g->stamp = stamp;

	// init player!
	g->player.sprite = player_0000;
	g->player.anim = 0;
	g->player.frame = 0;

	g->water = water;
	g->prev_yaw = yaw;

	return g;
}

void DeleteGame(Game* g)
{
	if (g)
	{
		if (g->renderer)
			DeleteRenderer(g->renderer);
		if (g->physics)
			DeletePhysics(g->physics);
		free(g);
	}
}

void Game::Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
	float lt[4] = { 1,0,1,0.5 };

	PhysicsIO io;
	io.x_force = 0;
	io.y_force = 0;
	io.torque = 0;
	io.water = water;
	io.jump = false;

	if (input.drag == 1 && input.size[0]>0 && input.size[1]>0)
	{
		int x = (float)input.pos[0] / input.size[0];
		int y = (float)input.pos[1] / input.size[1];

		io.x_force = 2 * (input.pos[0] * 2 - input.size[0]) / (float)input.size[0];
		io.y_force = 2 * (input.size[1] - input.pos[1] * 2) / (float)input.size[1];
	}
	else
	{
		float speed = 1;
		if (input.IsKeyDown(A3D_LSHIFT) || input.IsKeyDown(A3D_RSHIFT))
			speed *= 0.5;
		io.x_force = (int)(input.IsKeyDown(A3D_RIGHT) || input.IsKeyDown(A3D_D)) - (int)(input.IsKeyDown(A3D_LEFT) || input.IsKeyDown(A3D_A));
		io.y_force = (int)(input.IsKeyDown(A3D_UP) || input.IsKeyDown(A3D_W)) - (int)(input.IsKeyDown(A3D_DOWN) || input.IsKeyDown(A3D_S));

		float len = sqrtf(io.x_force*io.x_force + io.y_force*io.y_force);
		if (len > 0)
			speed /= len;
		io.x_force *= speed;
		io.y_force *= speed;
	}

	if (input.drag == 2 && input.size[0] > 0)
	{
		float sensitivity = 200.0f / input.size[0];
		float yaw = prev_yaw - sensitivity * (input.pos[0] - input.drag_from[0]);
		io.torque = 0;
		SetPhysicsYaw(physics, yaw, 0);
	}
	else
	{
		io.torque = (int)(input.IsKeyDown(A3D_DELETE) || input.IsKeyDown(A3D_PAGEUP) || input.IsKeyDown(A3D_F1) || input.IsKeyDown(A3D_Q)) -
			(int)(input.IsKeyDown(A3D_INSERT) || input.IsKeyDown(A3D_PAGEDOWN) || input.IsKeyDown(A3D_F2) || input.IsKeyDown(A3D_E));
	}

	io.jump = input.jump;
	int steps = Animate(physics, _stamp, &io);
	if (steps > 0)
		input.jump = false;

	if (!(input.drag == 2 && input.size[0] > 0))
		prev_yaw = io.yaw; // readback yaw if not dragged

	player.pos[0] = io.pos[0];
	player.pos[1] = io.pos[1];
	player.pos[2] = io.pos[2];

	// update / animate:
	player.dir = io.player_dir;

	if (io.player_stp < 0)
	{
		// choose sprite by active items
		player.sprite = player_0000;
		player.anim = 0;
		player.frame = 0;
	}
	else
	{
		// choose sprite by active items
		player.sprite = player_0000;
		player.anim = 1;
		player.frame = io.player_stp / 1024 % player_0000->anim[player.anim].length;
	}

	::Render(renderer, _stamp, terrain, world, water, 1.0, io.yaw, io.pos, lt,
		width, height, ptr, player.sprite, player.anim, player.frame, player.dir);

	// keyb.Paint(ptr, width, height, 5);

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

					uint8_t f_rgb[3] = { f_b,f_g,f_r };
					uint8_t b_rgb[3] = { b_b,b_g,b_r };
					uint32_t chr = c->gl;

					fwrite(&chr, sizeof(uint32_t), 1, f);
					fwrite(f_rgb, 1, 3, f);
					fwrite(b_rgb, 1, 3, f);
				}
			}

			fclose(f);
		}
	}

	stamp = _stamp;
}

void Game::OnSize(int w, int h)
{
	memset(&input, 0, sizeof(Input));
	input.size[0] = w;
	input.size[1] = h;
}

void Game::OnKeyb(GAME_KEYB keyb, int key)
{
	// handle layers first ...

	// if nothing focused 
	if (keyb == GAME_KEYB::KEYB_DOWN)
	{
		if (key == A3D_F10)
			input.shot = true;
		if (key == A3D_SPACE)
			input.jump = true;
		input.key[key >> 3] |= 1 << (key & 0x7);
	}
	else
	if (keyb == GAME_KEYB::KEYB_UP)
		input.key[key >> 3] &= ~(1 << (key & 0x7));
}

void Game::OnMouse(GAME_MOUSE mouse, int x, int y)
{
	// handle layers first ...

	// if nothing focused
	switch (mouse)
	{
		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 
			if (input.but == 0)
			{
				input.drag = 1;
				input.drag_from[0] = x;
				input.drag_from[1] = y;
			}
			input.but |= 1; 
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_UP:   
			if (input.drag == 1) 
				input.drag = 0; 
			input.but &= ~1; 
			break;
		case GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN: 
			if (input.but == 0)
			{
				input.drag = 2;
				input.drag_from[0] = x;
				input.drag_from[1] = y;
			}
			if (input.drag == 1)
				input.jump = true;
			input.but |= 2; 
			break;
		case GAME_MOUSE::MOUSE_RIGHT_BUT_UP:   
			if (input.drag == 2) 
				input.drag = 0; 
			input.but &= ~2; 
			break;
	}

	input.pos[0] = x;
	input.pos[1] = y;
}

void Game::OnTouch(GAME_TOUCH touch, int id, int x, int y)
{
	// handle layers first ...
}

void Game::OnFocus(bool set)
{
	// if loosing focus, clear all tracking / dragging / keyboard states
	if (!set)
	{
		int w = input.size[0], h = input.size[1];
		memset(&input, 0, sizeof(Input));
		input.size[0] = w;
		input.size[1] = h;
	}
}
