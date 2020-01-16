
// here we're gonna define sprite
// it must havew:
// - one or more frames, each containing:
//   - one or more direction views, each with reflection image

#include "string.h"

#include "sprite.h"
#include "upng.h"

static Sprite* sprite_head = 0;
static Sprite* sprite_tail = 0;

struct SpriteInst
{
	Sprite* sprite;
	int pos[3]; // ?

	int anim;
	int anim_time; // ?
};

/*
Sprite* LoadPlayerSword(const char* path);
Sprite* LoadPlayerShield(const char* path);
Sprite* LoadPlayerSwordShield(const char* path);
Sprite* LoadWolf(const char* path);
Sprite* LoadWolfPlayer(const char* path);
Sprite* LoadWolfPlayerSword(const char* path);
Sprite* LoadWolfPlayerShield(const char* path);
Sprite* LoadWolfPlayerSwordShield(const char* path);
*/

Sprite* LoadPlayer(const char* path)
{
	uint8_t recolor[] = 
	{
		1, // num of colors 
		170,0,170, 170,0,0, // purple->red shirt
	};
	Sprite* s = LoadSprite(path, "player", true, recolor);

	if (s)
	{
		// detach from sprite list

		if (s->prev)
			s->prev->next = s->next;
		else
			sprite_head = s->next;

		if (s->next)
			s->next->prev = s->prev;
		else
			sprite_tail = s->prev;

		s->prev = 0;
		s->next = 0;
	}

	return s;
}

void FreeSprite(Sprite* spr)
{
	if (spr->prev)
		spr->prev->next = spr->next;
	else
	if (spr == sprite_head) // ensure it is not detached sprite from sprite list
		sprite_head = spr->next;

	if (spr->next)
		spr->next->prev = spr->prev;
	else
	if (spr == sprite_tail) // ensure it is not detached sprite from sprite list
		sprite_tail = spr->prev;

	for (int f = 0; f < spr->frames; f++)
		free(spr->atlas[f].cell);

	free(spr->atlas);

	for (int a = 0; a < spr->anims; a++)
		free(spr->anim[a].frame_idx);

	if (spr->name)
		free(spr->name);

	free(spr);
}

Sprite* GetFirstSprite()
{
	return sprite_head;
}

Sprite* GetPrevSprite(Sprite* s)
{
	if (!s)
		return 0;
	return s->prev;
}

Sprite* GetNextSprite(Sprite* s)
{
	if (!s)
		return 0;
	return s->next;
}

int GetSpriteName(Sprite* s, char* buf, int size)
{
	if (!s)
	{
		if (buf && size > 0)
			*buf = 0;
		return 0;
	}

	int len = (int)strlen(s->name);

	if (buf && size > 0)
		strncpy(buf, s->name, size);

	return len + 1;
}

void SetSpriteCookie(Sprite* s, void* cookie)
{
	if (s)
		s->cookie = cookie;
}

void* GetSpriteCookie(Sprite* s)
{
	if (!s)
		return 0;
	return s->cookie;
}

Sprite* LoadSprite(const char* path, const char* name, bool has_refl, const uint8_t* recolor)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return 0;

	/////////////////////////////////
	// GZ INTRO:

	struct GZ
	{
		uint8_t id1, id2, cm, flg;
		uint8_t mtime[4];
		uint8_t xfl, os;
	};

	GZ gz;
	fread(&gz, 10, 1, f);

	/*
	assert(gz.id1 == 31 && gz.id2 == 139 && "gz identity");
	assert(gz.cm == 8 && "deflate method");
	*/

	if (gz.id1 != 31 || gz.id2 != 139 || gz.cm != 8)
	{
		fclose(f);
		return 0;
	}

	if (gz.flg & (1 << 2/*FEXTRA*/))
	{
		int hi, lo;
		fread(&hi, 1, 1, f);
		fread(&lo, 1, 1, f);

		int len = (hi << 8) | lo;
		fseek(f, len, SEEK_CUR);
	}

	if (gz.flg & (1 << 3/*FNAME*/))
	{
		uint8_t ch;
		do
		{
			ch = 0;
			fread(&ch, 1, 1, f);
		} while (ch);
	}

	if (gz.flg & (1 << 4/*FCOMMENT*/))
	{
		uint8_t ch;
		do
		{
			ch = 0;
			fread(&ch, 1, 1, f);
		} while (ch);
	}

	if (gz.flg & (1 << 1/*FFHCRC*/))
	{
		uint16_t crc;
		fread(&crc, 2, 1, f);
	}

	// deflated data blocks ...
	// read everything till end of file, trim tail by 8 bytes (crc32,isize)

	long now = ftell(f);
	fseek(f, 0, SEEK_END);

	unsigned long insize = ftell(f) - now - 8;
	unsigned char* in = (unsigned char*)malloc(insize);
	fseek(f, now, SEEK_SET);

	fread(in, 1, insize, f);

	void* out = u_inflate(in, insize);
	free(in);

	/////////////////////////////////
	// GZ OUTRO:

	uint32_t crc32, isize;
	fread(&crc32, 4, 1, f);
	fread(&isize, 4, 1, f);
	fclose(f);

	assert(out && isize == *(uint32_t*)out);

	/////////////////////////////////
	// CREATE SPRITE from XP
	// ...

	int layers = *((int*)out + 1);
	int width = *((int*)out + 2);
	int height = *((int*)out + 3);

	if (layers < 3 || width < 1 || height < 1)
	{
		u_inflate_free(out);
		return 0;
	}

