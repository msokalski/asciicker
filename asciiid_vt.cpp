#include <malloc.h>
#include <string.h>
#include <stdio.h>

// just for write(fd)
#include <unistd.h>

#include "asciiid_platform.h"
#include "asciiid_term.h"

struct SGR // 8 bytes
{
    uint8_t bk[3];
    uint8_t fg[3];
    uint16_t fl; 

    enum SGR_FLAGS
    {
        // font decoration modes (4 bits)

        SGR_NORMAL = 0, 
        SGR_BOLD = 1, 
        SGR_FAINT = 2,
        SGR_NORMAL_UNDERLINED = 3,
        SGR_BOLD_UNDERLINED = 4,
        SGR_FAINT_UNDERLINED = 5,
        SGR_NORMAL_DBL_UNDERLINED = 6,
        SGR_BOLD_DBL_UNDERLINED = 7,
        SGR_FAINT_DBL_UNDERLINED = 8,

        // font decoration flags (4 bits)

        SGR_ITALIC = 16,
        SGR_BLINK = 32,
        SGR_INVERSE = 64,
        SGR_CROSSED_OUT = 128,

        // font size and cell part modes (4 bits)

        SGR_DBL_WIDTH_LEFT = 0x0100, //1<<8,
        SGR_DBL_WIDTH_RIGHT = 0x0200,
        SGR_DBL_HEIGHT_UPPER = 0x0300, // non-standard
        SGR_DBL_HEIGHT_LOWER = 0x0400, // non-standard
        SGR_DBL_SIZE_UPPER_LEFT = 0x0500,
        SGR_DBL_SIZE_UPPER_RIGHT = 0x0600,
        SGR_DBL_SIZE_LOWER_LEFT = 0x0700,
        SGR_DBL_SIZE_LOWER_RIGHT = 0x0800,

        // vt cell flags

        SGR_PALETTIZED_FG = 0x1000,
        SGR_PALETTIZED_BG = 0x2000,

        SGR_LAST_BIT_LEFT = 0x4000, // FREEE

        // 1 bit can be saved by
        // merging 2 4bit modes into 1 9x9 (7bit)

        // if still more bits are needed
        // hi part of ch has 11 spare bits!

        SGR_INVISIBLE = 0x8000, // make it not rendered at all
    };
};

struct VT_CELL // 12 bytes
{
    uint32_t ch;
    SGR sgr;
};

struct A3D_VT
{
    A3D_PTY* pty;

    int chr_val;
    int chr_ctx;
    int seq_ctx;    

    int str_len;

    bool UTF8;

    struct
    {
        int x, y;
        int gl, gr;
        SGR sgr;
        bool auto_wrap;
        int single_shift;
    } save_cursor;

    uint16_t x, y;
    uint8_t gl, gr; // 0-3
    uint8_t single_shift; // only 2 or 3 (0-inactive)
    SGR sgr;

    // shouldn't we merge it into bitfields?
    bool auto_wrap;
    bool app_keypad;

    uint8_t G_table[4];

    // parsed not persisted yet
    unsigned char buf[256]; // size will be adjusted to maximize thoughput

    static const int MAX_COLUMNS = 65536;
    static const int MAX_ARCHIVE_LINES = 65536;
    static const int MAX_TEMP_CELLS = 256;

    // no reflows
    // HARD wrap occurs at MAX_COLUMNS
    // soft wrap at current w if auto_wrap is enabled

    struct LINE
    {
        int cells;
        VT_CELL cell[1];
    };

    int scroll; // scroll back amount (used by renderer, does not affect VT)
    int lines;  // min = h max = MAX_ARCHIVE_LINES
    int first_line; // starts incrementing after lines reaches MAX_ARCHIVE_LINES and wraps at MAX_ARCHIVE_LINES
    LINE* line[MAX_ARCHIVE_LINES];

    int w,h;

    VT_CELL temp[MAX_TEMP_CELLS]; // prolongs current line
    int temp_len; // after reaching 256 it must be baked into current line

    int Write(int chr)
    {
        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];

        VT_CELL cell = { (uint32_t)chr, sgr};  
        
        // if sgr has dbl-width font:
        // if auto_wrap ensure x<w-1 ! 
        // we need to place two halfs of glyph, advance x by 2!

