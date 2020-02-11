#include <string.h>
#include <math.h>
#include "game.h"
#include "platform.h"

static const int stand_us_per_frame = 30000;
static const int fall_us_per_frame = 30000;
static const int attack_us_per_frame = 20000;

struct TalkBox
{
	int max_width, max_height;
	int size[2];
	int cursor_xy[2];
	int cursor_pos;
	int len;

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
				Cookie* c = (Cookie*)cookie;
				if (c->y - dy < 0 || c->y - dy >= c->height)
					return;

				static const uint8_t white = 16 + 5 * 1 + 5 * 6 + 5 * 36;
				static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;
				static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;

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
							if (x < 0 || x >= c->width)
								continue;

							AnsiCell* ac = ar + x;
							ac->fg = white;							ac->bk = dk_grey;
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
		assert(bl >= 0);

		AnsiCell* ll = ptr + left + lower * width;
		AnsiCell* bc = ptr + center + bottom * width;
		AnsiCell* lr = ptr + right + lower * width;
		AnsiCell* ul = ptr + left + upper * width;
		AnsiCell* ur = ptr + right + upper * width;

		static const uint8_t black = 16;
		static const uint8_t lt_grey = 16 + 3 * 1 + 3 * 6 + 3 * 36;
		static const uint8_t dk_grey = 16 + 2 * 1 + 2 * 6 + 2 * 36;

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
			for (int i = left + 1; i < right; i++)
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

struct ACTION { enum
{
	NONE = 0, // IDLE/MOVE
	ATTACK,
	/*HIT,*/
	FALL,
	DEAD,
	STAND,
	SIZE
};};

struct WEAPON { enum
{
	NONE = 0,
	REGULAR_SWORD,
	SIZE
};};

struct SHIELD { enum
{
	NONE = 0,
	REGULAR_SHIELD,
	SIZE
};};

struct HELMET { enum
{
	NONE = 0,
	SIZE
};};

struct ARMOR { enum
{
	NONE = 0,
	SIZE
};};

struct MOUNT { enum
{
	NONE = 0,
	WOLF,
	SIZE
};};

Sprite* player[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* player_fall[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* player_attack[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* wolfie[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* wolfie_attack[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 };
Sprite* wolfie_fall[ARMOR::SIZE][HELMET::SIZE][SHIELD::SIZE][WEAPON::SIZE] = { 0 }; // todo

Sprite* wolf = 0;
Sprite* wolf_attack = 0; // todo
Sprite* wolf_fall = 0;   // todo

Sprite* player_naked = 0; // what to do?

void LoadSprites()
{
#ifdef _WIN32
	_set_printf_count_output(1);
#endif

	for (int a = 0; a < ARMOR::SIZE; a++)
	{
		for (int h = 0; h < HELMET::SIZE; h++)
		{
			for (int s = 0; s < SHIELD::SIZE; s++)
			{
				int name = 0;
				char path[64];

				for (int w = 0; w < WEAPON::SIZE; w++)
				{
					sprintf(path, "./sprites/%nplayer-%x%x%x%x.xp", &name, a, h, s, w);
					player[a][h][s][w] = LoadSprite(path, path+name, 0, false);

					sprintf(path, "./sprites/%nplydie-%x%x%x%x.xp", &name, a, h, s, w);
					player_fall[a][h][s][w] = LoadSprite(path, path + name, 0, false);

					sprintf(path, "./sprites/%nwolfie-%x%x%x%x.xp", &name, a, h, s, w);
					wolfie[a][h][s][w] = LoadSprite(path, path + name, 0, false);

					wolfie_fall[a][h][s][w] = 0;
				}

				player_attack[a][h][s][WEAPON::NONE] = 0;
				wolfie_attack[a][h][s][WEAPON::NONE] = 0;
				for (int w = 1; w < WEAPON::SIZE; w++)
				{
					sprintf(path, "./sprites/%nattack-%x%x%x%x.xp", &name, a, h, s, w);
					player_attack[a][h][s][w] = LoadSprite(path, path + name, 0, false);

					sprintf(path, "./sprites/%nwolack-%x%x%x%x.xp", &name, a, h, s, w);
					wolfie_attack[a][h][s][w] = LoadSprite(path, path + name, 0, false);
				}
			}
		}
	}
}

Sprite* GetSprite(const SpriteReq* req)
{
	assert(req);

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
					return player[req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::ATTACK:
					return player_attack[req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::FALL:
				case ACTION::DEAD:
				case ACTION::STAND:
					return player_fall[req->armor][req->helmet][req->shield][req->weapon];
			}
			return 0;
		}

		case MOUNT::WOLF:
		{
			switch (req->action)
			{
				case ACTION::NONE:
					return wolfie[req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::ATTACK:
					return wolfie_attack[req->armor][req->helmet][req->shield][req->weapon];

				case ACTION::FALL:
				case ACTION::DEAD:
				case ACTION::STAND:
					return wolfie_fall[req->armor][req->helmet][req->shield][req->weapon];
			}	
			return 0;
		}
	}

	return 0;
}

void FreeSprites()
{
	if (wolf)
		FreeSprite(wolf);

	if (wolf_attack)
		FreeSprite(wolf_attack);

	if (wolf_fall)
		FreeSprite(wolf_fall);

	if (player_naked)
		FreeSprite(player_naked);

	for (int a = 0; a < ARMOR::SIZE; a++)
	{
		for (int h = 0; h < HELMET::SIZE; h++)
		{
			for (int s = 0; s < SHIELD::SIZE; s++)
			{
				for (int w = 0; w < WEAPON::SIZE; w++)
				{
					if (player[a][h][s][w])
						FreeSprite(player[a][h][s][w]);
					if (player_fall[a][h][s][w])
						FreeSprite(player_fall[a][h][s][w]);
					if (wolfie[a][h][s][w])
						FreeSprite(wolfie[a][h][s][w]);
					if (wolfie_fall[a][h][s][w])
						FreeSprite(wolfie_fall[a][h][s][w]);
					if (player_attack[a][h][s][w])
						FreeSprite(player_attack[a][h][s][w]);
					if (wolfie_attack[a][h][s][w])
						FreeSprite(wolfie_attack[a][h][s][w]);
				}
			}
		}
	}
}

Game* CreateGame(int water, float pos[3], float yaw, float dir, uint64_t stamp)
{
	// load defaults
	Game* g = (Game*)malloc(sizeof(Game));
	memset(g, 0, sizeof(Game));

	g->keyb_hide = keyb.hide;

	g->renderer = CreateRenderer(stamp);
	g->physics = CreatePhysics(terrain, world, pos, dir, yaw, stamp);
	g->stamp = stamp;

	// init player!
	g->player.req.mount = MOUNT::NONE;
	g->player.req.armor = ARMOR::NONE;
	g->player.req.helmet = HELMET::NONE;
	g->player.req.shield = SHIELD::REGULAR_SHIELD;
	g->player.req.weapon = WEAPON::REGULAR_SWORD;
	g->player.req.action = ACTION::NONE;

	g->player.sprite = GetSprite(&g->player.req);
	g->player.anim = 0; // ???

	g->player.frame = 0;

	g->water = water;
	g->prev_yaw = yaw;

	return g;
}

void DeleteGame(Game* g)
{
	if (g)
	{
		if (g->player.talk_box)
			free(g->player.talk_box);
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
	p[1] = (input.size[1]-1 - 2*p[1] + render_size[1] * font_size[1]) / (2 * font_size[1]);
}

bool Human::SetActionNone(uint64_t stamp)
{
	if (req.action == ACTION::NONE)
		return true;
	int old = req.action;
	req.action = ACTION::NONE;

	Sprite* spr = GetSprite(&req);
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

bool Human::SetActionAttack(uint64_t stamp)
{
	if (req.action == ACTION::ATTACK)
		return true;
	if (req.action == ACTION::FALL || 
		req.action == ACTION::STAND || 
		req.action == ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::ATTACK;

	Sprite* spr = GetSprite(&req);
	if (!spr)
	{
		req.action = old;
		return false;
	}
	sprite = spr;

	anim = 0;
	frame = 2;
	action_stamp = stamp;
	return true;
}

bool Human::SetActionStand(uint64_t stamp)
{
	if (req.action == ACTION::STAND)
		return true;

	if (req.action != ACTION::FALL && req.action != ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::STAND;

	Sprite* spr = GetSprite(&req);
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

bool Human::SetActionFall(uint64_t stamp)
{
	if (req.action == ACTION::FALL)
		return true;

	if (req.action == ACTION::DEAD)
		return false;

	int old = req.action;
	req.action = ACTION::FALL;

	Sprite* spr = GetSprite(&req);
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

bool Human::SetActionDead(uint64_t stamp)
{
	int old = req.action;
	req.action = ACTION::DEAD;

	Sprite* spr = GetSprite(&req);
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

	Sprite* spr = GetSprite(&req);
	if (!spr)
	{
		req.weapon = old;
		return false;
	}
	sprite = spr;
}

bool Human::SetShield(int s)
{
	if (s == req.shield)
		return true;

	int old = req.shield;
	req.shield = s;

	Sprite* spr = GetSprite(&req);
	if (!spr)
	{
		req.shield = old;
		return false;
	}
	sprite = spr;
}

bool Human::SetHelmet(int h)
{
	if (h == req.helmet)
		return true;

	int old = req.helmet;
	req.helmet = h;

	Sprite* spr = GetSprite(&req);
	if (!spr)
	{
		req.helmet = old;
		return false;
	}
	sprite = spr;
}

bool Human::SetArmor(int a)
{
	if (a == req.armor)
		return true;

	int old = req.armor;
	req.armor = a;

	Sprite* spr = GetSprite(&req);
	if (!spr)
	{
		req.armor = old;
		return false;
	}
	sprite = spr;
}

bool Human::SetMount(int m)
{
	if (req.action != ACTION::NONE)
		return false;

	if (m == req.mount)
		return true;
	int old = req.mount;
	req.mount = m;

	Sprite* s = GetSprite(&req);
	if (!s)
	{
		req.mount = old;
		return false;
	}
	sprite = s;
}

void Game::Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
	if (_stamp-stamp > 500000) // treat lags longer than 0.5s as stall
		stamp = _stamp;

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
					io.x_force = 2 * (input.contact[i].pos[0] * 2 - input.size[0]) / (float)input.size[0];
					io.y_force = 2 * (input.size[1] - input.contact[i].pos[1] * 2) / (float)input.size[1];					
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
					SetPhysicsYaw(physics, yaw, 0);

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
			io.x_force = (int)(input.IsKeyDown(A3D_RIGHT) || input.IsKeyDown(A3D_D)) - (int)(input.IsKeyDown(A3D_LEFT) || input.IsKeyDown(A3D_A));
			io.y_force = (int)(input.IsKeyDown(A3D_UP) || input.IsKeyDown(A3D_W)) - (int)(input.IsKeyDown(A3D_DOWN) || input.IsKeyDown(A3D_S));

			float len = sqrtf(io.x_force*io.x_force + io.y_force*io.y_force);
			if (len > 0)
				speed /= len;
			io.x_force *= speed;
			io.y_force *= speed;
		}

		if (!torque_handled)
		{
			io.torque = (int)(input.IsKeyDown(A3D_DELETE) || input.IsKeyDown(A3D_PAGEUP) || input.IsKeyDown(A3D_F1) || input.IsKeyDown(A3D_Q)) -
				(int)(input.IsKeyDown(A3D_INSERT) || input.IsKeyDown(A3D_PAGEDOWN) || input.IsKeyDown(A3D_F2) || input.IsKeyDown(A3D_E));
		}
	}

	io.jump = input.jump;

	if (player.req.action == ACTION::FALL || 
		player.req.action == ACTION::STAND ||
		player.req.action == ACTION::DEAD)
	{
		io.x_force = 0;
		io.y_force = 0;
		io.jump = false;
	}

	int steps = Animate(physics, _stamp, &io);
	if (steps > 0)
		input.jump = false;

	prev_yaw = io.yaw;

	player.pos[0] = io.pos[0];
	player.pos[1] = io.pos[1];
	player.pos[2] = io.pos[2];

	switch (player.req.action)
	{
		case ACTION::ATTACK:
		{
			static const int frames[] = { 2,2,2,1,1,1,0,0,0,0, 0,1,1,2,2,3,3,4,4,4,4,4,4, 3,3,3, 2,2,2 };
			int frame_index = (_stamp - player.action_stamp) / attack_us_per_frame;
			assert(frame_index >= 0);
			if (frame_index >= sizeof(frames) / sizeof(int))
				player.SetActionNone(_stamp);
			else
				player.frame = frames[frame_index];
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

	::Render(renderer, _stamp, terrain, world, water, 1.0, io.yaw, io.pos, lt,
		width, height, ptr, player.sprite, player.anim, player.frame, player.dir);

	if (player.talk_box)
		player.talk_box->Paint(ptr, width, height, width / 2, height / 2 + 8, (TalkBox_blink & 63) < 32);

	if (show_keyb || keyb_hide < 1 + 5 * 5 + 1)
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
		keyb_pos[1] = 1 - keyb_hide;
		keyb_mul = mul;

		uint8_t key[32];
		for (int i = 0; i < 32; i++)
			key[i] = keyb_key[i] | input.key[i];
		keyb.Paint(ptr, keyb_pos[0], keyb_pos[1], width, height, keyb_mul, key);
	}
	
	if (show_keyb)
	{
		if (keyb_hide > 0)
		{
			keyb_hide -= f120;
			if (keyb_hide < 0)
				keyb_hide = 0;
		}
	}
	else
	{
		if (keyb_hide < keyb.hide)
		{
			keyb_hide += f120;
			if (keyb_hide > keyb.hide)
				keyb_hide = keyb.hide;
		}
	}

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

	// in case it comes from the real keyboard
	// if emulated, theoretically caller must revert it
	// but in practice it will reset it later to emulated cap
	KeybAutoRepChar = 0;

	if (keyb == GAME_KEYB::KEYB_DOWN)
	{
		bool auto_rep = (key & A3D_AUTO_REPEAT) != 0;
		key &= ~A3D_AUTO_REPEAT;

		if ((key == A3D_TAB || key == A3D_ESCAPE) && !auto_rep)
		{
			if (!player.talk_box && key == A3D_TAB)
			{
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

		if (key == A3D_F10 && !auto_rep)
			input.shot = true;

		if (!player.talk_box && !auto_rep)
		{
			if (key == A3D_SPACE)
				input.jump = true;
			if (key == A3D_ENTER)
				player.SetActionAttack(stamp);
			if (key == A3D_HOME)
				player.SetActionStand(stamp);
			if (key == A3D_END)
				player.SetActionFall(stamp);
			if (key == A3D_1)
				player.SetMount(!player.req.mount);
			if (key == A3D_2)
				player.SetWeapon(!player.req.weapon);
			if (key == A3D_3)
				player.SetShield(!player.req.shield);
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
		}
	}
	else
	if (keyb == GAME_KEYB::KEYB_UP)
		input.key[key >> 3] &= ~(1 << (key & 0x7));
	else
	if (keyb == GAME_KEYB::KEYB_CHAR)
	{
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
			if (key == A3D_LEFT)
				player.talk_box->MoveCursorX(-1);
			if (key == A3D_RIGHT)
				player.talk_box->MoveCursorX(+1);
			if (key == A3D_UP)
				player.talk_box->MoveCursorY(-1);
			if (key == A3D_DOWN)
				player.talk_box->MoveCursorY(+1);
		}
		else
		{
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
				if (key != A3D_TAB)
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
			if (!player.talk_box)
			{
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

	if (show_keyb)
	{
		bool shift_on = ((input.key[A3D_LSHIFT >> 3] | keyb_key[A3D_LSHIFT >> 3]) & (1 << (A3D_LSHIFT & 7))) != 0;
		char ch=0;
		cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, &ch, shift_on);

		if (b!=1 && cap > 0)
			cap = 0;

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
				keyb_key[cap >> 3] ^= 1 << (cap & 7);  // toggle shift
			}
			else
			{
				if (ch)
					OnKeyb(GAME_KEYB::KEYB_CHAR, ch); // like from terminal!
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
			// check if on player / talkbox
			hit = PlayerHit(this, x, y);
			if (hit)
				con->action = Input::Contact::PLAYER;
			else
			if (id>0 && cp[0] < 5 &&
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
						input.jump = true;
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

	con->drag = b;

	con->pos[0] = x;
	con->pos[1] = y;
	con->drag_from[0] = x;
	con->drag_from[1] = y;

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
			int cap = keyb.GetCap(cp[0] - keyb_pos[0], cp[1] - keyb_pos[1], keyb_mul, 0,false);
			if (cap != con->keyb_cap)
			{
				con->action = Input::Contact::NONE;
				
				int uncap = con->keyb_cap;
				if (uncap != A3D_LSHIFT)
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

	switch (con->action)
	{
		case Input::Contact::KEYBCAP:
		{
			// maybe we should clear it also when another cap is pressed?
			if (con->keyb_cap!=A3D_LSHIFT)
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
				{
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
		case MOUSE_WHEEL_UP:
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 
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
					input.jump = true;
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
				if (input.contact[id].keyb_cap!=A3D_LSHIFT)
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
		memset(&input, 0, sizeof(Input));
		input.size[0] = w;
		input.size[1] = h;
	}
}
