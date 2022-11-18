#include <stdint.h>
#include <string.h>

#include "game_api.h"
#include "game.h"

void* akAPI_Buff = 0;

#define CODE(...) #__VA_ARGS__

void akAPI_Init()
{
    akAPI_Exec( CODE(
    {
        this.akGetF32 = function(buf_ofs)
        {
            buf_ofs += akAPI_Buff>>2;
            return Module.HEAPF32[buf_ofs];
        };
        this.akSetF32 = function(val, buf_ofs)
        {
            buf_ofs += akAPI_Buff>>2;
            Module.HEAPF32[buf_ofs] = val;
        };
        this.akReadF32 = function(arr,arr_ofs,buf_ofs,num)
        {
            buf_ofs += akAPI_Buff>>2;
            for (let i=0; i<num; i++)
                arr[i+arr_ofs] = Module.HEAPF32[i+buf_ofs];
        };
        this.akWriteF32 = function(arr,arr_ofs,buf_ofs,num)
        {
            buf_ofs += akAPI_Buff>>2;
            for (let i=0; i<num; i++)
                Module.HEAPF32[i+buf_ofs] = arr[i+arr_ofs];
        }; 
        this.akGetI32 = function(buf_ofs)
        {
            buf_ofs += akAPI_Buff>>2;
            return Module.HEAP32[buf_ofs];
        };
        this.akSetI32 = function(val, buf_ofs)
        {
            buf_ofs += akAPI_Buff>>2;
            Module.HEAP32[buf_ofs] = val;
        };
        this.akReadI32 = function(arr,arr_ofs,buf_ofs,num)
        {
            buf_ofs += akAPI_Buff>>2;
            for (let i=0; i<num; i++)
                arr[i+arr_ofs] = Module.HEAP32[i+buf_ofs];
        };
        this.akWriteI32 = function(arr,arr_ofs,buf_ofs,num)
        {
            buf_ofs += akAPI_Buff>>2;
            for (let i=0; i<num; i++)
                Module.HEAP32[i+buf_ofs] = arr[i+arr_ofs];
        };
        this.akGetStr = function(buf_ofs)
        {
            return UTF8ToString(akAPI_Buff+buf_ofs,0xFFFF-buf_ofs);
        };
        this.akSetStr = function(str,buf_ofs)
        {
            stringToUTF8(str,akAPI_Buff+buf_ofs,0xFFFF-buf_ofs);
        };

        this.akAPI_Back = [];
        let cb = function(fnc) { return (typeof fnc === 'function') ? fnc : null; };


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

            getAction : function() { akAPI_Call(10); return akGetI32(0); },
            setAction : function(int) { akSetI32(Number(int)|0,0); akAPI_Call(11); },

            getMove : function(arr3, ofs) { akAPI_Call(12); akReadF32(arr3,ofs|0,0,3); },
            setMove : function(arr3, ofs) { akWriteF32(arr3,ofs|0,0,3); akAPI_Call(13); },

            //////////////////////////////////////////////////////////////////////

            say  : function(str) { akSetStr(String(str),0); akAPI_Call(100); },
            jump : function()    { akAPI_Call(101); },

            //////////////////////////////////////////////////////////////////////

            isGrounded : function() { akAPI_Call(200); return akGetI32(0)!=0; },

            //////////////////////////////////////////////////////////////////////
            onSay: function(fnc) { akAPI_Back[0] = cb(fnc); },
            onItem: function(fnc) { akAPI_Back[1] = cb(fnc); }
        };
       
        Object.freeze(ak);


        this.akAPI_OnSay = function () 
        { 
            let f = akAPI_Back[0];
            if (f) 
            {
                f(akGetStr(0)); 
                akSetI32(1,0);
            }
            akSetI32(0,0);
        };

        this.akAPI_OnItem = function () 
        {
            let f = akAPI_Back[1];
            if (f) 
            {

                let str = f(akGetStr(0));
                if (typeof str === 'string')
                {
                    akSetI32(0,2);
                    akSetStr(str,4);
                }
                else
                    akSetI32(0,1);
            }
            akSetI32(0,0);
        };


    }),-1,true);
}

void akAPI_Free()
{
    // allocated by platform specific thing
    // free(akAPI_Buff);
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
            // do it smooth?
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
            strncpy(game->player.name,(char*)akAPI_Buff,128);
            ConvertToCP437(game->player.name_cp437,(char*)akAPI_Buff,32);
            game->player.name_cp437[31] = 0;
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

        case 10:
        // getAction : function() { akAPI_Call(10); return akGetI32(0); }.
        {
            *(int*)akAPI_Buff = game->player.req.action;
            break;
        }

        case 11:
        // setAction : function(int) { akSetI32(Number(int)|0,0); akAPI_Call(11); },
        {
            uint64_t stamp = game->stamp;
            switch (*(int*)akAPI_Buff)
            {
                case ACTION::NONE:   game->player.SetActionNone(stamp);   break;
                case ACTION::ATTACK: game->player.SetActionAttack(stamp); break;
                case ACTION::FALL:   game->player.SetActionFall(stamp);   break;
                case ACTION::DEAD:   game->player.SetActionDead(stamp);   break;
                case ACTION::STAND:  game->player.SetActionStand(stamp);  break;
            }
            break;
        }

        case 12: 
        // getMove: function(arr3, ofs) { akAPI_Call(12); akReadF32(arr3,ofs|0,0,3); }
        {
            memcpy(akAPI_Buff, game->input.api_move, sizeof(float[3]));
            break;
        }
        case 13: 
        // setMove: function(arr3, ofs) { akWriteF32(arr3,ofs|0,0,3); akAPI_Call(13); }
        {
            memcpy(game->input.api_move, akAPI_Buff, sizeof(float[3]));
            break;
        }


        ////////////////////////////////////////////////////////////////////////

        case 100: 
        // say : function(str) { akSetStr(str,0); akAPI_Call(100); },
        {
            char* str = (char*)akAPI_Buff + AKAPI_BUF_SIZE/2;
            ConvertToCP437(str, (char*)akAPI_Buff, AKAPI_BUF_SIZE/2);
            ((char*)akAPI_Buff)[AKAPI_BUF_SIZE-1] = 0;

            int len = strlen(str);
            game->player.Say(str, len, game->stamp);
            break;
        }

        case 101: 
        // jump : function() { akAPI_Call(101); },
        {
            game->input.jump = true;
            break;
        }

        ////////////////////////////////////////////////////////////////////////

        case 200: 
        // isGrounded : function() { akAPI_Call(200); return akGetI32(0)!=0; },
        {
            *(int*)akAPI_Buff = game->prev_grounded ? 1:0;
            break;
        }        

        default:
            break;
    }
}
