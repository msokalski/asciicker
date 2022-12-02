#include <math.h>

#include "mainmenu.h"
#include "enemygen.h"
#include "fast_rand.h"
#include "font1.h"

#include "upng.h"
#include "rgba8.h"

extern Game* game;
extern Terrain* terrain;
extern World* world;
extern Material mat[256];
extern char base_path[1024];

static int  game_loading = 0; // 0-not_loaded, 1-load requested, 2-loading, 3-loaded
static bool show_continue = false;

static uint64_t mainmenu_stamp = 0;

////////////////////////////////////////
static uint32_t* xxx_table = 0;
static uint32_t  xxx_step = 0;
static uint32_t  xxx_offs = 0;
static uint32_t  xxx_size = 0;
static uint32_t  xxx_size2 = 0;

static uint16_t* menu_bk_img=0;
static int menu_bk_width=0;
static int menu_bk_height=0;

static const int pal_size = 216;
static uint8_t pal[pal_size][3] = {{0}};
static uint8_t half_tone[2][216][216][3] = {{{{0}}}};

static Sprite* menu_logo_sprite = 0;


struct Manifest
{
    const char* xp;    // this should be embedded using --preload-file
    const char* title; // short title (big font)
    const char* desc;  // long description (small font)
    const char* a3d;   // world file
    const char* ajs;   // game script 
    void* cookie;      // this contains menu runtime data (loaded sprites etc. or ad cookie)

    // if this is terminator, all fileds should be null
    // if this is dir, a3d must be null and ajs must point to Manifest array of children
    // if this is server based game, ajs must be null and a3d must contain address
    // if this is "coming soon" / ad, both a3d and ajs must be null, cookie may point to url 

    /*
    .ajs is required to initialize world with:
    - ak.setWater (55)
    - ak.setDir   (0)
    - ak.setYaw   (45)
    - ak.setPos   (0,15,0)
    - ak.setLight (1,0,1,.5)
    */

    /*
    .ajs optionally may hook these 2 to handle loading/saving game state
    - function onRead(arrbuf) -> applies modifications stored in arrbuf to the world
    - function onWrite() -> stores modified world state in array buffer, returns arrbuf
    // - read will be called only during fresh page load -> CreateGame
    // - write will be called on 'beforeunload' event or when process is about to
    //   terminate when there's game currently playing or is suspended by main menu
    */
};


char cookie_ad[] = "https://twitter.com/mrgumix";

static Manifest dev_toys_manifest_arr[]=
{
    {
        "dev_toy.xp",
        "DEV TOY1",
        "Example showing thing1, source: https://...dev_toy1",
        "dev_toys.a3d",
        "dev_toy1.ajs",
        0 // cookie
    },

    {
        "dev_toy.xp",
        "DEV TOY2",
        "Example showing thing2, source: https://...dev_toy2",
        "dev_toys.a3d",
        "dev_toy2.ajs",
        0 // cookie
    },

    {
        "dev_toy.xp",
        "DEV TOY3",
        "Example showing thing3, source: https://...dev_toy3",
        "dev_toys.a3d",
        "dev_toy3.ajs",
        0 // cookie
    },

    {0} // terminator
};

static Manifest manifest[]=
{
    {
        "tutorial.xp",
        "CONTROLS TUTORIAL ",
        "Tutorial teaching you how to control the game",
        "tutorial.a3d",
        "tutorial.ajs",
        0 // cookie
    },

    {
        "y9.xp",
        "Y9 DEMO",
        "Latest official demo world containig few playable quest",
        "game_map_y9.a3d",
        "game_map_y9.ajs",
        0 // cookie
    },

    {
        "y9_online.xp",
        "Y9 MULTIPLAYER DEMO",
        "Latest official multiplayer demo",
        "y9_server", // if ajs (below) is null, this is wss/endpoint 
        0, // real a3d and ajs files will be sent by server during joining
        0 // cookie
    },

    {
        "dev_toys.xp",
        "DEV TOYS",
        "Latest official dev toys",
        0, // this is directory!
        (const char*)dev_toys_manifest_arr, // and here are children
        0 // cookie
    },

    {
        "gumix.xp",
        "GUMIX NEWS",
        "",
        0,        // this
        0,        // is an ad
        cookie_ad // with a cookie
    },

    {0} // terminator
};

