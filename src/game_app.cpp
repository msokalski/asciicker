
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#ifdef __linux__
# include <linux/limits.h>
#include <linux/input.h>
#include <linux/joystick.h>
#else
# include <limits.h>
#endif
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#ifdef USE_GPM
# include <gpm.h>
#endif

// work around including <netinet/tcp.h>
// which also defines TCP_CLOSE
#ifndef TCP_DELAY
#define TCP_NODELAY 1
#endif

#else
#define PATH_MAX 1024
#endif

#include <assert.h>

#include "render.h"
#include "physics.h"
#include "sprite.h"
#include "matrix.h"

#include "network.h"

// FOR GL 
#include "term.h"
#include "gl.h"
#include "gl45_emu.h"
#include "rgba8.h"

#include "game.h"
#include "enemygen.h"

int tty = -1;

// configurable, or auto lookup?

void Buzz()
{
}

char base_path[1024] = "./";

void SyncConf()
{
}

char conf_path[1024]="";
const char* GetConfPath()
{
	if (conf_path[0] == 0)
	{
		#if defined(__linux__) || defined(__APPLE__)
        const char* user_dir = getenv("SNAP_USER_DATA");
        if (!user_dir || user_dir[0]==0)
        {
            user_dir = getenv("HOME");
            if (!user_dir || user_dir[0]==0)
                sprintf(conf_path,"%sasciicker.cfg",base_path);
            else
                sprintf(conf_path,"%s/asciicker.cfg",user_dir);
        }
        else
            sprintf(conf_path,"%s/asciicker.cfg",user_dir);

 		#elif defined(_WIN32)
		
		const char* user_dir = getenv("APPDATA");
		if (!user_dir || user_dir[0] == 0)
			sprintf(conf_path, "%sasciicker.cfg", base_path);
		else
			sprintf(conf_path, "%s\\asciicker.cfg", user_dir);
		
		#endif
	}

	return conf_path;
}

#if defined(__linux__) || defined(__APPLE__)
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
int mouse_x = -1;
int mouse_y = -1;
int mouse_down = 0;
int gpm = -1;

bool GetWH(int wh[2])
{
    struct winsize size = {0};
    if (ioctl(0, TIOCGWINSZ, (char *)&size)>=0)
    {
        wh[0] = size.ws_col;
        wh[1] = size.ws_row;

        if (wh[0] > 160)
            wh[0] = 160;
        if (wh[1] > 90)
            wh[1] = 90;
    
    	return true;
    }

	return false;
}


void SetScreen(bool alt)
{
    // kitty kitty ...
    const char* term = 0; // getenv("TERM");
    if (term && strcmp(term,"xterm-kitty")==0)
    {
        xterm_kitty = alt;
        int w = write(STDOUT_FILENO, alt?"\x1B[?2017h":"\x1B[?2017l", 8);
    }
    
    // // \x1B[?1002h only drags \x1B[?1003h all mouse events
    // \x1B[?1006h enable extended mouse encodings in SGR < Bc;Px;Pym|M format 
    static const char* on_str = "\x1B[?1049h" "\x1B[H" "\x1B[?7l" "\x1B[?25l" "\x1B[?1002h" "\x1B[?1006h"; // +home, -wrap, -cursor, +mouse
    static const char* off_str = "\x1B[39m;\x1B[49m" "\x1B[?1049l" "\x1B[?7h" "\x1B[?25h" "\x1B[?1002l" "\x1B[?1006l"; // +def_fg/bg, +wrap, +cursor, -mouse
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
		int w = write(STDOUT_FILENO,on_str,on_len);
    }
	else
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
		int w = write(STDOUT_FILENO,off_str,off_len);

        if (tty>=0)
        {
            int wh[2];
            GetWH(wh);
            char jump[64]; // jump to last line, reset palette then clear it line
            int len = sprintf(jump,"\x1B[%d;%df\x1B]R\x1B[2K",wh[1],1);
            w = write(STDOUT_FILENO,jump,len);
        }
    }
}

