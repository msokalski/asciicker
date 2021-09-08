cmake_minimum_required(VERSION 3.16)   

option(COMPILE_SERVER "Compile server" ON)
option(COMPILE_GAME "Compile game" ON)
option(COMPILE_GAME_TERM "Compile game_term" ON)
option(COMPILE_ASCIIID "Compile asciiid" ON)

project("asciicker_binaries") 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_GENERATOR)
	set(CMAKE_GENERATOR "Unix Makefiles")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE EQUAL Release)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -save-temps=obj -pthread -DUSE_SDL")

set(CMAKE_EXE_LINKER_FLAGS "-lutil -lGL -lX11 -lXinerama -lgpm -lSDL2")

if(COMPILE_ASCIIID)
  include(CMake/make_asciiid.cmake)
endif()

if(COMPILE_GAME)
  include(CMake/make_asciicker.cmake)
endif()

if(COMPILE_GAME_TERM)
  include(CMake/make_asciicker_term.cmake)
endif()

if(COMPILE_SERVER)
  include(CMake/make_asciicker_server.cmake)
endif()