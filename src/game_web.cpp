
#include <emscripten.h>
#include <stdint.h>

#include "terrain.h"
#include "game.h"
#include "enemygen.h"
#include "sprite.h"
#include "world.h"
#include "render.h"

#include <time.h>

char base_path[1024] = "./"; // (const)


void Buzz()
{
    EM_ASM(
    {
        if (gamepad>=0 && "getGamePads" in navigator)
        {
            let gm = navigator.getGamepads()[gamepad];
            let va = gm.vibrationActuator;
            if (va)
            {
                va.playEffect(va.type, {startDelay: 0,  duration: 50,  weakMagnitude: 1,  strongMagnitude: 1});
            }
        }
        else
        if ("vibrate" in navigator) 
        {
            navigator.vibrate(50);
        }
    });
}

uint64_t GetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void SyncConf()
{
    EM_ASM( FS.syncfs( function(e) {} ); );
}

const char* GetConfPath()
{
    return "/data/asciicker.cfg";
}

Server* server = 0; // this is to fullfil game.cpp externs!

struct GameServer : Server
{
    uint8_t send_buf[1+256];
};

bool Server::Send(const uint8_t* data, int size)
{
    GameServer* gs = (GameServer*)server;
    if (size > 256)
        return false;
    gs->send_buf[0] = size;
    memcpy(gs->send_buf+1, data, size);
    int s = EM_ASM_INT( return Send(); );
	return s > 0;
}

void Server::Proc()
{
}

void Server::Log(const char* str)
{
    GameServer* gs = (GameServer*)server;
    int len = strlen(str);
    if (len>255)
        len=255;
    gs->send_buf[0] = len;
    memcpy(gs->send_buf+1,str,len);
    gs->send_buf[len+1] = 0;
    EM_ASM( ConsoleLog(); );
}

Game* game = 0;
Terrain* terrain = 0;
World* world = 0;
AnsiCell* render_buf = 0;

Material mat[256];
void* GetMaterialArr()
{
    return mat;
}

int user_zoom = 0;
bool PrevGLFont()
{
    user_zoom--;
    if (user_zoom<-8)
        user_zoom=-8;

    EM_ASM({user_zoom=$0;Resize(null);},user_zoom);
    return true;
}

bool NextGLFont()
{
    user_zoom++;
    if (user_zoom>8)
        user_zoom=8;

    EM_ASM({user_zoom=$0;Resize(null);},user_zoom);
    return true;
}

void exit_handler(int signum)
{
    EM_ASM(
    {
        try 
        { 
            if (window.history.length<=1)
                window.close();
            else
            if (history.state && history.state.inline == 1)
            {
                // we can't close, we cant go back
                // should we really go forward?

                // history.forward(); 
            }
            else
                history.back();
        }
        catch(e) {}
    });
}

void ToggleFullscreen(Game* g)
{
    EM_ASM(
    {
        let elem = document.body;

        if (!document.fullscreenElement) 
        {
            if ("requestFullscreen" in elem)
                elem.requestFullscreen().catch(err => { });
        } 
        else 
        {
            if ("exitFullscreen" in document)
                document.exitFullscreen();
        }
    });
}

bool IsFullscreen(Game* g)
{
    int fs = 
    EM_ASM_INT(
    {
        let elem = document.body;

        if (!document.fullscreenElement) 
        {
            return 0;
        } 
        else 
        {
            return 1;
        }
    });

    return fs!=0;
}


int main(int argc, char* argv[])
{
    return 0;
}

int Main()
{
    // here we must already know if serer on not server

    float water = 55.0f;
    float yaw = 45;
    float dir = 0;
    float pos[3] = {0,15,0};
    uint64_t stamp;

    LoadSprites();

    {
        FILE* f = fopen("a3d/game_map_y8.a3d","rb");

        if (f)
        {
            terrain = LoadTerrain(f);
            
            if (terrain)
            {
                for (int i=0; i<256; i++)
                {
                    if ( fread(mat[i].shade,1,sizeof(MatCell)*4*16,f) != sizeof(MatCell)*4*16 )
                        break;
                }

                world = LoadWorld(f,false);
                if (world)
                {
                    // reload meshes too
                    Mesh* m = GetFirstMesh(world);

                    while (m)
                    {
                        char mesh_name[256];
                        GetMeshName(m,mesh_name,256);
                        char obj_path[4096];
                        sprintf(obj_path,"%smeshes/%s",base_path,mesh_name);
                        if (!UpdateMesh(m,obj_path))
                        {
                            // what now?
                            // missing mesh file!
                            printf("failed to load mesh %s\n", mesh_name);
                            return -5;
                        }

                        m = GetNextMesh(m);
                    }

                    LoadEnemyGens(f);
                }
                else
                {
                    printf("failed to load world\n");
                    return -4;
                }
            }
            else
            {
                printf("failed to load terrain\n");
                return -3;
            }

            fclose(f);
        }
        else
        {
            printf("failed to open game_map_y7.a3d\n");
            return -2;
        }

        // if (!terrain || !world)
        //    return -1;

        // add meshes from library that aren't present in scene file
        char mesh_dirname[4096];
        sprintf(mesh_dirname,"%smeshes",base_path);
        //a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

        // this is the only case when instances has no valid bboxes yet
        // as meshes weren't present during their creation
        // now meshes are loaded ...
        // so we need to update instance boxes with (,true)

        if (world)
        {
            RebuildWorld(world, true);
            #ifdef DARK_TERRAIN
            float lt[4] = { 1,0,1,0.5 };
            UpdateTerrainDark(terrain, world, lt, false);
            #endif    
        }
    }

    render_buf = (AnsiCell*)malloc(sizeof(AnsiCell) * 160 * 160);
    if (!render_buf)
    {
        printf("failed to allocate render buffer\n");
        return -7;
    }

    stamp = GetTime();
    game = CreateGame(water,pos,yaw,dir,stamp);

    printf("all ok\n");
    return 0;
}

