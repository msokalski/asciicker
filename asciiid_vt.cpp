#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// just for write(fd)
#include <unistd.h>

// just to inform dump about term size
#include <sys/ioctl.h>

#include "asciiid_platform.h"
#include "asciiid_term.h"


#define DONE() done(__LINE__)
void done(int line)
{
    int bbb=0;
}

#define TODO() todo(__LINE__)
void todo(int line)
{
    int aaa=0;
}

#define WARN() warn(__LINE__)
void warn(int line)
{
    int aaa=0;
}

static bool a3dProcessVT(A3D_VT* vt);


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

    A3D_THREAD* pty_reader;
    A3D_MUTEX* mutex; // prevents renderer and vt processing threads to clash

    int dump_dirty; // received something after last dump

    volatile int closing;

    int chr_val;
    int chr_ctx;
    int seq_ctx;    

    int str_len; // incoming sequence length

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

    int scroll_top;
    int scroll_bottom;

    // shouldn't we merge it into bitfields?
    bool auto_wrap;
    bool app_keypad;
    bool app_cursors;
    bool ins_mode; // CSI 4 h / CSI 4 l

    uint8_t G_table[4];

    // parsed not persisted yet
    unsigned char buf[256]; // size will be adjusted to maximize thoughput

    static const int MAX_COLUMNS = 65536;
    static const int MAX_ARCHIVE_LINES = 65536;
    static const int MAX_TEMP_CELLS = 256;
    static const int MAX_STR_LEN = 256;

    // incomming long sequence
    char str[MAX_STR_LEN];

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
    int temp_ins; // only in insert mode, x position of temp

    static void* PtyRead(void* pvt)
    {
        A3D_VT* vt = (A3D_VT*)pvt;
        while (!vt->closing)
        {
            if (!a3dProcessVT(vt))
                break;

            int a=0;
        }

        void* ret = 0;
        return ret;
    }

    inline bool Resize(int cols, int rows)
    {
        if (cols<2)
            cols=2;
        if (rows<2)
            rows=2;
        if (rows>MAX_ARCHIVE_LINES)
            rows = MAX_ARCHIVE_LINES;

        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];         

        if (lines<=rows)
        {
            y += first_line; 
            first_line = 0;
        }
        else
        {
            y += rows - h;
            first_line = lines - rows;
        }

        w = cols;
        h = rows;

        if (y<0 || y>=h)
        {
            Flush();

            if (y<0)
                y=0;
            else
                y=h-1;
        }

        return true;        
    }

    inline int GetCurLineLen()
    {
        if (y>=lines)
            return 0;
        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];
        if (!l)
            return 0;
        
        return l->cells + temp_len;        
    }

    inline bool GotoXY(int col, int row)
    {
        if (row<0)
            row = 0;
        if (row>=h)
            row = h-1;
        if (col<0)
            col=0;

        if (row!=y)
            Flush();

        x = col;
        y = row;

        return true;
    }

    inline bool EraseChars(int num)
    {
        Flush();
        
        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];        
        if (!l)
            return true;

        int end = x+num < l->cells ? x+num : l->cells;
        VT_CELL blank = { 0x20, sgr };
        for (int i=x; i<end; i++)
            l->cell[i] = blank;
        return true;
    }

    inline bool EraseRow(int mode)
    {
        Flush();

        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];        
        if (!l)
            return true;

        if (mode==0)
        {
            //return true;
            if (x>=l->cells)
                return true;
            if (x==0)
            {
                free(l);
                line[lidx] = 0;
                return true;
            }
            l = (LINE*)realloc(l,sizeof(LINE) + sizeof(VT_CELL) * (x-1));
            if (!l)
                return false;
            l->cells = x;
            line[lidx] = l;
            return true;
        }

        if (mode==1)
        {
            //return true;
            if (x==0)
                return true;
            if (x>=l->cells)
            {
                free(l);
                line[lidx] = 0;
                return true;
            }

            VT_CELL blank = { 0x20, sgr };
            for (int i=0; i<x; i++)
                l->cell[i] = blank;

            return true;
        }

        if (mode==2)
        {
            free(l);
            line[lidx] = 0;
            return true;
        }

        return true;
    }

    inline bool EraseRows(int from, int to)
    {
        if (from<0)
            from=0;
        if (to>h)
            to=h;
        
        if (y>=from && y<to)
            Flush();

        for (int i=from; i<to; i++)
        {
            int lidx = (first_line + i) % MAX_ARCHIVE_LINES;
            LINE* l = line[lidx];
            if (l)
            {
                free(l);
                line[lidx] = 0;
            }
        }

        return true;
    }

    inline bool DeleteChars(int num)
    {
        if (y<scroll_top || y >= lines || y>=scroll_bottom)
            return true;

        Flush();

        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];
        if (!l || l->cells + temp_len < x)
            return true;

        int cells = l->cells;

        int end = x+num;
        if (end >= cells) // trim right
        {
            if (x==0)
            {
                free(l);
                line[lidx] = 0;
                return true;
            }
            l = (LINE*)realloc(l,sizeof(LINE) + sizeof(VT_CELL) * (x-1));
            if (!l)
                return false;
            l->cells = x;
            line[lidx] = l;
            return true;
        }

        cells-=num;
        if (cells<0)
            cells=0;

        for (int i=x; i<cells; i++)
            l->cell[i] = l->cell[i+num];

        l = (LINE*)realloc(l,sizeof(LINE) + sizeof(VT_CELL) * (cells-1));
        if (!l)
            return false;
        l->cells = cells;
        line[lidx] = l;
        return true;
    }

    inline bool Scroll(int num)
    {
        bool ret;
        int at = y;
        y = scroll_top;
        if (num>0)
            ret = DeleteLines(num);
        else
        if (num<0)
            ret = InsertLines(-num);

        y = at;
        return ret;        
    }

    inline bool DeleteLines(int num)
    {
        if (y >= lines || y>=scroll_bottom || y<scroll_top)
            return true;

        Flush();

        int end = y+num < scroll_bottom ? y+num : scroll_bottom;

        // free deleted lines
        for (int i=y; i<end; i++)
        {
            int lidx = (first_line + i) % MAX_ARCHIVE_LINES;
            LINE* l = line[lidx];
            if (l)
            {
                free(l);
                line[lidx] = 0;
            }            
        }

        // scroll up all survivors, null their origin
        for (int i=end; i<scroll_bottom; i++)
        {
            int from = (first_line + i) % MAX_ARCHIVE_LINES;
            int to = (first_line + i-num) % MAX_ARCHIVE_LINES;

            line[to] = line[from];
            line[from] = 0;
        }
    }

    inline bool InsertLines(int num)
    {
        if (y >= lines || y >= scroll_bottom || y<scroll_top)
            return true; // nutting to scroll

        Flush();

        int kill_from = scroll_bottom - num > y ? scroll_bottom - num : y;

        // kill lines scrolled out of the bottom
        for (int i=kill_from; i<scroll_bottom; i++)
        {
            int lidx = (first_line + i) % MAX_ARCHIVE_LINES;
            LINE* l = line[lidx];
            if (l)
            {
                free(l);
                line[lidx] = 0;
            }
        }

        // scroll survivors
        for (int i=scroll_bottom-1; i>=y+num; i--)
        {
            int from = (first_line + i - num) % MAX_ARCHIVE_LINES;
            int to = (first_line + i) % MAX_ARCHIVE_LINES;
            line[to] = line[from];
            line[from] = 0;           
        }

        // adjust num of lines if was between top and bottom of scroll range
        if (lines >=scroll_top && lines < scroll_bottom)
           lines = lines+num < scroll_bottom ? lines+num : scroll_bottom;

        return true;
    }

    inline bool Flush()
    {
        if (temp_len) // apply temp to line, needed if going to change y
        {
            int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
            LINE* l = line[lidx];
            int cells = l ? l->cells : 0;

            if (ins_mode)
            {
                LINE* L = (LINE*)malloc(sizeof(LINE) + sizeof(VT_CELL)*(cells + temp_len-1));
                L->cells = cells + temp_len;
                if (temp_ins > 0)
                    memcpy(L->cell, l->cell, sizeof(VT_CELL)*temp_ins);
                memcpy(L->cell + temp_ins, temp, sizeof(VT_CELL)*temp_len);
                if (temp_ins < cells)
                    memcpy(L->cell + temp_ins + temp_len, l->cell + temp_ins, sizeof(VT_CELL)*(cells-temp_ins));
                free(l);
                line[lidx] = L;
            }
            else
            {
                l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*(cells + temp_len-1));
                if (!l) // mem-problem
                    return false;
                line[lidx] = l;
                memcpy(l->cell + cells, temp, sizeof(VT_CELL)*temp_len);
            }
            
            l->cells = cells + temp_len;
            temp_len = 0;
        }
    }

    inline bool InsertBlanks(int num)
    {
        if (!num)
            return true;

        Flush();
        
        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];
        int cells = l ? l->cells : 0;

        l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL) * (cells+num-1));
        if (!l)
            return false;
        l->cells = cells+num;
        line[lidx] = l;

        for (int i=cells-1; i>=x; i--)
            l->cell[i+num] = l->cell[i];
        
        VT_CELL blank = { 0x20, sgr };

        for (int i=0; i<num; i++)
            l->cell[i+x] = blank;
            
        if (GetCurLineLen() > w)
            WARN();
    }

    inline bool Write(int chr)
    {
        int lidx = (first_line + y) % MAX_ARCHIVE_LINES;
        LINE* l = line[lidx];

        VT_CELL cell = {(uint32_t)chr, sgr};  
        
        if (x>=w && auto_wrap)
        {
            int cells = l ? l->cells : 0;
                  
            Flush();
            x=0; 

            if (y==h-1)
            {
                first_line++;
                lidx = (first_line + y) % MAX_ARCHIVE_LINES;
                l = line[lidx];
                if (lines<MAX_ARCHIVE_LINES)
                    lines++;
                else
                if (l)
                {
                    free(l);
                    line[lidx]=0;
                }
            }
            else
            {
                y++;
                lidx = (first_line + y) % MAX_ARCHIVE_LINES;
                if (y>=lines)
                    lines = y+1;
            }
        }
        else
        if (y>=lines)
            lines = y+1;

        int cells = l ? l->cells : 0;

        if (ins_mode)
        {
            if (temp_len)
            {
                if (x != temp_ins + temp_len || temp_len == MAX_TEMP_CELLS)
                {
                    Flush();
                    if (x>cells)
                    {
                        // insert blanks to avoid gaps on flush
                        l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*(x-1));
                        l->cells = x;
                        line[lidx] = l;

                        VT_CELL blank = { 0x20, sgr };
                        for (int i=cells; i<x; i++)
                            l->cell[i] = blank;
                    }
                    temp_ins = x; // set new temp insertion pos
                }
                // otherwise continue with temp
            }
            else
            {
                if (x>cells)
                {
                    // insert blanks to avoid gaps on flush
                    l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*(x-1));
                    l->cells = x;
                    line[lidx] = l;

                    VT_CELL blank = { 0x20, sgr };
                    for (int i=cells; i<x; i++)
                        l->cell[i] = blank;
                }
                temp_ins = x; // set new temp pos
            }

            temp[x++ - temp_ins] = cell;
            temp_len++;

            // kill scroll out ?
            if (temp_ins < cells && cells + temp_len > w)
                l->cells = w - temp_len;

            if (GetCurLineLen())
                WARN();
            return true;
        }

        if (x < cells) // in line
        {
            l->cell[x++] = cell; // store char in line
        }
        else
        if (x < cells + MAX_TEMP_CELLS) // in temp
        {
            VT_CELL blank = { 0x20, sgr };
            for (int i=cells+temp_len; i<x; i++)
                temp[temp_len++] = blank; // prolong temp with blanks
            temp[x++ - cells] = cell; // store char in temp
            temp_len = x - cells > temp_len ? x - cells : temp_len;   
        }            
        else
        {
            l = (LINE*)realloc(l, sizeof(LINE) + sizeof(VT_CELL)*x); // note extra 1!
            if (!l)
                return false;
            line[lidx] = l;
            for (int i=0; i<temp_len; i++)
                l->cell[i+cells] = temp[i]; // bake current temp
            VT_CELL blank = { 0x20, sgr };
            for (int i=cells+temp_len; i<x; i++)
                l->cell[i] = blank; // expand with blanks
            temp_len = 0;
            l->cells = x+1; // store char in line
            l->cell[x++] = cell;
        }

        if (GetCurLineLen() > w)
            WARN();

        return true;
    }
};


