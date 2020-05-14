



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

# AUTO SETUP (will be called everytime as all vars are set to this batch env only)
hash emcc 2>/dev/null || { pushd ~/emsdk; source ./emsdk_env.sh --build=Release; popd; }

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
    world.cpp \
    inventory.cpp \
    terrain.cpp \
    sprite.cpp \
    physics.cpp \
    render.cpp \
    upng.c \
    tinfl.c \
    -o .web/index.html \
    --shell-file game_web.html \
    -s EXPORTED_FUNCTIONS='["_main","_Load","_Render","_Size","_Keyb","_Mouse","_Touch","_Focus","_Join","_Packet"]' \
    -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s NO_EXIT_RUNTIME=1 \
    -lidbfs.js \
    --preload-file a3d/game_map.a3d \
    --preload-file sprites/grid-alpha-sword.xp \
    --preload-file sprites/grid-apple.xp \
    --preload-file sprites/grid-beet.xp \
    --preload-file sprites/grid-big-axe.xp \
    --preload-file sprites/grid-big-hammer.xp \
    --preload-file sprites/grid-big-mace.xp \
    --preload-file sprites/grid-blue-potion.xp \
    --preload-file sprites/grid-bread.xp \
    --preload-file sprites/grid-carrot.xp \
    --preload-file sprites/grid-cheese.xp \
    --preload-file sprites/grid-cherry.xp \
    --preload-file sprites/grid-crossbow.xp \
    --preload-file sprites/grid-cucumber.xp \
    --preload-file sprites/grid-cyan-potion.xp \
    --preload-file sprites/grid-cyan-ring.xp \
    --preload-file sprites/grid-egg.xp \
    --preload-file sprites/grid-flail.xp \
    --preload-file sprites/grid-gold-potion.xp \
    --preload-file sprites/grid-gold-ring.xp \
    --preload-file sprites/grid-green-potion.xp \
    --preload-file sprites/grid-grey-potion.xp \
    --preload-file sprites/grid-heavy-armor.xp \
    --preload-file sprites/grid-heavy-helmet.xp \
    --preload-file sprites/grid-heavy-shield.xp \
    --preload-file sprites/grid-light-armor.xp \
    --preload-file sprites/grid-light-helmet.xp \
    --preload-file sprites/grid-light-shield.xp \
    --preload-file sprites/grid-lumber-axe.xp \
    --preload-file sprites/grid-meat.xp \
    --preload-file sprites/grid-milk.xp \
    --preload-file sprites/grid-pink-potion.xp \
    --preload-file sprites/grid-pink-ring.xp \
    --preload-file sprites/grid-plum.xp \
    --preload-file sprites/grid-plus-sword.xp \
    --preload-file sprites/grid-red-potion.xp \
    --preload-file sprites/grid-small-mace.xp \
    --preload-file sprites/grid-small-saber.xp \
    --preload-file sprites/grid-small-sword.xp \
    --preload-file sprites/grid-water.xp \
    --preload-file sprites/grid-white-ring.xp \
    --preload-file sprites/grid-wine.xp \
    --preload-file sprites/item-apple.xp \
    --preload-file sprites/item-armor.xp \
    --preload-file sprites/item-axe.xp \
    --preload-file sprites/item-beet.xp \
    --preload-file sprites/item-blue-potion.xp \
    --preload-file sprites/item-bread.xp \
    --preload-file sprites/item-carrot.xp \
    --preload-file sprites/item-cheese.xp \
    --preload-file sprites/item-cherry.xp \
    --preload-file sprites/item-crossbow.xp \
    --preload-file sprites/item-cucumber.xp \
    --preload-file sprites/item-cyan-potion.xp \
    --preload-file sprites/item-cyan-ring.xp \
    --preload-file sprites/item-egg.xp \
    --preload-file sprites/item-flail.xp \
    --preload-file sprites/item-gold-potion.xp \
    --preload-file sprites/item-gold-ring.xp \
    --preload-file sprites/item-green-potion.xp \
    --preload-file sprites/item-grey-potion.xp \
    --preload-file sprites/item-hammer.xp \
    --preload-file sprites/item-helmet.xp \
    --preload-file sprites/item-mace.xp \
    --preload-file sprites/item-meat.xp \
    --preload-file sprites/item-milk.xp \
    --preload-file sprites/item-pink-potion.xp \
    --preload-file sprites/item-pink-ring.xp \
    --preload-file sprites/item-plum.xp \
    --preload-file sprites/item-red-potion.xp \
    --preload-file sprites/item-shield.xp \
    --preload-file sprites/item-sword.xp \
    --preload-file sprites/item-water.xp \
    --preload-file sprites/item-white-ring.xp \
    --preload-file sprites/item-wine.xp \
    --preload-file sprites/character.xp \
    --preload-file sprites/desert_plants.xp \
    --preload-file sprites/inventory.xp \
    --preload-file sprites/wolfie-0011.xp \
    --preload-file sprites/wolfie.xp \
    --preload-file sprites/player-nude.xp \
    --preload-file sprites/attack-0001.xp \
    --preload-file sprites/attack-0011.xp \
    --preload-file sprites/player-0000.xp \
    --preload-file sprites/player-0001.xp \
    --preload-file sprites/player-0010.xp \
    --preload-file sprites/player-0011.xp \
    --preload-file sprites/plydie-0000.xp \
    --preload-file sprites/plydie-0001.xp \
    --preload-file sprites/plydie-0010.xp \
    --preload-file sprites/plydie-0011.xp \
    --preload-file sprites/wolack-0001.xp \
    --preload-file sprites/wolack-0011.xp \
    --preload-file sprites/wolfie-0000.xp \
    --preload-file sprites/wolfie-0001.xp \
    --preload-file sprites/wolfie-0010.xp \
    --preload-file sprites/attack-0101.xp \
    --preload-file sprites/attack-0111.xp \
    --preload-file sprites/attack-1001.xp \
    --preload-file sprites/attack-1011.xp \
    --preload-file sprites/attack-1101.xp \
    --preload-file sprites/attack-1111.xp \
    --preload-file sprites/player-0100.xp \
    --preload-file sprites/player-0101.xp \
    --preload-file sprites/player-0110.xp \
    --preload-file sprites/player-0111.xp \
    --preload-file sprites/player-1000.xp \
    --preload-file sprites/player-1001.xp \
    --preload-file sprites/player-1010.xp \
    --preload-file sprites/player-1011.xp \
    --preload-file sprites/player-1100.xp \
    --preload-file sprites/player-1101.xp \
    --preload-file sprites/player-1110.xp \
    --preload-file sprites/player-1111.xp \
    --preload-file sprites/plydie-0100.xp \
    --preload-file sprites/plydie-0101.xp \
    --preload-file sprites/plydie-0110.xp \
    --preload-file sprites/plydie-0111.xp \
    --preload-file sprites/plydie-1000.xp \
    --preload-file sprites/plydie-1001.xp \
    --preload-file sprites/plydie-1010.xp \
    --preload-file sprites/plydie-1011.xp \
    --preload-file sprites/plydie-1100.xp \
    --preload-file sprites/plydie-1101.xp \
    --preload-file sprites/plydie-1110.xp \
    --preload-file sprites/plydie-1111.xp \
    --preload-file sprites/wolack-0101.xp \
    --preload-file sprites/wolack-0111.xp \
    --preload-file sprites/wolack-1001.xp \
    --preload-file sprites/wolack-1011.xp \
    --preload-file sprites/wolack-1101.xp \
    --preload-file sprites/wolack-1111.xp \
    --preload-file sprites/wolfie-0100.xp \
    --preload-file sprites/wolfie-0101.xp \
    --preload-file sprites/wolfie-0110.xp \
    --preload-file sprites/wolfie-0111.xp \
    --preload-file sprites/wolfie-1000.xp \
    --preload-file sprites/wolfie-1001.xp \
    --preload-file sprites/wolfie-1010.xp \
    --preload-file sprites/wolfie-1011.xp \
    --preload-file sprites/wolfie-1100.xp \
    --preload-file sprites/wolfie-1101.xp \
    --preload-file sprites/wolfie-1110.xp \
    --preload-file sprites/wolfie-1111.xp \
    --preload-file meshes/bridge-1.akm \
    --preload-file meshes/fence.akm \
    --preload-file meshes/rock-1.akm \
    --preload-file meshes/bush-1.akm \
    --preload-file meshes/house-3.akm \
    --preload-file meshes/skull.akm \
    --preload-file meshes/tree-1.akm \
    --preload-file meshes/tree-2.akm \
    --preload-file meshes/ship-1.akm \
    --preload-file meshes/laundry-1.akm \
    --preload-file meshes/bridge-2.akm \
    --preload-file meshes/old-tree-1.akm \
    --preload-file meshes/old-tree-2.akm

# run in mini-server

emrun --no_browser --port 8888 .web/index.html
