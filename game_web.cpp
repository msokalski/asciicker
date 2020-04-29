
#include <emscripten.h>
#include <stdint.h>

#include "terrain.h"
#include "game.h"
#include "sprite.h"
#include "world.h"
#include "render.h"

#include <time.h>
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
        FILE* f = fopen("a3d/game_map.a3d","rb");

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
                        sprintf(obj_path,"%smeshes/%s","./"/*root_path*/,mesh_name);
                        if (!UpdateMesh(m,obj_path))
                        {
                            // what now?
                            // missing mesh file!
                            printf("failed to load mesh %s\n", mesh_name);
                            return -5;
                        }

                        m = GetNextMesh(m);
                    }
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
            printf("failed to open game_map.a3d\n");
            return -2;
        }

        // if (!terrain || !world)
        //    return -1;

        // add meshes from library that aren't present in scene file
        char mesh_dirname[4096];
        sprintf(mesh_dirname,"%smeshes","./"/*root_path*/);
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
            UpdateTerrainDark(terrain, world, lt);
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
    void Load()
    {
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
        strcpy(player_name,name);
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