// private
void a3dSetPtyVT(A3D_PTY* pty, A3D_VT* vt);
A3D_VT* a3dGetPtyVT(A3D_PTY* pty);

static void Reset(A3D_VT* vt)
{
    vt->UTF8 = true;
    vt->str_len = 0;
    vt->dump_dirty |= 2;

    vt->G_table[0] = 0;
    vt->G_table[1] = 0;
    vt->G_table[2] = 0;
    vt->G_table[3] = 0;

    vt->gl = 0;
    vt->gr = 0;
    vt->single_shift = 0;
    vt->auto_wrap = true;
    vt->ins_mode = false;

    vt->scroll_top = 0;
    vt->scroll_bottom = vt->h;

    vt->app_keypad = false;
    vt->app_cursors = false;

    vt->chr_val = 0;
    vt->chr_ctx = 0;
    vt->seq_ctx = 0;

    for (int i=0; i<vt->lines; i++)
    {
        if (vt->line[i])
        {
            free(vt->line[i]);
            vt->line[i]=0;
        }
    }

    vt->temp_len = 0;
    vt->temp_ins = 0;
    vt->lines = 0;
    vt->x = 0;
    vt->y = 0;
    vt->scroll = 0;
    vt->sgr.bk[0] = 0;
    vt->sgr.bk[1] = 0;
    vt->sgr.bk[2] = 0;
    vt->sgr.fg[0] = 192;
    vt->sgr.fg[1] = 192;
    vt->sgr.fg[2] = 192;
    vt->sgr.fl = 0;
}

A3D_VT* a3dCreateVT(int w, int h, const char* path, char* const argv[], char* const envp[])
{
    A3D_PTY* pty = a3dOpenPty(w, h, path, argv, envp);
    if (!pty)
        return 0;

    A3D_VT* vt = (A3D_VT*)malloc(sizeof(A3D_VT));
    memset(vt->line,0,sizeof(A3D_VT::LINE*)*A3D_VT::MAX_ARCHIVE_LINES);
    vt->w = w;
    vt->h = h;
    vt->lines = 0;

    Reset(vt);

    a3dSetPtyVT(pty,vt);
    vt->pty = pty;

    vt->mutex = a3dCreateMutex();
    vt->pty_reader = a3dCreateThread(A3D_VT::PtyRead,vt);

    vt->dump_dirty = 2; // resized or reset

    return vt;
}

