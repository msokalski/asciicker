mkdir .run
g++ -o .run/asciiid \
asciiid.cpp \
asciiid_x11.cpp \
asciiid_urdo.cpp \
terrain.cpp \
texheap.cpp \
gl.c \
imgui_impl_opengl3.cpp \
	-lGL \
	-lX11 \
	-L/usr/X11R6/lib \
	-I/usr/X11R6/include


