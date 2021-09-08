#-----------
# ASCIICKER
#-----------

set(ASCIICKER_SOURCE
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

set(ASCIICKER_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -save-temps=obj -pthread -DGAME -O3 -DUSE_SDL")

set(ASCIICKER_LINKER_FLAGS -lutil -lGL -lX11 -lXinerama -lgpm -lSDL2 -pthread)

add_executable("asciicker_sdl" ${ASCIICKER_SOURCE})

set_target_properties("asciicker_sdl" PROPERTIES COMPILE_FLAGS ${ASCIICKER_COMPILE_FLAGS} LINK_OPTIONS "${ASCIICKER_LINKER_FLAGS}")