
g++ -O3 -pthread \
    png2xp.cpp \
    ../rgba8.cpp \
    ../tinfl.c \
    ../upng.c \
    -o png2xp 