struct Gamma
{
    uint16_t dec[256];  // 0..8192 incl
    uint8_t  enc[8193]; // 0..255 incl

    Gamma()
    {
        for (int i=0; i<256; i++)
        {
            double t = i / 255.0;
            t = t >= 0.04045 ? pow((t+0.055)/1.055, 2.4) : t/12.92;
            dec[i] = (uint16_t)round(t * 8192.0);
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

static void Bilinear(const uint16_t* src, int pitch, uint8_t x, uint8_t y, uint16_t* dst)
{
    // NEAREST TEST
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    return;

    // +---------+---------+
    // |   src   |  src+3  |
    // |  R,G,B  |  R,G,B  | < y=0
    // |         |         |
    // +---------+---------+ < y=128
    // |  src+p  | src+3+p |
    // |  R,G,B  |  R,G,B  | < y=256
    // |         |         |
    // +---------+---------+
    //      ^    ^    ^
    //     x=0 x=128 x=256

    // src must be (dst will be) normalized to (0..8192 incl)

    const uint16_t* lwr = src;
    const uint16_t* upr = src + pitch;
    const uint32_t qx = x;
    const uint32_t qy = y;
    const uint32_t px = 256-qx;
    const uint32_t py = 256-qy;
    const uint32_t r_ofs = 1<<15;

    const uint32_t pypx = py * px;
    const uint32_t pyqx = py * qx;
    const uint32_t qypx = qy * px;
    const uint32_t qyqx = qy * qx;

    dst[0] = (pypx * lwr[0] + pyqx * lwr[3] + qypx * upr[0] + qyqx * upr[3] + r_ofs) >> 16; 
    dst[1] = (pypx * lwr[1] + pyqx * lwr[4] + qypx * upr[1] + qyqx * upr[4] + r_ofs) >> 16; 
    dst[2] = (pypx * lwr[2] + pyqx * lwr[5] + qypx * upr[2] + qyqx * upr[5] + r_ofs) >> 16; 
}

static uint32_t Extract4(const uint16_t* c1, const uint16_t* c2, const uint16_t* c3, const uint16_t* c4)
{
    const int xxx_3 = 3;
    int i = 
        (gamma_tables.enc[(c1[0] + c2[0] + c3[0] + c4[0] + 2) >> 2] + xxx_offs) / xxx_3 +
        (gamma_tables.enc[(c1[1] + c2[1] + c3[1] + c4[1] + 2) >> 2] + xxx_offs) / xxx_3 * xxx_size +
        (gamma_tables.enc[(c1[2] + c2[2] + c3[2] + c4[2] + 2) >> 2] + xxx_offs) / xxx_3 * xxx_size2;

    return xxx_table[ i ];    
}

static uint32_t Extract2(const uint16_t* c1, const uint16_t* c2)
{
    const int xxx_3 = 3;
    int i = 
        (gamma_tables.enc[(c1[0] + c2[0]) >> 1] + xxx_offs) / xxx_3 +
        (gamma_tables.enc[(c1[1] + c2[1]) >> 1] + xxx_offs) / xxx_3 * xxx_size +
        (gamma_tables.enc[(c1[2] + c2[2]) >> 1] + xxx_offs) / xxx_3 * xxx_size2;

    return xxx_table[ i ];    
}

static void Accumulate(uint16_t a[3], const int16_t v[3])
{
    a[0] = std::max(0, std::min(8192, a[0]+v[0] ));
    a[1] = std::max(0, std::min(8192, a[1]+v[1] ));
    a[2] = std::max(0, std::min(8192, a[2]+v[2] ));
}

#define DITHERING
static void ScaleImg(const uint16_t* src, int src_w, int src_h, const float src_xywh[4], 
                     AnsiCell* dst, int dst_w, int dst_h, int dst_pitch=0)
{
    const int src_pitch = src_w * 3;

    #ifdef DITHERING
    // DITHERING STUFF
    int16_t e0[160][3] = {{0}};
    int16_t e1[160][3] = {{0}};
    int16_t e2[160][3] = {{0}};

    // [0]-current line, [1]-next line, [2]-nextnext line
    int16_t (*dither[3])[3] = {e0,e1,e2};
    #endif

    if (dst_pitch<=0)
        dst_pitch = dst_w;

    // offset start pos by +half dst px and -half src px
    const int sx = (int)round(256.0 * src_xywh[0] + 128.0 * src_xywh[2] / (2*dst_w) - 128);
    const int sy = (int)round(256.0 * src_xywh[1] + 128.0 * src_xywh[3] / (2*dst_h) - 128);
    const int dx = (int)round(256.0 * src_xywh[2] / (2*dst_w));
    const int dy = (int)round(256.0 * src_xywh[3] / (2*dst_h));

    // for enlarging near src edges, or arbitrary src_rect (partially outside src image)
    // we'd need also to handle sampling outside src img !!!
    // that's the reason to keep src_w, src_h for clamping

    int cy1 = sy;

    for (int y=0; y<dst_h; y++)
    {
        int cx1 = sx;
        int cy2 = cy1+dy;

        cy1 = sy + (int)round(256.0 * src_xywh[3] * (2 * y + 0) / (2*dst_h));
        cy2 = sy + (int)round(256.0 * src_xywh[3] * (2 * y + 1) / (2*dst_h));

        uint8_t ry1 = cy1 & 0xFF;
        const uint16_t* lwr = src + src_pitch * (cy1 >> 8);

        uint8_t ry2 = cy2 & 0xFF;
        const uint16_t* upr = src + src_pitch * (cy2 >> 8);

        AnsiCell* ptr = dst + y * dst_pitch;

        for (int x=0; x<dst_w; x++)
        {
            int cx2 = cx1+dx;

            cx1 = sx + (int)round(256.0 * src_xywh[2] * (2 * x + 0) / (2*dst_w));
            cx2 = sx + (int)round(256.0 * src_xywh[2] * (2 * x + 1) / (2*dst_w));

            uint8_t rx1 = cx1 & 0xFF;
            uint8_t rx2 = cx2 & 0xFF;

            if (!((cx1>>8)>=0 && (cx1>>8)<src_w &&
                  (cx2>>8)>=0 && (cx2>>8)<src_w &&
                  (cy1>>8)>=0 && (cy1>>8)<src_h &&
                  (cy2>>8)>=0 && (cy2>>8)<src_h))
            {
                printf("PROBLEM AT X=%d, Y=%d, (%d,%d)\n", x,y,2*x,2*y);
                printf("DST W=%d, H=%d, (%d,%d)\n", dst_w,dst_h,2*dst_w,2*dst_h);

                printf("cx1: %d.%d , cx2: %d.%d , cy1: %d.%d , cy2: %d.%d\n",
                    cx1>>8,cx1&0xff, cx2>>8,cx1&0xff, cy1>>8,cx1&0xff, cy2>>8,cx1&0xff);

                printf("src_xywh: %f , %f , %f , %f\n", 
                    src_xywh[0],src_xywh[1],src_xywh[2],src_xywh[3]);

                printf("sx: %d , sy: %d , dx: %d , dy: %d\n", 
                    sx,sy,dx,dy);

                assert(0);
            }

            uint16_t LL[3], LR[3], UL[3], UR[3];
            Bilinear(lwr + (cx1 >> 8)*3, src_pitch, rx1,ry1, LL); 
            Bilinear(lwr + (cx2 >> 8)*3, src_pitch, rx2,ry1, LR);
            Bilinear(upr + (cx1 >> 8)*3, src_pitch, rx1,ry2, UL);
            Bilinear(upr + (cx2 >> 8)*3, src_pitch, rx2,ry2, UR);

            // read & apply errors with clamping 
            #ifdef DITHERING
            Accumulate(LL, dither[0][x]);
            Accumulate(LR, dither[0][x]);
            Accumulate(UL, dither[0][x]);
            Accumulate(UR, dither[0][x]);

            // reset
            dither[0][x][0] = 0;
            dither[0][x][1] = 0;
            dither[0][x][2] = 0;
            #endif

            // we have 4 filtered samples, let's ANSIfy them into the single cell

            // calc 4 encoded reference colors (for calcing errors)
            int ll[3] ={gamma_tables.enc[LL[0]],gamma_tables.enc[LL[1]],gamma_tables.enc[LL[2]]};
            int lr[3] ={gamma_tables.enc[LR[0]],gamma_tables.enc[LR[1]],gamma_tables.enc[LR[2]]};
            int ul[3] ={gamma_tables.enc[UL[0]],gamma_tables.enc[UL[1]],gamma_tables.enc[UL[2]]};
            int ur[3] ={gamma_tables.enc[UR[0]],gamma_tables.enc[UR[1]],gamma_tables.enc[UR[2]]};

            // now reconstruct rgb values from the palette
            uint32_t l_slot = Extract2(LL,UL);
            uint32_t r_slot = Extract2(LR,UR);
            uint32_t b_slot = Extract2(LL,LR);
            uint32_t t_slot = Extract2(UL,UR);
            uint32_t d_slot = Extract4(LL,LR,UL,UR);

            const uint8_t* l = pal[(l_slot>>16) & 0xFF];
            const uint8_t* r = pal[(r_slot>>16) & 0xFF];
            const uint8_t* b = pal[(b_slot>>16) & 0xFF];
            const uint8_t* t = pal[(t_slot>>16) & 0xFF];
            const uint8_t* d = half_tone[d_slot>>24][d_slot&0xFF][(d_slot>>8)&0xFF];

            // calc errors
            int lr_err = 
                2*(std::abs(l[0] - ll[0]) + std::abs(l[0] - ul[0]) + std::abs(r[0] - lr[0]) + std::abs(r[0] - ur[0])) +
                3*(std::abs(l[1] - ll[1]) + std::abs(l[1] - ul[1]) + std::abs(r[1] - lr[1]) + std::abs(r[1] - ur[1])) +
                1*(std::abs(l[2] - ll[2]) + std::abs(l[2] - ul[2]) + std::abs(r[2] - lr[2]) + std::abs(r[2] - ur[2]));

            int bt_err = 
                2*(std::abs(b[0] - ll[0]) + std::abs(b[0] - lr[0]) + std::abs(t[0] - ul[0]) + std::abs(t[0] - ur[0])) +
                3*(std::abs(b[1] - ll[1]) + std::abs(b[1] - lr[1]) + std::abs(t[1] - ul[1]) + std::abs(t[1] - ur[1])) +
                1*(std::abs(b[2] - ll[2]) + std::abs(b[2] - lr[2]) + std::abs(t[2] - ul[2]) + std::abs(t[2] - ur[2]));

            int ht_err = 
                2*(std::abs(d[0] - ll[0]) + std::abs(d[0] - lr[0]) + std::abs(d[0] - ul[0]) + std::abs(d[0] - ur[0])) +
                3*(std::abs(d[1] - ll[1]) + std::abs(d[1] - lr[1]) + std::abs(d[1] - ul[1]) + std::abs(d[1] - ur[1])) +
                1*(std::abs(d[2] - ll[2]) + std::abs(d[2] - lr[2]) + std::abs(d[2] - ul[2]) + std::abs(d[2] - ur[2]));


            #ifdef DITHERING
            int32_t dev[3] =
            {
                LL[0]+LR[0]+UL[0]+UR[0],
                LL[1]+LR[1]+UL[1]+UR[1],
                LL[2]+LR[2]+UL[2]+UR[2]
            };
            #endif

            // pick best and calculate deviations
            if (ht_err < lr_err && ht_err < bt_err)
            {
                #ifdef DITHERING
                dev[0] -=  4 * gamma_tables.dec[d[0]];
                dev[1] -=  4 * gamma_tables.dec[d[1]];
                dev[2] -=  4 * gamma_tables.dec[d[2]];
                #endif

                dst->fg = ((d_slot>>8) & 0xFF) + 16;
                dst->bk = (d_slot & 0xFF) + 16;
                dst->gl = (d_slot>>24) + 176;
                dst->spare = 0;
            }
            else
            if (bt_err < lr_err)
            {
                #ifdef DITHERING
                dev[0] -= 2 * (gamma_tables.dec[b[0]] + gamma_tables.dec[t[0]]);
                dev[1] -= 2 * (gamma_tables.dec[b[1]] + gamma_tables.dec[t[1]]);
                dev[2] -= 2 * (gamma_tables.dec[b[2]] + gamma_tables.dec[t[2]]);
                #endif

                dst->fg = ((b_slot>>16) & 0xFF) + 16;
                dst->bk = ((t_slot>>16) & 0xFF) + 16;
                dst->gl = 220;
                dst->spare = 0;
            }
            else
            {
                #ifdef DITHERING
                dev[0] -= 2 * (gamma_tables.dec[l[0]] + gamma_tables.dec[r[0]]);
                dev[1] -= 2 * (gamma_tables.dec[l[1]] + gamma_tables.dec[r[1]]);
                dev[2] -= 2 * (gamma_tables.dec[l[2]] + gamma_tables.dec[r[2]]);
                #endif

                dst->fg = ((l_slot>>16) & 0xFF) + 16;
                dst->bk = ((r_slot>>16) & 0xFF) + 16;
                dst->gl = 221;
                dst->spare = 0;
            }

            // finaly distribute deviations
            #ifdef DITHERING
            dev[0] /= 32;
            dev[1] /= 32;
            dev[2] /= 32;

            if (x<dst_w-1)
            {
                dither[0][x+1][0] += dev[0];
                dither[0][x+1][1] += dev[1];
                dither[0][x+1][2] += dev[2];

                if (x<dst_w-2)
                {
                    dither[0][x+2][0] += dev[0];
                    dither[0][x+2][1] += dev[1];
                    dither[0][x+2][2] += dev[2];
                }
            }

            if (y<dst_h-1)
            {
                dither[1][x][0] += dev[0];
                dither[1][x][1] += dev[1];
                dither[1][x][2] += dev[2];

                if (x>0)
                {
                    dither[1][x-1][0] += dev[0];
                    dither[1][x-1][1] += dev[1];
                    dither[1][x-1][2] += dev[2];
                }

                if (x<dst_w-1)
                {
                    dither[1][x+1][0] += dev[0];
                    dither[1][x+1][1] += dev[1];
                    dither[1][x+1][2] += dev[2];
                }

                if (y<dst_h-2)
                {
                    dither[2][x][0] += dev[0];
                    dither[2][x][1] += dev[1];
                    dither[2][x][2] += dev[2];
                }
            }
            #endif

            cx1 = cx2 + dx;
            dst++;
        }

        cy1 = cy2 + dy;

        #ifdef DITHERING
        int16_t (*roll)[3] = dither[0];
        dither[0] = dither[1];
        dither[1] = dither[2];
        dither[2] = roll;
        #endif
    }
}

static void FreeImg(uint16_t* img)
{
    free(img);
}

static uint16_t* LoadImg(const char* path, int* w, int* h)
{
	upng_t* upng = upng_new_from_file(path);

	if (!upng)
		return 0;

	if (upng_get_error(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return 0;
	}

	if (upng_header(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return 0;
	}    

	int format, width, height, depth;
	format = upng_get_format(upng);
	width = upng_get_width(upng);
	height = upng_get_height(upng);

    if (format != UPNG_RGB8)
    {
		upng_free(upng);
		return 0;
    }

	if (upng_decode(upng) != UPNG_EOK)
	{
		upng_free(upng);
		return 0;
	}

	const uint8_t* buf = upng_get_buffer(upng);

    // allocate extra row and 1 px so Bilinear sampler won't overflow
    int wh3 = (width*(height+1) + 1)*3;
	uint16_t* pix = (uint16_t*)malloc(wh3*sizeof(uint16_t));

    // reflect vertically and decode gamma!
    for (int i=0,y=0; y<height; y++)
    {
        int j = (height - y - 1) * width * 3;
        for (int x=0; x<width; x++, i+=3, j+=3)
        {
            pix[j+0] = gamma_tables.dec[buf[i+0]];
            pix[j+1] = gamma_tables.dec[buf[i+1]];
            pix[j+2] = gamma_tables.dec[buf[i+2]];
        }
    }

    *w = width;
    *h = height;

	upng_free(upng);
    return pix;
}

extern "C" void *tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

int LoadMainMenuSprites(const char* base_path)
{
    // init palette entries
    /*
    FILE* ppp = fopen("666.gpl","wb");
    fprintf(ppp,"GIMP Palette\n");
    fprintf(ppp,"Name: 666\n");
    fprintf(ppp,"\n");
    fprintf(ppp,"#");
    */
	for (int i=0; i<pal_size; i++)
	{
		int j = i;
		pal[i][2] = j%6*51; j /= 6;
		pal[i][1] = j%6*51; j /= 6;
		pal[i][0] = j%6*51; j /= 6;

        //fprintf(ppp,"%3d %3d %3d    mycolor %d\n",pal[i][0],pal[i][1],pal[i][2],i);
	}
    //fclose(ppp);

    // init half_tone mapper
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
                    half_tone[g][c0][c1][c] = 
                        gamma_tables.enc[( 
                            c0_w * gamma_tables.dec[pal[c0][c]] + 
                            c1_w * gamma_tables.dec[pal[c1][c]] + 2) >> 2 ];
                }
            }
        }
    }

    // load inverse palettizer

	char path[1024];
	sprintf(path,"%spalettes/palette.gz", base_path);

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
	int r;
	r=(int)fread(&gz, 10, 1, f);

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
		r=(int)fread(&hi, 1, 1, f);
		r=(int)fread(&lo, 1, 1, f);

		int len = (hi << 8) | lo;
		fseek(f, len, SEEK_CUR);
	}

