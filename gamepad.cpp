#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "sprite.h"
#include "gamepad.h"

#include "fast_rand.h"


extern char base_path[1024];
static Sprite* gamepad_sprite = 0;
static char gamepad_name[256] = "";
static int gamepad_axes = 0;
static int gamepad_buttons =0;
static bool gamepad_connected = false;

// inverse maps - dependences, 0xFF terminated!
// rebuild them every time mapping changes
static uint8_t* button_mapping[15] = {0};
static uint8_t* axis_mapping[6] = {0};

// currently defined mapping
static uint8_t gamepad_mapping[256] = {0};
static int16_t gamepad_input[256]; // all are positive!

// cache for determining if event triggers is neccessary
// also for painting mapped gamepad state 
static int16_t gamepad_axis_output[6] = {0};
static int16_t gamepad_button_output[15] = {0};

// used for painting raw inputs 
static int16_t gamepad_axis[256] = { 0 };
static int16_t gamepad_button[256] = { 0 };


static const char* gamepad_half_axis_name[]=
{
	"Ll","Lr", "Lu","Ld", "Rl","Rr", "Ru","Rd", "Lt","Lt", "Rt","Rt"
};

static const char* gamepad_button_name[]=
{
	"A ","B ","X ","Y ", "Q ","G ","M ", "L ","R ", "Ls","Rs", "Du","Dd","Dl","Dr"
};

struct SpriteElem
{
	int src_x, src_y; // position on atlas (top to bottom!)
	int w, h;         // element width, height
	int dst_x, dst_y; // assembly shift (top to bottom!)
};

struct InputElem
{
	int src_x, src_y;
	int w, h;
};

static const InputElem axis_proto[] =
{
	{ 1,40, 10,3 },
	{ 1,43, 10,3 },
	{ 1,46, 10,3 },

	{ 11,40, 10,3 },
	{ 11,43, 10,3 },
	{ 11,46, 10,3 },

	{ 21,40, 10,3 },
	{ 21,43, 10,3 },
	{ 21,46, 10,3 },
	{0}
};

static const InputElem button_proto[] =
{
	{1,50,10,3},
	{1,53,10,3},
	{1,56,10,3},
	{11,50,10,3},
	{11,53,10,3},
	{11,56,10,3},
	{21,50,10,3},
	{21,53,10,3},
	{21,56,10,3},
	{0}
};

