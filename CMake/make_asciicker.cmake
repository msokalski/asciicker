#-----------
# GAME
#-----------

set(GAME_SOURCE
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
	"src/gamepad.cpp"
	"src/font1.cpp"
)

add_executable("asciicker" ${GAME_SOURCE})

set_target_properties("asciicker" PROPERTIES COMPILE_FLAGS "-DGAME")