	if (gz.flg & (1 << 3/*FNAME*/))
	{
		uint8_t ch;
		do
		{
			ch = 0;
			r=(int)fread(&ch, 1, 1, f);
		} while (ch);
	}

	if (gz.flg & (1 << 4/*FCOMMENT*/))
	{
		uint8_t ch;
		do
		{
			ch = 0;
			r=(int)fread(&ch, 1, 1, f);
		} while (ch);
	}

	if (gz.flg & (1 << 1/*FFHCRC*/))
	{
		uint16_t crc;
		r=(int)fread(&crc, 2, 1, f);
	}

	// deflated data blocks ...
	// read everything till end of file, trim tail by 8 bytes (crc32,isize)

	long now = ftell(f);
	fseek(f, 0, SEEK_END);

	unsigned long insize = ftell(f) - now - 8;
	unsigned char* in = (unsigned char*)malloc(insize);
	fseek(f, now, SEEK_SET);

	r=(int)fread(in, 1, insize, f);


	size_t out_size=0;
	void* out = tinfl_decompress_mem_to_heap(in, insize, &out_size, 0);
	// void* out = u_inflate(in, insize);
	free(in);

	/////////////////////////////////
	// GZ OUTRO:

	uint32_t crc32, isize;
	r=(int)fread(&crc32, 4, 1, f);
	r=(int)fread(&isize, 4, 1, f);
	fclose(f);

