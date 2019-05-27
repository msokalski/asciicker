
#include <malloc.h>
#include "asciiid_term.h"
#include "fast_rand.h"

TermScreen* CreateScreen(int width, int height)
{

    TermScreen* screen =  (TermScreen*)malloc(sizeof(TermScreen) + (width*height-1)*sizeof(TermCell));
    screen->width = width;
    screen->height = height;
    return screen;
}

void DeleteScreen(TermScreen* screen)
{
    free(screen);
}

// give it null to have randomized fill
void ClearScreen(TermScreen* screen, int x, int y, int w, int h, const TermCell* fill)
{
    int x2 = x+w;
    x2 = x2<screen->width ? x2 : screen->width;
    int y2 = y+h;
    y2 = y2<screen->height ? y2 : screen->height;

    int x1 = x>=0 ? x : 0;
    int y1 = y>=0 ? y : 0;

    if (fill)
    {
        for (int y=y1; y<y2; y++)
        {
            TermCell* ptr = screen->cell + x1 + y*screen->width;
            for (int x=x1; x<x2; ptr++,x++)
                *ptr = *fill;
        }
    }
    else
    {
        // randomize chunks 1-4 chars len each
        TermCell c;
        int len = 0;
        for (int y=y1; y<y2; y++)
        {
            TermCell* ptr = screen->cell + x1 + y*screen->width;

            for (int x=x1; x<x2; ptr++,x++)
            {
                if (len==0)
                {
                    *(uint64_t*)&c = (uint64_t)fast_rand() | ((uint64_t)fast_rand()<<15) | ((uint64_t)fast_rand()<<30) | ((uint64_t)fast_rand()<<45);
                    c.ch = fast_rand()&0xFF;
                    len = 1; // (fast_rand()&3) + 1;
                }

                *ptr = c;
                len--;
            }
        }
    }
}

