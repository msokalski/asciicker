/*
uPNG -- altered by @msokalski ( added missing image formats, replaced defective inflate with tinfl.c )
uPNG -- derived from LodePNG version 20100808

Copyright (c) 2005-2010 Lode Vandevenne
Copyright (c) 2010 Sean Middleditch

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.

		2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "upng.h"

#define MAKE_BYTE(b) ((b) & 0xFF)
#define MAKE_DWORD(a,b,c,d) ((MAKE_BYTE(a) << 24) | (MAKE_BYTE(b) << 16) | (MAKE_BYTE(c) << 8) | MAKE_BYTE(d))
#define MAKE_DWORD_PTR(p) MAKE_DWORD((p)[0], (p)[1], (p)[2], (p)[3])

#define CHUNK_IHDR MAKE_DWORD('I','H','D','R')
#define CHUNK_IDAT MAKE_DWORD('I','D','A','T')
#define CHUNK_IEND MAKE_DWORD('I','E','N','D')
#define CHUNK_PLTE MAKE_DWORD('P','L','T','E')
#define CHUNK_tRNS MAKE_DWORD('t','R','N','S')

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285

#define NUM_DEFLATE_CODE_SYMBOLS 288	/*256 literals, the end code, some length codes, and 2 unused codes */
#define NUM_DISTANCE_SYMBOLS 32	/*the distance codes have their own symbols, 30 used, 2 unused */
#define NUM_CODE_LENGTH_CODES 19	/*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros */
#define MAX_SYMBOLS 288 /* largest number of symbols used by any tree type */

#define DEFLATE_CODE_BITLEN 15
#define DISTANCE_BITLEN 15
#define CODE_LENGTH_BITLEN 7
#define MAX_BIT_LENGTH 15 /* largest bitlen used by any tree type */

#define DEFLATE_CODE_BUFFER_SIZE (NUM_DEFLATE_CODE_SYMBOLS * 2)
#define DISTANCE_BUFFER_SIZE (NUM_DISTANCE_SYMBOLS * 2)
#define CODE_LENGTH_BUFFER_SIZE (NUM_DISTANCE_SYMBOLS * 2)

#define SET_ERROR(upng,code) do { (upng)->error = (code); (upng)->error_line = __LINE__; } while (0)

#define upng_chunk_length(chunk) MAKE_DWORD_PTR(chunk)
#define upng_chunk_type(chunk) MAKE_DWORD_PTR((chunk) + 4)
#define upng_chunk_critical(chunk) (((chunk)[4] & 32) == 0)

typedef enum upng_state {
	UPNG_ERROR		= -1,
	UPNG_DECODED	= 0,
	UPNG_HEADER		= 1,
	UPNG_NEW		= 2
} upng_state;

typedef enum upng_color {
	UPNG_LUM		= 0,
	UPNG_RGB		= 2,
	UPNG_IDX		= 3,
	UPNG_LUMA		= 4,
	UPNG_RGBA		= 6
} upng_color;

typedef struct upng_source {
	const unsigned char*	buffer;
	unsigned long			size;
	char					owning;
} upng_source;

typedef struct upng_pal {
	unsigned len;
	unsigned trns; 
	unsigned char rgba[1024];
} upng_pal;

struct upng_t {
	unsigned		width;
	unsigned		height;

	upng_color		color_type;
	unsigned		color_depth;
	upng_format		format;

	unsigned char*	buffer;
	unsigned long	size;

	upng_error		error;
	unsigned		error_line;

	upng_state		state;
	upng_source		source;

	upng_pal pal;
};

size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);
static upng_error uz_inflate_data(upng_t* upng, unsigned char* out, unsigned long outsize, const unsigned char *in, unsigned long insize, unsigned long inpos)
{
	size_t written = tinfl_decompress_mem_to_mem(out, outsize, in + inpos, insize - inpos, 0);
	if (written == (size_t)-1)
		SET_ERROR(upng, UPNG_EMALFORMED);
	return upng->error;
}

