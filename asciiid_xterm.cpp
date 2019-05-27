#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include "asciiid_term.h"

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

/*
struct UTF8
{
    UTF8(uint16_t x)
    {
        if (x<0x0080)
        {
            len = 1;
            data[0] = x & 0x7F;
            data[1] = 0;
            data[2] = 0;
        }
        else
        if (x<0x0800)
        {
            len = 2;
            data[0] = 0xC0 | ( ( x >> 6 ) & 0x1F ); 
            data[1] = 0x80 | ( x & 0x3F );
            data[2] = 0;
        }
        else
        {
            len = 3;
            data[0] = 0xE0 | ( ( x >> 12 ) & 0x0F );
            data[1] = 0x80 | ( ( x >> 6 ) & 0x3F );
            data[2] = 0x80 | ( x & 0x3F );
        }
    }

    int Print(char* buf) const
    {
        for (int i=0; i<len; i++)
            buf[i] = data[i];
        return len;
    }

    unsigned char len;
    char data[3];
};

static const UTF8 cp437_utf8[256] =
{
    UTF8(0x0020), UTF8(0x263A), UTF8(0x263B), UTF8(0x2665), UTF8(0x2666), UTF8(0x2663), UTF8(0x2660), UTF8(0x2022), 
    UTF8(0x25D8), UTF8(0x25CB), UTF8(0x25D9), UTF8(0x2642), UTF8(0x2640), UTF8(0x266A), UTF8(0x266B), UTF8(0x263C),
    UTF8(0x25BA), UTF8(0x25C4), UTF8(0x2195), UTF8(0x203C), UTF8(0x00B6), UTF8(0x00A7), UTF8(0x25AC), UTF8(0x21A8), 
    UTF8(0x2191), UTF8(0x2193), UTF8(0x2192), UTF8(0x2190), UTF8(0x221F), UTF8(0x2194), UTF8(0x25B2), UTF8(0x25BC),
    UTF8(0x0020), UTF8(0x0021), UTF8(0x0022), UTF8(0x0023), UTF8(0x0024), UTF8(0x0025), UTF8(0x0026), UTF8(0x0027),
    UTF8(0x0028), UTF8(0x0029), UTF8(0x002A), UTF8(0x002B), UTF8(0x002C), UTF8(0x002D), UTF8(0x002E), UTF8(0x002F),
    UTF8(0x0030), UTF8(0x0031), UTF8(0x0032), UTF8(0x0033), UTF8(0x0034), UTF8(0x0035), UTF8(0x0036), UTF8(0x0037),
    UTF8(0x0038), UTF8(0x0039), UTF8(0x003A), UTF8(0x003B), UTF8(0x003C), UTF8(0x003D), UTF8(0x003E), UTF8(0x003F),
    UTF8(0x0040), UTF8(0x0041), UTF8(0x0042), UTF8(0x0043), UTF8(0x0044), UTF8(0x0045), UTF8(0x0046), UTF8(0x0047),
    UTF8(0x0048), UTF8(0x0049), UTF8(0x004A), UTF8(0x004B), UTF8(0x004C), UTF8(0x004D), UTF8(0x004E), UTF8(0x004F),
    UTF8(0x0050), UTF8(0x0051), UTF8(0x0052), UTF8(0x0053), UTF8(0x0054), UTF8(0x0055), UTF8(0x0056), UTF8(0x0057),
    UTF8(0x0058), UTF8(0x0059), UTF8(0x005A), UTF8(0x005B), UTF8(0x005C), UTF8(0x005D), UTF8(0x005E), UTF8(0x005F),
    UTF8(0x0060), UTF8(0x0061), UTF8(0x0062), UTF8(0x0063), UTF8(0x0064), UTF8(0x0065), UTF8(0x0066), UTF8(0x0067),
    UTF8(0x0068), UTF8(0x0069), UTF8(0x006A), UTF8(0x006B), UTF8(0x006C), UTF8(0x006D), UTF8(0x006E), UTF8(0x006F),
    UTF8(0x0070), UTF8(0x0071), UTF8(0x0072), UTF8(0x0073), UTF8(0x0074), UTF8(0x0075), UTF8(0x0076), UTF8(0x0077),
    UTF8(0x0078), UTF8(0x0079), UTF8(0x007A), UTF8(0x007B), UTF8(0x007C), UTF8(0x007D), UTF8(0x007E), UTF8(0x2302),
    UTF8(0x00C7), UTF8(0x00FC), UTF8(0x00E9), UTF8(0x00E2), UTF8(0x00E4), UTF8(0x00E0), UTF8(0x00E5), UTF8(0x00E7), 
    UTF8(0x00EA), UTF8(0x00EB), UTF8(0x00E8), UTF8(0x00EF), UTF8(0x00EE), UTF8(0x00EC), UTF8(0x00C4), UTF8(0x00C5), 
    UTF8(0x00C9), UTF8(0x00E6), UTF8(0x00C6), UTF8(0x00F4), UTF8(0x00F6), UTF8(0x00F2), UTF8(0x00FB), UTF8(0x00F9), 
    UTF8(0x00FF), UTF8(0x00D6), UTF8(0x00DC), UTF8(0x00A2), UTF8(0x00A3), UTF8(0x00A5), UTF8(0x20A7), UTF8(0x0192), 
    UTF8(0x00E1), UTF8(0x00ED), UTF8(0x00F3), UTF8(0x00FA), UTF8(0x00F1), UTF8(0x00D1), UTF8(0x00AA), UTF8(0x00BA), 
    UTF8(0x00BF), UTF8(0x2310), UTF8(0x00AC), UTF8(0x00BD), UTF8(0x00BC), UTF8(0x00A1), UTF8(0x00AB), UTF8(0x00BB), 
    UTF8(0x2591), UTF8(0x2592), UTF8(0x2593), UTF8(0x2502), UTF8(0x2524), UTF8(0x2561), UTF8(0x2562), UTF8(0x2556), 
    UTF8(0x2555), UTF8(0x2563), UTF8(0x2551), UTF8(0x2557), UTF8(0x255D), UTF8(0x255C), UTF8(0x255B), UTF8(0x2510), 
    UTF8(0x2514), UTF8(0x2534), UTF8(0x252C), UTF8(0x251C), UTF8(0x2500), UTF8(0x253C), UTF8(0x255E), UTF8(0x255F), 
    UTF8(0x255A), UTF8(0x2554), UTF8(0x2569), UTF8(0x2566), UTF8(0x2560), UTF8(0x2550), UTF8(0x256C), UTF8(0x2567), 
    UTF8(0x2568), UTF8(0x2564), UTF8(0x2565), UTF8(0x2559), UTF8(0x2558), UTF8(0x2552), UTF8(0x2553), UTF8(0x256B), 
    UTF8(0x256A), UTF8(0x2518), UTF8(0x250C), UTF8(0x2588), UTF8(0x2584), UTF8(0x258C), UTF8(0x2590), UTF8(0x2580), 
    UTF8(0x03B1), UTF8(0x00DF), UTF8(0x0393), UTF8(0x03C0), UTF8(0x03A3), UTF8(0x03C3), UTF8(0x00B5), UTF8(0x03C4), 
    UTF8(0x03A6), UTF8(0x0398), UTF8(0x03A9), UTF8(0x03B4), UTF8(0x221E), UTF8(0x03C6), UTF8(0x03B5), UTF8(0x2229), 
    UTF8(0x2261), UTF8(0x00B1), UTF8(0x2265), UTF8(0x2264), UTF8(0x2320), UTF8(0x2321), UTF8(0x00F7), UTF8(0x2248), 
    UTF8(0x00B0), UTF8(0x2219), UTF8(0x00B7), UTF8(0x221A), UTF8(0x207F), UTF8(0x00B2), UTF8(0x25A0), UTF8(0x0020)
};
*/