void a3dDestroyVT(A3D_VT* vt)
{
    vt->closing = 1;
    a3dClosePTY(vt->pty); // should force pty_reader to exit

    a3dWaitForThread(vt->pty_reader);
    a3dDeleteMutex(vt->mutex);

    for (int i=0; i<vt->lines; i++)
        if (vt->line[i])
            free(vt->line[i]);
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

static bool a3dProcessVT(A3D_VT* vt)
{
    int len = a3dReadPTY(vt->pty, vt->buf, 256);
    if (len<=0)
        return false;

    a3dMutexLock(vt->mutex);

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
        a3dMutexUnlock(vt->mutex);
        a3dDumpVT(vt);
        usleep(1000);
        a3dMutexLock(vt->mutex);

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

        if (vt->y>=1 && vt->y<=5)
        {
            int kot=0;
        }

        // we have a char in chr_val!
        int chr = chr_val;
        chr_val = 0;

        if (chr==0x2510)
        {
            int aaa=0;
        }

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
                        TODO:
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
                    if (vt->x>0)
                        vt->x--;
                    DONE();
                    continue;
                }

                case 0x09: // HT
                {
                    vt->x += (8-(vt->x&0x7)); // (1..8)
                    DONE();
                    continue;
                }

                case 0x0A: // LF
                case 0x0B: // VT
                case 0x0C: // FF
                {
                    // add line if y == h-1
                    // - recycle if lines == MAX_ARCHIVE_LINES
                    vt->Flush();
                    if (vt->y == vt->h-1)
                    {
                        if (vt->lines == A3D_VT::MAX_ARCHIVE_LINES)
                        {
                            vt->first_line++;
                            int lidx = (vt->first_line + vt->y) % A3D_VT::MAX_ARCHIVE_LINES;
                            if (vt->line[lidx])
                            {
                                free(vt->line[lidx]);
                                vt->line[lidx] = 0;
                            }
                        }
                        else
                        {
                            vt->lines++;
                            if (vt->lines >= vt->h)
                            {
                                vt->first_line++;
                                int lidx = (vt->first_line + vt->y) % A3D_VT::MAX_ARCHIVE_LINES;
                                if (vt->line[lidx])
                                {
                                    free(vt->line[lidx]);
                                    vt->line[lidx] = 0;
                                }
                            }
                        }
                    }
                    else
                        vt->y++;
                    
                    DONE();
                    continue;
                }

                case 0x0D: // CR
                {
                    vt->GotoXY(0,vt->y);
                    DONE();
                    continue;
                }

                case 0x0E: // SO
                    // switch to G1
                    vt->gl = 1;
                    DONE();
                    continue;

                case 0x0F: // SI
                    // switch to G0
                    vt->gl = 0;
                    DONE();
                    continue;

                default:
                    if (chr<0x20)
                    {
                        // invalid C0
                        TODO();
                        continue;
                    }
            }

            out_chr:

            if (chr<0x20 || chr>=127)
            {
                int aaa=0;
            }

            vt->Write(chr);

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
                        vt->GotoXY(vt->x,vt->y+1);
                        DONE();
                        break;
                    }

                    case 'E': // (NEL)
                    {
                        // Next Line
                        // Equivalent to CR+LF. Used to mark end-of-line on some IBM mainframes. 
                        seq_ctx = 0;
                        vt->GotoXY(0,vt->y+1);
                        DONE();
                        break;
                    }

                    case 'H': // (HTS)
                    {
                        // Character Tabulation Set, Horizontal Tabulation Set
                        // Causes a character tabulation stop to be set at the active position. 
                        seq_ctx = 0;
                        TODO();
                        break;
                    }
                    
                    case 'M': // (RI)
                    {
                        // Reverse Line Feed, Reverse Index
                        seq_ctx = 0;
                        vt->GotoXY(vt->x,vt->y-1);
                        DONE();
                        break;
                    }

                    case 'N': // (SS2)
                    {
                        // Single-Shift 2
                        // Next character invokes a graphic character from the G2 graphic sets respectively. 
                        seq_ctx = 0;
                        vt->single_shift = 2;
                        DONE();
                        break;
                    }

                    case 'O': // (SS3)
                    {
                        // Single-Shift 3
                        // Next character invokes a graphic character from the G3 graphic sets respectively. In systems that conform to ISO/IEC 4873 (ECMA-43), even if a C1 set other than the default is used, these two octets may only be used for this purpose.                         break;
                        seq_ctx = 0;
                        vt->single_shift = 3;
                        DONE();
                        break;
                    }

                    case 'V': // (SPA)
                    {
                        // Start of Protected Area
                        seq_ctx = 0;
                        TODO();
                        break;
                    }

                    case 'W': // (EPA)
                    {
                        // End of Protected Area
                        seq_ctx = 0;
                        TODO();
                        break;
                    }

                    case '\\':// (ST)
                    {
                        // String Terminator
                        seq_ctx = 0;
                        DONE();
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
                        TODO();
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
                        DONE();
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
                        DONE();
                        break;

                    case '<': // Exit VT52 mode (Enter VT100 mode).
                        seq_ctx = 0;
                        TODO();
                        break;

                    case '=': // DECPAM Application Keypad ()
                        vt->app_keypad = true;
                        seq_ctx = 0;
                        DONE();
                        break; 

                    case '>': // DECPNM Normal Keypad ()    
                        vt->app_keypad = false;
                        seq_ctx = 0;
                        DONE();
                        break;

                    case 'F': // Cursor to lower left corner of screen (if enabled by the hpLowerleftBugCompat resource). 
                    {
                        vt->x = 0;
                        vt->y = vt->h-1;
                        seq_ctx = 0;
                        DONE();
                        break;
                    }

                    case 'c': // RIS 	Full Reset () 
                        Reset(vt);
                        seq_ctx = 0;
                        DONE();
                        break;

                    case 'l': // Memory Lock (per HP terminals). Locks memory above the cursor. 
                        seq_ctx = 0;
                        TODO();
                        break;

                    case 'm': // Memory Unlock (per HP terminals) 
                        seq_ctx = 0;
                        TODO();
                        break;

                    case 'n': // LS2 	Invoke the G2 Character Set () 
                        seq_ctx = 0;
                        vt->gr = 2;
                        DONE();
                        break;

                    case 'o': // LS3 	Invoke the G3 Character Set () 
                        seq_ctx = 0;
                        vt->gr = 3;
                        DONE();
                        break;

                    case '|': // LS3R 	Invoke the G3 Character Set as GR (). Has no visible effect in xterm. 
                        seq_ctx = 0;
                        TODO();
                        break;

                    case '}': // LS2R 	Invoke the G2 Character Set as GR (). Has no visible effect in xterm. 
                        seq_ctx = 0;
                        TODO();
                        break;
                        
                    case '~': // LS1R 	Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
                        seq_ctx = 0;
                        TODO();
                        break;
                        
                    default:
                        seq_ctx = 0;
                        TODO();
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
                        TODO();
                        break;
                    case 'G':
                        seq_ctx = 0;
                        // set C1 sending mode to 8-bit seqs
                        TODO();
                        break;
                    case 'L':
                        seq_ctx = 0;
                        // set ANSI conformance level = 1
                        TODO();
                        break;
                    case 'M':
                        seq_ctx = 0;
                        // set ANSI conformance level = 2
                        TODO();
                        break;
                    case 'N':
                        seq_ctx = 0;
                        // set ANSI conformance level = 3
                        TODO();
                        break;

                    default: 
                        seq_ctx = 0;
                        TODO();
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
                        TODO();
                        break;

                    case '4':
                        seq_ctx = 0;
                        // set double-height line, lower half
                        // current whole line is permanently set to it
                        TODO();
                        break;

                    case '5':
                        seq_ctx = 0;
                        // set single-width line
                        // current whole line is permanently set to it
                        TODO();
                        break;

                    case '6':
                        seq_ctx = 0;
                        // set double-width line
                        // current whole line is permanently set to it
                        TODO();
                        break;

                    case '8':
                        seq_ctx = 0;
                        // screen align test
                        // fills screen with EEEE and sets different modes to lines 
                        TODO();
                        break;

                    default: 
                        seq_ctx = 0;
                        TODO();
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
                        DONE();
                        break;
                    case 'G':
                        seq_ctx = 0;
                        UTF8 = true; // Select UTF-8 character set, ISO 2022.
                        DONE();
                        break;

                    default: 
                        seq_ctx = 0;                
                        TODO();
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
                        DONE();
                        break;
                    case 'A':
                        seq_ctx = 0;
                        // set G0/G1 = United Kingdom (UK)
                        vt->G_table[g_index] = 1;
                        DONE();
                        break;
                    case '4':
                        seq_ctx = 0;
                        // set G0/G1 = Dutch
                        vt->G_table[g_index] = 2;
                        DONE();
                        break;
                    case 'C': case '5':
                        seq_ctx = 0;
                        // set G0/G1 = Finnish
                        vt->G_table[g_index] = 3;
                        DONE();
                        break;
                    case 'R': case 'f':
                        seq_ctx = 0;
                        // set G0/G1 = French
                        vt->G_table[g_index] = 4;
                        DONE();
                        break;
                    case 'Q': case '9':
                        seq_ctx = 0;
                        // set G0/G1 = French Canadian
                        vt->G_table[g_index] = 5;
                        DONE();
                        break;
                    case 'K':
                        seq_ctx = 0;
                        // set G0/G1 = German
                        vt->G_table[g_index] = 6;
                        DONE();
                        break;
                    case 'Y':
                        seq_ctx = 0;
                        // set G0/G1 = Italian
                        vt->G_table[g_index] = 7;
                        DONE();
                        break;
                    case '`': case 'E': case '6':
                        seq_ctx = 0;
                        // set G0/G1 = Norwegian/Danish
                        vt->G_table[g_index] = 8;
                        DONE();
                        break;
                    case 'Z':
                        seq_ctx = 0;
                        // set G0/G1 = Spanish
                        vt->G_table[g_index] = 9;
                        DONE();
                        break;
                    case 'H': case '7': 
                        seq_ctx = 0;
                        // set G0/G1 = Swedish
                        vt->G_table[g_index] = 10;
                        DONE();
                        break;
                    case '=':
                        seq_ctx = 0;
                        // set G0/G1 = Swiss
                        vt->G_table[g_index] = 11;
                        DONE();
                        break;
                    case '0':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Special Character and Line Drawing Set
                        vt->G_table[g_index] = 12;
                        DONE();
                        break;
                    case '<':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Supplemental
                        vt->G_table[g_index] = 13;
                        DONE();
                        break;
                    case '>':
                        seq_ctx = 0;
                        // set G0/G1 = DEC Technical
                        vt->G_table[g_index] = 14;
                        DONE();
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
                        seq_ctx = 0;                
                        TODO();
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
                        DONE();
                        break;
                    case 'F': // ISO Greek Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 27;
                        DONE();
                        break;
                    case 'H': // ISO Hebrew Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 28;
                        DONE();
                        break;
                    case 'L': // ISO Latin-Cyrillic
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 29;
                        DONE();
                        break;
                    case 'M': // ISO Latin-5 Supplemental
                        seq_ctx = 0;
                        // set to G1,G2 or G3
                        vt->G_table[g_index] = 30;
                        DONE();
                        break;

                    default: 
                        seq_ctx = 0;                
                        TODO();
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
                        DONE();
                        break;
                    case '?': // DEC Greek
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 16;
                        DONE();
                        break;
                    case '4': // DEC Hebrew
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 17;
                        DONE();
                        break;

                    default:
                        seq_ctx = 0;                
                        TODO();
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
                        DONE();
                        break;
                    case '6': // Portuguese
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 19;
                        DONE();
                        break;
                    case '2': // Turkish
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 20;
                        DONE();
                        break;
                    case '5': // DEC Supplemental Graphics
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 21;
                        DONE();
                        break;
                    case '0': // DEC Turkish
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 22;
                        DONE();
                        break;
                    case '3': // SCS NRCS
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 23;
                        DONE();
                        break;

                    default:
                        seq_ctx = 0;                
                        TODO();
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
                        DONE();
                        break;
                    case '5': // DEC Russian
                        seq_ctx = 0;                
                        vt->G_table[g_index] = 25;
                        DONE();
                        break;

                    default:
                        seq_ctx = 0;                
                        TODO();
                }

                break;
            }

            case '[': // (CSI)
            {
                if (chr < 0x20)
                {
                    // err
                    str_len = 0;
                    seq_ctx = 0;
                    TODO();
                }
                else
                if (chr <= 0x40)
                {
                    // values ( 0 .. 9 ), 
                    // separators ( ; : ), 
                    // prefixes: ( ? > = ! ) 
                    // postfixes: ( SP " $ # ' * )
                    if (str_len < A3D_VT::MAX_STR_LEN)
                        vt->str[str_len++] = (char)chr;
                }
                else
                if (str_len >= A3D_VT::MAX_STR_LEN)
                {
                    // terminator but sequence is truncated
                    // err
                    str_len = 0;
                    seq_ctx = 0;
                    TODO();
                }
                else
                {
                    vt->str[str_len] = 0;
                    // terminators ( @ .. ~ )
                    switch (chr)
                    {
                        case '@': 
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps SP @
                            // Shift left Ps columns(s) (default = 1) (SL), ECMA-48.
                            if (str_len && vt->str[str_len-1] == ' ')
                            {
                                TODO();
                                break;
                            }

                            // CSI Ps @ 
                            // Insert Ps (Blank) Character(s) (default = 1) (ICH).
                            vt->InsertBlanks(Ps);
                            DONE();
                            break;
                        }
                        case 'A': 
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps SP A
                            // Shift right Ps columns(s) (default = 1) (SR), ECMA-48.
                            if (str_len && vt->str[str_len-1] == ' ')
                            {
                                TODO();
                                break;
                            }

                            // CSI Ps A 
                            // Cursor Up Ps Times (default = 1) (CUU).
                            if (Ps)
                                vt->GotoXY(vt->x,vt->y-Ps);
                            DONE();
                            break;
                        }
                        case 'B':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps B
                            // Cursor Down Ps Times (default = 1) (CUD).
                            if (Ps)
                                vt->GotoXY(vt->x,vt->y+Ps);
                            DONE();
                            break;
                        }
                        case 'C':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps C
                            // Cursor Forward Ps Times (default = 1) (CUF).
                            if (Ps)
                                vt->GotoXY(vt->x+Ps,vt->y);
                            DONE();
                            break;
                        }
                        case 'D':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps D
                            // Cursor Backward Ps Times (default = 1) (CUB).
                            if (Ps)
                                vt->GotoXY(vt->x-Ps,vt->y);
                            DONE();
                            break;
                        }
                        case 'E':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps E
                            // Cursor Next Line Ps Times (default = 1) (CNL).
                            if (Ps)
                                vt->GotoXY(0,vt->y+Ps);
                            DONE();
                            break;
                        }
                        case 'F':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps F
                            // Cursor Preceding Line Ps Times (default = 1) (CPL).
                            if (Ps)
                                vt->GotoXY(0,vt->y-Ps);
                            DONE();
                            break;
                        }
                        case 'G':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;

                            // CSI Ps G
                            // Cursor Character Absolute  [column] (default = [row,1]) (CHA).
                            vt->GotoXY(Ps-1,vt->y);
                            DONE();
                            break;
                        }
                        case 'H':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Psx, Psy = strtol(str, &end, 10);
                            if (end == str)
                            {
                                Psy=1;
                                Psx=1;
                            }
                            else
                            if (*end == ';')
                            {
                                str = end+1;
                                Psx = strtol(str, &end, 10);
                                if (end == str)
                                    Psx=1;
                            }
                            
                            // CSI Ps;Ps H
                            // Cursor Position [row;column] (default = [1,1]) (CUP).
                            vt->GotoXY(Psx-1,Psy-1);
                            DONE();
                            break;
                        }
                        case 'I':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                        
                            // CSI Ps I
                            // Cursor Forward Tabulation Ps tab stops (default = 1) (CHT).
                            if (Ps>0)
                                vt->GotoXY(vt->x + 8*(Ps-1) + (8-(vt->x&0x7)) ,vt->y);
                            DONE();
                            break;
                        }
                        case 'J': 
                        {
                            if (str_len && vt->str[0]=='?')
                            {
                                // CSI ? Ps J
                                // Erase in Display (DECSED), VT220.
                                TODO();
                                break;
                            }

                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=0;  

                            // CSI Ps J 
                            // Erase in Display (ED), VT100.

                            switch (Ps)
                            {
                                case 0: 
                                    // erase all lines below
                                    vt->EraseRows(vt->y+1,vt->h); 
                                    DONE();
                                    break;

                                case 1: 
                                    // erase all lines above
                                    vt->EraseRows(0,vt->y); 
                                    DONE();
                                    break;

                                case 2: 
                                    // erase everything
                                    vt->EraseRows(0,vt->h); 
                                    DONE();
                                    break;

                                case 3: 
                                    // erase saved lines ??? (xterm)
                                    TODO();
                                    break;

                                default:
                                    TODO();
                            }
                            break;
                        }
                        case 'K': 
                        {
                            if (str_len && vt->str[0] == '?')
                            {
                                // CSI ? Ps K
                                // Erase in Line (DECSEL), VT220.
                                TODO();
                                break;
                            }

                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=0;

                            // CSI Ps K 
                            // Erase in Line (EL), VT100.
                            vt->EraseRow(Ps);
                            DONE();
                            break;
                        }
                        case 'L':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;  

                            // CSI Ps L
                            // Insert Ps Line(s) (default = 1) (IL).
                            if (Ps>0)
                                vt->InsertLines(Ps);
                            DONE();
                            break;
                        }
                        case 'M':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;  

                            // CSI Ps M
                            // Delete Ps Line(s) (default = 1) (DL).
                            if (Ps>0)
                                vt->DeleteLines(Ps);
                            DONE();
                            break;
                        }
                        case 'P':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;  

                            // CSI Ps P
                            // Delete Ps Character(s) (default = 1) (DCH).
                            if (Ps>0)
                                vt->DeleteChars(Ps);
                            DONE();
                            break;
                        }
                        case 'S': 
                        {
                            if (str_len && vt->str[0]=='?')
                            {
                                // CSI ? Pi;Pa;Pv S
                                // If configured to support either Sixel Graphics or ReGIS Graphics...
                                TODO();
                                break;
                            }

                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;  

                            // CSI Ps S
                            // Scroll up Ps lines (default = 1) (SU), VT420, ECMA-48. 

                            if (Ps>0)
                                vt->Scroll(Ps);
                            DONE();
                            break;
                        }
                        case 'T': 
                        {
                            if (str_len && vt->str[0]=='>')
                            {
                                // CSI > Ps;Ps T
                                // Reset one or more features of the title modes to the default value.
                                TODO();
                                break;
                            }
                            if (!str_len || !strchr(vt->str,';'))
                            {
                                // CSI Ps T 
                                // Scroll down Ps lines (default = 1) (SD), VT420.
                                char* end = 0;
                                char* str = vt->str;
                                int Ps = strtol(str, &end, 10);
                                if (end == str)
                                    Ps=1;                                  
                                if (Ps>0)
                                    vt->Scroll(-Ps);
                                DONE();
                                break;
                            }
                            // CSI Ps;Ps;Ps;Ps;Ps T 
                            // Initiate highlight mouse tracking.
                            TODO();
                            break;
                        }
                        case 'X':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                                  
                            // CSI Ps X
                            // Erase Ps Character(s) (default = 1) (ECH).
                            if (Ps>0)
                                vt->EraseChars(Ps);
                            DONE();
                            break;
                        }
                        case 'Z':
                        {
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                        
                            // CSI Ps Z
                            // Cursor Backward Tabulation Ps tab stops (default = 1) (CBT).
                            if (Ps>0)
                            {
                                if ((vt->x&0x7)==0)
                                    Ps++;
                                vt->GotoXY(vt->x - 8*(Ps-1) - (vt->x&0x7) ,vt->y);
                            }
                            DONE();
                            break;
                        }
                        case '^':
                        {
                            // CSI Ps ^
                            // Scroll down Ps lines (default = 1) (SD), ECMA-48.
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                                  
                            if (Ps>0)
                                vt->Scroll(-Ps);
                            DONE();
                            break;
                        }
                        case '`':
                        {
                            // CSI Pm `
                            // Character Position Absolute  [column] (default = [row,1])
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;
                            int w = vt->GetCurLineLen();
                            int x = Ps-1 < w ? Ps-1 : w;
                            vt->GotoXY(x,vt->y);
                            DONE();
                            break;
                        }
                        case 'a':
                        {
                            // CSI Pm a
                            // Character Position Relative  [columns] (default = [row,col+1])
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;
                            int w = vt->GetCurLineLen();
                            int x = x+Ps < w ? x+Ps : w;
                            vt->GotoXY(x,vt->y);
                            DONE();
                            break;
                        }
                        case 'b':
                            // CSI Ps b
                            // Repeat the preceding graphic character Ps times (REP).
                            TODO();
                            break;
                        case 'c': 

                            // CSI Ps c 
                            // Send Device Attributes (Primary DA).

                            // CSI = Ps c 
                            // Send Device Attributes (Tertiary DA).

                            // CSI > Ps c
                            // Send Device Attributes (Secondary DA).

                            TODO();
                            break;
                        case 'd':
                        {
                            // CSI Pm d
                            // Line Position Absolute  [row] (default = [1,column]) (VPA).
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                            
                            vt->GotoXY(vt->x, Ps-1);
                            DONE();
                            break;
                        }
                        case 'e':
                        {
                            // CSI Pm e
                            // Line Position Relative  [rows] (default = [row+1,column]) (VPR).
                            char* end = 0;
                            char* str = vt->str;
                            int Ps = strtol(str, &end, 10);
                            if (end == str)
                                Ps=1;                            
                            vt->GotoXY(vt->x, vt->y+Ps);
                            DONE();
                            break;
                        }
                        case 'f':
                        {
                            // CSI Ps;Ps f
                            // Horizontal and Vertical Position [row;column] (default = [1,1]) (HVP).
                            // how is this different from (CUP)?
                            char* end = 0;
                            char* str = vt->str;
                            int Psx, Psy = strtol(str, &end, 10);
                            if (end == str)
                            {
                                Psy=1;
                                Psx=1;
                            }
                            else
                            if (*end == ';')
                            {
                                str = end+1;
                                Psx = strtol(str, &end, 10);
                                if (end == str)
                                    Psx=1;
                            }
                            
                            vt->GotoXY(Psx-1,Psy-1);
                            DONE();
                            break;
                        }
                        case 'g':
                            // CSI Ps g
                            TODO();
                            break;
                        case 'h': // CRITICAL
                        {
                            if (str_len >= 1)
                            {
                                if (str_len == 1 && vt->str[0] == '4')
                                {
                                    vt->Flush();
                                    vt->ins_mode = true;
                                    DONE();
                                    break;
                                }

                                if (vt->str[0] == '?')
                                {
                                    char* end = 0;
                                    char* str = vt->str+1;
                                    int Ps = strtol(str, &end, 10);
                                    if (end != str)
                                    {
                                        switch (Ps)
                                        {
                                            case 1: // Application Cursor Keys (DECCKM), VT100.
                                                vt->app_cursors = true;
                                                DONE(); 
                                                break;
                                            case 2: // Designate USASCII for character sets G0-G3
                                                TODO(); 
                                                break;
                                            case 3: // 132 Column Mode (DECCOLM), VT100.
                                                TODO(); 
                                                break;
                                            case 4: // Smooth (Slow) Scroll (DECSCLM), VT100.
                                                TODO(); 
                                                break;
                                            case 5: // Reverse Video (DECSCNM), VT100.
                                                TODO(); 
                                                break;
                                            case 6: // Origin Mode (DECOM), VT100.
                                                TODO(); 
                                                break;
                                            case 7: // Auto-wrap Mode (DECAWM), VT100.
                                                vt->auto_wrap = true;
                                                DONE(); 
                                                break;
                                            case 8: // Auto-repeat Keys (DECARM), VT100.
                                                TODO(); 
                                                break;
                                            case 9: // Send Mouse X & Y on button press.
                                                TODO(); 
                                                break;
                                            case 10: // Show toolbar (rxvt).
                                                TODO(); 
                                                break;
                                            case 12: // Start Blinking Cursor (AT&T 610).
                                                TODO(); 
                                                break;
                                            case 13: // Start Blinking Cursor (set only via resource or menu).
                                                TODO(); 
                                                break;
                                            case 14: // Enable XOR of Blinking Cursor control sequence and menu.
                                                TODO(); 
                                                break;
                                            case 18: // Print form feed (DECPFF), VT220.
                                                TODO(); 
                                                break;
                                            case 19: // Set print extent to full screen (DECPEX), VT220.
                                                TODO(); 
                                                break;
                                            case 25: // Show Cursor (DECTCEM), VT220.
                                                TODO(); 
                                                break;
                                            case 30: // Show scrollbar (rxvt).
                                                TODO(); 
                                                break;
                                            case 35: // Enable font-shifting functions (rxvt).
                                                TODO(); 
                                                break;
                                            case 38: // Enter Tektronix Mode (DECTEK), VT240, xterm.
                                                TODO(); 
                                                break;
                                            case 40: // Allow 80 -> 132 Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 41: // more(1) fix (see curses resource).
                                                TODO(); 
                                                break;
                                            case 42: // Enable National Replacement Character sets (DECNRCM), VT220.
                                                TODO(); 
                                                break;
                                            case 44: // Turn On Margin Bell, xterm.
                                                TODO(); 
                                                break;
                                            case 45: // Reverse-wraparound Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 46: // Start Logging, xterm.  This is normally disabled by a compile-time option.
                                                TODO(); 
                                                break;
                                            case 47: // Use Alternate Screen Buffer, xterm.  This may be disabled by the titeInhibit resource.
                                                TODO(); 
                                                break;
                                            case 66: // Application keypad (DECNKM), VT320.
                                                TODO(); 
                                                break;
                                            case 67: // Backarrow key sends backspace (DECBKM), VT340, VT420.
                                                TODO(); 
                                                break;
                                            case 69: // Enable left and right margin mode (DECLRMM), VT420 and up.
                                                TODO(); 
                                                break;
                                            case 95: // Do not clear screen when DECCOLM is set/reset (DECNCSM), VT510 and up
                                                TODO(); 
                                                break;
                                            case 1000: // Send Mouse X & Y on button press and release.
                                                TODO(); 
                                                break;
                                            case 1001: // Use Hilite Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1002: // Use Cell Motion Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1003: // Use All Motion Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1004: // Send FocusIn/FocusOut events, xterm.
                                                TODO(); 
                                                break;
                                            case 1005: // Enable UTF-8 Mouse Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 1006: // Enable SGR Mouse Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 1007: // Enable Alternate Scroll Mode, xterm
                                                TODO(); 
                                                break;
                                            case 1010: // Scroll to bottom on tty output (rxvt).
                                                TODO(); 
                                                break;
                                            case 1011: // Scroll to bottom on key press (rxvt).
                                                TODO(); 
                                                break;
                                            case 1015: // Enable urxvt Mouse Mode.
                                                TODO(); 
                                                break;
                                            case 1034: // Interpret "meta" key, xterm.  This sets eighth bit of keyboard input
                                                TODO(); 
                                                break;
                                            case 1035: // Enable special modifiers for Alt and NumLock keys, xterm.
                                                TODO(); 
                                                break;
                                            case 1036: // Send ESC when Meta modifies a key, xterm.
                                                TODO(); 
                                                break;
                                            case 1037: // Send DEL from the editing-keypad Delete key, xterm.
                                                TODO(); 
                                                break;
                                            case 1039: // Send ESC  when Alt modifies a key, xterm.
                                                TODO(); 
                                                break;
                                            case 1040: // Keep selection even if not highlighted, xterm.
                                                TODO(); 
                                                break;
                                            case 1041: // Use the CLIPBOARD selection, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1042: // Enable Urgency window manager hint when Control-G is received, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1043: // Enable raising of the window when Control-G is received, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1044: // Reuse the most recent data copied to CLIPBOARD, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1046: // Enable switching to/from Alternate Screen Buffer, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1047: // Use Alternate Screen Buffer, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1048: // Save cursor as in DECSC, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1049: // Save cursor as in DECSC, xterm.  After saving the cursor, switch to the Alternate Screen Buffer, clearing it first.
                                                TODO(); 
                                                break;
                                            case 1050: // Set terminfo/termcap function-key mode, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1051: // Set Sun function-key mode, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1052: // Set HP function-key mode, xterm.
                                                TODO(); 
                                                break;                                                                                                   
                                            case 1053: // Set SCO function-key mode, xterm.
                                                TODO(); 
                                                break; 
                                            case 1060: // Set legacy keyboard emulation (i.e, X11R6), xterm.
                                                TODO(); 
                                                break;
                                            case 1061: // Set VT220 keyboard emulation, xterm.
                                                TODO(); 
                                                break;                                                                                                                                                                                                    
                                            case 2004: // Set bracketed paste mode, xterm.
                                                TODO(); 
                                                break; 

                                            default:
                                                TODO(); 
                                        }
                                        break;
                                    }

                                    TODO();
                                    break;
                                }
                                TODO();
                                break;
                            }

                            // CSI Pm h 
                            // CSI ? Pm h
                            TODO();
                            break;
                        }
                        case 'i': 
                        
                            // CSI Pm i 
                            // Media Copy (MC).

                            // CSI ? Pm i
                            // Media Copy (MC), DEC-specific.

                            TODO();
                            break;
                        case 'l': // CRITICAL
                        {
                            if (str_len >= 1)
                            {
                                if (str_len == 1 && vt->str[0] == '4')
                                {
                                    vt->Flush();
                                    vt->ins_mode = false;
                                    DONE();
                                    break;
                                }

                                if (vt->str[0] == '?')
                                {
                                    char* end = 0;
                                    char* str = vt->str+1;
                                    int Ps = strtol(str, &end, 10);
                                    if (end != str)
                                    {
                                        switch (Ps)
                                        {
                                            case 1: // Normal Cursor Keys (DECCKM), VT100.
                                                vt->app_cursors = false;
                                                DONE(); 
                                                break;
                                            case 2: // Designate VT52 mode (DECANM), VT100.
                                                TODO(); 
                                                break;
                                            case 3: // 80 Column Mode (DECCOLM), VT100.
                                                TODO(); 
                                                break;
                                            case 4: // Jump (Fast) Scroll (DECSCLM), VT100.
                                                TODO(); 
                                                break;
                                            case 5: // Normal Video (DECSCNM), VT100.
                                                TODO(); 
                                                break;
                                            case 6: // Normal Cursor Mode (DECOM), VT100.
                                                TODO(); 
                                                break;
                                            case 7: // No Auto-wrap Mode (DECAWM), VT100.
                                                vt->auto_wrap = false;
                                                DONE(); 
                                                break;
                                            case 8: // No Auto-repeat Keys (DECARM), VT100.
                                                TODO(); 
                                                break;
                                            case 9: // Don't send Mouse X & Y on button press, xterm.
                                                TODO(); 
                                                break;
                                            case 10: // Hide toolbar (rxvt).
                                                TODO(); 
                                                break;
                                            case 12: // Stop Blinking Cursor (AT&T 610).
                                                TODO(); 
                                                break;
                                            case 13: // Disable Blinking Cursor.
                                                TODO(); 
                                                break;
                                            case 14: // Disable XOR of Blinking Cursor control sequence and menu.
                                                TODO(); 
                                                break;
                                            case 18: // Don't print form feed (DECPFF).
                                                TODO(); 
                                                break;
                                            case 19: // Limit print to scrolling region (DECPEX).
                                                TODO(); 
                                                break;
                                            case 25: // Hide Cursor (DECTCEM), VT220.
                                                TODO(); 
                                                break;
                                            case 30: // Don't show scrollbar (rxvt).
                                                TODO(); 
                                                break;
                                            case 35: // Disable font-shifting functions (rxvt).
                                                TODO(); 
                                                break;
                                            /*    
                                            case 38: // Enter Tektronix Mode (DECTEK), VT240, xterm.
                                                TODO(); 
                                                break;
                                            */
                                            case 40: // Disallow 80 -> 132 Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 41: // No more(1) fix (see curses resource).
                                                TODO(); 
                                                break;
                                            case 42: // Disable National Replacement Character sets
                                                TODO(); 
                                                break;
                                            case 44: // Turn Off Margin Bell, xterm.
                                                TODO(); 
                                                break;
                                            case 45: // No Reverse-wraparound Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 46: // Stop Logging, xterm.
                                                TODO(); 
                                                break;
                                            case 47: // Use Normal Screen Buffer, xterm.
                                                TODO(); 
                                                break;
                                            case 66: // Numeric keypad (DECNKM), VT320.
                                                TODO(); 
                                                break;
                                            case 67: // Backarrow key sends delete (DECBKM), VT340, VT420.
                                                TODO(); 
                                                break;
                                            case 69: // Disable left and right margin mode (DECLRMM), VT420 and up.
                                                TODO(); 
                                                break;
                                            case 95: // Clear screen when DECCOLM is set/reset (DECNCSM), VT510 and up.
                                                TODO(); 
                                                break;
                                            case 1000: // Don't send Mouse X & Y on button press and release.
                                                TODO(); 
                                                break;
                                            case 1001: // Don't use Hilite Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1002: // Don't use Cell Motion Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1003: // Don't use All Motion Mouse Tracking, xterm.
                                                TODO(); 
                                                break;
                                            case 1004: // Don't send FocusIn/FocusOut events, xterm.
                                                TODO(); 
                                                break;
                                            case 1005: // Disable UTF-8 Mouse Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 1006: // Disable SGR Mouse Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 1007: // Disable Alternate Scroll Mode, xterm.
                                                TODO(); 
                                                break;
                                            case 1010: // Don't scroll to bottom on tty output (rxvt).
                                                TODO(); 
                                                break;
                                            case 1011: // Don't scroll to bottom on key press (rxvt).
                                                TODO(); 
                                                break;
                                            case 1015: // Disable urxvt Mouse Mode.
                                                TODO(); 
                                                break;
                                            case 1034: // Don't interpret "meta" key, xterm.  
                                                TODO(); 
                                                break;
                                            case 1035: // Disable special modifiers for Alt and NumLock keys, xterm.
                                                TODO(); 
                                                break;
                                            case 1036: // Don't send ESC  when Meta modifies a key, xterm.
                                                TODO(); 
                                                break;
                                            case 1037: // end VT220 Remove from the editing-keypad Delete key, xterm.
                                                TODO(); 
                                                break;
                                            case 1039: // Don't send ESC when Alt modifies a key, xterm.
                                                TODO(); 
                                                break;
                                            case 1040: // Do not keep selection when not highlighted, xterm.
                                                TODO(); 
                                                break;
                                            case 1041: // Use the PRIMARY selection, xterm.  
                                                TODO(); 
                                                break;                                                
                                            case 1042: // Disable Urgency window manager hint when Control-G is received, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1043: // Disable raising of the window when Control-G is received, xterm.
                                                TODO(); 
                                                break;  
                                            /*                                              
                                            case 1044: // Reuse the most recent data copied to CLIPBOARD, xterm.
                                                TODO(); 
                                                break;
                                            */                                              
                                            case 1046: // Disable switching to/from Alternate Screen Buffer, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1047: // Use Normal Screen Buffer, xterm.  
                                                TODO(); 
                                                break;                                                
                                            case 1048: // Restore cursor as in DECRC, xterm.  
                                                TODO(); 
                                                break;                                                
                                            case 1049: // Use Normal Screen Buffer and restore cursor as in DECRC, xterm.
                                                TODO(); 
                                                break;
                                            case 1050: // Reset terminfo/termcap function-key mode, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1051: // Reset Sun function-key mode, xterm.
                                                TODO(); 
                                                break;                                                
                                            case 1052: // Reset HP function-key mode, xterm.
                                                TODO(); 
                                                break;                                                                                                   
                                            case 1053: // Reset SCO function-key mode, xterm.
                                                TODO(); 
                                                break; 
                                            case 1060: // Reset legacy keyboard emulation (i.e, X11R6), xterm.
                                                TODO(); 
                                                break;
                                            case 1061: // Reset keyboard emulation to Sun/PC style, xterm.
                                                TODO(); 
                                                break;                                                                                                                                                                                                    
                                            case 2004: // Reset bracketed paste mode, xterm.
                                                TODO(); 
                                                break; 

                                            default:
                                                TODO(); 
                                        }
                                        break;
                                    }

                                    TODO();
                                    break;
                                }
                                TODO();
                                break;
                            }

                            // CSI Pm l 
                            // CSI ? Pm l
                            TODO();
                            break;
                        }
                        case 'm': 
                        {
                            // CSI > Ps;Ps m
                            if (str_len && vt->str[0]=='>')
                            {
                                TODO();
                                break;
                            }

                            if (str_len)
                            {
                                char* str = vt->str;
                                while(true)
                                {
                                    if (*str == ';')
                                    {
                                        // default
                                        int Ps = 0;
                                        str++;
                                        continue;
                                    }
                                    char* end = 0;
                                    int Ps = strtol(str, &end, 10), Psb;
                                    if (end != str)
                                    {
                                        if (Ps == 8)
                                        {
                                            // invisible
                                            int aaa = 0;
                                        }
                                        if (*end==';')
                                            str = end+1;
                                        else
                                            break;
                                    }
                                }
                            }
                            // CSI Pm m 
                            //TODO(); // SGR!!!!!!!!!!!!!!!
                            break;
                        }
                        case 'n': 
                            // CSI Ps n 
                            // CSI > Ps n 
                            // CSI ? Ps n
                            TODO();
                            break;
                        case 'p': 
                            if (str_len==1 && vt->str[0] == '!')
                            {
                                // CSI ! p 
                                // Soft terminal reset (DECSTR), VT220 and up.
                                Reset(vt);
                                DONE();
                                break;
                            }

                            // CSI > Ps p 
                            // CSI Ps;Ps " p 
                            // CSI Ps $ p 
                            // CSI ? Ps $ p 
                            // CSI # p 
                            // CSI Ps;Ps # p
                            TODO();
                            break;
                        case 'q': 
                            // CSI Ps " q 
                            // CSI Ps SP q 
                            // CSI # q
                            TODO();
                            break;
                        case 'r': 
                            // CSI Ps;Ps r (CRITICAL)
                            // Set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM), VT100.
                            if (!str_len || vt->str[0] != '?' || vt->str[str_len-1] != '$')
                            {
                                char* end = 0;
                                char* str = vt->str;
                                int Pst = strtol(str, &end, 10), Psb;
                                if (end == str)
                                {
                                    Pst=1;
                                    Psb=vt->h;
                                }
                                else
                                if (*end == ';')
                                {
                                    str = end+1;
                                    Psb = strtol(str, &end, 10);
                                    if (end == str)
                                        Psb=vt->h;
                                }

                                vt->scroll_top = Pst-1;
                                vt->scroll_bottom = Psb;
                                
                                DONE();
                                break;
                            }

                            // CSI ? Pm r 
                            // CSI RECT;Ps $ r
                            TODO();
                            break;
                        case 's': 
                            // CSI s 
                            // CSI Pl;Pr s 
                            // CSI ? Pm s
                            TODO();
                            break;
                        case 't': 
                            // CSI Ps;Ps;Ps t 
                            // CSI > Ps;Ps t 
                            // CSI Ps SP t 
                            // CSI RECT;Ps $ t
                            TODO();
                            break;
                        case 'u': 
                            // CSI u 
                            // CSI Ps SP u
                            TODO();
                            break;
                        case 'v': 
                            // CSI RECT;Pp;Pt;Pl;Pp $ v
                            TODO();
                            break;
                        case 'w': 
                            // CSI Ps $ w 
                            // CSI RECT ' w
                            TODO();
                            break;
                        case 'x': 
                            // CSI Ps x 
                            // CSI Ps * x 
                            // CSI Pc;RECT $ x
                            TODO();
                            break;
                        case 'y': 
                            // CSI Ps # y 
                            // CSI Pi;Pg;RECT * y
                            TODO();
                            break;
                        case 'z': 
                            // CSI Ps;Pu ' z 
                            // CSI RECT $ z
                            TODO();
                            break;
                        case '{': 
                            // CSI Pm ' { 
                            // CSI # { 
                            // CSI Ps;Ps # { 
                            // CSI RECT $ { 
                            TODO();
                            break;
                        case '|': 
                            // CSI RECT # | 
                            // CSI Ps $ | 
                            // CSI Ps ' | 
                            // CSI Ps * |
                            TODO();
                            break;
                        case '}':  
                            // CSI # }
                            // CSI Pm ' }
                            TODO();
                            break;
                        case '~': 
                            // CSI Pm ' ~
                            TODO();
                            break;
                        default:
                            TODO();
                    }

                    // end of seq
                    str_len = 0;
                    seq_ctx = 0;
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
                    //TODO();
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
                    TODO();
                }

                break;
            }

            default:
                // error seq, dump it as regular chars!
                str_len = 0;
                seq_ctx = 0;
                TODO();
        }
    }

    // persist state till we have more data to process
    vt->UTF8 = UTF8;
    vt->chr_val = chr_val;
    vt->chr_ctx = chr_ctx;
    vt->seq_ctx = seq_ctx;
    vt->str_len = str_len;

    vt->dump_dirty |= 1; // processed input
    a3dMutexUnlock(vt->mutex);

    a3dDumpVT(vt);
    return true;
}