	// assert(out && isize == *(uint32_t*)out);
	assert(out && isize == out_size);

    xxx_step = (uint8_t)*(uint32_t*)out;
    xxx_table = (uint32_t*)out + 1;
	xxx_offs = xxx_step >> 1;
	xxx_size = 255 / xxx_step + 1;
    xxx_size2 = xxx_size * xxx_size;

    assert(xxx_step == 3);

    // prepare GAMMA decoder and encoder
    // linear space will be in range (0..8192 incl)
    sprintf(path,"%simages/menu.png", base_path);
    menu_bk_img = LoadImg(path, &menu_bk_width, &menu_bk_height);

    if (!menu_bk_img)
        return 0;

    sprintf(path,"%ssprites/asciicker.xp", base_path);
    menu_logo_sprite = LoadSprite(path,"asciicker");

    // parse manifest, load sprites (oridinary sync)
    // and store cookies (ad cookies require copying original value into the actual cookie)

    printf("MAIN MENU OK\n");

    return 0;
}

void FreeMainMenuSprites()
{
    // undo LoadMainMenuSprites

    if (xxx_table)
        free(xxx_table-1);

    if (menu_bk_img)
        FreeImg(menu_bk_img);

    if (menu_logo_sprite)
        FreeSprite(menu_logo_sprite);
}