#define FLUSH() \
    do \
    { \
        int w = write(STDOUT_FILENO,out,out_pos); \
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

uint8_t pal_16[256];

const uint8_t pal_rgba[256][3]=
{
    //{0,0,0},{0,0,170},{0,170,0},{0,85,170},{170,0,0},{170,0,170},{170,170,0},{170,170,170},
    //{85,85,85},{85,85,255},{85,255,85},{85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255},

    {0,0,0},{170,0,0},{0,170,0},{170,85,0},{0,0,170},{170,0,170},{0,170,170},{170,170,170},
    {85,85,85},{255,85,85},{85,255,85},{255,255,85},{85,85,255},{255,85,255},{85,255,255},{255,255,255},

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

    int fg16 = 0;
    int bk16 = 1;

#ifdef USE_GPM
    if (gpm>=0)
    {
        // bake mouse into buffer
        if (mouse_x>=0 && mouse_y>=0 && mouse_x<w && mouse_y<h)
        {
            static const AnsiCell mouse = { 0, 231, '+', 0 };
            buf[mouse_x + w*(h-1-mouse_y)] = mouse;
        }
    }
#endif // USE_GPM


    if (tty>=0)
    {
        // in linux virtual console we will use just 2 colors
        WRITE("\x1B[%d;%d;%dm",(fg16&7)+(fg16<8?30:90),(bk16&7)+40,bk16<8?25:5);    

        for (int y = h-1; y>=0; y--)
        {
            AnsiCell* ptr = buf + y*w;
            for (int x=0; x<w; x++,ptr++)
            {
                const char* chr = utf[ptr->gl];
                if (ptr->fg != fg)
                {
                    if (ptr->bk != bk)
                    {
                        WRITE("\e]P%X%02x%02x%02x", fg16, pal_rgba[ptr->fg][0], pal_rgba[ptr->fg][1], pal_rgba[ptr->fg][2]);
                        WRITE("\e]P%X%02x%02x%02x", bk16, pal_rgba[ptr->bk][0], pal_rgba[ptr->bk][1], pal_rgba[ptr->bk][2]);
                        WRITE("%s", chr);
                    }
                    else
                    {
                        WRITE("\e]P%X%02x%02x%02x", fg16, pal_rgba[ptr->fg][0], pal_rgba[ptr->fg][1], pal_rgba[ptr->fg][2]);
                        WRITE("%s", chr);
                    }
                }
                else
                {
                    if (ptr->bk != bk)
                    {
                        WRITE("\e]P%X%02x%02x%02x", bk16, pal_rgba[ptr->bk][0], pal_rgba[ptr->bk][1], pal_rgba[ptr->bk][2]);
                        WRITE("%s", chr);
                    }
                    else
                        WRITE("%s",chr);
                }
                bk=ptr->bk;
                fg=ptr->fg;
            }

            if (y)
                WRITE("\n");
        }
    }
    else
    {
        for (int y = h-1; y>=0; y--)
        {
            AnsiCell* ptr = buf + y*w;
            for (int x=0; x<w; x++,ptr++)
            {
                //const char* chr = (x+y)&1 ? "X":"Y";
                const char* chr = utf[ptr->gl];
                if (ptr->fg != fg)
                    if (ptr->bk != bk)
                        WRITE("\x1B[38;5;%d;48;5;%dm%s",ptr->fg,ptr->bk,chr);
                    else
                        WRITE("\x1B[38;5;%dm%s",ptr->fg,chr);
                else
                    if (ptr->bk != bk)
                        WRITE("\x1B[48;5;%dm%s",ptr->bk,chr);
                    else
                        WRITE("%s",chr);

                bk=ptr->bk;
                fg=ptr->fg;
            }

            if (y)
                WRITE("\n");
        }
    }

    FLUSH();
}

bool running = false;
void exit_handler(int signum)
{
    running = false;
    SetScreen(false);
    if (tty>0)
    {
        // restore old font
        const char* temp_dir = getenv("SNAP_USER_DATA");
        if (!temp_dir || !temp_dir[0])
            temp_dir = "/tmp";
        
        char cmd[2048];
        sprintf(cmd,"setfont %s/asciicker.%d.psf; rm %s/asciicker.%d.psf; clear;", temp_dir, tty, temp_dir, tty);
        system(cmd);
    }

    exit(0);
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

		gl3CreateTextures(GL_TEXTURE_2D, 1, &fnt->tex);
		gl3TextureStorage2D(fnt->tex, 1, ifmt, w, h);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		gl3TextureSubImage2D(fnt->tex, 0, 0, 0, w, h, fmt, type, buf ? buf : data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		float white_transp[4] = { 1,1,1,0 };

		gl3TextureParameteri2D(fnt->tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl3TextureParameteri2D(fnt->tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl3TextureParameteri2D(fnt->tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		gl3TextureParameteri2D(fnt->tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		gl3TextureParameterfv2D(fnt->tex, GL_TEXTURE_BORDER_COLOR, white_transp);


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
		gl3TextureSubImage2D(tex, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texel);
	}

	uint8_t GetTexel(int x, int y)
	{
		uint8_t texel[4];
		gl3GetTextureSubImage(tex, 0, x, y, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 4, texel);
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

// make term happy
float pos_x,pos_y,pos_z;
float rot_yaw;
int probe_z;
float global_lt[4];
World* world=0;
Terrain* terrain=0;

int font_zoom = 0;

int GetGLFont(int wh[2], const int wnd_wh[2])
{
    float err = 0;

    assert(wnd_wh);
    float area = (float)(wnd_wh[0]*wnd_wh[1]);

	int j = -1;
    for (int i=0; i<fonts_loaded; i++)
    {
        MyFont* f = font + i;
        float e = fabsf( 120.0f*75.0f - area / ((f->width>>4)*(f->height>>4)));

        if (!i || e<err)
        {
			j = i;
            err = e;
        }
    }

	j += font_zoom;
	j = j < 0 ? 0 : j >= fonts_loaded ? fonts_loaded - 1 : j;

	MyFont* f = font + j;

	if (wh)
	{
		wh[0] = f->width;
		wh[1] = f->height;
	}

	return f->tex;
}

static int tty_font = 4;
static const int tty_fonts[] = {6,8,10,12,14,16,18,20,24,28,32,-1};

// TODO WEB ZOOMING & FULLSCREENING!
// ...

#ifdef PURE_TERM
static bool xterm_fullscreen = false;
void ToggleFullscreen(Game* g)
{
    const char* term_env = getenv("TERM");
    if (!term_env)
        term_env = "";
    if (strcmp( term_env, "linux" ) != 0)
    {
        xterm_fullscreen = !xterm_fullscreen;
        if (xterm_fullscreen)
            int w = write(STDOUT_FILENO, "\033[9;1t",6);
        else
            int w = write(STDOUT_FILENO, "\033[9;0t",6);
    }
}

bool IsFullscreen(Game* g)
{
    return xterm_fullscreen;
}
#endif

bool PrevGLFont()
{
    #ifdef PURE_TERM
    if (tty>0)
    {
        tty_font--;
        if (tty_font<0)
            tty_font=0;
        char cmd[1024];
        sprintf(cmd,"setfont %sfonts/cp437_%dx%d.png.psf", base_path, tty_fonts[tty_font], tty_fonts[tty_font]);
        system(cmd);
    }
    else
    {
        // this will work only if xterm has enabled font ops
        int w = write(STDOUT_FILENO, "\033]50;#-1\a",9);
        if (xterm_fullscreen)
            int w = write(STDOUT_FILENO, "\033[9;1t",6);
        else
            int w = write(STDOUT_FILENO, "\033[9;0t",6);
    }
    #else
	font_zoom--;
	if (font_zoom < -fonts_loaded / 2)
	{
		font_zoom = -fonts_loaded / 2;
		return false;
	}
	TermResizeAll();
    #endif
	return true;
}

bool NextGLFont()
{
    #ifdef PURE_TERM
    if (tty>0)
    {
        tty_font++;
        if (tty_fonts[tty_font]<0)
            tty_font--;
        char cmd[1024];
        sprintf(cmd,"setfont %sfonts/cp437_%dx%d.png.psf", base_path, tty_fonts[tty_font], tty_fonts[tty_font]);
        system(cmd);
    }
    else
    {
        // this will work only if xterm has enabled font ops
        int w = write(STDOUT_FILENO, "\033]50;#+1\a",9);
        if (xterm_fullscreen)
            int w = write(STDOUT_FILENO, "\033[9;1t",6);
        else
            int w = write(STDOUT_FILENO, "\033[9;0t",6);
    }
    #else
	font_zoom++;
	if (font_zoom > fonts_loaded/2)
	{
		font_zoom = fonts_loaded/2;
		return false;
	}
	TermResizeAll();
    #endif
	return true;
}

Server* server = 0;

struct GameServer : Server
{
	TCP_SOCKET server_socket;

	static const int buf_size = 1<<16;
	uint8_t buf[buf_size];
	int buf_ofs;

	struct MSG_FIFO
	{
		uint8_t* ptr;
		int size;
	};

	static const int max_msg_size = 1 << 8;
	static const int msg_size = buf_size / max_msg_size;
	MSG_FIFO msg[msg_size];

	int msg_read; // r/w only by main-thread wrapped at 256 to 0
	int msg_write; // r/w only by net-thread wrapped at 256 to 0

	volatile unsigned int msg_num; // inter_inc by net-thread, inter_and(0) by main-thread

	bool Start()
	{
		head = 0;
		tail = 0;

		others = (Human*)malloc(sizeof(Human) * max_clients);
		
		buf_ofs = 0;
		msg_read = 0;
		msg_write = 0;
		msg_num = 0;

		bool ok = THREAD_CREATE_DETACHED(Entry, this);

		if (!ok)
		{
			return false;
		}

        stamp = GetTime();

		return true;
	}

	void Recv()
	{
		while (1)
		{
			while (msg_num == msg_size)
				THREAD_SLEEP(15);

			int r = WS_READ(server_socket, buf + buf_ofs, 2048, 0);

			MSG_FIFO* m = msg + msg_write;
			m->size = r;
			m->ptr = buf + buf_ofs;

			INTERLOCKED_INC(&msg_num);

            if (r<=0)
                break;

			msg_write = (msg_write + 1)&(msg_size - 1);

			buf_ofs += r;
			if (buf_size - buf_ofs < max_msg_size)
				buf_ofs = 0;
		}
	}

	static void* Entry(void* arg)
	{
		GameServer* gs = (GameServer*)arg;
		gs->Recv();
		return 0;
	}

	void Stop()
	{
		// finish thread
		// close socket
		if (server_socket != INVALID_TCP_SOCKET)
		{
			TCP_CLOSE(server_socket);
			TCP_CLEANUP();
		}

		server_socket = INVALID_TCP_SOCKET;

		if (others)
			free(others);
		others = 0;
	}
};

bool Server::Send(const uint8_t* data, int size)
{
	GameServer* gs = (GameServer*)this;
	int w = WS_WRITE(gs->server_socket, (const uint8_t*)data, size, 0, 0x2);
	if (w <= 0)
	{
        gs->Stop();
        free(others);
        free(server);
        server=0;
		return false;
	}
	return true;
}

void Server::Proc()
{
	GameServer* gs = (GameServer*)this;
	int num = gs->msg_num;
	for (int i = 0; i < num; i++)
	{
		GameServer::MSG_FIFO* m = gs->msg + gs->msg_read;
        if (m->size<=0)
        {
            free(others);
            free(server);
            server = 0;
            return;
        }
		Server::Proc(m->ptr, m->size); // this would be called directly by JS
		gs->msg_read = (gs->msg_read + 1)&(GameServer::msg_size - 1);
	}

	INTERLOCKED_SUB(&gs->msg_num, num);
}

void Server::Log(const char* str)
{
    //printf("%s",str);
}

GameServer* Connect(const char* addr, const char* port, const char* path, const char* user)
{
	int iResult;

	// Initialize Winsock
	iResult = TCP_INIT();
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 0;
	}

	TCP_SOCKET server_socket = INVALID_TCP_SOCKET;

	const char* hostname = addr;
	const char* portname = port;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo* result = 0;
	iResult = getaddrinfo(hostname, portname, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		TCP_CLEANUP();
		return 0;
	}

	// socket create and varification 
	server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (server_socket == INVALID_TCP_SOCKET)
	{
		printf("socket creation failed...\n");
		TCP_CLEANUP();
		return 0;
	}
	else
		printf("Socket successfully created..\n");

	// connect the client socket to server socket 
	if (connect(server_socket, result->ai_addr, (int)result->ai_addrlen) != 0)
	{
		printf("connection with the server failed...\n");
		TCP_CLOSE(server_socket);
		TCP_CLEANUP();
		return 0;
	}
	else
		printf("connected to the server..\n");

    int optval = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&optval, sizeof(optval)) != 0)
    {
        // ok we can live without it
    }

	optval = 1;
	if (setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&optval, sizeof(optval)) != 0)
	{
		// ok we can live without it
	}

	freeaddrinfo(result);

	// first, send HTTP->WS upgrade request (over http)
	const char* request_fmt =
		"GET /%s HTTP/1.1\r\n"
        "Host: %s\r\n"
#ifdef _WIN32
		"User-Agent: native-asciicker-windows\r\n"
#else
		"User-Agent: native-asciicker-linux\r\n"
#endif
		"Accept: */*\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Key: btsPdKGunHdaTPnSSDlfow==\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n"
		"Upgrade: WebSocket\r\n"
		"Connection: Upgrade\r\n\r\n";

    char request[2048];
    sprintf(request, request_fmt, path, addr);

	int w = TCP_WRITE(server_socket, (uint8_t*)request, (int)strlen(request));
	if (w < 0)
	{
		TCP_CLOSE(server_socket);
		TCP_CLEANUP();
		return 0;
	}

	// wait for response (check HTTP status / headers)
	struct Headers
	{
		static int cb(const char* header, const char* value, void* param)
		{
			Headers* h = (Headers*)param;

			if (header)
			{
				if (strcmp("Content-Length", header) == 0)
					h->content_len = atoi(value);
			}

			return 0;
		}

		int content_len;
	} headers;

	headers.content_len = 0;

	char buf[2048];
	int over_read = HTTP_READ(server_socket, Headers::cb, &headers, buf);

	if (over_read < 0 || over_read > headers.content_len || headers.content_len > 2048)
	{
		TCP_CLOSE(server_socket);
		TCP_CLEANUP();
		return 0;
	}

	while (headers.content_len > over_read)
	{
		int r = TCP_READ(server_socket, (uint8_t*)buf + over_read, headers.content_len - over_read);
		if (r <= 0)
		{
			TCP_CLOSE(server_socket);
			TCP_CLEANUP();
			return 0;
		}
		over_read += r;
	}

	// server should stay silent till we join the game
	// send JOIN command along with user name (over ws)
	STRUCT_REQ_JOIN req_join = { 0 };
	req_join.token = 'J';
	strncpy(req_join.name, user, 30);
	int ws = WS_WRITE(server_socket, (uint8_t*)&req_join, sizeof(STRUCT_REQ_JOIN), 0, 0x2);
	if (ws <= 0)
	{
		TCP_CLOSE(server_socket);
		TCP_CLEANUP();
		return 0;
	}

	// Recv for ID (over ws) (it can send us some content to display!)
	STRUCT_RSP_JOIN rsp_join = { 0 };
	ws = WS_READ(server_socket, (uint8_t*)&rsp_join, sizeof(STRUCT_RSP_JOIN), 0);
	if (ws <= 0 || rsp_join.token != 'j')
	{
		TCP_CLOSE(server_socket);
		TCP_CLEANUP();
		return 0;
	}

	int ID = rsp_join.id;
	printf("connected with ID:%d/%d\n", ID, rsp_join.maxcli);

	GameServer* gs = (GameServer*)malloc(sizeof(GameServer));
	gs->server_socket = server_socket;
	gs->max_clients = rsp_join.maxcli;

	return gs;
}

extern "C" void DumpLeakCounter();

#if defined(__linux__) || defined(__APPLE__)

static int find_tty()
{
    char buf[256];
    char* ptr;
    FILE* f;
    int r;
    char stat;
    int ppid,pgrp,sess,tty;

    int pid = getpid();

    while (pid>0)
    {
        sprintf(buf,"/proc/%d/stat",pid);
        f = fopen(buf,"r");
        if (!f)
            return 0;
        r = fread(buf,1,255,f);
        fclose(f);
        if (r<=0)
            return 0;
        buf[r] = 0;
        ptr = strchr(buf,')');
        if (!ptr || !ptr[1])
            return 0;
        r = sscanf(ptr+2,"%c %d %d %d %d", &stat,&ppid,&pgrp,&sess,&tty);
        if (r!=5)
            return 0;
        if ( (tty&~63) == 1024 && (tty&63) )
            return tty&63;
        pid = ppid;
    }
    return 0;
}
#endif

#ifdef __linux__

#define KEY_MAX_LARGE 0x2FF
//#define KEY_MAX_SMALL 0x1FF
#define AXMAP_SIZE (ABS_MAX + 1)
#define BTNMAP_SIZE (KEY_MAX_LARGE - BTN_MISC + 1)

static uint16_t js_btnmap[BTNMAP_SIZE];
static uint8_t  js_axmap[AXMAP_SIZE];

static const int js_btnmap_sdl[]=
{
    0, 1, -1,    // A,B
    2, 3, -1,    // X,Y
    9,10, -1,-1, // L,R SHOULDERS
    4,6,5,       // BACK, START, GUIDE
    7,8, -1,     // L,R STICK
    -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1    
};

static const int js_axmap_sdl[]=
{
    0,1,4, // LEFT X,Y,TRIG
    2,3,5, // RIGHT X,Y,TRIG

    -1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1,

    0xFE,0xFF, // X,Y AXIS FOR DIRPAD (left:13, right:14, up:11, down:12) !!!
    -1,-1, -1,-1,-1,-1, 
    -1,-1,-1,-1, -1,-1,-1,-1
};
#endif


int scan_js(char* gamepad_name, int* gamepad_axes, int* gamepad_buttons, uint8_t* gamepad_mapping)
{
    #ifdef __linux__
    static int index = 0;
    static int skip = 0;

    if (skip>0)
    {
        skip--;
        return -1;
    }

    char js_term_dev[32];
    sprintf(js_term_dev,"/dev/input/js%d",index);

    int fd = -1;
    if ((fd = open(js_term_dev, O_RDONLY)) < 0) 
    {
        // printf("can't open %s\n", js_term_dev);
        index = (index+1) & 0xF;
        skip = 10;
		fd = -1;
	}
    else
    {
        #define NAME_LENGTH 128
        unsigned char axes = 2;
        unsigned char buttons = 2;
        int version = 0x000800;
        char name[NAME_LENGTH] = "Unknown";

        ioctl(fd, JSIOCGVERSION, &version);
        ioctl(fd, JSIOCGAXES, &axes);
        ioctl(fd, JSIOCGBUTTONS, &buttons);
        ioctl(fd, JSIOCGNAME(NAME_LENGTH), name);

        strcpy(gamepad_name,name);
        *gamepad_axes = axes;
        *gamepad_buttons = buttons;

        // fetch button map
        memset(js_btnmap,-1,sizeof(js_btnmap));
        int btnmap_res = ioctl(fd, JSIOCGBTNMAP, js_btnmap);

        // fetch axis map
        memset(js_axmap,-1,sizeof(js_axmap));
        int axmap_res = ioctl(fd, JSIOCGAXMAP, js_axmap);


        // construct mapping
        uint8_t* m = gamepad_mapping;
        for (int i=0; i<buttons; i++)
        {
            int abs = 2*axes + i;
            switch(js_btnmap[i])
            {
                case BTN_A: m[abs] = (1<<7) | (0<<6) | 0x00; break;
                case BTN_B: m[abs] = (1<<7) | (0<<6) | 0x01; break;
                case BTN_X: m[abs] = (1<<7) | (0<<6) | 0x02; break;
                case BTN_Y: m[abs] = (1<<7) | (0<<6) | 0x03; break;

                case BTN_SELECT: m[abs] = (1<<7) | (0<<6) | 0x04/*back_button*/; break;
                case BTN_MODE:   m[abs] = (1<<7) | (0<<6) | 0x05/*guide_button*/; break;
                case BTN_START:  m[abs] = (1<<7) | (0<<6) | 0x06/*start_button*/; break;

                case BTN_THUMBL: m[abs] = (1<<7) | (0<<6) | 0x07/*left_stick_button*/; break;
                case BTN_THUMBR: m[abs] = (1<<7) | (0<<6) | 0x08/*right_stick_button*/; break;

                case BTN_TL: m[abs] = (1<<7) | (0<<6) | 0x09/*left_shoulder_button*/; break;
                case BTN_TR: m[abs] = (1<<7) | (0<<6) | 0x0A/*right_shoulder_button*/; break;

                case BTN_TL2: m[abs] = (0<<7) | (0<<6) | 0x02/*left_trigger_axis*/; break;
                case BTN_TR2: m[abs] = (0<<7) | (0<<6) | 0x05/*right_trigger_axis*/; break;

                default: 
                    m[i] = 0xFF;
            }
        }

        for (int i=0; i<axes; i++)
        {
            int neg = 2*i;
            int pos = 2*i+1;
            switch(js_axmap[i])
            {
                case 0: //left-x
                    m[neg] = (0<<7) | (1<<6) | 0x00; 
                    m[pos] = (0<<7) | (0<<6) | 0x00; 
                    break;

                case 1: //left-y
                    m[neg] = (0<<7) | (1<<6) | 0x01; 
                    m[pos] = (0<<7) | (0<<6) | 0x01; 
                    break;

                case 2: // left-z (compressed, 0x04 output is unsigned )
                    m[neg] = (0<<7) | (0<<6) | 0x04;
                    m[pos] = (0<<7) | (0<<6) | 0x04; 
                    break;

                case 3: //right-x
                    m[neg] = (0<<7) | (1<<6) | 0x02; 
                    m[pos] = (0<<7) | (0<<6) | 0x02; 
                    break;

                case 4: //right-y
                    m[neg] = (0<<7) | (1<<6) | 0x03; 
                    m[pos] = (0<<7) | (0<<6) | 0x03; 
                    break;

                case 5: //right-z (compressed, 0x05 output is unsigned )
                    m[neg] = (0<<7) | (0<<6) | 0x05; 
                    m[pos] = (0<<7) | (0<<6) | 0x05; 
                    break;

                case 16: // dirpad-x
                    m[neg] = (1<<7) | (0<<6) | 0x0D; 
                    m[pos] = (1<<7) | (0<<6) | 0x0E; 
                    break;

                case 17: // dirpad-y
                    m[neg] = (1<<7) | (0<<6) | 0x0B; 
                    m[pos] = (1<<7) | (0<<6) | 0x0C; 
                    break;

                default: 
                    m[i] = 0xFF;
            }
        }
    }

    return fd;
    #endif        

    return -1;    
}


bool read_js(int fd)
{
    #ifdef __linux__
        #define MAX_JS_READ 64
        js_event js_arr[MAX_JS_READ];
        int size = read(fd, js_arr, sizeof(js_event)*MAX_JS_READ);
        if (size<=0 || size % sizeof(js_event))
            return false;

        /*
        static int dirpad_x = 0;
        static int dirpad_y = 0;
        */

        int n = size / sizeof(js_event);
        for (int i=0; i<n; i++)
        {
            js_event* js = js_arr+i;

            // process
            switch(js->type & ~JS_EVENT_INIT) 
            {
                case JS_EVENT_BUTTON:
                    GamePadButton(js->number,js->value ? 32767 : 0);
                    break;
                case JS_EVENT_AXIS:
                    GamePadAxis(js->number,js->value == -32768 ? -32767 : js->value);
                    break;
            }
        }

        return true;
    #endif

    return false;
}

int main(int argc, char* argv[])
{
	/*
	FILE* fpal = fopen("d:\\ascii-work\\asciicker.act", "wb");
	for (int i = 0; i < 16; i++)
	{
		uint8_t col[3] = { 0,0,0 };
		fwrite(col, 3, 1, fpal);
	}
	for (int r = 0; r < 6; r++)
	for (int g = 0; g < 6; g++)
	for (int b = 0; b < 6; b++)
	{
		uint8_t col[3] = { r*51,g*51,b*51 };
		fwrite(col, 3, 1, fpal);
	}
	for (int i = 0; i < 24; i++)
	{
		uint8_t col[3] = { 0,0,0 };
		fwrite(col, 3, 1, fpal);
	}
	return 0;
	*/

    char abs_buf[PATH_MAX];
    char* abs_path = 0;

    if (argc < 1)
        strcpy(base_path,"./");
    else
    {
        size_t len = 0;
        #if defined(__linux__) || defined(__APPLE__)
        abs_path = realpath(argv[0], abs_buf);
        char* last_slash = strrchr(abs_path, '/');
        if (!last_slash)
            strcpy(base_path,"./");
        else
        {
            len = last_slash - abs_path + 1;
            memcpy(base_path,abs_path,len);
            base_path[len] = 0;
        }
        #else
        GetFullPathNameA(argv[0],1024,abs_buf,&abs_path);
		memcpy(base_path, abs_buf, abs_path - abs_buf);
		#endif

        len = strlen(base_path);

		if (len > 4)
		{
			char* dotrun[4] =
			{
				strstr(base_path, "/build/"),
#ifdef _WIN32
				strstr(base_path, "\\build\\"),
				strstr(base_path, "\\build/"),
				strstr(base_path, "/build\\"),
#else
				0,0,0
#endif
			};

			int dotpos = -1;
			for (int i = 0; i < 4; i++)
			{
				if (dotrun[i])
				{
					int pos = dotrun[i] - base_path;
					if (dotpos < 0 || pos < dotpos)
						dotpos = pos;
				}
			}

			if (dotpos >= 0)
				base_path[dotpos+1] = 0;
		}
    }

    printf("exec path: %s\n", argv[0]);
    printf("BASE PATH: %s\n", base_path);

    /*
    int c16 = 13;
    printf("\x1B[%d;%dm%s",(c16&7)+40,c16<8?25:5,"\n");

    for (int b=0; b<6; b++)
    {
        for (int r=0; r<6; r++)
        {
            for (int g=0; g<6; g++)
            {
                int c256 = b + 6*g + 36*r + 16;
                printf("\e]P%X%02x%02x%02x", c16, pal_rgba[c256][0], pal_rgba[c256][1], pal_rgba[c256][2]);
                printf(" ");
            }
        }
        printf("\n");
    }

    exit(0);
    */



#ifdef _WIN32
	
	PostMessage(GetConsoleWindow(), WM_SYSCOMMAND, SC_MINIMIZE, 0);
	//_CrtSetBreakAlloc(5971);
#endif

    // if there is no '-term' arg given run A3D (GL) term
    // ...

    // otherwise continue with following crap
    // ...

    // consider configuring kbd, it solves only key up, not multiple keys down
    /*
        gsettings set org.gnome.desktop.peripherals.keyboard repeat-interval 30
        gsettings set org.gnome.desktop.peripherals.keyboard delay 250    
    */

	// if -user is given get its name and connect to server !


	char* url = 0; // must be in form [user@]server_address/path[:port]

	// to be upgraded by host (good if no encryption is needed but can't connect on weird port)
	/*
	const char* user = "player";
	const char* addr = "asciicker.com";
	const char* path = "/ws/y8/";
	const char* port = "80";
	*/

	// to be upgraded by host (good if encryption is needed)
	/*
	const char* user = "player";
	const char* addr = "asciicker.com";
	const char* path = "/ws/y8/";
	const char* port = "443";
	*/

	// directly (best but no encryption and requires weird port to be allowed to send request to)
	/*
	const char* user = "player";
	const char* addr = "asciicker.com";
	const char* path = "/ws/y8/"; // just to check if same as server expects
	const char* port = "8080";
	*/

    bool term = false;
    for (int p=1; p<argc; p++)
    {
        if (strcmp(argv[p],"-term")==0)
            term = true;
		else
		if (p+1<argc)
		{
			if (strcmp(argv[p], "-url") == 0)
			{
				p++;
				url = argv[p];
			}
		}
    }

	// NET_TODO:
	// if url is given try to open connection
	GameServer* gs = 0;

	if (url)
	{
		// [user@]server_address/path[:port]
		char* monkey = strchr(url, '@');
		char* colon = monkey ? strchr(monkey, ':') : strchr(url, ':');
        char* slash = colon ? strchr(colon, '/') : monkey ? strchr(monkey, '/') : strchr(url, '/');

        char def_user[] = "player";
        char def_port[] = "8080";
        char def_path[] = "";

		char* addr = url;
		char* user = def_user;
		char* port = def_port;
		char* path = def_path;

		if (monkey)
		{
			*monkey = 0;
			user = url;
			addr = monkey + 1;
		}

		if (colon)
		{
			*colon = 0;
			port = colon + 1;
		}

        if (slash)
        {
            *slash = 0;
            path = slash + 1;
        }

        if (addr && addr[0])
        {
            gs = Connect(addr, port, path, user);
            if (!gs)
            {
                printf("Couldn't connect to server, starting solo ...\n");
            }
        }

		strcpy(player_name, user);
        ConvertToCP437(player_name_cp437, player_name);

		// here we should know if server is present or not
		// so we can creare game or term with or without server
		// ...
	}
    else
    {
        strcpy(player_name, "player");
        ConvertToCP437(player_name_cp437, player_name);
    }
    

    float water = 55;
    float dir = 0;

    float yaw = 45;
    float pos[3] = {0,15,0};
    float lt[4] = {1,0,1,.5};

	float last_yaw = yaw;

	LoadSprites();

	{
        char a3d_path[1024];
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
            UpdateTerrainDark(terrain, world, lt, false);
            #endif
        }
	}

	if (gs)
	{
		server = gs;

		if (!gs->Start())
		{
			TCP_CLEANUP();
			return false;
		}
	}

    #ifndef PURE_TERM
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

        if (TermOpen(0, yaw, pos, MyFont::Free))
        {
            char font_dirname[1024];
            sprintf(font_dirname, "%sfonts", base_path); // = "./fonts";
            fonts_loaded = 0;
            a3dListDir(font_dirname, MyFont::Scan, font_dirname);

			LoopInterface li = { GamePadMount, GamePadUnmount, GamePadButton, GamePadAxis };
            a3dLoop(&li);
        }

		// NET_TODO:
		// close network if open
		// ...

        if (terrain)
            DeleteTerrain(terrain);

        if (world)
            DeleteWorld(world);

		PurgeItemInstCache();

		FreeSprites();

		DumpLeakCounter();

#ifdef _WIN32
		_CrtDumpMemoryLeaks();
#endif
        return 0;
    }

#endif // #ifndef PURE_TERM

#if defined(__linux__) || defined(__APPLE__)

    // recursively check if we are on TTY console or 'vt'
    const char* term_env = getenv("TERM");
    if (!term_env)
        term_env = "";

    printf("TERM=%s\n",term_env);

    if (strcmp( term_env, "linux" ) == 0)
    {
        tty = find_tty();
    }

    if (tty > 0)
    {
        // store current font
        char cmd[1024];
        const char* temp_dir = getenv("SNAP_USER_DATA");
        if (!temp_dir || !temp_dir[0])
            temp_dir = "/tmp";
        sprintf(cmd,"setfont -O %s/asciicker.%d.psf;", temp_dir,tty);
        system(cmd);

        // setup default font
        sprintf(cmd,"setfont %sfonts/cp437_%dx%d.png.psf", base_path, tty_fonts[tty_font], tty_fonts[tty_font]);
        system(cmd);

#ifdef USE_GPM
        Gpm_Connect conn;
        conn.eventMask  = ~0;   /* Want to know about all the events */
        conn.defaultMask = 0;   /* don't handle anything by default  */
        conn.minMod     = 0;    /* want everything                   */  
        conn.maxMod     = ~0;   /* all modifiers included            */
        
        gpm_handler = 0;        
        gpm_visiblepointer = 0;
        gpm = Gpm_Open(&conn, tty);

        if (gpm >=0)
        {
            int wh[2];
            GetWH(wh);
            mouse_x = wh[0]/2;
            mouse_y = wh[1]/2;
            printf("connected to gpm\n");
        }
        else
        {
            printf("failed to connect to gpm\n");
            // exit(0);
        }
#endif // USE_GPM
    }
    else
    if (strncmp(term_env,"xterm",5)==0)
    {
        printf("VIRTUAL TERMINAL EMULGLATOR\n");

        /*
            1.  MANDATORY: install -gumix-*... fonts
            
                cd ..directory_with_fonts
                if not exist font.dir
                    mkfontdir
                xset fp+ /home/user/asciiid/fonts
                xset fp rehash

            2.  CONFIGURING XTerm during startup

                2A. CONFIGURE Xterm to use 7 gumix fonts on startup
                    xterm \
                    -xrm "xterm*font1: -gumix-*-*-*-*-*-8-*-*-*-*-*-*" \
                    -xrm "xterm*font2: -gumix-*-*-*-*-*-10-*-*-*-*-*-*" \
                    -xrm "xterm*font3: -gumix-*-*-*-*-*-12-*-*-*-*-*-*" \
                    -xrm "xterm*font:  -gumix-*-*-*-*-*-14-*-*-*-*-*-*" \
                    -xrm "xterm*font4: -gumix-*-*-*-*-*-16-*-*-*-*-*-*" \
                    -xrm "xterm*font5: -gumix-*-*-*-*-*-18-*-*-*-*-*-*" \
                    -xrm "xterm*font6: -gumix-*-*-*-*-*-20-*-*-*-*-*-*"

                    -xrm "xterm*allowWindowOps: true"
                    -xrm "xterm*allowFontOps: true"

                    // color and mouse ops are enabled by default
                    // title and TCap Ops are not neccessary 

                    user can switch between fonts using SHIFT+NUMPAD(+/-)

                2B. As above but all resource strings can be stored in .Xresources
                    optionally using asciicker as a class name (insteead of xterm)
                    xterm -class asciicker

                2C. CONFIGURE Xterm on startup to use just 1 font
                    xterm -fn -gumix-*-*-*-*-*-14-*-*-*-*-*-*"

            3.  Setting current font in RUN-TIME!
                Possible only if xterm has 'Allow Font Ops' enabled
                echo -ne "\e]50;-gumix-*-*-*-*-*-14-*-*-*-*-*-*-*\a"
                
                also  it is possible to switch between 7 fonts (replace <n> with digit):
                echo -ne "\e]50;#<n>\a"

                both methods could be uses to compensate for too small or too large resolution
        */

        // palette setup
        for (int col = 16; col<232; col++)
        {
            const uint8_t* c = pal_rgba[col];
            printf("\x1B]4;%d;#%02x%02x%02x\a", col, c[0],c[1],c[2]);
        }
        printf("\n");
    }
    else
        printf("UNKNOWN TERMINAL\n");

    // try opening js device
    int gamepad_axes = 0;
    int gamepad_buttons = 0;
    char gamepad_name[256] = {0};
    uint8_t gamepad_mapping[256] = {0};
    int jsfd = scan_js(gamepad_name,&gamepad_axes,&gamepad_buttons,gamepad_mapping);

    SetScreen(true);

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

    /*
    if (!xterm_kitty)
    {
        SetScreen(false);
        printf("TERMINATING: this pre-update build requires kitty terminal to run\n");
        return -1;
    }
    */

    //terrain = 0;
    //world = 0;

    Game* game = 0;

    AnsiCell* buf = 0;
    int wh[2] = {-1,-1};    

    uint64_t begin = GetTime();
    uint64_t stamp = begin;
    uint64_t frames = 0;

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

        double noon_pos[4] =
        {
            noon_yaw[0]*cos(lit_pitch*M_PI / 180),
            noon_yaw[1]*cos(lit_pitch*M_PI / 180),
            sin(lit_pitch*M_PI / 180),
            0
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

    game = CreateGame(water,pos,yaw,dir,stamp);
    
    if (jsfd>=0)
    {
        // report mount
        GamePadMount(gamepad_name,gamepad_axes,gamepad_buttons,gamepad_mapping);
    }

    while(running)
    {
        if (jsfd<0)
        {
            // report mount
            jsfd = scan_js(gamepad_name,&gamepad_axes,&gamepad_buttons,gamepad_mapping);
            if (jsfd >= 0)
                GamePadMount(gamepad_name,gamepad_axes,gamepad_buttons,gamepad_mapping);
        }        
     
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

        struct pollfd pfds[3]={0};
        if (gpm>=0)
        {
            pfds[0].fd = STDIN_FILENO;
            pfds[0].events = POLLIN; 
            pfds[0].revents = 0;

            pfds[1].fd = gpm;
            pfds[1].events = POLLIN; 
            pfds[1].revents = 0;

            if (jsfd>=0)
            {
                pfds[2].fd = jsfd;
                pfds[2].events = POLLIN|POLL_HUP|POLL_ERR; 
                pfds[2].revents = 0;
                poll(pfds, 3, 0); // 0 no timeout, -1 block

                if (pfds[2].revents & (POLLHUP|POLLERR))
                {
                    GamePadUnmount();
                    close(jsfd);
                    jsfd=-1;                    
                }
                else
                if (pfds[2].revents & POLLIN) 
                {
                    if (!read_js(jsfd))
                    {
                        GamePadUnmount();
                        close(jsfd);
                        jsfd=-1;
                    }
                }
            }
            else
                poll(pfds, 2, 0); // 0 no timeout, -1 block

#ifdef USE_GPM
            if (pfds[1].revents & POLLIN)
            {
                static int mouse_read = 0;
                static int mouse_write = 0;
                static Gpm_Event mouse_buf[64];
                int bytes = read(gpm,(char*)mouse_buf + mouse_write,32*sizeof(Gpm_Event));
                mouse_write += bytes;

                int events = mouse_write / sizeof(Gpm_Event) - mouse_read;

                while (events)
                {
                    Gpm_Event* event = mouse_buf + mouse_read;

                    events--;
                    mouse_read++;

                    mouse_x += event->dx;
                    mouse_y += event->dy;

                    if (mouse_x >= wh[0])
                        mouse_x = wh[0]-1;
                    if (mouse_x < 0)
                        mouse_x = 0;
                    if (mouse_y >= wh[1])
                        mouse_y = wh[1]-1;
                    if (mouse_y < 0)
                        mouse_y = 0;

                    bool xy_processed = false;

                    if (event->wdy>0)
                    {  
                        xy_processed = true;
                        game->OnMouse(Game::MOUSE_WHEEL_UP, mouse_x, mouse_y);
                    }
                    else
                    if (event->wdy<0)
                    {
                        xy_processed = true;
                        game->OnMouse(Game::MOUSE_WHEEL_DOWN, mouse_x, mouse_y);
                    }

                    if (event->type & GPM_DOWN)
                    {
                        if (!(mouse_down&GPM_B_LEFT) && (event->buttons & GPM_B_LEFT))
                        {
                            xy_processed = true;
                            mouse_down|=GPM_B_LEFT;
                            game->OnMouse(Game::MOUSE_LEFT_BUT_DOWN, mouse_x, mouse_y);
                        }
                        if (!(mouse_down&GPM_B_MIDDLE) && (event->buttons & GPM_B_MIDDLE))
                        {
                            xy_processed = true;
                            mouse_down|=GPM_B_MIDDLE;
                            game->OnMouse(Game::MOUSE_MIDDLE_BUT_DOWN, mouse_x, mouse_y);
                        }
                        if (!(mouse_down&GPM_B_RIGHT) && (event->buttons & GPM_B_RIGHT))
                        {
                            xy_processed = true;
                            mouse_down|=GPM_B_RIGHT;
                            game->OnMouse(Game::MOUSE_RIGHT_BUT_DOWN, mouse_x, mouse_y);
                        }
                    }

                    if (event->type & GPM_UP)
                    {
                        if ((mouse_down&GPM_B_LEFT) && (event->buttons & GPM_B_LEFT))
                        {
                            xy_processed = true;
                            mouse_down&=~GPM_B_LEFT;
                            game->OnMouse(Game::MOUSE_LEFT_BUT_UP, mouse_x, mouse_y);
                        }
                        if ((mouse_down&GPM_B_MIDDLE) && (event->buttons & GPM_B_MIDDLE))
                        {
                            xy_processed = true;
                            mouse_down&=~GPM_B_MIDDLE;
                            game->OnMouse(Game::MOUSE_MIDDLE_BUT_UP, mouse_x, mouse_y);
                        }
                        if ((mouse_down&GPM_B_RIGHT) && (event->buttons & GPM_B_RIGHT))
                        {
                            xy_processed = true;
                            mouse_down&=~GPM_B_RIGHT;
                            game->OnMouse(Game::MOUSE_RIGHT_BUT_UP, mouse_x, mouse_y);
                        }
                    }                

                    if (!xy_processed && (event->type & (GPM_MOVE|GPM_DRAG)))
                    {
                        xy_processed = true;
                        game->OnMouse(Game::MOUSE_MOVE, mouse_x, mouse_y);
                    }
                }

                if (mouse_write>=32*sizeof(Gpm_Event))
                {
                    size_t tail = mouse_write - sizeof(Gpm_Event)*mouse_read;
                    if (tail)
                        memcpy(mouse_buf,mouse_buf + mouse_read, tail);
                    mouse_write = tail;
                    mouse_read = 0;
                }
            }
#endif // USE_GPM
        }
        else
        {
            // prep for poll
            pfds[0].fd = STDIN_FILENO;
            pfds[0].events = POLLIN; 
            pfds[0].revents = 0;

            if (jsfd>=0)
            {
                pfds[1].fd = jsfd;
                pfds[1].events = POLLIN|POLLHUP|POLLERR; 
                pfds[1].revents = 0;
                poll(pfds, 2, 0); // 0 no timeout, -1 block

                if (pfds[1].revents & (POLLHUP|POLLERR))
                {
                    GamePadUnmount();
                    close(jsfd);
                    jsfd=-1;                    
                }
                else
                if (pfds[1].revents & POLLIN) 
                {
                    if (!read_js(jsfd))
                    {
                        GamePadUnmount();
                        close(jsfd);
                        jsfd=-1;
                    }
                }
            }
            else
                poll(pfds, 1, 0); // 0 no timeout, -1 block
        }

        if (pfds[0].revents & POLLIN) 
        {
            static int stream_bytes = 0;
            static char stream[256];

            char fresh[256];

            int fresh_bytes = read(STDIN_FILENO, fresh, 256 - stream_bytes);
            memcpy(stream + stream_bytes, fresh, fresh_bytes);

            int bytes = stream_bytes + fresh_bytes;

            /*
            FILE* kl = fopen("keylog.txt","a");
            for (int i=0; i<bytes; i++)
                fprintf(kl,"0x%02X, ",stream[i]);
            fprintf(kl,"\n");
            fclose(kl);
            */

            int i = 0;
            while (i<bytes)
            {
                int j=i;

                int type = 0, mods = 0;

                if (stream[i]>=' ' && stream[i]<=127 || 
                    stream[i]==8 || stream[i]=='\r' || stream[i]=='\n' || stream[i]=='\t')
                {
                    if (stream[i] == ' ')
                    {
                        // we need it both as char for TalkBox and as press for Jump
                        game->OnKeyb(Game::GAME_KEYB::KEYB_CHAR, ' ');
                        game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_SPACE);
                    }
                    else
                    if (stream[i] == '\t')
                        game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_TAB);
                    else
                    if (stream[i] == 127) // backspace (8) is encoded as del (127)
                        game->OnKeyb(Game::GAME_KEYB::KEYB_CHAR, 8);
                    else
                        game->OnKeyb(Game::GAME_KEYB::KEYB_CHAR, stream[i]);
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
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_F1);
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == 'O' && stream[i+2] == 'Q')
                {
                    // F2
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_F2);
                    i+=3;
                }

                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '3' && stream[i+3] == '~')
                {
                    // DEL
                    game->OnKeyb(Game::GAME_KEYB::KEYB_CHAR, 127);
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '2' && stream[i+3] == '~')
                {
                    // INS
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_INSERT);
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '5' && stream[i+3] == '~')
                {
                    // PGUP
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_PAGEUP);
                    i+=4;
                }
                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '6' && stream[i+3] == '~')
                {
                    // PGDN
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_PAGEDOWN);
                    i+=4;
                }

                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'D')
                {
                    // left
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_LEFT);
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'C')
                {
                    // right
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_RIGHT);
                    i+=3;
                }                
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'A')
                {
                    // up
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_UP);
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'B')
                {
                    // down
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_DOWN);
                    i+=3;
                }    
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'H')
                {
                    // home
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_HOME);
                    i+=3;
                }
                if (bytes-i >=3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == 'F')
                {
                    // end
                    game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_END);
                    i+=3;
                }    

                if (bytes-i >=4 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '1' && stream[i+3] == ';')
                {
                    if (bytes-i < 6)
                        break;

                    int mods = stream[i+4] - 0x31;
                    int a3d_mods = 0;

                    if (mods&1)
                    {
                        // shift
                        a3d_mods |= 0x1<<8;
                    }
                    if (mods&2)
                    {
                        // alt
                        a3d_mods |= 0x2<<8;
                    }
                    if (mods&4)
                    {
                        // ctrl ???
                        a3d_mods |= 0x4<<8;
                    }
                    

                    switch (stream[i+5])
                    {
                        case 'P': // f1
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_F1 | a3d_mods);
                            break;
                        case 'Q': // f2
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_F2 | a3d_mods);
                            break;
                        case '3': // del
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_DELETE | a3d_mods);
                            break;
                        case '2': // ins
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_INSERT | a3d_mods);
                            break;
                        case '5': // pgup
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_PAGEUP | a3d_mods);
                            break;
                        case '6': // pgdn
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_PAGEDOWN | a3d_mods);
                            break;

                        case 'A': // up
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_UP | a3d_mods);
                            break;
                        case 'B': // down
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_DOWN | a3d_mods);
                            break;
                        case 'C': // right
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_RIGHT | a3d_mods);
                            break;
                        case 'D': // left
                            game->OnKeyb(Game::GAME_KEYB::KEYB_PRESS, A3D_LEFT | a3d_mods);
                            break;
                    }

                    i+=6;
                }            

                // mouse SGR (1006) -> CSI < Bc;Px;Py;M (press) or CSI < Bc;Px;Py;m (release) 
                if (bytes-i >= 3 && stream[i] == 0x1B && stream[i+1] == '[' && stream[i+2] == '<')
                {
                    int k=i+3;

                    int val[3]={0,0,0};
                    int fields=0, offset=0;
                    while (bytes-k>0)
                    {
                        if (stream[k]<'0' || stream[k]>'9')
                        {
                            int c = stream[k];
                            val[fields] = atoi(stream+k-offset);
                            fields++;
                            offset=0;
                            k++;

                            switch (c)
                            {
                                case ';':
                                    if (fields<3)
                                        continue;
                                    break;

                                case 'm':
                                case 'M':
                                {
                                    if (fields==3)
                                    {
                                        int but = val[0] & 0x3;

										if (val[0] >= 64)
										{
											// wheel
											if (but == 0)
												game->OnMouse(Game::MOUSE_WHEEL_UP, val[1] - 1, val[2] - 1);
											else
											if (but == 1)
												game->OnMouse(Game::MOUSE_WHEEL_DOWN, val[1] - 1, val[2] - 1);
										}
										else
                                        if (val[0] >= 32)
                                        {
                                            // motion
                                            int a=0;
                                            game->OnMouse(Game::MOUSE_MOVE,val[1]-1,val[2]-1);
                                        }
                                        else
                                        if (c=='M')
                                        {
                                            switch(but)
                                            {
                                                case 0:
                                                    game->OnMouse(Game::MOUSE_LEFT_BUT_DOWN,val[1]-1,val[2]-1);
                                                    break;
                                                case 2:
                                                    game->OnMouse(Game::MOUSE_RIGHT_BUT_DOWN,val[1]-1,val[2]-1);
                                                    break;

                                                default:
                                                    game->OnMouse(Game::MOUSE_MOVE,val[1]-1,val[2]-1);
                                            }
                                        }
                                        else
                                        if (c=='m')
                                        {
                                            switch(but)
                                            {
                                                case 0:
                                                    game->OnMouse(Game::MOUSE_LEFT_BUT_UP,val[1]-1,val[2]-1);
                                                    break;
                                                case 2:
                                                    game->OnMouse(Game::MOUSE_RIGHT_BUT_UP,val[1]-1,val[2]-1);
                                                    break;
                                                default:
                                                    game->OnMouse(Game::MOUSE_MOVE,val[1]-1,val[2]-1);
                                            }
                                        }
                                    }
                                    break;
                                }
                            }

                            i=k;
                            break;
                        }
                        else
                        {
                            offset++;
                            k++;
                        }
                    }
                    
                    continue;
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

        // get current terminal w,h
        // if changed realloc renderer output
        int nwh[2] = {0,0};
        GetWH(nwh);
        if (nwh[0]!=wh[0] || nwh[1]!=wh[1])
        {
            game->OnSize(nwh[0],nwh[1], 1,1);

            wh[0] = nwh[0];
            wh[1] = nwh[1];

            /*
            if (wh[0]*wh[1]>160*90)
            {
                float scale = sqrtf(160*90 / (wh[0]*wh[1]));
                wh[0] = floor(wh[0] * scale + 0.5f);
                wh[1] = floor(wh[1] * scale + 0.5f);
            }
            */
            buf = (AnsiCell*)realloc(buf,wh[0]*wh[1]*sizeof(AnsiCell));
        }

        // check all pressed chars array, release if timed out

        // check number of chars awaiting in buffer
        // -> read them all, mark as pressed for 0.1 sec / if already pressed prolong

		// NET_TODO:
		// if connection is open:
		// dispatch all queued messages to game
		// 1. flush list
		// 2. reverse order
		// 3. dispatch every message with term->game->OnMessage()

		if (server)
			server->Proc();

		// render
        if (wh[0]>0 && wh[1]>0)
        {
		    game->Render(stamp,buf,wh[0],wh[1]);
            // write to stdout
            Print(buf,wh[0],wh[1],CP437_UTF8);
        }

        frames++;
    }

    if (jsfd>=0)
    {
        GamePadUnmount();
        close(jsfd);
        jsfd = -1;
    }

	// NET_TODO:
	// close network if open
	// ...

    exit:
    uint64_t end = GetTime();

#ifdef USE_GPM
    if (gpm>=0)
    {
        Gpm_Close();
    }
#endif // USE_GPM

    if (terrain)
        DeleteTerrain(terrain);

    if (world)
        DeleteWorld(world);

    if (buf)
        free(buf);

    if (game)
        DeleteGame(game);

    SetScreen(false);

    printf("FPS: %f (%dx%d)\n", frames * 1000000.0 / (end-begin), wh[0], wh[1]);

#else

    printf("Currently -term parameter is unsupported on Windows\n");

#endif

	PurgeItemInstCache();

	FreeSprites();

#ifdef _WIN32
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
