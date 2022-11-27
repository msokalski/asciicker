
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "../upng.h"
#include "../rgba8.h"

uint64_t GetTime()
{
	#ifdef _WIN32
	//return GetTickCount() * (uint64_t)1000;
	LARGE_INTEGER c;
	static LARGE_INTEGER f;
	static BOOL bf = QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&c);
	uint64_t n = c.QuadPart;
	uint64_t d = f.QuadPart;
	uint64_t m = 1000000;
	// calc n*m/d carefully!
	// naive mul/div would work for upto 5h on 1GHz freq
	// we exploit fact that m*d fits in uint64 (upto 18THz freq)
	// so n%d*m fits as well
	return n / d * m + n % d * m / d;
	#else
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
	#endif
}

uint32_t* LoadImg(const char* path, int* w, int* h)
{
	upng_t* upng = upng_new_from_file(path);

	if (!upng)
		return 0;

	if (upng_get_error(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return 0;
	}

	if (upng_decode(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return 0;
	}

	int format, width, height, depth;
	format = upng_get_format(upng);
	width = upng_get_width(upng);
	height = upng_get_height(upng);
	depth = upng_get_bpp(upng) / 8;

	const void* buf = upng_get_buffer(upng);
	const void* pal_buf = upng_get_pal_buffer(upng);
	unsigned pal_size = upng_get_pal_size(upng);

	uint32_t* pix = (uint32_t*)malloc(width * height * 4);
	Convert_UI32_AABBGGRR(pix, (A3D_ImageFormat)format, width, height, buf, pal_size, pal_buf);

	upng_free(upng);

	if (w)
		*w = width;
	if (h)
		*h = height;

	return pix;
}

#pragma pack(push,1)
struct XPCell
{
	uint32_t glyph;
	uint8_t fg[3];
	uint8_t bk[3];
};
#pragma pack(pop)


uint8_t CHN(uint32_t p, int c) 
{ 
	return (p>>(c*8))&0xff; 
}

// 216C
/*
uint8_t PAL(uint8_t v) 
{
	return (v+25)/51*51;
}

uint8_t PAL_LO(uint8_t v)             
{
	return (v)/51*51;
}

uint8_t PAL_HI(uint8_t v) 
{
	return ((v)+50)/51*51;
}
*/

// 8C test (3 bits!!!)
/*
uint8_t PAL(uint8_t v) 
{
	return (v+128)/255*255;
}

uint8_t PAL_LO(uint8_t v) 
{
	return (v)/255*255;
}

uint8_t PAL_HI(uint8_t v) 
{
	return ((v)+255)/255*255;
}
*/

// 27C test
/*
uint8_t PAL(uint8_t v) 
{
	return (v+64)/128*128;
}

uint8_t PAL_LO(uint8_t v) 
{
	return (v)/128*128;
}

uint8_t PAL_HI(uint8_t v) 
{
	return ((v)+128)/128*128;
}
*/


// 64C test
/*
uint8_t PAL(uint8_t v) 
{
	return (v+42)/85*85;
}

uint8_t PAL_LO(uint8_t v) 
{
	return (v)/85*85;
}

uint8_t PAL_HI(uint8_t v) 
{
	return ((v)+84)/85*85;
}
*/

// 4096C test
/*
uint8_t PAL(uint8_t v) 
{
	return (v+8)/17*17;
}

uint8_t PAL_LO(uint8_t v) 
{
	return (v)/17*17;
}

uint8_t PAL_HI(uint8_t v) 
{
	return ((v)+16)/17*17;
}
*/

// NO PAL (all 16M colors)
/*
uint8_t PAL(uint8_t v) 
{
	return v;
}

uint8_t PAL_LO(uint8_t v) 
{
	return v;
}

uint8_t PAL_HI(uint8_t v) 
{
	return v;
}
*/

int32_t ABS(int32_t v) 
{
	return v<0 ? -v : v;
}

uint32_t DIF(uint8_t c[3], uint32_t r) 
{
	return 
	    2*ABS((int32_t)(c[0])-(int32_t)(CHN(r,0))) +
    	3*ABS((int32_t)(c[1])-(int32_t)(CHN(r,1))) +
    	1*ABS((int32_t)(c[2])-(int32_t)(CHN(r,2)));
}

void DEV(uint8_t c[3], uint32_t r, int d[3])
{
	/*
	d[0] += (int32_t)(c[0])-(int32_t)CHN(r,0);
	d[1] += (int32_t)(c[1])-(int32_t)CHN(r,1);
	d[2] += (int32_t)(c[2])-(int32_t)CHN(r,2);
	*/

	d[0] += (int32_t)(c[0]) * c[0] - (int32_t)CHN(r,0) * CHN(r,0);
	d[1] += (int32_t)(c[1]) * c[1] - (int32_t)CHN(r,1) * CHN(r,1);
	d[2] += (int32_t)(c[2]) * c[2] - (int32_t)CHN(r,2) * CHN(r,2);
}

int MIN(int a, int b)
{
	return a < b ? a : b;
}

int MAX(int a, int b)
{
	return a > b ? a : b;
}

void ADD(uint32_t* p, int d[3])
{
	uint32_t v = *p;
	/*
	int c[3] =
	{
		MIN(255,MAX(0,CHN(v,0) + d[0])),
		MIN(255,MAX(0,CHN(v,1) + d[1])),
		MIN(255,MAX(0,CHN(v,2) + d[2]))
	};
	*/

	int c[3] =
	{
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,0) * CHN(v,0) + d[0]))),
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,1) * CHN(v,1) + d[1]))),
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,2) * CHN(v,2) + d[2]))),
	};

	*p = c[0] | (c[1]<<8) | (c[2]<<16);
}

