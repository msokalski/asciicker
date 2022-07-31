#-----------
# ASCIICKER_MAC
#-----------

set(TARGET "asciicker")

set(ASCIICKER_MAC_SOURCE
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

set(ASCIICKER_MAC_CXX_FLAGS	-std=c++17)
set(ASCIICKER_MAC_CPP_FLAGS	-g -save-temps=obj -pthread)
set(ASCIICKER_MAC_C_FLAGS	)
set(ASCIICKER_MAC_LD_FLAGS	-save-temps=obj -pthread -lutil -framework OpenGL -lXinerama -lSDL2-2.0.0)

add_executable(${TARGET} ${ASCIICKER_MAC_SOURCE})

target_compile_options(
	${TARGET} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${ASCIICKER_MAC_CXX_FLAGS}>
	$<$<COMPILE_LANGUAGE:C,CXX>:${ASCIICKER_MAC_CPP_FLAGS}>
	$<$<COMPILE_LANGUAGE:C>:${ASCIICKER_MAC_C_FLAGS}>
)

target_compile_definitions(
	${TARGET} PRIVATE -DGAME -DUSE_SDL
)

target_link_options(
	${TARGET} PRIVATE ${ASCIICKER_MAC_LD_FLAGS}
)