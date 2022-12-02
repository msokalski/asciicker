

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <thread>

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

struct Gamma
{
    int16_t  dec[256];  // 0..8192 incl
    uint8_t  enc[8193]; // 0..255 incl

    Gamma()
    {
        for (int i=0; i<256; i++)
        {
            double t = i / 255.0;
            t = t >= 0.04045 ? pow((t+0.055)/1.055, 2.4) : t/12.92;
            dec[i] = (int16_t)round(t * 8192.0);
        } 
        
        for (int i=0; i<=8192; i++)
        {
            double t = i / 8192.0;
            t = t > 0.0031308 ? 1.055*pow(t, 1.0/2.4) - 0.055 : 12.92*t;
            enc[i] = (uint8_t)round(255.0 * t);
        }
    }
};

static Gamma gamma_tables;

/*
double DEC(uint8_t e)
{
	double t = e / 255.0;
	return t >= 0.04045 ? pow((t+0.055)/1.055, 2.4) : t/12.92;
}

uint8_t ENC(double d)
{
	if (d<0)
		d=0;
	if (d>1)
		d=1;
	return (uint8_t)round(255.0 * (d > 0.0031308 ? 1.055*pow(d, 1.0/2.4) - 0.055 : 12.92*d));
}
*/

int16_t DEC(uint8_t e)
{
	return gamma_tables.dec[e];
}

uint8_t ENC(int16_t d)
{
	if (d<0)
		return 0;
	if (d>8192)
		return 255;
	return gamma_tables.enc[d];
}

uint32_t ERR(uint8_t c[3], uint8_t r[3]) 
{
	// let's try CIE76
	/*
	struct LAB
	{
		static double f(double t)
		{
			const double d = 6.0/29;
			return t > d*d*d ? pow(t,1.0/3.0) : t / (3*d*d + 4.0/29.0);
		}

		LAB(uint8_t c[3])
		{
			double R = DEC(c[0]);
			double G = DEC(c[1]);
			double B = DEC(c[2]);

			double X = 0.4124564 * R + 0.3575761 * G + 0.1804375 * B;
			double Y = 0.2126729 * R + 0.7151522 * G + 0.0721750 * B;
			double Z = 0.0193339 * R + 0.1191920 * G + 0.9503041 * B;

			const double Xn =  96.4214;			
			const double Yn = 100.0000;
			const double Zn = 108.8840;

			L = 116 * f(Y/Yn) - 16;
			a = 500 * (f(X/Xn) - f(Y/Yn));
			b = 200 * (f(Y/Yn) - f(Z/Zn));
		}

		double L,a,b;
	};

	LAB q1{c},q2{r};

	double da = q1.a-q2.a;
	double db = q1.b-q2.b;
	double dL = q1.L-q2.L;
	double err = sqrt(dL*dL + da*da + db*db);

	return err;
	*/ 

	// lets try euclidean
	/*
	int dr = (int32_t)(c[0])-(int32_t)(r[0]);
	int dg = (int32_t)(c[1])-(int32_t)(r[1]);
	int db = (int32_t)(c[2])-(int32_t)(r[2]);

	// fast sqrt table[MAX_SUM], MAX_SUM = 2*255*255+3*255*255+255*255 = 6*255*255
	return //(int)sqrt
		(2*dr*dr+3*dg*dg+db*db);
	*/

	uint32_t chr =  
	    2*ABS((int32_t)(c[0])-(int32_t)(r[0])) +
    	3*ABS((int32_t)(c[1])-(int32_t)(r[1])) +
    	1*ABS((int32_t)(c[2])-(int32_t)(r[2]));

	int lum_c = 2*c[0] + 3*c[1] + c[2];
	int lum_r = 2*r[0] + 3*r[1] + r[2];

	return chr;// + ABS(lum_c-lum_r);
}

uint32_t ERR(uint8_t c[3], uint32_t r) 
{
	uint8_t d[3] = {CHN(r,0),CHN(r,1),CHN(r,2)};
	return ERR(c,d);
}

void DEV(uint8_t c[3], uint32_t r, int d[3])
{
	d[0] += (int)DEC(c[0]) - DEC(CHN(r,0));
	d[1] += (int)DEC(c[1]) - DEC(CHN(r,1));
	d[2] += (int)DEC(c[2]) - DEC(CHN(r,2));
}

