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

        this.akAPI_Back = Array(256);
        let cb = function(idx,fnc) 
        { 
            fnc = typeof fnc === 'function' ? fnc : null;
            akAPI_Back[idx] = fnc;

            // last 256 bits of api buffer contains
            // flags set if given cb is active
            let adr = akAPI_Buff+65536+(idx>>3);
            let flg = Module.HEAPU8[adr];
            let bit = 1<<(idx&0x7);
            flg = fnc ? flg|bit : flg&~bit; 
            Module.HEAPU8[adr] = flg;
        };

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

            getWater : function() { akAPI_Call(14); return akGetF32(0); },
            setWater : function(flt) { akSetF32(Number(flt)||0,0); akAPI_Call(15); },

            getLight : function(arr4, ofs) { akAPI_Call(16); akReadF32(arr4,ofs|0,0,4); },
            setLight : function(arr4, ofs) { akWriteF32(arr4,ofs|0,0,4); akAPI_Call(17); },

            //////////////////////////////////////////////////////////////////////

            say  : function(str) { akSetStr(String(str),0); akAPI_Call(100); },
            jump : function()    { akAPI_Call(101); },

            //////////////////////////////////////////////////////////////////////

            isGrounded : function() { akAPI_Call(200); return akGetI32(0)!=0; },

            //////////////////////////////////////////////////////////////////////
            onSay: function(fnc) { cb(0,fnc); },
            onItem: function(fnc) { cb(1,fnc); },
            onFrame: function(fnc) { cb(2,fnc); }
        };
       
        Object.freeze(ak);

        this.akAPI_CB = function(id)
        {
            let fnc = akAPI_Back[id];
            let ret,t;
            switch(id)
            {
                case 0: // onSay(str) -> bool
                    let str = akGetStr(0);
                    ret = fnc.apply(akAPI_This,[str]);

                    t = typeof ret;

                    if (t == 'boolean')
                        akSetI32(ret?1:0,0);
                    else
                        akSetI32(1,0);
                    break;

                case 1: // onItem(action,story,kind,subkind,weight,desc) -> bool/int/str/[int,str]/{story_id,desc}
                    let story_id = akGetI32(1);
                    let desc = akGetStr(20);
                    ret = fnc.apply(akAPI_This,[
                        akGetI32(0), story_id, akGetI32(2),
                        akGetI32(3), akGetI32(4), desc]);
                    
                    t = typeof ret;
                    if (t == 'boolean')
                    {
                        akSetI32(ret?1:0,0);
                        if (!ret)
                            break;
                    }

                    akSetI32(1,0);

                    if (t == 'number')
                    {
                        akSetI32(ret|0,4);
                        akSetStr(desc,8);
                    }
                    else
                    if (t == 'string')
                    {
                        akSetI32(story_id,4);
                        akSetStr(ret,8);
                    }
                    else
                    if (t == 'array')
                    {
                        if (typeof ret[0] == 'number')
                            akSetI32(ret[0]|4);
                        else
                            akSetI32(story_id,4);

                        if (typeof ret[1] == 'string')
                            akSetStr(ret[1],8);
                        else
                            akSetStr(desc,8);
                    }
                    else
                    if (t == 'object')
                    {
                        if (typeof ret.story_id == 'number')
                            akSetI32(ret.story_id|4);
                        else
                            akSetI32(story_id,4);

                        if (typeof ret.desc == 'string')
                            akSetStr(ret.desc,8);
                        else
                            akSetStr(desc,8);
                    }
                    else
                    {
                        akSetI32(story_id,4);
                        akSetStr(desc,8);
                    }
                    break;

                case 2:
                // onFrame()  
                {
                    ret = fnc.apply(akAPI_This);            
                    break;
                }
            }
        };

    }),-1,true);
}

bool akAPI_CheckCB(int id)
{
    int bit = 1<<(id&0x7);
    uint8_t* ptr = (uint8_t*)akAPI_Buff + 65536 + (id>>3);
    return (*ptr & bit) != 0;
}

bool akAPI_OnSay(const char* str, int len, 
                 bool* allowed)
{
    const int id = 0;
    if (!akAPI_CheckCB(id))
        return false;

    if (len<0)
        len=strlen(str);

    if (len>255)
        len=255;

    memcpy((char*)akAPI_Buff,str,len);
    ((char*)akAPI_Buff)[len] = 0;
    
    akAPI_CB(id);

    if (allowed)
        *allowed = *(int*)akAPI_Buff != 0;

    return true; 
}

bool akAPI_OnItem(int action, int story_id, int kind, int subkind, int weight, const char* desc,
                  bool* allowed, int* out_story_id, const char** out_desc)
{
    const int id = 1;
    if (!akAPI_CheckCB(id))
        return false;

    int* ptr = (int*)akAPI_Buff;
    ptr[0] = action;
    ptr[1] = story_id;
    ptr[2] = kind;
    ptr[3] = subkind;
    ptr[4] = weight;

    int len=strlen(desc);
    if (len>31)
        len=31;

    memcpy((char*)akAPI_Buff+20,desc,len);
    ((char*)akAPI_Buff+20)[len] = 0;

    akAPI_CB(id);    

    if (allowed)
        *allowed = *(int*)akAPI_Buff != 0;
    if (out_story_id)
        *out_story_id = *(int*)akAPI_Buff ? *((int*)akAPI_Buff+4) : story_id;
    if (out_desc)
        *out_desc = *(int*)akAPI_Buff ? (char*)akAPI_Buff+8 : desc;

    return true;
}

bool akAPI_OnFrame()
{
    const int id = 2;
    if (!akAPI_CheckCB(id))
        return false;

    akAPI_CB(id);    
    return true;
}

void akAPI_Free()
{
    // allocated by platform specific thing
    // free(akAPI_Buff);
}

extern "C" void akAPI_Call(int id)
{
    if (!game || game->main_menu)
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

        case 14: 
        // getWater : function() { akAPI_Call(13); return akGetF32(0); },
        {
            *(float*)akAPI_Buff = game->water;
            break;
        }
        case 15:
        // setWater : function(flt) { akSetF32(Number(flt)||0,0); akAPI_Call(14); },
        {
            game->water = *(float*)akAPI_Buff,0;
            break;
        }

        case 16: 
        // getLight : function(arr4, ofs) { akAPI_Call(15); akReadF32(arr4,ofs|0,0,4); },
        {
            memcpy(akAPI_Buff, game->light, sizeof(float[4]));
            break;
        }
        case 17: 
        // setLight : function(arr4, ofs) { akWriteF32(arr4,ofs|0,0,4); akAPI_Call(16); },
        {
            memcpy(game->light, akAPI_Buff, sizeof(float[4]));
            break;
        }

        ////////////////////////////////////////////////////////////////////////

        case 100: 
        // say : function(str) { akSetStr(str,0); akAPI_Call(100); },
        {
            //char* str = (char*)akAPI_Buff + 256;
            //ConvertToCP437(str, (char*)akAPI_Buff, 256);
            //((char*)akAPI_Buff)[256-1] = 0;

            char* str = (char*)akAPI_Buff;

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
