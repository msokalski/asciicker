
// here we're gonna define sprite
// it must havew:
// - one or more frames, each containing:
//   - one or more direction views, each with reflection image

#define _USE_MATH_DEFINES
#include <math.h>

#include <string.h>

#include "sprite.h"
#include "upng.h"

static Sprite* sprite_head = 0;
static Sprite* sprite_tail = 0;

struct SpriteInst
{
	Sprite* sprite;
	int pos[3]; // ?
	int anim;
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
		0
		/*
		1, // num of colors 
		170,0,170, 170,0,0, // purple->red shirt
		*/
	};



	Sprite* s = LoadSprite(path, "player", /*true,*/ recolor, true);

	/*
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
	*/

	return s;
}

void FreeSprite(Sprite* spr)
{
	assert(spr->refs>=1);

	if (spr->refs > 1)
	{
		spr->refs--;
		return;
	}

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

int RGB2PAL(const uint8_t* rgb)
{
	int r = (rgb[0] + 25) / 51;
	int g = (rgb[1] + 25) / 51;
	int b = (rgb[2] + 25) / 51;
	return 16 + 36 * r + 6 * g + b;
}

void PAL2RGB(int pal, uint8_t* rgb)
{
	pal -= 16;
	int r = pal / 36;
	pal -= r * 36;
	int g = pal / 6;
	pal -= g * 6;
	int b = pal;
	rgb[0] = r * 51;
	rgb[1] = g * 51;
	rgb[2] = b * 51;
}

int AverageGlyphTransp(const AnsiCell* ptr, int mask);

extern "C" void *tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

Sprite* LoadSprite(const char* path, const char* name, /*bool has_refl,*/ const uint8_t* recolor, bool detached)
{
	if (!detached && !recolor)
	{
		// first, lookup linked sprites, return pointer to already loaded one if found
		Sprite* s = GetFirstSprite();
		while (s)
		{
			if (strcmp(s->name, name) == 0)
			{
				s->refs++;
				return s;
			}
			s = s->next;
		}
	}

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


	size_t out_size=0;
	void* out = tinfl_decompress_mem_to_heap(in, insize, &out_size, 0);
	// void* out = u_inflate(in, insize);
	free(in);

	/////////////////////////////////
	// GZ OUTRO:

	uint32_t crc32, isize;
	fread(&crc32, 4, 1, f);
	fread(&isize, 4, 1, f);
	fclose(f);

	// assert(out && isize == *(uint32_t*)out);
	assert(out && isize == out_size);

	/////////////////////////////////
	// CREATE SPRITE from XP
	// ...

	int layers = *((int*)out + 1);
	int width = *((int*)out + 2);
	int height = *((int*)out + 3);

	if (layers < 3 || width < 1 || height < 1)
	{
		free(out);
		//u_inflate_free(out);
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
		{
			if (m == layers-1 && merge[c].fg[0] == 0 && merge[c].fg[1] == 255 && merge[c].fg[2] == 255)
			{
				bool fg_transp =
					layer2[c].fg[0] == layer0[c].bk[0] &&
					layer2[c].fg[1] == layer0[c].bk[1] &&
					layer2[c].fg[2] == layer0[c].bk[2];
				bool bk_transp =
					layer2[c].bk[0] == layer0[c].bk[0] &&
					layer2[c].bk[1] == layer0[c].bk[1] &&
					layer2[c].bk[2] == layer0[c].bk[2];

				if (layer2[c].bk[0] == 255 && layer2[c].bk[1] == 0 && layer2[c].bk[2] == 255)
				{
					fg_transp = true;
					bk_transp = true;
				}

				bool swoosh_bk_transp =
					merge[c].bk[0] == layer0[c].bk[0] &&
					merge[c].bk[1] == layer0[c].bk[1] &&
					merge[c].bk[2] == layer0[c].bk[2];

				// if bg is also swoosh, unify to full block
				if (merge[c].bk[0] == 0 && merge[c].bk[1] == 255 && merge[c].bk[2] == 255)
					merge[c].glyph = 219;

				int mask = 0;
				switch (merge[c].glyph)
				{
					case 0:
					case 32:
						if (merge[c].bk[0] != 255 || merge[c].bk[1] != 0 || merge[c].bk[2] != 255)
							layer2[c] = merge[c];
						break;

					case 220: // lower
						if (!mask)
							mask = 3;
					case 221: // left
						if (!mask)
							mask = 5;
					case 222: // right
						if (!mask)
							mask = 10;
					case 223: // upper
						if (!mask)
							mask = 12;

						if (swoosh_bk_transp)
						{
							// if halfblock with background transparent
							AnsiCell ac;
							ac.gl = layer2[c].glyph;
							ac.fg = fg_transp ? 255 : RGB2PAL(layer2[c].fg);
							ac.bk = bk_transp ? 255 : RGB2PAL(layer2[c].bk);

							// - calc 2 averages under swoosh fg and under swoosh bg
							int fg = AverageGlyphTransp(&ac, mask);
							int bk = AverageGlyphTransp(&ac, 0xf ^ mask);

							if (fg == 255)
							{
								// - if fg average is transparent set fg to swoosh color
								layer2[c].fg[0] = 0;
								layer2[c].fg[1] = 255;
								layer2[c].fg[2] = 255;
							}
							else
							{
								//   otherwise set fg to lighten fg average
								PAL2RGB(LightenColor(fg), layer2[c].fg);
							}

							if (fg == 255)
							{
								// - if bk average is transparent set bk to transparent
								layer2[c].bk[0] = layer0[c].bk[0];
								layer2[c].bk[1] = layer0[c].bk[1];
								layer2[c].bk[2] = layer0[c].bk[2];
							}
							else
							{
								//   otherwise set bk to bk average 
								PAL2RGB(bk, layer2[c].bk);
							}

							// - set glyph to swoosh glyph
							layer2[c].glyph = merge[c].glyph;
						}
						else
						{
							// if halfblock with background opaque
							AnsiCell ac;
							ac.gl = layer2[c].glyph;
							ac.fg = fg_transp ? 255 : RGB2PAL(layer2[c].fg);
							ac.bk = bk_transp ? 255 : RGB2PAL(layer2[c].bk);

							// - calc only average under swoosh fg
							int fg = AverageGlyphTransp(&ac, mask);

							// - if fg average is transparent set fg to swoosh color
							if (fg == 255)
							{
								// - if fg average is transparent set fg to swoosh color
								layer2[c].fg[0] = 0;
								layer2[c].fg[1] = 255;
								layer2[c].fg[2] = 255;
							}
							else
							{
								//   otherwise set fg to lighten fg average
								PAL2RGB(LightenColor(fg), layer2[c].fg);
							}

							// - set bk to swoosh bk
							layer2[c].bk[0] = merge[c].bk[0];
							layer2[c].bk[1] = merge[c].bk[1];
							layer2[c].bk[2] = merge[c].bk[2];

							// - set glyph to swoosh glyph
							layer2[c].glyph = merge[c].glyph;
						}

						break;

					default:
						// if fullblock

						if (fg_transp && bk_transp)
						{
							layer2[c] = merge[c];
						}
						else
						{
							if (fg_transp)
							{
								// set fg to swoosh color
								layer2[c].fg[0] = 0;
								layer2[c].fg[1] = 255;
								layer2[c].fg[2] = 255;
							}
							else
							{
								// lighten fg if not transparent
								layer2[c].fg[0] = std::min(255, layer2[c].fg[0] + 51);
								layer2[c].fg[1] = std::min(255, layer2[c].fg[1] + 51);
								layer2[c].fg[2] = std::min(255, layer2[c].fg[2] + 51);
							}

							if (bk_transp)
							{
								// set bk to swoosh color
								layer2[c].bk[0] = 0;
								layer2[c].bk[1] = 255;
								layer2[c].bk[2] = 255;
							}
							else
							{
								// lighten bk if not transparent
								layer2[c].bk[0] = std::min(255, layer2[c].bk[0] + 51);
								layer2[c].bk[1] = std::min(255, layer2[c].bk[1] + 51);
								layer2[c].bk[2] = std::min(255, layer2[c].bk[2] + 51);
							}
							// - keep underlying glyph
						}
				}
			}
			else
			if (merge[c].bk[0] != 255 || merge[c].bk[1] != 0 || merge[c].bk[2] != 255)
				layer2[c] = merge[c];
		}
	}

	// if there is no marks on layer0 treat it as single image

	const int max_anims = 16;
	int projs = 1;
	int anims = 1;
	int anim_len[max_anims] = { 1 };
	int anim_sum = 1;
	int angles = layer0[0].GetDigit();
	if (angles > 0)
	{
		projs = 2;
		anim_sum = 0;
		anims = 0;
		for (int a = 1; a < width; a++)
		{
			int len = layer0[height*a].GetDigit();
			if (len > 0)
			{
				anim_sum += len;
				anim_len[anims] = len;
				anims++;
			}
			else
				break;
		}

		if (!anims)
		{
			anims = 1;
			anim_sum = 1;
		}
	}
	else
	{
		angles = 1;

		anim_sum = 0;
		anims = 0;
		for (int a = 1; a < width; a++)
		{
			int len = layer0[height*a].GetDigit();
			if (len > 0)
			{
				anim_sum += len;
				anim_len[anims] = len;
				anims++;
			}
			else
				break;
		}

		if (!anims)
		{
			anims = 1;
			anim_sum = 1;
		}
	}

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

			frame->meta_xy[0] = 0;
			frame->meta_xy[1] = 0;

			AnsiCell* c = (AnsiCell*)malloc(sizeof(AnsiCell)*fr_width*fr_height);
			frame->cell = c;

			frame->ref[0] = fr_width; // in half blocks! (means x-middle)

			int rgb_div;

			if (projs<2 || 2 * fr_x < fr_num_x)
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

					if (c0->glyph == 2) // meta-pos
					{
						// in half cells
						frame->meta_xy[0] = (x-x0)*2 - frame->ref[0];
						frame->meta_xy[1] = (y1 - 1 - y)*2 - frame->ref[1];
					}

					c->gl = c2->glyph;

					bool bk_transp = (c2->bk[0] == c0->bk[0] && c2->bk[1] == c0->bk[1] && c2->bk[2] == c0->bk[2]);
					bool fg_transp = (c2->fg[0] == c0->bk[0] && c2->fg[1] == c0->bk[1] && c2->fg[2] == c0->bk[2]);

					bool fg_swoosh = (c2->fg[0] == 0 && c2->fg[1] == 255 && c2->fg[2] == 255);
					bool bk_swoosh = (c2->bk[0] == 0 && c2->bk[1] == 255 && c2->bk[2] == 255);

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

					if (bk_swoosh)
						c->bk = 254;
					else
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
									break;
								}
							}
						}

						int r = (c2->bk[0] * 5 + 128) / rgb_div;
						int g = (c2->bk[1] * 5 + 128) / rgb_div;
						int b = (c2->bk[2] * 5 + 128) / rgb_div;

						c->bk = 16 + 36 * r + g * 6 + b;
					}

					if (fg_swoosh)
						c->fg = 254;
					else
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
									break;
								}
							}
						}

						int r = (c2->fg[0] * 5 + 128) / rgb_div;
						int g = (c2->fg[1] * 5 + 128) / rgb_div;
						int b = (c2->fg[2] * 5 + 128) / rgb_div;

						c->fg = 16 + 36 * r + g * 6 + b;
					}

					if (recolor)
					{
						for (int i = 1 + 6 * recolor[0]; recolor[i]; i += 2)
						{
							if (c2->glyph == recolor[i])
							{
								c->gl = recolor[i + 1];
								break;
							}
						}
					}
				}
			}
		}
	}

	Sprite* sprite = (Sprite*)malloc(sizeof(Sprite) + sizeof(Sprite::Anim));

	sprite->refs = 1;
	sprite->cookie = 0;
	sprite->projs = projs;
	sprite->angles = angles;
	sprite->anims = anims;
	sprite->atlas = atlas;
	sprite->frames = frames;

	for (int i = 0; i < anims; i++)
		sprite->anim[i].length = anim_len[i];

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

	float cos30 = cosf(30 * (M_PI / 180));
	float z = fr_height / cos30 * HEIGHT_SCALE;
	float dz = ref[0][1]*0.5f / cos30 * HEIGHT_SCALE;

	float zoom = 2.0 / 3.0;

	sprite->proj_bbox[0] = -fr_width * .5f * zoom;
	sprite->proj_bbox[1] = +fr_width * .5f * zoom;
	sprite->proj_bbox[2] = -fr_width * .5f * zoom;
	sprite->proj_bbox[3] = +fr_width * .5f * zoom;
	sprite->proj_bbox[4] = -dz * zoom;
	sprite->proj_bbox[5] = (z-dz) * zoom;

	/////////////////////////////////
	
	// u_inflate_free(out);
	free(out);

	if (detached)
	{
		sprite->prev = 0;
		sprite->next = 0;
	}
	else
	{
		sprite->prev = sprite_tail;
		if (sprite_tail)
			sprite_tail->next = sprite;
		else
			sprite_head = sprite;
		sprite->next = 0;
		sprite_tail = sprite;
	}

	if (name)
		sprite->name = strdup(name);
	else
		sprite->name = 0;

	return sprite;
}

