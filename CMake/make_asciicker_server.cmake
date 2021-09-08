#-----------
# SERVER
#-----------

set(SERVER_SOURCE
    "src/game_svr.cpp"
    "src/network.cpp"
    "src/sha1.c"
    "src/game.cpp"
    "src/enemygen.cpp"
    "src/render.cpp"
    "src/terrain.cpp"
    "src/world.cpp"
    "src/inventory.cpp"
    "src/physics.cpp"
    "src/sprite.cpp"
    "src/tinfl.c"
)

set(SERVER_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -save-temps=obj -pthread -DSERVER -O3")

set(SERVER_LINKER_FLAGS -lutil -pthread)

add_executable("asciicker_server" ${SERVER_SOURCE})

set_target_properties("asciicker_server" PROPERTIES COMPILE_FLAGS ${SERVER_COMPILE_FLAGS} LINK_OPTIONS "${SERVER_LINKER_FLAGS}")