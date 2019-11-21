
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#endif

#include <assert.h>

#include "asciiid_render.h"
#include "physics.h"
#include "sprite.h"
#include "matrix.h"

// FOR GL 
#include "asciiid_term.h"
#include "gl.h"
#include "rgba8.h"


#ifdef __linux__
/*
https://superuser.com/questions/1185824/configure-vga-colors-linux-ubuntu
https://int10h.org/oldschool-pc-fonts/fontlist/
https://www.zap.org.au/software/fonts/console-fonts-distributed/psftx-freebsd-11.1/index.html
ftp://ftp.zap.org.au/pub/fonts/console-fonts-zap/console-fonts-zap-2.2.tar.xz
*/

template <uint16_t C> static int UTF8(char* buf)
{
    if (C<0x0080)
    {
        buf[0]=C&0xFF;
        return 1;            
    }

    if (C<0x0800)
    {
        buf[0] = 0xC0 | ( ( C >> 6 ) & 0x1F ); 
        buf[1] = 0x80 | ( C & 0x3F );
        return 2;
    }

    buf[0] = 0xE0 | ( ( C >> 12 ) & 0x0F );
    buf[1] = 0x80 | ( ( C >> 6 ) & 0x3F );
    buf[2] = 0x80 | ( C & 0x3F );        
    return 3;
}

static int (* const CP437[256])(char*) =
{
    UTF8<0x0020>, UTF8<0x263A>, UTF8<0x263B>, UTF8<0x2665>, UTF8<0x2666>, UTF8<0x2663>, UTF8<0x2660>, UTF8<0x2022>, 
    UTF8<0x25D8>, UTF8<0x25CB>, UTF8<0x25D9>, UTF8<0x2642>, UTF8<0x2640>, UTF8<0x266A>, UTF8<0x266B>, UTF8<0x263C>,
    UTF8<0x25BA>, UTF8<0x25C4>, UTF8<0x2195>, UTF8<0x203C>, UTF8<0x00B6>, UTF8<0x00A7>, UTF8<0x25AC>, UTF8<0x21A8>, 
    UTF8<0x2191>, UTF8<0x2193>, UTF8<0x2192>, UTF8<0x2190>, UTF8<0x221F>, UTF8<0x2194>, UTF8<0x25B2>, UTF8<0x25BC>,
    UTF8<0x0020>, UTF8<0x0021>, UTF8<0x0022>, UTF8<0x0023>, UTF8<0x0024>, UTF8<0x0025>, UTF8<0x0026>, UTF8<0x0027>,
    UTF8<0x0028>, UTF8<0x0029>, UTF8<0x002A>, UTF8<0x002B>, UTF8<0x002C>, UTF8<0x002D>, UTF8<0x002E>, UTF8<0x002F>,
    UTF8<0x0030>, UTF8<0x0031>, UTF8<0x0032>, UTF8<0x0033>, UTF8<0x0034>, UTF8<0x0035>, UTF8<0x0036>, UTF8<0x0037>,
    UTF8<0x0038>, UTF8<0x0039>, UTF8<0x003A>, UTF8<0x003B>, UTF8<0x003C>, UTF8<0x003D>, UTF8<0x003E>, UTF8<0x003F>,
    UTF8<0x0040>, UTF8<0x0041>, UTF8<0x0042>, UTF8<0x0043>, UTF8<0x0044>, UTF8<0x0045>, UTF8<0x0046>, UTF8<0x0047>,
    UTF8<0x0048>, UTF8<0x0049>, UTF8<0x004A>, UTF8<0x004B>, UTF8<0x004C>, UTF8<0x004D>, UTF8<0x004E>, UTF8<0x004F>,
    UTF8<0x0050>, UTF8<0x0051>, UTF8<0x0052>, UTF8<0x0053>, UTF8<0x0054>, UTF8<0x0055>, UTF8<0x0056>, UTF8<0x0057>,
    UTF8<0x0058>, UTF8<0x0059>, UTF8<0x005A>, UTF8<0x005B>, UTF8<0x005C>, UTF8<0x005D>, UTF8<0x005E>, UTF8<0x005F>,
    UTF8<0x0060>, UTF8<0x0061>, UTF8<0x0062>, UTF8<0x0063>, UTF8<0x0064>, UTF8<0x0065>, UTF8<0x0066>, UTF8<0x0067>,
    UTF8<0x0068>, UTF8<0x0069>, UTF8<0x006A>, UTF8<0x006B>, UTF8<0x006C>, UTF8<0x006D>, UTF8<0x006E>, UTF8<0x006F>,
    UTF8<0x0070>, UTF8<0x0071>, UTF8<0x0072>, UTF8<0x0073>, UTF8<0x0074>, UTF8<0x0075>, UTF8<0x0076>, UTF8<0x0077>,
    UTF8<0x0078>, UTF8<0x0079>, UTF8<0x007A>, UTF8<0x007B>, UTF8<0x007C>, UTF8<0x007D>, UTF8<0x007E>, UTF8<0x2302>,
    UTF8<0x00C7>, UTF8<0x00FC>, UTF8<0x00E9>, UTF8<0x00E2>, UTF8<0x00E4>, UTF8<0x00E0>, UTF8<0x00E5>, UTF8<0x00E7>, 
    UTF8<0x00EA>, UTF8<0x00EB>, UTF8<0x00E8>, UTF8<0x00EF>, UTF8<0x00EE>, UTF8<0x00EC>, UTF8<0x00C4>, UTF8<0x00C5>, 
    UTF8<0x00C9>, UTF8<0x00E6>, UTF8<0x00C6>, UTF8<0x00F4>, UTF8<0x00F6>, UTF8<0x00F2>, UTF8<0x00FB>, UTF8<0x00F9>, 
    UTF8<0x00FF>, UTF8<0x00D6>, UTF8<0x00DC>, UTF8<0x00A2>, UTF8<0x00A3>, UTF8<0x00A5>, UTF8<0x20A7>, UTF8<0x0192>, 
    UTF8<0x00E1>, UTF8<0x00ED>, UTF8<0x00F3>, UTF8<0x00FA>, UTF8<0x00F1>, UTF8<0x00D1>, UTF8<0x00AA>, UTF8<0x00BA>, 
    UTF8<0x00BF>, UTF8<0x2310>, UTF8<0x00AC>, UTF8<0x00BD>, UTF8<0x00BC>, UTF8<0x00A1>, UTF8<0x00AB>, UTF8<0x00BB>, 
    UTF8<0x2591>, UTF8<0x2592>, UTF8<0x2593>, UTF8<0x2502>, UTF8<0x2524>, UTF8<0x2561>, UTF8<0x2562>, UTF8<0x2556>, 
    UTF8<0x2555>, UTF8<0x2563>, UTF8<0x2551>, UTF8<0x2557>, UTF8<0x255D>, UTF8<0x255C>, UTF8<0x255B>, UTF8<0x2510>, 
    UTF8<0x2514>, UTF8<0x2534>, UTF8<0x252C>, UTF8<0x251C>, UTF8<0x2500>, UTF8<0x253C>, UTF8<0x255E>, UTF8<0x255F>, 
    UTF8<0x255A>, UTF8<0x2554>, UTF8<0x2569>, UTF8<0x2566>, UTF8<0x2560>, UTF8<0x2550>, UTF8<0x256C>, UTF8<0x2567>, 
    UTF8<0x2568>, UTF8<0x2564>, UTF8<0x2565>, UTF8<0x2559>, UTF8<0x2558>, UTF8<0x2552>, UTF8<0x2553>, UTF8<0x256B>, 
    UTF8<0x256A>, UTF8<0x2518>, UTF8<0x250C>, UTF8<0x2588>, UTF8<0x2584>, UTF8<0x258C>, UTF8<0x2590>, UTF8<0x2580>, 
    UTF8<0x03B1>, UTF8<0x00DF>, UTF8<0x0393>, UTF8<0x03C0>, UTF8<0x03A3>, UTF8<0x03C3>, UTF8<0x00B5>, UTF8<0x03C4>, 
    UTF8<0x03A6>, UTF8<0x0398>, UTF8<0x03A9>, UTF8<0x03B4>, UTF8<0x221E>, UTF8<0x03C6>, UTF8<0x03B5>, UTF8<0x2229>, 
    UTF8<0x2261>, UTF8<0x00B1>, UTF8<0x2265>, UTF8<0x2264>, UTF8<0x2320>, UTF8<0x2321>, UTF8<0x00F7>, UTF8<0x2248>, 
    UTF8<0x00B0>, UTF8<0x2219>, UTF8<0x00B7>, UTF8<0x221A>, UTF8<0x207F>, UTF8<0x00B2>, UTF8<0x25A0>, UTF8<0x0020>
};

