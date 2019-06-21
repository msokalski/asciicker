#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include "asciiid_term.h"
#include "asciiid_platform.h"

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

static const int CP437_UCS2[256] =
{
    0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, 
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5, 
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510, 
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567, 
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229, 
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00FF
};

#include <math.h>

int int_descent(const void* a, const void* b)
{
    return ((const int*)b)[1] - ((const int*)a)[1];
}

struct PHF
{
    int range; // hash values range
    int minkey;
    int shift;
    int height;
    PHF* recurse;
    int* row; // if recurse is NOT null it is [recurse->range] otherwise [height] 

    int Hash(int key) const
    {
        int c = key - minkey;
        int y = c >> shift;

        assert(y>=0 && y<height);

        if (recurse)
            y = recurse->Hash(y);

        int x = c & ((1<<shift)-1);

        c = x + row[y];
        assert(c>=0 && c<range);

        return c;
    }

    void Free()
    {
        if (row)
            free(row);
        if (recurse)
        {
            recurse->Free();
            free(recurse);
        }
    }

    int Init(const int* charset, int size, int rec = 0)
    {
        uint64_t t0 = a3dGetTime();
        // if charset is not invertible simply use first glyph that is mapped to that char
        int gm_size; // must be pow2 >= size

        int max_char = 0;
        int min_char = 0;
        for (int i=0; i<size; i++)
        {
            min_char = min_char < charset[i] ? min_char : charset[i];
            max_char = max_char > charset[i] ? max_char : charset[i];
        }

        int domain = max_char - min_char + 1;

        // for (int aspect = -8; aspect<=8; aspect++)
        int aspect = 0;

        int hash_size = (int)floor( pow(2.0,aspect)*sqrt(domain) );

        hash_size = hash_size < 16 ? hash_size : 16;

        //if (rec && hash_size>=4)
        //   hash_size>>=3;

        shift = 0;
        while (hash_size)
        {
            hash_size>>=1;
            shift++;
        }

        int width = 1<<shift;
        height = (domain + width-1) / width;

        row = (int*)malloc(sizeof(int) * height);
        memset(row,0,sizeof(int) * height);

        minkey = min_char;

        int mask = width - 1;

        // bin array so we can quickly check for collisions
        int pitch = (width+7)/8;
        int bytes = pitch * height;
        uint8_t* arr = (uint8_t*)malloc(bytes);
        memset(arr,0,bytes);
        for (int i=0; i<size; i++)
        {
            int c = charset[i] - min_char;
            int y = c >> shift;
            int x = c & mask; // unshifted!
            arr[(x>>3) + pitch*y] |= 1<<(x&7);
        }

        // this will let us start from highest occupancy rows
        int* fil = (int*)malloc(sizeof(int) * height*2); // {row & fil}
        memset(fil,0,sizeof(int) * height*2);
        for (int i=0; i<height; i++)
            fil[2*i] = i;
        for (int i=0; i<size; i++)
        {
            int c = charset[i] - min_char;
            int y = c >> shift;
            fil[2*y+1]++;
        }

        qsort(fil, height, sizeof(int)*2, int_descent);

        int ner = height; // num of non-empty-rows
        for (int y=height-1; y>=0; y--)
        {
            if (fil[2*y+1])
                break;
            ner--;                
        }


        int min_shift = 0;
        int max_shift = 0;
        int max_idx = -1;

        for (int f = 0; f < height; f++)
        {
            int fy = fil[2*f];

            int s = 0;
            // initial s could be negative
            // so first existing on this row char appears on x=0
            for (int fx=0; fx<width; fx++)
            {
                if ( arr[(fx>>3) + pitch*fy] & (1<<(fx&7)) )
                    break;
                s--;
            }

            if (s==-width)
            {
                // empty row
                row[fy] = 0;
                continue;
            }

            int t = width-1;
            for (int fx=t; fx>=0; fx--)
            {
                if ( arr[(fx>>3) + pitch*fy] & (1<<(fx&7)) )
                    break;
                t--;
            }            

            do // s loop until all collisions are resolved
            {
                // run all chars on this row
                int fx=0;
                for (; fx<width; fx++)
                {
                    if ( arr[(fx>>3) + pitch*fy] & (1<<(fx&7)) )
                    {
                        // check if fx+s is already occupied by already processed rows
                        for (int g = 0; g < f; g++)
                        {
                            int gy = fil[2*g];
                            int gx = fx+s-row[gy];

                            // check if there is a char on 

                            if (gx>=0 && gx<width)
                            {

                                if ( arr[(gx>>3) + pitch*gy] & (1<<(gx&7)) )
                                {
                                    s++;
                                    g = f;   // exit g loop
                                    fx = -1; // restart chars
                                }
                            }
                        }
                    }
                }

                if (fx >= 0)
                {
                    if (t+s>max_idx)
                        max_idx = t+s;
                    if (s>max_shift)
                        max_shift = s;
                    if (s<min_shift)
                        min_shift = s;
                    row[fy] = s;
                    break;
                }

            } while (1);

            // printf("%d / %d\n", f, height);
        }

        range = max_idx+1;
        recurse = 0;


        free(arr);

        uint64_t t1 = a3dGetTime();

        int* test = (int*)malloc( range * sizeof(int) );
        memset(test,-1,range * sizeof(int));

        //printf("test: ");
        for (int i=0; i<size; i++)
        {
            int index = Hash(charset[i]);
            //if (i<20)
            //    printf("%d ",index);
            assert( index >= 0 );
            assert( index < range );
            assert( test[index] == -1 || test[index] == charset[i]);
            test[index] = charset[i];
        }
        //printf("... \n");

        free(test);

        int shift_range = max_shift - min_shift;
        int shift_bits = 0;
        while (shift_range)
        {
            shift_range >>= 1;
            shift_bits++;
        }

        //printf("elapsed %jd us\n", t1-t0);
        //printf("range = %d, width = %d, height = %d\n", range, width, height);
        //printf("index efficiency   = %5.2f %%\n", (double)size*100/range);
        //printf("storage efficiency = %5.2f bits/key\n", (double)shift_bits*height / size);

        // let's check recurency
        if (rec)
        {
            int* yarr = (int*)malloc(sizeof(int)*ner);
            for (int i=0; i<ner; i++)
                yarr[i] = fil[2*i];

            //printf("------------------\n");
            PHF* rphf = (PHF*)malloc(sizeof(PHF));
            rphf->Init(yarr,ner,rec-1);
            //printf("------------------\n");

            // if performed well
            {
                int* rrow = (int*)malloc(sizeof(int) * rphf->range);
                memset(rrow,0,sizeof(int) * rphf->range);
                for (int i=0; i<ner; i++)
                {
                    int j = rphf->Hash( yarr[i] );
                    rrow[j] = row[fil[2*i]];
                }
                free(row);
                row = rrow;
                recurse = rphf;
                

                // print compressed data
                printf("minkey=%d, shift=%d\n", minkey, shift);
                for (int r=0; r<rphf->range-1; r++)
                    printf( (r&7)==7 ? "%d,\n":"%d,", row[r] );
                printf( "%d\n", row[rphf->range-1] );                

                // test once again with recursion
                int* test = (int*)malloc( range * sizeof(int) );
                memset(test,-1,range * sizeof(int));

                //printf("test: ");
                for (int i=0; i<size; i++)
                {
                    int index = Hash(charset[i]);
                    //if (i<20)
                    //{
                    //    printf("%d ",index);
                    //    fflush(stdout);
                    //}
                    assert( index >= 0 );
                    assert( index < range );
                    assert( test[index] == -1 || test[index] == charset[i]);
                    test[index] = charset[i];
                }

                //printf("... \n");


                free(test);


            }

            free(yarr);
            // else
            //  rphf->Destroy();
        }
        else
        {
            // print uncompressed data
            printf("minkey=%d, shift=%d\n", minkey, shift);
            for (int y=0; y<height-1; y++)
                printf( (y&7)==7 ? "%d,\n":"%d,", row[y] );
            printf( "%d\n", row[height-1] );
        }
        
        free(fil);
    }
};

PHF perfect_hash;
int perfect_hash_ok = perfect_hash.Init(CP437_UCS2,256,1);

void SetScreen(bool alt)
{
	if (alt)
		printf("\x1B[?1049h\x1B[H"); // +home
	else
		printf("\x1B[?1049l");    
}