// should be replaced with a3dWriteKey(ki) a3dWriteChar(ch) and a3dMouse(x,y,mi)
int a3dWriteVT(A3D_VT* vt, const void* buf, size_t size)
{
    return a3dWritePTY(vt->pty, buf, size);
}

bool a3dGetVTCursorsMode(A3D_VT* vt)
{
    return vt->app_cursors;
}

static int DumpChr(char* buf, VT_CELL* cell)
{
    int chr = cell->ch;

    if (chr<0x80)
    {
        *(buf++) = (char)chr;
        return 1;
    }
    else
    if (chr<0x800)
    {
        *(buf++) = (char)( ((chr>>6)&0x1F) | 0xC0 );
        *(buf++) = (char)( (chr&0x3f) | 0x80 );
        return 2;
    }
    else
    if (chr<0x10000)
    {
        *(buf++) = (char)( ((chr>>12)&0x0F)|0xE0 );
        *(buf++) = (char)( ((chr>>6)&0x3f) | 0x80 );
        *(buf++) = (char)( (chr&0x3f) | 0x80 );
        return 3;
    }
    else
    if (chr<0x101000)
    {
        *(buf++) = (char)( ((chr>>18)&0x07)|0xF0 );
        *(buf++) = (char)( ((chr>>12)&0x3f) | 0x80 );
        *(buf++) = (char)( ((chr>>6)&0x3f) | 0x80 );
        *(buf++) = (char)( (chr&0x3f) | 0x80 );
        return 4;
    }

    return 0;
}

