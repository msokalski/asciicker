#include <math.h>

#include "mainmenu.h"
#include "enemygen.h"
#include "fast_rand.h"
#include "font1.h"

extern Game* game;
extern Terrain* terrain;
extern World* world;
extern Material mat[256];
extern char base_path[1024];

static int  game_loading = 0; // 0-not_loaded, 1-load requested, 2-loading, 3-loaded
static bool show_continue = false;

static uint64_t mainmenu_stamp = 0;

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

static void Bilinear(const uint16_t src[3], int pitch, uint8_t x, uint8_t y, uint16_t dst[3])
{
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
    const uint32_t px = 128-qx;
    const uint32_t py = 128-qy;
    const uint32_t r_ofs = 1<<17;

    dst[0] = (py * (px * lwr[0] + qx * lwr[3]) + qy * (px * upr[0] + qx * upr[3]) + r_ofs) >> 18; 
    dst[1] = (py * (px * lwr[1] + qx * lwr[4]) + qy * (px * upr[1] + qx * upr[4]) + r_ofs) >> 18; 
    dst[2] = (py * (px * lwr[2] + qx * lwr[5]) + qy * (px * upr[2] + qx * upr[5]) + r_ofs) >> 18; 
}

extern "C" void *tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

int LoadMainMenuSprites(const char* base_path)
{
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

    // no use yet, was just testing
    free(out);

    // prepare GAMMA decoder and encoder
    // linear space will be in range (0..32768 incl)


    // load background png into linear rgb space (0..32768 incl)

    // DEC(1):   1/3302.25    * 32768 =  9,922931335 (delta = ~10)
    // DEC(2):   2/3302.25    * 32768 = 19,845862669
    // DEC(254): 0,991102097  * 32768 = 32476,43
    // DEC(255): 1.000000000  * 32768 = 32768,00     (delta = ~292)

    // t = 1/255
    // l = 1/(255*12.95) = 1/3302.25



    // parse manifest, load sprites (oridinary sync)
    // and store cookies (ad cookies require copying original value into the actual cookie)

    printf("MAIN MENU OK\n");

    return 0;
}

void FreeMainMenuSprites()
{
    // undo LoadMainMenuSprites
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
    int s = width*height;
    for (int i=0; i<s; i++)
        *((uint32_t*)ptr+i) = fast_rand() | (fast_rand()<<15);// | (fast_rand()<<30);

    int x = 5, y = height - 5;
    Font1Paint(ptr,width,height, x,y, "ASCIICKER", FONT1_GOLD_SKIN);

    int w,h;
    Font1Size("ASCIICKER",&w,&h);
    y -= h;

    Font1Paint(ptr,width,height, x,y, "MAIN MENU", FONT1_PINK_SKIN);
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