bool xterm_kitty = false;
void SetScreen(bool alt)
{

    // kitty kitty ...
    const char* term = getenv("TERM");
    if (term && strcmp(term,"xterm-kitty")==0)
    {
        xterm_kitty = alt;
        write(STDOUT_FILENO, alt?"\x1B[?2017h":"\x1B[?2017l", 8);
    }
    
    static const char* on_str = "\x1B[?1049h" "\x1B[H" "\x1B[?7l" "\x1B[?25l" "\x1B[?1002h"; // +home, -wrap, -cursor, +mouse
    static const char* off_str = "\x1B[39m;\x1B[49m" "\x1B[?1049l" "\x1B[?7h" "\x1B[?25h" "\x1B[?1002l"; // +def_fg/bg, +wrap, +cursor, -mouse
    static int on_len = strlen(on_str);
    static int off_len = strlen(off_str);

    static struct termios old;

	if (alt)
    {
        tcgetattr(STDIN_FILENO, &old);
        struct termios t = old;
		t.c_lflag |=  ISIG;
		t.c_iflag &= ~IGNBRK;
		t.c_iflag |=  BRKINT;
		t.c_lflag &= ~ICANON; /* disable buffered i/o */
		t.c_lflag &= ~ECHO; /* disable echo mode */

        tcsetattr(STDIN_FILENO, TCSANOW, &t);
		write(STDOUT_FILENO,on_str,on_len);
    }
	else
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
		write(STDOUT_FILENO,off_str,off_len);
    }
}

#define FLUSH() \
    do \
    { \
        write(STDOUT_FILENO,out,out_pos); \
        out_pos=0; \
    } while(0)

#define WRITE(...) \
    do \
    { \
        out_pos += sprintf(out+out_pos,__VA_ARGS__); \
        if (out_pos>=out_size-48) FLUSH(); \
    } while(0)