        if (x>=w && auto_wrap)
        {
            if (temp_len) // needed if going to change y
            {
                l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*(l->cells+temp_len-1));
                if (!l) // mem-problem
                    return -1;
                line[lidx] = l;
                memcpy(l->cell + l->cells, temp, sizeof(VT_CELL)*temp_len);
                l->cells += temp_len;
                temp_len = 0;
            }
            x=0; 
            y++;

            lidx = (first_line + y) % MAX_ARCHIVE_LINES;
            l = line[lidx];

            // TODO:
            // HANDLE ADDING / REUSING LINES
            // ...
        }
        else
        {
            // HANDLE ADDING LINES
            if (y>=lines)
            {
                // ...
            }

            if (x < l->cells) // in line
            {
                l->cell[x++] = cell; // store char in line
            }
            else
            if (x < l->cells + MAX_TEMP_CELLS) // in temp
            {
                VT_CELL blank = { 0x20, sgr };
                for (int i=l->cells+temp_len; i<x; i++)
                    temp[temp_len++] = blank; // prolong temp with blanks
                temp[x++ - l->cells] = cell; // store char in temp
                temp_len = x - l->cells > temp_len ? x - l->cells : temp_len;   
            }            
            else
            {
                l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*x); // note extra 1!
                if (!l)
                    return -1;
                line[lidx] = l;
                for (int i=0; i<temp_len; i++)
                    l->cell[i+l->cells] = temp[i]; // bake current temp
                VT_CELL blank = { 0x20, sgr };
                for (int i=l->cells+temp_len; i<x; i++)
                    l->cell[i] = blank; // expand with blanks
                temp_len = 0;
                l->cells = x+1; // store char in line
                l->cell[x++] = cell;
            }
        }

        return x;        
    }
};

// private
void a3dSetPtyVT(A3D_PTY* pty, A3D_VT* vt);
A3D_VT* a3dGetPtyVT(A3D_PTY* pty);

static void Scroll(A3D_VT* vt)
{
    if (vt->y >= vt->h)
    {
        // move upper part of screen to archive

        int scroll = vt->y - vt->h + 1;
        if (scroll < vt->h)
            memmove(vt->screen, vt->screen + scroll* vt->w * sizeof(VT_CELL), (vt->h - scroll) * vt->w * sizeof(VT_CELL));
        VT_CELL blank = { 0x20, vt->sgr };
        VT_CELL* clear = vt->screen + (vt->h - scroll) * vt->w;
        for (int i = 0; i < scroll * vt->w; i++)
            clear[i] = blank;

        vt->y = vt->h-1;
    }

    // DO IT ALWAYS after resize
    // x can be as large as MAX_INT is then it will wrap to 0
    if (vt->x<0)
        vt->x += (1<<31);
    if (vt->y<0)
        vt->y=0;
}

static void Reset(A3D_VT* vt)
{
    vt->UTF8 = true;
    vt->str_len = 0;

    vt->G_table[0] = 0;
    vt->G_table[1] = 0;
    vt->G_table[2] = 0;
    vt->G_table[3] = 0;

    vt->gl = 0;
    vt->gr = 0;
    vt->single_shift = 0;
    vt->auto_wrap = false;
    vt->single_shift = 0;

    vt->app_keypad = false;

    vt->chr_val = 0;
    vt->chr_ctx = 0;
    vt->seq_ctx = 0;

    memset(vt->screen,0,sizeof(VT_CELL)*vt->w*vt->h);
}

A3D_VT* a3dCreateVT(int w, int h, const char* path, char* const argv[], char* const envp[])
{
    A3D_PTY* pty = a3dOpenPty(w, h, path, argv, envp);
    if (!pty)
        return 0;

    A3D_VT* vt = (A3D_VT*)malloc(sizeof(A3D_VT));
    vt->screen = (VT_CELL*)malloc(sizeof(VT_CELL)*w*h);
    vt->w = w;
    vt->h = h;

    Reset(vt);

    a3dSetPtyVT(pty,vt);
    vt->pty = pty;

    return vt;
}

void a3dDestroyVT(A3D_VT* vt)
{
    a3dClosePTY(vt->pty);
    if (vt->screen)
        free(vt->screen);
    free(vt);
}

int utf8_parser[8][256]; // 0x001FFFFF char (partial) , 0x07000000 target state (if 0: ready), 0x80000000 error reset flag

