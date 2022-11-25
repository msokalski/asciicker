
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/*
// 64C test

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

int32_t ABS(int32_t v) 
{
	return v<0 ? -v : v;
}

uint32_t DIF(uint8_t c[3], uint32_t r) 
{
	return 
	    1*ABS((int32_t)(c[0])-(int32_t)(CHN(r,0))) +
    	1*ABS((int32_t)(c[1])-(int32_t)(CHN(r,1))) +
    	1*ABS((int32_t)(c[2])-(int32_t)(CHN(r,2)));
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

    for (int x=0; x<w; x+=2)
    {
	    for (int y=0; y<h; y+=2)
        {
			int* stat = 0;

            uint32_t ll = pix[x + w*y];
            uint32_t lr = pix[x + 1 + w*y];
            uint32_t ul = pix[x + w*y + w];
            uint32_t ur = pix[x + 1 + w*y + w];

            uint8_t M[3] = 
			{
            	(uint8_t)((CHN(ll,0) + CHN(lr,0) + CHN(ul,0) + CHN(ur,0) + 2) / 4),
            	(uint8_t)((CHN(ll,1) + CHN(lr,1) + CHN(ul,1) + CHN(ur,1) + 2) / 4),
            	(uint8_t)((CHN(ll,2) + CHN(lr,2) + CHN(ul,2) + CHN(ur,2) + 2) / 4),
			};

            uint8_t ML[3] = { (uint8_t)PAL_LO(M[0]), (uint8_t)PAL_LO(M[1]), (uint8_t)PAL_LO(M[2]) };
            uint8_t MH[3] = { (uint8_t)PAL_HI(M[0]), (uint8_t)PAL_HI(M[1]), (uint8_t)PAL_HI(M[2]) };

			uint8_t D[8][3]=
			{
				{ ML[0], ML[1], ML[2] },
				{ ML[0], ML[1], MH[2] },
				{ ML[0], MH[1], ML[2] },
				{ ML[0], MH[1], MH[2] },
				{ MH[0], ML[1], ML[2] },
				{ MH[0], ML[1], MH[2] },
				{ MH[0], MH[1], ML[2] },
				{ MH[0], MH[1], MH[2] },
			};

			int d_err;
			int d_gl=0, d_c0, d_c1;

			// find best 2 color indices C0, C1
			// and mixing factor  3/4 C0 + 1/4 C1 or 2/4 C0 + 2/4 C1
			for (int gl=1; gl<3; gl++)
			{
				int c0_w = 4-gl;
				int c1_w = gl;
				for (int c0 = 0; c0<8; c0++)
				{
					for (int c1 = 0; c1<8; c1++)
					{
						uint8_t G[3] =
						{
							(uint8_t)((c0_w * D[c0][0] + c1_w * D[c1][0])/4),
							(uint8_t)((c0_w * D[c0][1] + c1_w * D[c1][1])/4),
							(uint8_t)((c0_w * D[c0][2] + c1_w * D[c1][2])/4),
						};

						// calc err
						int g_err = DIF(G,ll) + DIF(G,lr) + DIF(G,ul) + DIF(G,ur);

				

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

			// find out if using half-blocks is beter than dither blocks

            uint8_t L[3] = 
            {
                (uint8_t)PAL( (CHN(ll,0) + CHN(ul,0) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ll,1) + CHN(ul,1) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ll,2) + CHN(ul,2) + 1) / 2 ),
            };

            uint8_t R[3] = 
            {
                (uint8_t)PAL( (CHN(lr,0) + CHN(ur,0) + 1) / 2 ),
                (uint8_t)PAL( (CHN(lr,1) + CHN(ur,1) + 1) / 2 ),
                (uint8_t)PAL( (CHN(lr,2) + CHN(ur,2) + 1) / 2 ),
            };

            uint8_t B[3] =
            {
                (uint8_t)PAL( (CHN(ll,0) + CHN(lr,0) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ll,1) + CHN(lr,1) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ll,2) + CHN(lr,2) + 1) / 2 ),
            };

            uint8_t T[3] =
            { 
                (uint8_t)PAL( (CHN(ul,0) + CHN(ur,0) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ul,1) + CHN(ur,1) + 1) / 2 ),
                (uint8_t)PAL( (CHN(ul,2) + CHN(ur,2) + 1) / 2 ),
            };


            int v_err = DIF(L,ll) + DIF(L,ul) + DIF(R,lr) + DIF(R,ur);
            int h_err = DIF(B,ll) + DIF(B,lr) + DIF(T,ul) + DIF(T,ur);

			XPCell cell;

			if (d_err < v_err && d_err < h_err)
			{
                cell.bk[0]=D[d_c0][0];
				cell.bk[1]=D[d_c0][1];
				cell.bk[2]=D[d_c0][2];

                cell.fg[0]=D[d_c1][0];
				cell.fg[1]=D[d_c1][1];
				cell.fg[2]=D[d_c1][2];

				cell.glyph = d_gl+175;
				stat = dither_blocks+d_gl-1;
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
				stat = half_blocks+1;
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
				stat = half_blocks+0;
			}


			if (memcmp(cell.bk,cell.fg,3)==0)
			{
				cell.glyph = 219; //32;
				stat = &solids;
			}

			(*stat)++;


			int b = cell.bk[0] | (cell.bk[1]<<8) | (cell.bk[2]<<16);
			int f = cell.fg[0] | (cell.fg[1]<<8) | (cell.fg[2]<<16);

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
		
			fwrite(&cell,sizeof(XPCell),1,xp);
        }
    } 

	fclose(xp);

    free(pix);
	free(hist);

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

    return 0;
}