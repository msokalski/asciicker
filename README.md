# Asciicker

![GIF](misc/asciicker.gif)

### Important note about scripts and makefiles:
Scripts and makefiles must be ran from the root of the repository.

## Installation
Firstly, you need to choose build type:
- Cmake (Every OS)
- MSBuild, aka Visual Studio (Windows only)
- Make (Every OS)

### For Windows it's recommended to use Visual Studio build system:
1) Install [Visual Studio](https://visualstudio.microsoft.com/)
2) Download zip archive of the repository:


![Download](misc/github.png) 


and unzip it, the repository folder should be named `asciicker-master`.

3) Open VisualStudio and click `Import project`, then navigate to the folder where you stored `asciicker-master` and select it
4) Retarget solutions (this step may not be needed in some cases)


![Retarget](misc/VS_Retarget.png)


5) Install [SDL2](https://www.libsdl.org/download-2.0.php) 
(You need the `devel`opment package as shown in screenshot below)


![SDL](misc/SDL.png)


5) Copy `include` and `lib` directories from the SDL2 package you have downloaded 
into `SDL\` directory in the root of the repository
6) Build `asciicker`


![Build](misc/VS_Build.png)


7) Copy `lib\(your_architecture_here)\SDL2.dll` into `build` directory
8) Run `asciicker`!

For every other OS, it's recommended to use CMake:
1) Install [CMake](https://cmake.org/download/)
2) Install [git](https://git-scm.com/downloads)
3) Install preferred build system, main ones are: 
[Ninja](https://github.com/ninja-build/ninja/releases) (All),
[Make](https://www.gnu.org/software/make/) (Linux and macOS), 
[MinGW-Make](https://sourceforge.net/projects/mingw/) (Windows), 
[MSYS-Make](https://www.msys2.org/) (Windows).
4) Open terminal and enter: `git clone https://github.com/Niki4tap/asciicker.git && cd asciicker && mkdir build && cd build`
5) Build:
```
Ninja:  cmake -G "Ninja" .. && ninja
Make:   cmake -G "Unix MakeFiles" .. && make
MinGW:  cmake -G "MinGW Makefiles" .. && mingw32-make
MSYS:   cmake -G "MSYS Makefiles" .. && make
```
6) Run `asciicker`!

### Makefiles:
1) Install [git](https://git-scm.com/downloads)
2) Install [Make](https://www.gnu.org/software/make/)
3) Open terminal and enter: `git clone https://github.com/Niki4tap/asciicker.git && cd asciicker && mkdir build`
4) Build:
```
Linux: make -f makefile_asciiid && make -f makefile_asciicker && make -f makefile_asciicker_term && make -f makefile_asciicker_server && cd build
MacOS: make -f makefile_asciiid_mac && make -f makefile_asciicker_mac && make -f makefile_asciicker_term_mac && make -f makefile_asciicker_server && cd build
```
5) Run `asciicker`!

### Web:
0) When building for web it's recommended to use linux or macOS.
1) Install [emsdk](https://github.com/emscripten-core/emsdk)
2) Install [git](https://git-scm.com/downloads)
3) Open terminal and enter: `git clone https://github.com/Niki4tap/asciicker.git && cd asciicker && mkdir build && Scripts/build-web.sh`
