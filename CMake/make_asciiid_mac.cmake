#-----------
# ASCIIID_MAC
#-----------

set(ASCIID_MAC_SOURCE
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
)

set(ASCIIID_MAC_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -save-temps=obj -pthread -DEDITOR -O3 -DUSE_SDL")

set(ASCIIID_MAC_LINKER_FLAGS -lutil -framework OpenGL -lXinerama -lSDL2-2.0.0 -pthread)

add_executable("asciiid_mac" ${ASCIID_MAC_SOURCE})

set_target_properties("asciiid_mac" PROPERTIES COMPILE_FLAGS ${ASCIIID_MAC_COMPILE_FLAGS} LINK_OPTIONS "${ASCIIID_MAC_LINKER_FLAGS}")