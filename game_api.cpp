#include <stdint.h>
#include <string.h>

#include "game_api.h"
#include "game.h"

void* akAPI_Buff = 0;

void akAPI_Init()
{
    akAPI_Buff = malloc(AKAPI_BUF_SIZE);
}

void akAPI_Free()
{
    free(akAPI_Buff);
}

extern "C" void akAPI_Call(int id)
{
    printf("akAPI_Call(%d)\n",id);

    if (!game)
        return;

    switch (id)
    {
        case 0: 
        // getPos: function(arr3, ofs) { akAPI_Call(0); akReadF32(arr3,ofs|0,0,3); }
        {
            memcpy(akAPI_Buff, game->player.pos, sizeof(float[3]));
            break;
        }
        case 1: 
        // setPos: function(arr3, ofs) { akWriteF32(arr3,ofs|0,0,3); akAPI_Call(1); }
        {
            SetPhysicsPos(game->physics,(float*)akAPI_Buff,0);
            break;
        }

        case 2: 
        // getDir: function() { akAPI_Call(2); return akGetF32(0); }
        {
            *(float*)akAPI_Buff = game->player.dir;
            break;
        }
        case 3: 
        // setDir: function(flt) { akSetF32(flt,0); akAPI_Call(3); }
        {
            SetPhysicsDir(game->physics,*(float*)akAPI_Buff);
            break;
        }

        case 4: 
        // getYaw: function() { akAPI_Call(4); akGetF32(0); }
        {
            *(float*)akAPI_Buff = game->prev_yaw;
            break;
        }
        case 5: 
        // setYaw: function(flt) { akSetF32(flt,0); akAPI_Call(5); }
        {
            SetPhysicsYaw(game->physics,*(float*)akAPI_Buff,0);
            break;
        }

        case 6:
        // getName: function() { akAPI_Call(6); return akGetStr(0); },
        {
            int i=0;
            while (i<31 && game->player.name_cp437[i])
            {
                unsigned char cp437 = (unsigned char)game->player.name_cp437[i];
                // convert cp437 to ucs2
                // ...
                uint16_t ucs2 = (uint16_t)cp437;
                *((uint16_t*)akAPI_Buff+i) = ucs2;
                i++;
            }
            *((uint16_t*)akAPI_Buff+i) = 0;
            break;
        }

        case 7:
        // setName: function(str) { akSetStr(str); akAPI_Call(7); },
        {
            int i=0;
            while (i<31 && *((uint16_t*)akAPI_Buff+i))
            {
                uint16_t ucs2 = *((uint16_t*)akAPI_Buff+i);
                // convert ucs2 to cp437
                // ...
                unsigned char cp437 = (unsigned char)ucs2;
                game->player.name_cp437[i] = cp437;
                i++;
            }
            game->player.name_cp437[i] = 0;
            break;
        }

        default:
            break;
    }
}
