



# BASED ON:
# https://webassembly.org/getting-started/developers-guide/

# install emscripten:
# $ cd ~
# git clone https://github.com/emscripten-core/emsdk.git
# cd emsdk
# ./emsdk install latest
# ./emsdk activate latest

# BEFORE BUILDING setup env vars to terminal
# cd ~/emsdk
# source ./emsdk_env.sh --build=Release


# now we can build, 
# for the first time it will compile and cache libc

rm .web/index.html
rm .web/index.js
rm .web/index.wasm
rm .web/index.data

emcc --emrun -O3 \
    game_web.cpp \
    mesh.cpp \
    terrain.cpp \
    sprite.cpp \
    physics.cpp \
    render.cpp \
    upng.c \
    -o .web/index.html \
    --shell-file game_web.html \
    -s EXPORTED_FUNCTIONS='["_main","_AsciickerUpdate","_AsciickerRender","_AsciickerGetYaw"]' \
    -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    --preload-file a3d/game.a3d \
    --preload-file sprites/player-0011.xp \
    --preload-file sprites/attack-0011.xp \
	--preload-file sprites/inventory.xp \
    --preload-file meshes/bridge-1.ply \
    --preload-file meshes/fence.ply \
    --preload-file meshes/rock-1.ply \
    --preload-file meshes/bush-1.ply \
    --preload-file meshes/house-3.ply \
    --preload-file meshes/skull.ply \
    --preload-file meshes/tree-1.ply \
    --preload-file meshes/tree-2.ply \
    --preload-file meshes/ship-1.ply 

# run in mini-server
emrun --no_browser --port 8080 .web/index.html