void FillRect(AnsiCell* ptr, int width, int height, int x, int y, int w, int h, AnsiCell ac)
{
	if (x < 0)
	{
		w -= -x;
		x = 0;
	}
	if (x + w > width)
		w = width - x;

	if (y < 0)
	{
		h -= -y;
		y = 0;
	}
	if (y + h > height)
		h = height - y;

	int x1 = x;
	int x2 = x + w;

	int y1 = y;
	int y2 = y + h;

	AnsiCell* dst = ptr + x1 + y1 * width;
	for (y = y1; y < y2; y++)
	{
		for (int i = 0; i < w; i++)
			dst[i] = ac;
		dst += width;
	}
}

void BlitSprite(AnsiCell* ptr, int width, int height, const Sprite::Frame* sf, int x, int y, const int clip[4], bool src_clip, AnsiCell* bk)
{
	int sx = 0, sy = 0, w = sf->width, h = sf->height;
	if (clip)
	{
		if (src_clip)
		{
			if (clip[0] >= clip[2] || clip[0] >= sf->width || clip[2] < 0 ||
				clip[1] >= clip[3] || clip[1] >= sf->height || clip[3] < 0)
				return;

			if (clip[0] < 0)
				x += -clip[0];
			else
			{
				sx = clip[0];
				w -= clip[0];
			}

			if (clip[2] < sx + w)
				w -= sx + w - clip[2];

			if (clip[1] < 0)
				y += -clip[1];
			else
			{
				sy = clip[1];
				h -= clip[1];
			}

			if (clip[3] < sy + h)
				h -= sy + h - clip[3];
		}
		else
		{
			if (x < clip[0])
			{
				w -= clip[0] - x;
				sx += clip[0] - x;
				x = clip[0];
			}
			if (x + w > clip[2])
				w = clip[2] - x;

			if (y < clip[1])
			{
				h -= clip[1] - y;
				sy += clip[1] - y;
				y = clip[1];
			}
			if (y + h > clip[3])
				h = clip[3] - y;
		}
	}

	if (x < 0)
	{
		w -= -x;
		sx += -x;
		x = 0;
	}
	if (x + w > width)
		w = width - x;

	if (y < 0)
	{
		h -= -y;
		sy += -y;
		y = 0;
	}
	if (y + h > height)
		h = height - y;

	int x1 = x;
	int x2 = x + w;

	int y1 = y;
	int y2 = y + h;

	if (x2 <= x1)
		return;

	AnsiCell* dst = ptr + x1 + y1 * width;
	const AnsiCell* src = sf->cell + sx + sy * sf->width;

	if (bk)
	{
		AnsiCell* ptr = dst;
		for (y = y1; y < y2; y++)
		{
			for (int i = 0; i < w; i++)
				ptr[i] = *bk;
			ptr += width;
		}
	}

	for (y = y1; y < y2; y++)
	{
		for (int i = 0; i < w; i++)
		{
			if (src[i].bk == 255)
			{
				// if both bk and fg are transparent -> ignore
				// if bk is transparent and gl is <space> -> ignore
				if (src[i].fg == 255 || src[i].gl == 32)
					continue;
				
				switch (src[i].gl)
				{
					case 220: // fg-lower
						dst[i].bk = AverageGlyph(dst + i, 0xC);
						break;

					case 221: // fg-left
						dst[i].bk = AverageGlyph(dst + i, 0xA);
						break;

					case 222: // fg-right
						dst[i].bk = AverageGlyph(dst + i, 0x5);
						break;

					case 223: // fg-upper
						dst[i].bk = AverageGlyph(dst + i, 0x3);
						break;

					default:
						dst[i].bk = AverageGlyph(dst + i, 0xF);
				}

				dst[i].fg = src[i].fg;
				dst[i].gl = src[i].gl;
			}
			else
			{
				if (src[i].fg == 255)
				{
					// if fg is transparent and gl is <full-blk> -> ignore
					if (src[i].gl == 219)
						continue;

					switch (src[i].gl)
					{
						case 220: // fg-lower
							dst[i].fg = AverageGlyph(dst + i, 0x3);
							break;

						case 221: // fg-left
							dst[i].fg = AverageGlyph(dst + i, 0x5);
							break;

						case 222: // fg-right
							dst[i].fg = AverageGlyph(dst + i, 0xA);
							break;

						case 223: // fg-upper
							dst[i].fg = AverageGlyph(dst + i, 0xC);
							break;

						default:
							dst[i].fg = AverageGlyph(dst + i, 0xF);
					}

					dst[i].bk = src[i].bk;
					dst[i].gl = src[i].gl;
				}
				else // if none of fg and bk is transparent -> replace
					dst[i] = src[i];
			}
		}
		dst += width;
		src += sf->width;
	}
}

