
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

	int c[3] =
	{
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,0) * CHN(v,0) + d[0]))),
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,1) * CHN(v,1) + d[1]))),
		MIN(255,MAX(0,(int)sqrt((int)CHN(v,2) * CHN(v,2) + d[2]))),
	};

	*p = c[0] | (c[1]<<8) | (c[2]<<16);
}

uint8_t hack[2][216][216][3];
void INIT_HACK(uint8_t pal[][3], int pal_size)
{
	for (int gl=1; gl<3; gl++)
	{
		int g = gl-1;
		int c0_w = 4 - gl;
		int c1_w = gl;
		for (int c0=0; c0<216; c0++)
		{
			for (int c1=0; c1<216; c1++)
			{
				for (int c=0; c<3; c++)
				{
					hack[g][c0][c1][c] = (uint8_t)(pow( (c0_w * pow(pal[c0][c], 2.2) + c1_w * pow(pal[c1][c], 2.2)) / 4, 1.0/2.2 ) + 0.499);
				}
			}
		}
	}
}

void HACK(uint8_t* G, int gl, uint8_t c0, uint8_t c1)
{
	int g = gl-1;
	G[0] = hack[g][c0][c1][0];
	G[1] = hack[g][c0][c1][1];
	G[2] = hack[g][c0][c1][2];
}

uint32_t Make(uint32_t src, XPCell* ptr, uint8_t pal[][3], int pal_size)
{
	int d_err;
	int d_gl=0, d_c0, d_c1;

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
				uint8_t G[3];
				HACK(G, gl,c0,c1);

				// calc err
				int g_err = 4 * DIF(G,src);

				if (!d_gl || g_err < d_err)
				{
					d_err = g_err;
					d_gl = gl;
					d_c0 = c0;
					d_c1 = c1;
				}
			}
		}
	}

	int e = -1;
	int p;
	int c[3] = { CHN(src,0), CHN(src,1), CHN(src,2) };
	for (int i=0; i<pal_size; i++)
	{
		int ie = 2*ABS(c[0]-pal[i][0]) + 3*ABS(c[1]-pal[i][1]) + ABS(c[2]-pal[i][2]);
		if (e<0 || ie<e)
		{
			e = ie;
			p = i;
		}
	}		

	return d_c0 == d_c1 ? d_c0 | (d_c1<<8) | (p<<16) : d_c0 | (d_c1<<8) | (p<<16) | ((d_gl-1)<<24);
}

