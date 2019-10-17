
// here we're gonna define sprite
// it must havew:
// - one or more frames, each containing:
//   - one or more direction views, each with reflection image

#include "sprite.h"
#include "upng.h"

struct SpriteInst
{
	Sprite* sprite;
	int pos[3]; // ?

	int anim;
	int anim_time; // ?
};

Sprite* LoadPlayer(const char* path)
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

	assert(gz.id1 == 31 && gz.id2 == 139 && "gz identity");
	assert(gz.cm == 8 && "deflate method");

	if (gz.flg & (1 << 2/*FEXTRA*/))
	{
		int hi,lo;
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

	#pragma pack(push,1)
	struct XPCell
	{
		uint32_t glyph;
		uint8_t fg[3];
		uint8_t bk[3];
	};
	#pragma pack(pop)

	XPCell* layer0 = (XPCell*)((int*)out + 4); // bg specifies color key
	XPCell* layer1 = (XPCell*)((int*)(layer0 + width * height) + 2); // glyph specifies height + '0'
	XPCell* layer2 = (XPCell*)((int*)(layer1 + width * height) + 2); // image map

	int fr_num_x = (2/*refl*/ * (1/*idle*/ + 8/*walk*/));
	int fr_num_y = 8/*angl*/;

	int frames = fr_num_y * fr_num_x;
	Sprite::Frame* atlas = (Sprite::Frame*)malloc(sizeof(Sprite::Frame)*frames);

	int fr_width = width / fr_num_x;
	int fr_height = height / fr_num_y;

	for (int fr_y=0; fr_y < fr_num_y; fr_y++)
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

			int rgb_div = 255;

			if (2 * fr_x < fr_num_x)
			{
				// proj
				frame->ref[1] = 2; // in half blocks!
				frame->ref[2] = -1;
			}
			else
			{
				// refl
				frame->ref[1] = (fr_height - 1) * 2; // in half blocks!
				frame->ref[2] = -8;

				rgb_div = 400;
			}

			int x0 = fr_x * fr_width, x1 = x0 + fr_width;
			int y0 = fr_y * fr_height, y1 = y0 + fr_height;
			for (int y = y1-1; y >= y0; y--)
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
						int r = (c2->bk[0] * 5 + 128) / rgb_div;
						int g = (c2->bk[1] * 5 + 128) / rgb_div;
						int b = (c2->bk[2] * 5 + 128) / rgb_div;
						c->bk = 16 + r + g*6 + b*36;
					}

					if (fg_transp)
						c->fg = 255;
					else
					{
						int r = (c2->fg[0] * 5 + 128) / rgb_div;
						int g = (c2->fg[1] * 5 + 128) / rgb_div;
						int b = (c2->fg[2] * 5 + 128) / rgb_div;
						c->fg = 16 + r + g * 6 + b * 36;
					}
				}
			}
		}
	}

	Sprite* sprite = (Sprite*)malloc(sizeof(Sprite) + sizeof(Sprite::Anim));

	sprite->angles = 8;
	sprite->anims = 2;
	sprite->atlas = atlas;
	sprite->frames = frames;

	sprite->anim[0].length = 1;
	sprite->anim[1].length = 8;

	for (int anim=0; anim<sprite->anims; anim++)
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

	return sprite;

	// let's begin from something trivial...
	// just load player.xp (in known layout)
	
	// layer0: defines colorkey
	// layer1: height map
	// layer2: image map

	// 8 rows, each 9 cells (vertically) - 8 angles CCW starting from front view
	// image have left (projection) and right (reflection) halfs
	// each half has 9 frames, each 5 cells (horizontally)
	// first frame is just still idle
	// next 8 frames are walk animation

	// some example layout def grammar:
	// X:[R2][F1,F8]  // [R2] split width into 2 halfs, [F1+F8] two animations, first has 1 frame, second has 8 frames
	// Y:[A8]         // [A8] 8 angles

	// wolf comes in another (assuming every mount is just 1 frame animation):

	// X:[A8]
	// Y:[R2][F1,F1,F1,F1,F1]
}

void FreeSprite(Sprite* spr)
{
	for (int f = 0; f < spr->frames; f++)
		free(spr->atlas[f].cell);

	free(spr->atlas);

	for (int a = 0; a < spr->anims; a++)
		free(spr->anim[a].frame_idx);

	free(spr);
}