void PaintFrame(AnsiCell* ptr, int width, int height, int x, int y, int w, int h, const int dst_clip[4], uint8_t fg, uint8_t bk, bool dbl, bool combine)
{
	int sx = x, sy = y;
	int sw = w, sh = h;

	bool l = true, r = true, b = true, t = true;

	x++; y++;
	w -= 2; h -= 2;

	if (dst_clip)
	{
		if (sx < dst_clip[0])
			l = false;
		if (sx+sw > dst_clip[2])
			r = false;
		if (sy < dst_clip[1])
			b = false;
		if (sy + sh > dst_clip[3])
			t = false;

		if (x < dst_clip[0])
		{
			w -= dst_clip[0] - x;
			x = dst_clip[0];
		}

		if (x + w > dst_clip[2])
			w = dst_clip[2] - x;

		if (y < dst_clip[1])
		{
			h -= dst_clip[1] - y;
			y = dst_clip[1];
		}

		if (y + h > dst_clip[3])
			h = dst_clip[3] - y;
	}

	if (sx < 0)
		l = false;
	if (sx + sw > width)
		r = false;
	if (sy < 0)
		b = false;
	if (sy + sh > height)
		t = false;


	if (x < 0)
	{
		w -= -x;
		x = 0;
	}
	if (x + w > width)
		w = width - x;

	if (y < 0)
	{
		h -= -y;
		y = 0;
	}
	if (y + h > height)
		h = height - y;

	if (w <= 0 || h <= 0)
		return;

	int x1 = x, x2 = x + w;
	int y1 = y*width, y2 = (y + h)*width;

	static const uint8_t bit2gl[16] = { 0, 0, 0, 187, 0, 205, 201, 203, 0, 188, 186, 185, 200, 202, 204, 206 };

	static uint8_t gl2bit_raw[256] = { 0x0 };
	static uint8_t gl2bit_cmb[256] = { 0xFF };
	if (gl2bit_cmb[0] == 0xff)
	{
		memset(gl2bit_cmb, 0, 176);
		for (int i = 176; i < 224; i++)
		{
			for (int j = 0; j < 16; j++)
				if (i == bit2gl[j])
					gl2bit_cmb[i] = j;
		}
	}

	uint8_t* gl2bit = combine ? gl2bit_cmb : gl2bit_raw;

	if (b)
	{
		AnsiCell* row = ptr + sy * width;

		if (l)
		{
			row[sx].gl = bit2gl[gl2bit[row[sx].gl] | 0xC];
			if (fg != 255)
				row[sx].fg = fg;
			if (bk != 255)
				row[sx].bk = bk;
		}

		for (int dx = x1; dx < x2; dx++)
		{
			row[dx].gl = bit2gl[gl2bit[row[dx].gl] | 0x5];
			if (fg != 255)
				row[dx].fg = fg;
			if (bk != 255)
				row[dx].bk = bk;
		}

		if (r)
		{
			row[sx + sw-1].gl = bit2gl[gl2bit[row[sx + sw-1].gl] | 0x9];
			if (fg != 255)
				row[sx + sw-1].fg = fg;
			if (bk != 255)
				row[sx + sw-1].bk = bk;
		}
	}

	if (t)
	{
		AnsiCell* row = ptr + (sy+sh-1) * width;

		if (l)
		{
			row[sx].gl = bit2gl[gl2bit[row[sx].gl] | 0x6];
			if (fg != 255)
				row[sx].fg = fg;
			if (bk != 255)
				row[sx].bk = bk;
		}

		for (int dx = x1; dx < x2; dx++)
		{
			row[dx].gl = bit2gl[gl2bit[row[dx].gl] | 0x5];
			if (fg != 255)
				row[dx].fg = fg;
			if (bk != 255)
				row[dx].bk = bk;
		}

		if (r)
		{
			row[sx + sw-1].gl = bit2gl[gl2bit[row[sx + sw-1].gl] | 0x3];
			if (fg != 255)
				row[sx + sw-1].fg = fg;
			if (bk != 255)
				row[sx + sw-1].bk = bk;
		}
	}

	if (l)
	{
		AnsiCell* col = ptr + sx;
		for (int dy = y1; dy < y2; dy+=width)
		{
			col[dy].gl = bit2gl[gl2bit[col[dy].gl] | 0xA];
			if (fg != 255)
				col[dy].fg = fg;
			if (bk != 255)
				col[dy].bk = bk;
		}
	}

	if (r)
	{
		AnsiCell* col = ptr + sx + sw - 1;
		for (int dy = y1; dy < y2; dy+=width)
		{
			col[dy].gl = bit2gl[gl2bit[col[dy].gl] | 0xA];
			if (fg != 255)
				col[dy].fg = fg;
			if (bk != 255)
				col[dy].bk = bk;
		}
	}
}