void LoadGame()
{
    // a little trouble here,
    // this func needs to be async on web platform:
    // (will fetch: a3d file, referenced meshes and .ajs script)
    // we must run fetches and lock ui until promise resolves
    // also adding cancel button would be a nice touch

    float water = 55;
    float dir = 0;
    float yaw = 45;
    float pos[3] = {0,15,0};
    float lt[4] = {1,0,1,.5};

    {
        // here the path should be taken from the module manifest
        // ...

        char a3d_path[1024 + 20];
        sprintf(a3d_path,"%sa3d/game_map_y8.a3d", base_path);
        FILE* f = fopen(a3d_path, "rb");

        // TODO:
        // if GameServer* gs != 0
        // DO NOT LOAD ITEMS!
        // we will receive them from server

        if (f)
        {
            terrain = LoadTerrain(f);

            if (terrain)
            {
                for (int i = 0; i < 256; i++)
                {
                    if (fread(mat[i].shade, 1, sizeof(MatCell) * 4 * 16, f) != sizeof(MatCell) * 4 * 16)
                        break;
                }

                world = LoadWorld(f, false);
                if (world)
                {
                    // reload meshes too
                    Mesh* m = GetFirstMesh(world);

                    while (m)
                    {
                        char mesh_name[256];
                        GetMeshName(m, mesh_name, 256);
                        char obj_path[4096];
                        sprintf(obj_path, "%smeshes/%s", base_path, mesh_name);
                        if (!UpdateMesh(m, obj_path))
                        {
                            // what now?
                            // missing mesh file!
                        }

                        m = GetNextMesh(m);
                    }

                    LoadEnemyGens(f);
                }
            }

            fclose(f);
        }

        // if (!terrain || !world)
        //    return -1;

        // add meshes from library that aren't present in scene file
        char mesh_dirname[4096];
        sprintf(mesh_dirname, "%smeshes", base_path);
        //a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

        // this is the only case when instances has no valid bboxes yet
        // as meshes weren't present during their creation
        // now meshes are loaded ...
        // so we need to update instance boxes with (,true)

        if (world)
        {
            RebuildWorld(world, true);
            #ifdef DARK_TERRAIN
            // TODO: add Patch Iterator holding stack of parents
            // and single Patch dark updater
            UpdateTerrainDark(terrain, world, lt, false);
            #endif
        }
    }

    InitGame(game, water, pos, yaw, dir, lt, mainmenu_stamp);

    // here we should also execute some .ajs
    // ...

    // if it sets onRead, and we have same saved progress, call it now.
    // ...
}