void PAL(uint8_t c[3], uint8_t pal[][3], int pal_size)
{
	int e = -1;
	int p;
	for (int i=0; i<pal_size; i++)
	{
		int ie = 2*ABS(c[0]-pal[i][0]) + 3*ABS(c[1]-pal[i][1]) + ABS(c[2]-pal[i][2]);
		if (e<0 || ie<e)
		{
			e = ie;
			p = i;
		}
	}
	c[0] = pal[p][0];
	c[1] = pal[p][1];
	c[2] = pal[p][2];
}

void Do(uint32_t src[4], XPCell* ptr, int dev[3], uint8_t pal[][3], int pal_size)
{
	uint32_t ll = src[0];
	uint32_t lr = src[1];
	uint32_t ul = src[2];
	uint32_t ur = src[3];

	int d_err;
	int d_gl=0, d_c0, d_c1;
	int d_dev[3];

	// find best 2 color indices C0, C1
	// and mixing factor  3/4 C0 + 1/4 C1 or 2/4 C0 + 2/4 C1
	for (int gl=1; gl<3; gl++)
	{
		int c0_w = 4-gl;
		int c1_w = gl;
		for (int c0 = 0; c0<pal_size; c0++)
		{
			for (int c1 = 0; c1<pal_size; c1++)
			{
				/*
				if (ABS(pal[c0][0] - pal[c1][0]) > 51 ||
					ABS(pal[c0][1] - pal[c1][1]) > 51 ||
					ABS(pal[c0][2] - pal[c1][2]) > 51)
					continue;

				uint8_t G[3] =
				{
					(uint8_t)((c0_w * pal[c0][0] + c1_w * pal[c1][0])/4),
					(uint8_t)((c0_w * pal[c0][1] + c1_w * pal[c1][1])/4),
					(uint8_t)((c0_w * pal[c0][2] + c1_w * pal[c1][2])/4),
				};
				*/

				// make use gamma = 2 (solves bleached colors and bad dither blocks)
				uint8_t G[3] =
				{
					(uint8_t)sqrt( (c0_w * pal[c0][0] * pal[c0][0] + c1_w * pal[c1][0] * pal[c1][0]) / 4 ),
					(uint8_t)sqrt( (c0_w * pal[c0][1] * pal[c0][1] + c1_w * pal[c1][1] * pal[c1][1]) / 4 ),
					(uint8_t)sqrt( (c0_w * pal[c0][2] * pal[c0][2] + c1_w * pal[c1][2] * pal[c1][2]) / 4 ),
				};

				// calc err
				int g_err = DIF(G,ll) + DIF(G,lr) + DIF(G,ul) + DIF(G,ur);

				if (!d_gl || g_err < d_err)
				{
					d_err = g_err;
					d_gl = gl;
					d_c0 = c0;
					d_c1 = c1;

					d_dev[0]=0;
					d_dev[1]=0;
					d_dev[2]=0;

					DEV(G,ll,d_dev);
					DEV(G,lr,d_dev);
					DEV(G,ul,d_dev);
					DEV(G,ur,d_dev);
				}
			}
		}
	}

	// find out if using half-blocks is beter than dither blocks

	uint8_t L[3] = 
	{
		(uint8_t)( (CHN(ll,0) + CHN(ul,0) + 1) / 2 ),
		(uint8_t)( (CHN(ll,1) + CHN(ul,1) + 1) / 2 ),
		(uint8_t)( (CHN(ll,2) + CHN(ul,2) + 1) / 2 ),
	};

	PAL(L,pal,pal_size);

	uint8_t R[3] = 
	{
		(uint8_t)( (CHN(lr,0) + CHN(ur,0) + 1) / 2 ),
		(uint8_t)( (CHN(lr,1) + CHN(ur,1) + 1) / 2 ),
		(uint8_t)( (CHN(lr,2) + CHN(ur,2) + 1) / 2 ),
	};

	PAL(R,pal,pal_size);

	uint8_t B[3] =
	{
		(uint8_t)( (CHN(ll,0) + CHN(lr,0) + 1) / 2 ),
		(uint8_t)( (CHN(ll,1) + CHN(lr,1) + 1) / 2 ),
		(uint8_t)( (CHN(ll,2) + CHN(lr,2) + 1) / 2 ),
	};

	PAL(B,pal,pal_size);

	uint8_t T[3] =
	{ 
		(uint8_t)( (CHN(ul,0) + CHN(ur,0) + 1) / 2 ),
		(uint8_t)( (CHN(ul,1) + CHN(ur,1) + 1) / 2 ),
		(uint8_t)( (CHN(ul,2) + CHN(ur,2) + 1) / 2 ),
	};

	PAL(T,pal,pal_size);


	int v_err = DIF(L,ll) + DIF(L,ul) + DIF(R,lr) + DIF(R,ur);
	int h_err = DIF(B,ll) + DIF(B,lr) + DIF(T,ul) + DIF(T,ur);

	XPCell cell;

	if (d_err < v_err && d_err < h_err)
	{
		cell.bk[0]=pal[d_c0][0];
		cell.bk[1]=pal[d_c0][1];
		cell.bk[2]=pal[d_c0][2];

		cell.fg[0]=pal[d_c1][0];
		cell.fg[1]=pal[d_c1][1];
		cell.fg[2]=pal[d_c1][2];

		cell.glyph = d_gl+175;

		dev[0]+=d_dev[0];
		dev[1]+=d_dev[1];
		dev[2]+=d_dev[2];
	}
	else
	if (v_err < h_err)
	{
		cell.bk[0]=R[0];
		cell.bk[1]=R[1];
		cell.bk[2]=R[2];

		cell.fg[0]=L[0];
		cell.fg[1]=L[1];
		cell.fg[2]=L[2];

		cell.glyph = 221;

		DEV(L,ll,dev);
		DEV(L,ul,dev);
		DEV(R,lr,dev);
		DEV(R,ur,dev);
	}
	else
	{
		cell.bk[0]=B[0];
		cell.bk[1]=B[1];
		cell.bk[2]=B[2];

		cell.fg[0]=T[0];
		cell.fg[1]=T[1];
		cell.fg[2]=T[2];

		cell.glyph = 220;

		DEV(B,ll,dev);
		DEV(B,lr,dev);
		DEV(T,ul,dev);
		DEV(T,ur,dev);		
	}


	if (memcmp(cell.bk,cell.fg,3)==0)
	{
		cell.glyph = 219; //32;
	}

	*(ptr++) = cell;
}