static upng_error uz_inflate(upng_t* upng, unsigned char *out, unsigned long outsize, const unsigned char *in, unsigned long insize)
{
	/* we require two bytes for the zlib data header */
	if (insize < 2) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way */
	if ((in[0] * 256 + in[1]) % 31 != 0) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/*error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec */
	if ((in[0] & 15) != 8 || ((in[0] >> 4) & 15) > 7) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* the specification of PNG says about the zlib stream: "The additional flags shall not specify a preset dictionary." */
	if (((in[1] >> 5) & 1) != 0) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* create output buffer */
	uz_inflate_data(upng, out, outsize, in, insize, 2);

	return upng->error;
}

/*Paeth predicter, used by PNG filter type 4*/
static int paeth_predictor(int a, int b, int c)
{
	int p = a + b - c;
	int pa = p > a ? p - a : a - p;
	int pb = p > b ? p - b : b - p;
	int pc = p > c ? p - c : c - p;

	if (pa <= pb && pa <= pc)
		return a;
	else if (pb <= pc)
		return b;
	else
		return c;
}

static void unfilter_scanline(upng_t* upng, unsigned char *recon, const unsigned char *scanline, const unsigned char *precon, unsigned long bytewidth, unsigned char filterType, unsigned long length)
{
	/*
	   For PNG filter method 0
	   unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte, the filter works byte per byte (bytewidth = 1)
	   precon is the previous unfiltered scanline, recon the result, scanline the current one
	   the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
	   recon and scanline MAY be the same memory address! precon must be disjoint.
	 */

	unsigned long i;
	switch (filterType) {
	case 0:
		for (i = 0; i < length; i++)
			recon[i] = scanline[i];
		break;
	case 1:
		for (i = 0; i < bytewidth; i++)
			recon[i] = scanline[i];
		for (i = bytewidth; i < length; i++)
			recon[i] = scanline[i] + recon[i - bytewidth];
		break;
	case 2:
		if (precon)
			for (i = 0; i < length; i++)
				recon[i] = scanline[i] + precon[i];
		else
			for (i = 0; i < length; i++)
				recon[i] = scanline[i];
		break;
	case 3:
		if (precon) {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i] + precon[i] / 2;
			for (i = bytewidth; i < length; i++)
				recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
		} else {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i];
			for (i = bytewidth; i < length; i++)
				recon[i] = scanline[i] + recon[i - bytewidth] / 2;
		}
		break;
	case 4:
		if (precon) {
			for (i = 0; i < bytewidth; i++)
				recon[i] = (unsigned char)(scanline[i] + paeth_predictor(0, precon[i], 0));
			for (i = bytewidth; i < length; i++)
				recon[i] = (unsigned char)(scanline[i] + paeth_predictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
		} else {
			for (i = 0; i < bytewidth; i++)
				recon[i] = scanline[i];
			for (i = bytewidth; i < length; i++)
				recon[i] = (unsigned char)(scanline[i] + paeth_predictor(recon[i - bytewidth], 0, 0));
		}
		break;
	default:
		SET_ERROR(upng, UPNG_EMALFORMED);
		break;
	}
}