static const uint16_t glyph_coverage[256] =
{
	0x0000,0x2222,0x4433,0x3412,0x2312,0x2323,0x2312,0x1111,0x3333,0x1111,0x3333,0x4122,0x2222,0x2203,0x3322,0x3322,
	0x1212,0x2121,0x2222,0x2211,0x3321,0x2222,0x0022,0x2233,0x2211,0x1122,0x2121,0x1212,0x0111,0x2222,0x1122,0x2211,
	0x0000,0x2211,0x1100,0x2322,0x2211,0x1112,0x2222,0x1100,0x0201,0x1201,0x2211,0x1111,0x0011,0x1100,0x0011,0x2102,
	0x3222,0x1211,0x2112,0x2121,0x2221,0x2221,0x1222,0x2101,0x2222,0x2211,0x1111,0x1111,0x1111,0x1111,0x1101,0x2111,
	0x3212,0x2222,0x2322,0x1212,0x2322,0x2312,0x2302,0x1222,0x2222,0x1111,0x2012,0x2322,0x0322,0x3322,0x2322,0x2212,
	0x2302,0x2221,0x2312,0x2221,0x2211,0x2222,0x2211,0x2222,0x2222,0x2211,0x2322,0x1212,0x0320,0x2121,0x1200,0x0011,
	0x1100,0x1122,0x1322,0x1112,0x2122,0x1112,0x1202,0x1122,0x1322,0x1111,0x2121,0x1212,0x1111,0x1222,0x1122,0x1112,
	0x1113,0x1122,0x1112,0x1121,0x1211,0x1122,0x1111,0x1122,0x1112,0x1122,0x1112,0x1211,0x1111,0x1111,0x1200,0x1122,
	0x1212,0x1122,0x2112,0x2222,0x1122,0x1222,0x2222,0x1212,0x2212,0x2212,0x1212,0x1111,0x2211,0x1211,0x2222,0x1122,
	0x1212,0x1122,0x3322,0x2222,0x1122,0x1122,0x1222,0x1222,0x1122,0x2222,0x2222,0x1212,0x1312,0x2222,0x1221,0x3112,
	0x1222,0x1111,0x2122,0x1122,0x1322,0x2222,0x2211,0x2211,0x1112,0x1101,0x1110,0x2232,0x2232,0x1122,0x2211,0x2211,
	0x1111,0x2222,0x3333,0x1111,0x1212,0x1313,0x2222,0x1123,0x1213,0x2222,0x2222,0x2222,0x2222,0x2311,0x1312,0x0112,
	0x2110,0x2211,0x1122,0x2121,0x1111,0x2222,0x3131,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,
	0x3311,0x2222,0x0033,0x3211,0x3121,0x2131,0x1132,0x3333,0x3333,0x1201,0x1021,0x4444,0x0044,0x0404,0x4040,0x4400,
	0x1212,0x2212,0x1201,0x2222,0x1212,0x1112,0x1112,0x1211,0x2222,0x2212,0x2222,0x1222,0x1212,0x2213,0x1211,0x2222,
	0x2211,0x1312,0x0212,0x0211,0x1202,0x2012,0x1111,0x1212,0x2200,0x0000,0x0000,0x2011,0x2200,0x2100,0x2222,0x1111,
};

