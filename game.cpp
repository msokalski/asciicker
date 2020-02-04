#include <string.h>
#include <math.h>
#include "game.h"
#include "platform.h"

struct TalkBox
{
	int max_width, max_height;
	int size[2];
	int cursor_xy[2];
	int cursor_pos;
	int len;
	bool show_cursor;

	void Paint(AnsiCell* ptr, int width, int height, int x, int y, bool cursor) const
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
				static const uint8_t white = 16 + 5 * 1 + 5 * 6 + 5 * 36;
				static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
				static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;

				Cookie* c = (Cookie*)cookie;
				AnsiCell* ar = c->ptr + c->x + c->width * (c->y - dy);
				for (int i=0; i<len; i++)
				{
					if (str[i] == '\n')
					{
						// TODO: 
						// fill till span with spaces!
						// ...
						for (int x = dx + i; x < c->span; x++)
						{
							AnsiCell* ac = ar + x;
							ac->fg = white;
							ac->bk = dk_grey;
							ac->gl = ' ';
							ac->spare = 0;
						}
						c->rows++;
						break;
					}
					AnsiCell* ac = ar + i + dx;
					ac->fg = white;
					ac->bk = dk_grey;
					ac->gl = str[i];
					ac->spare = 0;
				}
			}
		};

		int w = (size[0]+1); // +1 for left margin, |1 to make it odd
		
		Cookie cookie = { this, ptr, width, height, x - w/2 +1, y + size[1]+2, w, 0 };
		int bl = Reflow(0, 0, Cookie::Print, &cookie);
		assert(bl >= 0);

		AnsiCell* ll = ptr + x - w / 2 - 1 + (y + 1) * width;
		AnsiCell* bc = ptr + x + y * width;
		AnsiCell* lr = ptr + x + w / 2 + 1 + (y + 1) * width;
		AnsiCell* ul = ptr + x - w / 2 - 1 + (y + 4 + size[1]) * width;
		AnsiCell* ur = ptr + x + w / 2 + 1 + (y + 4 + size[1]) * width;

		static const uint8_t black = 16;
		static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;
		static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;

		bc->bk = black;
		bc->fg = lt_grey;
		bc->gl = 179;

		bc += width;

		bc->bk = black;
		bc->fg = lt_grey;
		bc->gl = 194;

		ll->bk = black;
		ll->fg = lt_grey;
		ll->gl = 192;

		lr->bk = black;
		lr->fg = lt_grey;
		lr->gl = 217;

		ul->bk = black;
		ul->fg = lt_grey;
		ul->gl = 218;

		ur->bk = black;
		ur->fg = lt_grey;
		ur->gl = 191;

		for (int i = 1; i <= w / 2; i++)
		{
			ll[i].bk = black;
			ll[i].fg = lt_grey;
			ll[i].gl = 196;
			lr[-i].bk = black;
			lr[-i].fg = lt_grey;
			lr[-i].gl = 196;

			ul[i].bk = black;
			ul[i].fg = lt_grey;
			ul[i].gl = 196;
			ur[-i].bk = black;
			ur[-i].fg = lt_grey;
			ur[-i].gl = 196;
		}

		bc += width*(1+size[1]+2);

		bc->bk = black;
		bc->fg = lt_grey;
		bc->gl = 196;

		ll += width;
		lr += width;

		for (int i = 2; i < w+2; i++)
		{
			ll[i].bk = dk_grey;
			ll[i].fg = black;
			ll[i].gl = ' ';
		}

		for (int i = 0; i < size[1]+2; i++)
		{
			ll->bk = black;
			ll->fg = lt_grey;
			ll->gl = 179;
			ll[1].bk = dk_grey;
			ll[1].fg = black;
			ll[1].gl = ' ';

			lr->bk = black;
			lr->fg = lt_grey;
			lr->gl = 179;

			ll += width;
			lr += width;
		}

		ll -= width;

		for (int i = 2; i < w+2; i++)
		{
			ll[i].bk = dk_grey;
			ll[i].fg = black;
			ll[i].gl = ' ';
		}
	}

	void MoveCursor(int dx, int dy)
	{
		// update cursor pos
		cursor_xy[0] += dx;
		cursor_xy[1] += dy;

		if (cursor_xy[0] < 0)
			cursor_xy[0] = 0;
		if (cursor_xy[1] < 0)
			cursor_xy[1] = 0;

		if (cursor_xy[1] >= size[1])
			cursor_xy[1] = size[1] - 1;

		int bl = Reflow(0, 0, false);

		assert(bl >= 0);

		if (cursor_xy[0] > (bl & 0xFF))
			cursor_xy[0] = bl & 0xFF;

		cursor_pos = (bl >> 8) + cursor_xy[0];
	}
	
	bool Input(int ch)
	{
		// insert / delete char, update size and cursor pos

		if (ch == 127)
		{
			if (cursor_pos > 0)
			{
				if (cursor_pos < len)
					memmove(buf + cursor_pos - 1, buf + cursor_pos, len - cursor_pos);
				cursor_pos--;
				len--;

				int _size[2], _pos[2];
				int bl = Reflow(_size, _pos, false);

				assert(bl >= 0);

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
				if (Reflow(_size, _pos, false) >= 0)
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
	// where l = cursor_xy[1] line length and b = cursor_pos at cursor_xy[1] line begining
	int Reflow(int* _size, int* _pos, void (*print)(int x, int y, const char* str, int len, void* cookie)=0, void* cookie=0) const
	{
		int x = 0, y = 0;
		int cx = 0, cy = 0;
		int wordlen = 0;

		int w = 0, l = 0, b = 0;

		for (int c = 0; c < len; c++)
		{
			assert(x < max_width);

			if (c == cursor_pos)
			{
				cx = x;
				cy = y;
			}

			// distinguish from regular character, space and line breaker
			if (buf[c] == ' ')
			{
				if (print)
				{
					/*
					for (int i=c-wordlen; i<c; i++)
						printf("%c", buf[i]);
					printf("~");
					*/
					print(x - wordlen, y, buf + c - wordlen, wordlen+1, cookie); // +1 to include space char
				}

				wordlen = 0;
				x++;

				if (x > w)
				{
					w = x;
					if (y == cursor_xy[1])
						l = w;
				}

				if (x == max_width)
				{
					if (print)
					{
						//printf("\n");
						print(x, y, "\n", 1, cookie);
					}
					x = 0;
					y++;
					if (y == cursor_xy[1])
						b = c+1;
					if (y == max_height && max_height)
						return -1;
				}
			}
			else
			if (buf[c] == '\n')
			{
				// treat it as the last word's char!

				if (print)
				{
					/*
					for (int i = c - wordlen; i < c; i++)
						printf("%c", buf[i]);
					printf("@\n");
					*/
					print(x - wordlen, y, buf + c - wordlen, wordlen+1, cookie); // including '\n'
				}

				wordlen = 0;
				x = 0;
				y++;
				if (y == max_height && max_height)
					return -1;
				if (y == cursor_xy[1])
					b = c+1;
				if (x > w) // too late, x==0 !!!
				{
					w = x;
					if (y == cursor_xy[1])
						l = w;
				}
			}
			else
			{
				x++;
				wordlen++;
				if (x == max_width)
				{
					if (x > wordlen)
					{
						if (print)
						{
							//printf("\n");
							print(x - wordlen, y, "\n", 1, cookie);
						}

						c -= wordlen; // retry with word beggining on next line
					}
					else
					{
						w = max_width;
						if (print)
						{
							/*
							for (int i = c - (wordlen-1); i <= c; i++)
								printf("%c", buf[i]);
							*/

							print(0, y, buf + c - (wordlen-1), wordlen, cookie);

							// no '\n' for fully filled line
						}
					}
				
					y++;
					if (y == max_height && max_height)
						return -1;
					x = 0;
					if (y == cursor_xy[1])
						b = c+1;
					wordlen = 0;
				}
			}
		}

		if (print)
		{
			/*
			for (int i = len - wordlen; i < len; i++)
				printf("%c", buf[i]);
			printf("\n");
			*/
			print(x - wordlen, y, buf + len - wordlen, wordlen, cookie);
			print(x, y, "\n", 1, cookie);
		}

		if (wordlen && x > w)
		{
			w = x;
			if (y == cursor_xy[1])
				l = w;
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

		return (b<<8) | l;
	}

	char buf[256];
};

TalkBox talk_box;

int Test()
{
	talk_box.cursor_pos = 0;
	talk_box.max_width = 33; // MAKE IT ODD!!!
	talk_box.max_height = 0; // off
	talk_box.show_cursor = false;

	// char demo[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";
	char demo[] = "Lorem ipsum! fucking donor.";
	talk_box.len = strlen(demo);
	memcpy(talk_box.buf, demo, talk_box.len);

	return talk_box.Reflow(talk_box.size, talk_box.cursor_xy);
}

int test_passed = Test();

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
		static const uint8_t white = 16 + 5 * 1 + 5 * 6 + 5 * 36;
		static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;
		static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
		static const uint8_t black = 16 + 0;

		static const uint8_t lt_red = 16 + 1 * 1 + 1 * 6 + 5 * 36;
		static const uint8_t dk_red = 16 + 0 * 1 + 0 * 6 + 3 * 36;

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
		for (; dx < w - 1; dx++)
			if (dx>=dx_lo && dx<dx_hi)
				top[dx] = bevel[b][0];
		if (dx >= dx_lo && dx < dx_hi)
			top[dx] = bevel[b][1];

		int cap_index = 0;
		for (int i = 0; i <= width_mul; i++)
			if (cap[i][0])
				cap_index = i;
			else
				break;

		const char* upper_cap = cap[cap_index][0];
		const char* lower_cap = cap[cap_index][1];

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

		if (0 >= dx_lo && 0 < dx_hi)
			bottom[0] = bevel[b][1];
		dx = 1;
		for (; dx < w; dx++)
			if (dx >= dx_lo && dx < dx_hi)
				bottom[dx] = bevel[b][2];

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
							*ch = 127;
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

		static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
		static const uint8_t black = 16 + 0;

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

void Game::ScreenToCell(int p[2]) const
{
	p[0] = (2*p[0] - input.size[0] + render_size[0] * font_size[0]) / (2 * font_size[0]);
	p[1] = (input.size[1] - 2*p[1] + render_size[1] * font_size[1]) / (2 * font_size[1]);
}

void Game::Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
	render_size[0] = width;
	render_size[1] = height;

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
	if (!show_keyb)
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
	if (!show_keyb)
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

	if (show_keyb)
	{
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
		keyb_pos[1] = 1;
		keyb_mul = mul;

		uint8_t key[32];
		for (int i = 0; i < 32; i++)
			key[i] = keyb_key[i] | input.key[i];
		keyb.Paint(ptr, keyb_pos[0], keyb_pos[1], width, height, keyb_mul, key);
	}

	talk_box.Paint(ptr, width, height, width / 2, height / 2 + 8, false);

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

void Game::OnSize(int w, int h, int fw, int fh)
{
	memset(&input, 0, sizeof(Input));
	input.size[0] = w;
	input.size[1] = h;
	font_size[0] = fw;
	font_size[1] = fh;
}

void Game::OnKeyb(GAME_KEYB keyb, int key)
{
	// handle layers first ...

	// if nothing focused 
	if (keyb == GAME_KEYB::KEYB_DOWN)
	{
		if (key == A3D_TAB)
		{
			if (show_keyb)
				memset(keyb_key, 0, 32);
			show_keyb = !show_keyb;
		}
		if (key == A3D_ESCAPE && show_keyb)
		{
			if (show_keyb)
				memset(keyb_key, 0, 32);
			show_keyb = false;
		}
		if (key == A3D_F10)
			input.shot = true;
		if (key == A3D_SPACE && !show_keyb)
			input.jump = true;
		input.key[key >> 3] |= 1 << (key & 0x7);
	}
	else
	if (keyb == GAME_KEYB::KEYB_UP)
		input.key[key >> 3] &= ~(1 << (key & 0x7));
	else
	if (keyb == GAME_KEYB::KEYB_CHAR)
	{
		if (key != 9) // we skip all TABs
		{
			// if type box is visible pass this input to it
			printf("CH:%d (%c)\n", key, key);
		}
	}
	else
	if (keyb == GAME_KEYB::KEYB_PRESS)
	{
		// it is like a KEYB_CHAR (not producing releases) but for non-printable keys
		// main input from terminals 
		// ....

		/* code */
	}
}

void Game::OnMouse(GAME_MOUSE mouse, int x, int y)
{
	// handle layers first ...

	// if nothing focused
	switch (mouse)
	{
		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 

			if (show_keyb && !input.drag)
			{
				int cp[2] = { x,y };
				ScreenToCell(cp);

				bool shift_on = ((input.key[A3D_LSHIFT >> 3] | keyb_key[A3D_LSHIFT >> 3]) & (1 << (A3D_LSHIFT & 7))) != 0;
				char ch=0;
				int cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, &ch, shift_on);

				if (cap > 0)
				{
					if (cap == A3D_LSHIFT)
					{
						keyb_key[cap >> 3] ^= 1 << (cap & 7);  // toggle shift
					}
					else
					{
						if (ch)
							OnKeyb(GAME_KEYB::KEYB_CHAR, ch); // like from terminal!
						keyb_key[cap >> 3] |= 1 << (cap & 7);  // just to hilight keycap
					}
					keyb_cap[10/*mouse_touch_id*/] = cap;

					// setup autorepeat initial delay>
					// ...
				}
				else
				// negative cap -> hit outside entire keyboard
				if (cap < 0)
				{
					if (input.but == 0)
					{
						input.drag = 1;
						input.drag_from[0] = x;
						input.drag_from[1] = y;
					}
				}
			}
			else
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

	if (mouse != MOUSE_LEFT_BUT_DOWN)
	{
		if (show_keyb)
		{
			if (keyb_cap[10/*mouse_touch_id*/])
			{
				if (mouse == MOUSE_LEFT_BUT_UP)
				{
					int uncap = keyb_cap[10/*mouse_touch_id*/];
					if (uncap!=A3D_LSHIFT)
						keyb_key[uncap >> 3] &= ~(1 << (uncap & 7));  // un-hilight keycap
					keyb_cap[10/*mouse_touch_id*/] = 0;
				}
				else
				{
					int cp[2] = { x,y };
					ScreenToCell(cp);
					int cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, 0,false);
					if (cap != keyb_cap[10/*mouse_touch_id*/])
					{
						int uncap = keyb_cap[10/*mouse_touch_id*/];
						if (uncap != A3D_LSHIFT)
							keyb_key[uncap >> 3] &= ~(1 << (uncap & 7));  // un-hilight keycap
						keyb_cap[10/*mouse_touch_id*/] = 0;
					}
				}
			}
		}
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