static void unfilter(upng_t* upng, unsigned char *out, const unsigned char *in, unsigned w, unsigned h, unsigned bpp)
{
	/*
	   For PNG filter method 0
	   this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 it's called 7 times)
	   out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
	   w and h are image dimensions or dimensions of reduced image, bpp is bpp per pixel
	   in and out are allowed to be the same memory address!
	 */

	unsigned y;
	unsigned char *prevline = 0;

	unsigned long bytewidth = (bpp + 7) / 8;	/*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise */
	unsigned long linebytes = (w * bpp + 7) / 8;

	for (y = 0; y < h; y++) {
		unsigned long outindex = linebytes * y;
		unsigned long inindex = (1 + linebytes) * y;	/*the extra filterbyte added to each row */
		unsigned char filterType = in[inindex];

		unfilter_scanline(upng, &out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
		if (upng->error != UPNG_EOK) {
			return;
		}

		prevline = &out[outindex];
	}
}

static void remove_padding_bits(unsigned char *out, const unsigned char *in, unsigned long olinebits, unsigned long ilinebits, unsigned h)
{
	/*
	   After filtering there are still padding bpp if scanlines have non multiple of 8 bit amounts. They need to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers for the Adam7 code, the color convert code and the output to the user.
	   in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must have >= ilinebits*h bpp, out must have >= olinebits*h bpp, olinebits must be <= ilinebits
	   also used to move bpp after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
	   only useful if (ilinebits - olinebits) is a value in the range 1..7
	 */
	unsigned y;
	unsigned long diff = ilinebits - olinebits;
	unsigned long obp = 0, ibp = 0;	/*bit pointers */
	for (y = 0; y < h; y++) {
		unsigned long x;
		for (x = 0; x < olinebits; x++) {
			unsigned char bit = (unsigned char)((in[(ibp) >> 3] >> (7 - ((ibp) & 0x7))) & 1);
			ibp++;

			if (bit == 0)
				out[(obp) >> 3] &= (unsigned char)(~(1 << (7 - ((obp) & 0x7))));
			else
				out[(obp) >> 3] |= (1 << (7 - ((obp) & 0x7)));
			++obp;
		}
		ibp += diff;
	}
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from the IDAT chunks*/
static void post_process_scanlines(upng_t* upng, unsigned char *out, unsigned char *in, const upng_t* info_png)
{
	unsigned bpp = upng_get_bpp(info_png);
	unsigned w = info_png->width;
	unsigned h = info_png->height;

	if (bpp == 0) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return;
	}

	if (bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8) {
		unfilter(upng, in, in, w, h, bpp);
		if (upng->error != UPNG_EOK) {
			return;
		}
		remove_padding_bits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
	} else {
		unfilter(upng, out, in, w, h, bpp);	/*we can immediatly filter into the out buffer, no other steps needed */
	}
}

static upng_format determine_format(upng_t* upng) {
	switch (upng->color_type) {
	case UPNG_LUM:
		switch (upng->color_depth) {
		case 1:
			return UPNG_LUMINANCE1;
		case 2:
			return UPNG_LUMINANCE2;
		case 4:
			return UPNG_LUMINANCE4;
		case 8:
			return UPNG_LUMINANCE8;
		case 16:
			return UPNG_LUMINANCE16;			
		default:
			return UPNG_BADFORMAT;
		}
	case UPNG_RGB:
		switch (upng->color_depth) {
		case 8:
			return UPNG_RGB8;
		case 16:
			return UPNG_RGB16;
		default:
			return UPNG_BADFORMAT;
		}

	case UPNG_IDX:
		if (upng->pal.trns)
		{
			switch (upng->color_depth) {
			case 1:
				return UPNG_INDEX1_RGBA;
			case 2:
				return UPNG_INDEX2_RGBA;
			case 4:
				return UPNG_INDEX4_RGBA;
			case 8:
				return UPNG_INDEX8_RGBA;
			default:
				return UPNG_BADFORMAT;
			}
		}
		else
		{
			switch (upng->color_depth) {
			case 1:
				return UPNG_INDEX1_RGB;
			case 2:
				return UPNG_INDEX2_RGB;
			case 4:
				return UPNG_INDEX4_RGB;
			case 8:
				return UPNG_INDEX8_RGB;
			default:
				return UPNG_BADFORMAT;
			}
		}
		

	case UPNG_LUMA:
		switch (upng->color_depth) {
		case 8:
			return UPNG_LUMINANCE_ALPHA8;
		case 16:
			return UPNG_LUMINANCE_ALPHA16;
		default:
			return UPNG_BADFORMAT;
		}
	case UPNG_RGBA:
		switch (upng->color_depth) {
		case 8:
			return UPNG_RGBA8;
		case 16:
			return UPNG_RGBA16;
		default:
			return UPNG_BADFORMAT;
		}
	default:
		return UPNG_BADFORMAT;
	}
}

static void upng_free_source(upng_t* upng)
{
	if (upng->source.owning != 0) {
		free((void*)upng->source.buffer);
	}

	upng->source.buffer = NULL;
	upng->source.size = 0;
	upng->source.owning = 0;
}

/*read the information from the header and store it in the upng_Info. return value is error*/
upng_error upng_header(upng_t* upng)
{
	/* if we have an error state, bail now */
	if (upng->error != UPNG_EOK) {
		return upng->error;
	}

	/* if the state is not NEW (meaning we are ready to parse the header), stop now */
	if (upng->state != UPNG_NEW) {
		return upng->error;
	}

	/* minimum length of a valid PNG file is 29 bytes
	 * FIXME: verify this against the specification, or
	 * better against the actual code below */
	if (upng->source.size < 29) {
		SET_ERROR(upng, UPNG_ENOTPNG);
		return upng->error;
	}

	/* check that PNG header matches expected value */
	if (upng->source.buffer[0] != 137 || upng->source.buffer[1] != 80 || upng->source.buffer[2] != 78 || upng->source.buffer[3] != 71 || upng->source.buffer[4] != 13 || upng->source.buffer[5] != 10 || upng->source.buffer[6] != 26 || upng->source.buffer[7] != 10) {
		SET_ERROR(upng, UPNG_ENOTPNG);
		return upng->error;
	}

	/* check that the first chunk is the IHDR chunk */
	if (MAKE_DWORD_PTR(upng->source.buffer + 12) != CHUNK_IHDR) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* read the values given in the header */
	upng->width = MAKE_DWORD_PTR(upng->source.buffer + 16);
	upng->height = MAKE_DWORD_PTR(upng->source.buffer + 20);
	upng->color_depth = upng->source.buffer[24];
	upng->color_type = (upng_color)upng->source.buffer[25];

	/* determine our color format */
	upng->format = determine_format(upng);
	if (upng->format == UPNG_BADFORMAT) {
		SET_ERROR(upng, UPNG_EUNFORMAT);
		return upng->error;
	}

	/* check that the compression method (byte 27) is 0 (only allowed value in spec) */
	if (upng->source.buffer[26] != 0) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* check that the compression method (byte 27) is 0 (only allowed value in spec) */
	if (upng->source.buffer[27] != 0) {
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	/* check that the compression method (byte 27) is 0 (spec allows 1, but uPNG does not support it) */
	if (upng->source.buffer[28] != 0) {
		SET_ERROR(upng, UPNG_EUNINTERLACED);
		return upng->error;
	}

	upng->state = UPNG_HEADER;
	return upng->error;
}

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
upng_error upng_decode(upng_t* upng)
{
	const unsigned char *chunk;
	unsigned char* compressed;
	unsigned char* inflated;
	unsigned long compressed_size = 0, compressed_index = 0;
	unsigned long inflated_size;
	upng_error error;

	/* if we have an error state, bail now */
	if (upng->error != UPNG_EOK) {
		return upng->error;
	}

	/* parse the main header, if necessary */
	upng_header(upng);
	if (upng->error != UPNG_EOK) {
		return upng->error;
	}

	/* if the state is not HEADER (meaning we are ready to decode the image), stop now */
	if (upng->state != UPNG_HEADER) {
		return upng->error;
	}

	/* release old result, if any */
	if (upng->buffer != 0) {
		free(upng->buffer);
		upng->buffer = 0;
		upng->size = 0;
	}

	/* first byte of the first chunk after the header */
	chunk = upng->source.buffer + 33;

	upng->pal.trns = 0;
	upng->pal.len = 0;

	/* scan through the chunks, finding the size of all IDAT chunks, and also
	 * verify general well-formed-ness */
	while (chunk < upng->source.buffer + upng->source.size) {
		unsigned long length;
		const unsigned char *data;	/*the data in the chunk */

		/* make sure chunk header is not larger than the total compressed */
		if ((unsigned long)(chunk - upng->source.buffer + 12) > upng->source.size) {
			SET_ERROR(upng, UPNG_EMALFORMED);
			return upng->error;
		}

		/* get length; sanity check it */
		length = upng_chunk_length(chunk);
		if (length > INT_MAX) {
			SET_ERROR(upng, UPNG_EMALFORMED);
			return upng->error;
		}

		/* make sure chunk header+paylaod is not larger than the total compressed */
		if ((unsigned long)(chunk - upng->source.buffer + length + 12) > upng->source.size) {
			SET_ERROR(upng, UPNG_EMALFORMED);
			return upng->error;
		}

		/* get pointer to payload */
		data = chunk + 8;

		/* parse chunks */
		if (upng_chunk_type(chunk) == CHUNK_IDAT) {
			compressed_size += length;
		} else if (upng_chunk_type(chunk) == CHUNK_IEND) {
			break;
		} else if (upng_chunk_type(chunk) == CHUNK_PLTE) {
			int len = length / 3;
			int old = upng->pal.len;
			if (len>256 || len*3 != length)
			{
				SET_ERROR(upng, UPNG_EMALFORMED);
				return upng->error;
			}
			upng->pal.len = len > old ? len : old;
			for (int i = 0; i < len; i++)
			{
				upng->pal.rgba[4 * i + 0] = chunk[8+0 + 3 * i];
				upng->pal.rgba[4 * i + 1] = chunk[8+1 + 3 * i];
				upng->pal.rgba[4 * i + 2] = chunk[8+2 + 3 * i];
			}
			for (int i = old; i < len; i++)
				upng->pal.rgba[4 * i + 3] = 0xFF;
		} else if (upng_chunk_type(chunk) == CHUNK_tRNS) {
			if (length>256)
			{
				SET_ERROR(upng, UPNG_EMALFORMED);
				return upng->error;
			}
			for (unsigned i = upng->pal.len; i < length; i++)
			{
				upng->pal.rgba[4*i+0]=0;				
				upng->pal.rgba[4*i+1]=0;				
				upng->pal.rgba[4*i+2]=0;				
			}
			upng->pal.len = upng->pal.len > length ? upng->pal.len : length;
			upng->pal.trns = length;
			for (unsigned i = 0; i < length; i++)
				upng->pal.rgba[4 * i + 3] = chunk[8 + i];
		} else if (upng_chunk_critical(chunk)) {
			SET_ERROR(upng, UPNG_EUNSUPPORTED);
			return upng->error;
		}

		chunk += upng_chunk_length(chunk) + 12;
	}

	/* allocate enough space for the (compressed and filtered) image data */
	compressed = (unsigned char*)malloc(compressed_size);
	if (compressed == NULL) {
		SET_ERROR(upng, UPNG_ENOMEM);
		return upng->error;
	}

	/* scan through the chunks again, this time copying the values into
	 * our compressed buffer.  there's no reason to validate anything a second time. */
	chunk = upng->source.buffer + 33;
	while (chunk < upng->source.buffer + upng->source.size) {
		unsigned long length;
		const unsigned char *data;	/*the data in the chunk */

		length = upng_chunk_length(chunk);
		data = chunk + 8;

		/* parse chunks */
		if (upng_chunk_type(chunk) == CHUNK_IDAT) {
			memcpy(compressed + compressed_index, data, length);
			compressed_index += length;
		} else if (upng_chunk_type(chunk) == CHUNK_IEND) {
			break;
		}

		chunk += upng_chunk_length(chunk) + 12;
	}

	/* allocate space to store inflated (but still filtered) data */
	inflated_size = ((upng->width * (upng->height * upng_get_bpp(upng) + 7)) / 8) + upng->height;
	inflated = (unsigned char*)malloc(inflated_size);
	if (inflated == NULL) {
		free(compressed);
		SET_ERROR(upng, UPNG_ENOMEM);
		return upng->error;
	}

	/* decompress image data */
	error = uz_inflate(upng, inflated, inflated_size, compressed, compressed_size);
	if (error != UPNG_EOK) {
		free(compressed);
		free(inflated);
		return upng->error;
	}

	/* free the compressed compressed data */
	free(compressed);

	/* allocate final image buffer */
	upng->size = (upng->height * upng->width * upng_get_bpp(upng) + 7) / 8;
	upng->buffer = (unsigned char*)malloc(upng->size);
	if (upng->buffer == NULL) {
		free(inflated);
		upng->size = 0;
		SET_ERROR(upng, UPNG_ENOMEM);
		return upng->error;
	}

	/* unfilter scanlines */
	post_process_scanlines(upng, upng->buffer, inflated, upng);
	free(inflated);

	if (upng->error != UPNG_EOK) {
		free(upng->buffer);
		upng->buffer = NULL;
		upng->size = 0;
	} else {
		upng->state = UPNG_DECODED;
	}

	/* we are done with our input buffer; free it if we own it */
	upng_free_source(upng);

	return upng->error;
}

static upng_t* upng_new(void)
{
	upng_t* upng;

	upng = (upng_t*)malloc(sizeof(upng_t));
	if (upng == NULL) {
		return NULL;
	}

	upng->buffer = NULL;
	upng->size = 0;

	upng->width = upng->height = 0;

	upng->color_type = UPNG_RGBA;
	upng->color_depth = 8;
	upng->format = UPNG_RGBA8;

	upng->state = UPNG_NEW;

	upng->error = UPNG_EOK;
	upng->error_line = 0;

	upng->source.buffer = NULL;
	upng->source.size = 0;
	upng->source.owning = 0;

	return upng;
}

upng_t* upng_new_from_bytes(const unsigned char* buffer, unsigned long size)
{
	upng_t* upng = upng_new();
	if (upng == NULL) {
		return NULL;
	}

	upng->source.buffer = buffer;
	upng->source.size = size;
	upng->source.owning = 0;

	return upng;
}

upng_t* upng_new_from_file(const char *filename)
{
	upng_t* upng;
	unsigned char *buffer;
	FILE *file;
	long size;

	upng = upng_new();
	if (upng == NULL) {
		return NULL;
	}

	file = fopen(filename, "rb");
	if (file == NULL) {
		SET_ERROR(upng, UPNG_ENOTFOUND);
		return upng;
	}

	/* get filesize */
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	/* read contents of the file into the vector */
	buffer = (unsigned char *)malloc((unsigned long)size);
	if (buffer == NULL) {
		fclose(file);
		SET_ERROR(upng, UPNG_ENOMEM);
		return upng;
	}
	
	size_t ret = fread(buffer, 1, (unsigned long)size, file);
	fclose(file);
	if (ret != size)
	{
		free(buffer);
		SET_ERROR(upng, UPNG_EMALFORMED);
		return upng;
	}
	/* set the read buffer as our source buffer, with owning flag set */
	upng->source.buffer = buffer;
	upng->source.size = size;
	upng->source.owning = 1;

	return upng;
}

void upng_free(upng_t* upng)
{
	/* deallocate image buffer */
	if (upng->buffer != NULL) {
		free(upng->buffer);
	}

	/* deallocate source buffer, if necessary */
	upng_free_source(upng);

	/* deallocate struct itself */
	free(upng);
}

upng_error upng_get_error(const upng_t* upng)
{
	return upng->error;
}

unsigned upng_get_error_line(const upng_t* upng)
{
	return upng->error_line;
}

unsigned upng_get_width(const upng_t* upng)
{
	return upng->width;
}

unsigned upng_get_height(const upng_t* upng)
{
	return upng->height;
}

unsigned upng_get_bpp(const upng_t* upng)
{
	return upng_get_bitdepth(upng) * upng_get_components(upng);
}

unsigned upng_get_components(const upng_t* upng)
{
	switch (upng->color_type) {
	case UPNG_LUM:
		return 1;
	case UPNG_RGB:
		return 3;
	case UPNG_IDX:
		return 1;
	case UPNG_LUMA:
		return 2;
	case UPNG_RGBA:
		return 4;
	default:
		return 0;
	}
}

unsigned upng_get_bitdepth(const upng_t* upng)
{
	return upng->color_depth;
}

unsigned upng_get_pixelsize(const upng_t* upng)
{
	unsigned bits = upng_get_bitdepth(upng) * upng_get_components(upng);
	bits += bits % 8;
	return bits;
}

upng_format upng_get_format(const upng_t* upng)
{
	return upng->format;
}

const unsigned char* upng_get_buffer(const upng_t* upng)
{
	return upng->buffer;
}

unsigned upng_get_size(const upng_t* upng)
{
	return upng->size;
}

const unsigned char* upng_get_pal_buffer(const upng_t* upng)
{
	return upng->pal.rgba;
}

unsigned upng_get_pal_size(const upng_t* upng)
{
	return upng->pal.len;
}



