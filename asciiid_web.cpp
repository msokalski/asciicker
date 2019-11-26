
#include "terrain.h"
#include "physics.h"
#include "sprite.h"
#include "mesh.h"
#include "asciiid_render.h"

#include <time.h>
uint64_t GetTime()
{
	static timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}


Physics* phys = 0;
Terrain* terrain = 0;
World* world = 0;
Sprite* player_sprite = 0;
AnsiCell* render_buf = 0;

Material mat[256];
void* GetMaterialArr()
{
    return mat;
}

static const float water = 55.0f;
static const float zoom = 1.0;
static const float lt[4] = {1,0,1,.5};
static PhysicsIO io;

int main()
{
    float player_dir = 0;
    int player_stp = -1;
    float yaw = 45;
    float dir = 0;
    float pos[3] = {0,15,0};
    uint64_t stamp;

    player_sprite = LoadPlayer("./sprites/player.xp");
    if (!player_sprite)
    {
        printf("failed to load player.xp\n");
        return -1;
    }

    {
        FILE* f = fopen("a3d/game.a3d","rb");

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

                world = LoadWorld(f);
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
            printf("failed to open game.a3d\n");
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
            RebuildWorld(world, true);
    }

    render_buf = (AnsiCell*)malloc(sizeof(AnsiCell) * 160 * 160);
    if (!render_buf)
    {
        printf("failed to allocate render buffer\n");
        return -7;
    }

    stamp = GetTime();
    phys = CreatePhysics(terrain,world,pos,dir,yaw,stamp);
    if (!phys)
    {
        printf("failed to create physics\n");
        return -6;
    }

    io.jump = false;
    io.water = water;
    io.torque = 0;
    io.x_force = 0;
    io.y_force = 0;
    io.pos[0] = pos[0];
    io.pos[1] = pos[1];
    io.pos[2] = pos[2];
    io.yaw = yaw;
    io.player_dir = 0;
    io.player_stp = -1;

    printf("all ok\n");
    return 0;
}

extern "C"
{
    bool AsciickerUpdate(float x_force, float y_force, float torque, bool jump)
    {
        printf("In: AsciickerUpdate(%f,%f,%f,%s)\n", x_force, y_force, torque, jump?"true":"false");
        io.jump = jump;
        io.water = water;
        io.torque = torque;
        io.x_force = x_force;
        io.y_force = y_force;

        // return jump status
        uint64_t stamp = GetTime();
        Animate(phys, stamp, &io);
        return io.jump;
    }

    void* AsciickerRender(int width, int height)
    {
        printf("In: AsciickerRender(%d,%d)\n", width, height);
        Render(terrain,world,water,zoom,io.yaw,io.pos,lt, width,height, render_buf, io.player_dir, io.player_stp);
        return render_buf;
    }
}