// it turns out we should use our own palette
// it's quite different than xterm!!!!!
const uint8_t pal_rgba[256][3]=
{
    {  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},
    {  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},

    {  0,  0,  0},{  0,  0, 51},{  0,  0,102},{  0,  0,153},{  0,  0,204},{  0,  0,255},
    {  0, 51,  0},{  0, 51, 51},{  0, 51,102},{  0, 51,153},{  0, 51,204},{  0, 51,255},
    {  0,102,  0},{  0,102, 51},{  0,102,102},{  0,102,153},{  0,102,204},{  0,102,255},
    {  0,153,  0},{  0,153, 51},{  0,153,102},{  0,153,153},{  0,153,204},{  0,153,255},
    {  0,204,  0},{  0,204, 51},{  0,204,102},{  0,204,153},{  0,204,204},{  0,204,255},
    {  0,255,  0},{  0,255, 51},{  0,255,102},{  0,255,153},{  0,255,204},{  0,255,255},

    { 51,  0,  0},{ 51,  0, 51},{ 51,  0,102},{ 51,  0,153},{ 51,  0,204},{ 51,  0,255},
    { 51, 51,  0},{ 51, 51, 51},{ 51, 51,102},{ 51, 51,153},{ 51, 51,204},{ 51, 51,255},
    { 51,102,  0},{ 51,102, 51},{ 51,102,102},{ 51,102,153},{ 51,102,204},{ 51,102,255},
    { 51,153,  0},{ 51,153, 51},{ 51,153,102},{ 51,153,153},{ 51,153,204},{ 51,153,255},
    { 51,204,  0},{ 51,204, 51},{ 51,204,102},{ 51,204,153},{ 51,204,204},{ 51,204,255},
    { 51,255,  0},{ 51,255, 51},{ 51,255,102},{ 51,255,153},{ 51,255,204},{ 51,255,255},
    
    {102,  0,  0},{102,  0, 51},{102,  0,102},{102,  0,153},{102,  0,204},{102,  0,255},
    {102, 51,  0},{102, 51, 51},{102, 51,102},{102, 51,153},{102, 51,204},{102, 51,255},
    {102,102,  0},{102,102, 51},{102,102,102},{102,102,153},{102,102,204},{102,102,255},
    {102,153,  0},{102,153, 51},{102,153,102},{102,153,153},{102,153,204},{102,153,255},
    {102,204,  0},{102,204, 51},{102,204,102},{102,204,153},{102,204,204},{102,204,255},
    {102,255,  0},{102,255, 51},{102,255,102},{102,255,153},{102,255,204},{102,255,255},
    
    {153,  0,  0},{153,  0, 51},{153,  0,102},{153,  0,153},{153,  0,204},{153,  0,255},
    {153, 51,  0},{153, 51, 51},{153, 51,102},{153, 51,153},{153, 51,204},{153, 51,255},
    {153,102,  0},{153,102, 51},{153,102,102},{153,102,153},{153,102,204},{153,102,255},
    {153,153,  0},{153,153, 51},{153,153,102},{153,153,153},{153,153,204},{153,153,255},
    {153,204,  0},{153,204, 51},{153,204,102},{153,204,153},{153,204,204},{153,204,255},
    {153,255,  0},{153,255, 51},{153,255,102},{153,255,153},{153,255,204},{153,255,255},
    
    {204,  0,  0},{204,  0, 51},{204,  0,102},{204,  0,153},{204,  0,204},{204,  0,255},
    {204, 51,  0},{204, 51, 51},{204, 51,102},{204, 51,153},{204, 51,204},{204, 51,255},
    {204,102,  0},{204,102, 51},{204,102,102},{204,102,153},{204,102,204},{204,102,255},
    {204,153,  0},{204,153, 51},{204,153,102},{204,153,153},{204,153,204},{204,153,255},
    {204,204,  0},{204,204, 51},{204,204,102},{204,204,153},{204,204,204},{204,204,255},
    {204,255,  0},{204,255, 51},{204,255,102},{204,255,153},{204,255,204},{204,255,255},

    {255,  0,  0},{255,  0, 51},{255,  0,102},{255,  0,153},{255,  0,204},{255,  0,255},
    {255, 51,  0},{255, 51, 51},{255, 51,102},{255, 51,153},{255, 51,204},{255, 51,255},
    {255,102,  0},{255,102, 51},{255,102,102},{255,102,153},{255,102,204},{255,102,255},
    {255,153,  0},{255,153, 51},{255,153,102},{255,153,153},{255,153,204},{255,153,255},
    {255,204,  0},{255,204, 51},{255,204,102},{255,204,153},{255,204,204},{255,204,255},
    {255,255,  0},{255,255, 51},{255,255,102},{255,255,153},{255,255,204},{255,255,255},        
};

void Print(AnsiCell* buf, int w, int h, const char utf[256][4])
{
    // heading
    // w x (fg,bg,3bytes)

    int bk=-1,fg=-1;

    // home

    // 2.3MB out buffer
    const int out_size = 3/*header*/ + 40/*fg,bg,ch*/ * 320/*width*/ * 180/*height*/ + 180/*'\n'*/; // 4096;
    char out[out_size];
    int out_pos = 0;

    WRITE("\x1B[H");

    for (int y = h-1; y>=0; y--)
    {
        AnsiCell* ptr = buf + y*w;
        for (int x=0; x<w; x++,ptr++)
        {
            //const char* chr = (x+y)&1 ? "X":"Y";
            const char* chr = utf[ptr->gl];
            if (ptr->fg != fg)
            {
                if (ptr->bk != bk)
                    //WRITE("\x1B[38;5;%d;48;5;%dm%s",ptr->fg,ptr->bk,chr);
                    WRITE("\x1B[38;2;%d;%d;%d;48;2;%d;%d;%dm%s",pal_rgba[ptr->fg][0],pal_rgba[ptr->fg][1],pal_rgba[ptr->fg][2], pal_rgba[ptr->bk][0],pal_rgba[ptr->bk][1],pal_rgba[ptr->bk][2], chr);
                else
                    //WRITE("\x1B[38;5;%dm%s",ptr->fg,chr);
                    WRITE("\x1B[38;2;%d;%d;%dm%s",pal_rgba[ptr->fg][0],pal_rgba[ptr->fg][1],pal_rgba[ptr->fg][2], chr);
            }
            else
            {
                if (ptr->bk != bk)
                    //WRITE("\x1B[48;5;%dm%s",ptr->bk,chr);
                    WRITE("\x1B[48;2;%d;%d;%dm%s",pal_rgba[ptr->bk][0],pal_rgba[ptr->bk][1],pal_rgba[ptr->bk][2], chr);
                else
                    WRITE("%s",chr);
            }
            bk=ptr->bk;
            fg=ptr->fg;
        }

        if (y)
            WRITE("\n");
    }

    FLUSH();
}