#pragma pack(push,1)
	struct XPCell
	{
		uint32_t glyph;
		uint8_t fg[3];
		uint8_t bk[3];

		int GetDigit() const
		{
			int digit = -1;
			if (glyph >= '0' && glyph <= '9')
				digit = glyph - '0';
			else
			if (glyph >= 'A' && glyph <= 'Z')
				digit = glyph + 0xA - 'A';
			else
			if (glyph >= 'a' && glyph <= 'z')
				digit = glyph + 0xa - 'a';

			return digit;
		}
	};
#pragma pack(pop)

	int cells = width * height;
	XPCell* layer0 = (XPCell*)((int*)out + 4); // bg specifies color key
	XPCell* layer1 = (XPCell*)((int*)(layer0 + cells) + 2); // glyph specifies height + '0'
	XPCell* layer2 = (XPCell*)((int*)(layer1 + cells) + 2); // image map

	// merge layers above 2
	XPCell* merge = layer2;
	for (int m = 3; m < layers; m++)
	{
		merge = (XPCell*)((int*)(merge + cells) + 2);
		for (int c = 0; c < cells; c++)
			if (merge[c].bk[0] != 0xFF || merge[c].bk[1] != 0x00 || merge[c].bk[2] != 0xFF)
				layer2[c] = merge[c];
	}

	const int max_anims = 16;
	int projs = has_refl ? 2:1; // proj and refl
	int anims = 1;
	int anim_len[max_anims] = { 1 };
	int anim_sum = 1;
	int angles = layer0[0].GetDigit();
	if (angles > 0)
	{
		for (int a = 1; a < width; a++)
		{
			int len = layer0[height*a].GetDigit();
			if (len > 0)
			{
				anim_sum += len;
				anim_len[anims] = len;
				anims++;
			}
		}
	}
	else
		angles = 1;

	int fr_num_x = (projs * anim_sum);
	int fr_num_y = angles;

	int frames = fr_num_y * fr_num_x;
	Sprite::Frame* atlas = (Sprite::Frame*)malloc(sizeof(Sprite::Frame)*frames);

	int fr_width = width / fr_num_x;
	int fr_height = height / fr_num_y;

	int ref[2][3] =
	{
		{ fr_width,0,0 },
		{ fr_width,2*fr_height,0 },
	};

	if (height >= 2)
	{
		int y_proj = layer0[1+0].GetDigit();
		int y_refl = layer0[1+height].GetDigit();
		if (y_proj >= 0 && y_proj <= 2 * fr_height)
			ref[0][1] = y_proj;
		if (y_refl >= 0 && y_refl <= 2 * fr_height)
			ref[1][1] = 2 * fr_height - y_refl;
	}

	if (height >= 3)
	{
		int z_proj = layer0[2 + 0].GetDigit();
		int z_refl = layer0[2 + height].GetDigit();
		if (z_proj >= 0)
			ref[0][2] = -z_proj;
		if (z_refl >= 0)
			ref[1][2] = -z_refl;
	}

	for (int fr_y = 0; fr_y < fr_num_y; fr_y++)
	{
		for (int fr_x = 0; fr_x < fr_num_x; fr_x++)
		{
			Sprite::Frame* frame = atlas + fr_x + fr_y * fr_num_x;
			// alloc frame store
			// ...
			frame->width = fr_width;
			frame->height = fr_height;

			AnsiCell* c = (AnsiCell*)malloc(sizeof(AnsiCell)*fr_width*fr_height);
			frame->cell = c;

			frame->ref[0] = fr_width; // in half blocks! (means x-middle)

			int rgb_div;

			if (2 * fr_x < fr_num_x)
			{
				// proj:

				// PLAYER
				// frame->ref[1] = 2; // in half blocks!
				// frame->ref[2] = -1; // foot cell (spare=1) gets z = 0.5*dz/dy (half cell above reference)

				// WOLFIE
				//frame->ref[1] = 3;
				//frame->ref[2] = -2;

				frame->ref[0] = ref[0][0];
				frame->ref[1] = ref[0][1];
				frame->ref[2] = ref[0][2];
				rgb_div = 255;

			}
			else
			{
				// refl

				// PLAYER
				// frame->ref[1] = 2 * fr_height - 2; // in half blocks!
				// frame->ref[2] = -15; // foot cell (spare=7) gets z = -0.5*dz/dy (half cell below reference)

				// WOLFIE
				//frame->ref[1] = 2*fr_height -1;
				//frame->ref[2] = -17;

				frame->ref[0] = ref[1][0];
				frame->ref[1] = ref[1][1];
				frame->ref[2] = ref[1][2];
				rgb_div = 400;
			}

			int x0 = fr_x * fr_width, x1 = x0 + fr_width;
			int y0 = fr_y * fr_height, y1 = y0 + fr_height;
			for (int y = y1 - 1; y >= y0; y--)
			{
				for (int x = x0; x < x1; x++, c++)
				{
					int cell = x * height + y;
					XPCell* c0 = layer0 + cell;
					XPCell* c1 = layer1 + cell;
					XPCell* c2 = layer2 + cell;

					c->gl = c2->glyph;

					bool bk_transp = (c2->bk[0] == c0->bk[0] && c2->bk[1] == c0->bk[1] && c2->bk[2] == c0->bk[2]);
					bool fg_transp = (c2->fg[0] == c0->bk[0] && c2->fg[1] == c0->bk[1] && c2->fg[2] == c0->bk[2]);

					if (c2->bk[0] == 255 && c2->bk[1] == 0 && c2->bk[2] == 255)
					{
						// rexpaint transp
						bk_transp = true;
						fg_transp = true;
					}

					if (c1->glyph >= '0' && c1->glyph <= '9')
						c->spare = c1->glyph - '0';
					else
						if (c1->glyph >= 'A' && c1->glyph <= 'Z')
							c->spare = 10 + c1->glyph - 'A';
						else
							c->spare = 0xFF; // undefined

					if (bk_transp)
						c->bk = 255;
					else
					{
						if (recolor)
						{
							for (int i = 0; i < recolor[0]; i++)
							{
								int j = 1 + 6 * i;
								const uint8_t* re_src = recolor + j;
								const uint8_t* re_dst = re_src + 3;

								if (c2->bk[0] == re_src[0] &&
									c2->bk[1] == re_src[1] &&
									c2->bk[2] == re_src[2])
								{
									c2->bk[0] = re_dst[0];
									c2->bk[1] = re_dst[1];
									c2->bk[2] = re_dst[2];
								}
							}
						}

						int r = (c2->bk[0] * 5 + 128) / rgb_div;
						int g = (c2->bk[1] * 5 + 128) / rgb_div;
						int b = (c2->bk[2] * 5 + 128) / rgb_div;

						c->bk = 16 + 36 * r + g * 6 + b;
					}

					if (fg_transp)
						c->fg = 255;
					else
					{
						if (recolor)
						{
							for (int i = 0; i < recolor[0]; i++)
							{
								int j = 1 + 6 * i;
								const uint8_t* re_src = recolor + j;
								const uint8_t* re_dst = re_src + 3;

								if (c2->fg[0] == re_src[0] &&
									c2->fg[1] == re_src[1] &&
									c2->fg[2] == re_src[2])
								{
									c2->fg[0] = re_dst[0];
									c2->fg[1] = re_dst[1];
									c2->fg[2] = re_dst[2];
								}
							}
						}

						int r = (c2->fg[0] * 5 + 128) / rgb_div;
						int g = (c2->fg[1] * 5 + 128) / rgb_div;
						int b = (c2->fg[2] * 5 + 128) / rgb_div;
						c->fg = 16 + 36 * r + g * 6 + b;
					}
				}
			}
		}
	}

	Sprite* sprite = (Sprite*)malloc(sizeof(Sprite) + sizeof(Sprite::Anim));

	sprite->cookie = 0;
	sprite->angles = 8;
	sprite->anims = 2;
	sprite->atlas = atlas;
	sprite->frames = frames;

	sprite->anim[0].length = 1;
	sprite->anim[1].length = 8;

	for (int anim = 0; anim < sprite->anims; anim++)
		sprite->anim[anim].frame_idx = (int*)malloc(sizeof(int) * 2/*proj,refl*/ * sprite->angles * sprite->anim[anim].length);

	for (int refl = 0; refl < 2; refl++)
	{
		int rx = refl * fr_num_x / 2;
		for (int angl = 0; angl < sprite->angles; angl++)
		{
			int x = rx;
			int y = angl;
			for (int anim = 0; anim < sprite->anims; anim++)
			{
				for (int frame = 0; frame < sprite->anim[anim].length; frame++)
				{
					int idx = x + y * fr_num_x;
					sprite->anim[anim].frame_idx[(refl*sprite->angles + angl)*sprite->anim[anim].length + frame] = idx;
					x++;
				}
			}
		}
	}

	sprite->proj_bbox;

	/////////////////////////////////
	u_inflate_free(out);

	sprite->prev = sprite_tail;
	if (sprite_tail)
		sprite_tail->next = sprite;
	else
		sprite_head = sprite;
	sprite->next = 0;
	sprite_tail = sprite;

	if (name)
		sprite->name = strdup(name);
	else
		sprite->name = 0;

	return sprite;
}
