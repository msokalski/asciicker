# Fork of msokalski/asciiid

![GIF](misc/asciicker.gif)

## Installation
Firstly, you need to choose build type:
- Cmake (Every OS)
- MSBuild, aka Visual Studio (Windows only)
- Make (Every OS)

For windows it's recommended to use Visual Studio build system:
1) Install [Visual Studio](https://visualstudio.microsoft.com/)
2) ![Download repository](misc/github.png) and unzip it, the folder should be named `asciiid-sdl`, rename it to `asciiid` (Important!)
3) Open VisualStudio and click `Import project`, then navigate to the folder where you stored `asciiid` and select it 
4) Retarget solutions (this step may not be needed in some cases)
![Retarget](misc/VS_Retarget.png)
5) Install [SDL2](https://www.libsdl.org/download-2.0.php)


![SDL](misc/SDL.png)


5) Copy `include` and `lib` directories into `SDL\` directory in the root of the repository
6) Build asciiid
7) Copy `lib\(your_architecture_here)\SDL.dll` into `build` directory
8) Run `asciiid`!

For every other OS, it's recommended to use CMake:
1) Install [CMake](https://cmake.org/download/)
2) Install [git](https://git-scm.com/downloads)
3) Install prefered build system, main ones are: [Ninja](https://github.com/ninja-build/ninja/releases), [Make](https://www.gnu.org/software/make/), [MinGW-Make](https://sourceforge.net/projects/mingw/), [MSYS-Make](https://www.msys2.org/).
4) Open terminal and enter: `git clone https://github.com/Niki4tap/asciiid.git && cd asciiid && mkdir build && cd build`
5) Build:
```
Ninja:  cmake -G "Ninja" .. && ninja
Make:   cmake -G "Unix MakeFiles" && make
MinGW:  cmake -G "MinGW Makefiles" && mingw32-make
MSYS:   cmake -G "MSYS Makefiles" && make
```
6) Run `asciiid`!

MakeFiles:
1) Install [git](https://git-scm.com/downloads)
2) Install [Make](https://www.gnu.org/software/make/)
3) Open terminal and enter: `git clone https://github.com/Niki4tap/asciiid.git && cd asciiid && mkdir build && cd MakeFiles && make -f makefile_asciiid && make -f makefile_game && make -f makefile_game_term && make -f makefile_server && cd ../build`
4) Run `asciiid`!