void ADD(uint32_t* p, int d[3])
{
	uint32_t v = *p;

	uint8_t c[3] =
	{
		ENC(DEC(CHN(v,0)) + d[0]),
		ENC(DEC(CHN(v,1)) + d[1]),
		ENC(DEC(CHN(v,2)) + d[2]),
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
					hack[g][c0][c1][c] = ENC( (c0_w * DEC(pal[c0][c]) + c1_w * DEC(pal[c1][c])) / 4 );
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

uint32_t Make(uint32_t src, uint8_t pal[][3], int pal_size)
{
	uint32_t d_err;
	int d_gl=0, d_c0=0, d_c1=0;

	// find best 2 color indices C0, C1
	// and mixing factor  3/4 C0 + 1/4 C1 or 2/4 C0 + 2/4 C1
	for (int gl=1; gl<3; gl++)
	{
		int c0_w = 4-gl;
		int c1_w = gl;
		for (int c0 = 0; c0<pal_size; c0++)
		{
			int t = c0;
			int r0 = t%6; t/=6;
			int g0 = t%6; t/=6;
			int b0 = t;

			for (int c1 = 0; c1<pal_size; c1++)
			{
				t = c1;
				int r1 = t%6; t/=6;
				int g1 = t%6; t/=6;
				int b1 = t;

				// TODO:
				// reimplement max dither steps limit into palette lookup. 
				// if there exist a solid color in the palette such
				// difference from current (reconstructed) half-tone to 
				// that solid color is smaller than to the next or prev
				// half-tone shade (gl +/- 1) we should invalidate this
				// half-tone (solid color will be used instead)

				// MAX DITHER STEPS LIMIT = 1
				//if (std::abs(r0-r1)>1 || std::abs(g0-g1)>1 || std::abs(b0-b1)>1)
				//	continue;

				// MAX DITHER STEPS LIMIT = 2 
				if (std::abs(r0-r1)>2 || std::abs(g0-g1)>2 || std::abs(b0-b1)>2)
					continue;

				uint8_t G[3];
				HACK(G, gl,c0,c1);

				// calc err
				uint32_t g_err = 4 * ERR(G,src);

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

	uint32_t e = 0;
	int p = -1;
	uint8_t c[3] = { CHN(src,0), CHN(src,1), CHN(src,2) };
	for (int i=0; i<pal_size; i++)
	{
		uint32_t ie = ERR(c, pal[i]);
		if (p<0 || ie<e)
		{
			e = ie;
			p = i;
		}
	}		

	return d_c0 == d_c1 ? d_c0 | (d_c1<<8) | (p<<16) : d_c0 | (d_c1<<8) | (p<<16) | ((d_gl-1)<<24);
}

// return err
uint32_t Do(uint32_t src[4], XPCell* ptr, int dev[3], uint8_t pal[][3], int pal_size, uint32_t* xxx, int xxx_step)
{
	uint32_t ll = src[0];
	uint32_t lr = src[1];
	uint32_t ul = src[2];
	uint32_t ur = src[3];

	uint32_t d_err;
	int d_gl=0, d_c0, d_c1;

	// target rgb
	uint8_t G[3] = 
	{
		ENC( (DEC(CHN(ll,0)) + DEC(CHN(ul,0)) + DEC(CHN(lr,0)) + DEC(CHN(ur,0))) / 4 ),
		ENC( (DEC(CHN(ll,1)) + DEC(CHN(ul,1)) + DEC(CHN(lr,1)) + DEC(CHN(ur,1))) / 4 ),
		ENC( (DEC(CHN(ll,2)) + DEC(CHN(ul,2)) + DEC(CHN(lr,2)) + DEC(CHN(ur,2))) / 4 ),
	};	

	int xxx_offs = xxx_step / 2;
	int xxx_size = 255 / xxx_step + 1;
	G[0] = (G[0] + xxx_offs ) / xxx_step;
	G[1] = (G[1] + xxx_offs ) / xxx_step;
	G[2] = (G[2] + xxx_offs ) / xxx_step;

	uint32_t xxx_slot = xxx[ G[0] + G[1]*xxx_size + G[2]*xxx_size*xxx_size ];

	d_c0 = xxx_slot & 0xFF;
	d_c1 = (xxx_slot >> 8) & 0xFF;
	d_gl = ((xxx_slot >> 24) & 0xFF) + 1;

	// reconstructed rgb
	HACK(G, d_gl,d_c0,d_c1);

	d_err = ERR(G,ll) + ERR(G,lr) + ERR(G,ul) + ERR(G,ur);

	// find best 2 color indices C0, C1
	// and mixing factor  3/4 C0 + 1/4 C1 or 2/4 C0 + 2/4 C1

	// find out if using half-blocks is beter than dither blocks

	uint8_t L[3] = 
	{
		ENC( (DEC(CHN(ll,0)) + DEC(CHN(ul,0))) / 2),
		ENC( (DEC(CHN(ll,1)) + DEC(CHN(ul,1))) / 2),
		ENC( (DEC(CHN(ll,2)) + DEC(CHN(ul,2))) / 2),
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
		ENC( (DEC(CHN(lr,0)) + DEC(CHN(ur,0))) / 2),
		ENC( (DEC(CHN(lr,1)) + DEC(CHN(ur,1))) / 2),
		ENC( (DEC(CHN(lr,2)) + DEC(CHN(ur,2))) / 2),
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
		ENC( (DEC(CHN(ll,0)) + DEC(CHN(lr,0))) / 2),
		ENC( (DEC(CHN(ll,1)) + DEC(CHN(lr,1))) / 2),
		ENC( (DEC(CHN(ll,2)) + DEC(CHN(lr,2))) / 2),
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
		ENC( (DEC(CHN(ul,0)) + DEC(CHN(ur,0))) / 2),
		ENC( (DEC(CHN(ul,1)) + DEC(CHN(ur,1))) / 2),
		ENC( (DEC(CHN(ul,2)) + DEC(CHN(ur,2))) / 2),
	};
	T[0] = (T[0] + xxx_offs ) / xxx_step;
	T[1] = (T[1] + xxx_offs ) / xxx_step;
	T[2] = (T[2] + xxx_offs ) / xxx_step;
	xxx_slot = xxx[ T[0] + T[1]*xxx_size + T[2]*xxx_size*xxx_size ];
	xxx_slot = (xxx_slot >> 16) & 0xFF;
	T[0] = pal[xxx_slot][0];
	T[1] = pal[xxx_slot][1];
	T[2] = pal[xxx_slot][2];	


	uint32_t v_err = ERR(L,ll) + ERR(L,ul) + ERR(R,lr) + ERR(R,ur);
	uint32_t h_err = ERR(B,ll) + ERR(B,lr) + ERR(T,ul) + ERR(T,ur);

	XPCell cell;
	uint32_t err = 0;

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

		DEV(G, ll, dev);
		DEV(G, lr, dev);
		DEV(G, ul, dev);
		DEV(G, ur, dev);
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
		pal[i][2] = j%6*51; j /= 6;
		pal[i][1] = j%6*51; j /= 6;
		pal[i][0] = j%6*51; j /= 6;
	}

	INIT_HACK(pal,pal_size);

	if (argc==3)
	{
		int step = atoi(argv[1]);
		if (step<1 || step>255)
		{
			printf("step = %d is invalid, keep in range 1-255\n", step);
			return -1;
		}

		// make palette file

		FILE* plt = fopen(argv[2], "wb");
		if (!pal)
		{
			printf("can't write to: %s\n", argv[2]);
			return -3;
		}

		// write step size
		fwrite(&step,4,1,plt);

		int steps = 255 / step + 1;
		uint32_t* xxx = (uint32_t*)malloc(sizeof(uint32_t) * steps * steps * steps);

		struct Task
		{
			int B;
			uint32_t* ptr;
			uint8_t (*pal)[3];
			int pal_size;
			int step;
			std::thread* thr;

			static void Run(Task* t)
			{
				int B = t->B;
				uint32_t* ptr = t->ptr;
				int step = t->step;
				int pal_size = t->pal_size;
				uint8_t (*pal)[3] = t->pal;

				for (int G = 0; G < 256; G += step)
				{
					for (int R = 0; R < 256; R += step)
					{
						uint32_t p = R | (G << 8) | (B << 16);

						if ((R & 0xFF) == 0)
							printf("%d: %d/%d\n", B, G, 256);

						uint32_t w = Make(p, pal, pal_size);

						*ptr = w;
						ptr++;

						//fwrite(&w,4,1,plt);
					}
				}
			}
		};

		Task task[256];

		// write reverse lookup table
		for (int B=0; B<256; B+=step)
		{
			task[B].B = B;
			task[B].ptr = xxx + steps * steps * (B / step);
			task[B].pal = pal;
			task[B].pal_size = pal_size;
			task[B].step = step;
			task[B].thr = new std::thread(Task::Run, task + B);
		}

		for (int B = 0; B < 256; B += step)
		{
			task[B].thr->join();
			delete task[B].thr;
		}

		fwrite(xxx, sizeof(uint32_t)* steps* steps* steps, 1, plt);
		free(xxx);
		fclose(plt);
		return 0;
	}

	// load plt file
	FILE* plt = fopen(argv[1],"rb");
	if (!plt)
	{
		printf("Can't read plt file: %s", argv[1]);
		return -1;
	}

	size_t ok=0;
	int32_t xxx_step;
	ok = fread(&xxx_step,4,1,plt);
	if (!ok)
		return -7;
	int xxx_steps = 255 / xxx_step + 1;
	uint32_t* xxx = (uint32_t*)malloc(xxx_steps*xxx_steps*xxx_steps * sizeof(uint32_t));
	ok = fread(xxx,xxx_steps*xxx_steps*xxx_steps * sizeof(uint32_t),1,plt);
	if (!ok)
		return -7;
	fclose(plt);

    int w,h;
    uint32_t* pix = LoadImg(argv[2],&w,&h);

    if (!pix)
    {
		free(xxx);
        printf("can't load: %s\n", argv[2]);
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
		sprintf(xp_buff,"%s.xp",argv[3]);
    }

	int dither_blocks[2] = {0};
	int half_blocks[2] = {0};
	int solids = 0;

	uint8_t* hist = (uint8_t*)malloc(256*256*256/8);
	int used = 0;
	memset(hist, 0, 256*256*256/8);

	FILE* xp = fopen(xp_name, "wb");
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

			if ((x==38*2 || x==39*2) && y==8*2)
			{
				printf(".\n");
			}

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

			// -4 is 100%
			// (atkinson dither: 6x 1/8)
			dev[0] /= -4 * 8;
			dev[1] /= -4 * 8;
			dev[2] /= -4 * 8;

			if (x<w-2)
			{
				ADD(pix + x+2 + w*y, dev);
				ADD(pix + x+2 + 1 + w*y, dev);
				ADD(pix + x+2 + w*y + w, dev);
				ADD(pix + x+2 + 1 + w*y + w, dev);

				if (x<w-4)
				{
					ADD(pix + x+4 + w*y, dev);
					ADD(pix + x+4 + 1 + w*y, dev);
					ADD(pix + x+4 + w*y + w, dev);
					ADD(pix + x+4 + 1 + w*y + w, dev);
				}
			}

			if (y<h-2)
			{
				if (x>=2)
				{
					ADD(pix + x+2*w + -2 + w*y, dev);
					ADD(pix + x+2*w + -1 + w*y, dev);
					ADD(pix + x+2*w + -2 + w*y + w, dev);
					ADD(pix + x+2*w + -1 + w*y + w, dev);
				}

				ADD(pix + x+2*w + w*y, dev);
				ADD(pix + x+2*w + 1 + w*y, dev);
				ADD(pix + x+2*w + w*y + w, dev);
				ADD(pix + x+2*w + 1 + w*y + w, dev);

				if (x<w-2)
				{
					ADD(pix + x+2*w + 2 + w*y, dev);
					ADD(pix + x+2*w + 3 + w*y, dev);
					ADD(pix + x+2*w + 2 + w*y + w, dev);
					ADD(pix + x+2*w + 3 + w*y + w, dev);
				}

				if (y<h-4)
				{
					ADD(pix + x+4*w + w*y, dev);
					ADD(pix + x+4*w + 1 + w*y, dev);
					ADD(pix + x+4*w + w*y + w, dev);
					ADD(pix + x+4*w + 1 + w*y + w, dev);
				}
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