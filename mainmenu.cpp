#include "mainmenu.h"
#include "enemygen.h"
#include "fast_rand.h"

extern Game* game;
extern Terrain* terrain;
extern World* world;
extern Material mat[256];
extern char base_path[1024];

static bool game_initialized = false;

static uint64_t mainmenu_stamp = 0;

static void RunGame()
{
    // JUST TESTING !!!!!
    /*
    if (game_initialized)
    {
        // let's test fresh restart
        FreeGame(game);
        game_initialized = false;
    }
    */

    if (!game_initialized)
    {
        /*
        if (terrain)
            DeleteTerrain(terrain);

        if (world)
            DeleteWorld(world);

        PurgeItemInstCache();
        */

        float water = 55;
        float dir = 0;
        float yaw = 45;
        float pos[3] = {0,15,0};
        float lt[4] = {1,0,1,.5};

        {
            char a3d_path[1024 + 20];
            sprintf(a3d_path,"%sa3d/game_map_y8.a3d", base_path);
            FILE* f = fopen(a3d_path, "rb");

            // TODO:
            // if GameServer* gs != 0
            // DO NOT LOAD ITEMS!
            // we will receive them from server

            if (f)
            {
                terrain = LoadTerrain(f);

                if (terrain)
                {
                    for (int i = 0; i < 256; i++)
                    {
                        if (fread(mat[i].shade, 1, sizeof(MatCell) * 4 * 16, f) != sizeof(MatCell) * 4 * 16)
                            break;
                    }

                    world = LoadWorld(f, false);
                    if (world)
                    {
                        // reload meshes too
                        Mesh* m = GetFirstMesh(world);

                        while (m)
                        {
                            char mesh_name[256];
                            GetMeshName(m, mesh_name, 256);
                            char obj_path[4096];
                            sprintf(obj_path, "%smeshes/%s", base_path, mesh_name);
                            if (!UpdateMesh(m, obj_path))
                            {
                                // what now?
                                // missing mesh file!
                            }

                            m = GetNextMesh(m);
                        }

                        LoadEnemyGens(f);
                    }
                }

                fclose(f);
            }

            // if (!terrain || !world)
            //    return -1;

            // add meshes from library that aren't present in scene file
            char mesh_dirname[4096];
            sprintf(mesh_dirname, "%smeshes", base_path);
            //a3dListDir(mesh_dirname, MeshScan, mesh_dirname);

            // this is the only case when instances has no valid bboxes yet
            // as meshes weren't present during their creation
            // now meshes are loaded ...
            // so we need to update instance boxes with (,true)

            if (world)
            {
                RebuildWorld(world, true);
                #ifdef DARK_TERRAIN
                // TODO: add Patch Iterator holding stack of parents
                // and single Patch dark updater
                UpdateTerrainDark(terrain, world, lt, false);
                #endif
            }
        }

        InitGame(game, water, pos, yaw, dir, mainmenu_stamp);
        game_initialized = true;
    }

    game->main_menu = false;
}

void MainMenu_Render(uint64_t _stamp, AnsiCell* ptr, int width, int height)
{
    mainmenu_stamp = _stamp;
    int s = width*height;
    for (int i=0; i<s; i++)
        *((uint32_t*)ptr+i) = fast_rand() | (fast_rand()<<15) | (fast_rand()<<30);
}

void MainMenu_OnSize(int w, int h, int fw, int fh)
{
}

void MainMenu_OnKeyb(GAME_KEYB keyb, int key)
{
    if (keyb == GAME_KEYB::KEYB_CHAR)
    {
        RunGame();
    }
}

void MainMenu_OnMouse(GAME_MOUSE mouse, int x, int y)
{
    if (mouse == GAME_MOUSE::MOUSE_LEFT_BUT_DOWN ||
        mouse == GAME_MOUSE::MOUSE_RIGHT_BUT_DOWN ||
        mouse == GAME_MOUSE::MOUSE_MIDDLE_BUT_DOWN)
    {
        RunGame();
    }
}

void MainMenu_OnTouch(GAME_TOUCH touch, int id, int x, int y)
{
    if (touch == GAME_TOUCH::TOUCH_BEGIN)
    {
        RunGame();
    }
}

void MainMenu_OnFocus(bool set)
{
}

void MainMenu_OnPadMount(bool connect)
{
}

void MainMenu_OnPadButton(int b, bool down)
{
    RunGame();
}

void MainMenu_OnPadAxis(int a, int16_t pos)
{
}