static const uint16_t palette_rgb[256] =
{
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x001, 0x002, 0x003, 0x004, 0x005,
	0x010, 0x011, 0x012, 0x013, 0x014, 0x015,
	0x020, 0x021, 0x022, 0x023, 0x024, 0x025,
	0x030, 0x031, 0x032, 0x033, 0x034, 0x035,
	0x040, 0x041, 0x042, 0x043, 0x044, 0x045,
	0x050, 0x051, 0x052, 0x053, 0x054, 0x055,

	0x100, 0x101, 0x102, 0x103, 0x104, 0x105,
	0x110, 0x111, 0x112, 0x113, 0x114, 0x115,
	0x120, 0x121, 0x122, 0x123, 0x124, 0x125,
	0x130, 0x131, 0x132, 0x133, 0x134, 0x135,
	0x140, 0x141, 0x142, 0x143, 0x144, 0x145,
	0x150, 0x151, 0x152, 0x153, 0x154, 0x155,

	0x200, 0x201, 0x202, 0x203, 0x204, 0x205,
	0x210, 0x211, 0x212, 0x213, 0x214, 0x215,
	0x220, 0x221, 0x222, 0x223, 0x224, 0x225,
	0x230, 0x231, 0x232, 0x233, 0x234, 0x235,
	0x240, 0x241, 0x242, 0x243, 0x244, 0x245,
	0x250, 0x251, 0x252, 0x253, 0x254, 0x255,

	0x300, 0x301, 0x302, 0x303, 0x304, 0x305,
	0x310, 0x311, 0x312, 0x313, 0x314, 0x315,
	0x320, 0x321, 0x322, 0x323, 0x324, 0x325,
	0x330, 0x331, 0x332, 0x333, 0x334, 0x335,
	0x340, 0x341, 0x342, 0x343, 0x344, 0x345,
	0x350, 0x351, 0x352, 0x353, 0x354, 0x355,

	0x400, 0x401, 0x402, 0x403, 0x404, 0x405,
	0x410, 0x411, 0x412, 0x413, 0x414, 0x415,
	0x420, 0x421, 0x422, 0x423, 0x424, 0x425,
	0x430, 0x431, 0x432, 0x433, 0x434, 0x435,
	0x440, 0x441, 0x442, 0x443, 0x444, 0x445,
	0x450, 0x451, 0x452, 0x453, 0x454, 0x455,

	0x500, 0x501, 0x502, 0x503, 0x504, 0x505,
	0x510, 0x511, 0x512, 0x513, 0x514, 0x515,
	0x520, 0x521, 0x522, 0x523, 0x524, 0x525,
	0x530, 0x531, 0x532, 0x533, 0x534, 0x535,
	0x540, 0x541, 0x542, 0x543, 0x544, 0x545,
	0x550, 0x551, 0x552, 0x553, 0x554, 0x555,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};