extern "C"
{
    void Load(const char* name)
    {
        if (name)
        {
            strcpy(player_name,name);
            ConvertToCP437(player_name_cp437,player_name);
        }

        Main();
    }

    void* Render(int width, int height)
    {
        if (game && render_buf)
        {
            game->Render(GetTime(),render_buf,width,height);
            return render_buf;
        }

        return 0;
    }

    void Size(int w, int h, int fw, int fh)
    {
        if (game)
            game->OnSize(w,h,fw,fh);
    }

    void Keyb(int type, int val)
    {
        if (game)
            game->OnKeyb((Game::GAME_KEYB)type,val);
    }

    void Mouse(int type, int x, int y)
    {
        if (game)
            game->OnMouse((Game::GAME_MOUSE)type, x, y);
    }

    void Touch(int type, int id, int x, int y)
    {
        if (game)
            game->OnTouch((Game::GAME_TOUCH)type, id, x, y);
    }

    void GamePad(int ev, int idx, float val)
    {
        static int gamepad_axes = 0;
        static int gamepad_buttons = 0;
        static uint8_t gamepad_mapping[256];

        switch (ev)
        {
            case 0:
            {
                if (!idx)
                    GamePadUnmount();
                else
                    GamePadMount("fixme",gamepad_axes,gamepad_buttons,gamepad_mapping);
                break;
            }
            case 1:
            {
                int16_t v = (int16_t)(val*32767);
                GamePadButton(idx,v);
                break;
            }
            case 2:
            {
                // works on chrome
                // int16_t v = val>1.0 ? (int16_t)-32768 : (int16_t)(val*32767);

                // maybe firefox too:
                int16_t v = val>1.0 || val<-1.0 ? (int16_t)-32768 : (int16_t)(val*32767);

                GamePadAxis(idx,v);
                break;
            }
            case 3:
            {
                // mapping size loword buttons, hiword axes
                gamepad_buttons = idx & 0xFFFF;
                gamepad_axes = (idx>>16) & 0xFFFF;
                break;
            }
            case 4:
            {
                // map button, byte0 val, byte1 button idx
                int map_idx = ((idx>>8) & 0xFF) + 2 * gamepad_axes; 
                gamepad_mapping[map_idx] = idx&0xFF;
                break;
            }
            case 5:
            {
                // map axis, byte0 neg, byte1 pos, byte2 axis idx
                int map_neg = 2*((idx>>16) & 0xFF); 
                int map_pos = map_neg+1;
                gamepad_mapping[map_neg] = idx&0xFF;
                gamepad_mapping[map_pos] = (idx>>8)&0xFF;
                break;
            }
        }
    }

    void Focus(int set)
    {
        if (game)
            game->OnFocus(set!=0);
    }

    void* Join(const char* name, int id, int max_cli)
    {
        if (id<0)
        {
            if (server)
            {
                free(server->others);
                free(server);
                server = 0;
            }
               return 0;
        }

        if (name)
        {
            strcpy(player_name,name);
            ConvertToCP437(player_name_cp437,player_name);
        }
        // alloc server, prepare for Packet()s
        GameServer* gs = (GameServer*)malloc(sizeof(GameServer));
        memset(gs,0,sizeof(GameServer));
        server = gs;
        server->others = (Human*)malloc(sizeof(Human)*max_cli);
        return gs->send_buf;
    }

    void Packet(const uint8_t* ptr, int size)
    {
        if (server)
        {
            bool ok = server->Proc(ptr, size);
        }
    }
}