int InitParser()
{
    int err = 0x00200000;

    int mode = 0;
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24);
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24);
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24);
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 1; // expecting second byte of 2 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = i&0x3F;
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 2; // expecting second byte of 3 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = ((i&0x3F)<<6) | (0x4<<24);
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 3; // expecting second byte of 4 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = ((i&0x3F)<<12) | (0x5<<24);
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 4; // expecting third byte of 3 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = i&0x3F;
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 5; // expecting third byte of 4 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = ((i&0x3F)<<6) | (0x6<<24);
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 6; // expecting fourth byte of 4 byte UTF8 code
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i | 0x80000000;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = i&0x3F;
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24) | 0x80000000;
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24) | 0x80000000;
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24) | 0x80000000;
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    mode = 7; // recovering from muted error
    for (int i=0; i<0x80; i++)
        utf8_parser[mode][i] = i;
    for (int i=0x80; i<0xC0; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error
    for (int i=0xC0; i<0xE0; i++)
        utf8_parser[mode][i] = ((i&0x1F)<<6) | (0x01<<24);
    for (int i=0xE0; i<0xF0; i++)
        utf8_parser[mode][i] = ((i&0xF)<<12) | (0x02<<24);
    for (int i=0xF0; i<0xF8; i++)
        utf8_parser[mode][i] = ((i&0x7)<<18) | (0x03<<24);
    for (int i=0xF8; i<0x100; i++)
        utf8_parser[mode][i] = 0x87000000; // muted error

    return 1;
}

int parsed = InitParser();