bool running = false;
void exit_handler(int signum)
{
    running = false;
}

bool GetWH(int wh[2])
{
	struct winsize size;
	if (ioctl(0, TIOCGWINSZ, (char *)&size))
	{
		wh[0] = 80;
		wh[1] = 50;
		return false;
	}

	wh[0] = size.ws_col;
	wh[1] = size.ws_row;
	return true;
}

uint64_t GetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

#else

#define GetTime() a3dGetTime()

#endif

Material mat[256];
void* GetMaterialArr()
{
    return mat;
}

void* GetFontArr();
int fonts_loaded=0;
struct MyFont
{
	static bool Scan(A3D_DirItem item, const char* name, void* cookie)
	{
		if (!(item&A3D_FILE))
			return true;

		char buf[4096];
		snprintf(buf,4095,"%s/%s",(char*)cookie,name);
		buf[4095]=0;

		a3dLoadImage(buf, 0, MyFont::Load);
		return true;
	}

	static int Sort(const void* a, const void* b)
	{
		MyFont* fa = (MyFont*)a;
		MyFont* fb = (MyFont*)b;

		int qa = fa->width*fa->height;
		int qb = fb->width*fb->height;

		return qa - qb;
	}

	static void Free()
	{
		MyFont* fnt = (MyFont*)GetFontArr();
		for (int i=0; i<fonts_loaded; i++)
		{
			glDeleteTextures(1,&fnt[i].tex);
		}
	}