static const InputElem slot_proto[] =
{
	{42,45, 4,3},
	{ 0 }
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
	{ 46,6, 4,4, 15,19 }, // DL	
	{ 46,10, 4,4, 20,19}, // DR
	{ 47,1, 3,4, 18,16}, // DU
	{ 47,14, 3,5, 18,21}, // DD

	{ 47,35, 3,4, 36,16 }, // A
	{ 47,39, 3,4, 39,13 }, // B
	{ 47,27, 3,4, 33,13 }, // X
	{ 47,31, 3,4, 36,10 }, // Y

	{ 47,43, 3,4, 20,13 }, // Q
	{ 47,47, 3,4, 28,13 }, // M
	{ 47,51, 3,4, 24,11 }, // G

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

static int UpdateAxisOutput(int a, uint32_t* out)
{
	// a in [0..5]
	// run inverse map of a
	// accumulate input_values
	// clamp to int16_t, compare with previous value, if different update and return out

	uint8_t* dep = axis_mapping[a];
	if (!dep)
		return 0;

	int accum = 0;

	while (*dep != 0xFF)
	{
		uint8_t d = *dep;
		int v = gamepad_input[d];
		int m = gamepad_mapping[d];

		// check hacked unsigned
		int in = d>>1;
		if (((m&0x80) == 0) && gamepad_mapping[2*in] == gamepad_mapping[2*in+1])
		{
			if (d&1) // update once per hack pair
			{
				if (m&0x40)
					accum += ((int)gamepad_input[2*in] - (int)gamepad_input[2*in+1] + 32768)/2; // unsigned flipped
				else
					accum += ((int)gamepad_input[2*in+1] - (int)gamepad_input[2*in] + 32768)/2; // unsigned normal
			}
		}
		else
		{
			if (m&0x40)
				accum -= v;
			else
				accum += v;
		}

		dep++;
	}

	if (accum < -32767)
		accum = -32767;
	if (accum > +32767)
		accum = +32767;
	
	if (accum != gamepad_axis_output[a]) 
	{
		gamepad_axis_output[a] = accum;
		*out = (accum&0xffff) | (a<<24) | (0<<16);
		return 1;
	}

	return 0;
}

static int UpdateButtonOutput(int b, uint32_t* out)
{
	// b in [0..14]

	uint8_t* dep = button_mapping[b];
	if (!dep)
		return 0;

	int accum = 0;

	while (*dep != 0xFF)
	{
		uint8_t d = *dep;
		int v = gamepad_input[d];
		int m = gamepad_mapping[d];

		if (m&0x40)
			accum -= v;
		else
			accum += v;

		dep++;
	}

	if (accum < 0)
		accum = 0;
	if (accum > +32767)
		accum = +32767;
	
	if (accum != gamepad_button_output[b]) 
	{
		gamepad_button_output[b] = accum;
		*out = (accum&0xffff) | (b<<24) | (1<<16);
		return 1;
	}	

	return 0;
}

int UpdateGamePadAxis(int a, int16_t v, uint32_t out[2])
{
	// prevent wrap	at -max flipping
	if (v==-32768)
		v=-32767;

	gamepad_axis[a] = v;

	int16_t neg = v<0 ? -v : 0;
	int16_t pos = v>0 ? +v : 0;

	int outs = 0;

	if (gamepad_input[2*a+0] != neg)
	{
		gamepad_input[2*a+0] = neg;
		uint8_t m = gamepad_mapping[2*a+0];
		if (m != 0xFF)
		{
			// update negative part of input
			if (m&0x80)
				outs += UpdateButtonOutput(m&0x3F, out+outs);
			else
				outs += UpdateAxisOutput(m&0x3F, out+outs);
		}
	}

	if (gamepad_input[2*a+1] != pos)
	{
		gamepad_input[2*a+1] = pos;
		uint8_t m = gamepad_mapping[2*a+1];
		if (m != 0xFF)
		{
			// update positive part of input
			if (m&0x80)
				outs += UpdateButtonOutput(m&0x3F, out+outs);
			else
				outs += UpdateAxisOutput(m&0x3F, out+outs);
		}
	}

	return outs;
}

int UpdateGamePadButton(int b, int16_t v, uint32_t out[1])
{
	gamepad_button[b] = v;

	int16_t pos = v>0 ? +v : 0;

	int outs = 0;

	if (gamepad_input[2*gamepad_axes + b] != pos)
	{
		gamepad_input[2*gamepad_axes + b] = pos;
		uint8_t m = gamepad_mapping[2*gamepad_axes + b];
		if (m != 0xFF)
		{
			if (m&0x80)
				outs += UpdateButtonOutput(m&0x3F, out + outs);
			else
				outs += UpdateAxisOutput(m&0x3F, out + outs);
		}
	}

	return outs;
}

void ConnectGamePad(const char* name, int axes, int buttons, const uint8_t mapping[])
{
	strcpy(gamepad_name, name);
	gamepad_buttons = buttons;
	gamepad_axes = axes;
	gamepad_connected = true;

	int mappings = 2*axes+buttons;
	memcpy(gamepad_mapping,mapping,mappings);

	// build dependency maps for outputs
	int axis_len[6] = {0};
	int button_len[15] = {0};

	for (int i=0; i<mappings; i++)
	{
		uint8_t m = gamepad_mapping[i];

		if (m!=0xFF)
		{
			int j = m&0x3F;
			if ((m&0x80) == 0)
				axis_len[j]++;
			else
				button_len[j]++;
		}
	}

	for (int i=0; i<6; i++)
	{
		if (axis_len[i])
		{
			axis_mapping[i] = (uint8_t*)malloc(axis_len[i]+1);
			axis_mapping[i][axis_len[i]] = 0xFF;
		}
		else
			axis_mapping[i] = 0;
	}

	for (int i=0; i<15; i++)
	{
		if (button_len[i])
		{
			button_mapping[i] = (uint8_t*)malloc(button_len[i]+1);
			button_mapping[i][button_len[i]] = 0xFF;
		}
		else
			button_mapping[i] = 0;
	}

	memset(axis_len,0,sizeof(int)*6);
	memset(button_len,0,sizeof(int)*15);

	for (int i=0; i<mappings; i++)
	{
		uint8_t m = gamepad_mapping[i];

		if (m!=0xFF)
		{
			int j = m&0x3F;
			if ((m&0x80) == 0)
				axis_mapping[j][axis_len[j]++] = i;
			else
				button_mapping[j][button_len[j]++] = i;
		}
	}

	memset(gamepad_axis_output, 0, sizeof(int16_t)*6);
	memset(gamepad_button_output, 0, sizeof(int16_t)*15);
	memset(gamepad_input, 0, sizeof(int16_t)*(2*axes+buttons));
	memset(gamepad_axis, 0, sizeof(int16_t)*axes);
	memset(gamepad_button, 0, sizeof(int16_t)*buttons);
}

void DisconnectGamePad()
{
	for (int i=0; i<6; i++)
		if (axis_mapping[i])
			free(axis_mapping[i]);
	for (int i=0; i<15; i++)
		if (button_mapping[i])
			free(button_mapping[i]);

	gamepad_name[0]=0;
	gamepad_buttons = 0;
	gamepad_axes = 0;
	gamepad_connected = false;
}

static bool CalcLayout(int width, int height, int layout[] /*ec,er,ew,eh*/);

void PaintGamePad(AnsiCell* ptr, int width, int height)
{
	if (!gamepad_sprite)
		return;

	int layout[4];
	CalcLayout(width, height, layout);

	int ec = layout[0];
	int er = layout[1];
	int ew = layout[2];
	int eh = layout[3];

	Sprite::Frame* sf = gamepad_sprite->atlas;

	int w = sf->width;
	int h = sf->height;

	/*
	int x = (width - w) / 2;
	int y = height - h + 8 - 4; // <- num of buttons, axes and screen height dependent !!!
	*/

	// upper left corner of layout
	int x = (width - ew) / 2 -4; // -4 offset from sprite edge to handle 
	int y = height - (height - eh) / 2 +8-2; // +8 pad body from sprite's top, -2 trigger height

	int elems = 31;
	for (int e = 0; e < elems; e++)
	{
		// overlays
		if (e == 10 && gamepad_button_output[7] <= 32767 / 2)
			continue;
		if (e == 11 && gamepad_button_output[8] <= 32767 / 2)
			continue;

		if (e == 12 && gamepad_button_output[13] <= 32767 / 2)
			continue;
		if (e == 13 && gamepad_button_output[14] <= 32767 / 2)
			continue;
		if (e == 14 && gamepad_button_output[11] <= 32767 / 2)
			continue;
		if (e == 15 && gamepad_button_output[12] <= 32767 / 2)
			continue;

		if (e == 16 && gamepad_button_output[0] <= 32767 / 2)
			continue;
		if (e == 17 && gamepad_button_output[1] <= 32767 / 2)
			continue;
		if (e == 18 && gamepad_button_output[2] <= 32767 / 2)
			continue;
		if (e == 19 && gamepad_button_output[3] <= 32767 / 2)
			continue;

		if (e == 20 && gamepad_button_output[4] <= 32767 / 2)
			continue;
		if (e == 21 && gamepad_button_output[6] <= 32767 / 2)
			continue;
		if (e == 22 && gamepad_button_output[5] <= 32767 / 2)
			continue;
		/*
		if (e == 24 && gamepad_axis_output[4] <= 32767 / 2)
			continue;
		if (e == 26 && gamepad_axis_output[5] <= 32767 / 2)
			continue;
		*/

		int clip_left = 0;
		int clip_right = 0;

		if (e == 24)
			clip_right = (gamepad_proto[e].w-2) - gamepad_axis_output[4] * (gamepad_proto[e].w-2) / 32767;

		if (e == 26)
			clip_left = (gamepad_proto[e].w-2) - gamepad_axis_output[5] * (gamepad_proto[e].w-2) / 32767;

		if (e == 28 && gamepad_button_output[9] <= 32767 / 2)
			continue;
		if (e == 30 && gamepad_button_output[10] <= 32767 / 2)
			continue;

		int dx, dy;
		int clip[4];

		dx = x + gamepad_proto[e].dst_x;
		dy = y - (gamepad_proto[e].dst_y + gamepad_proto[e].h - 1);

		//dx -= 3;
		//dy += 5;

		if (e == 8 || e == 10)
		{
			if (gamepad_axes > 0)
				dx += gamepad_axis_output[0] * 2 / 22000;
			if (gamepad_axes > 1)
				dy -= gamepad_axis_output[1] * 2 / 22000;
		}
		if (e == 9 || e == 11)
		{
			if (gamepad_axes > 2)
				dx += gamepad_axis_output[2] * 2 / 22000;
			if (gamepad_axes > 3)
				dy -= gamepad_axis_output[3] * 2 / 22000;
		}

		clip[0] = gamepad_proto[e].src_x + clip_left;
		clip[1] = h - 1 - (gamepad_proto[e].src_y + gamepad_proto[e].h - 1);
		clip[2] = clip[0] + gamepad_proto[e].w - clip_right;
		clip[3] = clip[1] + gamepad_proto[e].h;

		BlitSprite(ptr, width, height, sf, dx + clip_left, dy, clip);
	}

	int col_x[3] = { 1+3,16+1,31-1 };
	int row_y = 35;
	int row = 0;
	int col = 2;

	/*
	static int t = 0;
	t++;
	if (t == 100)
		t = 0;
	*/

	for (int a = 0; a < gamepad_axes; a++)
	{
		// all in col=2
		//int v = sinf((a*10 + t)*2*M_PI / 100) * 32767;
		int v = gamepad_axis[a];
		int i = v * 9 / 2 / 32767 +4;

		int dx, dy;
		int clip[4];

		dx = x + col_x[col];
		dy = y - (row_y + row * 3);

		//dx -= 3;
		//dy += 5;

		clip[0] = axis_proto[i].src_x;
		clip[1] = h - 1 - (axis_proto[i].src_y + axis_proto[i].h - 1);
		clip[2] = clip[0] + axis_proto[i].w;
		clip[3] = clip[1] + axis_proto[i].h;

		BlitSprite(ptr, width, height, sf, dx, dy, clip);

		AnsiCell* label = ptr + (dx + 1) + (dy + 1) * width;
		if (dy+1>=0 && dy+1<height)
		{
			if (dx+2>=0 && dx+2<width)
				label[1].gl = '0' + a / 10;
			if (dx+3>=0 && dx+3<width)
				label[2].gl = '0' + a % 10;
		}


		clip[0] = slot_proto[0].src_x;
		clip[1] = h - 1 - (slot_proto[0].src_y + slot_proto[0].h - 1);
		clip[2] = clip[0] + slot_proto[0].w;
		clip[3] = clip[1] + slot_proto[0].h;

		dx += button_proto[i].w - 1;
		BlitSprite(ptr, width, height, sf, dx, dy, clip);

		uint8_t neg = gamepad_mapping[2*a];
		if (neg!=0xFF && dy+1>=0 && dy+1<height && dx+1>=0 && dx+1<width)
		{
			label = ptr + (dx + 1) + (dy + 1) * width;
			const char* name;
			if (neg&0x80)
				name = gamepad_button_name[neg&0x3F];
			else
				name = gamepad_half_axis_name[2*(neg&0x3F) + (neg&0x40 ? 0 : 1)];
			label[0].gl = name[0];
			label[1].gl = name[1];
		}

		dx += slot_proto[0].w - 1;
		BlitSprite(ptr, width, height, sf, dx, dy, clip);

		uint8_t pos = gamepad_mapping[2*a+1];
		if (pos!=0xFF && dy+1>=0 && dy+1<height && dx+1>=0 && dx+1<width)
		{
			label = ptr + (dx + 1) + (dy + 1) * width;
			const char* name;
			if (pos&0x80)
				name = gamepad_button_name[pos&0x3F];
			else
				name = gamepad_half_axis_name[2*(pos&0x3F) + (pos&0x40 ? 0 : 1)];
			label[0].gl = name[0];
			label[1].gl = name[1];
		}

		row++;
	}

	row = 0;
	col = 0;

	for (int b = 0; b < gamepad_buttons; b++)
	{
		//int v = sinf((b*10 + t) * 2 * M_PI / 100) * 16383 + 16384;
		int v = gamepad_button[b];
		int i = (v * 8 + 16384) / 32767;

		int dx, dy;
		int clip[4];

		dx = x + col_x[col];
		dy = y - (row_y + row * 3);

		//dx -= 3;		
		//dy += 5;

		clip[0] = button_proto[i].src_x;
		clip[1] = h - 1 - (button_proto[i].src_y + button_proto[i].h - 1);
		clip[2] = clip[0] + button_proto[i].w;
		clip[3] = clip[1] + button_proto[i].h;

		BlitSprite(ptr, width, height, sf, dx, dy, clip);

		AnsiCell* label = ptr + (dx + 1) + (dy + 1) * width;
		if (dy+1>=0 && dy+1<height)
		{
			if (dx+2>=0 && dx+2<width)
				label[1].gl = '0' + b / 10;
			if (dx+3>=0 && dx+3<width)
				label[2].gl = '0' + b % 10;
		}

		clip[0] = slot_proto[0].src_x;
		clip[1] = h - 1 - (slot_proto[0].src_y + slot_proto[0].h - 1);
		clip[2] = clip[0] + slot_proto[0].w;
		clip[3] = clip[1] + slot_proto[0].h;

		dx += button_proto[i].w - 1;
		BlitSprite(ptr, width, height, sf, dx, dy, clip);

		uint8_t pos = gamepad_mapping[2*gamepad_axes + b];
		if (pos!=0xFF && dy+1>=0 && dy+1<height && dx+1>=0 && dx+1<width)
		{
			label = ptr + (dx + 1) + (dy + 1) * width;
			const char* name;
			if (pos&0x80)
				name = gamepad_button_name[pos&0x3F];
			else
				name = gamepad_half_axis_name[2*(pos&0x3F) + (pos&0x40 ? 0 : 1)];
			label[0].gl = name[0];
			label[1].gl = name[1];
		}

		col++;

		if (row >= gamepad_axes)
		{
			if (col == 3)
			{
				col = 0;
				row++;
			}
		}
		else
		{
			if (col == 2)
			{
				col = 0;
				row++;
			}
		}
	}

}

static bool CalcLayout(int width, int height, int layout[] /*ec,er,w,h*/)
{
	// return number of columns

	// rules:
	// - rightmost column is a bit wider for axes
	// - all axes are placed in rightmost column
	// - buttons can be placed on any column

	// optimal solution:
	// 1 all inputs fits on screen
	// 2 w/h aspect ratio is closest to width/height


	int N = gamepad_axes + gamepad_buttons;

	float perfect_aspect = logf((float)width/(float)height);

	float best_err;
	float best_as;
	int best_ec;
	int best_er;
	int best_dw;
	int best_dh;

	bool valid = false;

	for (int ec = 0; ec < 5; ec++) 
	{
		// N = ec*(7+er) + er*3
		// N = 7ec + er(3+ec)
		// er = (N - 7ec) / (3+ec)

		int roundup = 2+ec;
		int er = (N - 8*ec + roundup) / (3+ec);

		if (ec==0)
			er = er < gamepad_axes ? gamepad_axes : er;
		else
			er = er+8 < gamepad_axes ? gamepad_axes-8 : er;

		int dw,dh;

		dw = 43;
		if (ec)
			dw += 16 + 13*(ec-1); // first ec is 16, every next is 13

		dh = er ? 26 + 3*er : 24;

		float aspect = logf((float)dw / (float)dh);
		float err = fabsf(aspect - perfect_aspect);

		if (ec==0 || err < best_err && (!valid || dw<=width && dh<=height))
		{
			best_err = err;
			best_ec = ec;
			best_er = er;
			best_dw = dw;
			best_dh = dh;
			best_as = aspect;

			if (dw<=width && dh<=height)
				valid = true;
		}
	}

	layout[0] = best_ec;
	layout[1] = best_er;
	layout[2] = best_dw;
	layout[3] = best_dh;

	return valid;
}

/*

add save and exit buttons to mapper

On mapper open:
- save current map to spare array
- disable all gamepad input processing (other than tool)
- clicking on input slot: UNMAP
- draggin from gamepad picture onto input slot: MAP/REMAP
- closing tool REVERTS ALL spare mapping to current
- clicking on SAVE overwrites spare with current and re-writes cfg file

On gamepad connect:
- check if cfg contains mapping for this gamepad (by name, and number of axes and buttons)
- if not, display black ribbon warning at the bottom and mount with defaults from OS
- otherwise mount with mapping from cfg

*/