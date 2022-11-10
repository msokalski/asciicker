#include <stdint.h>
#include <string.h>

#include "game_api.h"
#include "game.h"

void* akAPI_Buff = 0;

#define CODE(...) #__VA_ARGS__

void akAPI_Init()
{
    akAPI_Buff = malloc(AKAPI_BUF_SIZE);

    akAPI_Exec( CODE(
        this.akOnSay = null;
        function akAPI_OnSay() 
        { 
            if (akOnSay) 
            {
                akPrint("calling callback (not null)\n");
                akOnSay(akGetStr(0)); 
                akSetI32(1,0);
            }
            else
            {
                akPrint("not going to call callback (is null)\n");
                akSetI32(0,0);
            }
        }

        this.ak = 
        {
            getPos : function(arr3, ofs) { akAPI_Call(0); akReadF32(arr3,ofs|0,0,3); },
            setPos : function(arr3, ofs) { akWriteF32(arr3,ofs|0,0,3); akAPI_Call(1); },

            getDir : function() { akAPI_Call(2); return akGetF32(0); },
            setDir : function(flt) { akSetF32(Number(flt)||0,0); akAPI_Call(3); },

            getYaw : function() { akAPI_Call(4); return akGetF32(0); },
            setYaw : function(flt) { akSetF32(Number(flt)||0,0); akAPI_Call(5); },

            getName : function() { akAPI_Call(6); return akGetStr(0); },
            setName : function(str) { akSetStr(String(str),0); akAPI_Call(7); },

            getMount : function() { akAPI_Call(8); return akGetI32(0); },
            setMount : function(int) { akSetI32(Number(int)|0,0); akAPI_Call(9); },

            say : function(str) { akSetStr(String(str),0); akAPI_Call(101); },

            onSay: function(fnc) 
            {
                if (typeof fnc === 'function')
                {
                    akPrint("FNC PASSED\n");
                    akOnSay = fnc;
                }
                else
                {
                    akPrint("FNC IS NOT A FUNCTION\n");
                    akOnSay = null;
                }
            }
        };
    ),-1,true);
}

void akAPI_Free()
{
    free(akAPI_Buff);
}

extern "C" void akAPI_Call(int id)
{
    if (!game)
    {
        printf("game = NULL!\n");
        return;
    }

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
            strcpy((char*)akAPI_Buff,game->player.name);
            break;
        }

        case 7:
        // setName: function(str) { akSetStr(str,0); akAPI_Call(7); },
        {
            strcpy(game->player.name,(char*)akAPI_Buff);
            // TODO: convert utf8 to cp437!
            strncpy(game->player.name_cp437,(char*)akAPI_Buff,32);
            break;
        }

        case 8: 
        // getMount : function() { akAPI_Call(8); return akGetI32(0); },
        {
            *(int*)akAPI_Buff = game->player.req.mount;
            break;
        }

        case 9: 
        // setMount : function(int) { akSetI32(int,0); akAPI_Call(9); },
        {
            game->player.SetMount(*(int*)akAPI_Buff);
            break;
        }

        case 100:

        case 101: 
        // say : function(str) { akSetStr(str,0); akAPI_Call(101); },
        {
            // TODO: convert utf8 to cp437!
            const char* str = (char*)akAPI_Buff;
            int len = strlen(str);
            game->player.Say(str, len, game->stamp);
            break;
        }

        default:
            break;
    }
}
