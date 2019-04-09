if [ ! -d .run ]
then
	mkdir .run
fi
g++ -g -o .run/asciiid \
asciiid.cpp \
asciiid_x11.cpp \
asciiid_urdo.cpp \
terrain.cpp \
texheap.cpp \
rgba8.cpp \
upng.c \
gl.c \
imgui_impl_opengl3.cpp \
imgui/imgui.cpp \
imgui/imgui_demo.cpp \
imgui/imgui_draw.cpp \
imgui/imgui_widgets.cpp \
	-lGL \
	-lX11 
#	-L/usr/X11R6/lib \
#	-I/usr/X11R6/include