// TESTING!
bool a3dDumpVT(A3D_VT* vt)
{
    if (!vt->dump_dirty)
        return false;

    a3dMutexLock(vt->mutex);

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);    
    int tw = ws.ws_col;
    int th = ws.ws_row;

    // flush! temp
    // vt->Flush();

    write(STDOUT_FILENO,"\e[H",3); // goto home

    int h = vt->h;
    if ((vt->dump_dirty & 2) == 0)
    {
        // if terminal is not reset or resized
        // we can limit output to vt->lines
        h = h < vt->lines ? h : vt->lines;
    }

    h = h<th ? h : th;

    char buf[4+4*256] = "\e[2K"; // clear every line

    for (int i=0; i<h; i++)
    {
        int c = 4;
        int lidx = (vt->first_line + i) % A3D_VT::MAX_ARCHIVE_LINES;
        if (vt->temp_len && i==vt->y)
        {
            A3D_VT::LINE* l = vt->line[lidx]; 
            int lc = l ? l->cells : 0;
            int cells = lc + vt->temp_len;
            int w = cells < 256 ? cells : 255;
            w = w < vt->w ? w : vt->w;
            w = w < tw ? w : tw;

            if (vt->ins_mode)
            {
                int x=0;
                for (; x<vt->temp_ins && x<w; x++)
                    c += DumpChr(buf+c, vt->line[lidx]->cell + x);
                for (int t=0; t<vt->temp_len && x+t<w; t++)
                    c += DumpChr(buf+c, vt->temp + t);
                for (x=vt->temp_ins + vt->temp_len; x<w; x++)
                    c += DumpChr(buf+c, vt->line[lidx]->cell + x - vt->temp_len);
            }
            else
            {
                int x=0;
                for (; x<lc && x<w; x++)
                    c += DumpChr(buf+c, vt->line[lidx]->cell + x);
                for (; x<w; x++)
                    c += DumpChr(buf+c, vt->temp + x - lc);
            }
        }
        else
        if (vt->line[lidx])
        {
            int w = vt->line[lidx]->cells < 256 ? vt->line[lidx]->cells : 255;
            w = w < vt->w ? w : vt->w;
            w = w < tw ? w : tw;
            for (int x=0; x<w; x++)
                c += DumpChr(buf+c, vt->line[lidx]->cell + x);
        }

        if (i<h-1)
            buf[c++] = '\n';
        write(STDOUT_FILENO, buf, c);
    }

    char move[256];
    int move_len = sprintf(move,"\e[%d;%dH",vt->y+1,vt->x+1);
    write(STDOUT_FILENO,move,move_len);

    vt->dump_dirty = 0;

    a3dMutexUnlock(vt->mutex);

    return true;
}