// return err
int Do(uint32_t src[4], XPCell* ptr, int dev[3], uint8_t pal[][3], int pal_size, uint32_t* xxx, int xxx_step)
{
	uint32_t ll = src[0];
	uint32_t lr = src[1];
	uint32_t ul = src[2];
	uint32_t ur = src[3];

	int d_err;
	int d_gl=0, d_c0, d_c1;
	int d_dev[3];

	// target rgb
	uint8_t G[3] = 
	{
		(uint8_t)pow( (pow(CHN(ll,0),2.2) + pow(CHN(ul,0),2.2) + pow(CHN(lr,0),2.2) + pow(CHN(ur,0),2.2)) / 4, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ll,1),2.2) + pow(CHN(ul,1),2.2) + pow(CHN(lr,1),2.2) + pow(CHN(ur,1),2.2)) / 4, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ll,2),2.2) + pow(CHN(ul,2),2.2) + pow(CHN(lr,2),2.2) + pow(CHN(ur,2),2.2)) / 4, 1.0/2.2 ),
	};	

	int xxx_offs = xxx_step / 2;
	int xxx_size = 256 / xxx_step;
	G[0] = (G[0] + xxx_offs ) / xxx_step;
	G[1] = (G[1] + xxx_offs ) / xxx_step;
	G[2] = (G[2] + xxx_offs ) / xxx_step;

	uint32_t xxx_slot = xxx[ G[0] + G[1]*xxx_size + G[2]*xxx_size*xxx_size ];

	d_c0 = xxx_slot & 0xFF;
	d_c1 = (xxx_slot >> 8) & 0xFF;
	d_gl = ((xxx_slot >> 24) & 0xFF) + 1;

	// reconstructed rgb
	HACK(G, d_gl,d_c0,d_c1);

	int g_err = DIF(G,ll) + DIF(G,lr) + DIF(G,ul) + DIF(G,ur);


	// find best 2 color indices C0, C1
	// and mixing factor  3/4 C0 + 1/4 C1 or 2/4 C0 + 2/4 C1

	// find out if using half-blocks is beter than dither blocks

	uint8_t L[3] = 
	{
		(uint8_t)pow( (pow(CHN(ll,0),2.2) + pow(CHN(ul,0),2.2)) / 2 , 1.0/2.2),
		(uint8_t)pow( (pow(CHN(ll,1),2.2) + pow(CHN(ul,1),2.2)) / 2 , 1.0/2.2),
		(uint8_t)pow( (pow(CHN(ll,2),2.2) + pow(CHN(ul,2),2.2)) / 2 , 1.0/2.2),
	};

	L[0] = (L[0] + xxx_offs ) / xxx_step;
	L[1] = (L[1] + xxx_offs ) / xxx_step;
	L[2] = (L[2] + xxx_offs ) / xxx_step;
	xxx_slot = xxx[ L[0] + L[1]*xxx_size + L[2]*xxx_size*xxx_size ];
	xxx_slot = (xxx_slot >> 16) & 0xFF;
	L[0] = pal[xxx_slot][0];
	L[1] = pal[xxx_slot][1];
	L[2] = pal[xxx_slot][2];	

	uint8_t R[3] = 
	{
		(uint8_t)pow( pow(CHN(lr,0),2.2) + pow(CHN(ur,0),2.2) / 2, 1.0/2.2 ),
		(uint8_t)pow( pow(CHN(lr,1),2.2) + pow(CHN(ur,1),2.2) / 2, 1.0/2.2 ),
		(uint8_t)pow( pow(CHN(lr,2),2.2) + pow(CHN(ur,2),2.2) / 2, 1.0/2.2 ),
	};
	R[0] = (R[0] + xxx_offs ) / xxx_step;
	R[1] = (R[1] + xxx_offs ) / xxx_step;
	R[2] = (R[2] + xxx_offs ) / xxx_step;
	xxx_slot = xxx[ R[0] + R[1]*xxx_size + R[2]*xxx_size*xxx_size ];
	xxx_slot = (xxx_slot >> 16) & 0xFF;
	R[0] = pal[xxx_slot][0];
	R[1] = pal[xxx_slot][1];
	R[2] = pal[xxx_slot][2];	

	uint8_t B[3] =
	{
		(uint8_t)pow( (pow(CHN(ll,0),2.2) + pow(CHN(lr,0),2.2)) / 2, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ll,1),2.2) + pow(CHN(lr,1),2.2)) / 2, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ll,2),2.2) + pow(CHN(lr,2),2.2)) / 2, 1.0/2.2 ),
	};
	B[0] = (B[0] + xxx_offs ) / xxx_step;
	B[1] = (B[1] + xxx_offs ) / xxx_step;
	B[2] = (B[2] + xxx_offs ) / xxx_step;
	xxx_slot = xxx[ B[0] + B[1]*xxx_size + B[2]*xxx_size*xxx_size ];
	xxx_slot = (xxx_slot >> 16) & 0xFF;
	B[0] = pal[xxx_slot][0];
	B[1] = pal[xxx_slot][1];
	B[2] = pal[xxx_slot][2];	

	uint8_t T[3] =
	{ 
		(uint8_t)pow( (pow(CHN(ul,0),2.2) + pow(CHN(ur,0),2.2)) / 2, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ul,1),2.2) + pow(CHN(ur,1),2.2)) / 2, 1.0/2.2 ),
		(uint8_t)pow( (pow(CHN(ul,2),2.2) + pow(CHN(ur,2),2.2)) / 2, 1.0/2.2 ),
	};
	T[0] = (T[0] + xxx_offs ) / xxx_step;
	T[1] = (T[1] + xxx_offs ) / xxx_step;
	T[2] = (T[2] + xxx_offs ) / xxx_step;
	xxx_slot = xxx[ T[0] + T[1]*xxx_size + T[2]*xxx_size*xxx_size ];
	xxx_slot = (xxx_slot >> 16) & 0xFF;
	T[0] = pal[xxx_slot][0];
	T[1] = pal[xxx_slot][1];
	T[2] = pal[xxx_slot][2];	


	int v_err = DIF(L,ll) + DIF(L,ul) + DIF(R,lr) + DIF(R,ur);
	int h_err = DIF(B,ll) + DIF(B,lr) + DIF(T,ul) + DIF(T,ur);

	XPCell cell;
	int err = -1;

	if (d_err < v_err && d_err < h_err)
	{
		err = d_err;
		
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
		err = v_err;

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
		err = h_err;

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
	return err;
}