int main(int argc, char* argv[])
{
	uint64_t t0 = GetTime();

    if (argc<2)
    {
        printf("usage: png2xp <file.png> [<file.xp>]\n");
        return 1;
    }

    int w,h;
    uint32_t* pix = LoadImg(argv[1],&w,&h);

    if (!pix)
    {
        printf("can't load: %s\n", argv[1]);
        return -1;
    }

    if ((w|h)&1)
    {
        printf("png's width & height must be even but it is: %dx%d\n", w,h);
        return -2;
    }

	const char* xp_name;
	char xp_buff[4096];
	if (argc>=3)
	{
		xp_name = argv[2];
	}
	else
    {
		xp_name = xp_buff;
		sprintf(xp_buff,"%s.xp",argv[1]);
    }

	int dither_blocks[2] = {0};
	int half_blocks[2] = {0};
	int solids = 0;

	uint8_t* hist = (uint8_t*)malloc(256*256*256/8);
	int used = 0;
	memset(hist, 0, 256*256*256/8);

	FILE* xp = fopen(xp_name, "w");
	if (!xp)
    {
        printf("can't write to: %s\n", argv[2]);
        return -3;
    }

	int32_t hdr[] = {-1, 1/*layers*/, w/2, h/2};

	fwrite(hdr, sizeof(hdr), 1, xp);

	XPCell* buf = (XPCell*)malloc(sizeof(XPCell) * (w/2) * (h/2));
	XPCell* ptr = buf;


    uint32_t* org = (uint32_t*)malloc(sizeof(uint32_t)*w*h);
	memcpy(org,pix,sizeof(uint32_t)*w*h); // for error check

	const int pal_size = 216; // ASCIICKER TERM
	uint8_t pal[pal_size][3];
	for (int i=0; i<pal_size; i++)
	{
		int j = i;
		pal[i][0] = j%6*51; j /= 6;
		pal[i][1] = j%6*51; j /= 6;
		pal[i][2] = j%6*51; j /= 6;
	}

	/*
	const int pal_size = 16; // ANSI
	uint8_t pal[pal_size][3] = 
	{
		{0x00,0x00,0x00},{0x55,0x55,0x55},{0x00,0x00,0xAA},{0x55,0x55,0xFF},
		{0x00,0xAA,0x00},{0x55,0xFF,0x55},{0x00,0xAA,0xAA},{0x55,0xFF,0xFF},
		{0xAA,0x00,0x00},{0xFF,0x55,0x55},{0xAA,0x00,0xAA},{0xFF,0x55,0xFF},
		{0xAA,0x55,0x00},{0xFF,0xFF,0x55},{0xAA,0xAA,0xAA},{0xFF,0xFF,0xFF},
	};
	*/

    for (int x=0; x<w; x+=2)
    {
	    for (int y=0; y<h; y+=2)
        {
			int* stat = 0;

			uint32_t src[4] = 
			{
				pix[x + w*y],
				pix[x + 1 + w*y],
				pix[x + w*y + w],
				pix[x + 1 + w*y + w]
			};

			int dev[3] = {0,0,0};
			Do(src,ptr,dev, pal, pal_size);

			switch (ptr->glyph)
			{
				case 219: solids++; break;
				case 220: half_blocks[0]++; break;
				case 221: half_blocks[1]++; break;
				case 176: dither_blocks[0]++; break;
				case 177: dither_blocks[1]++; break;
			}

			int b = ptr->bk[0] | (ptr->bk[1]<<8) | (ptr->bk[2]<<16);
			int f = ptr->fg[0] | (ptr->fg[1]<<8) | (ptr->fg[2]<<16);

			if ( !(hist[b>>3] & (1<<(b&7))) )
			{
				hist[b>>3] |= (1<<(b&7));
				used++;
			}

			if ( !(hist[f>>3] & (1<<(f&7))) )
			{
				hist[f>>3] |= (1<<(f&7));
				used++;
			}

			ptr++;

			// if no dithering
			continue;

			// -4 is 100% , -8 is 50%, ... -1024 should be 0%
			dev[0]/=-6;
			dev[1]/=-6;
			dev[2]/=-6;

			int hlf[3] = {dev[0]/2,dev[1]/2,dev[2]/2};
			dev[0] -= hlf[0];
			dev[1] -= hlf[1];
			dev[2] -= hlf[2];

			if (x<w-2)
			{
				ADD(pix + x+2 + w*y, dev);
				ADD(pix + x+2 + 1 + w*y, dev);
				ADD(pix + x+2 + w*y + w, dev);
				ADD(pix + x+2 + 1 + w*y + w, dev);
			}

			if (y<h-2)
			{
				ADD(pix + x+2*w + w*y, hlf);
				ADD(pix + x+2*w + 1 + w*y, hlf);
				ADD(pix + x+2*w + w*y + w, hlf);
				ADD(pix + x+2*w + 1 + w*y + w, hlf);
			}
        }
    } 

	fwrite(buf,sizeof(XPCell),(w/2)*(h/2),xp);

	fclose(xp);

    free(pix);
	free(hist);

	ptr = buf;
	uint64_t err = 0;
    for (int x=0; x<w; x+=2)
    {
	    for (int y=0; y<h; y+=2)
        {
			uint32_t src[4] = 
			{
				org[x + w*y],
				org[x + 1 + w*y],
				org[x + w*y + w],
				org[x + 1 + w*y + w]
			};

			// calc err
			switch (ptr->glyph)
			{
				case 219: solids++; break;
					err += DIF(ptr->bk, src[0]);
					err += DIF(ptr->bk, src[1]);
					err += DIF(ptr->bk, src[2]);
					err += DIF(ptr->bk, src[3]);
					break;

				case 220: 
					err += DIF(ptr->bk, src[0]);
					err += DIF(ptr->bk, src[1]);
					err += DIF(ptr->fg, src[2]);
					err += DIF(ptr->fg, src[3]);
					break;

				case 221: half_blocks[1]++; break;
					err += DIF(ptr->bk, src[1]);
					err += DIF(ptr->bk, src[3]);
					err += DIF(ptr->fg, src[0]);
					err += DIF(ptr->fg, src[2]);
					break;

				case 176: 
				{	
					uint8_t mix[3] = 
					{
						(uint8_t)sqrt((3*ptr->bk[0]*ptr->bk[0] + 1*ptr->fg[0]*ptr->fg[0])/4),
						(uint8_t)sqrt((3*ptr->bk[1]*ptr->bk[1] + 1*ptr->fg[1]*ptr->fg[1])/4),
						(uint8_t)sqrt((3*ptr->bk[2]*ptr->bk[2] + 1*ptr->fg[2]*ptr->fg[2])/4),
					};
					err += DIF(mix, src[0]);
					err += DIF(mix, src[1]);
					err += DIF(mix, src[2]);
					err += DIF(mix, src[3]);
					break;
				}

				case 177:
				{
					uint8_t mix[3] = 
					{
						(uint8_t)sqrt((2*ptr->bk[0]*ptr->bk[0] + 2*ptr->fg[0]*ptr->fg[0])/4),
						(uint8_t)sqrt((2*ptr->bk[1]*ptr->bk[1] + 2*ptr->fg[1]*ptr->fg[1])/4),
						(uint8_t)sqrt((2*ptr->bk[2]*ptr->bk[2] + 2*ptr->fg[2]*ptr->fg[2])/4),
					};
					err += DIF(mix, src[0]);
					err += DIF(mix, src[1]);
					err += DIF(mix, src[2]);
					err += DIF(mix, src[3]);
					break;
				}
			}

			ptr++;
        }
    } 

	free(org);
	free(buf);

	uint64_t t1 = GetTime();

	printf("Converted in %d us\n", (int)(t1-t0));

	printf(	"solids: %5d\n"
	   		"dith25: %5d\n"
	   		"dith50: %5d\n"
			"half_v: %5d\n"
			"half_h: %5d\n",
			solids, 
			dither_blocks[0], 
			dither_blocks[1], 
			half_blocks[0],
			half_blocks[1] );
 
	printf(	"colors: %5d\n", used);
	printf(	"avrerr: %5.2f\n", (double)err / (w*h*3));

    return 0;
} 

