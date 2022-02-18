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
	"src/font1.cpp"
	"src/gamepad.cpp"
)

add_executable("asciicker_server" ${SERVER_SOURCE})

set_target_properties("asciicker_server" PROPERTIES COMPILE_FLAGS "-DSERVER")