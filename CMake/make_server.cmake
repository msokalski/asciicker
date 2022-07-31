#-----------
# SERVER
#-----------

set(TARGET "asciicker_server")

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

set(SERVER_CXX_FLAGS	-std=c++17)
set(SERVER_CPP_FLAGS	-save-temps=obj -pthread)
set(SERVER_C_FLAGS		)
set(SERVER_LD_FLAGS		-save-temps=obj -pthread -lutil)

add_executable(${TARGET} ${SERVER_SOURCE})

target_compile_options(
	${TARGET} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${SERVER_CXX_FLAGS}>
	$<$<COMPILE_LANGUAGE:C,CXX>:${SERVER_CPP_FLAGS}>
	$<$<COMPILE_LANGUAGE:C>:${SERVER_C_FLAGS}>
)

target_compile_definitions(
	${TARGET} PRIVATE -DSERVER
)

target_link_options(
	${TARGET} PRIVATE ${SERVER_LD_FLAGS}
)