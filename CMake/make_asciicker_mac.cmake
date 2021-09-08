#-----------
# ASCIICKER_MAC
#-----------

set(ASCIICKER_SOURCE_MAC
	"src/gl.c"
	"src/gl45_emu.cpp"
	"src/x11.cpp"
	"src/sdl.cpp"
	"src/term.cpp"
	"src/game.cpp"
	"src/enemygen.cpp"
	"src/game_app.cpp"
	"src/network.cpp"
	"src/render.cpp"
	"src/terrain.cpp"
	"src/world.cpp"
	"src/inventory.cpp"
	"src/physics.cpp"
	"src/sprite.cpp"
	"src/upng.c"
	"src/tinfl.c"
	"src/rgba8.cpp"
)

set(ASCIICKER_MAC_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -save-temps=obj -pthread -DGAME -O3 -DUSE_SDL")

set(ASCIICKER_MAC_LINKER_FLAGS -lutil -framework OpenGL -lXinerama -lSDL2-2.0.0 -pthread)

add_executable("asciicker_sdl_mac" ${ASCIICKER_SOURCE_MAC})

set_target_properties("asciicker_sdl_mac" PROPERTIES COMPILE_FLAGS ${ASCIICKER_MAC_COMPILE_FLAGS} LINK_OPTIONS "${ASCIICKER_MAC_LINKER_FLAGS}")