
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

	/////////////////////////////////
	u_inflate_free(out);
	return 0;

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