#if 0
// return pal output size, it can be smaller than requested size!
int MakePal(const uint32_t* pix, int wh, uint32_t pal[], int pal_size)
{
	if (pal_size <= 1)
		return 0;

	// 1, extract unique colors and number of occurences
	int* hist = (int*)malloc(256*256*256 * sizeof(int));
	int size = 0;
	memset(hist, -1, 256*256*256 * sizeof(int));
	for (int i=0; i<wh; i++)
	{
		uint32_t p = pix[i] & 0xffffff;
		if ( hist[p]<0 )
			hist[p] = size++;
	}

	if (size<=pal_size)
	{
		size = 0;
		memset(hist, -1, 256*256*256 * sizeof(int));
		for (int i=0; i<wh; i++)
		{
			uint32_t p = pix[i] & 0xffffff;
			if ( hist[p]<0 )
			{
				pal[size] = p;
				hist[p] = size++;
			}
		}
		free(hist);
		return size;		
	}

	struct Point
	{
		int n;
		uint8_t c[3];
	};

	Point* data = (Point*)malloc(sizeof(Point) * size);
	size = 0;
	memset(hist, -1, 256*256*256 * sizeof(int));	
	for (int i=0; i<wh; i++)
	{
		uint32_t p = pix[i] & 0xffffff;
		if ( hist[p]<0 )
		{
			Point* d = data+size;
			d->c[0] = CHN(p,0);
			d->c[1] = CHN(p,1);
			d->c[2] = CHN(p,2);
			d->n = 1;
			hist[p] = size++;
		}
		else
		{
			Point* d = data+hist[p];
			d->n++;			
		}
	}

	free(hist);
	

	struct Centroid
	{
		int c[3];
	};

	Centroid* ctr = (Centroid*)malloc(sizeof(Centroid) * pal_size);

	// k-means plus plus seeding

	uint32_t rnd = pix[rand() % wh];

	ctr->c[0] = CHN(rnd,0);
	ctr->c[1] = CHN(rnd,1);
	ctr->c[2] = CHN(rnd,2);

	for (int i = 1; i<size; i++)
	{
		// find minimum distance to closest 
	}	



	free(data);

	for (int i=0; i<pal_size; i++)
	{
		
	}

	free(ctr);
}

/*

1. do without palette -> all halfblocks
2. pick smallest gradient cell
   finding closest color to 4 samples average 




*/
#endif