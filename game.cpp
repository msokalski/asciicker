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

	int cache_bl; // cache cursor's line offset and length

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
		assert(bl >= 0);
		assert(bl == cache_bl);

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

	void MoveCursorX(int dx)
	{
		if (dx < 0 && cursor_pos>0 || dx > 0 && cursor_pos < len)
		{
			cursor_pos += dx;
			if (cursor_pos < 0)
				cursor_pos = 0;
			if (cursor_pos > len)
				cursor_pos = len;

			// if jumped to another line then reflow
			if (cursor_pos < (cache_bl >> 8) || cursor_pos >= (cache_bl & 0xFF))
			{
				int _pos[2];
				int bl = Reflow(0, _pos); // pos is given -> calc bl for cursor_pos instead of cursor_xy[1]

				assert(bl >= 0);
				cache_bl = bl; // problem: bl is calculated for OLD cursor_xy[1]! it should be calculated for cursor_pos!

				cursor_xy[0] = _pos[0];
				cursor_xy[1] = _pos[1];

				/*
				// work around: update it once again after setting NEW cursor_xy[1]
				bl = Reflow(0, _pos);
				cache_bl = bl;
				*/
			}
			else
			{
				cursor_xy[0] += dx;
			}
		}
	}

	void MoveCursorY(int dy)
	{
		if (dy < 0 && cursor_xy[1]>0 || dy > 0 && cursor_xy[1] < size[1] - 1)
		{
			// update cursor pos
			cursor_xy[1] += dy;

			if (cursor_xy[1] < 0)
				cursor_xy[1] = 0;

			if (cursor_xy[1] >= size[1])
				cursor_xy[1] = size[1] - 1;

			int bl = Reflow(0, 0);
			assert(bl >= 0);
			cache_bl = bl; // here it is ok ( no pos is given )

			if (cursor_xy[0] >= (bl & 0xFF))
				cursor_xy[0] = (bl & 0xFF) - 1;

			cursor_pos = (bl >> 8) + cursor_xy[0];
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

			assert(bl >= 0);
			cache_bl = bl;

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

				assert(bl >= 0);
				cache_bl = bl;

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
					cache_bl = bl;
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
	int Reflow(int* _size, int* _pos, void (*print)(int x, int y, const char* str, int len, void* cookie)=0, void* cookie=0) const
	{
		int x = 0, y = 0;
		int cx = 0, cy = 0;
		int wordlen = 0;
					
		int w = 2, l = 0, b = 0;

		// todo:
		// actually we need to call print() only on y++ and last line!

		int temp_b = 0;
		int current_line = _pos ? -1 : cursor_xy[1];

		for (int c = 0; c < len; c++)
		{
			assert(x < max_width);

			if (x == 0)
				temp_b = c;

			if (c == cursor_pos)
			{
				cx = x;
				cy = y;

				if (current_line < 0)
					current_line = y;
			}

			if (y == current_line)
			{
				b = temp_b;
				l = x;
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

				if (y == current_line)
					l = x;


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
				if (y == current_line)
					l = x+1;

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
						w = max_width;
						if (y == current_line)
							l = max_width;

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
			if (x == 0)
				temp_b = len;

			if (len == cursor_pos)
			{
				cx = x;
				cy = y;

				if (current_line < 0)
					current_line = y;
			}

			if (y == current_line)
			{
				b = temp_b;
				l = x + 1; // like w
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

		return (b<<8) | l;
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
	// must be detached to do not interfere with meshprefs of asciiid
	player_0000 = LoadSprite("./sprites/player-0000.xp", "player_0000.xp", 0, true);
	wolfie_0011 = LoadSprite("./sprites/wolfie-0011.xp", "wolfie-0011.xp", 0, true);
	plydie_0000 = LoadSprite("./sprites/plydie-0000.xp", "plydie-0000.xp", 0, true);
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

	g->keyb_hide = keyb.hide;

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

void Game::Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
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
	if (!player.talk_box)
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
	if (!player.talk_box)
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
				player.talk_box->cache_bl = 1;
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
			}
		}

		if (key == A3D_F10 && !auto_rep)
			input.shot = true;
		if (key == A3D_SPACE && !player.talk_box && !auto_rep)
			input.jump = true;

		if (!auto_rep)
			input.key[key >> 3] |= 1 << (key & 0x7);

		// temp (also with auto_rep)
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
				player.talk_box->cache_bl = 1;
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

void Game::OnMouse(GAME_MOUSE mouse, int x, int y)
{
	// handle layers first ...

	// if nothing focused
	switch (mouse)
	{
		// they are handled
		// after switch !!!
		case MOUSE_MOVE:
		case MOUSE_WHEEL_DOWN:
		case MOUSE_WHEEL_UP:
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_DOWN: 
			player_hit = false;
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

					// setup autorepeat initial delay...
					// not for shift
					KeybAuroRepDelayStamp = stamp;
					KeybAutoRepChar = ch; // must be nulled on any real keyb input!
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

						// check player hit
						player_hit = PlayerHit(this, x, y);
					}
				}
			}
			else
			if (input.but == 0)
			{
				input.drag = 1;
				input.drag_from[0] = x;
				input.drag_from[1] = y;

				// check player hit
				player_hit = PlayerHit(this, x, y);
			}

			input.but |= 1; 
			break;

		case GAME_MOUSE::MOUSE_LEFT_BUT_UP: 

			if (player_hit)
			{
				player_hit = false;
				int down[2] = { input.drag_from[0], input.drag_from[1] };
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
					}
					else
					{
						// open talk_box (and keyb if not open)
						TalkBox_blink = 32;
						player.talk_box = (TalkBox*)malloc(sizeof(TalkBox));
						memset(player.talk_box, 0, sizeof(TalkBox));
						player.talk_box->max_width = 33;
						player.talk_box->max_height = 7; // 0: off
						player.talk_box->cache_bl = 1;
						int s[2],p[2];
						player.talk_box->Reflow(s,p);
						player.talk_box->size[0] = s[0];
						player.talk_box->size[1] = s[1];
						player.talk_box->cursor_xy[0] = p[0];
						player.talk_box->cursor_xy[1] = p[1];
						show_keyb = true;
					}
				}
			}

			// if released at same pos as but_down, it is click. then:
			//   if pos is on player:
			//      if talkbox is open: hide talkbox ( & keyb if open )
			//         otherwise:       show talkbox ( & keyb if not open )
			//      (keyb cannot be open without talkbox... at the moment)

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
					KeybAutoRepChar = 0;
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
						KeybAutoRepChar = 0;
					}
				}
			}
		}
	}

	input.pos[0] = x;
	input.pos[1] = y;

	if (player_hit)
	{
		int down[2] = { input.drag_from[0], input.drag_from[1] };
		ScreenToCell(down);

		int up[2] = { x, y };
		ScreenToCell(up);

		up[0] -= down[0];
		up[1] -= down[1];

		if (up[0] * up[0] > 1 || up[1] * up[1] > 1)
		{
			player_hit = false;
		}
	}
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
