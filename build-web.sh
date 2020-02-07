



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

cp asciicker.png .web/asciicker.png
cp asciicker.json .web/asciicker.json
cp asciicker.js .web/asciicker.js

emcc --emrun -O3 \
    game.cpp \
    game_web.cpp \
    mesh.cpp \
    terrain.cpp \
    sprite.cpp \
    physics.cpp \
    render.cpp \
    upng.c \
    tinfl.c \
    -o .web/index.html \
    --shell-file game_web.html \
    -s EXPORTED_FUNCTIONS='["_main","_Render","_Size","_Keyb","_Mouse","_Touch","_Focus"]' \
    -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    --preload-file a3d/game_sprites.a3d \
    --preload-file sprites/player-0000.xp \
    --preload-file sprites/wolfie-0011.xp \
	--preload-file sprites/plydie-0000.xp \
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
