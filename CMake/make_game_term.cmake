#-----------
# GAME_TERM
#-----------

set(GAME_TERM_SOURCE
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

add_executable("game_term" ${GAME_TERM_SOURCE})

set_target_properties("game_term" PROPERTIES COMPILE_FLAGS "-DPURE_TERM")