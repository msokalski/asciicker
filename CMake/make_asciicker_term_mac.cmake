#-----------
# ASCIICKER_TERM_MAC
#-----------

set(ASCIICKER_TERM_MAC_SOURCE
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

set(ASCIICKER_TERM_MAC_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -save-temps=obj -pthread -DGAME -DPURE_TERM -O3")

set(ASCIICKER_TERM_MAC_LINKER_FLAGS -lutil -pthread)

add_executable("asciicker_term_mac" ${ASCIICKER_TERM_MAC_SOURCE})

set_target_properties("asciicker_term_mac" PROPERTIES COMPILE_FLAGS ${ASCIICKER_TERM_MAC_COMPILE_FLAGS} LINK_OPTIONS "${ASCIICKER_TERM_MAC_LINKER_FLAGS}")