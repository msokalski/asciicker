cmake_minimum_required(VERSION 3.16)   

project("asciicker_binaries") 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_GENERATOR)
	set(CMAKE_GENERATOR "Unix Makefiles")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER clang)
endif()

if(NOT MAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER clang++)
endif()

if(CMAKE_BUILD_TYPE EQUAL Release)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -save-temps=obj -pthread")

set(CMAKE_EXE_LINKER_FLAGS "-lutil -lGL -lX11 -lXinerama -lgpm")

include(CMake/make_asciiid.cmake)

include(CMake/make_game.cmake)

include(CMake/make_game_term.cmake)

include(CMake/make_server.cmake)