static void ResetGame()
{
    // let's test fresh restart
    FreeGame(game);

    if (terrain)
        DeleteTerrain(terrain);

    if (world)
        DeleteWorld(world);

    PurgeItemInstCache();

    // we would also RESET:
    // - akAPI_This to {}
    // - all CB handlers to null
}

void MainMenu_Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
    if (game_loading == 0)
        show_continue = false;
    if (game_loading == 3)
        show_continue = true;

    mainmenu_stamp = _stamp;

    /*
    int s = width*height;
    for (int i=0; i<s; i++)
        *((uint32_t*)ptr+i) = fast_rand() | (fast_rand()<<15);// | (fast_rand()<<30);
    */

    float dst_aspect = (float)width / height;
    float img_aspect = (float)menu_bk_width / menu_bk_height;

    float src_xywh[4];
    if (dst_aspect > img_aspect)
    {
        src_xywh[2] = menu_bk_width;
        src_xywh[0] = 0;
        src_xywh[3] = menu_bk_width / dst_aspect;
        src_xywh[1] = 0;//0.5f * (menu_bk_height - src_xywh[3]);
    }
    else
    {
        src_xywh[3] = menu_bk_height;
        src_xywh[1] = 0;
        src_xywh[2] = menu_bk_height * dst_aspect;
        src_xywh[0] = 0.5f * (menu_bk_width - src_xywh[2]);
    }


    ScaleImg(menu_bk_img, menu_bk_width, menu_bk_height, src_xywh, ptr, width, height);


    int x = 5, y = height - 5;
    // Font1Paint(ptr,width,height, x,y, "ASCIICKER", FONT1_GOLD_SKIN);
    int logo_x = (width - menu_logo_sprite->atlas->width + 1) / 2 + 1;
    // int logo_y = (height - menu_logo_sprite->atlas->height) / 2;
    BlitSprite(ptr,width,height,menu_logo_sprite->atlas, logo_x,y-menu_logo_sprite->atlas->height/2);

    y -= menu_logo_sprite->atlas->height;

    int w,h;
    Font1Size("MAIN MENU",&w,&h);

    Font1Paint(ptr,width,height, (width-w) / 2, y, "MAIN MENU", FONT1_PINK_SKIN);
    y-= 2*h;

    if (show_continue)
    {
        Font1Paint(ptr,width,height, x,y, "1.", FONT1_GOLD_SKIN);
        Font1Paint(ptr,width,height, x+7,y, "CONTINUE GAME", FONT1_GREY_SKIN);
        y -= h;

        Font1Paint(ptr,width,height, x,y, "2.", FONT1_GOLD_SKIN);
        Font1Paint(ptr,width,height, x+7,y, "START NEW GAME", FONT1_GREY_SKIN);
        y -= h;
    }
    else
    {
        Font1Paint(ptr,width,height, x,y, "1.", FONT1_GOLD_SKIN);
        Font1Paint(ptr,width,height, x+7,y, "START NEW GAME", FONT1_GREY_SKIN);
        y -= h;
    }

    if (game_loading == 1)
    {
        if (y > 5)
            y = 5;

        Font1Size("LOADING",&w,&h);
        Font1Paint(ptr,width,height, (width-w)/2,y, "LOADING", FONT1_PINK_SKIN);

        game_loading = 2;
    }
    else
    if (game_loading == 2)
    {
        LoadGame(); // here we should provide manifest index
        game_loading = 3;
        game->main_menu = false;
    }
}

