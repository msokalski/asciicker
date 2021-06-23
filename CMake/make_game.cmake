#-----------
# GAME
#-----------

set(GAME_SOURCE
	"src/gl.c"
	"src/x11.cpp"
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

add_executable("game" ${GAME_SOURCE})

set_target_properties("game" PROPERTIES COMPILE_FLAGS "-DGAME")