	static void Load(void* cookie, A3D_ImageFormat f, int w, int h, const void* data, int palsize, const void* palbuf)
	{
		if (fonts_loaded==256)
			return;
			
		MyFont* fnt = (MyFont*)GetFontArr() + fonts_loaded;

		fnt->width = w;
		fnt->height = h;

		int ifmt = GL_RGBA8;
		int fmt = GL_RGBA;
		int type = GL_UNSIGNED_BYTE;

		uint32_t* buf = (uint32_t*)malloc(w * h * sizeof(uint32_t));

		uint8_t rgb[3] = { 0xff,0xff,0xff };
		ConvertLuminance_UI32_LLZZYYXX(buf, rgb, f, w, h, data, palsize, palbuf);

		glCreateTextures(GL_TEXTURE_2D, 1, &fnt->tex);
		glTextureStorage2D(fnt->tex, 1, ifmt, w, h);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(fnt->tex, 0, 0, 0, w, h, fmt, type, buf ? buf : data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		float white_transp[4] = { 1,1,1,0 };

		glTextureParameteri(fnt->tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(fnt->tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(fnt->tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(fnt->tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTextureParameterfv(fnt->tex, GL_TEXTURE_BORDER_COLOR, white_transp);


		/*
		// if we want to filter font we'd have first to
		// modify 3 things in font sampling by shader:
		// - clamp uv to glyph boundary during sampling
		// - fade result by distance normalized to 0.5 of texel 
		//   between unclamped uv to clamping glyph boundary
		// - use manual lod as log2(font_zoom)

		int max_lod = 0;
		while (!((w & 1) | (h & 1)))
		{
			max_lod++;
			w >>= 1;
			h >>= 1;
		}
		glGenerateTextureMipmap(fnt->tex);
		glTextureParameteri(fnt->tex, GL_TEXTURE_MAX_LOD, max_lod);
		*/

		if (buf)
			free(buf);

		fonts_loaded++;

		qsort(GetFontArr(), fonts_loaded, sizeof(MyFont), MyFont::Sort);
	}

	void SetTexel(int x, int y, uint8_t val)
	{
		uint8_t texel[4] = { 0xFF,0xFF,0xFF,val };
		glTextureSubImage2D(tex, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texel);
	}

	uint8_t GetTexel(int x, int y)
	{
		uint8_t texel[4];
		glGetTextureSubImage(tex, 0, x, y, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4, texel);
		return texel[3];
	}

	int width;
	int height;

	GLuint tex;
} font[256];

void* GetFontArr()
{
	return font;
}


Sprite* player_sprite = 0;

// make term happy
float pos_x,pos_y,pos_z;
float rot_yaw;
int probe_z;
float global_lt[4];
World* world;
Terrain* terrain;

int GetGLFont(int wh[2], const int wnd_wh[2])
{
    int tex = 0;
    float err = 0;

    assert(wnd_wh);
    float area = wnd_wh[0]*wnd_wh[1];

    for (int i=0; i<fonts_loaded; i++)
    {
        MyFont* f = font + i;
        float e = fabsf( 120.0f*75.0f - area / ((f->width>>4)*(f->height>>4)));

        if (!i || e<err)
        {
            err = e;
            tex = f->tex;

            if (wh)
            {
                wh[0] = f->width;
                wh[1] = f->height;
            }
        }
    }

	return tex;
}

int main(int argc, char* argv[])
{
    // if there is no '-term' arg given run A3D (GL) term
    // ...

    // otherwise continue with following crap
    // ...

    // consider configuring kbd, it solves only key up, not multiple keys down
    /*
        gsettings set org.gnome.desktop.peripherals.keyboard repeat-interval 30
        gsettings set org.gnome.desktop.peripherals.keyboard delay 250    
    */

    bool term = false;
    for (int p=1; p<argc; p++)
    {
        if (strcmp(argv[p],"-term")==0)
            term = true;
    }


    float water = 55;

    float yaw = 45;
    float pos[3] = {0,15,0};
    float lt[4] = {1,0,1,.5};

    player_sprite = LoadPlayer("./sprites/player.xp");
    if (!player_sprite)
        return -1;

    if (!term)
    {
        probe_z = (int)water;

        pos_x = pos[0];
        pos_y = pos[1];
        pos_z = pos[2];
        rot_yaw = yaw;

        global_lt[0] = lt[0];
        global_lt[1] = lt[1];
        global_lt[2] = lt[2];
        global_lt[3] = lt[3];

        {
            FILE* f = fopen("a3d/game.a3d","rb");

            if (f)
            {
                terrain = LoadTerrain(f);

                if (terrain)
                {
                    for (int i=0; i<256; i++)
                    {
                        if ( fread(mat[i].shade,1,sizeof(MatCell)*4*16,f) != sizeof(MatCell)*4*16 )
                            break;
                    }

                    world = LoadWorld(f);
                    if (world)
                    {
                        // reload meshes too
                        Mesh* m = GetFirstMesh(world);

                        while (m)
                        {
                            char mesh_name[256];
                            GetMeshName(m,mesh_name,256);
                            char obj_path[4096];
                            sprintf(obj_path,"%smeshes/%s","./"/*root_path*/,mesh_name);
                            if (!UpdateMesh(m,obj_path))
                            {
                                // what now?
                                // missing mesh file!
                            }

                            m = GetNextMesh(m);
                        }
                    }
                }

                fclose(f);
            }

            if (!terrain || !world)
               return -1;

            // add meshes from library that aren't present in scene file
            char mesh_dirname[4096];
            sprintf(mesh_dirname,"%smeshes","./"/*root_path*/);
            //a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

            // this is the only case when instances has no valid bboxes yet
            // as meshes weren't present during their creation
            // now meshes are loaded ...
            // so we need to update instance boxes with (,true)
            RebuildWorld(world, true);
        }

        TermOpen(0, yaw, pos);

        char font_dirname[] = "./fonts";
        fonts_loaded = 0;
        a3dListDir(font_dirname, MyFont::Scan, font_dirname);

        a3dLoop();
        return 0;
    }


#ifdef __linux__

    int signals[]={SIGTERM,SIGHUP,SIGINT,SIGTRAP,SIGILL,SIGABRT,SIGKILL,0};
    struct sigaction new_action, old_action;
    new_action.sa_handler = exit_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    for (int i=0; signals[i]; i++)
    {
        sigaction(signals[i], NULL, &old_action);
        if (old_action.sa_handler != SIG_IGN)
            sigaction(signals[i], &new_action, NULL);
    }

    running = true;

    SetScreen(true);

    /*
    if (!xterm_kitty)
    {
        SetScreen(false);
        printf("TERMINATING: this pre-update build requires kitty terminal to run\n");
        return -1;
    }
    */

    float player_dir = 0;
    int  player_stp = -1;    

    Physics* phys = 0;
    //Terrain* terrain = 0;
    //World* world = 0;
    terrain = 0;
    world = 0;

    AnsiCell* buf = 0;
    int wh[2] = {-1,-1};    

    uint64_t begin = GetTime();
    uint64_t stamp = begin;
    uint64_t frames = 0;

    const int sticky_input = 30000;

    int mouse_b=0; // num of buttons down (we can't be sure which ones!)
    int mouse_x;
    int mouse_y;    

    if (!player_sprite)
        goto exit;

    {
        FILE* f = fopen("a3d/game.a3d","rb");

        if (f)
        {
            terrain = LoadTerrain(f);

            if (terrain)
            {
                for (int i=0; i<256; i++)
                {
                    if ( fread(mat[i].shade,1,sizeof(MatCell)*4*16,f) != sizeof(MatCell)*4*16 )
                        break;
                }

                world = LoadWorld(f);
                if (world)
                {
                    // reload meshes too
                    Mesh* m = GetFirstMesh(world);

                    while (m)
                    {
                        char mesh_name[256];
                        GetMeshName(m,mesh_name,256);
                        char obj_path[4096];
                        sprintf(obj_path,"%smeshes/%s","./"/*root_path*/,mesh_name);
                        if (!UpdateMesh(m,obj_path))
                        {
                            // what now?
                            // missing mesh file!
                        }

                        m = GetNextMesh(m);
                    }
                }
            }

            fclose(f);
        }

        if (!terrain || !world)
            goto exit;

        // add meshes from library that aren't present in scene file
        char mesh_dirname[4096];
        sprintf(mesh_dirname,"%smeshes","./"/*root_path*/);
        //a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

        // this is the only case when instances has no valid bboxes yet
        // as meshes weren't present during their creation
        // now meshes are loaded ...
        // so we need to update instance boxes with (,true)
        RebuildWorld(world, true);
    }

    // init CP437 to UTF8 converter
    char CP437_UTF8 [256][4];
    for (int i=0; i<256; i++)
        CP437_UTF8[i][CP437[i](CP437_UTF8[i])]=0;

    int kbd[256];
    memset(kbd,0,sizeof(int)*256);

    // LIGHT
    {
        float lit_yaw = 45;
        float lit_pitch = 30;//90;
        float lit_time = 12.0f;
        float ambience = 0.5;

        double noon_yaw[2] =
        {
            // zero is behind viewer
            -sin(-lit_yaw*M_PI / 180),
            -cos(-lit_yaw*M_PI / 180),
        };

        double dusk_yaw[3] =
        {
            -noon_yaw[1],
            noon_yaw[0],
            0
        };

        double noon_pos[3] =
        {
            noon_yaw[0]*cos(lit_pitch*M_PI / 180),
            noon_yaw[1]*cos(lit_pitch*M_PI / 180),
            sin(lit_pitch*M_PI / 180)
        };

        double lit_axis[3];

        CrossProduct(dusk_yaw, noon_pos, lit_axis);

        double time_tm[16];
        Rotation(lit_axis, (lit_time-12)*M_PI / 12, time_tm);

        double lit_pos[4];
        Product(time_tm, noon_pos, lit_pos);

        lt[0] = (float)lit_pos[0];
        lt[1] = (float)lit_pos[1];
        lt[2] = (float)lit_pos[2];
        lt[3] = ambience;    
    }

    // get initial time stamp
    begin = GetTime();
    stamp = begin;

    phys = CreatePhysics(terrain,world,pos,player_dir,yaw,stamp);

    while(running)
    {
        bool mouse_j = false;

        // get time stamp
        uint64_t now = GetTime();
        int dt = now-stamp;
        stamp = now;      

        if (!xterm_kitty)
        {
            for (int i=0; i<256; i++)
            {
                kbd[i] -= dt;
                if (kbd[i]<0)
                    kbd[i]=0;
            }
        }

        // prep for poll
        struct pollfd pfds[1];
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN; 
        pfds[0].revents = 0;

        PhysicsIO io = {0};
        io.water = 55;

        poll(pfds, 1, 0); // 0 no timeout, -1 block

        if (pfds[0].revents & POLLIN) 
        {
            static int stream_bytes = 0;
            static char stream[256];

            char fresh[256];

            int fresh_bytes = read(STDIN_FILENO, fresh, 256 - stream_bytes);
            memcpy(stream + stream_bytes, fresh, fresh_bytes);

            int bytes = stream_bytes + fresh_bytes;

            int i = 0;
            while (i<bytes)
            {
                int j=i;

                int type = 0, mods = 0;

                /*
                if (!xterm_kitty)
                {
                    FILE* log = fopen("log.bin","a+");
                    fprintf(log,"0x%02X(%c)\n",stream[i],stream[i]);
                    fclose(log);
                    i++;
                    continue;
                }
                */


                // unescaped input can only jump
                if (stream[i]>=' ' && stream[i]<=127)
                {
                    switch (stream[i])
                    {
                        case ' ': mouse_j = true; kbd['A']=sticky_input; break;
                    }
                    
                    i++;
                    continue;
                }

                // MUST HAVE
                // TODO: non-kitty just to rotate, should emu sticky down
                // F1,F2,DEL,INS,PGUP,PGDN
                // ...

                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == 'O' && stream[i+2] == 'P')
                {
                    // F1
                    kbd['/']=sticky_input;
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == 'O' && stream[i+2] == 'Q')
                {
                    // F2
                    kbd['*']=sticky_input;
                    i+=3;
                }

                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '3' && stream[i+3] == '~')
                {
                    // DEL
                    kbd['3']=sticky_input;
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '2' && stream[i+3] == '~')
                {
                    // INS
                    kbd['2']=sticky_input;
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '5' && stream[i+3] == '~')
                {
                    // PGUP
                    kbd['8']=sticky_input;
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '6' && stream[i+3] == '~')
                {
                    // PGDN
                    kbd['9']=sticky_input;
                    i+=4;
                }

                

                // SHOULD HAVE
                // TODO: non-kitty arrows (in case mouse in not available - YUCK!)
                // scan for mods: +shift (slower) +alt (jump)
                // ...

                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'D')
                {
                    // left
                    kbd['5']=sticky_input;
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'C')
                {
                    // right
                    kbd['4']=sticky_input;
                    i+=3;
                }                
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'A')
                {
                    // up
                    kbd['7']=sticky_input;
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'B')
                {
                    // down
                    kbd['6']=sticky_input;
                    i+=3;
                }    

                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '1' && stream[i+3] == ';')
                {
                    if (bytes-i < 6)
                        break;

                    int mods = stream[i+4] - 0x31;

                    if (mods&1)
                    {
                        // shift
                        kbd[128+'a'] = sticky_input;
                    }
                    if (mods&2)
                    {
                        // alt
                        kbd[128+'c'] = sticky_input;
                    }

                    switch (stream[i+5])
                    {
                        case 'P': // f1
                            kbd['/']=sticky_input;
                            break;
                        case 'Q': // f2
                            kbd['*']=sticky_input;
                            break;
                        case '3': // del
                            kbd['3']=sticky_input;
                            break;
                        case '2': // ins
                            kbd['2']=sticky_input;
                            break;
                        case '5': // pgup
                            kbd['8']=sticky_input;
                            break;
                        case '6': // pgdn
                            kbd['9']=sticky_input;
                            break;

                        case 'A': // up
                            kbd['7']=sticky_input;
                            break;
                        case 'B': // down
                            kbd['6']=sticky_input;
                            break;
                        case 'C': // right
                            kbd['4']=sticky_input;
                            break;
                        case 'D': // left
                            kbd['5']=sticky_input;
                            break;
                    }

                    i+=6;
                }            

                // mouse
                if (bytes-i >= 3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'M')
                {
                    // code = (0=mb1_down,1=mb2_down,2=mb3_down,3=release) + higher bits for mods
                    // x,y

                    if (bytes-i < 6)
                        break;

                    if ((stream[i+3]&0x60) == 0x20) // ensure base without motion
                    {
                        if ((stream[i+3]&3) == 3)
                        {
                            if (mouse_b)
                                mouse_b--;
                        }
                        else
                        {
                            mouse_b++;
                            if (mouse_b>=2) // emu jump
                                mouse_j = true;
                        }
                    }

                    mouse_x = (uint8_t)stream[i+4] - 33;
                    mouse_y = (uint8_t)stream[i+5] - 33;

                    i+=6;
                }

                // 3. kitty keys
                if (bytes-i >= 3 && stream[i] == 0x1B && stream[i+1] == '_' && stream[i+2] == 'K')
                {
                    if (bytes-i < 8)
                        break;

                    if (bytes-i == 8 && stream[i+6] != 0x1B)
                        break;

                    switch (stream[i+3])
                    {
                        case 'p': type = +1; //press
                            break;
                        case 't': type =  0; //repeat
                            break;
                        case 'r': type = -1; //release
                            break;
                    }

                    if (stream[i+4]>='A' && stream[i+4]<='P')
                    {
                        mods = stream[i+4] -'A';
                    }

                    int codelen = 0;
                    if (stream[i+6]==0x1B && stream[i+7]=='\\')
                        codelen=1;
                    else
                    if (stream[i+7]==0x1B && stream[i+8]=='\\')
                        codelen=2;

                    if ((mods & 4) && stream[i+5]=='U' && codelen==1) // CTRL+C
                    {
                        memset(kbd,0,sizeof(int)*256);
                        running = false;
                        break;
                    }

                    // store shift 
                    if ( (mods&1) && type>=0 )
                        kbd[128+'a'] = 1;
                    else
                        kbd[128+'a'] = 0;

                    if (codelen==2 && stream[i+5]=='B')
                    {
                        switch (stream[i+6])
                        {
                            case 'A': // F17
                            case 'B': // F18
                            case 'C': // F19
                            case 'D': // F20
                            case 'E': // F21
                            case 'F': // F22
                            case 'G': // F23
                            case 'H': // F24
                            case 'I': // F25
                            case 'J': // KP0
                            case 'K': // KP1
                            case 'L': // KP2
                            case 'M': // KP3
                            case 'N': // KP4
                            case 'O': // KP5
                            case 'P': // KP6
                            case 'Q': // KP7
                            case 'R': // KP8
                            case 'S': // KP9
                            case 'T': // KP.
                            case 'U': // KP/
                            case 'V': // KP*
                            case 'W': // KP-
                            case 'X': // KP+
                            case 'Y': // KP enter
                            case 'Z': // KP=
                            case 'a': // left shift
                            case 'b': // left ctrl
                            case 'c': // left alt
                            case 'd': // left super (win?)
                            case 'e': // right shift
                            case 'f': // right ctrl
                            case 'g': // right alt
                            case 'h': // right super (win?)
                            {
                                if (type>0)
                                {
                                    kbd[ 128 + stream[i+6] ] = 1;
                                }

                                if (type<0)
                                {
                                    kbd[ 128 + stream[i+6] ] = 0;
                                }

                                i+=9;
                                break;
                            }
                        }
                    }
                    else
                    if (codelen==1)
                    {
                        switch (stream[i+5])
                        {
                            case '0': // TAB
                            case '1': // BACKSPACE
                            case '2': // INSERT
                            case '3': // DELETE
                            case '4': // RIGHT
                            case '5': // LEFT
                            case '6': // DOWN
                            case '7': // UP
                            case '8': // PAGEUP
                            case '9': // PAGEDN

                            case 'A': // SPACE
                            case 'B': // APOSTROPHE
                            case 'C': // COMMA
                            case 'D': // MINUS
                            case 'E': // PERIOD
                            case 'F': // SLASH

                            case 'G': // 0
                            case 'H': // 1
                            case 'I': // 2
                            case 'J': // 3
                            case 'K': // 4
                            case 'L': // 5
                            case 'M': // 6
                            case 'N': // 7
                            case 'O': // 8
                            case 'P': // 9
                            case 'Q': // SEMICOLON
                            case 'R': // EQUAL

                            case 'S': // A
                            case 'T': // B
                            case 'U': // C
                            case 'V': // D
                            case 'W': // E
                            case 'X': // F
                            case 'Y': // G
                            case 'Z': // H
                            case 'a': // I
                            case 'b': // J
                            case 'c': // K
                            case 'd': // L
                            case 'e': // M
                            case 'f': // N
                            case 'g': // O
                            case 'h': // P
                            case 'i': // Q
                            case 'j': // R
                            case 'k': // S
                            case 'l': // T
                            case 'm': // U
                            case 'n': // V
                            case 'o': // W
                            case 'p': // X
                            case 'q': // Y
                            case 'r': // Z

                            case 's': // [
                            case 't': // BACKSLASH
                            case 'u': // ]

                            case 'v': // ` GRAVE ACCENT
                            case 'w': // WORLD-1
                            case 'x': // WORLD-2
                            case 'y': // ESCAPE
                            case 'z': // ENTER

                            case '.': // HOME
                            case '-': // END

                            case '^': // PRINT SCREEN
                            case '+': // SCROLLOCK
                            case '!': // PAUSE
                            case '=': // NUMLOCK
                            case ':': // CAPSLOCK

                            case '/': // F1
                            case '*': // F2
                            case '?': // F3
                            case '&': // F4
                            case '<': // F5
                            case '>': // F6
                            case '(': // F7
                            case ')': // F8
                            case '[': // F9
                            case ']': // F10
                            case '{': // F11
                            case '}': // F12
                            case '@': // F13
                            case '%': // F14
                            case '$': // F15
                            case '#': // F16
                            {
                                if (type>0)
                                {
                                    kbd[ stream[i+5] ] =  1;
                                }

                                if (type<0)
                                {
                                    kbd[ stream[i+5] ] = 0;
                                }

                                i+=8;
                                break;
                            }
                        }
                    }
                }

                if (j==i)
                    break;
            }

            if (i)
            {
                // something parsed
                stream_bytes = bytes - i;
                memmove(stream, stream+i, stream_bytes);
            }
            else
            {
                // reset in hope of resync
                memset(kbd,0,sizeof(int)*256);
                stream_bytes = 0;
            }
        }

        if (xterm_kitty)
        {
            float speed = 1.0;
            if (kbd[128 + 'a'] || kbd[128 + 'e']) // SHIFT (emulated from mods)
                speed *= 0.5;
            io.jump = kbd[128 + 'c'] || kbd[128 + 'g'] || kbd['A'] || mouse_j; // ALT / space
            io.x_force = (kbd['4'] - kbd['5'])*speed; // LEFT-RIGHT
            io.y_force = (kbd['7'] - kbd['6'])*speed; // UP-DOWN

            float len = sqrtf(io.x_force*io.x_force+io.y_force*io.y_force);
            if (len>0)
                speed /= len;
            io.x_force *= speed;
            io.y_force *= speed;


            io.torque = (int)(kbd['3'] || kbd['8'] || kbd['/']) - // DEL,PGUP,F1
                        (int)(kbd['2'] || kbd['9'] || kbd['*']);  // INS,PGDN,F2
        }
        else
        {
            // parse cursor keys + keypad + modifiers shift/alt
            // io.slow = false; // check for last input was capital or with shift
            float speed = 1.0;
            if (kbd[128 + 'a'] || kbd[128 + 'e']) // SHIFT (emulated from mods)
                speed *= 0.5;
            io.jump = kbd['A'] || kbd[128+'c'] || mouse_j;
            io.x_force = (kbd['4'] - kbd['5'])*speed; // LEFT-RIGHT
            io.y_force = (kbd['7'] - kbd['6'])*speed; // UP-DOWN

            float len = sqrtf(io.x_force*io.x_force+io.y_force*io.y_force);
            if (len>0)
                speed /= len;
            io.x_force *= speed;
            io.y_force *= speed;

            io.torque = (int)(kbd['3'] || kbd['8'] || kbd['/']) - // DEL,PGUP,F1
                        (int)(kbd['2'] || kbd['9'] || kbd['*']);  // INS,PGDN,F2        
        }

        if (mouse_b)
        {
            // override keyb
            float speed = 1.0;
            io.x_force = speed*2*(mouse_x*2 - wh[0]) / (float)wh[0];
            io.y_force = speed*2*(wh[1] - mouse_y*2) / (float)wh[1];
        }

        Animate(phys,stamp,&io);

        if (!io.jump)
        {
            if (xterm_kitty)
            {
                kbd[128 + 'c'] = 0;
                kbd[128 + 'g'] = 0;
            }
            else
                kbd[' '] = 0;
        }

        // get current terminal w,h
        // if changed realloc renderer output
        int nwh[2] = {0,0};
        GetWH(nwh);
        if (nwh[0]!=wh[0] || nwh[1]!=wh[1])
        {
            wh[0] = nwh[0];
            wh[1] = nwh[1];

            if (wh[0]*wh[1]>160*90)
            {
                float scale = sqrtf(160*90 / (wh[0]*wh[1]));
                wh[0] = floor(wh[0] * scale + 0.5f);
                wh[1] = floor(wh[1] * scale + 0.5f);
            }

            buf = (AnsiCell*)realloc(buf,wh[0]*wh[1]*sizeof(AnsiCell));
        }

        // check all pressed chars array, release if timed out

        // check number of chars awaiting in buffer
        // -> read them all, mark as pressed for 0.1 sec / if already pressed prolong

        // render
        Render(terrain,world,water,1.0,io.yaw,io.pos,lt,wh[0],wh[1],buf,io.player_dir,io.player_stp);

        // write to stdout
        Print(buf,wh[0],wh[1],CP437_UTF8);
        frames++;
    }

    exit:
    uint64_t end = GetTime();

    if (player_sprite)
	    FreeSprite(player_sprite);

    if (terrain)
        DeleteTerrain(terrain);

    if (world)
        DeleteWorld(world);

    if (buf)
        free(buf);

    SetScreen(false);

    
    printf("FPS: %f (%dx%d)\n", frames * 1000000.0 / (end-begin), wh[0], wh[1]);

#else

    printf("Currently -term parameter is unsupported on Windows\n");

#endif

	return 0;
}