bool a3dProcessVT(A3D_VT* vt)
{
    int len = a3dReadPTY(vt->pty, vt->buf, 256);
    if (len<=0)
        return false;

    int siz = len;
    unsigned char* buf = vt->buf;

    int chr_val = vt->chr_val;
    int chr_ctx = vt->chr_ctx;
    int seq_ctx = vt->seq_ctx;

    bool UTF8 = vt->UTF8;

    int str_len = vt->str_len;

    int i = 0;
    while (i<siz)
    {
        if (UTF8)
        {
            int code = utf8_parser[chr_ctx][buf[i++]];
            int mask = ~(code>>31);
            str_len &= mask; // utf error resets current sequence len
            seq_ctx &= mask; // utf error resets sequence context
            chr_val &= mask; // utf error resets character accum
            chr_val |= code & 0x001FFFFF; // accumulate 
            chr_ctx = (code >> 24) & 0x7; // target context
        }
        else
        {
            chr_ctx = 0;
            chr_val = buf[i++];
        }

        if (chr_ctx)
            continue;

        // we have a char in chr_val!
        int chr = chr_val;
        chr_val = 0;

        if (seq_ctx == 0)
        {
            if (chr == 0x1B)
            {
                seq_ctx = chr;
                continue;
            }

            // C1 (8-Bit) Control Characters
            /*
            if (chr >= 0x80 && chr<0xA0)
            {
                switch (chr)
                {
                    case 0x84: // (IND)
                        // Index
                        // Move the active position one line down, 
                        // to eliminate ambiguity about the meaning of LF. 
                        // Deprecated in 1988 and withdrawn in 1992 from 
                        // ISO/IEC 6429 (1986 and 1991 respectively for ECMA-48). 
                        seq_ctx = 0;
                        continue;
                    case 0x85: // (NEL)
                        // Next Line
                        // Equivalent to CR+LF. Used to mark end-of-line on some IBM mainframes. 
                        seq_ctx = 0;
                        continue;
                    case 0x88: // (HTS)
                        // Character Tabulation Set, Horizontal Tabulation Set
                        // Causes a character tabulation stop to be set at the active position. 
                        seq_ctx = 0;
                        continue;
                    case 0x8D: // (RI)
                        // Reverse Line Feed, Reverse Index
                        seq_ctx = 0;
                        break;
                    case 0x8E: // (SS2)
                        // Single-Shift 2
                        // Next character invokes a graphic character from the G2 graphic sets respectively. 
                        seq_ctx = 0;
                        vt->single_shift = 2;
                        break;
                    case 0x8F: // (SS3)
                        // Single-Shift 3
                        // Next character invokes a graphic character from the G3 graphic sets respectively. 
                        // In systems that conform to ISO/IEC 4873 (ECMA-43), even if a C1 set other than 
                        // the default is used, these two octets may only be used for this purpose.                         break;
                        seq_ctx = 0;
                        vt->single_shift = 3;
                        break;
                    case 0x90: // (DCS)
                        // Device Control String
                        // Followed by a string of printable characters (0x20 through 0x7E) and 
                        // format effectors (0x08 through 0x0D), terminated by ST (0x9C). 
                        seq_ctx = 'P';
                        break;
                    case 0x96: // (SPA)
                        // Start of Protected Area
                        seq_ctx = 0;
                        break;
                    case 0x97: // (EPA)
                        // End of Protected Area
                        seq_ctx = 0;
                        break;
                    case 0x98: // (SOS)
                        // Start of String
                        // Followed by a control string terminated by ST (0x9C) 
                        // that may contain any character except SOS or ST. 
                        // Not part of the first edition of ISO/IEC 6429.[9]
                        seq_ctx = 'X';
                        break;
                    case 0x9A: // (SCI/DECID)
                        // TODO:
                        // return terminal ID
                        seq_ctx = 0;
                        break;
                    case 0x9B: // (CSI)
                        // Control Sequence Introducer
                        // Used to introduce control sequences that take parameters. 
                        seq_ctx = '[';
                        break;
                    case 0x9C: // (ST)
                        // String Terminator
                        seq_ctx = 0;
                        break;
                    case 0x9D: // (OSC)
                        // Operating System Command
                        // Followed by a string of printable characters (0x20 through 0x7E) 
                        // and format effectors (0x08 through 0x0D), terminated by ST (0x9C). 
                        // These three control codes were intended for use to allow in-band 
                        // signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = ']';
                        break;
                    case 0x9E: // (PM)
                        // Privacy Message 
                        // Followed by a string of printable characters (0x20 through 0x7E)
                        // and format effectors (0x08 through 0x0D), terminated by ST (0x9C). 
                        // These three control codes were intended for use to allow in-band 
                        // signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = '^';
                        break;
                    case 0x9F: // (APC)
                        // Application Program Command 
                        // Followed by a string of printable characters (0x20 through 0x7E) 
                        // and format effectors (0x08 through 0x0D), terminated by ST (0x9C). 
                        // These three control codes were intended for use to allow in-band 
                        // signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = '_';
                        break;

                    default:
                        // invalid C1
                        continue;
                }
            }
            */

            // C0 Single-character functions
            switch (chr)
            {
                case 0x05: // ENQ
                    // respond with empty string
                    continue;

                case 0x07: // BEL
                    // beep
                    // ...
                    continue;

                case 0x08: // BS
                {
                    // backspace (move left 1 cell, unwrap if needed and go up)
                    // echo -e "aaa\x08\x08b" -> aba
                    // echo -e "aaa\x09\x08b" -> aaa    b (backspace by 1 cell even if there was a tab!)
                    if (vt->x>0)
                        vt->x--;

                    continue;
                }

                case 0x09: // HT
                {
                    // Horizontal tab
                    // echo -e "aaa\x09b" -> aaa     b 
                    VT_CELL blank = { 0x20, vt->sgr };
                    do
                    {
                        if (vt->auto_wrap && vt->x >= vt->w)
                        {
                            vt->x=0;
                            vt->y++;
                            Scroll(vt);
                        }

                        if (vt->x < vt->w)
                            vt->screen[vt->x + vt->y * vt->w] = blank;

                        vt->x++;
                    } while (vt->x&0x7);
                    
                    continue;
                }

                case 0x0A: // LF
                case 0x0B: // VT
                case 0x0C: // FF
                {
                    // move cursor down, add new line if needed
                    vt->y++;
                    Scroll(vt);

                    continue;
                }

                case 0x0E: // SO
                    // switch to G1
                    vt->gl = 1;
                    continue;

                case 0x0F: // SI
                    // switch to G0
                    vt->gl = 0;
                    continue;

                default:
                    if (chr<0x20)
                    {
                        // invalid C0
                        continue;
                    }
            }

            out_chr:

            if (vt->auto_wrap && vt->x >= vt->w)
            {
                vt->x=0;
                vt->y++;
                Scroll(vt);
            }

            if (vt->x < vt->w)
            {
                VT_CELL cell = { (uint32_t)chr, vt->sgr };
                vt->screen[vt->x + vt->y * vt->w] = cell;
            }

            vt->x++;

            continue;
        }

        switch (seq_ctx)
        {
            case 0x1B:
            {
                switch (chr)
                {
                    case 0x1B: // ESC ESC -> print ESC
                        goto out_chr;

                    case 'D': // (IND)
                    {
                        // Index
                        // Move the active position one line down, to eliminate ambiguity about the meaning of LF. Deprecated in 1988 and withdrawn in 1992 from ISO/IEC 6429 (1986 and 1991 respectively for ECMA-48). 
                        seq_ctx = 0;
                        vt->y++;
                        Scroll(vt);

                        break;
                    }

                    case 'E': // (NEL)
                    {
                        // Next Line
                        // Equivalent to CR+LF. Used to mark end-of-line on some IBM mainframes. 
                        seq_ctx = 0;
                        vt->x=0;
                        vt->y++;
                        Scroll(vt);

                        break;
                    }

                    case 'H': // (HTS)
                    {
                        // Character Tabulation Set, Horizontal Tabulation Set
                        // Causes a character tabulation stop to be set at the active position. 
                        seq_ctx = 0;
                        break;
                    }
                    
                    case 'M': // (RI)
                    {
                        // Reverse Line Feed, Reverse Index
                        seq_ctx = 0;
                        if (vt->y > 0)
                            vt->y--;
                        break;
                    }

                    case 'N': // (SS2)
                    {
                        // Single-Shift 2
                        // Next character invokes a graphic character from the G2 graphic sets respectively. 
                        seq_ctx = 0;
                        vt->single_shift = 2;
                        break;
                    }

                    case 'O': // (SS3)
                    {
                        // Single-Shift 3
                        // Next character invokes a graphic character from the G3 graphic sets respectively. In systems that conform to ISO/IEC 4873 (ECMA-43), even if a C1 set other than the default is used, these two octets may only be used for this purpose.                         break;
                        seq_ctx = 0;
                        vt->single_shift = 3;
                        break;
                    }

                    case 'V': // (SPA)
                    {
                        // Start of Protected Area
                        seq_ctx = 0;
                        break;
                    }

                    case 'W': // (EPA)
                    {
                        // End of Protected Area
                        seq_ctx = 0;
                        break;
                    }

                    case '\\':// (ST)
                    {
                        // String Terminator
                        seq_ctx = 0;
                        break;
                    }

                    case 'P': // (DCS)
                        // Device Control String
                        // Followed by a string of printable characters (0x20 through 0x7E) and format effectors (0x08 through 0x0D), terminated by ST (0x9C). 
                        seq_ctx = 'P';
                        break;
                    case 'X': // (SOS)
                        // Start of String
                        // Followed by a control string terminated by ST (0x9C) that may contain any character except SOS or ST. Not part of the first edition of ISO/IEC 6429.[9]
                        seq_ctx = 'X';
                        break;
                    case '[': // (CSI)
                        // Control Sequence Introducer
                        // Used to introduce control sequences that take parameters. 
                        seq_ctx = '[';
                        break;
                    case ']': // (OSC)
                        // Operating System Command
                        // Followed by a string of printable characters (0x20 through 0x7E) and format effectors (0x08 through 0x0D), terminated by ST (0x9C). These three control codes were intended for use to allow in-band signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = ']';
                        break;
                    case '^': // (PM)
                        // Privacy Message 
                        // Followed by a string of printable characters (0x20 through 0x7E) and format effectors (0x08 through 0x0D), terminated by ST (0x9C). These three control codes were intended for use to allow in-band signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = '^';
                        break;
                    case '_': // (APC)
                        // Application Program Command 
                        // Followed by a string of printable characters (0x20 through 0x7E) and format effectors (0x08 through 0x0D), terminated by ST (0x9C). These three control codes were intended for use to allow in-band signaling of protocol information, but are rarely used for that purpose. 
                        seq_ctx = '_';
                        break;

                    // -----------------

                    case ' ': // F,G,L,M,N - 7/8 bits vt->app modes & ANSI conformance levels
                    case '#': // 3,4,5,6,8 - DEC dbl-width / dbl-height & alignment test
                    case '%': // @ - Select default character set ISO 8859-1, G - Select UTF-8 character set, ISO 2022.
                    case '(': // one or two chars ID of 94-character set for G0 VT100.
                    case ')': // one or two chars ID of 94-character set for G1 VT100.
                    case '*': // one or two chars ID of 94-character set for G2 VT220.
                    case '+': // one or two chars ID of 94-character set for G3 VT220.
                    case '-': // ONE char ID of 96-character set for G1 VT300-VT500.
                    case '.': // ONE char ID of 96-character set for G2 VT300-VT500.
                    case '/': // ONE char ID of 96-character set for G3 VT300-VT500.
                    {
                        seq_ctx = chr;
                        break;
                    }

                    case '6': // Back Index (DECBI), VT420 and up.
                        seq_ctx = 0;
                        break;

                    case '7': // DECSC Save Cursor 
                    /*
                        Saves the following items in the terminal's memory:

                            Cursor position
                            Character attributes set by the SGR command
                            Character sets (G0, G1, G2, or G3) currently in GL and GR
                            Wrap flag (autowrap or no autowrap)
                            State of origin mode (DECOM)
                            Selective erase attribute
                            Any single shift 2 (SS2) or single shift 3 (SS3) functions sent
                    */
                        vt->save_cursor.x = vt->x;
                        vt->save_cursor.y = vt->y;
                        vt->save_cursor.sgr = vt->sgr;
                        vt->save_cursor.gl = vt->gl; 
                        vt->save_cursor.gr = vt->gr;
                        vt->save_cursor.auto_wrap = vt->auto_wrap;
                        vt->save_cursor.single_shift = vt->single_shift;
                        seq_ctx = 0;
                        break;

                    case '8': // DECRC Restore Cursor 
                        vt->x = vt->save_cursor.x;
                        vt->y = vt->save_cursor.y;
                        vt->sgr = vt->save_cursor.sgr;
                        vt->gl = vt->save_cursor.gl; 
                        vt->gr = vt->save_cursor.gr;
                        vt->auto_wrap = vt->save_cursor.auto_wrap;
                        vt->single_shift = vt->save_cursor.single_shift;
                        seq_ctx = 0;
                        break;

                    case '=': // DECPAM Application Keypad ()
                        vt->app_keypad = true;
                        seq_ctx = 0;
                        break; 

                    case '<': // DECPNM Normal Keypad ()    
                        vt->app_keypad = false;
                        seq_ctx = 0;
                        break;

                    case 'F': // Cursor to lower left corner of screen (if enabled by the hpLowerleftBugCompat resource). 
                    {
                        vt->x = 0;
                        vt->y = vt->h-1;
                        seq_ctx = 0;
                        break;
                    }

                    case 'c': // RIS 	Full Reset () 
                        seq_ctx = 0;
                        break;

                    case 'l': // Memory Lock (per HP terminals). Locks memory above the cursor. 
                        seq_ctx = 0;
                        break;

                    case 'm': // Memory Unlock (per HP terminals) 
                        seq_ctx = 0;
                        break;

                    case 'n': // LS2 	Invoke the G2 Character Set () 
                        seq_ctx = 0;
                        vt->gr = 2;
                        break;

                    case 'o': // LS3 	Invoke the G3 Character Set () 
                        seq_ctx = 0;
                        vt->gr = 3;
                        break;

                    case '|': // LS3R 	Invoke the G3 Character Set as GR (). Has no visible effect in xterm. 
                        seq_ctx = 0;
                        break;

                    case '}': // LS2R 	Invoke the G2 Character Set as GR (). Has no visible effect in xterm. 
                        seq_ctx = 0;
                        break;
                        
                    case '~': // LS1R 	Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
                        seq_ctx = 0;
                        break;
                        
                    default:
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;
                }
                break;
            }

            case ' ':
            {
                switch (chr)
                {
                    case 'F':
                        seq_ctx = 0;
                        // set C1 sending mode to 7-bit seqs
                        break;
                    case 'G':
                        seq_ctx = 0;
                        // set C1 sending mode to 8-bit seqs
                        break;
                    case 'L':
                        seq_ctx = 0;
                        // set ANSI conformance level = 1
                        break;
                    case 'M':
                        seq_ctx = 0;
                        // set ANSI conformance level = 2
                        break;
                    case 'N':
                        seq_ctx = 0;
                        // set ANSI conformance level = 3
                        break;

                    default: 
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;
                }
                break;
            }

            case '#':
            {
                switch (chr)
                {
                    case '3':
                        seq_ctx = 0;
                        // set double-height line, upper half
                        // current line is permanently set to it
                        break;

                    case '4':
                        seq_ctx = 0;
                        // set double-height line, lower half
                        // current whole line is permanently set to it
                        break;

                    case '5':
                        seq_ctx = 0;
                        // set single-width line
                        // current whole line is permanently set to it
                        break;

                    case '6':
                        seq_ctx = 0;
                        // set double-width line
                        // current whole line is permanently set to it
                        break;

                    case '8':
                        seq_ctx = 0;
                        // screen align test
                        // fills screen with EEEE and sets different modes to lines 
                        break;

                    default: 
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;
                }
                break;
            }

            case '%':
            {
                switch (chr)
                {
                    case '@':
                        seq_ctx = 0;
                        UTF8 = false; // Select default character set.  That is ISO 8859-1 (ISO 2022).
                        break;
                    case 'G':
                        seq_ctx = 0;
                        UTF8 = true; // Select UTF-8 character set, ISO 2022.
                        break;

                    default: 
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }
                
                break;
            }

            case '(': // one or two chars ID of 94-character set for G0 VT100.
            case ')': // one or two chars ID of 94-character set for G1 VT100.
            case '*': // one or two chars ID of 94-character set for G2 VT220.
            case '+': // one or two chars ID of 94-character set for G3 VT220.            
            {
                int g_index = seq_ctx - '(';

                switch (chr)
                {
                    case 'B':
                        seq_ctx = 0;
                        // set G0/G1 = USASCII
                        vt->G_table[g_index] = 0;
                        break;
                    case 'A':
                        seq_ctx = 0;
                        // set G0/G1 = United Kingdom (UK)
                        vt->G_table[g_index] = 1;
                        break;
                    case '4':
                        seq_ctx = 0;
                        // set G0/G1 = Dutch
                        vt->G_table[g_index] = 2;
                        break;
                    case 'C': case '5':
                        seq_ctx = 0;
                        // set G0/G1 = Finnish
                        vt->G_table[g_index] = 3;
                        break;
                    case 'R': case 'f':
                        seq_ctx = 0;
                        // set G0/G1 = French
                        vt->G_table[g_index] = 4;
                        break;
                    case 'Q': case '9':
                        seq_ctx = 0;
                        // set G0/G1 = French Canadian
                        vt->G_table[g_index] = 5;
                        break;
                    case 'K':
                        seq_ctx = 0;
                        // set G0/G1 = German
                        vt->G_table[g_index] = 6;
                        break;
                    case 'Y':
                        seq_ctx = 0;
                        // set G0/G1 = Italian
                        vt->G_table[g_index] = 7;
                        break;
                    case '`': case 'E': case '6':
                        seq_ctx = 0;
                        // set G0/G1 = Norwegian/Danish
                        vt->G_table[g_index] = 8;
                        break;
                    case 'Z':
                        seq_ctx = 0;
                        // set G0/G1 = Spanish
                        vt->G_table[g_index] = 9;
                        break;
                    case 'H': case '7': 
                        seq_ctx = 0;
                        // set G0/G1 = Swedish
                        vt->G_table[g_index] = 10;
                        break;
                    case '=':
                        seq_ctx = 0;
                        // set G0/G1 = Swiss
                        vt->G_table[g_index] = 11;
                        break;
                    case '0':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Special Character and Line Drawing Set
                        vt->G_table[g_index] = 12;
                        break;
                    case '<':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Supplemental
                        vt->G_table[g_index] = 13;
                        break;
                    case '>':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Technical
                        vt->G_table[g_index] = 14;
                        break;

                    case '\"': // require '>' or '?' or '4'
                        seq_ctx |= '\"' << 8;
                        break;
                    case '%': // require '=' or '6' or '2' or '5' or '0' or '3'
                        seq_ctx |= '%' << 8;
                        break;
                    case '&': // require '4' or '5'
                        seq_ctx |= '&' << 8;
                        break;

                    default: 
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }
                break;
            }

            case '-': // ONE char ID of 96-character set for G1 VT300-VT500.
            case '.': // ONE char ID of 96-character set for G2 VT300-VT500.
            case '/': // ONE char ID of 96-character set for G3 VT300-VT500.
            {
                int g_index = seq_ctx + 1 - '-';

                switch (chr)
                {
                    case 'A': // ISO Latin-1 Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 26;
                        break;
                    case 'F': // ISO Greek Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 27;
                        break;
                    case 'H': // ISO Hebrew Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 28;
                        break;
                    case 'L': // ISO Latin-Cyrillic
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 29;
                        break;
                    case 'M': // ISO Latin-5 Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 30;
                        break;

                    default: 
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }

                break;
            }            

            case  '(' | ('\"' << 8):
            case  ')' | ('\"' << 8):
            case  '*' | ('\"' << 8):
            case  '+' | ('\"' << 8):
            {
                int g_index = (seq_ctx & 0xFF) - '(';

                switch (chr)
                {
                    case '>': // Greek
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 15;
                        break;
                    case '?': // DEC Greek
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 16;
                        break;
                    case '4': // DEC Hebrew
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 17;
                        break;

                    default:
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }

                break;
            }

            case  '(' | ('%' << 8):
            case  ')' | ('%' << 8):
            case  '*' | ('%' << 8):
            case  '+' | ('%' << 8):
            {
                int g_index = (seq_ctx & 0xFF) - '(';

                switch (chr)
                {
                    case '=': // Hebrew
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 18;
                        break;
                    case '6': // Portuguese
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 19;
                        break;
                    case '2': // Turkish
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 20;
                        break;
                    case '5': // DEC Supplemental Graphics
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 21;
                        break;
                    case '0': // DEC Turkish
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 22;
                        break;
                    case '3': // SCS NRCS
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 23;
                        break;

                    default:
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }

                break;
            }

            case  '(' | ('&' << 8):
            case  ')' | ('&' << 8):
            case  '*' | ('&' << 8):
            case  '+' | ('&' << 8):
            {
                int g_index = (seq_ctx & 0xFF) - '(';

                switch (chr)
                {
                    case '4': // DEC Cyrillic
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 24;
                        break;
                    case '5': // DEC Russian
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 25;
                        break;

                    default:
                        // error seq, dump it as regular chars!
                        // ...
                        seq_ctx = 0;                
                }

                break;
            }

            case '[': // (CSI)
            {
                if (chr==';' || chr>='0' && chr<='9')
                    str_len++;
                else
                {
                    // consider anything else terminates
                    // positively or negatively (default)
                    switch (chr)
                    {
                        default:
                            str_len = 0;
                            seq_ctx = 0;
                    }
                }
                break;
            }

            case 'P': // (DCS) -> Ps;Ps|PtST or $ qPtST or Ps$tPtST or +pPtST or +qPtST
            case 'X': // (SOS) not in xterm spec
            case ']': // (OSC) -> Ps;PtBEL or Ps;PtST - hmm BEL!
            case '^': // (PM)  -> skipped by xterm
            case '_': // (APC) -> skipped by xterm
            {
                if (chr == 0x9C || chr == 0x07 && seq_ctx == ']')
                {
                    // do something with string from our secret place!
                    // ...
                    str_len = 0;
                    seq_ctx = 0; // ok
                }
                else
                if (chr >= 0x20 && chr<=0x7E || chr>=0x08 && chr<= 0x0D || seq_ctx == 'X' && chr!=0x98)
                {
                    // looks like 0x08 (BEL) can also terminate OSC
                    // keep in secret place (null)
                    str_len++;
                }
                else
                {
                    // invalid char!!!
                    str_len = 0;
                    seq_ctx = 0;
                }

                break;
            }

            default:
                // error seq, dump it as regular chars!
                // ...
                str_len = 0;
                seq_ctx = 0;
        }
    }

    // persist state till we have more data to process
    vt->UTF8 = UTF8;
    vt->chr_val = chr_val;
    vt->chr_ctx = chr_ctx;
    vt->seq_ctx = seq_ctx;
    vt->str_len = str_len;

    return true;
}

int a3dWriteVT(A3D_VT* vt, const void* buf, size_t size)
{
    return a3dWritePTY(vt->pty, buf, size);
}