int LightenColor(int c)
{
	// todo make lookup table 
	// !!!
	c -= 16;
	int r = c / 36;
	c -= 36 * r;
	int g = c / 6;
	c -= 6 * g;
	int b = c;

	r += 1;
	g += 1;
	b += 1;

	if (r > 5)
		r = 5;
	if (g > 5)
		g = 5;
	if (b > 5)
		b = 5;

	return 16 + r * 36 + g * 6 + b;
}

int DarkenGlyph(const AnsiCell* ptr)
{
	int rgb = palette_rgb[AverageGlyph(ptr,0xF)];
	int r = rgb & 0xF;
	int g = (rgb>>4) & 0xF;
	int b = (rgb>>8) & 0xF;
	r = r > 1 ? r - 2 : 0;
	g = g > 1 ? g - 2 : 0;
	b = b > 1 ? b - 2 : 0;

	return 16 + r + 6 * g + 36 * b;
}

int AverageGlyph(const AnsiCell* ptr, int mask)
{
	// SIMPLER:
	// calc averaged coverage over masked quadrants
	// if result is greater than half, return foreground otherwise background

	int cov = glyph_coverage[ptr->gl];

	int num = 0;
	int sum = 0;
	if (mask & 1)
	{
		sum += cov & 0xf;
		num++;
	}
	if (mask & 2)
	{
		sum += (cov >> 4) & 0xf;
		num++;
	}
	if (mask & 4)
	{
		sum += (cov >> 8) & 0xf;
		num++;
	}
	if (mask & 8)
	{
		sum += (cov >> 12) & 0xf;
		num++;
	}

	if (sum > num * 2)
		return ptr->fg != 255 ? ptr->fg : ptr->bk;
	return ptr->bk != 255 ? ptr->bk : ptr->fg;
}

int AverageGlyphTransp(const AnsiCell* ptr, int mask)
{
	// same as AverageGlyph but doesnt flip to fg->bk/bk->fg if fg/bk is transparent

	int cov = glyph_coverage[ptr->gl];

	int num = 0;
	int sum = 0;
	if (mask & 1)
	{
		sum += cov & 0xf;
		num++;
	}
	if (mask & 2)
	{
		sum += (cov >> 4) & 0xf;
		num++;
	}
	if (mask & 4)
	{
		sum += (cov >> 8) & 0xf;
		num++;
	}
	if (mask & 8)
	{
		sum += (cov >> 12) & 0xf;
		num++;
	}

	if (sum > num * 2)
		return ptr->fg;
	return ptr->bk;
}