int PrintScreen(const TermScreen* screen, const uint8_t ipal[1<<24])
{
    int ret = 0;

    // read w,h

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);    

    int x=0, y=0;
    int w = ws.ws_col;
    int h = ws.ws_row;

    int x2 = x+w;
    x2 = x2<screen->width ? x2 : screen->width;

    int y2 = y+h;
    y2 = y2<screen->height ? y2 : screen->height;

    int x1 = x>=0 ? x : 0;
    int y1 = y>=0 ? y : 0;

    const int max_cols = 512;
    const int max_line_bytes = max_cols * (22+3) + 11 + 1; 
    const int max_buffer_lines = 2;
    const int header_size = 28;
    const int buffer_size = header_size + max_buffer_lines*max_line_bytes;
    const int buffer_flush_size = buffer_size - max_line_bytes;
    
    char buf[buffer_size];
    int siz=header_size;
    
    // disable wrap, set fg=0, bg=0, goto 1,1
    memcpy(buf, "\x1B[38;5;0m\x1B[48;5;0m\x1B[7l\x1B[1;1H", header_size);

    uint8_t prev_fg = 0;
    uint8_t prev_bg = 0;

    for (int y=y1; y<y2; y++)
    {
        const TermCell* ptr = screen->cell + x1 + y*screen->width;
        for (int x=0; x<x2; ptr++,x++)
        {
            uint8_t fg = ipal[ptr->fg[0] + 256*ptr->fg[1] + 65536*ptr->fg[2]];
            uint8_t bg = ipal[ptr->bg[0] + 256*ptr->bg[1] + 65536*ptr->bg[2]];

            if (fg!=prev_fg)
            {
                prev_fg = fg;
                // ESC[ 38;5;<n> m Select foreground color
                memcpy(buf+siz,"\x1B[38;5;",7);
                if (fg<10)
                {
                    buf[siz+7] = fg+'0';
                    buf[siz+8] = 'm';
                    siz+=9;
                }
                else
                if (fg<100)
                {
                    buf[siz+7] = '0'+fg/10;
                    buf[siz+8] = '0'+fg%10;
                    buf[siz+9] = 'm';
                    siz+=10;
                }
                else
                {
                    if (fg >= 200)
                    {
                        buf[siz+7] = '2'; 
                        fg -= 200;
                    }
                    else
                    {
                        buf[siz+7] = '1'; 
                        fg -= 100;
                    }
                    buf[siz+8] = '0'+fg/10;
                    buf[siz+9] = '0'+fg%10;
                    buf[siz+10] = 'm';
                    siz+=11;
                }
            }

            if (bg!=prev_bg)
            {
                prev_bg = bg;
                //ESC[ 48;5;<n> m Select background color
                memcpy(buf+siz,"\x1B[48;5;",7);
                if (bg<10)
                {
                    buf[siz+7] = bg+'0';
                    buf[siz+8] = 'm';
                    siz+=9;
                }
                else
                if (bg<100)
                {
                    buf[siz+7] = '0'+bg/10;
                    buf[siz+8] = '0'+bg%10;
                    buf[siz+9] = 'm';
                    siz+=10;
                }
                else
                {
                    if (bg >= 200)
                    {
                        buf[siz+7] = '2'; 
                        bg -= 200;
                    }
                    else
                    {
                        buf[siz+7] = '1'; 
                        bg -= 100;
                    }
                    buf[siz+8] = '0'+bg/10;
                    buf[siz+9] = '0'+bg%10;
                    buf[siz+10] = 'm';
                    siz+=11;
                }
            }

            // siz += cp437_utf8[ptr->ch].Print(buf+siz);
            siz += CP437[ptr->ch](buf+siz);

            assert(siz<=buffer_size);
        }

        if (y<y2-1)
        {
            if (prev_bg!=0)
            {
                prev_bg=0;
                memcpy(buf+siz,"\x1B[48;5;0m",9);
                siz+=9;
            }
            buf[siz++] = '\n';
            if (siz >= buffer_flush_size)
            {
                ret += write(STDOUT_FILENO,buf,siz);
                siz=0;
            }
        }
    }

    if (siz)
    {
        ret += write(STDOUT_FILENO,buf,siz);
        siz=0;
    }

    return ret;
}