void MainMenu_OnSize(int w, int h, int fw, int fh)
{
}

void MainMenu_OnKeyb(GAME_KEYB keyb, int key)
{
    if (keyb == GAME_KEYB::KEYB_CHAR)
    {
        if (game_loading==3)
        {
            if (key == '1')
                game->main_menu = false;
            if (key == '2')
            {
                ResetGame();
                game_loading = 1;
            }
        }
        else
        if (game_loading==0)
        {
            if (key == '1')
                game_loading = 1;
        }
    }
}

void MainMenu_OnMouse(GAME_MOUSE mouse, int x, int y)
{
    if (mouse == GAME_MOUSE::MOUSE_LEFT_BUT_DOWN ||
        mouse == GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN ||
        mouse == GAME_MOUSE::MOUSE_MIDDLE_BUT_DOWN)
    {
        if (game_loading==3)
            game->main_menu = false;
        else
        if (game_loading==0)
            game_loading = 1;
    }
}

void MainMenu_OnTouch(GAME_TOUCH touch, int id, int x, int y)
{
    if (touch == GAME_TOUCH::TOUCH_BEGIN)
    {
        if (game_loading==3)
            game->main_menu = false;
        else
        if (game_loading==0)
            game_loading = 1;
    }
}

void MainMenu_OnFocus(bool set)
{
}

void MainMenu_OnPadMount(bool connect)
{
}

void MainMenu_OnPadButton(int b, bool down)
{
    if (game_loading==3)
        game->main_menu = false;
    else
    if (game_loading==0)
        game_loading = 1;
}

void MainMenu_OnPadAxis(int a, int16_t pos)
{
}