int main(int argc, char* argv[])
{
	uint64_t t0 = GetTime();

    if (argc<2)
    {
        printf("usage: \nTEACH:   png2xp <step(1-17)> <out_file.plt> \nCONVERT: png2xp <in_file.plt> <in_file.png> <out_file.xp>\n");
        return 1;
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

	const int pal_size = 216; // ASCIICKER TERM
	uint8_t pal[pal_size][3];
	for (int i=0; i<pal_size; i++)
	{
		int j = i;
		pal[i][0] = j%6*51; j /= 6;
		pal[i][1] = j%6*51; j /= 6;
		pal[i][2] = j%6*51; j /= 6;
	}

	INIT_HACK(pal,pal_size);

	if (argc==3)
	{
		int step = atoi(argv[1]);
		if (step<1 || step>17)
		{
			printf("step = %d is invalid, keep in range 1-17\n", step);
			return -1;
		}

		// make palette file

		FILE* plt = fopen(argv[2], "w");
		if (!pal)
		{
			printf("can't write to: %s\n", argv[2]);
			return -3;
		}

		// write step size
		fwrite(&step,4,1,plt);

		// write reverse lookup table
		for (int B=0; B<256; B+=step)
		for (int G=0; G<256; G+=step)
		for (int R=0; R<256; R+=step)
		{
			uint32_t p = R | (G<<8) | (B<<16);

			if ((p&0xFFFF) == 0)
				printf("%d/%d\n",(p>>16)+1,256);

			XPCell cell;
			uint32_t w = Make(p, &cell, pal, pal_size);

			fwrite(&w,4,1,plt);
		}

		fclose(plt);
		return 0;
	}

	// load plt file
	FILE* plt = fopen(argv[1],"r");
	if (!plt)
	{
		printf("Can't read plt file: %s", argv[1]);
		return -1;
	}

	int ok=0;
	int32_t xxx_step;
	ok = fread(&xxx_step,4,1,plt);
	if (!ok)
		return -7;
	int xxx_steps = 256 / xxx_step;
	uint32_t* xxx = (uint32_t*)malloc(xxx_steps*xxx_steps*xxx_steps * sizeof(uint32_t));
	ok = fread(xxx,xxx_steps*xxx_steps*xxx_steps,1,plt);
	if (!ok)
		return -7;
	fclose(plt);

    int w,h;
    uint32_t* pix = LoadImg(argv[2],&w,&h);

    if (!pix)
    {
		free(xxx);
        printf("can't load: %s\n", argv[1]);
        return -1;
    }

    if ((w|h)&1)
    {
		free(xxx);
        printf("png's width & height must be even but it is: %dx%d\n", w,h);
        return -2;
    }

	const char* xp_name;
	char xp_buff[4096];
	if (argc>=4)
	{
		xp_name = argv[3];
	}
	else
    {
		xp_name = xp_buff;
		sprintf(xp_buff,"%s.xp",argv[2]);
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
		free(xxx);
        printf("can't write to: %s\n", argv[2]);
        return -3;
    }

	int32_t hdr[] = {-1, 1/*layers*/, w/2, h/2};

	XPCell* buf = (XPCell*)malloc(sizeof(XPCell) * (w/2) * (h/2));
	XPCell* ptr = buf;

	uint64_t err = 0;

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
			err += Do(src,ptr,dev, pal, pal_size, xxx, xxx_step);

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

			switch (ptr->glyph)
			{
				case 219: solids++; break;
				case 220: half_blocks[0]++; break;
				case 221: half_blocks[1]++; break;
				case 176: dither_blocks[0]++; break;
				case 177: dither_blocks[1]++; break;
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

	fwrite(hdr,4,4,xp);
	fwrite(buf,sizeof(XPCell),(w/2)*(h/2),xp);

	fclose(xp);

    free(pix);
	free(hist);

	free(buf);
	free(xxx);	

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
		int centroid;
		int distance;
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