/*
bool a3dDumpVT(A3D_VT* vt)
{
    if (!vt->dump_dirty)
        return false;

    a3dMutexLock(vt->mutex);

    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);    
    int tw = ws.ws_col;
    int th = ws.ws_row;

    // flush! temp
    vt->Flush();

    write(STDOUT_FILENO,"\e[H",3);

    int h = vt->lines < vt->h ? vt->lines : vt->h;
    h = h<th ? h : th;

    for (int i=0; i<h; i++)
    {
        write(STDOUT_FILENO, "\e[2K", 4);
        int lidx = (vt->first_line + i) % A3D_VT::MAX_ARCHIVE_LINES;
        if (vt->line[lidx])
        {
            char buf[4*256];
            int c = 0;
            int w = vt->line[lidx]->cells < 256 ? vt->line[lidx]->cells : 255;
            w = w < vt->w ? w : vt->w;
            w = w < tw ? w : tw;
            for (int x=0; x<w; x++)
            {
                int chr = vt->line[lidx]->cell[x].ch;

                if (chr<0x80)
                {
                    buf[c++] = (char)chr;
                }
                else
                if (chr<0x800)
                {
                    buf[c++] = (char)( ((chr>>6)&0x1F) | 0xC0 );
                    buf[c++] = (char)( (chr&0x3f) | 0x80 );
                }
                else
                if (chr<0x10000)
                {
                    buf[c++] = (char)( ((chr>>12)&0x0F)|0xE0 );
                    buf[c++] = (char)( ((chr>>6)&0x3f) | 0x80 );
                    buf[c++] = (char)( (chr&0x3f) | 0x80 );
                }
                else
                if (chr<0x101000)
                {
                    buf[c++] = (char)( ((chr>>18)&0x07)|0xF0 );
                    buf[c++] = (char)( ((chr>>12)&0x3f) | 0x80 );
                    buf[c++] = (char)( ((chr>>6)&0x3f) | 0x80 );
                    buf[c++] = (char)( (chr&0x3f) | 0x80 );
                }
            }

            if (i<vt->h-1)
                buf[c++] = '\n';
            write(STDOUT_FILENO, buf, c);
        }
        else
        {
            if (i<vt->h-1)
                write(STDOUT_FILENO, "\n", 1);
        }
    }


    char move[256];
    int move_len = sprintf(move,"\e[%d;%dH",vt->y+1,vt->x+1);
    write(STDOUT_FILENO,move,move_len);

    vt->dump_dirty = false;

    a3dMutexUnlock(vt->mutex);

    return true;
}
*/