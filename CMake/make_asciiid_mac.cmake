#-----------
# ASCIIID_MAC
#-----------

set(TARGET "asciiid")

set(ASCIIID_MAC_SOURCE
	"src/asciiid.cpp"
	"src/term.cpp"
	"src/x11.cpp"
	"src/sdl.cpp"
	"src/urdo.cpp"
	"src/game.cpp"
	"src/enemygen.cpp"
	"src/render.cpp"
	"src/terrain.cpp"
	"src/world.cpp"
	"src/inventory.cpp"
	"src/physics.cpp"
	"src/sprite.cpp"
	"src/texheap.cpp"
	"src/rgba8.cpp"
	"src/upng.c"
	"src/tinfl.c"
	"src/gl.c"
	"src/gl45_emu.cpp"
	"src/imgui_impl_opengl3.cpp"
	"src/imgui/imgui.cpp"
	"src/imgui/imgui_demo.cpp"
	"src/imgui/imgui_draw.cpp"
	"src/imgui/imgui_widgets.cpp"
	"src/font1.cpp"
	"src/gamepad.cpp"
)

set(ASCIIID_MAC_CXX_FLAGS	-std=c++17 -g)
set(ASCIIID_MAC_CPP_FLAGS	-save-temps=obj -pthread)
set(ASCIIID_MAC_C_FLAGS		)
set(ASCIIID_MAC_LD_FLAGS	-save-temps=obj -pthread -lutil -framework OpenGL -lSDL2-2.0.0)

add_executable(${TARGET} ${ASCIIID_MAC_SOURCE})

target_compile_options(
	${TARGET} PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:${ASCIIID_MAC_CXX_FLAGS}>
	$<$<COMPILE_LANGUAGE:C,CXX>:${ASCIIID_MAC_CPP_FLAGS}>
	$<$<COMPILE_LANGUAGE:C>:${ASCIIID_MAC_C_FLAGS}>
)

target_compile_definitions(
	${TARGET} PRIVATE -DEDITOR -DUSE_SDL
)

target_link_options(
	${TARGET} PRIVATE ${ASCIIID_MAC_LD_FLAGS}
)