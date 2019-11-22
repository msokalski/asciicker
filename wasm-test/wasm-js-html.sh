



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

rm index.html
rm index.js
rm index.wasm

emcc --emrun test.cpp -o index.html


# run in mini-server
emrun --no_browser --port 8080 .
