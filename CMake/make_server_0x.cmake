#-----------
# SERVER_0x
#-----------

set(TARGET "asciicker_server")

set(SERVER_0x_SOURCE
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

set(SERVER_0x_CXX_FLAGS	-std=gnu++0x)
set(SERVER_0x_CPP_FLAGS	-save-temps=obj -pthread)
set(SERVER_0x_C_FLAGS	)
set(SERVER_0x_LD_FLAGS	-save-temps=obj -pthread -lutil)

add_executable(${TARGET} ${SERVER_0x_SOURCE})

target_compile_options(
	${TARGET} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${SERVER_0x_CXX_FLAGS}>
	$<$<COMPILE_LANGUAGE:C,CXX>:${SERVER_0x_CPP_FLAGS}>
	$<$<COMPILE_LANGUAGE:C>:${SERVER_0x_C_FLAGS}>
)

target_compile_definitions(
	${TARGET} PRIVATE -DSERVER
)

target_link_options(
	${TARGET} PRIVATE ${SERVER_0x_LD_FLAGS}
)
